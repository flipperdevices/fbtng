#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t FuriHalBus;

/** Early initialization */
void furi_hal_bus_init_early(void);

/** Early de-initialization */
void furi_hal_bus_deinit_early(void);

/**
 * Enable a peripheral by turning the clocking on and deasserting the reset.
 * @param [in] bus Peripheral to be enabled.
 * @warning Peripheral must be in disabled state in order to be enabled.
 */
void furi_hal_bus_enable(FuriHalBus bus);

/**
 * Reset a peripheral by sequentially asserting and deasserting the reset.
 * @param [in] bus Peripheral to be reset.
 * @warning Peripheral must be in enabled state in order to be reset.
 */
void furi_hal_bus_reset(FuriHalBus bus);

/**
 * Disable a peripheral by turning the clocking off and asserting the reset.
 * @param [in] bus Peripheral to be disabled.
 * @warning Peripheral must be in enabled state in order to be disabled.
 */
void furi_hal_bus_disable(FuriHalBus bus);

/** Check if peripheral is enabled
 *
 * @param[in]  bus   The peripheral to check
 *
 * @return     true if enabled or always enabled, false otherwise
 */
bool furi_hal_bus_is_enabled(FuriHalBus bus);

#ifdef __cplusplus
}
#endif
