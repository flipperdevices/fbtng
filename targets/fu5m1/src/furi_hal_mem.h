#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    FuriHalMemPoolSram1,
    FuriHalMemPoolSram2,
    FuriHalMemPoolSram3,
    FuriHalMemPoolSram5,
    FuriHalMemPoolSram6,
    FuriHalMemPoolSram4,
    FuriHalMemPoolPSram,
    FuriHalMemPoolMax,
} FuriHalMemPool;

typedef enum {
    FuriHalMemPoolCapsGFXMMU = 1 << 0,
    FuriHalMemPoolCapsGPU2D = 1 << 1,
    FuriHalMemPoolCapsLTDC = 1 << 2,
    FuriHalMemPoolCapsUSBOTGHS = 1 << 3,
    FuriHalMemPoolCapsSDMMC = 1 << 4,
    FuriHalMemPoolCapsSBus = 1 << 5,
    FuriHalMemPoolCapsICache = 1 << 6, // ?
} FuriHalMemPoolCaps;

char* furi_hal_mem_pool_get_name(FuriHalMemPool pool);

void* furi_hal_mem_pool_get_addr(FuriHalMemPool pool);

size_t furi_hal_mem_pool_get_size(FuriHalMemPool pool);

void furi_hal_mem_print_memory_layout(void);

#ifdef __cplusplus
}
#endif