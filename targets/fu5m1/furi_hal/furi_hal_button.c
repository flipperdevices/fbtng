#include "furi_hal_button.h"
#include "furi_hal.h"
#include "furi_hal_half_lpuart.h"

#include "furi_hal_resources.h"

#define TAG "Button"

#define FURI_HAL_BUTTON_FLAG_B0_B5 0b10000000
#define FURI_HAL_BUTTON_FLAG_B6_B11 0b11000000
#define FURI_HAL_BUTTON_FLAG_MASK 0b11000000

#define FURI_HAL_BUTTON_MASK 0b00111111
//#define FURI_HAL_BUTTON_B0_B5_DEFALUT 0b00111111  //all button off
//#define FURI_HAL_BUTTON_B6_B11_DEFALUT 0b00111111 //all button off
#define FURI_HAL_BUTTON_B0_B5_DEFALUT 0b00000000 //all button off
#define FURI_HAL_BUTTON_B6_B11_DEFALUT 0b00000000 //all button off

#define FURI_HAL_BUTTON_FLAG_CMD 0b00000000
#define FURI_HAL_BUTTON_FLAG_CMD_MASK 0b10000000

typedef struct {
    uint8_t state_b0_b6;
    uint8_t state_b7_b11;
} FuriHalButtonState;

FuriHalButtonState furi_hal_button_state = {
    .state_b0_b6 = FURI_HAL_BUTTON_B0_B5_DEFALUT,
    .state_b7_b11 = FURI_HAL_BUTTON_B6_B11_DEFALUT};

static void furi_hal_button_init_gpio(void) {
    furi_hal_gpio_init(&gpio_button_trig, GpioModeInput, GpioPullNo, GpioSpeedLow);
}

static void furi_hal_button_deinit_gpio(void) {
    furi_hal_gpio_init(&gpio_button_trig, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
}

static void furi_hal_button_irq_cb(FuriHalHalfLPUartEvent event, void* context) {
    if(event & FuriHalHalfLPUartEventData) {
        uint8_t data = furi_hal_half_lpuart_get_rx_data();
        if((data & FURI_HAL_BUTTON_FLAG_MASK) == FURI_HAL_BUTTON_FLAG_B0_B5) {
            furi_hal_button_state.state_b0_b6 = ~data & FURI_HAL_BUTTON_MASK;
        } else if((data & FURI_HAL_BUTTON_FLAG_MASK) == FURI_HAL_BUTTON_FLAG_B6_B11) {
            furi_hal_button_state.state_b7_b11 = ~data & FURI_HAL_BUTTON_MASK;
        }

        // To print the button state (delete this block after testing)
        if((data & FURI_HAL_BUTTON_FLAG_MASK) == FURI_HAL_BUTTON_FLAG_B6_B11) {
            char tmp_str[] = "0xFFFFFFFF";
            furi_log_puts("Button B0-B0: 0x");
            itoa(furi_hal_button_state.state_b0_b6, tmp_str, 16);
            furi_log_puts(tmp_str);

            furi_log_puts(" B6-B11: 0x");
            itoa(furi_hal_button_state.state_b7_b11, tmp_str, 16);
            furi_log_puts(tmp_str);
            furi_log_puts("\r\n");
        }
    }

    if(event & FuriHalHalfLPUartEventIdle) {
        //idle line detected, packet transmission may have ended
    }
    if(event & FuriHalHalfLPUartEventOverrunError) {
        //err
    }
}

void furi_hal_button_init(void) {
    furi_hal_half_lpuart_set_rx_callback(furi_hal_button_irq_cb, NULL);
    furi_hal_button_init_gpio();
    furi_hal_half_lpuart_init();
}

void furi_hal_button_deinit(void) {
    furi_hal_half_lpuart_set_rx_callback(NULL, NULL);
    furi_hal_button_deinit_gpio();
    furi_hal_half_lpuart_deinit();
}
