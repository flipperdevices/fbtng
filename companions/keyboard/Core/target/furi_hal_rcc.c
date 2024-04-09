#include "furi_hal_rcc.h"

#include <stm32g0xx_ll_rcc.h>
#include <stm32g0xx_ll_utils.h>
#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_pwr.h>

void furi_hal_rcc_btn_clock(void){
	/* GPIO Ports Clock Enable */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
}

void furi_hal_rcc_system_clock(void){
	  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
	  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	  /* SysTick_IRQn interrupt configuration */
	  NVIC_SetPriority(SysTick_IRQn, 3);
}

void furi_hal_rcc_system_clock_config(void){
  /* HSI configuration and activation */
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1)
  {
  }

  LL_RCC_SetHSIDiv(LL_RCC_HSI_DIV_8);
  /* Set AHB prescaler*/
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  /* Sysclk activation on the HSI */
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  }

  /* Set APB1 prescaler*/
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

  LL_Init1msTick(2000000);

  /* Update CMSIS variable (which can be updated also through SystemCoreClockUpdate function) */
  LL_SetSystemCoreClock(2000000);
}
