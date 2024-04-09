#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//button
#define BTN_0_Pin LL_GPIO_PIN_0
#define BTN_0_GPIO_Port GPIOA
#define BTN_0_EXTI_IRQn EXTI0_1_IRQn
#define BTN_1_Pin LL_GPIO_PIN_1
#define BTN_1_GPIO_Port GPIOA
#define BTN_1_EXTI_IRQn EXTI0_1_IRQn
#define BTN_2_Pin LL_GPIO_PIN_2
#define BTN_2_GPIO_Port GPIOA
#define BTN_2_EXTI_IRQn EXTI2_3_IRQn
#define BTN_3_Pin LL_GPIO_PIN_3
#define BTN_3_GPIO_Port GPIOA
#define BTN_3_EXTI_IRQn EXTI2_3_IRQn
#define BTN_4_Pin LL_GPIO_PIN_4
#define BTN_4_GPIO_Port GPIOA
#define BTN_4_EXTI_IRQn EXTI4_15_IRQn
#define BTN_5_Pin LL_GPIO_PIN_5
#define BTN_5_GPIO_Port GPIOA
#define BTN_5_EXTI_IRQn EXTI4_15_IRQn
#define BTN_6_Pin LL_GPIO_PIN_11
#define BTN_6_GPIO_Port GPIOA
#define BTN_6_EXTI_IRQn EXTI4_15_IRQn
#define BTN_7_Pin LL_GPIO_PIN_12
#define BTN_7_GPIO_Port GPIOA
#define BTN_7_EXTI_IRQn EXTI4_15_IRQn
#define BTN_8_Pin LL_GPIO_PIN_6
#define BTN_8_GPIO_Port GPIOB
#define BTN_8_EXTI_IRQn EXTI4_15_IRQn
#define BTN_9_Pin LL_GPIO_PIN_7
#define BTN_9_GPIO_Port GPIOB
#define BTN_9_EXTI_IRQn EXTI4_15_IRQn
#define BTN_10_Pin LL_GPIO_PIN_14
#define BTN_10_GPIO_Port GPIOC
#define BTN_10_EXTI_IRQn EXTI4_15_IRQn
#define BTN_11_Pin LL_GPIO_PIN_15
#define BTN_11_GPIO_Port GPIOC
#define BTN_11_EXTI_IRQn EXTI4_15_IRQn

//Triger
#define TRG_Pin LL_GPIO_PIN_13
#define TRG_GPIO_Port GPIOA

//Led
#define LED_B_T3_CH1_Pin LL_GPIO_PIN_6
#define LED_B_T3_CH1_GPIO_Port GPIOA
#define LED_G_T3_CH2_Pin LL_GPIO_PIN_7
#define LED_G_T3_CH2_GPIO_Port GPIOA
#define LED_R_T3_CH3_Pin LL_GPIO_PIN_0
#define LED_R_T3_CH3_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif
