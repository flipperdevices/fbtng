#pragma once

#include "stm32u5xx.h"
#include "stm32u5xx_ll_dlyb.h"
#include <stdbool.h>

#define FURI_HAL_OSPI_FUNCTIONAL_MODE_INDIRECT_WRITE ((uint32_t)0x00000000)         /*!< Indirect write mode    */
#define FURI_HAL_OSPI_FUNCTIONAL_MODE_INDIRECT_READ  ((uint32_t)OCTOSPI_CR_FMODE_0) /*!< Indirect read mode     */
#define FURI_HAL_OSPI_FUNCTIONAL_MODE_AUTO_POLLING   ((uint32_t)OCTOSPI_CR_FMODE_1) /*!< Automatic polling mode */
#define FURI_HAL_OSPI_FUNCTIONAL_MODE_MEMORY_MAPPED  ((uint32_t)OCTOSPI_CR_FMODE)   /*!< Memory-mapped mode     */

/** @defgroup OSPI_Timeout_definition OSPI Timeout definition
  * @{
  */
#define FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE       ((uint32_t)5000U)                                               /* 5 s */
/**
  * @}
  */

/** @defgroup OSPI_DualQuad OSPI Dual-Quad
  * @{
  */
#define FURI_HAL_OSPI_DUALQUAD_DISABLE            ((uint32_t)0x00000000U)                                         /*!< Dual-Quad mode disabled */
#define FURI_HAL_OSPI_DUALQUAD_ENABLE             ((uint32_t)OCTOSPI_CR_DMM)
/**
  * @}
  */

/** @defgroup OSPI_ClockMode OSPI Clock Mode
  * @{
  */
#define FURI_HAL_OSPI_CLOCK_MODE_0                ((uint32_t)0x00000000U)                                         /*!< CLK must stay low while nCS is high  */
#define FURI_HAL_OSPI_CLOCK_MODE_3                ((uint32_t)OCTOSPI_DCR1_CKMODE)  
/**
  * @}
  */

/** @defgroup OSPI_MemoryType OSPI Memory Type
  * @{
  */
#define FURI_HAL_OSPI_MEMTYPE_MICRON              ((uint32_t)0x00000000U)                                         /*!< Micron mode       */
#define FURI_HAL_OSPI_MEMTYPE_MACRONIX            ((uint32_t)OCTOSPI_DCR1_MTYP_0)                                 /*!< Macronix mode     */
#define FURI_HAL_OSPI_MEMTYPE_APMEMORY            ((uint32_t)OCTOSPI_DCR1_MTYP_1)                                 /*!< AP Memory mode    */
#define FURI_HAL_OSPI_MEMTYPE_MACRONIX_RAM        ((uint32_t)(OCTOSPI_DCR1_MTYP_1 | OCTOSPI_DCR1_MTYP_0))         /*!< Macronix RAM mode */
#define FURI_HAL_OSPI_MEMTYPE_HYPERBUS            ((uint32_t)OCTOSPI_DCR1_MTYP_2) 
/**
  * @}
  */

/** @defgroup OSPI_FreeRunningClock OSPI Free Running Clock
  * @{
  */
#define FURI_HAL_OSPI_FREERUNCLK_DISABLE          ((uint32_t)0x00000000U)                                         /*!< CLK is not free running               */
#define FURI_HAL_OSPI_FREERUNCLK_ENABLE           ((uint32_t)OCTOSPI_DCR1_FRCK)  
/**
  * @}
  */

/** @defgroup OSPI_WrapSize OSPI Wrap-Size
  * @{
  */
#define FURI_HAL_OSPI_WRAP_NOT_SUPPORTED          ((uint32_t)0x00000000U)                                         /*!< wrapped reads are not supported by the memory   */
#define FURI_HAL_OSPI_WRAP_16_BYTES               ((uint32_t)OCTOSPI_DCR2_WRAPSIZE_1)                             /*!< external memory supports wrap size of 16 bytes  */
#define FURI_HAL_OSPI_WRAP_32_BYTES               ((uint32_t)(OCTOSPI_DCR2_WRAPSIZE_0 | OCTOSPI_DCR2_WRAPSIZE_1)) /*!< external memory supports wrap size of 32 bytes  */
#define FURI_HAL_OSPI_WRAP_64_BYTES               ((uint32_t)OCTOSPI_DCR2_WRAPSIZE_2)                             /*!< external memory supports wrap size of 64 bytes  */
#define FURI_HAL_OSPI_WRAP_128_BYTES              ((uint32_t)(OCTOSPI_DCR2_WRAPSIZE_0 | OCTOSPI_DCR2_WRAPSIZE_2)) /*!< external memory supports wrap size of 128 bytes */
/**
  * @}
  */

