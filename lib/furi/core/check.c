#include "check.h"
#include "log.h"
#include "common_defines.h"

#include <furi_hal_debug.h>
#include <furi_hal_interrupt.h>
#include <furi_hal_rtc.h>

#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <stdlib.h>

static const char* __furi_check_message = NULL;
static uint32_t __furi_check_registers[13] = {0};

static struct {
    FuriCrashCallback callback;
    void* context;
} registered_crash_handler = {0};

/** Load r12 value to __furi_check_message and store registers to __furi_check_registers */
#define GET_MESSAGE_AND_STORE_REGISTERS()               \
    asm volatile("ldr r11, =__furi_check_message    \n" \
                 "str r12, [r11]                    \n" \
                 "ldr r12, =__furi_check_registers  \n" \
                 "stm r12, {r0-r11}                 \n" \
                 "str lr, [r12, #48]                \n" \
                 :                                      \
                 :                                      \
                 : "memory");

/** Restore registers and halt MCU
 * 
 * - Always use it with GET_MESSAGE_AND_STORE_REGISTERS
 * - If debugger is(was) connected this routine will raise bkpt
 * - If debugger is not connected then endless loop
 * 
 */
#define RESTORE_REGISTERS_AND_HALT_MCU(debug)           \
    register bool r0 asm("r0") = debug;                 \
    asm volatile("cbnz  r0, with_debugger%=         \n" \
                 "ldr   r12, =__furi_check_registers\n" \
                 "ldm   r12, {r0-r11}               \n" \
                 "loop%=:                           \n" \
                 "wfi                               \n" \
                 "b     loop%=                      \n" \
                 "with_debugger%=:                  \n" \
                 "ldr   r12, =__furi_check_registers\n" \
                 "ldm   r12, {r0-r11}               \n" \
                 "debug_loop%=:                     \n" \
                 "bkpt  0x00                        \n" \
                 "wfi                               \n" \
                 "b     debug_loop%=                \n" \
                 :                                      \
                 : "r"(r0)                              \
                 : "memory");

extern size_t xPortGetTotalHeapSize(void);

static void __furi_put_uint32_as_text(uint32_t data) {
    char tmp_str[] = "-2147483648";
    itoa(data, tmp_str, 10);
    furi_log_puts(tmp_str);
}

static void __furi_put_uint32_as_hex(uint32_t data) {
    char tmp_str[] = "0xFFFFFFFF";
    itoa(data, tmp_str, 16);
    furi_log_puts(tmp_str);
}

static const FuriCrashLogInterface crash_log_interface = {
    .puts = furi_log_puts,
    .print_uint32_as_hex = __furi_put_uint32_as_hex,
    .print_uint32_as_dec = __furi_put_uint32_as_text,
};

void furi_crash_handler_add(FuriCrashCallback callback, void* context) {
    furi_check(registered_crash_handler.callback == NULL);
    registered_crash_handler.callback = callback;
    registered_crash_handler.context = context;
}

static void __furi_print_register_info(void) {
    // Print registers
    for(uint8_t i = 0; i < 12; i++) {
        furi_log_puts("\r\n\tr");
        __furi_put_uint32_as_text(i);
        furi_log_puts(" : ");
        __furi_put_uint32_as_hex(__furi_check_registers[i]);
    }

    furi_log_puts("\r\n\tlr : ");
    __furi_put_uint32_as_hex(__furi_check_registers[12]);
}

static void __furi_print_stack_info(void) {
    furi_log_puts("\r\n\tstack watermark: ");
    __furi_put_uint32_as_text(uxTaskGetStackHighWaterMark(NULL) * 4);
}

static void __furi_print_heap_info(void) {
    furi_log_puts("\r\n\t     heap total: ");
    __furi_put_uint32_as_text(xPortGetTotalHeapSize());
    furi_log_puts("\r\n\t      heap free: ");
    __furi_put_uint32_as_text(xPortGetFreeHeapSize());
    furi_log_puts("\r\n\t heap watermark: ");
    __furi_put_uint32_as_text(xPortGetMinimumEverFreeHeapSize());
}

