/**
 * @file furi_hal_power.h
 * Power HAL API
 */
#pragma once

#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Reset device
 */
FURI_NORETURN void furi_hal_power_reset(void);

#ifdef __cplusplus
}
#endif