/** @defgroup OSPI_SampleShifting OSPI Sample Shifting
  * @{
  */
#define FURI_HAL_OSPI_SAMPLE_SHIFTING_NONE        ((uint32_t)0x00000000U)                                         /*!< No shift        */
#define FURI_HAL_OSPI_SAMPLE_SHIFTING_HALFCYCLE   ((uint32_t)OCTOSPI_TCR_SSHIFT)    
/**
  * @}
  */

/** @defgroup OSPI_DelayHoldQuarterCycle OSPI Delay Hold Quarter Cycle
  * @{
  */
#define FURI_HAL_OSPI_DHQC_DISABLE                ((uint32_t)0x00000000U)                                         /*!< No Delay             */
#define FURI_HAL_OSPI_DHQC_ENABLE                 ((uint32_t)OCTOSPI_TCR_DHQC)        
/**
  * @}
  */

/** @defgroup OSPI_DelayBlockBypass OSPI Delay Block Bypaas
  * @{
  */
#define FURI_HAL_OSPI_DELAY_BLOCK_USED            ((uint32_t)0x00000000U)                                         /*!< Sampling clock is delayed by the delay block */
#define FURI_HAL_OSPI_DELAY_BLOCK_BYPASSED        ((uint32_t)OCTOSPI_DCR1_DLYBYP)    
/**
  * @}
  */

/** @defgroup OSPI_TimeOutActivation OSPI Timeout Activation
  * @{
  */
#define FURI_HAL_OSPI_TIMEOUT_COUNTER_DISABLE     ((uint32_t)0x00000000U)                                         /*!< Timeout counter disabled, nCS remains active               */
#define FURI_HAL_OSPI_TIMEOUT_COUNTER_ENABLE      ((uint32_t)OCTOSPI_CR_TCEN)                                     /*!< Timeout counter enabled, nCS released when timeout expires */
/**
  * @}
  */

/** @defgroup OSPI_TimeOutActivation OSPI Timeout Activation
  * @{
  */
#define FURI_HAL_OSPI_TIMEOUT_COUNTER_DISABLE     ((uint32_t)0x00000000U)                                         /*!< Timeout counter disabled, nCS remains active               */
#define FURI_HAL_OSPI_TIMEOUT_COUNTER_ENABLE      ((uint32_t)OCTOSPI_CR_TCEN)                                     /*!< Timeout counter enabled, nCS released when timeout expires */
/**
  * @}
  */

/** @defgroup OSPIM_IOPort OSPI IO Manager IO Port
  * @{
  */
#define FURI_HAL_OSPIM_IOPORT_NONE              ((uint32_t)0x00000000U)                                          /*!< IOs not used */
#define FURI_HAL_OSPIM_IOPORT_1_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x1U))                          /*!< Port 1 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_1_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x1U))                          /*!< Port 1 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_2_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x2U))                          /*!< Port 2 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_2_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x2U))                          /*!< Port 2 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_3_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x3U))                          /*!< Port 3 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_3_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x3U))                          /*!< Port 3 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_4_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x4U))                          /*!< Port 4 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_4_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x4U))                          /*!< Port 4 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_5_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x5U))                          /*!< Port 5 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_5_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x5U))                          /*!< Port 5 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_6_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x6U))                          /*!< Port 6 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_6_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x6U))                          /*!< Port 6 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_7_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x7U))                          /*!< Port 7 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_7_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x7U))                          /*!< Port 7 - IO[7:4] */
#define FURI_HAL_OSPIM_IOPORT_8_LOW             ((uint32_t)(OCTOSPIM_PCR_IOLEN | 0x8U))                          /*!< Port 8 - IO[3:0] */
#define FURI_HAL_OSPIM_IOPORT_8_HIGH            ((uint32_t)(OCTOSPIM_PCR_IOHEN | 0x8U))                          /*!< Port 8 - IO[7:4] */
/**
  * @}
  */

