#include <core/log.h>
#include <furi_hal_bus.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>
#include <stm32u5xx_ll_rcc.h>
#include <stm32u5xx_ll_usart.h>

static void todo_uart_init(void) {
    furi_hal_bus_enable(FuriHalBusUSART1);

    furi_hal_gpio_init_ex(
        &gpio_log_usart_tx,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedLow,
        GpioAltFn7USART1);

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);

    LL_USART_ConfigCharacter(
        USART1, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);
    LL_USART_SetTransferDirection(USART1, LL_USART_DIRECTION_TX);
    LL_USART_SetHWFlowCtrl(USART1, LL_USART_HWCONTROL_NONE);
    LL_USART_SetBaudRate(
        USART1, SystemCoreClock, LL_USART_PRESCALER_DIV1, LL_USART_OVERSAMPLING_16, 230400);

    LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_EnableFIFO(USART1);
    LL_USART_ConfigAsyncMode(USART1);
    LL_USART_Enable(USART1);
}

static void todo_uart_tx(uint8_t data) {
    while(LL_USART_IsActiveFlag_TXE_TXFNF(USART1) == 0) {
    }
    LL_USART_TransmitData8(USART1, data);
}

void todo_uart_log_cb(const uint8_t* data, size_t size, void* context) {
    UNUSED(context);
    for(size_t i = 0; i < size; i++) {
        todo_uart_tx(data[i]);
    }
}

void furi_hal_todo_init(void) {
    todo_uart_init();

    FuriLogHandler todo_uart_log_handler = {
        .callback = todo_uart_log_cb,
        .context = NULL,
    };
    furi_log_add_handler(todo_uart_log_handler);

    FURI_LOG_I("TodoLog", "Init OK");
}