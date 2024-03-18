#include "st_hal.h"
#include "dbg_log.h"
#include "printf.h"
#include <string.h>

static DbgLogLevel log_level_set = DBG_LOG_LVL_NONE;

void dbg_log(DbgLogLevel level, const char* tag, const char* format, ...) {
    if(level > log_level_set) {
        return;
    }
    char level_letter = '?';
    if(level == DBG_LOG_LVL_ERROR) {
        level_letter = 'E';
    } else if(level == DBG_LOG_LVL_WARNING) {
        level_letter = 'W';
    } else if(level == DBG_LOG_LVL_INFO) {
        level_letter = 'I';
    }

    uint32_t timestamp = 0; //chTimeI2MS(chVTGetSystemTime());

    printf_("[%c] %lu.%03lu [%s]: ", level_letter, timestamp / 1000, timestamp % 1000, tag);

    va_list ap;
    va_start(ap, format);
    vprintf_(format, ap);
    va_end(ap);

    printf_("\r\n");
}

void dbg_dump_line(uint32_t addr, const uint8_t* data, uint32_t len) {
    printf_("\t%08lX: ", addr);

    char text_line[17];
    memset(text_line, 0, 17);

    for(uint8_t i = 0; i < 16; i++) {
        if(i < len) {
            printf_("%02X ", data[i]);
            if((data[i] < 0x20) || (data[i] >= 0x7F)) {
                text_line[i] = '.';
            } else {
                text_line[i] = data[i];
            }
        } else {
            printf_("   ");
        }
    }

    printf_("| %s\r\n", text_line);
}

void dbg_dump(DbgLogLevel level, const char* tag, const uint8_t* data, uint32_t len) {
    if(level > log_level_set) {
        return;
    }
    char level_letter = '?';
    if(level == DBG_LOG_LVL_ERROR) {
        level_letter = 'E';
    } else if(level == DBG_LOG_LVL_WARNING) {
        level_letter = 'W';
    } else if(level == DBG_LOG_LVL_INFO) {
        level_letter = 'I';
    }

    uint32_t timestamp = 0; //chTimeI2MS(chVTGetSystemTime());

    printf_(
        "%lu.%03lu [%c][%s]: Dump len: %lu\r\n",
        timestamp / 1000,
        timestamp % 1000,
        level_letter,
        tag,
        len);

    uint32_t addr = 0;

    for(addr = 0; addr < len; addr += 16) {
        uint32_t line_len = len - addr;
        if(line_len > 16) {
            line_len = 16;
        }
        dbg_dump_line(addr, &data[addr], line_len);
    }

    printf_("\r\n");
}

void dbg_log_init(DbgLogLevel level_max) {
    log_level_set = level_max;

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

    LL_GPIO_InitTypeDef uart_gpio = {0};
    uart_gpio.Pin = LL_GPIO_PIN_9;
    uart_gpio.Mode = LL_GPIO_MODE_ALTERNATE;
    uart_gpio.Speed = LL_GPIO_SPEED_FREQ_LOW;
    uart_gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    uart_gpio.Pull = LL_GPIO_PULL_NO;
    uart_gpio.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &uart_gpio);

    LL_USART_InitTypeDef uart_cfg = {0};
    uart_cfg.PrescalerValue = LL_USART_PRESCALER_DIV1;
    uart_cfg.BaudRate = 230400;
    uart_cfg.DataWidth = LL_USART_DATAWIDTH_8B;
    uart_cfg.StopBits = LL_USART_STOPBITS_1;
    uart_cfg.Parity = LL_USART_PARITY_NONE;
    uart_cfg.TransferDirection = LL_USART_DIRECTION_TX;
    uart_cfg.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    uart_cfg.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &uart_cfg);

    LL_USART_SetTXFIFOThreshold(USART1, LL_USART_FIFOTHRESHOLD_1_8);
    LL_USART_EnableFIFO(USART1);
    LL_USART_ConfigAsyncMode(USART1);
    LL_USART_Enable(USART1);

    printf_("\r\n");
}

static inline void console_uart_tx(uint8_t data) {
    while(LL_USART_IsActiveFlag_TXE_TXFNF(USART1) == 0) {
    }
    LL_USART_TransmitData8(USART1, data);
}

void putchar_(char c) {
    console_uart_tx(c);
}