/** @defgroup OSPI_DelayHoldQuarterCycle OSPI Delay Hold Quarter Cycle
  * @{
  */
#define FURI_HAL_OSPI_DHQC_DISABLE                ((uint32_t)0x00000000U)                                         /*!< No Delay             */
#define FURI_HAL_OSPI_DHQC_ENABLE                 ((uint32_t)OCTOSPI_TCR_DHQC)                                    /*!< Delay Hold 1/4 cycle */
/**
  * @}
  */

/** @defgroup OSPI_OperationType OSPI Operation Type
  * @{
  */
#define FURI_HAL_OSPI_OPTYPE_COMMON_CFG           ((uint32_t)0x00000000U)                                         /*!< Common configuration (indirect or auto-polling mode) */
#define FURI_HAL_OSPI_OPTYPE_READ_CFG             ((uint32_t)0x00000001U)                                         /*!< Read configuration (memory-mapped mode)              */
#define FURI_HAL_OSPI_OPTYPE_WRITE_CFG            ((uint32_t)0x00000002U)                                         /*!< Write configuration (memory-mapped mode)             */
#define FURI_HAL_OSPI_OPTYPE_WRAP_CFG             ((uint32_t)0x00000003U)                                         /*!< Wrap configuration (memory-mapped mode)              */
/**
  * @}
  */

/** @defgroup OSPI_InstructionMode OSPI Instruction Mode
  * @{
  */
#define FURI_HAL_OSPI_INSTRUCTION_NONE            ((uint32_t)0x00000000U)                                         /*!< No instruction               */
#define FURI_HAL_OSPI_INSTRUCTION_1_LINE          ((uint32_t)OCTOSPI_CCR_IMODE_0)                                 /*!< Instruction on a single line */
#define FURI_HAL_OSPI_INSTRUCTION_2_LINES         ((uint32_t)OCTOSPI_CCR_IMODE_1)                                 /*!< Instruction on two lines     */
#define FURI_HAL_OSPI_INSTRUCTION_4_LINES         ((uint32_t)(OCTOSPI_CCR_IMODE_0 | OCTOSPI_CCR_IMODE_1))         /*!< Instruction on four lines    */
#define FURI_HAL_OSPI_INSTRUCTION_8_LINES         ((uint32_t)OCTOSPI_CCR_IMODE_2)                                 /*!< Instruction on eight lines   */
/**
  * @}
  */

/** @defgroup OSPI_InstructionSize OSPI Instruction Size
  * @{
  */
#define FURI_HAL_OSPI_INSTRUCTION_8_BITS          ((uint32_t)0x00000000U)                                         /*!< 8-bit instruction  */
#define FURI_HAL_OSPI_INSTRUCTION_16_BITS         ((uint32_t)OCTOSPI_CCR_ISIZE_0)                                 /*!< 16-bit instruction */
#define FURI_HAL_OSPI_INSTRUCTION_24_BITS         ((uint32_t)OCTOSPI_CCR_ISIZE_1)                                 /*!< 24-bit instruction */
#define FURI_HAL_OSPI_INSTRUCTION_32_BITS         ((uint32_t)OCTOSPI_CCR_ISIZE)                                   /*!< 32-bit instruction */
/**
  * @}
  */

/** @defgroup OSPI_InstructionDtrMode OSPI Instruction DTR Mode
  * @{
  */
#define FURI_HAL_OSPI_INSTRUCTION_DTR_DISABLE     ((uint32_t)0x00000000U)                                         /*!< DTR mode disabled for instruction phase */
#define FURI_HAL_OSPI_INSTRUCTION_DTR_ENABLE      ((uint32_t)OCTOSPI_CCR_IDTR)                                    /*!< DTR mode enabled for instruction phase  */
/**
  * @}
  */

/** @defgroup OSPI_AddressMode OSPI Address Mode
  * @{
  */
