#include <furi_hal.h>

#define TAG "FuriHal"

void furi_hal_init_early() {
    furi_hal_cortex_init_early();
    // furi_hal_clock_init_early();
    furi_hal_bus_init_early();
    // furi_hal_dma_init_early();
    furi_hal_resources_init_early();
    // furi_hal_os_init();
    // furi_hal_spi_config_init_early();
    // furi_hal_i2c_init_early();
    // furi_hal_light_init();
    // furi_hal_rtc_init_early();
}

void furi_hal_deinit_early() {
    // furi_hal_rtc_deinit_early();
    // furi_hal_i2c_deinit_early();
    // furi_hal_spi_config_deinit_early();
    furi_hal_resources_deinit_early();
    // furi_hal_dma_deinit_early();
    furi_hal_bus_deinit_early();
    // furi_hal_clock_deinit_early();
}