#include <furi.h>
#include <gui/gui.h>
#include <gui/gui_i.h>
#include "furi_hal_usb_cdc.h"
#include "input_i.h"

#define TAG "CDC screen"

static Input* input = NULL;

static void cdc_tx_cb(uint8_t itf, void* context) {
    UNUSED(itf);
    UNUSED(context);
    // FURI_LOG_I(TAG, "tx_cb");
}

static uint8_t key_state = 0;

static bool key_state_read(uint8_t key) {
    return key_state & (1 << key);
}

static void cdc_rx_cb(uint8_t itf, void* context) {
    UNUSED(itf);
    UNUSED(context);
    // FURI_LOG_I(TAG, "rx_cb");

    uint8_t data = 0;
    size_t size = 0;

    do {
        size = furi_hal_cdc_read(0, &data, 1);
        if(size > 0) {
            uint8_t tmp_state = key_state;

            if(data & 0b10000000) {
                data = data & 0b01111111;
                tmp_state |= 1 << data;
            } else {
                data = data & 0b01111111;
                tmp_state &= ~(1 << data);
            }

            if(tmp_state != key_state) {
                key_state = tmp_state;

                if(input) {
                    furi_thread_flags_set(input->thread_id, INPUT_THREAD_FLAG_ISR);
                }
            }
        }
    } while(size > 0);
}

static void cdc_ctr_line(uint8_t itf, bool dtr, bool rts, void* context) {
    UNUSED(itf);
    UNUSED(context);
    FURI_LOG_I(TAG, "ctr_line dtr:%u rts:%u", dtr, rts);
}

static const char parity_name[] = {'N', 'O', 'E', 'M', 'S'};
static const char* stop_name[] = {"1", "1.5", "2"};

static void cdc_config(uint8_t itf, CdcLineCoding* config, void* context) {
    UNUSED(itf);
    UNUSED(context);
    furi_assert(config->parity <= 4);
    furi_assert(config->stop_bits <= 2);
    FURI_LOG_I(
        TAG,
        "config %lu/%u%c%s",
        config->bit_rate,
        config->data_bits,
        parity_name[config->parity],
        stop_name[config->stop_bits]);
}

static const CdcCallbacks cdc_cb = {
    .rx_callback = cdc_rx_cb,
    .tx_done_callback = cdc_tx_cb,
    .ctrl_line_callback = cdc_ctr_line,
    .config_callback = cdc_config,
};

static void cdc_write(const uint8_t* buf, uint32_t count) {
    uint32_t written = 0;
    while(written < count) {
        written += furi_hal_cdc_write(0, buf + written, count - written);
    }
}

static void commit_cdc(uint8_t* data, size_t size, CanvasOrientation orientation, void* context) {
    UNUSED(orientation);
    uint32_t* frame_count = (uint32_t*)context;

    const uint8_t magic[] = {0x55, 0x14, 0x69, 0x88};
    const uint8_t suffix[] = {0x00, 0x00, 0xFF, 0xF0};
    cdc_write(magic, sizeof(magic));
    cdc_write((const uint8_t*)frame_count, sizeof(uint32_t));
    cdc_write((const uint8_t*)data, size);
    cdc_write(suffix, sizeof(suffix));

    *frame_count += 1;
}

int32_t cdc_screen(void* p) {
    UNUSED(p);

    CdcContext cdc_cfg = {
        .callbacks = &cdc_cb,
        .context = NULL,
    };

    FURI_LOG_I(TAG, "Started");
    furi_hal_usb_set_config(&usb_cdc, &cdc_cfg);
    uint32_t frame_count = 0;

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_framebuffer_callback(gui, commit_cdc, &frame_count);

    while(true) {
        gui_update(gui);
        furi_delay_ms(1000);
    }

    gui_remove_framebuffer_callback(gui, commit_cdc, &frame_count);
    furi_hal_usb_set_config(NULL, NULL);
    furi_record_close(RECORD_GUI);
    return 0;
}

