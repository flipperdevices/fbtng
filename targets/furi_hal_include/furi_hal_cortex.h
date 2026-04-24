/**
 * @file furi_hal_cortex.h
 * ARM Cortex HAL
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <core/common_defines.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Early init stage for cortex
 */
void furi_hal_cortex_init_early(void);

/** Reset device
 */
FURI_NORETURN void furi_hal_cortex_system_reset(void);

typedef enum {
    FuriHalCortexJumpDFU,
    FuriHalCortexJumpSRAM,
    FuriHalCortexJumpFlash,
} FuriHalCortexJumpType;

/** Jump to DFU bootloader
 */
FURI_NORETURN void furi_hal_cortex_jump(FuriHalCortexJumpType jump_type);

typedef enum {
    FuriHalCortexComp0,
    FuriHalCortexComp1,
    FuriHalCortexComp2,
    FuriHalCortexComp3,
} FuriHalCortexComp;

typedef enum {
    FuriHalCortexCompSizeWord = 0b10,
    FuriHalCortexCompSizeHalfWord = 0b01,
    FuriHalCortexCompSizeByte = 0b00,
} FuriHalCortexCompSize;

typedef enum {
    FuriHalCortexCompFunctionPC = 0b100,
    FuriHalCortexCompFunctionRead = 0b101,
    FuriHalCortexCompFunctionWrite = 0b110,
    FuriHalCortexCompFunctionReadWrite = 0b110,
} FuriHalCortexCompFunction;

/** Enable DWT comparator
 * 
 * Allows to programmatically set instruction/data breakpoints.
 * 
 * More details on how it works can be found in armv7m official documentation:
 * https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/The-Data-Watchpoint-and-Trace-unit/The-DWT-comparators
 * https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/The-Data-Watchpoint-and-Trace-unit/Comparator-Function-registers--DWT-FUNCTIONn
 *
 * @param[in]  comp      The Comparator
 * @param[in]  function  The Comparator Function to use
 * @param[in]  value     The value
 * @param[in]  mask      The mask
 * @param[in]  size      The size
 */
void furi_hal_cortex_comp_enable(
    FuriHalCortexComp comp,
    FuriHalCortexCompFunction function,
    uint32_t value,
    uint32_t mask,
    FuriHalCortexCompSize size);

/** Reset DWT comparator
 *
 * @param[in]  comp  The Comparator
 */
void furi_hal_cortex_comp_reset(FuriHalCortexComp comp);

#ifdef __cplusplus
}
#endif
