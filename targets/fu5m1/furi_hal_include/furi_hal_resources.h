#pragma once

#include <furi.h>
#include <furi_hal_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const GpioPin gpio_mco;
extern const GpioPin gpio_swdio;
extern const GpioPin gpio_swclk;
extern const GpioPin gpio_led;
extern const GpioPin gpio_log_usart_tx;
extern const GpioPin gpio_usb_dm;
extern const GpioPin gpio_usb_dp;
extern const GpioPin gpio_log_usart_rx;

extern const GpioPin gpio_uart5_tx;
extern const GpioPin gpio_uart5_rx;

extern const GpioPin gpio_test_pc0;
extern const GpioPin gpio_test_pc1;

extern const GpioPin gpio_octospi1_psram_io0;
extern const GpioPin gpio_octospi1_psram_io1;
extern const GpioPin gpio_octospi1_psram_io2;
extern const GpioPin gpio_octospi1_psram_io3;
extern const GpioPin gpio_octospi1_psram_io4;
extern const GpioPin gpio_octospi1_psram_io5;
extern const GpioPin gpio_octospi1_psram_io6;
extern const GpioPin gpio_octospi1_psram_io7;
extern const GpioPin gpio_octospi1_psram_clk;
extern const GpioPin gpio_octospi1_psram_ncs;
extern const GpioPin gpio_octospi1_psram_dqs;

extern const GpioPin gpio_sd_card_power_switch;
extern const GpioPin gpio_sd_card_detect;

extern const GpioPin gpio_sd_card_d0;
extern const GpioPin gpio_sd_card_d1;
extern const GpioPin gpio_sd_card_d2;
extern const GpioPin gpio_sd_card_d3;
extern const GpioPin gpio_sd_card_ck;
extern const GpioPin gpio_sd_card_cmd;

extern const GpioPin gpio_pssi_pdck;
extern const GpioPin gpio_pssi_rdy;
extern const GpioPin gpio_pssi_de;
extern const GpioPin gpio_pssi_d0;
extern const GpioPin gpio_pssi_d1;
extern const GpioPin gpio_pssi_d2;
extern const GpioPin gpio_pssi_d3;
extern const GpioPin gpio_pssi_d4;
extern const GpioPin gpio_pssi_d5;
extern const GpioPin gpio_pssi_d6;
extern const GpioPin gpio_pssi_d7;
extern const GpioPin gpio_pssi_d8;
extern const GpioPin gpio_pssi_d9;
extern const GpioPin gpio_pssi_d10;
extern const GpioPin gpio_pssi_d11;
extern const GpioPin gpio_pssi_d12;
extern const GpioPin gpio_pssi_d13;
extern const GpioPin gpio_pssi_d14;
extern const GpioPin gpio_pssi_d15;

void furi_hal_resources_init_early(void);

void furi_hal_resources_deinit_early(void);

void furi_hal_resources_init(void);

#ifdef __cplusplus
}
#endif
