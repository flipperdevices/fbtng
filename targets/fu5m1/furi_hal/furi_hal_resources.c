#include <furi_hal_resources.h>
#include <furi_hal_bus.h>

const GpioPin gpio_swdio = {.port = GPIOA, .pin = LL_GPIO_PIN_13};
const GpioPin gpio_swclk = {.port = GPIOA, .pin = LL_GPIO_PIN_14};
const GpioPin gpio_sd_card_power_switch = {.port = GPIOI, .pin = LL_GPIO_PIN_13};
const GpioPin gpio_sd_card_detect = {.port = GPIOI, .pin = LL_GPIO_PIN_14};

void furi_hal_resources_init_early() {
    furi_hal_bus_enable(FuriHalBusGPIOA);
    furi_hal_bus_enable(FuriHalBusGPIOB);
    furi_hal_bus_enable(FuriHalBusGPIOC);
    furi_hal_bus_enable(FuriHalBusGPIOD);
    furi_hal_bus_enable(FuriHalBusGPIOE);
#if defined(GPIOF)
    furi_hal_bus_enable(FuriHalBusGPIOF);
#endif
    furi_hal_bus_enable(FuriHalBusGPIOG);
    furi_hal_bus_enable(FuriHalBusGPIOH);
#if defined(GPIOI)
    furi_hal_bus_enable(FuriHalBusGPIOI);
#endif
#if defined(GPIOJ)
    furi_hal_bus_enable(FuriHalBusGPIOJ);
#endif

    // SD Card stepdown control
    furi_hal_gpio_write(&gpio_sd_card_power_switch, 0);
    furi_hal_gpio_init(
        &gpio_sd_card_power_switch, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);

    // SD Card detect
    furi_hal_gpio_init(&gpio_sd_card_detect, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
}

void furi_hal_resources_deinit_early() {
    furi_hal_bus_disable(FuriHalBusGPIOA);
    furi_hal_bus_disable(FuriHalBusGPIOB);
    furi_hal_bus_disable(FuriHalBusGPIOC);
    furi_hal_bus_disable(FuriHalBusGPIOD);
    furi_hal_bus_disable(FuriHalBusGPIOE);
#if defined(GPIOF)
    furi_hal_bus_disable(FuriHalBusGPIOF);
#endif
    furi_hal_bus_disable(FuriHalBusGPIOG);
    furi_hal_bus_disable(FuriHalBusGPIOH);
#if defined(GPIOI)
    furi_hal_bus_disable(FuriHalBusGPIOI);
#endif
#if defined(GPIOJ)
    furi_hal_bus_disable(FuriHalBusGPIOJ);
#endif
}