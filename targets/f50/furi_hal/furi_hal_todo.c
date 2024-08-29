#include <core/log.h>
#include <furi_hal_bus.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>
#include <stm32u5xx_ll_rcc.h>
#include <stm32u5xx_ll_usart.h>

typedef struct {
    USART_TypeDef* usart;
} UartDriver;

UartDriver* driver_uart_init(
    USART_TypeDef* usart,
    const GpioPin* gpio,
    const GpioAltFn alt_fn,
    uint32_t baudrate) {
    FuriHalBus bus;
    uint32_t clock_source;

    switch((uint32_t)usart) {
    case(uint32_t)USART1:
        bus = FuriHalBusUSART1;
        clock_source = LL_RCC_USART1_CLKSOURCE_PCLK2;
        break;

    default:
        furi_crash("Unknown USART");
        break;
    }

    UartDriver* driver = malloc(sizeof(UartDriver));
    driver->usart = usart;

    furi_hal_bus_enable(bus);
    furi_hal_gpio_init_ex(gpio, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedLow, alt_fn);

    LL_RCC_SetUSARTClockSource(clock_source);

    LL_USART_ConfigCharacter(
        usart, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);
    LL_USART_SetTransferDirection(usart, LL_USART_DIRECTION_TX);
    LL_USART_SetHWFlowCtrl(usart, LL_USART_HWCONTROL_NONE);
    LL_USART_SetBaudRate(
        usart, SystemCoreClock, LL_USART_PRESCALER_DIV1, LL_USART_OVERSAMPLING_16, baudrate);

    LL_USART_SetTXFIFOThreshold(usart, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_EnableFIFO(usart);
    LL_USART_ConfigAsyncMode(usart);
    LL_USART_Enable(usart);

    return driver;
}

void driver_uart_tx(UartDriver* driver, uint8_t data) {
    while(LL_USART_IsActiveFlag_TXE_TXFNF(driver->usart) == 0) {
    }
    LL_USART_TransmitData8(driver->usart, data);
}

void todo_uart_log_cb(const uint8_t* data, size_t size, void* context) {
    UartDriver* driver = context;
    for(size_t i = 0; i < size; i++) {
        driver_uart_tx(driver, data[i]);
    }
}

void furi_hal_todo_init(void) {
    UartDriver* driver = driver_uart_init(USART1, &gpio_log_usart_tx, GpioAltFn7USART1, 230400);

    FuriLogHandler todo_uart_log_handler = {
        .callback = todo_uart_log_cb,
        .context = driver,
    };
    furi_log_add_handler(todo_uart_log_handler);

    FURI_LOG_I("TodoLog", "Init OK");
}
