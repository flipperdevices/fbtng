/**
 * @file furi_hal_memory.h
 * Memory HAL API
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init memory pool manager
 */
void furi_hal_memory_init(void);

/**
 * @brief Allocate memory from separate memory pool. That memory can't be freed.
 *
 * @param size
 * @return void*
 */
void* furi_hal_memory_alloc(size_t size);

/**
 * @brief Get free memory pool size
 *
 * @return size_t
 */
size_t furi_hal_memory_get_free(void);

/**
 * @brief Get max free block size from memory pool
 *
 * @return size_t
 */
size_t furi_hal_memory_max_pool_block(void);

typedef struct {
    void* start;
    void* end;
    size_t size_bytes;
} FuriHalMemoryRegion;

typedef enum {
    FuriHalMemoryRegionIdHeap,
} FuriHalMemoryRegionId;

/** Get memory region information
 *
 * @param[in]  index  Region number
 * @return     Details of specified memory region
 */
const FuriHalMemoryRegion* furi_hal_memory_get_region(uint32_t index);

/** Get memory region count
 *
 * @return      Number of available memory regions
 */
uint32_t furi_hal_memory_get_region_count(void);

/** Check whether the address is within the given region
 *
 * @param[in] address The address to be checked
 * @param[in] index Region number
 * @return @c true if the address is within the region (inclusive), @c false otherwise
 */
bool furi_hal_memory_is_address_in_region(const void* address, uint32_t index);

typedef enum {
    FuriHalMemoryHeapTrackModeNone = 0, /**< Disable allocation tracking */
    FuriHalMemoryHeapTrackModeMain, /**< Enable allocation tracking for main application thread */
    FuriHalMemoryHeapTrackModeTree, /**< Enable allocation tracking for main and children application threads */
    FuriHalMemoryHeapTrackModeAll, /**< Enable allocation tracking for all threads */
} FuriHalMemoryHeapTrackMode;

/** Set Heap Track mode
 *
 * @param[in]  mode  The mode to set
 */
void furi_hal_memory_set_heap_track_mode(FuriHalMemoryHeapTrackMode mode);

/** Get RTC Heap Track mode
 *
 * @return     The RTC heap track mode.
 */
FuriHalMemoryHeapTrackMode furi_hal_memory_get_heap_track_mode(void);

#ifdef __cplusplus
}
#endif
