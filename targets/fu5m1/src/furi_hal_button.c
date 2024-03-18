#include "furi_hal_button.h"
#include "furi_hal_half_lpuart.h"

#include "stm32u5xx_ll_bus.h"
#include "stm32u5xx_ll_gpio.h"

#include "dbg_log.h"
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
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

// static void furi_hal_button_deinit_gpio(void) {
// 	LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };

// 	GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
// 	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
// 	LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
// }

static void furi_hal_button_irq_cb(FuriHalHalfLPUartEvent event, void* context) {
    if(event & FuriHalHalfLPUartEventData) {
        uint8_t data = furi_hal_half_lpuart_get_rx_data();
        if((data & FURI_HAL_BUTTON_FLAG_MASK) == FURI_HAL_BUTTON_FLAG_B0_B5) {
            furi_hal_button_state.state_b0_b6 = ~data & FURI_HAL_BUTTON_MASK;
        } else if((data & FURI_HAL_BUTTON_FLAG_MASK) == FURI_HAL_BUTTON_FLAG_B6_B11) {
            furi_hal_button_state.state_b7_b11 = ~data & FURI_HAL_BUTTON_MASK;
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
