#pragma once

typedef enum {
    FuriHalInterruptIdTimer0,
    FuriHalInterruptIdTimer1,
    FuriHalInterruptIdTimer2,
    FuriHalInterruptIdTimer3,
//     FuriHalInterruptIdCAP_SENSOR,
    // FuriHalInterruptIdComp2,
    // FuriHalInterruptIdComp1,
    FuriHalInterruptIdUDMA1,
//     FuriHalInterruptIdADC,
    FuriHalInterruptIdULPSS_UART,
//     FuriHalInterruptIdI2C2,
//     FuriHalInterruptIdI2S1,
//     FuriHalInterruptIdIR_DECODER,
//     FuriHalInterruptIdSSI2,
//     FuriHalInterruptIdFIM,
//     FuriHalInterruptIdULP_EGPIO_PIN,
//     FuriHalInterruptIdULP_EGPIO_GROUP,
//     FuriHalInterruptIdNPSS_TO_MCU_WDT_INTR,
//     FuriHalInterruptIdNPSS_TO_MCU_GPIO_INTR,
// #ifdef SLI_SI917B0
//     FuriHalInterruptNPSS_TO_MCU_SYSRTC_INTR,
// #else
//     FuriHalInterruptNPSS_TO_MCU_CMP_RF_WKP_INTR,
// #endif
//     FuriHalInterruptIdNPSS_TO_MCU_BOD_INTR,
//     FuriHalInterruptIdNPSS_TO_MCU_BUTTON_INTR,
//     FuriHalInterruptIdNPSS_TO_MCU_SDC_INTR,
//     FuriHalInterruptIdNPSS_TO_MCU_WIRELESS_INTR,
//     FuriHalInterruptIdNPSS_MCU_INTR,
//     FuriHalInterruptIdMCU_CAL_ALARM,
//     FuriHalInterruptIdMCU_CAL_RTC,
    FuriHalInterruptIdGPDMA,
    FuriHalInterruptIdUDMA0,
//     FuriHalInterruptIdCT,
//     FuriHalInterruptIdHIF0,
//     FuriHalInterruptIdHIF1,
//     FuriHalInterruptIdSIO,
    FuriHalInterruptIdUSART0,
    FuriHalInterruptIdUART1,
//     FuriHalInterruptIdEGPIO_WAKEUP,
//     FuriHalInterruptIdI2C0,
//     FuriHalInterruptIdSSISlave,
//     FuriHalInterruptIdGSPI0,
//     FuriHalInterruptIdSSI0,
//     FuriHalInterruptIdMCPWM,
//     FuriHalInterruptIdQEI,
//     FuriHalInterruptIdEGPIO_GROUP_0,
//     FuriHalInterruptIdEGPIO_GROUP_1,
//     FuriHalInterruptIdEGPIO_PIN_0,
//     FuriHalInterruptIdEGPIO_PIN_1,
//     FuriHalInterruptIdEGPIO_PIN_2,
//     FuriHalInterruptIdEGPIO_PIN_3,
//     FuriHalInterruptIdEGPIO_PIN_4,
//     FuriHalInterruptIdEGPIO_PIN_5,
//     FuriHalInterruptIdEGPIO_PIN_6,
//     FuriHalInterruptIdEGPIO_PIN_7,
//     FuriHalInterruptIdQSPI,
//     FuriHalInterruptIdI2C1,
// #ifdef SLI_SI917B0
//     FuriHalInterruptIdMVP,
//     FuriHalInterruptIdMVP_WAKEUP,
// #endif
//     FuriHalInterrupIdI2S0,
//     FuriHalInterrupIdPLL_CLOCK,
//     FuriHalInterrupIdTASS_P2P,
    FuriHalInterruptIdMax,
} FuriHalInterruptId;