#define FURI_HAL_OSPI_ADDRESS_NONE                ((uint32_t)0x00000000U)                                         /*!< No address               */
#define FURI_HAL_OSPI_ADDRESS_1_LINE              ((uint32_t)OCTOSPI_CCR_ADMODE_0)                                /*!< Address on a single line */
#define FURI_HAL_OSPI_ADDRESS_2_LINES             ((uint32_t)OCTOSPI_CCR_ADMODE_1)                                /*!< Address on two lines     */
#define FURI_HAL_OSPI_ADDRESS_4_LINES             ((uint32_t)(OCTOSPI_CCR_ADMODE_0 | OCTOSPI_CCR_ADMODE_1))       /*!< Address on four lines    */
#define FURI_HAL_OSPI_ADDRESS_8_LINES             ((uint32_t)OCTOSPI_CCR_ADMODE_2)                                /*!< Address on eight lines   */
/**
  * @}
  */

/** @defgroup OSPI_AddressSize OSPI Address Size
  * @{
  */
#define FURI_HAL_OSPI_ADDRESS_8_BITS              ((uint32_t)0x00000000U)                                         /*!< 8-bit address  */
#define FURI_HAL_OSPI_ADDRESS_16_BITS             ((uint32_t)OCTOSPI_CCR_ADSIZE_0)                                /*!< 16-bit address */
#define FURI_HAL_OSPI_ADDRESS_24_BITS             ((uint32_t)OCTOSPI_CCR_ADSIZE_1)                                /*!< 24-bit address */
#define FURI_HAL_OSPI_ADDRESS_32_BITS             ((uint32_t)OCTOSPI_CCR_ADSIZE)                                  /*!< 32-bit address */
/**
  * @}
  */

/** @defgroup OSPI_AddressDtrMode OSPI Address DTR Mode
  * @{
  */
#define FURI_HAL_OSPI_ADDRESS_DTR_DISABLE         ((uint32_t)0x00000000U)                                         /*!< DTR mode disabled for address phase */
#define FURI_HAL_OSPI_ADDRESS_DTR_ENABLE          ((uint32_t)OCTOSPI_CCR_ADDTR)                                   /*!< DTR mode enabled for address phase  */
/**
  * @}
  */

/** @defgroup OSPI_AlternateBytesMode OSPI Alternate Bytes Mode
  * @{
  */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_NONE        ((uint32_t)0x00000000U)                                         /*!< No alternate bytes               */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_1_LINE      ((uint32_t)OCTOSPI_CCR_ABMODE_0)                                /*!< Alternate bytes on a single line */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_2_LINES     ((uint32_t)OCTOSPI_CCR_ABMODE_1)                                /*!< Alternate bytes on two lines     */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_4_LINES     ((uint32_t)(OCTOSPI_CCR_ABMODE_0 | OCTOSPI_CCR_ABMODE_1))       /*!< Alternate bytes on four lines    */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_8_LINES     ((uint32_t)OCTOSPI_CCR_ABMODE_2)                                /*!< Alternate bytes on eight lines   */
/**
  * @}
  */

/** @defgroup OSPI_DataMode OSPI Data Mode
  * @{
  */
#define FURI_HAL_OSPI_DATA_NONE                   ((uint32_t)0x00000000U)                                         /*!< No data               */
#define FURI_HAL_OSPI_DATA_1_LINE                 ((uint32_t)OCTOSPI_CCR_DMODE_0)                                 /*!< Data on a single line */
#define FURI_HAL_OSPI_DATA_2_LINES                ((uint32_t)OCTOSPI_CCR_DMODE_1)                                 /*!< Data on two lines     */
#define FURI_HAL_OSPI_DATA_4_LINES                ((uint32_t)(OCTOSPI_CCR_DMODE_0 | OCTOSPI_CCR_DMODE_1))         /*!< Data on four lines    */
#define FURI_HAL_OSPI_DATA_8_LINES                ((uint32_t)OCTOSPI_CCR_DMODE_2)                                 /*!< Data on eight lines   */
/**
  * @}
  */

/** @defgroup OSPI_DataDtrMode OSPI Data DTR Mode
  * @{
  */
#define FURI_HAL_OSPI_DATA_DTR_DISABLE            ((uint32_t)0x00000000U)                                         /*!< DTR mode disabled for data phase */
#define FURI_HAL_OSPI_DATA_DTR_ENABLE             ((uint32_t)OCTOSPI_CCR_DDTR)                                    /*!< DTR mode enabled for data phase  */
/**
  * @}
  */

