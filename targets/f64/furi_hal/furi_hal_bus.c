#include <furi_hal_bus.h>
#include <furi.h>

// #define FURI_HAL_BUS_IGNORE (0x0U)

void furi_hal_bus_init_early(void) {
    FURI_CRITICAL_ENTER();

    FURI_CRITICAL_EXIT();
}

void furi_hal_bus_deinit_early(void) {
    FURI_CRITICAL_ENTER();

    FURI_CRITICAL_EXIT();
}

void furi_hal_bus_enable(FuriHalBus bus) {
    UNUSED(bus);
    // furi_check(bus < FuriHalBusMAX);

    FURI_CRITICAL_ENTER();

    FURI_CRITICAL_EXIT();
}

void furi_hal_bus_reset(FuriHalBus bus) {
    UNUSED(bus);
    // furi_check(bus < FuriHalBusMAX);

    FURI_CRITICAL_ENTER();

    FURI_CRITICAL_EXIT();
}

void furi_hal_bus_disable(FuriHalBus bus) {
    UNUSED(bus);
    // furi_check(bus < FuriHalBusMAX);

    FURI_CRITICAL_ENTER();

    FURI_CRITICAL_EXIT();
}

bool furi_hal_bus_is_enabled(FuriHalBus bus) {
    UNUSED(bus);
    // furi_check(bus < FuriHalBusMAX);

    bool ret = false;
    FURI_CRITICAL_ENTER();

    FURI_CRITICAL_EXIT();

    return ret;
}
