#include "furi_hal_gpio.h"
#include "furi_hal_common.h"
#include "furi_hal_rcc.h"
#include <stm32g0xx_ll_exti.h>


volatile uint8_t furi_hal_btn_update = 0;

inline uint8_t furi_hal_gpio_btn_get_update(void){
	return furi_hal_btn_update;
}

inline void furi_hal_gpio_btn_clear_update(void){
	furi_hal_btn_update = 0;
}

//void furi_hal_btn_LED(void)
//{
//
//  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
//
//  /* GPIO Ports Clock Enable */
//  //LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
//
//  /**/
//  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
//
//  /**/
//  GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
//  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
//  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
//  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
//  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
//  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//
//}

void furi_hal_gpio_btn_init(void) {
	furi_hal_rcc_btn_clock();

	LL_EXTI_InitTypeDef EXTI_InitStruct = { 0 };

	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE0);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE1);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE2);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE3);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE4);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE5);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE6);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTB, LL_EXTI_CONFIG_LINE7);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE11);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTA, LL_EXTI_CONFIG_LINE12);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTC, LL_EXTI_CONFIG_LINE14);
	LL_EXTI_SetEXTISource(LL_EXTI_CONFIG_PORTC, LL_EXTI_CONFIG_LINE15);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_1;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_2;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_3;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_4;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_5;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_6;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_7;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_11;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_12;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_14;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_15;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	LL_GPIO_SetPinPull(BTN_0_GPIO_Port, BTN_0_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_1_GPIO_Port, BTN_1_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_2_GPIO_Port, BTN_2_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_3_GPIO_Port, BTN_3_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_4_GPIO_Port, BTN_4_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_5_GPIO_Port, BTN_5_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_6_GPIO_Port, BTN_6_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_7_GPIO_Port, BTN_7_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_8_GPIO_Port, BTN_8_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_9_GPIO_Port, BTN_9_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_10_GPIO_Port, BTN_10_Pin, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinPull(BTN_11_GPIO_Port, BTN_11_Pin, LL_GPIO_PULL_NO);


	LL_GPIO_SetPinMode(BTN_0_GPIO_Port, BTN_0_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_1_GPIO_Port, BTN_1_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_2_GPIO_Port, BTN_2_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_3_GPIO_Port, BTN_3_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_4_GPIO_Port, BTN_4_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_5_GPIO_Port, BTN_5_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_6_GPIO_Port, BTN_6_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_7_GPIO_Port, BTN_7_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_8_GPIO_Port, BTN_8_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_9_GPIO_Port, BTN_9_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_10_GPIO_Port, BTN_10_Pin, LL_GPIO_MODE_INPUT);
	LL_GPIO_SetPinMode(BTN_11_GPIO_Port, BTN_11_Pin, LL_GPIO_MODE_INPUT);

	/* EXTI interrupt init*/
	NVIC_SetPriority(EXTI0_1_IRQn, 1);
	NVIC_EnableIRQ(EXTI0_1_IRQn);
	NVIC_SetPriority(EXTI2_3_IRQn, 1);
	NVIC_EnableIRQ(EXTI2_3_IRQn);
	NVIC_SetPriority(EXTI4_15_IRQn, 1);
	NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void furi_hal_gpio_trg_init(void){
	LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	LL_GPIO_SetOutputPin(TRG_GPIO_Port, TRG_Pin);

	GPIO_InitStruct.Pin = TRG_Pin;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(TRG_GPIO_Port, &GPIO_InitStruct);

}

static void furi_hal_gpio_btn_update(void) {

	//check falling IRQ Key
	uint16_t falling_irq_key = (EXTI->FPR1) & 0b1101100011111111;
	if (falling_irq_key) {
		//clear falling IRQ Key
		EXTI->FPR1 = falling_irq_key;
		furi_hal_btn_update = 1;
	}
	//check rising IRQ Key
	uint16_t rising_irq_key = (EXTI->RPR1) & 0b1101100011111111;
	if (rising_irq_key) {
		//clear rising IRQ Key
		EXTI->RPR1 = rising_irq_key;
		furi_hal_btn_update = 1;
	}

}

void EXTI0_1_IRQHandler(void){
	furi_hal_gpio_btn_update();
}

void EXTI2_3_IRQHandler(void){
	furi_hal_gpio_btn_update();
}

void EXTI4_15_IRQHandler(void){
	furi_hal_gpio_btn_update();
}
