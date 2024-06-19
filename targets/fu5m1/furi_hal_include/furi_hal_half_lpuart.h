#pragma once

#include "stm32u5xx.h"
#include "stddef.h"

typedef enum {
    FuriHalHalfLPUartEventData = (1 << 0), /**< Data: new data available */
    FuriHalHalfLPUartEventIdle = (1 << 1), /**< Idle: bus idle detected */
    FuriHalHalfLPUartEventFrameError = (1 << 2), /**< Framing Error: incorrect frame detected */
    FuriHalHalfLPUartEventNoiseError = (1 << 3), /**< Noise Error: noise on the line detected */
    FuriHalHalfLPUartEventOverrunError =
        (1 << 4), /**< Overrun Error: no space for received data */
} FuriHalHalfLPUartEvent;

typedef void (*FuriHalHalfLPUartRxCallback)(FuriHalHalfLPUartEvent event, void* context);

void furi_hal_half_lpuart_init(void);
void furi_hal_half_lpuart_deinit(void);
void furi_hal_half_lpuart_tx(const uint8_t* buffer, size_t buffer_size);
void furi_hal_half_lpuart_set_rx_callback(FuriHalHalfLPUartRxCallback callback, void* context);
uint8_t furi_hal_half_lpuart_get_rx_data(void);
