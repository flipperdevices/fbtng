#pragma once

#include <stm32g0xx_ll_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

uint8_t furi_hal_gpio_btn_get_update(void);
void furi_hal_gpio_btn_clear_update(void);
//void furi_hal_btn_LED(void);
void furi_hal_gpio_btn_init(void);
void furi_hal_gpio_trg_init(void);

#ifdef __cplusplus
}
#endif
