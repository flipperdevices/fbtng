#include <furi.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>

#define TAG "Blinker"

int32_t blinker(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Started");

    furi_hal_gpio_init(&gpio_led, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    while(1) {
        furi_hal_gpio_write(&gpio_led, 1);
        furi_delay_ms(500);
        furi_hal_gpio_write(&gpio_led, 0);
        furi_delay_ms(500);
    }

    return 0;
}