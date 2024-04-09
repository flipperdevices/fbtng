#include "furi_hal_led.h"
#include "furi_hal_common.h"

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_tim.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_dma.h>

void furi_hal_led_init(void){
	LL_TIM_InitTypeDef TIM_InitStruct = { 0 };
	LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = { 0 };

	LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);

	TIM_InitStruct.Prescaler = 8 - 1;
	TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
	TIM_InitStruct.Autoreload = 255;
	TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
	LL_TIM_Init(TIM3, &TIM_InitStruct);
	LL_TIM_DisableARRPreload(TIM3);
	LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH1);

	TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
	TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
	TIM_OC_InitStruct.CompareValue = 0;
	TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
	LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH1);
	LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH2);
	LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH2);
	LL_TIM_OC_EnablePreload(TIM3, LL_TIM_CHANNEL_CH3);
	LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);
	LL_TIM_OC_DisableFast(TIM3, LL_TIM_CHANNEL_CH3);
	LL_TIM_SetTriggerOutput(TIM3, LL_TIM_TRGO_RESET);
	LL_TIM_DisableMasterSlaveMode(TIM3);

	GPIO_InitStruct.Pin = LED_B_T3_CH1_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(LED_B_T3_CH1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED_G_T3_CH2_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(LED_G_T3_CH2_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED_R_T3_CH3_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(LED_R_T3_CH3_GPIO_Port, &GPIO_InitStruct);

	LL_TIM_OC_SetCompareCH1(TIM3, 0);
	LL_TIM_OC_SetCompareCH2(TIM3, 0);
	LL_TIM_OC_SetCompareCH4(TIM3, 0);

	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH2);
	LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH3);
	LL_TIM_SetCounter(TIM3, 0);
	LL_TIM_EnableCounter(TIM3);
}

void furi_hal_led_deinit(void) {
	LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_TIM3);

	LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	GPIO_InitStruct.Pin = LED_B_T3_CH1_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	LL_GPIO_Init(LED_B_T3_CH1_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED_G_T3_CH2_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	LL_GPIO_Init(LED_G_T3_CH2_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LED_R_T3_CH3_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
	LL_GPIO_Init(LED_R_T3_CH3_GPIO_Port, &GPIO_InitStruct);
}

void furi_hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
	if (r || g || b) {
		//Red channel
		LL_TIM_OC_SetCompareCH3(TIM3, r);
		//Green channel
		LL_TIM_OC_SetCompareCH2(TIM3, b);
		//Blue channel
		LL_TIM_OC_SetCompareCH1(TIM3, g);
		if (!LL_TIM_IsEnabledCounter(TIM3)) {
			//furi_hal_led_init();
			LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
			LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH2);
			LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH3);
			LL_TIM_EnableCounter(TIM3);
		}

	} else {
		if (LL_TIM_IsEnabledCounter(TIM3)) {
			LL_TIM_DisableCounter(TIM3);
			//Red channel
			LL_TIM_OC_SetCompareCH3(TIM3, 0);
			//Green channel
			LL_TIM_OC_SetCompareCH2(TIM3, 0);
			//Blue channel
			LL_TIM_OC_SetCompareCH1(TIM3, 0);
			LL_TIM_CC_DisableChannel(TIM3, LL_TIM_CHANNEL_CH1);
			LL_TIM_CC_DisableChannel(TIM3, LL_TIM_CHANNEL_CH2);
			LL_TIM_CC_DisableChannel(TIM3, LL_TIM_CHANNEL_CH3);
			//furi_hal_led_deinit();
		}
	}

}

uint16_t data_led1[12] = {};

