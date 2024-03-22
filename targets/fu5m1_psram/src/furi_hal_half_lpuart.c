#include "furi_hal_half_lpuart.h"

#include "stm32u5xx_ll_bus.h"
#include "stm32u5xx_ll_gpio.h"
#include "stm32u5xx_ll_rcc.h"
#include "stm32u5xx_ll_lpuart.h"
#include "stm32u5xx_ll_pwr.h"

typedef struct {
    FuriHalHalfLPUartRxCallback callback;
    void* context;
} FuriHalHalfUsart;

static FuriHalHalfUsart furi_hal_half_usart = {0};

void furi_hal_half_lpuart_init(void) {
    LL_LPUART_InitTypeDef LPUART_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    //LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_LSE); //max 9600
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_SYSCLK);

    /* Peripheral clock enable */
    LL_APB3_GRP1_EnableClock(LL_APB3_GRP1_PERIPH_LPUART1);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOG);
    LL_PWR_EnableVDDIO2();
    /**LPUART1 GPIO Configuration
	 PG7   ------> LPUART1_TX
	 */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
    LL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* LPUART1 interrupt Init */
    NVIC_SetPriority(LPUART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 4, 0));
    NVIC_EnableIRQ(LPUART1_IRQn);

    LPUART_InitStruct.PrescalerValue = LL_LPUART_PRESCALER_DIV1;
    LPUART_InitStruct.BaudRate = 115200;
    LPUART_InitStruct.DataWidth = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct.StopBits = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct.Parity = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    LL_LPUART_Init(LPUART1, &LPUART_InitStruct);
    LL_LPUART_SetTXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_SetRXFIFOThreshold(LPUART1, LL_LPUART_FIFOTHRESHOLD_1_8);
    LL_LPUART_EnableHalfDuplex(LPUART1);
    LL_LPUART_DisableFIFO(LPUART1);
    LL_LPUART_EnableOverrunDetect(LPUART1);
    LL_LPUART_EnableDMADeactOnRxErr(LPUART1);

    LL_LPUART_SetTransferDirection(USART2, LL_LPUART_DIRECTION_RX);
    LL_LPUART_EnableIT_RXNE_RXFNE(LPUART1);
    LL_LPUART_Enable(LPUART1);

    while(!LL_LPUART_IsActiveFlag_TEACK(LPUART1) || !LL_LPUART_IsActiveFlag_REACK(LPUART1))
        ;
}

void furi_hal_half_lpuart_tx(const uint8_t* buffer, size_t buffer_size) {
    if(LL_LPUART_IsEnabled(LPUART1) == 0) return;

    LL_LPUART_SetTransferDirection(LPUART1, LL_LPUART_DIRECTION_TX);
    while(buffer_size > 0) {
        while(!LL_LPUART_IsActiveFlag_TXE(LPUART1))
            ;

        LL_LPUART_TransmitData8(LPUART1, *buffer);

        buffer++;
        buffer_size--;
    }
    // Wait Tx Complete
    while(!LL_LPUART_IsActiveFlag_TC(LPUART1))
        ;
    LL_LPUART_SetTransferDirection(LPUART1, LL_LPUART_DIRECTION_RX);
}

void furi_hal_half_lpuart_set_rx_callback(FuriHalHalfLPUartRxCallback callback, void* context) {
    furi_hal_half_usart.callback = callback;
    furi_hal_half_usart.context = context;
}

uint8_t furi_hal_half_lpuart_get_rx_data(void) {
    return LL_LPUART_ReceiveData8(LPUART1);
}

void LPUART1_IRQHandler(void) {
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
