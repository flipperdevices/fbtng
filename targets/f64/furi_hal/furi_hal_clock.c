#include <furi_hal_clock.h>
#include <furi.h>

#include <sl_si91x_clock_manager.h>

#define TAG "FuriHalClock"

#define CPU_CLOCK_PLL_HZ 180000000UL

void furi_hal_clock_init_early(void) {
    SystemCoreClockUpdate();
}

void furi_hal_clock_deinit_early(void) {
}

void furi_hal_clock_init(void) {
    furi_check(sl_si91x_clock_manager_m4_set_core_clk(M4_SOCPLLCLK, CPU_CLOCK_PLL_HZ) == SL_STATUS_OK);

    FURI_LOG_I(TAG, "Init OK");
}

void furi_hal_clock_suspend_tick(void) {
    FURI_BIT_CLEAR(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);
}

void furi_hal_clock_resume_tick(void) {
    FURI_BIT_SET(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);
}
