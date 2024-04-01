#pragma once

#include <furi.h>
#include <furi_hal_gpio.h>

#include <stm32u5xx.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const GpioPin gpio_mco;
extern const GpioPin gpio_swdio;
extern const GpioPin gpio_swclk;
extern const GpioPin gpio_led;
extern const GpioPin gpio_log_usart_tx;

extern const GpioPin gpio_sd_card_power_switch;
extern const GpioPin gpio_sd_card_detect;

extern const GpioPin gpio_sd_card_d0;
extern const GpioPin gpio_sd_card_d1;
extern const GpioPin gpio_sd_card_d2;
extern const GpioPin gpio_sd_card_d3;
extern const GpioPin gpio_sd_card_ck;
extern const GpioPin gpio_sd_card_cmd;

void furi_hal_resources_init_early();

void furi_hal_resources_deinit_early();

void furi_hal_resources_init();

#ifdef __cplusplus
}
#endif