static void __furi_print_name(bool isr) {
    if(isr) {
        uint8_t exception_number = __get_IPSR();
        const char* name = furi_hal_interrupt_get_name(exception_number);
        furi_log_puts("[ISR ");
        if(name) {
            furi_log_puts(name);
        } else {
            __furi_put_uint32_as_text(__get_IPSR());
        }
        furi_log_puts("] ");
    } else {
        const char* name = pcTaskGetName(NULL);
        if(name == NULL) {
            furi_log_puts("[main] ");
        } else {
            furi_log_puts("[");
            furi_log_puts(name);
            furi_log_puts("] ");
        }
    }
}

FURI_NORETURN void __furi_crash_implementation(void) {
    __disable_irq();

    __BKPT();

    // FIXME: furi_crash() kills the debugger
    for(;;)
        ;

    GET_MESSAGE_AND_STORE_REGISTERS();

    bool isr = FURI_IS_IRQ_MODE();

    if(__furi_check_message == NULL) {
        __furi_check_message = "Fatal Error";
    } else if(__furi_check_message == (void*)__FURI_ASSERT_MESSAGE_FLAG) {
        __furi_check_message = "furi_assert failed";
    } else if(__furi_check_message == (void*)__FURI_CHECK_MESSAGE_FLAG) {
        __furi_check_message = "furi_check failed";
    }

    furi_log_puts("\r\n\033[0;31m[CRASH]");
    __furi_print_name(isr);
    furi_log_puts(__furi_check_message);

    __furi_print_register_info();
    if(!isr) {
        __furi_print_stack_info();
    }
    __furi_print_heap_info();

    if(registered_crash_handler.callback) {
        registered_crash_handler.callback(registered_crash_handler.context, &crash_log_interface);
    }

    // Check if debug enabled by DAP
    // https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/Debug-register-support-in-the-SCS/Debug-Halting-Control-and-Status-Register--DHCSR?lang=en
    bool debug = CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk;
#ifndef FURI_DEBUG
    if(debug) {
#endif
        furi_log_puts("\r\nSystem halted. Connect debugger for more info\r\n");
        furi_log_puts("\033[0m\r\n");
        furi_hal_debug_enable();

        RESTORE_REGISTERS_AND_HALT_MCU(debug);
#ifndef FURI_DEBUG
    } else {
        uint32_t ptr = (uint32_t)__furi_check_message;
        if(ptr < FLASH_BASE || ptr > (FLASH_BASE + FLASH_SIZE)) {
            ptr = (uint32_t) "Check serial logs";
        }
        furi_hal_rtc_set_fault_data(ptr);
        furi_log_puts("\r\nRebooting system.\r\n");
        furi_log_puts("\033[0m\r\n");
        furi_hal_power_reset();
    }
#endif
    __builtin_unreachable();
}

FURI_NORETURN void __furi_halt_implementation(void) {
    __disable_irq();
    GET_MESSAGE_AND_STORE_REGISTERS();

    bool isr = FURI_IS_IRQ_MODE();

    if(__furi_check_message == NULL) {
        __furi_check_message = "System halt requested.";
    }

    furi_log_puts("\r\n\033[0;31m[HALT]");
    __furi_print_name(isr);
    furi_log_puts(__furi_check_message);
    furi_log_puts("\r\nSystem halted. Bye-bye!\r\n");
    furi_log_puts("\033[0m\r\n");

    // Check if debug enabled by DAP
    // https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/Debug-register-support-in-the-SCS/Debug-Halting-Control-and-Status-Register--DHCSR?lang=en
    bool debug = CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk;
    RESTORE_REGISTERS_AND_HALT_MCU(debug);

    __builtin_unreachable();
}
