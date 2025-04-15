/**
 * @file stm32wb55_linker.h
 *
 * Linker defined symbols. Used in various part of firmware to understand
 * hardware boundaries.
 * 
 */
#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
typedef const char linker_symbol_t;
#else
typedef const void linker_symbol_t;
#endif

extern linker_symbol_t _stack_end; /**< end of stack */
extern linker_symbol_t _stack_size; /**< stack size */

extern linker_symbol_t _sidata; /**< data initial value start */
extern linker_symbol_t _sdata; /**< data start */
extern linker_symbol_t _edata; /**< data end */

extern linker_symbol_t _sbss; /**< bss start */
extern linker_symbol_t _ebss; /**< bss end */

extern const void __sram2a_start__; /**< RAM2a Heap start */
extern const void __sram2a_free__; /**< RAM2a Free space start */
extern const void __sram2b_start__; /**< RAM2b Start */

extern const void _sMB_MEM2; /**< RAM2a start */
extern const void _eMB_MEM2; /**< RAM2a end */

extern const void __heap_start__; /**< RAM1 Heap start */
extern const void __heap_end__; /**< RAM1 Heap end */
extern const void __heap_size__; /**< RAM1 Heap size (as symbol address) */

extern linker_symbol_t __free_flash_start__; /**< Free Flash space start */

#ifdef __cplusplus
}
#endif
