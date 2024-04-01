#pragma once

#include <stm32u5xx_ll_rcc.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Early initialization */
void furi_hal_clock_init_early();

/** Early deinitialization */
void furi_hal_clock_deinit_early();

/** Initialize clocks */
void furi_hal_clock_init();

/** Stop SysTick counter without resetting */
void furi_hal_clock_suspend_tick();

/** Continue SysTick counter operation */
void furi_hal_clock_resume_tick();

/** Switch clock from HSE to HSI */
void furi_hal_clock_switch_hse2hsi();

/** Switch clock from HSI to HSE */
void furi_hal_clock_switch_hsi2hse();

/** Switch clock from HSE to PLL
 *
 * @return     true if changed, false if failed or not possible at this moment
 */
bool furi_hal_clock_switch_hse2pll();

/** Switch clock from PLL to HSE
 *
 * @return     true if changed, false if failed or not possible at this moment
 */
bool furi_hal_clock_switch_pll2hse();

typedef enum {
    FuriHalClockHwSdMmc12,
} FuriHalClockHW;

uint32_t furi_hal_clock_get_freq(FuriHalClockHW hw);

#ifdef __cplusplus
}
#endif
