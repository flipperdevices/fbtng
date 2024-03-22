#pragma once

#include "stm32u5xx.h"

typedef void (*FuriHalPssiRxCallback)(uint8_t* buf, uint16_t size, void* context);

void furi_hal_pssi_init_bus8line(uint16_t buf_size);
void furi_hal_pssi_deinit(void);
void furi_hal_pssi_set_rx_callback(FuriHalPssiRxCallback callback, void* context);
void furi_hal_pssi_dma_receve_start(void);
void furi_hal_pssi_dma_receve_stop(void);
