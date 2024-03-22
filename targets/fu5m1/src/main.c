#include <furi.h>
#include <furi_hal.h>

int main(void) {
    // Initialize FURI layer
    furi_init();

    // Flipper critical FURI HAL
    furi_hal_init_early();
}