/** @defgroup OSPI_DQSMode OSPI DQS Mode
  * @{
  */
#define FURI_HAL_OSPI_DQS_DISABLE                 ((uint32_t)0x00000000U)                                         /*!< DQS disabled */
#define FURI_HAL_OSPI_DQS_ENABLE                  ((uint32_t)OCTOSPI_CCR_DQSE)                                    /*!< DQS enabled  */
/**
  * @}
  */

/** @defgroup OSPI_SIOOMode OSPI SIOO Mode
  * @{
  */
#define FURI_HAL_OSPI_SIOO_INST_EVERY_CMD         ((uint32_t)0x00000000U)                                         /*!< Send instruction on every transaction       */
#define FURI_HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD    ((uint32_t)OCTOSPI_CCR_SIOO)                                    /*!< Send instruction only for the first command */
/**
  * @}
  */

/** @defgroup OSPI_FlashID OSPI Flash Id
  * @{
  */
#define FURI_HAL_OSPI_FLASH_ID_1                  ((uint32_t)0x00000000U)                                         /*!< FLASH 1 selected */
#define FURI_HAL_OSPI_FLASH_ID_2                  ((uint32_t)OCTOSPI_CR_MSEL)                                     /*!< FLASH 2 selected */
/**
  * @}
  */

 /** @defgroup OSPI_AlternateBytesSize OSPI Alternate Bytes Size
  * @{
  */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_8_BITS      ((uint32_t)0x00000000U)                                         /*!< 8-bit alternate bytes  */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_16_BITS     ((uint32_t)OCTOSPI_CCR_ABSIZE_0)                                /*!< 16-bit alternate bytes */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_24_BITS     ((uint32_t)OCTOSPI_CCR_ABSIZE_1)                                /*!< 24-bit alternate bytes */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_32_BITS     ((uint32_t)OCTOSPI_CCR_ABSIZE)                                  /*!< 32-bit alternate bytes */
/**
  * @}
  */

 /** @defgroup OSPI_AlternateBytesSize OSPI Alternate Bytes Size
  * @{
  */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_8_BITS      ((uint32_t)0x00000000U)                                         /*!< 8-bit alternate bytes  */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_16_BITS     ((uint32_t)OCTOSPI_CCR_ABSIZE_0)                                /*!< 16-bit alternate bytes */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_24_BITS     ((uint32_t)OCTOSPI_CCR_ABSIZE_1)                                /*!< 24-bit alternate bytes */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_32_BITS     ((uint32_t)OCTOSPI_CCR_ABSIZE)                                  /*!< 32-bit alternate bytes */
/**
  * @}
  */

/** @defgroup OSPI_AlternateBytesDtrMode OSPI Alternate Bytes DTR Mode
  * @{
  */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_DTR_DISABLE ((uint32_t)0x00000000U)                                         /*!< DTR mode disabled for alternate bytes phase */
#define FURI_HAL_OSPI_ALTERNATE_BYTES_DTR_ENABLE  ((uint32_t)OCTOSPI_CCR_ABDTR)                                   /*!< DTR mode enabled for alternate bytes phase  */
/**
  * @}
  */

typedef struct {
    uint32_t clk_port; /*!< It indicates which port of the OSPI IO Manager is used for the CLK pins.
                                        This parameter can be a value between 1 and 8 */
    uint32_t dqs_port; /*!< It indicates which port of the OSPI IO Manager is used for the DQS pin.
                                        This parameter can be a value between 0 and 8, 0 means that signal not used */
    uint32_t ncs_port; /*!< It indicates which port of the OSPI IO Manager is used for the NCS pin.
                                        This parameter can be a value between 1 and 8 */
    uint32_t
        io_low_port; /*!< It indicates which port of the OSPI IO Manager is used for the IO[3:0] pins.
                                        This parameter can be a value of @ref OSPIM_IOPort */
    uint32_t
        io_high_port; /*!< It indicates which port of the OSPI IO Manager is used for the IO[7:4] pins.
                                        This parameter can be a value of @ref OSPIM_IOPort */
    uint32_t
        req2_ask_time; /*!< It indicates the minimum switching duration (in number of clock cycles) expected
                                        if some signals are multiplexed in the OSPI IO Manager with the other OSPI.
                                        This parameter can be a value between 1 and 256 */
} FuriHalOspiConfig;

