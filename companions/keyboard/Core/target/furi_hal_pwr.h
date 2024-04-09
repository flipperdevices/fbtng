#pragma once

#include <stm32g0xx_ll_rcc.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_pwr_power_down(void);
void furi_hal_pwr_tim_start(uint32_t timeout);
void furi_hal_pwr_tim_reset(void);

#ifdef __cplusplus
}
#endif
