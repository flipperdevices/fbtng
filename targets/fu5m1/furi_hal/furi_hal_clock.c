#include <core/log.h>
#include <furi_hal_bus.h>
#include <furi_hal_clock.h>
#include <stm32u5xx_ll_cortex.h>
#include <stm32u5xx_ll_system.h>
#include <stm32u5xx_ll_pwr.h>
#include <stm32u5xx_ll_utils.h>

#define TAG "FuriHalClock"

#define CPU_CLOCK_EARLY_HZ 4000000U
#define TICK_INT_PRIORITY 15U

void furi_hal_clock_init_early() {
    LL_SetSystemCoreClock(CPU_CLOCK_EARLY_HZ);
    LL_InitTick(SystemCoreClock, 1000U);
}

void furi_hal_clock_deinit_early() {
}

void furi_hal_clock_init() {
    // TODO: here lies the dragon
    // We need to figure out how to properly initialize the clocks

    LL_SYSTICK_DisableIT();
    furi_hal_bus_enable(FuriHalBusPWR);

    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4) {
    }

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    while(LL_PWR_IsActiveFlag_VOS() == 0) {
    }

    LL_RCC_HSE_Enable();
    while(LL_RCC_HSE_IsReady() != 1) {
    }

    LL_RCC_PLL1_ConfigDomain_SYS(LL_RCC_PLL1SOURCE_HSE, 1, 10, 1);
    LL_RCC_PLL1_EnableDomain_SYS();
    LL_RCC_SetPll1EPodPrescaler(LL_RCC_PLL1MBOOST_DIV_1);
    LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_8_16);

    LL_RCC_PLL1_Enable();
    while(LL_RCC_PLL1_IsReady() != 1) {
    }

    /* Intermediate AHB prescaler 2 when target frequency clock is higher than 80 MHz */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
    }

    /* Insure 1us transition state at intermediate medium speed clock*/
    for(__IO uint32_t i = (160 >> 1); i != 0; i--)
        ;

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);

    LL_SetSystemCoreClock(160000000);
    LL_InitTick(SystemCoreClock, 1000U);
    LL_SYSTICK_EnableIT();

    FURI_LOG_I(TAG, "Init OK");
}