#pragma once

#include <furi.h>
#include <furi_hal_gpio.h>

#include <stm32u5xx.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const GpioPin gpio_swdio;
extern const GpioPin gpio_swclk;

#ifdef __cplusplus
}
#endif
