#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/** Timer ISR */
typedef void (*FuriHalInterruptISR)(void* context);

typedef enum {
    // SDMMC
    FuriHalInterruptIdSdMmc1,

    // RCC
    FuriHalInterruptIdRcc,

    // Service value
    FuriHalInterruptIdMax,
} FuriHalInterruptId;

typedef enum {
    FuriHalInterruptPriorityLowest =
        -3, /**< Lowest priority level, you can use ISR-safe OS primitives */
    FuriHalInterruptPriorityLower =
        -2, /**< Lower priority level, you can use ISR-safe OS primitives */
    FuriHalInterruptPriorityLow =
        -1, /**< Low priority level, you can use ISR-safe OS primitives */
    FuriHalInterruptPriorityNormal =
        0, /**< Normal(default) priority level, you can use ISR-safe OS primitives */
    FuriHalInterruptPriorityHigh =
        1, /**< High priority level, you can use ISR-safe OS primitives */
    FuriHalInterruptPriorityHigher =
        2, /**< Higher priority level, you can use ISR-safe OS primitives */
    FuriHalInterruptPriorityHighest =
        3, /**< Highest priority level, you can use ISR-safe OS primitives */

    /* Special group, read docs first(ALL OF THEM: especially FreeRTOS configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY) */
    FuriHalInterruptPriorityKamiSama =
        6, /**< Forget about thread safety, you are god now. No one can prevent you from messing with OS critical section. You are not allowed to use any OS primitives, but who can stop you? Use this priority only for direct hardware interaction with LL HAL. */
} FuriHalInterruptPriority;

/** Initialize interrupt subsystem */
void furi_hal_interrupt_init();

/** Set ISR and enable interrupt with default priority
 *
 * @warning    Interrupt flags are not cleared automatically. You may want to
 *             ensure that your peripheral status flags are cleared.
 *
 * @param      index    - interrupt ID
 * @param      isr      - your interrupt service routine or use NULL to clear
 * @param      context  - isr context
 */
void furi_hal_interrupt_set_isr(FuriHalInterruptId index, FuriHalInterruptISR isr, void* context);

/** Set ISR and enable interrupt with custom priority
 *
 * @warning    Interrupt flags are not cleared automatically. You may want to
 *             ensure that your peripheral status flags are cleared.
 *
 * @param      index     - interrupt ID
 * @param      priority  - One of FuriHalInterruptPriority
 * @param      isr       - your interrupt service routine or use NULL to clear
 * @param      context   - isr context
 */
void furi_hal_interrupt_set_isr_ex(
    FuriHalInterruptId index,
    FuriHalInterruptPriority priority,
    FuriHalInterruptISR isr,
    void* context);

#ifdef __cplusplus
}
#endif
