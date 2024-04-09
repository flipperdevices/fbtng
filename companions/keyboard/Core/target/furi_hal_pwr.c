#include "furi_hal_pwr.h"
#include "furi_hal_led.h"
#include "furi_hal_gpio.h"

#include <stm32g0xx_ll_pwr.h>
#include <stm32g0xx_ll_tim.h>
#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_cortex.h>

volatile uint8_t power_down = 0;

/** @defgroup PWR_Regulator_state_in_SLEEP_STOP_mode  PWR regulator mode */
#define PWR_MAINREGULATOR_ON                (0x00000000u)  /*!< Regulator in main mode      */
#define PWR_LOWPOWERREGULATOR_ON            PWR_CR1_LPR    /*!< Regulator in low-power mode */

/** @defgroup PWR_SLEEP_mode_entry  PWR SLEEP mode entry  */
#define PWR_SLEEPENTRY_WFI                  ((uint8_t)0x01u)        /*!< Wait For Interruption instruction to enter Sleep mode */
#define PWR_SLEEPENTRY_WFE                  ((uint8_t)0x02u)        /*!< Wait For Event instruction to enter Sleep mode        */

/** @defgroup PWR_STOP_mode_entry  PWR STOP mode entry */
#define PWR_STOPENTRY_WFI                   ((uint8_t)0x01u)        /*!< Wait For Interruption instruction to enter Stop mode */
#define PWR_STOPENTRY_WFE                   ((uint8_t)0x02u)        /*!< Wait For Event instruction to enter Stop mode*/

/**
 * @brief  Enter Sleep or Low-power Sleep mode.
 * @note   In Sleep/Low-power Sleep mode, all I/O pins keep the same state as
 *         in Run mode.
 * @param  Regulator Specifies the regulator state in Sleep/Low-power Sleep
 *         mode. This parameter can be one of the following values:
 *           @arg @ref PWR_MAINREGULATOR_ON Sleep mode (regulator in main mode)
 *           @arg @ref PWR_LOWPOWERREGULATOR_ON Low-power Sleep mode (regulator
 *                     in low-power mode)
 * @note   Low-power Sleep mode is entered from Low-power Run mode only. In
 *         case Regulator parameter is set to Low Power but MCU is in Run mode,
 *         we will first enter in Low-power Run mode. Therefore, user should
 *         take care that HCLK frequency is less than 2 MHz.
 * @note   When exiting Low-power Sleep mode, the MCU is in Low-power Run mode.
 *         To switch back to Run mode, user must call
 *         HAL_PWREx_DisableLowPowerRunMode() API.
 * @param  SLEEPEntry Specifies if Sleep mode is entered with WFI or WFE
 *         instruction. This parameter can be one of the following values:
 *           @arg @ref PWR_SLEEPENTRY_WFI enter Sleep or Low-power Sleep
 *                     mode with WFI instruction
 *           @arg @ref PWR_SLEEPENTRY_WFE enter Sleep or Low-power Sleep
 *                     mode with WFE instruction
 * @note   When WFI entry is used, tick interrupt have to be disabled if not
 *         desired as the interrupt wake up source.
 * @retval None
 */

static void furi_hal_pwr_sleep_mode(uint32_t Regulator, uint8_t SLEEPEntry) {
	/* Set Regulator parameter */
	if (Regulator != PWR_MAINREGULATOR_ON) {
		/* If in run mode, first move to low-power run mode.
		 The system clock frequency must be below 2 MHz at this point. */
		if (LL_PWR_IsActiveFlag_REGLPF() == 0x00u) {
			LL_PWR_EnableLowPowerRunMode();
		}
	} else {
		/* If in low-power run mode at this point, exit it */
		if (LL_PWR_IsActiveFlag_REGLPF() != 0x00u) {
			LL_PWR_DisableLowPowerRunMode();
			/* Wait until REGLPF is reset */
			while (LL_PWR_IsActiveFlag_REGLPF()) {
				//Todo add timeout
//				if (wait_loop_index != 0U) {
//					wait_loop_index--;
//				} else {
//
//				}
			}
		}
	}

	LL_LPM_EnableSleep();

	/* Select SLEEP mode entry -------------------------------------------------*/
	if (SLEEPEntry == PWR_SLEEPENTRY_WFI) {
		/* Request Wait For Interrupt */
		__WFI();
	} else {
		/* Request Wait For Event */
		__SEV();
		__WFE();
		__WFE();
	}
}

