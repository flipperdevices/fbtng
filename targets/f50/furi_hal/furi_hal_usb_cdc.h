#pragma once

#include <stdint.h>
#include "furi_hal_usb.h"

#define CDC_MAX_PACKET_LEN (TUD_OPT_HIGH_SPEED ? 512 : 64)

typedef struct FURI_PACKED {
    uint32_t bit_rate;
    uint8_t stop_bits; ///< 0: 1 stop bit - 1: 1.5 stop bits - 2: 2 stop bits
    uint8_t parity; ///< 0: None - 1: Odd - 2: Even - 3: Mark - 4: Space
    uint8_t data_bits; ///< can be 5, 6, 7, 8 or 16
} CdcLineCoding;

typedef struct {
    void (*tx_done_callback)(uint8_t itf, void* context);
    void (*rx_callback)(uint8_t itf, void* context);
    void (*ctrl_line_callback)(uint8_t itf, bool dtr, bool rts, void* context);
    void (*config_callback)(uint8_t itf, CdcLineCoding* config, void* context);
    void (*send_break_callback)(uint8_t itf, uint16_t duration_ms, void* context);
} CdcCallbacks;

typedef struct {
    const CdcCallbacks* callbacks;
    void* context;
} CdcContext;

extern FuriHalUsbInterface usb_cdc;

bool furi_hal_cdc_is_connected(uint8_t itf);

uint8_t furi_hal_cdc_get_line_state(uint8_t itf);

void furi_hal_cdc_get_line_coding(uint8_t itf, CdcLineCoding* coding);

uint32_t furi_hal_cdc_available(uint8_t itf);

uint32_t furi_hal_cdc_read(uint8_t itf, void* buffer, uint32_t bufsize);

bool furi_hal_cdc_peek(uint8_t itf, uint8_t* chr);

void furi_hal_cdc_read_flush(uint8_t itf);

uint32_t furi_hal_cdc_write(uint8_t itf, void const* buffer, uint32_t bufsize);

uint32_t furi_hal_cdc_write_flush(uint8_t itf);

uint32_t furi_hal_cdc_write_available(uint8_t itf);

bool furi_hal_cdc_write_clear(uint8_t itf);
