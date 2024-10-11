#include <furi_hal_clock.h>
#include <furi.h>

#include <rsi_rom_clks.h>

#include <FreeRTOS.h>

#define TAG "FuriHalClock"

#define XTAL_FREQ_HZ 40000000UL
#define CPU_CLOCK_PLL_HZ 180000000UL

#define TICK_INT_PRIORITY 15U

void furi_hal_clock_init_early(void) {
    SystemCoreClockUpdate();
}

void furi_hal_clock_deinit_early(void) {
}

void furi_hal_clock_init(void) {
    furi_check(RSI_CLK_M4SocClkConfig(M4CLK, M4_ULPREFCLK, 0) == RSI_OK);
#if CPU_CLOCK_PLL_HZ > 120000000UL
    RSI_PS_PS4SetRegisters();
#endif
    furi_check(RSI_CLK_SetSocPllFreq(M4CLK, CPU_CLOCK_PLL_HZ, XTAL_FREQ_HZ) == RSI_OK);
    furi_check(RSI_CLK_M4SocClkConfig(M4CLK, M4_SOCPLLCLK, 0) == RSI_OK);

    SysTick_Config(SystemCoreClock / configTICK_RATE_HZ);

    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), TICK_INT_PRIORITY, 0));
    NVIC_EnableIRQ(SysTick_IRQn);

    FURI_LOG_I(TAG, "Init OK");
}

void furi_hal_clock_suspend_tick(void) {
    FURI_BIT_CLEAR(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);
}

void furi_hal_clock_resume_tick(void) {
    FURI_BIT_SET(SysTick->CTRL, SysTick_CTRL_ENABLE_Msk);
}
