#include <furi_hal.h>
#include <FreeRTOS.h>
#include <stm32u5xx_ll_cortex.h>
#include <stm32u5xx_ll_system.h>

#define TAG "FuriHalInterrupt"

#define FURI_HAL_INTERRUPT_DEFAULT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5)

typedef struct {
    FuriHalInterruptISR isr;
    void* context;
} FuriHalInterruptISRPair;

FuriHalInterruptISRPair furi_hal_interrupt_isr[FuriHalInterruptIdMax] = {0};

const IRQn_Type furi_hal_interrupt_irqn[FuriHalInterruptIdMax] = {
    // SDMMC
    [FuriHalInterruptIdSdMmc1] = SDMMC1_IRQn,

    // RCC
    [FuriHalInterruptIdRcc] = RCC_IRQn,
};

__attribute__((always_inline)) static inline void
    furi_hal_interrupt_call(FuriHalInterruptId index) {
    furi_check(furi_hal_interrupt_isr[index].isr);
    furi_hal_interrupt_isr[index].isr(furi_hal_interrupt_isr[index].context);
}

__attribute__((always_inline)) static inline void
    furi_hal_interrupt_enable(FuriHalInterruptId index, uint16_t priority) {
    NVIC_SetPriority(
        furi_hal_interrupt_irqn[index],
        NVIC_EncodePriority(NVIC_GetPriorityGrouping(), priority, 0));
    NVIC_EnableIRQ(furi_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) static inline void
    furi_hal_interrupt_clear_pending(FuriHalInterruptId index) {
    NVIC_ClearPendingIRQ(furi_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) static inline void
    furi_hal_interrupt_get_pending(FuriHalInterruptId index) {
    NVIC_GetPendingIRQ(furi_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) static inline void
    furi_hal_interrupt_set_pending(FuriHalInterruptId index) {
    NVIC_SetPendingIRQ(furi_hal_interrupt_irqn[index]);
}

__attribute__((always_inline)) static inline void
    furi_hal_interrupt_disable(FuriHalInterruptId index) {
    NVIC_DisableIRQ(furi_hal_interrupt_irqn[index]);
}

void furi_hal_interrupt_init() {
    NVIC_SetPriority(TAMP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(TAMP_IRQn);

    NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));

    NVIC_SetPriority(FPU_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
    NVIC_EnableIRQ(FPU_IRQn);

    LL_SYSCFG_DisableIT_FPU_IOC();
    LL_SYSCFG_DisableIT_FPU_DZC();
    LL_SYSCFG_DisableIT_FPU_UFC();
    LL_SYSCFG_DisableIT_FPU_OFC();
    LL_SYSCFG_DisableIT_FPU_IDC();
    LL_SYSCFG_DisableIT_FPU_IXC();

    LL_HANDLER_EnableFault(LL_HANDLER_FAULT_USG);
    LL_HANDLER_EnableFault(LL_HANDLER_FAULT_BUS);
    LL_HANDLER_EnableFault(LL_HANDLER_FAULT_MEM);

    FURI_LOG_I(TAG, "Init OK");
}

void furi_hal_interrupt_set_isr(FuriHalInterruptId index, FuriHalInterruptISR isr, void* context) {
    furi_hal_interrupt_set_isr_ex(index, FuriHalInterruptPriorityNormal, isr, context);
}

void furi_hal_interrupt_set_isr_ex(
    FuriHalInterruptId index,
    FuriHalInterruptPriority priority,
    FuriHalInterruptISR isr,
    void* context) {
    furi_check(index < FuriHalInterruptIdMax);
    furi_check(
        (priority >= FuriHalInterruptPriorityLowest &&
         priority <= FuriHalInterruptPriorityHighest) ||
        priority == FuriHalInterruptPriorityKamiSama);

    uint16_t real_priority = FURI_HAL_INTERRUPT_DEFAULT_PRIORITY - priority;

    if(isr) {
        // Pre ISR set
        furi_check(furi_hal_interrupt_isr[index].isr == NULL);
    } else {
        // Pre ISR clear
        furi_hal_interrupt_disable(index);
        furi_hal_interrupt_clear_pending(index);
    }

    furi_hal_interrupt_isr[index].isr = isr;
    furi_hal_interrupt_isr[index].context = context;
    __DMB();

    if(isr) {
        // Post ISR set
        furi_hal_interrupt_clear_pending(index);
        furi_hal_interrupt_enable(index, real_priority);
    } else {
        // Post ISR clear
    }
}

void SDMMC1_IRQHandler() {
    furi_hal_interrupt_call(FuriHalInterruptIdSdMmc1);
}

void TAMP_IRQHandler(void) {
    if(LL_RCC_LSE_IsCSSDetected()) {
        LL_RCC_LSE_DisableCSS();
        if(!LL_RCC_LSE_IsReady()) {
            FURI_LOG_E(TAG, "LSE CSS fired: resetting system");
            NVIC_SystemReset();
        } else {
            FURI_LOG_E(TAG, "LSE CSS fired: but LSE is alive");
            LL_RCC_LSE_EnableCSS(); // TODO: we really can recover from this?
        }
    }
}

void RCC_IRQHandler() {
    furi_hal_interrupt_call(FuriHalInterruptIdRcc);
}

void NMI_Handler() {
    if(LL_RCC_IsActiveFlag_HSECSS()) {
        LL_RCC_ClearFlag_HSECSS();
        FURI_LOG_E(TAG, "HSE CSS fired: resetting system");
        NVIC_SystemReset();
    }
}

void HardFault_Handler() {
    furi_crash("HardFault");
}

void MemManage_Handler() {
    if(FURI_BIT(SCB->CFSR, SCB_CFSR_MMARVALID_Pos)) {
        uint32_t memfault_address = SCB->MMFAR;
        if(memfault_address < (1024 * 1024)) {
            // from 0x00 to 1MB, see FuriHalMpuRegionNULL
            furi_crash("NULL pointer dereference");
        } else {
            // write or read of MPU region 1 (FuriHalMpuRegionStack)
            furi_crash("MPU fault, possibly stack overflow");
        }
    } else if(FURI_BIT(SCB->CFSR, SCB_CFSR_MSTKERR_Pos)) {
        // push to stack on MPU region 1 (FuriHalMpuRegionStack)
        furi_crash("MemManage fault, possibly stack overflow");
    }

    furi_crash("MemManage");
}

void BusFault_Handler() {
    furi_crash("BusFault");
}

void UsageFault_Handler() {
    furi_crash("UsageFault");
}

void DebugMon_Handler() {
}

void FPU_IRQHandler() {
    furi_crash("FpuFault");
}