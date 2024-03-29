#include <furi.h>
#include <furi_hal.h>
#include <flipper.h>

#define TAG "Main"

int32_t init_task(void* context) {
    UNUSED(context);

    // Flipper FURI HAL
    furi_hal_init();

    // Init flipper
    flipper_init();

    return 0;
}

int main(void) {
    // Initialize FURI layer
    furi_init();
    furi_log_set_level(FuriLogLevelDebug);

    // Flipper critical FURI HAL
    furi_hal_init_early();

    FuriThread* main_thread = furi_thread_alloc_ex("Init", 4096, init_task, NULL);

    furi_thread_start(main_thread);

    // Run Kernel
    furi_run();

    furi_crash("Kernel is Dead");
}