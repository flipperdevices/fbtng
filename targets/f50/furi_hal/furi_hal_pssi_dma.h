#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef void (*FuriHalPssiRxCallback)(uint8_t* buf, uint16_t size, void* context);

bool furi_hal_pssi_init_bus8line(uint16_t buf_size);
void furi_hal_pssi_deinit(void);
void furi_hal_pssi_set_rx_callback(FuriHalPssiRxCallback callback, void* context);
void furi_hal_pssi_dma_receive_start(void);
void furi_hal_pssi_dma_receive_stop(void);
