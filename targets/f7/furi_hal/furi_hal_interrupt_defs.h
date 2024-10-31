#pragma once

#include <stm32wbxx_ll_tim.h>

/** Device-specific interrupt identifier */
typedef enum {
    // TIM1, TIM16, TIM17
    FuriHalInterruptIdTim1TrgComTim17,
    FuriHalInterruptIdTim1Cc,
    FuriHalInterruptIdTim1UpTim16,

    // TIM2
    FuriHalInterruptIdTIM2,

    // DMA1
    FuriHalInterruptIdDma1Ch1,
    FuriHalInterruptIdDma1Ch2,
    FuriHalInterruptIdDma1Ch3,
    FuriHalInterruptIdDma1Ch4,
    FuriHalInterruptIdDma1Ch5,
    FuriHalInterruptIdDma1Ch6,
    FuriHalInterruptIdDma1Ch7,

    // DMA2
    FuriHalInterruptIdDma2Ch1,
    FuriHalInterruptIdDma2Ch2,
    FuriHalInterruptIdDma2Ch3,
    FuriHalInterruptIdDma2Ch4,
    FuriHalInterruptIdDma2Ch5,
    FuriHalInterruptIdDma2Ch6,
    FuriHalInterruptIdDma2Ch7,

    // RCC
    FuriHalInterruptIdRcc,

    // Comp
    FuriHalInterruptIdCOMP,

    // RTC
    FuriHalInterruptIdRtcAlarm,

    // HSEM
    FuriHalInterruptIdHsem,

    // LPTIMx
    FuriHalInterruptIdLpTim1,
    FuriHalInterruptIdLpTim2,

    //UARTx
    FuriHalInterruptIdUart1,

    //LPUARTx
    FuriHalInterruptIdLpUart1,

    // Service value
    FuriHalInterruptIdMax,
} FuriHalInterruptId;
