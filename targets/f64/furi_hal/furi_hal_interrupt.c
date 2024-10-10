#include <furi_hal_interrupt.h>
#include <furi_hal_os.h>

#include <furi.h>

#include <si91x_device.h>

#define TAG "FuriHalInterrupt"

#define FURI_HAL_INTERRUPT_DEFAULT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5)

#ifdef FURI_RAM_EXEC
#define FURI_HAL_INTERRUPT_ACCOUNT_START()
#define FURI_HAL_INTERRUPT_ACCOUNT_END()
#else
#define FURI_HAL_INTERRUPT_ACCOUNT_START() const uint32_t _isr_start = DWT->CYCCNT;
#define FURI_HAL_INTERRUPT_ACCOUNT_END()                    \
    const uint32_t _time_in_isr = DWT->CYCCNT - _isr_start; \
    furi_hal_interrupt.counter_time_in_isr_total += _time_in_isr;
#endif

typedef struct {
    FuriHalInterruptISR isr;
    void* context;
} FuriHalInterruptISRPair;

typedef struct {
    FuriHalInterruptISRPair isr[FuriHalInterruptIdMax];
    uint32_t counter_time_in_isr_total;
} FuriHalIterrupt;

static FuriHalIterrupt furi_hal_interrupt;

void furi_hal_interrupt_init(void) {
    NVIC_SetPriority(SVCall_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

    FURI_LOG_I(TAG, "Init OK");
}

const char* furi_hal_interrupt_get_name(uint8_t exception_number) {
    UNUSED(exception_number);
    return "Unknown Exception";
}

uint32_t furi_hal_interrupt_get_time_in_isr_total(void) {
    return furi_hal_interrupt.counter_time_in_isr_total;
}

void SysTick_Handler(void) {
    FURI_HAL_INTERRUPT_ACCOUNT_START();
    furi_hal_os_tick();
    FURI_HAL_INTERRUPT_ACCOUNT_END();
}
