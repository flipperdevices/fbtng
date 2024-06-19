#include <furi.h>
#include <furi_hal.h>
#include <stm32u5xx_safe.h>
#include <nema_vg_safe.h>

#define TAG "GPU2D"

// #define GPU2D_TRACE(...) FURI_LOG_D(TAG, __VA_ARGS__)
#define GPU2D_TRACE(...)

FuriMutex* nema_mutex_rb;
FuriSemaphore* nema_irq_sem;

#define RING_SIZE 1024 /* Ring Buffer Size in byte */
static nema_ringbuffer_t ring_buffer_str;
static volatile int last_cl_id = -1;

#define GPU2D_ITCTRL (0x0F8U) /*!< GPU2D Interrupt Control Register Offset            */
#define GPU2D_CLID (0x148U) /*!< GPU2D Last Command List Identifier Register Offset */

#define GPU2D_FLAG_CLC 0x00000001U /*!< Command List Complete Interrupt Flag  */

#define GPU2D_CLEAR_IT_FLAG(__FLAG__)                                               \
    do {                                                                            \
        __IO uint32_t* tmpreg = (__IO uint32_t*)((uint32_t)(GPU2D) + GPU2D_ITCTRL); \
        CLEAR_BIT(*tmpreg, __FLAG__);                                               \
    } while(0U)

static void gpu2d_reg_write(uint32_t offset, uint32_t value) {
    WRITE_REG(*(__IO uint32_t*)(GPU2D + offset), value);
}

static uint32_t gpu2d_reg_read(uint32_t offset) {
    return READ_REG(*(__IO uint32_t*)(GPU2D + offset));
}

static void gpu2d_irq(void* context) {
    UNUSED(context);
    uint32_t isr_flags = gpu2d_reg_read(GPU2D_ITCTRL);

    /* Command List Complete Interrupt management */
    if((isr_flags & GPU2D_FLAG_CLC) != 0U) {
        /* Clear the completion flag */
        GPU2D_CLEAR_IT_FLAG(GPU2D_FLAG_CLC);

        last_cl_id = gpu2d_reg_read(GPU2D_CLID);
        furi_semaphore_release(nema_irq_sem);
    }
}

static void gpu2d_error_irq(void* context) {
    UNUSED(context);
    furi_crash("GPU2D Error");
}

void nema_host_free(void* ptr) {
    GPU2D_TRACE("free 0x%08lX", (uint32_t)ptr);
    free(ptr);
}

void* nema_host_malloc(unsigned size) {
    GPU2D_TRACE("malloc %d", size);
    return malloc(size);
}

int nema_mutex_lock(int mutex_id) {
    GPU2D_TRACE("lock %d", mutex_id);
    furi_check(mutex_id == MUTEX_RB, "Nema: invalid mutex id");
    return (furi_mutex_acquire(nema_mutex_rb, FuriWaitForever) == FuriStatusOk) ? 0 : -1;
}

int nema_mutex_unlock(int mutex_id) {
    GPU2D_TRACE("unlock %d", mutex_id);
    furi_check(mutex_id == MUTEX_RB, "Nema: invalid mutex id");
    return (furi_mutex_release(nema_mutex_rb) == FuriStatusOk) ? 0 : -1;
}

nema_buffer_t nema_buffer_create(int size) {
    nema_buffer_t bo;
    memset(&bo, 0, sizeof(nema_buffer_t));
    bo.base_virt = malloc(size);
    bo.base_phys = (uint32_t)bo.base_virt;
    bo.size = size;
    bo.fd = 0;

    GPU2D_TRACE(
        "create virt, size %d, virt 0x%08lX, phys 0x%08lX",
        size,
        (uint32_t)bo.base_virt,
        (uint32_t)bo.base_phys);
    return bo;
}

nema_buffer_t nema_buffer_create_pool(int pool, int size) {
    UNUSED(pool);
    return nema_buffer_create(size);
}

void nema_buffer_destroy(nema_buffer_t* bo) {
    GPU2D_TRACE("destroy virt 0x%08lX", (uint32_t)bo->base_virt);
    free(bo->base_virt);
}

void nema_buffer_flush(nema_buffer_t* bo) {
    // TODO: SCB hal
    // SCB_CleanInvalidateDCache_by_Addr((uint32_t*)bo->base_virt, bo->size);
}

void* nema_buffer_map(nema_buffer_t* bo) {
    return bo->base_virt;
}

void nema_buffer_unmap(nema_buffer_t* bo) {
    UNUSED(bo);
}

int nema_wait_irq_cl(int cl_id) {
    while(last_cl_id < cl_id) {
        furi_semaphore_acquire(nema_irq_sem, FuriWaitForever);
    }

    return 0;
}

void nema_reg_write(uint32_t reg, uint32_t value) {
    GPU2D_TRACE("W 0x%08lX = 0x%08lX", reg, value);
    gpu2d_reg_write(reg, value);
}

uint32_t nema_reg_read(uint32_t reg) {
    GPU2D_TRACE("R 0x%08lX = 0x%08lX", reg, gpu2d_reg_read(reg));
    return gpu2d_reg_read(reg);
}

int32_t nema_sys_init(void) {
    int error_code = 0;

    /* Allocate ring_buffer memory */
    ring_buffer_str.bo = nema_buffer_create(RING_SIZE);
    assert(ring_buffer_str.bo.base_virt);

    /* Initialize Ring Buffer */
    error_code = nema_rb_init(&ring_buffer_str, 1);
    if(error_code < 0) {
        return error_code;
    }

    /* Reset last_cl_id counter */
    last_cl_id = 0;

    return error_code;
}

void furi_hal_gpu_init(void) {
    furi_hal_bus_enable(FuriHalBusGPU2D);

    nema_mutex_rb = furi_mutex_alloc(FuriMutexTypeNormal);
    nema_irq_sem = furi_semaphore_alloc(1, 0);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdGPU2D, gpu2d_irq, NULL);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdGPU2DError, gpu2d_error_irq, NULL);

    int err = nema_init();
    if(err < 0) {
        FURI_LOG_E(TAG, "Nema: init failed %d", err);
        furi_crash("Nema: init failed");
    }

    FURI_LOG_I(TAG, "initialized");
}