#include "furi_hal_half_lpuart.h"

#include "furi_hal_resources.h"
#include "furi_hal_interrupt.h"
#include "furi_hal_bus.h"

#include "stm32u5xx_ll_rcc.h"
#include "stm32u5xx_ll_lpuart.h"

typedef struct {
    FuriHalHalfLPUartRxCallback callback;
    void* context;
} FuriHalHalfUsart;

static FuriHalHalfUsart furi_hal_half_usart = {0};

void furi_hal_half_lpuart_irq(void* context) {
    FuriHalHalfLPUartEvent event = 0;
    // Notification flags
    if(LPUART1->ISR & USART_ISR_RXNE_RXFNE) {
        event |= FuriHalHalfLPUartEventData;
    }
    if(LPUART1->ISR & USART_ISR_IDLE) {
        LPUART1->ICR = USART_ICR_IDLECF;
        event |= FuriHalHalfLPUartEventIdle;
    }
    // Error flags
    if(LPUART1->ISR & USART_ISR_ORE) {
        LPUART1->ICR = USART_ICR_ORECF;
        event |= FuriHalHalfLPUartEventOverrunError;
    }

    if(furi_hal_half_usart.callback) {
        furi_hal_half_usart.callback(event, furi_hal_half_usart.context);
    } else {
        //clear USART_ISR_RXNE_RXFNE flag
        LL_LPUART_ReceiveData8(LPUART1);
    }
}

void furi_hal_half_lpuart_init(void) {
    //LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_LSE); //max 9600
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_SYSCLK);

    /* Peripheral clock enable */
    furi_hal_bus_enable(FuriHalBusLPUART1);

    furi_hal_gpio_init_ex(
        &gpio_half_lpuart_tx_rx,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedLow,
        GpioAltFn8LPUART1);

    /* LPUART1 interrupt Init */
    furi_hal_interrupt_set_isr(FuriHalInterruptIdLPUART1, furi_hal_half_lpuart_irq, NULL);

    LL_LPUART_ConfigCharacter(
        LPUART1, LL_LPUART_DATAWIDTH_8B, LL_LPUART_PARITY_NONE, LL_LPUART_STOPBITS_1);
    LL_LPUART_SetTransferDirection(LPUART1, LL_LPUART_DIRECTION_TX_RX);
    LL_LPUART_SetHWFlowCtrl(LPUART1, LL_LPUART_HWCONTROL_NONE);
    //if use LL_RCC_LPUART1_CLKSOURCE_LSE then max baudrate is 9600 and replace SystemCoreClock with 32768
    LL_LPUART_SetBaudRate(LPUART1, SystemCoreClock, LL_LPUART_PRESCALER_DIV1, 115200);

    LL_LPUART_SetTXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_SetRXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_EnableHalfDuplex(LPUART1);
    LL_LPUART_DisableFIFO(LPUART1);
    LL_LPUART_EnableOverrunDetect(LPUART1);
    LL_LPUART_EnableDMADeactOnRxErr(LPUART1);

    LL_LPUART_SetTransferDirection(USART2, LL_LPUART_DIRECTION_RX);
    LL_LPUART_EnableIT_RXNE_RXFNE(LPUART1);
    LL_LPUART_Enable(LPUART1);

    while(!LL_LPUART_IsActiveFlag_TEACK(LPUART1) || !LL_LPUART_IsActiveFlag_REACK(LPUART1));
}

void furi_hal_half_lpuart_deinit(void) {
    LL_LPUART_Disable(LPUART1);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdLPUART1, NULL, NULL);
    furi_hal_gpio_init(&gpio_half_lpuart_tx_rx, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_bus_disable(FuriHalBusLPUART1);
}

void furi_hal_half_lpuart_tx(const uint8_t* buffer, size_t buffer_size) {
    if(LL_LPUART_IsEnabled(LPUART1) == 0) return;

    LL_LPUART_SetTransferDirection(LPUART1, LL_LPUART_DIRECTION_TX);
    while(buffer_size > 0) {
        while(!LL_LPUART_IsActiveFlag_TXE(LPUART1));

        LL_LPUART_TransmitData8(LPUART1, *buffer);

        buffer++;
        buffer_size--;
    }
    // Wait Tx Complete
    while(!LL_LPUART_IsActiveFlag_TC(LPUART1));
    LL_LPUART_SetTransferDirection(LPUART1, LL_LPUART_DIRECTION_RX);
}

void furi_hal_half_lpuart_set_rx_callback(FuriHalHalfLPUartRxCallback callback, void* context) {
    furi_hal_half_usart.callback = callback;
    furi_hal_half_usart.context = context;
}

uint8_t furi_hal_half_lpuart_get_rx_data(void) {
    return LL_LPUART_ReceiveData8(LPUART1);
}
