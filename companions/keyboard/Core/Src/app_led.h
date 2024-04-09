#pragma once

#include "stm32g0xx.h"
#include <stddef.h>
#include "furi_hal_led.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t data_led[];
extern const size_t data_led_size;

extern const FuriHalLed data_rgb[];
extern const size_t data_rgb_size;

#ifdef __cplusplus
}
#endif
