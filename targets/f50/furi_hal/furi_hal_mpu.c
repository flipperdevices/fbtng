#include <core/check.h>
#include <core/log.h>
#include <furi_hal_mpu.h>
#include <stm32u5xx_ll_cortex.h>

#define TAG "MPU"

typedef enum {
    FuriHalMpuRegionNULL = 0x00, // region 0 used to protect null pointer dereference
    FuriHalMpuRegionStack = 0x01, // region 1 used to protect stack
} FuriHalMpuRegion;

void furi_hal_mpu_enable(void) {
    LL_MPU_Enable(LL_MPU_CTRL_PRIVILEGED_DEFAULT);
}

void furi_hal_mpu_disable(void) {
    LL_MPU_Disable();
}

void furi_hal_mpu_protect_no_access(FuriHalMpuRegion region, uint32_t address, uint32_t size) {
    // check that address and size is aligned to 32 bytes
    furi_check((address & 0x1F) == 0);
    furi_check((size & 0x1F) == 0);

    uint32_t indirect_attributes_index = region;
    uint32_t indirect_attributes = LL_MPU_NO_ALLOCATE;
    uint32_t attributes = LL_MPU_INSTRUCTION_ACCESS_DISABLE | LL_MPU_ACCESS_NOT_SHAREABLE |
                          LL_MPU_REGION_PRIV_RO;

    furi_hal_mpu_disable();
    LL_MPU_ConfigAttributes(indirect_attributes_index, indirect_attributes);
    LL_MPU_ConfigRegion(region, attributes, indirect_attributes_index, address, address + size);
    furi_hal_mpu_enable();
}

void furi_hal_mpu_protect_read_only(FuriHalMpuRegion region, uint32_t address, uint32_t size) {
    // check that address and size is aligned to 32 bytes
    furi_check((address & 0x1F) == 0);
    furi_check((size & 0x1F) == 0);

    uint32_t indirect_attributes_index = region;
    uint32_t indirect_attributes = LL_MPU_R_ALLOCATE;
    uint32_t attributes = LL_MPU_INSTRUCTION_ACCESS_ENABLE | LL_MPU_ACCESS_NOT_SHAREABLE |
                          LL_MPU_REGION_ALL_RO;

    furi_hal_mpu_disable();
    LL_MPU_ConfigAttributes(indirect_attributes_index, indirect_attributes);
    LL_MPU_ConfigRegion(region, attributes, indirect_attributes_index, address, address + size);
    furi_hal_mpu_enable();
}

void furi_hal_mpu_protect_disable(FuriHalMpuRegion region) {
    furi_hal_mpu_disable();
    LL_MPU_DisableRegion(region);
    furi_hal_mpu_enable();
}

void furi_hal_mpu_set_stack_protection(uint32_t* stack) {
    // Protection area address must be aligned to region size
    uint32_t stack_ptr = (uint32_t)stack;
    uint32_t mask = ((1 << (0x04 + 2)) - 1);
    stack_ptr &= ~mask;
    if(stack_ptr < (uint32_t)stack) stack_ptr += (mask + 1);

    furi_hal_mpu_protect_read_only(FuriHalMpuRegionStack, stack_ptr, 32);
}

void furi_hal_mpu_init(void) {
    furi_hal_mpu_enable();
    // NULL pointer dereference protection
    furi_hal_mpu_protect_no_access(FuriHalMpuRegionNULL, 0x00, 1024 * 1024);

    FURI_LOG_I(TAG, "Init OK");
}