static void input_press_timer_callback(void* arg) {
    InputPinState* input_pin = arg;
    InputEvent event;
    event.sequence_source = INPUT_SEQUENCE_SOURCE_HARDWARE;
    event.sequence_counter = input_pin->counter;
    event.key = input_pin->key;
    input_pin->press_counter++;
    if(input_pin->press_counter == INPUT_LONG_PRESS_COUNTS) {
        event.type = InputTypeLong;
        furi_pubsub_publish(input->event_pubsub, &event);
    } else if(input_pin->press_counter > INPUT_LONG_PRESS_COUNTS) {
        input_pin->press_counter--;
        event.type = InputTypeRepeat;
        furi_pubsub_publish(input->event_pubsub, &event);
    }
}

int32_t input_srv(void* p) {
    UNUSED(p);
    input = malloc(sizeof(Input));
    input->thread_id = furi_thread_get_current_id();
    input->event_pubsub = furi_pubsub_alloc();
    furi_record_create(RECORD_INPUT_EVENTS, input->event_pubsub);

#if INPUT_DEBUG
    furi_hal_gpio_init_simple(&gpio_ext_pa4, GpioModeOutputPushPull);
#endif

#ifdef SRV_CLI
    input->cli = furi_record_open(RECORD_CLI);
    cli_add_command(input->cli, "input", CliCommandFlagParallelSafe, input_cli, input);
#endif

    input->pin_states = malloc(InputKeyMAX * sizeof(InputPinState));

    for(size_t i = 0; i < InputKeyMAX; i++) {
        // furi_hal_gpio_add_int_callback(input_pins[i].gpio, input_isr, NULL);
        // input->pin_states[i].pin = &input_pins[i];
        input->pin_states[i].key = i;
        input->pin_states[i].state = key_state_read(i);
        input->pin_states[i].debounce = INPUT_DEBOUNCE_TICKS_HALF;
        input->pin_states[i].press_timer = furi_timer_alloc(
            input_press_timer_callback, FuriTimerTypePeriodic, &input->pin_states[i]);
        input->pin_states[i].press_counter = 0;
    }

    while(1) {
        bool is_changing = false;
        for(size_t i = 0; i < InputKeyMAX; i++) {
            bool state = key_state_read(i);
            if(state) {
                if(input->pin_states[i].debounce < INPUT_DEBOUNCE_TICKS)
                    input->pin_states[i].debounce += 1;
            } else {
                if(input->pin_states[i].debounce > 0) input->pin_states[i].debounce -= 1;
            }

            if(input->pin_states[i].debounce > 0 &&
               input->pin_states[i].debounce < INPUT_DEBOUNCE_TICKS) {
                is_changing = true;
            } else if(input->pin_states[i].state != state) {
                input->pin_states[i].state = state;

                // Common state info
                InputEvent event;
                event.sequence_source = INPUT_SEQUENCE_SOURCE_HARDWARE;
                event.key = i;

                // Short / Long / Repeat timer routine
                if(state) {
                    input->counter++;
                    input->pin_states[i].counter = input->counter;
                    event.sequence_counter = input->pin_states[i].counter;
                    furi_timer_start(input->pin_states[i].press_timer, INPUT_PRESS_TICKS);
                } else {
                    event.sequence_counter = input->pin_states[i].counter;
                    furi_timer_stop(input->pin_states[i].press_timer);
                    while(furi_timer_is_running(input->pin_states[i].press_timer))
                        furi_delay_tick(1);
                    if(input->pin_states[i].press_counter < INPUT_LONG_PRESS_COUNTS) {
                        event.type = InputTypeShort;
                        furi_pubsub_publish(input->event_pubsub, &event);
                    }
                    input->pin_states[i].press_counter = 0;
                }

                // Send Press/Release event
                event.type = input->pin_states[i].state ? InputTypePress : InputTypeRelease;
                furi_pubsub_publish(input->event_pubsub, &event);
            }
        }

        if(is_changing) {
#if INPUT_DEBUG
            furi_hal_gpio_write(&gpio_ext_pa4, 1);
#endif
            furi_delay_tick(1);
        } else {
#if INPUT_DEBUG
            furi_hal_gpio_write(&gpio_ext_pa4, 0);
#endif
            furi_thread_flags_wait(INPUT_THREAD_FLAG_ISR, FuriFlagWaitAny, FuriWaitForever);
        }
    }

    return 0;
}