typedef struct {
    uint32_t
        operation_type; /*!< It indicates if the configuration applies to the common registers or
                                           to the registers for the write operation (these registers are only
                                           used for memory-mapped mode).
                                           This parameter can be a value of @ref OSPI_OperationType */
    uint32_t flash_id; /*!< It indicates which external device is selected for this command (it
                                           applies only if Dualquad is disabled in the initialization structure).
                                           This parameter can be a value of @ref OSPI_FlashID */
    uint32_t instruction; /*!< It contains the instruction to be sent to the device.
                                           This parameter can be a value between 0 and 0xFFFFFFFF */
    uint32_t instruction_mode; /*!< It indicates the mode of the instruction.
                                           This parameter can be a value of @ref OSPI_InstructionMode */
    uint32_t instruction_size; /*!< It indicates the size of the instruction.
                                           This parameter can be a value of @ref OSPI_InstructionSize */
    uint32_t instruction_dtr_mode; /*!< It enables or not the DTR mode for the instruction phase.
                                           This parameter can be a value of @ref OSPI_InstructionDtrMode */
    uint32_t address; /*!< It contains the address to be sent to the device.
                                           This parameter can be a value between 0 and 0xFFFFFFFF */
    uint32_t address_mode; /*!< It indicates the mode of the address.
                                           This parameter can be a value of @ref OSPI_AddressMode */
    uint32_t address_size; /*!< It indicates the size of the address.
                                           This parameter can be a value of @ref OSPI_AddressSize */
    uint32_t address_dtr_mode; /*!< It enables or not the DTR mode for the address phase.
                                           This parameter can be a value of @ref OSPI_AddressDtrMode */
    uint32_t alternate_bytes; /*!< It contains the alternate bytes to be sent to the device.
                                           This parameter can be a value between 0 and 0xFFFFFFFF */
    uint32_t alternate_bytes_mode; /*!< It indicates the mode of the alternate bytes.
                                           This parameter can be a value of @ref OSPI_AlternateBytesMode */
    uint32_t alternate_bytes_size; /*!< It indicates the size of the alternate bytes.
                                           This parameter can be a value of @ref OSPI_AlternateBytesSize */
    uint32_t
        alternate_bytes_dtr_mode; /*!< It enables or not the DTR mode for the alternate bytes phase.
                                           This parameter can be a value of @ref OSPI_AlternateBytesDtrMode */
    uint32_t data_mode; /*!< It indicates the mode of the data.
                                           This parameter can be a value of @ref OSPI_DataMode */
    uint32_t nb_data; /*!< It indicates the number of data transferred with this command.
                                           This field is only used for indirect mode.
                                           This parameter can be a value between 1 and 0xFFFFFFFF */
    uint32_t data_dtr_mode; /*!< It enables or not the DTR mode for the data phase.
                                           This parameter can be a value of @ref OSPI_DataDtrMode */
    uint32_t dummy_cycles; /*!< It indicates the number of dummy cycles inserted before data phase.
                                           This parameter can be a value between 0 and 31 */
    uint32_t dqs_mode; /*!< It enables or not the data strobe management.
                                           This parameter can be a value of @ref OSPI_DQSMode */
    uint32_t sdio_mode; /*!< It enables or not the SIOO mode.
                                           This parameter can be a value of @ref OSPI_SIOOMode */
} FuriHalOspiCommand;

void furi_hal_ospi_init(void);
void furi_hal_ospi_deint(void);
bool furi_hal_ospi_abort(void);
void furi_hal_ospi_dlyb_set_config(LL_DLYB_CfgTypeDef* pdlyb_cfg);
bool furi_hal_ospi_dlyb_get_clock_period(LL_DLYB_CfgTypeDef* pdlyb_cfg);
void furi_hal_ospi_config_no_mux_ospi1(FuriHalOspiConfig* cfg);
bool furi_hal_ospi_command(FuriHalOspiCommand* cmd);
bool furi_hal_ospi_transmit(uint8_t* p_data);
bool furi_hal_ospi_receive(uint8_t* p_data);
void furi_hal_ospi_memory_mapped(uint32_t time_out_activation, uint32_t time_out_period);