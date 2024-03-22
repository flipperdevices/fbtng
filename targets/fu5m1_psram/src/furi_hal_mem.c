#include "furi_hal_mem.h"
#include <stm32u5xx_hal.h>
#include "apsxx08l.h"
#include "dbg_log.h"

#define TAG "mem"

extern const void _sdata; // .data start
extern const void _edata; // .data end
extern const void _sbss; // .bss start
extern const void _ebss; // .bss end
extern const void __heap_start__; // .heap end
extern const void __heap_end__; // .heap end

const uint8_t* data_start = (const uint8_t*)&_sdata;
const uint8_t* data_end = (const uint8_t*)&_edata;
const uint8_t* bss_start = (const uint8_t*)&_sbss;
const uint8_t* bss_end = (const uint8_t*)&_ebss;
const uint8_t* heap_start = (const uint8_t*)&__heap_start__;
const uint8_t* heap_end = (const uint8_t*)&__heap_end__;

typedef struct FuriMemPool {
    const char* name;
    uint32_t* pool;
    size_t size;
    uint32_t caps;
} FuriMemPool;

// RM0456 "Figure 1. System architecture", page 130
static const FuriMemPool furi_hal_memory_pool[] = {
    {
        .name = "SRAM1",
        .pool = (uint32_t*)SRAM1_BASE_NS,
        .size = SRAM1_SIZE,
        .caps = FuriHalMemPoolCapsGPU2D | FuriHalMemPoolCapsSDMMC,
    },
    {
        .name = "SRAM2",
        .pool = (uint32_t*)SRAM2_BASE_NS,
        .size = SRAM2_SIZE,
        .caps = FuriHalMemPoolCapsSBus,
    },
    {
        .name = "SRAM3",
        .pool = (uint32_t*)SRAM3_BASE_NS,
        .size = SRAM3_SIZE,
        .caps = FuriHalMemPoolCapsUSBOTGHS | FuriHalMemPoolCapsSBus,
    },
    {
        .name = "SRAM5",
        .pool = (uint32_t*)SRAM5_BASE_NS,
        .size = SRAM5_SIZE,
        .caps = FuriHalMemPoolCapsGFXMMU,
    },
    {
        .name = "SRAM6",
        .pool = (uint32_t*)SRAM6_BASE_NS,
        .size = SRAM6_SIZE,
        .caps = 0,
    },
    {
        .name = "SRAM4",
        .pool = (uint32_t*)SRAM4_BASE_NS,
        .size = SRAM4_SIZE,
        .caps = 0,
    },
    {
        .name = "PSRAM",
        .pool = (uint32_t*)OCTOSPI1_BASE,
        .size = APS12808L_RAM_SIZE,
        .caps = FuriHalMemPoolCapsICache,
    },
};

size_t furi_hal_memory_pool_count = sizeof(furi_hal_memory_pool) / sizeof(FuriMemPool);

char* furi_hal_mem_pool_get_name(FuriHalMemPool pool) {
    assert_param(pool < furi_hal_memory_pool_count);
    return (char*)furi_hal_memory_pool[pool].name;
}

void* furi_hal_mem_pool_get_addr(FuriHalMemPool pool) {
    assert_param(pool < furi_hal_memory_pool_count);
    return furi_hal_memory_pool[pool].pool;
}

size_t furi_hal_mem_pool_get_size(FuriHalMemPool pool) {
    assert_param(pool < furi_hal_memory_pool_count);
    return furi_hal_memory_pool[pool].size;
}

void furi_hal_mem_print_memory_layout(void) {
    DBG_LOG_I(TAG, "Memory layout:");
    DBG_LOG_I(
        TAG,
        "  .data: %p - %p, size %lu bytes",
        data_start,
        data_end,
        (uint32_t)data_end - (uint32_t)data_start);
    DBG_LOG_I(
        TAG,
        "  .bss : %p - %p, size %lu bytes",
        bss_start,
        bss_end,
        (uint32_t)bss_end - (uint32_t)bss_start);
    DBG_LOG_I(
        TAG,
        "  .heap: %p - %p, size %lu bytes",
        heap_start,
        heap_end,
        (uint32_t)heap_end - (uint32_t)heap_start);
    for(size_t i = 0; i < furi_hal_memory_pool_count; i++) {
        DBG_LOG_I(
            TAG,
            "  %s: %p - %p %u kb",
            furi_hal_memory_pool[i].name,
            furi_hal_memory_pool[i].pool,
            (void*)((size_t)furi_hal_memory_pool[i].pool + furi_hal_memory_pool[i].size),
            furi_hal_memory_pool[i].size / 1024);
    }
}