#include <furi.h>
#include <gui/gui.h>
#include <gui/gui_i.h>
#include "furi_hal_usb_cdc.h"
#include <input/input.h>

#define TAG "CDC screen"

static void cdc_tx_cb(uint8_t itf, void* context) {
    UNUSED(itf);
    UNUSED(context);
    // FURI_LOG_I(TAG, "tx_cb");
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
            InputKey key = (data & 0b01111111);
            if(data & 0b10000000) {
                input_key_press(key);
            } else {
                input_key_release(key);
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
