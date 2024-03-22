#pragma once
#include "furi_hal_bus.h"
#include "furi_hal_cortex.h"
#include "furi_hal_clock.h"
#include "furi_hal_debug.h"
#include "furi_hal_gpio.h"
#include "furi_hal_resources.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Early FuriHal init, only essential subsystems */
void furi_hal_init_early();

/** Early FuriHal deinit */
void furi_hal_deinit_early();

/** Init FuriHal */
void furi_hal_init();

/** Transfer execution to address
 *
 * @param[in]  address  pointer to new executable
 */
void furi_hal_switch(void* address);

#ifdef __cplusplus
}
#endif