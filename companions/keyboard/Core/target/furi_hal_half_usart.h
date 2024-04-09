#pragma once
#include "stm32g0xx.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FuriHalfUsartRxCallback)(uint8_t data);
void furi_hal_half_usart_init(void);
void furi_hal_half_usart_tx(uint8_t *buffer, uint32_t size);

#ifdef __cplusplus
}
#endif
