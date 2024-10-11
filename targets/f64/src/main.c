#include <furi.h>
#include <furi_hal.h>

#define TAG "Main"

static int32_t init_task(void* context) {
    UNUSED(context);

    furi_hal_init();
    furi_log_set_level(FuriLogLevelDebug);

    // GPIO_10
    const GpioPin gpio = {
        5,
        0,
        10,
    };

    furi_hal_bus_enable(FuriHalBusEGPIO_CLK);

    furi_hal_gpio_init_simple(&gpio, GpioModeOutputPushPull);

    for(;;) {
        furi_hal_gpio_write(&gpio, !furi_hal_gpio_read(&gpio));
        furi_delay_ms(500);
    }

    return 0;
}

int main(void) {
    furi_hal_init_early();

    furi_init();

    FuriThread* main_thread = furi_thread_alloc_ex("Init", 4096, init_task, NULL);
    furi_thread_start(main_thread);

    furi_run();

    furi_crash("Kernel is Dead");
}

void abort(void) {
    furi_crash("AbortHandler");
}
