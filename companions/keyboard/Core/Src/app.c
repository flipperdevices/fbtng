#include "app.h"
#include "furi_hal_rcc.h"
#include "furi_hal_gpio.h"
#include "furi_hal_pwr.h"
#include "furi_hal_half_usart.h"
#include "furi_hal_common.h"
#include "furi_hal_led.h"
#include <stm32g0xx_ll_usart.h>

#include "app_led.h"

#define APP_KEY_FLAG_B0_B5 0b10000000
#define APP_KEY_FLAG_B6_B11 0b11000000 << 0x08
#define APP_KEY_FLAG_MASK 0b11000000

#define APP_KEY_FLAG_CMD 0b00000000
#define APP_KEY_FLAG_CMD_MASK 0b10000000

void app_init(void) {
    furi_hal_rcc_system_clock();
    furi_hal_rcc_system_clock_config();

    furi_hal_gpio_btn_init();
    //furi_hal_btn_LED();

    furi_hal_half_usart_init(); //off if need SWD
    furi_hal_gpio_trg_init(); //off if need SWD

    furi_hal_pwr_tim_start(5000);

    //led
    furi_hal_led_init();
    furi_hal_led_dma_tim_init(33, (FuriHalLed*)data_led, data_led_size);
    //furi_hal_led_dma_tim_init( 500,  (FuriHalLed*) data_rgb, data_rgb_size);
    furi_hal_led_set_rgb(10, 0, 0);
}

//BTN0 - LL_EXTI_LINE_0
//BTN1 - LL_EXTI_LINE_1
//BTN2 - LL_EXTI_LINE_2
//BTN3 - LL_EXTI_LINE_3
//BTN4 - LL_EXTI_LINE_4
//BTN5 - LL_EXTI_LINE_5
//BTN6 - LL_EXTI_LINE_11
//BTN7 - LL_EXTI_LINE_12
//BTN8 - LL_EXTI_LINE_6
//BTN9 - LL_EXTI_LINE_7
//BTN10 - LL_EXTI_LINE_14
//BTN11 - LL_EXTI_LINE_15

uint16_t app_get_btn_state(void) {
    volatile uint16_t btn_state = 0;
    uint16_t pa = GPIOA->IDR & 0b0001100000111111; //A12, A11, A5, A4, A3, A2, A1, A0
    uint16_t pb = GPIOB->IDR & 0b0000000011000000; //B7, B6
    uint16_t pc = GPIOC->IDR & 0b1100000000000000; // C15, C14
    btn_state = ((pa >> 5) & 0b11000000) | (pa & 0b111111) | (pb << 2) |
                (pc >> 4); //C15, C14, B7, B6, A12, A11, A5, A4, A3, A2, A1, A0

    return btn_state;
}

uint16_t app_get_btn_data(uint16_t btn_state) {
    btn_state = (btn_state & 0xFC0) << 2 | APP_KEY_FLAG_B6_B11 | (btn_state & 0x3F) |
                APP_KEY_FLAG_B0_B5;
    return btn_state;
}

void app_btn_update(void) {
    volatile uint16_t btn_state = 0;
    //volatile uint16_t count = 0;
    if(furi_hal_gpio_btn_get_update()) {
        furi_hal_gpio_btn_clear_update();

        while(1) {
            btn_state = app_get_btn_state();
            if((btn_state == app_get_btn_state()) && (btn_state == app_get_btn_state())) break;
        }

        //while(!LL_USART_IsActiveFlag_IDLE(USART2));
        LL_GPIO_ResetOutputPin(TRG_GPIO_Port, TRG_Pin); //off if need SWD
        btn_state = app_get_btn_data(btn_state);
        furi_hal_half_usart_tx((uint8_t*)&btn_state, 2); //off if need SWD
        furi_hal_pwr_tim_reset();
        LL_GPIO_SetOutputPin(TRG_GPIO_Port, TRG_Pin); //off if need SWD
    }
}

void app_do_loop(void) {
    app_btn_update();
    //furi_hal_pwr_power_down();
}
