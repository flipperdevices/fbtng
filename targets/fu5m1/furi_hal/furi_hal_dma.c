#include <furi_hal_dma.h>
#include <furi_hal_bus.h>

void furi_hal_dma_init_early(void) {
    furi_hal_bus_enable(FuriHalBusGPDMA1);
    furi_hal_bus_enable(FuriHalBusLPDMA1);
    //furi_hal_bus_enable(FuriHalBusDMA2D);
}

void furi_hal_dma_deinit_early(void) {
    furi_hal_bus_disable(FuriHalBusGPDMA1);
    furi_hal_bus_disable(FuriHalBusLPDMA1);
    //furi_hal_bus_disable(FuriHalBusDMA2D);
}

