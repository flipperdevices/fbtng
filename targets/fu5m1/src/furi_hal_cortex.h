#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_cortex_init_early();

void furi_hal_cortex_delay_us(uint32_t microseconds);

uint32_t furi_hal_cortex_instructions_per_microsecond();

#ifdef __cplusplus
}
#endif
