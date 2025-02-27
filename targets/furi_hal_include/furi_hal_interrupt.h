#pragma once

#include <stdint.h>

#include <furi_hal_interrupt_defs.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Timer ISR */
typedef void (*FuriHalInterruptISR)(void* context);

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
void furi_hal_interrupt_init(void);

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

/** Get interrupt name by exception number.
 * Exception number can be obtained from IPSR register.
 * 
 * @param exception_number 
 * @return const char* or NULL if interrupt name is not found
 */
const char* furi_hal_interrupt_get_name(uint8_t exception_number);

/** Get total time(in CPU clocks) spent in ISR
 *
 * @return     total time in CPU clocks
 */
uint32_t furi_hal_interrupt_get_time_in_isr_total(void);

#ifdef __cplusplus
}
#endif
