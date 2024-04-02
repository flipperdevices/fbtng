#pragma once

/** @defgroup PSSI_DATA_WIDTH PSSI Data Width
  * @{
  */
#define FURI_HAL_PSSI_8BITS                  (0x00000000U)   /*!<  8 Bits  */
#define FURI_HAL_PSSI_16BITS                 (0x00000001U)   /*!< 16 Bits  */
#define FURI_HAL_PSSI_32BITS                 (0x00000002U)   /*!< 32 Bits  */
/**
  * @}
  */

/** @defgroup PSSI_BUS_WIDTH PSSI Bus Width
  * @{
  */
#define FURI_HAL_PSSI_8LINES                 (0x00000000U)   /*!< 8 data lines  */
#define FURI_HAL_PSSI_16LINES                (PSSI_CR_EDM)   /*!< 16 data lines */
/**
  * @}
  */

/** @defgroup ControlSignal_Configuration ControlSignal Configuration
  * @{
  */
#define FURI_HAL_PSSI_DE_RDY_DISABLE           (0x0U << PSSI_CR_DERDYCFG_Pos) /*!< Neither DE nor RDY are enabled */
#define FURI_HAL_PSSI_RDY_ENABLE               (0x1U << PSSI_CR_DERDYCFG_Pos) /*!< Only RDY enabled */
#define FURI_HAL_PSSI_DE_ENABLE                (0x2U << PSSI_CR_DERDYCFG_Pos) /*!< Only DE enabled */
#define FURI_HAL_PSSI_DE_RDY_ALT_ENABLE        (0x3U << PSSI_CR_DERDYCFG_Pos) /*!< Both RDY and DE alternate functions enabled */
#define FURI_HAL_PSSI_MAP_RDY_BIDIR_ENABLE     (0x4U << PSSI_CR_DERDYCFG_Pos) /*!< Bi-directional on RDY pin */
#define FURI_HAL_PSSI_RDY_MAP_ENABLE           (0x5U << PSSI_CR_DERDYCFG_Pos) /*!< Only RDY enabled, mapped to DE pin */
#define FURI_HAL_PSSI_DE_MAP_ENABLE            (0x6U << PSSI_CR_DERDYCFG_Pos) /*!< Only DE enabled, mapped to RDY pin */
#define FURI_HAL_PSSI_MAP_DE_BIDIR_ENABLE      (0x7U << PSSI_CR_DERDYCFG_Pos) /*!< Bi-directional on DE pin */
/**
  * @}
  */

/** @defgroup Clock_Polarity Clock Polarity
  * @{
  */
#define FURI_HAL_PSSI_FALLING_EDGE             (0x0U)            /*!< Fallling Edge */
#define FURI_HAL_PSSI_RISING_EDGE              (0x1U)            /*!< Rising Edge */
/**
  * @}
  */

/** @defgroup Data_Enable_Polarity Data Enable Polarity
  * @{
  */
#define FURI_HAL_PSSI_DEPOL_ACTIVE_LOW         (0x0U)            /*!< Active Low */
#define FURI_HAL_PSSI_DEPOL_ACTIVE_HIGH        (PSSI_CR_DEPOL)   /*!< Active High */
/**
  * @}
  */

/** @defgroup Reday_Polarity Reday Polarity
  * @{
  */
#define FURI_HAL_PSSI_RDYPOL_ACTIVE_LOW        (0x0U)            /*!< Active Low */
#define FURI_HAL_PSSI_RDYPOL_ACTIVE_HIGH       (PSSI_CR_RDYPOL)  /*!< Active High */
/**
  * @}
  */

 /** @defgroup PSSI_DEFINITION PSSI definitions
  * @{
  */
#define FURI_HAL_PSSI_MAX_NBYTE_SIZE         (0x10000U)         /* 64 KB */
#define FURI_HAL_PSSI_TIMEOUT_TRANSMIT       (0x0000FFFFU)      /*!< Timeout Value   */

#define FURI_HAL_PSSI_CR_OUTEN_INPUT         (0x00000000U)      /*!< Input Mode      */
#define FURI_HAL_PSSI_CR_OUTEN_OUTPUT        (PSSI_CR_OUTEN)    /*!< Output Mode     */

#define FURI_HAL_PSSI_CR_DMA_ENABLE          (PSSI_CR_DMAEN)    /*!< DMA Mode Enable */
#define FURI_HAL_PSSI_CR_DMA_DISABLE         (~PSSI_CR_DMAEN) /*!< DMA Mode Disable*/

#define FURI_HAL_PSSI_CR_16BITS              (PSSI_CR_EDM)      /*!< 16 Lines Mode   */
#define PFURI_HAL_SSI_CR_8BITS               (~PSSI_CR_EDM)   /*!< 8 Lines Mode    */

#define FURI_HAL_PSSI_FLAG_RTT1B             (PSSI_SR_RTT1B)    /*!< 1 Byte Fifo Flag */
#define FURI_HAL_PSSI_FLAG_RTT4B             (PSSI_SR_RTT4B)    /*!< 4 Bytes Fifo Flag*/
/**
  * @}
  */

/** @defgroup Clock_Polarity Clock Polarity
  * @{
  */
#define FURI_HAL_PSSI_FALLING_EDGE             (0x0U)            /*!< Fallling Edge */
#define FURI_HAL_PSSI_RISING_EDGE              (0x1U)            /*!< Rising Edge */
/**
  * @}
  */

 
/** @defgroup PSSI_Interrupts PSSI Interrupts
  * @{
  */

#define FURI_HAL_PSSI_FLAG_OVR_RIS            PSSI_RIS_OVR_RIS     /*!< Overrun, Underrun errors flag */
#define FURI_HAL_PSSI_FLAG_MASK               PSSI_RIS_OVR_RIS_Msk /*!< Overrun, Underrun errors Mask */
#define FURI_HAL_PSSI_FLAG_OVR_MIS            PSSI_MIS_OVR_MIS     /*!< Overrun, Underrun masked errors flag */
/**
  * @}
  */