#include <furi.h>
#include <furi_hal.h>

#define TAG "Main"

static int32_t init_task(void* context) {
    UNUSED(context);

    furi_hal_init();
    furi_log_set_level(FuriLogLevelDebug);

    static volatile uint32_t dummy;

    for(;;) {
        dummy++;
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
