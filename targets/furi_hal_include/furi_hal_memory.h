/**
 * @file furi_hal_memory.h
 * Memory HAL API
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Init control structures 
 * @note Must be done before first allocation
 */
void furi_hal_memory_init_early(void);

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
    size_t size_bytes;
} FuriHalMemoryRegion;

/** Get memory region count
 *
 * @return      Number of available memory regions
 */
size_t furi_hal_memory_regions_count(void);

/** Get memory region information
 *
 * @param[in]  index  Region number
 * @return     Details of specified memory region
 */
const FuriHalMemoryRegion* furi_hal_memory_regions_get(size_t index);

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
