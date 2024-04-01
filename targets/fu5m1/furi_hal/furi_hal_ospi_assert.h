#pragma once

#include "furi_hal_ospi.h"

#define IS_FURI_HAL_OSPI_OPERATION_TYPE(TYPE)       (((TYPE) == FURI_HAL_OSPI_OPTYPE_COMMON_CFG) || \
                                                    ((TYPE) == FURI_HAL_OSPI_OPTYPE_READ_CFG)   || \
                                                    ((TYPE) == FURI_HAL_OSPI_OPTYPE_WRITE_CFG)  || \
                                                    ((TYPE) == FURI_HAL_OSPI_OPTYPE_WRAP_CFG))

#define IS_FURI_HAL_OSPI_FLASH_ID(FLASHID)          (((FLASHID) == FURI_HAL_OSPI_FLASH_ID_1) || \
                                                    ((FLASHID) == FURI_HAL_OSPI_FLASH_ID_2))

#define IS_FURI_HAL_OSPI_INSTRUCTION_MODE(MODE)     (((MODE) == FURI_HAL_OSPI_INSTRUCTION_NONE)    || \
                                                    ((MODE) == FURI_HAL_OSPI_INSTRUCTION_1_LINE)  || \
                                                    ((MODE) == FURI_HAL_OSPI_INSTRUCTION_2_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_INSTRUCTION_4_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_INSTRUCTION_8_LINES))

#define IS_FURI_HAL_OSPI_INSTRUCTION_SIZE(SIZE)     (((SIZE) == FURI_HAL_OSPI_INSTRUCTION_8_BITS)  || \
                                                    ((SIZE) == FURI_HAL_OSPI_INSTRUCTION_16_BITS) || \
                                                    ((SIZE) == FURI_HAL_OSPI_INSTRUCTION_24_BITS) || \
                                                    ((SIZE) == FURI_HAL_OSPI_INSTRUCTION_32_BITS))

#define IS_FURI_HAL_OSPI_INSTRUCTION_DTR_MODE(MODE) (((MODE) == FURI_HAL_OSPI_INSTRUCTION_DTR_DISABLE) || \
                                                    ((MODE) == FURI_HAL_OSPI_INSTRUCTION_DTR_ENABLE))

#define IS_FURI_HAL_OSPI_ADDRESS_MODE(MODE)         (((MODE) == FURI_HAL_OSPI_ADDRESS_NONE)    || \
                                                    ((MODE) == FURI_HAL_OSPI_ADDRESS_1_LINE)  || \
                                                    ((MODE) == FURI_HAL_OSPI_ADDRESS_2_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_ADDRESS_4_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_ADDRESS_8_LINES))

#define IS_FURI_HAL_OSPI_ADDRESS_SIZE(SIZE)         (((SIZE) == FURI_HAL_OSPI_ADDRESS_8_BITS)  || \
                                                    ((SIZE) == FURI_HAL_OSPI_ADDRESS_16_BITS) || \
                                                    ((SIZE) == FURI_HAL_OSPI_ADDRESS_24_BITS) || \
                                                    ((SIZE) == FURI_HAL_OSPI_ADDRESS_32_BITS))

#define IS_FURI_HAL_OSPI_ADDRESS_DTR_MODE(MODE)     (((MODE) == FURI_HAL_OSPI_ADDRESS_DTR_DISABLE) || \
                                                    ((MODE) == FURI_HAL_OSPI_ADDRESS_DTR_ENABLE))

#define IS_FURI_HAL_OSPI_ALT_BYTES_MODE(MODE)       (((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_NONE)    || \
                                                    ((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_1_LINE)  || \
                                                    ((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_2_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_4_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_8_LINES))

#define IS_FURI_HAL_OSPI_ALT_BYTES_SIZE(SIZE)       (((SIZE) == FURI_HAL_OSPI_ALTERNATE_BYTES_8_BITS)  || \
                                                    ((SIZE) == FURI_HAL_OSPI_ALTERNATE_BYTES_16_BITS) || \
                                                    ((SIZE) == FURI_HAL_OSPI_ALTERNATE_BYTES_24_BITS) || \
                                                    ((SIZE) == FURI_HAL_OSPI_ALTERNATE_BYTES_32_BITS))

#define IS_FURI_HAL_OSPI_ALT_BYTES_DTR_MODE(MODE)   (((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE) || \
                                                    ((MODE) == FURI_HAL_OSPI_ALTERNATE_BYTES_DTR_ENABLE))

#define IS_FURI_HAL_OSPI_DATA_MODE(MODE)            (((MODE) == FURI_HAL_OSPI_DATA_NONE)    || \
                                                    ((MODE) == FURI_HAL_OSPI_DATA_1_LINE)  || \
                                                    ((MODE) == FURI_HAL_OSPI_DATA_2_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_DATA_4_LINES) || \
                                                    ((MODE) == FURI_HAL_OSPI_DATA_8_LINES))

#define IS_FURI_HAL_OSPI_NUMBER_DATA(NUMBER)        ((NUMBER) >= 1U)

#define IS_FURI_HAL_OSPI_DATA_DTR_MODE(MODE)        (((MODE) == FURI_HAL_OSPI_DATA_DTR_DISABLE) || \
                                                    ((MODE) == FURI_HAL_OSPI_DATA_DTR_ENABLE))

#define IS_FURI_HAL_OSPI_DUMMY_CYCLES(NUMBER)       ((NUMBER) <= 31U)

#define IS_FURI_HAL_OSPI_DQS_MODE(MODE)             (((MODE) == FURI_HAL_OSPI_DQS_DISABLE) || \
                                                    ((MODE) == FURI_HAL_OSPI_DQS_ENABLE))

#define IS_FURI_HAL_OSPI_SIOO_MODE(MODE)            (((MODE) == FURI_HAL_OSPI_SIOO_INST_EVERY_CMD) || \
                                                    ((MODE) == FURI_HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD))