void furi_hal_led_dma_tim_init(uint16_t update_ms, FuriHalLed *data, size_t size){

	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM17);

	LL_TIM_SetPrescaler(TIM17, 2000 - 1);
	LL_TIM_SetCounterMode(TIM17, LL_TIM_COUNTERMODE_UP);
	LL_TIM_SetAutoReload(TIM17, update_ms);
	LL_TIM_SetClockDivision(TIM17, LL_TIM_CLOCKDIVISION_DIV1);

    LL_TIM_SetClockSource(TIM17, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_DisableARRPreload(TIM17);
    LL_TIM_DisableIT_TRIG(TIM17);

    //Init DMA
	//Clock
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

	//DMA1 Channel_2 Req LL_DMAMUX_REQ_TIM17_UP for generator LL_DMAMUX_REQ_GEN_DMAMUX_CH1
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t) data);
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t) &data_led1);
	LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_2,
		LL_DMA_DIRECTION_MEMORY_TO_PERIPH | LL_DMA_MODE_CIRCULAR | LL_DMA_PERIPH_NOINCREMENT |
			LL_DMA_MEMORY_INCREMENT | LL_DMA_PDATAALIGN_HALFWORD | LL_DMA_MDATAALIGN_BYTE |
			LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, size/sizeof(uint8_t));
	LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_2, LL_DMAMUX_REQ_TIM17_UP);

	LL_DMAMUX_EnableEventGeneration(DMAMUX1, LL_DMAMUX_CHANNEL_1);
	LL_DMAMUX_SetSyncRequestNb(DMAMUX1, LL_DMAMUX_CHANNEL_1, 1);
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);

	//Setup DMAMUX
	LL_DMAMUX_SetRequestSignalID(DMAMUX1, LL_DMAMUX_REQ_GEN_0, LL_DMAMUX_REQ_GEN_DMAMUX_CH1);
	LL_DMAMUX_SetRequestGenPolarity(DMAMUX1, LL_DMAMUX_REQ_GEN_0, LL_DMAMUX_REQ_GEN_POL_RISING);
	LL_DMAMUX_SetGenRequestNb(DMAMUX1, LL_DMAMUX_REQ_GEN_0, 3);
	//Start DMAMUX
	LL_DMAMUX_EnableRequestGen(DMAMUX1, LL_DMAMUX_REQ_GEN_0);

	//DMA1 Channel_3 Req LL_DMAMUX_REQ_GENERATOR0
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t) data);
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t) &(TIM3->DMAR));
	LL_DMA_ConfigTransfer(DMA1, LL_DMA_CHANNEL_3,
		LL_DMA_DIRECTION_MEMORY_TO_PERIPH | LL_DMA_MODE_CIRCULAR | LL_DMA_PERIPH_NOINCREMENT |
			LL_DMA_MEMORY_INCREMENT | LL_DMA_PDATAALIGN_WORD | LL_DMA_MDATAALIGN_BYTE |
			LL_DMA_PRIORITY_MEDIUM);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, size/sizeof(uint8_t));
	LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_3, LL_DMAMUX_REQ_GENERATOR0);

	//Strat DMA
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);

	//Config DMABust TIM3
	LL_TIM_ConfigDMABurst(TIM3, LL_TIM_DMABURST_BASEADDR_CCR1, LL_TIM_DMABURST_LENGTH_3TRANSFERS);

	//    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_3);
	//    LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_3);


	//Start req
	LL_TIM_EnableDMAReq_UPDATE(TIM17);

//    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 4);
//    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);



//    LL_TIM_EnableIT_UPDATE(TIM17);
//    LL_TIM_DisableDMAReq_TRIG(TIM17);
//    NVIC_SetPriority(TIM17_IRQn, 4);
//    NVIC_EnableIRQ(TIM17_IRQn);


    LL_TIM_SetCounter(TIM17, 0);
    LL_TIM_EnableCounter(TIM17);
}

void furi_hal_led_dma_tim_deinit(void) {

	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_2);
	LL_DMAMUX_DisableRequestGen(DMAMUX1, LL_DMAMUX_REQ_GENERATOR0);
	LL_DMAMUX_DisableEventGeneration(DMAMUX1, LL_DMAMUX_CHANNEL_1);
	LL_AHB1_GRP1_DisableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	LL_APB2_GRP1_DisableClock(LL_APB2_GRP1_PERIPH_TIM17);
	//NVIC_DisableIRQ(TIM17_IRQn);
}

volatile uint8_t trig =0;

void TIM17_IRQHandler(void){
	if (LL_TIM_IsActiveFlag_UPDATE(TIM17)) {
		LL_TIM_ClearFlag_UPDATE(TIM17);
		if(trig){
			furi_hal_led_set_rgb(10,127,255);
			trig=0;
		} else {
			furi_hal_led_set_rgb(0,0,0);
			trig=1;
		}
	}
}
