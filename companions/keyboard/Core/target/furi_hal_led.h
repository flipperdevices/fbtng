#pragma once
#include "stm32g0xx.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t green;
	uint8_t blue;
	uint8_t red;
}FuriHalLed;

void furi_hal_led_init(void);
void furi_hal_led_deinit(void);
void furi_hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

void furi_hal_led_dma_tim_init(uint16_t update_ms, FuriHalLed *data, size_t size);
void furi_hal_led_dma_tim_deinit(void);

#ifdef __cplusplus
}
#endif
