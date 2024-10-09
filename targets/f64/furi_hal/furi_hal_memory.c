#include <furi_hal_memory.h>
#include <furi.h>

#include <si917_linker.h>

static FuriHalMemoryRegion memory_regions[] = {
    [FuriHalMemoryRegionIdHeap] = {
        .start = (void*)&__heap_start__,
        .size_bytes = 0,
    },
};

void furi_hal_memory_init(void) {
}

void* furi_hal_memory_alloc(size_t size) {
    UNUSED(size);
    return NULL;
}

size_t furi_hal_memory_get_free(void) {
    return 0;
}

size_t furi_hal_memory_max_pool_block(void) {
    return 0;
}

void furi_hal_memory_init_early(void) {
    memory_regions[FuriHalMemoryRegionIdHeap].size_bytes = &__heap_end__ - &__heap_start__;
}

uint32_t furi_hal_memory_get_region_count(void) {
    return COUNT_OF(memory_regions);
}

const FuriHalMemoryRegion* furi_hal_memory_get_region(uint32_t index) {
    furi_check(index < COUNT_OF(memory_regions));
    return &memory_regions[index];
}

void furi_hal_memory_set_heap_track_mode(FuriHalMemoryHeapTrackMode mode) {
    UNUSED(mode);
}

FuriHalMemoryHeapTrackMode furi_hal_memory_get_heap_track_mode(void) {
    return FuriHalMemoryHeapTrackModeNone;
}