/**
 * @brief  Enter Stop mode
 * @note   This API is named HAL_PWR_EnterSTOPMode to ensure compatibility with
 *         legacy code running on devices where only "Stop mode" is mentioned
 *         with main or low power regulator ON.
 * @note   In Stop mode, all I/O pins keep the same state as in Run mode.
 * @note   All clocks in the VCORE domain are stopped; the PLL, the HSI and the
 *         HSE oscillators are disabled. Some peripherals with the wakeup
 *         capability can switch on the HSI to receive a frame, and switch off
 *         the HSI after receiving the frame if it is not a wakeup frame.
 *         SRAM and register contents are preserved.
 *         The BOR is available.
 *         The voltage regulator can be configured either in normal (Stop 0) or
 *         low-power mode (Stop 1).
 * @note   When exiting Stop 0 or Stop 1 mode by issuing an interrupt or a
 *         wakeup event, the HSI RC oscillator is selected as system clock
 * @note   When the voltage regulator operates in low power mode (Stop 1),
 *         an additional startup delay is incurred when waking up. By keeping
 *         the internal regulator ON during Stop mode (Stop 0), the consumption
 *         is higher although the startup time is reduced.
 * @param  Regulator Specifies the regulator state in Stop mode
 *         This parameter can be one of the following values:
 *            @arg @ref PWR_MAINREGULATOR_ON  Stop 0 mode (main regulator ON)
 *            @arg @ref PWR_LOWPOWERREGULATOR_ON  Stop 1 mode (low power
 *                                                regulator ON)
 * @param  STOPEntry Specifies Stop 0 or Stop 1 mode is entered with WFI or
 *         WFE instruction. This parameter can be one of the following values:
 *            @arg @ref PWR_STOPENTRY_WFI  Enter Stop 0 or Stop 1 mode with WFI
 *                                         instruction.
 *            @arg @ref PWR_STOPENTRY_WFE  Enter Stop 0 or Stop 1 mode with WFE
 *                                         instruction.
 * @retval None
 */
static void furi_hal_pwr_stop_mode(uint32_t Regulator, uint8_t STOPEntry) {
	if (Regulator != PWR_MAINREGULATOR_ON) {
		/* Stop mode with Low-Power Regulator */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP1);
	} else {
		/* Stop mode with Main Regulator */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP0);
	}

	/* Set SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableDeepSleep();

	/* Select Stop mode entry --------------------------------------------------*/
	if (STOPEntry == PWR_STOPENTRY_WFI) {
		/* Request Wait For Interrupt */
		__WFI();
	} else {
		/* Request Wait For Event */
		__SEV();
		__WFE();
		__WFE();
	}

	/* Reset SLEEPDEEP bit of Cortex System Control Register */
	LL_LPM_EnableSleep();
}

void furi_hal_pwr_power_down(void){
	if (!furi_hal_gpio_btn_get_update()) {
		LL_SYSTICK_DisableIT();
		if (!power_down) {
			furi_hal_pwr_sleep_mode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI); //Sleep_mode
		} else {
			//furi_hal_led_dma_tim_deinit();
			furi_hal_led_deinit();
			furi_hal_pwr_stop_mode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); //STOP1_mode
			//furi_hal_pwr_stop_mode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI); ////STOP0_mode
			furi_hal_led_init();
			//furi_hal_led_dma_tim_init();
			//furi_hal_led_set_rgb(10,127,255);
			power_down = 0;
			furi_hal_pwr_tim_reset();
		}
		LL_SYSTICK_EnableIT();
	}
}


void furi_hal_pwr_tim_start(uint32_t timeout){
  //Automatically enable Stop Mode after N msec

  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM14);
  /* Peripheral clock enable */


  /* TIM17 interrupt Init */
  NVIC_SetPriority(TIM14_IRQn, 5);
  NVIC_EnableIRQ(TIM14_IRQn);

  TIM_InitStruct.Prescaler = 2000-1;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = timeout;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM14, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM14);

  //Start tim
  LL_TIM_EnableIT_UPDATE(TIM14);
  LL_TIM_SetCounter(TIM14, 0);
  LL_TIM_EnableCounter(TIM14);
}

void furi_hal_pwr_tim_reset(void){
	LL_TIM_SetCounter(TIM14, 0);
}

void TIM14_IRQHandler(void){
	if (LL_TIM_IsActiveFlag_UPDATE(TIM14)) {
		LL_TIM_ClearFlag_UPDATE(TIM14);
		power_down = 1;
//		uint8_t g[1] = {0x55};
//#ifndef SWD
//		func_tx((uint8_t*)g, 1);
//#endif
	}
}
