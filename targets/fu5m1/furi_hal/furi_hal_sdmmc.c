#include <core/log.h>
#include <furi_hal_bus.h>
#include <furi_hal_cortex.h>
#include <furi_hal_clock.h>
#include <furi_hal_interrupt.h>
#include <furi_hal_sdmmc.h>
#include <furi_hal_resources.h>
#include <stm32u5xx_ll_rcc.h>
#include <stm32u5xx_ll_sdmmc.h>

#define TAG "FuriHalSDMMC"
#define FURI_SDMMC_SWDATATIMEOUT ((uint32_t)1000U)
#define FURI_SDMMC_BLOCK SDMMC1
#define FURI_SDMMC_BUS FuriHalBusSDMMC1
#define FURI_SDMMC_IRQ SDMMC1_IRQn
#define FURI_SDMMC_PIN_ALTFN GpioAltFn12SDMMC1

#define SD_INIT_FREQ 400000U /* Initialization phase : 400 kHz max */
#define SD_NORMAL_SPEED_FREQ 25000000U /* Normal speed phase : 25 MHz max */
#define SD_HIGH_SPEED_FREQ 50000000U /* High speed phase : 50 MHz max */
#define SD_BLOCKSIZE ((uint32_t)512U) /*!< Block size is 512 bytes */

typedef enum {
    FuriHalSdErrorNone = SDMMC_ERROR_NONE,
    FuriHalSdErrorDataCrcFail = SDMMC_ERROR_DATA_CRC_FAIL,
    FuriHalSdErrorDataTimeout = SDMMC_ERROR_DATA_TIMEOUT,
    FuriHalSdErrorTxUnderrun = SDMMC_ERROR_TX_UNDERRUN,
    FuriHalSdErrorRxOverrun = SDMMC_ERROR_RX_OVERRUN,
    FuriHalSdErrorAddrMisaligned = SDMMC_ERROR_ADDR_MISALIGNED,
    FuriHalSdErrorBlockLenErr = SDMMC_ERROR_BLOCK_LEN_ERR,
    FuriHalSdErrorEraseSeqErr = SDMMC_ERROR_ERASE_SEQ_ERR,
    FuriHalSdErrorBadEraseParam = SDMMC_ERROR_BAD_ERASE_PARAM,
    FuriHalSdErrorWriteProtViolation = SDMMC_ERROR_WRITE_PROT_VIOLATION,
    FuriHalSdErrorLockUnlockFailed = SDMMC_ERROR_LOCK_UNLOCK_FAILED,
    FuriHalSdErrorComCrcFailed = SDMMC_ERROR_COM_CRC_FAILED,
    FuriHalSdErrorIllegalCmd = SDMMC_ERROR_ILLEGAL_CMD,
    FuriHalSdErrorCardEccFailed = SDMMC_ERROR_CARD_ECC_FAILED,
    FuriHalSdErrorCcErr = SDMMC_ERROR_CC_ERR,
    FuriHalSdErrorGeneralUnknownErr = SDMMC_ERROR_GENERAL_UNKNOWN_ERR,
    FuriHalSdErrorStreamReadUnderrun = SDMMC_ERROR_STREAM_READ_UNDERRUN,
    FuriHalSdErrorStreamWriteOverrun = SDMMC_ERROR_STREAM_WRITE_OVERRUN,
    FuriHalSdErrorCidCsdOverwrite = SDMMC_ERROR_CID_CSD_OVERWRITE,
    FuriHalSdErrorWpEraseSkip = SDMMC_ERROR_WP_ERASE_SKIP,
    FuriHalSdErrorCardEccDisabled = SDMMC_ERROR_CARD_ECC_DISABLED,
    FuriHalSdErrorEraseReset = SDMMC_ERROR_ERASE_RESET,
    FuriHalSdErrorAkeSeqErr = SDMMC_ERROR_AKE_SEQ_ERR,
    FuriHalSdErrorInvalidVoltRange = SDMMC_ERROR_INVALID_VOLTRANGE,
    FuriHalSdErrorAddrOutOfRange = SDMMC_ERROR_ADDR_OUT_OF_RANGE,
    FuriHalSdErrorRequestNotApplicable = SDMMC_ERROR_REQUEST_NOT_APPLICABLE,
    FuriHalSdErrorParam = SDMMC_ERROR_INVALID_PARAMETER,
    FuriHalSdErrorUnsupportedFeature = SDMMC_ERROR_UNSUPPORTED_FEATURE,
    FuriHalSdErrorBusy = SDMMC_ERROR_BUSY,
    FuriHalSdErrorDma = SDMMC_ERROR_DMA,
    FuriHalSdErrorTimeout = SDMMC_ERROR_TIMEOUT,
} FuriHalSdError;

typedef struct {
    uint8_t CSDStruct; /*!< CSD structure */
    uint8_t SysSpecVersion; /*!< System specification version */
    uint8_t Reserved1; /*!< Reserved */
    uint8_t TAAC; /*!< Data read access time 1 */
    uint8_t NSAC; /*!< Data read access time 2 in CLK cycles */
    uint8_t MaxBusClkFrec; /*!< Max. bus clock frequency */
    uint16_t CardComdClasses; /*!< Card command classes */
    uint8_t RdBlockLen; /*!< Max. read data block length */
    uint8_t PartBlockRead; /*!< Partial blocks for read allowed */
    uint8_t WrBlockMisalign; /*!< Write block misalignment */
    uint8_t RdBlockMisalign; /*!< Read block misalignment */
    uint8_t DSRImpl; /*!< DSR implemented */
    uint8_t Reserved2; /*!< Reserved */
    uint32_t DeviceSize; /*!< Device Size */
    uint8_t MaxRdCurrentVDDMin; /*!< Max. read current @ VDD min */
    uint8_t MaxRdCurrentVDDMax; /*!< Max. read current @ VDD max */
    uint8_t MaxWrCurrentVDDMin; /*!< Max. write current @ VDD min */
    uint8_t MaxWrCurrentVDDMax; /*!< Max. write current @ VDD max */
    uint8_t DeviceSizeMul; /*!< Device size multiplier */
    uint8_t EraseGrSize; /*!< Erase group size */
    uint8_t EraseGrMul; /*!< Erase group size multiplier */
    uint8_t WrProtectGrSize; /*!< Write protect group size */
    uint8_t WrProtectGrEnable; /*!< Write protect group enable */
    uint8_t ManDeflECC; /*!< Manufacturer default ECC */
    uint8_t WrSpeedFact; /*!< Write speed factor */
    uint8_t MaxWrBlockLen; /*!< Max. write data block length */
    uint8_t WriteBlockPaPartial; /*!< Partial blocks for write allowed */
    uint8_t Reserved3; /*!< Reserved */
    uint8_t ContentProtectAppli; /*!< Content protection application */
    uint8_t FileFormatGroup; /*!< File format group */
    uint8_t CopyFlag; /*!< Copy flag (OTP */
    uint8_t PermWrProtect; /*!< Permanent write protection */
    uint8_t TempWrProtect; /*!< Temporary write protection */
    uint8_t FileFormat; /*!< File format */
    uint8_t ECC; /*!< ECC code */
    uint8_t CSD_CRC; /*!< CSD CRC */
    uint8_t Reserved4; /*!< Always 1 */
} CardCSDInfo;

typedef struct {
    uint8_t DataBusWidth; /*!< Shows the currently defined data bus width */
    uint8_t SecuredMode; /*!< Card is in secured mode of operation */
    uint16_t CardType; /*!< Carries information about card type */
    uint32_t ProtectedAreaSize; /*!< Carries information about the capacity of protected area */
    uint8_t SpeedClass; /*!< Carries information about the speed class of the card */
    uint8_t PerformanceMove; /*!< Carries information about the card's performance move */
    uint8_t AllocationUnitSize; /*!< Carries information about the card's allocation unit size */
    uint16_t EraseSize; /*!< Determines the number of AUs to be erased in one operation */
    uint8_t EraseTimeout; /*!< Determines the timeout for any number of AU erase */
    uint8_t EraseOffset; /*!< Carries information about the erase offset */
    uint8_t UhsSpeedGrade; /*!< Carries information about the speed grade of UHS card */
    uint8_t
        UhsAllocationUnitSize; /*!< Carries information about the UHS card's allocation unit size */
    uint8_t VideoSpeedClass; /*!< Carries information about the Video Speed Class of UHS card */
} CardStatus;

typedef struct {
    FuriHalSdMmcPresentCallback present_callback;
    void* context;

    CardCSDInfo csd;
    CardStatus status;

    FuriHalSdInfo info;
    uint32_t card_rca;
    bool card_alive;
} SdMmc;

static SdMmc sdmmc1 = {0};

typedef enum {
    SdMmcDmaStateEnabled = 1 << 0,
    SdMmcDmaStateRxMulti = 1 << 1,
    SdMmcDmaStateRxSingle = 1 << 2,
    SdMmcDmaStateTxMulti = 1 << 3,
    SdMmcDmaStateTxSingle = 1 << 4,
} SdMmcDmaState;

typedef enum {
    SdMmcDmaEventComplete = 1 << 0,
    SdMmcDmaEventError = 1 << 1,
} SdMmcDmaEvent;

typedef struct {
    uint8_t* rx_buffer;
    size_t rx_size;
    const uint8_t* tx_buffer;
    size_t tx_size;

    uint32_t state;
    FuriHalSdError error;
    FuriEventFlag* event;
} SdMmcDmaContext;

static SdMmcDmaContext sdmmc_dma_context = {0};

static void furi_hal_sdmmc_gpio_init(void) {
    furi_hal_gpio_write(&gpio_sd_card_power_switch, 0);
    furi_hal_gpio_init(
        &gpio_sd_card_power_switch, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_sd_card_detect, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedLow);
}

bool furi_hal_sdmmc_is_sd_present(void) {
    bool sd_present = (furi_hal_gpio_read(&gpio_sd_card_detect) == 0);
    return sd_present;
}

void furi_hal_sdmmc_init(void) {
    furi_hal_sdmmc_gpio_init();
    sdmmc_dma_context.event = furi_event_flag_alloc();
    sdmmc1.card_alive = false;

    FURI_LOG_I(TAG, "Init OK");
}

static void furi_hal_sdmmc_present_callback(void* context) {
    SdMmc sdmmc = *(SdMmc*)context;
    if(sdmmc.present_callback) {
        sdmmc.present_callback(sdmmc.context);
    }
}

void furi_hal_sdmmc_set_presence_callback(FuriHalSdMmcPresentCallback callback, void* context) {
    sdmmc1.present_callback = callback;
    sdmmc1.context = context;

    if(sdmmc1.present_callback) {
        furi_hal_gpio_add_int_callback(
            &gpio_sd_card_detect, furi_hal_sdmmc_present_callback, &sdmmc1);
    } else {
        furi_hal_gpio_remove_int_callback(&gpio_sd_card_detect);
    }
}

static void furi_hal_sdmmc_periph_init(void) {
    furi_hal_gpio_init_ex(
        &gpio_sd_card_d0,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        FURI_SDMMC_PIN_ALTFN);
    furi_hal_gpio_init_ex(
        &gpio_sd_card_d1,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        FURI_SDMMC_PIN_ALTFN);
    furi_hal_gpio_init_ex(
        &gpio_sd_card_d2,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        FURI_SDMMC_PIN_ALTFN);
    furi_hal_gpio_init_ex(
        &gpio_sd_card_d3,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        FURI_SDMMC_PIN_ALTFN);
    furi_hal_gpio_init_ex(
        &gpio_sd_card_ck,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        FURI_SDMMC_PIN_ALTFN);
    furi_hal_gpio_init_ex(
        &gpio_sd_card_cmd,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        FURI_SDMMC_PIN_ALTFN);

    LL_RCC_SetSDMMCKernelClockSource(LL_RCC_SDMMC12_KERNELCLKSOURCE_PLL1);

    furi_hal_bus_enable(FURI_SDMMC_BUS);
}

static void furi_hal_sdmmc_periph_deinit(void) {
    furi_hal_bus_disable(FURI_SDMMC_BUS);
    furi_hal_gpio_init_simple(&gpio_sd_card_d0, GpioModeAnalog);
    furi_hal_gpio_init_simple(&gpio_sd_card_d1, GpioModeAnalog);
    furi_hal_gpio_init_simple(&gpio_sd_card_d2, GpioModeAnalog);
    furi_hal_gpio_init_simple(&gpio_sd_card_d3, GpioModeAnalog);
    furi_hal_gpio_init_simple(&gpio_sd_card_ck, GpioModeAnalog);
    furi_hal_gpio_init_simple(&gpio_sd_card_cmd, GpioModeAnalog);
}

static void furi_hal_sdmmc_card_enable_power(void) {
    furi_hal_gpio_write(&gpio_sd_card_power_switch, 1);
    // we need about 1.2ms to stabilize the power
    furi_delay_ms(2);
}

static void furi_hal_sdmmc_card_disable_power(void) {
    furi_hal_gpio_write(&gpio_sd_card_power_switch, 0);
}

static uint32_t sdmmc_power_on(void) {
    uint32_t errorstate;

    /* CMD0: GO_IDLE_STATE */
    errorstate = SDMMC_CmdGoIdleState(FURI_SDMMC_BLOCK);
    if(errorstate != FuriHalSdErrorNone) {
        return errorstate;
    }

    /* CMD8: SEND_IF_COND: Command available only on V2.0 cards */
    errorstate = SDMMC_CmdOperCond(FURI_SDMMC_BLOCK);
    if(errorstate == SDMMC_ERROR_CMD_RSP_TIMEOUT) /* No response to CMD8 */
    {
        FURI_LOG_D(TAG, "No response to CMD8, assume SD card v1.x");
        sdmmc1.info.version = FuriHalSdVersion1;

        /* CMD0: GO_IDLE_STATE */
        errorstate = SDMMC_CmdGoIdleState(FURI_SDMMC_BLOCK);
        if(errorstate != FuriHalSdErrorNone) {
            return errorstate;
        }
    } else {
        FURI_LOG_D(TAG, "Response to CMD8, assume SD card v2.x");
        sdmmc1.info.version = FuriHalSdVersion2;

        /* SEND CMD55 APP_CMD with RCA as 0 */
        errorstate = SDMMC_CmdAppCommand(FURI_SDMMC_BLOCK, 0);
        if(errorstate != FuriHalSdErrorNone) {
            return FuriHalSdErrorUnsupportedFeature;
        }
    }

    uint32_t count = 0U;
    uint32_t response = 0U;

    /* SD CARD */
    /* Send ACMD41 SD_APP_OP_COND with Argument 0x80100000 */
    while((count < SDMMC_MAX_VOLT_TRIAL) && (!((response >> 31U) == 1U))) {
        /* SEND CMD55 APP_CMD with RCA as 0 */
        errorstate = SDMMC_CmdAppCommand(FURI_SDMMC_BLOCK, 0);
        if(errorstate != FuriHalSdErrorNone) {
            return errorstate;
        }

        /* Send CMD41 */
        errorstate = SDMMC_CmdAppOperCommand(
            FURI_SDMMC_BLOCK,
            SDMMC_VOLTAGE_WINDOW_SD | SDMMC_HIGH_CAPACITY | SD_SWITCH_1_8V_CAPACITY);
        if(errorstate != FuriHalSdErrorNone) {
            return FuriHalSdErrorUnsupportedFeature;
        }

        /* Get command response */
        response = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP1);

        count++;
    }

    if(count >= SDMMC_MAX_VOLT_TRIAL) {
        return FuriHalSdErrorInvalidVoltRange;
    }

    /* Check card type */
    sdmmc1.info.type = FuriHalSdTypeSC;
    if((response & SDMMC_HIGH_CAPACITY) == SDMMC_HIGH_CAPACITY) {
        sdmmc1.info.type = FuriHalSdTypeHCXC;
    }

    return FuriHalSdErrorNone;
}

static inline uint32_t sdmmc_get_flags(uint32_t mask) {
    return __SDMMC_GET_FLAG(FURI_SDMMC_BLOCK, mask);
}

static inline void sdmmc_clear_flags(uint32_t flags) {
    __SDMMC_CLEAR_FLAG(FURI_SDMMC_BLOCK, flags);
}

static inline void sdmmc_clear_static_flags(void) {
    sdmmc_clear_flags(SDMMC_STATIC_FLAGS);
}

static inline void sdmmc_clear_static_data_flags(void) {
    sdmmc_clear_flags(SDMMC_STATIC_DATA_FLAGS);
}

static inline void sdmmc_disable_it(uint32_t it) {
    __SDMMC_DISABLE_IT(FURI_SDMMC_BLOCK, it);
}

static inline void sdmmc_enable_it(uint32_t it) {
    __SDMMC_ENABLE_IT(FURI_SDMMC_BLOCK, it);
}

static void sdmmc_parse_csd(CardCSDInfo* info, uint32_t csd[4]) {
    furi_check(sdmmc1.info.type == FuriHalSdTypeSC || sdmmc1.info.type == FuriHalSdTypeHCXC);

    info->CSDStruct = (uint8_t)((csd[0] & 0xC0000000U) >> 30U);
    info->SysSpecVersion = (uint8_t)((csd[0] & 0x3C000000U) >> 26U);
    info->Reserved1 = (uint8_t)((csd[0] & 0x03000000U) >> 24U);
    info->TAAC = (uint8_t)((csd[0] & 0x00FF0000U) >> 16U);
    info->NSAC = (uint8_t)((csd[0] & 0x0000FF00U) >> 8U);
    info->MaxBusClkFrec = (uint8_t)(csd[0] & 0x000000FFU);
    info->CardComdClasses = (uint16_t)((csd[1] & 0xFFF00000U) >> 20U);
    info->RdBlockLen = (uint8_t)((csd[1] & 0x000F0000U) >> 16U);
    info->PartBlockRead = (uint8_t)((csd[1] & 0x00008000U) >> 15U);
    info->WrBlockMisalign = (uint8_t)((csd[1] & 0x00004000U) >> 14U);
    info->RdBlockMisalign = (uint8_t)((csd[1] & 0x00002000U) >> 13U);
    info->DSRImpl = (uint8_t)((csd[1] & 0x00001000U) >> 12U);
    info->Reserved2 = 0U;

    if(sdmmc1.info.type == FuriHalSdTypeSC) {
        info->DeviceSize = (((csd[1] & 0x000003FFU) << 2U) | ((csd[2] & 0xC0000000U) >> 30U));
        info->MaxRdCurrentVDDMin = (uint8_t)((csd[2] & 0x38000000U) >> 27U);
        info->MaxRdCurrentVDDMax = (uint8_t)((csd[2] & 0x07000000U) >> 24U);
        info->MaxWrCurrentVDDMin = (uint8_t)((csd[2] & 0x00E00000U) >> 21U);
        info->MaxWrCurrentVDDMax = (uint8_t)((csd[2] & 0x001C0000U) >> 18U);
        info->DeviceSizeMul = (uint8_t)((csd[2] & 0x00038000U) >> 15U);
    } else {
        info->DeviceSize = (((csd[1] & 0x0000003FU) << 16U) | ((csd[2] & 0xFFFF0000U) >> 16U));
    }

    info->EraseGrSize = (uint8_t)((csd[2] & 0x00004000U) >> 14U);
    info->EraseGrMul = (uint8_t)((csd[2] & 0x00003F80U) >> 7U);
    info->WrProtectGrSize = (uint8_t)(csd[2] & 0x0000007FU);
    info->WrProtectGrEnable = (uint8_t)((csd[3] & 0x80000000U) >> 31U);
    info->ManDeflECC = (uint8_t)((csd[3] & 0x60000000U) >> 29U);
    info->WrSpeedFact = (uint8_t)((csd[3] & 0x1C000000U) >> 26U);
    info->MaxWrBlockLen = (uint8_t)((csd[3] & 0x03C00000U) >> 22U);
    info->WriteBlockPaPartial = (uint8_t)((csd[3] & 0x00200000U) >> 21U);
    info->Reserved3 = 0;
    info->ContentProtectAppli = (uint8_t)((csd[3] & 0x00010000U) >> 16U);
    info->FileFormatGroup = (uint8_t)((csd[3] & 0x00008000U) >> 15U);
    info->CopyFlag = (uint8_t)((csd[3] & 0x00004000U) >> 14U);
    info->PermWrProtect = (uint8_t)((csd[3] & 0x00002000U) >> 13U);
    info->TempWrProtect = (uint8_t)((csd[3] & 0x00001000U) >> 12U);
    info->FileFormat = (uint8_t)((csd[3] & 0x00000C00U) >> 10U);
    info->ECC = (uint8_t)((csd[3] & 0x00000300U) >> 8U);
    info->CSD_CRC = (uint8_t)((csd[3] & 0x000000FEU) >> 1U);
    info->Reserved4 = 1;
}

static void sdmmc_parse_info(FuriHalSdInfo* info, CardCSDInfo* csd, uint32_t cid[4]) {
    if(info->type == FuriHalSdTypeSC) {
        uint32_t block_count = (csd->DeviceSize + 1U);
        block_count *= (1UL << ((csd->DeviceSizeMul & 0x07U) + 2U));
        uint32_t block_size = (1UL << (csd->RdBlockLen & 0x0FU));

        info->logical_block_count = (block_count) * ((block_size) / SD_BLOCKSIZE);
        info->logical_block_size = SD_BLOCKSIZE;
    } else {
        info->logical_block_count = ((csd->DeviceSize + 1U) * 1024U);
        info->logical_block_size = SD_BLOCKSIZE;
    }

    info->manufacturer_id = (uint8_t)((cid[0] & 0xFF000000U) >> 24U);
    info->oem_id[0] = (char)((cid[0] & 0x00FF0000U) >> 16U);
    info->oem_id[1] = (char)((cid[0] & 0x0000FF00U) >> 8U);
    info->oem_id[2] = '\0';
    info->product_name[0] = (char)((cid[0] & 0x000000FFU) >> 0U);
    info->product_name[1] = (char)((cid[1] & 0xFF000000U) >> 24U);
    info->product_name[2] = (char)((cid[1] & 0x00FF0000U) >> 16U);
    info->product_name[3] = (char)((cid[1] & 0x0000FF00U) >> 8U);
    info->product_name[4] = (char)((cid[1] & 0x000000FFU) >> 0U);
    info->product_name[5] = '\0';
    info->product_revision_major = (uint8_t)((cid[2] & 0xFF000000U) >> 28U);
    info->product_revision_minor = (uint8_t)((cid[2] & 0xFF000000U) >> 24U) & 0x0FU;
    info->product_serial_number =
        (uint32_t)(((cid[2] & 0x00FFFFFFU) << 8U) | ((cid[3] & 0xFF000000U) >> 24U));
    info->manufacturing_month = (uint8_t)((cid[3] & 0x000FFF00U) >> 8U) & 0x0FU;
    info->manufacturing_year = 2000 + (uint16_t)((cid[3] & 0x000FFF00U) >> 12U);
}

static uint32_t sdmmc_init_card(void) {
    uint32_t errorstate;
    uint16_t rca = 0U;
    FuriHalCortexTimer timer = furi_hal_cortex_timer_get(SDMMC_CMDTIMEOUT * 1000U);

    uint32_t CSD[4] = {0};
    uint32_t CID[4] = {0};

    /* Check the power State */
    if(SDMMC_GetPowerState(FURI_SDMMC_BLOCK) == 0U) {
        /* Power off */
        return FuriHalSdErrorRequestNotApplicable;
    }

    /* Send CMD2 ALL_SEND_CID */
    errorstate = SDMMC_CmdSendCID(FURI_SDMMC_BLOCK);
    if(errorstate != FuriHalSdErrorNone) {
        return errorstate;
    } else {
        /* Get Card identification number data */
        CID[0U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP1);
        CID[1U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP2);
        CID[2U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP3);
        CID[3U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP4);
    }

    /* Send CMD3 SET_REL_ADDR with argument 0 */
    /* SD Card publishes its RCA. */
    while(rca == 0U) {
        errorstate = SDMMC_CmdSetRelAdd(FURI_SDMMC_BLOCK, &rca);
        if(errorstate != FuriHalSdErrorNone) {
            return errorstate;
        }
        if(furi_hal_cortex_timer_is_expired(timer)) {
            return FuriHalSdErrorTimeout;
        }
    }

    /* Get the SD card RCA */
    sdmmc1.card_rca = rca;

    /* Send CMD9 SEND_CSD with argument as card's RCA */
    errorstate = SDMMC_CmdSendCSD(FURI_SDMMC_BLOCK, (uint32_t)(sdmmc1.card_rca << 16U));
    if(errorstate != FuriHalSdErrorNone) {
        return errorstate;
    } else {
        /* Get Card Specific Data */
        CSD[0U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP1);
        CSD[1U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP2);
        CSD[2U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP3);
        CSD[3U] = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP4);
    }

    /* Parse parameters */
    sdmmc_parse_csd(&sdmmc1.csd, CSD);
    sdmmc_parse_info(&sdmmc1.info, &sdmmc1.csd, CID);

    /* Select the Card */
    errorstate =
        SDMMC_CmdSelDesel(FURI_SDMMC_BLOCK, (uint32_t)(((uint32_t)sdmmc1.card_rca) << 16U));
    if(errorstate != FuriHalSdErrorNone) {
        return errorstate;
    }

    /* All cards are initialized */
    return FuriHalSdErrorNone;
}

static FuriHalSdError sdmmc_send_status_command(uint32_t* pSDstatus) {
    SDMMC_DataInitTypeDef config = {0};
    FuriHalSdError errorstate;
    FuriHalCortexTimer timer = furi_hal_cortex_timer_get(FURI_SDMMC_SWDATATIMEOUT * 1000U);
    uint32_t count;
    uint32_t* data = pSDstatus;

    /* Check SD response */
    if((SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP1) & SDMMC_CARD_LOCKED) ==
       SDMMC_CARD_LOCKED) {
        return FuriHalSdErrorLockUnlockFailed;
    }

    /* Set block size for card if it is not equal to current block size for card */
    errorstate = SDMMC_CmdBlockLength(FURI_SDMMC_BLOCK, 64U);
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdBlockLength failed with error 0x%x", errorstate);
        return errorstate;
    }

    /* Send CMD55 */
    errorstate = SDMMC_CmdAppCommand(FURI_SDMMC_BLOCK, (uint32_t)(sdmmc1.card_rca << 16U));
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdAppCommand failed with error 0x%x", errorstate);
        return errorstate;
    }

    /* Configure the SD DPSM (Data Path State Machine) */
    config.DataTimeOut = SDMMC_DATATIMEOUT;
    config.DataLength = 64U;
    config.DataBlockSize = SDMMC_DATABLOCK_SIZE_64B;
    config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
    config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
    config.DPSM = SDMMC_DPSM_ENABLE;
    SDMMC_ConfigData(FURI_SDMMC_BLOCK, &config);

    /* Send ACMD13 (SD_APP_STAUS)  with argument as card's RCA */
    errorstate = SDMMC_CmdStatusRegister(FURI_SDMMC_BLOCK);
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdStatusRegister failed with error 0x%x", errorstate);
        return errorstate;
    }

    /* Get status data */
    while(!sdmmc_get_flags(
        SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DATAEND)) {
        if(sdmmc_get_flags(SDMMC_FLAG_RXFIFOHF)) {
            for(count = 0U; count < 8U; count++) {
                *data = SDMMC_ReadFIFO(FURI_SDMMC_BLOCK);
                data++;
            }
        }

        if(furi_hal_cortex_timer_is_expired(timer)) {
            return FuriHalSdErrorTimeout;
        }
    }

    if(sdmmc_get_flags(SDMMC_FLAG_DTIMEOUT)) {
        return FuriHalSdErrorDataTimeout;
    } else if(sdmmc_get_flags(SDMMC_FLAG_DCRCFAIL)) {
        return FuriHalSdErrorDataCrcFail;
    } else if(sdmmc_get_flags(SDMMC_FLAG_RXOVERR)) {
        return FuriHalSdErrorRxOverrun;
    } else {
        /* Nothing to do */
    }

    while((sdmmc_get_flags(SDMMC_FLAG_DPSMACT))) {
        *data = SDMMC_ReadFIFO(FURI_SDMMC_BLOCK);
        data++;

        if(furi_hal_cortex_timer_is_expired(timer)) {
            return FuriHalSdErrorTimeout;
        }
    }

    /* Clear all the static status flags*/
    sdmmc_clear_static_data_flags();

    return FuriHalSdErrorNone;
}

static bool sd_mmc_get_card_status(CardStatus* card_status) {
    uint32_t sd_status[16] = {0};
    FuriHalSdError errorstate;
    bool status = true;

    errorstate = sdmmc_send_status_command(sd_status);
    if(errorstate != FuriHalSdErrorNone) {
        /* Clear all the static flags */
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "sdmmc_send_status_command failed with error 0x%x", errorstate);
        status = false;
    } else {
        card_status->DataBusWidth = (uint8_t)((sd_status[0] & 0xC0U) >> 6U);
        card_status->SecuredMode = (uint8_t)((sd_status[0] & 0x20U) >> 5U);
        card_status->CardType = (uint16_t)(((sd_status[0] & 0x00FF0000U) >> 8U) |
                                           ((sd_status[0] & 0xFF000000U) >> 24U));
        card_status->ProtectedAreaSize =
            (((sd_status[1] & 0xFFU) << 24U) | ((sd_status[1] & 0xFF00U) << 8U) |
             ((sd_status[1] & 0xFF0000U) >> 8U) | ((sd_status[1] & 0xFF000000U) >> 24U));
        card_status->SpeedClass = (uint8_t)(sd_status[2] & 0xFFU);
        card_status->PerformanceMove = (uint8_t)((sd_status[2] & 0xFF00U) >> 8U);
        card_status->AllocationUnitSize = (uint8_t)((sd_status[2] & 0xF00000U) >> 20U);
        card_status->EraseSize =
            (uint16_t)(((sd_status[2] & 0xFF000000U) >> 16U) | (sd_status[3] & 0xFFU));
        card_status->EraseTimeout = (uint8_t)((sd_status[3] & 0xFC00U) >> 10U);
        card_status->EraseOffset = (uint8_t)((sd_status[3] & 0x0300U) >> 8U);
        card_status->UhsSpeedGrade = (uint8_t)((sd_status[3] & 0x00F0U) >> 4U);
        card_status->UhsAllocationUnitSize = (uint8_t)(sd_status[3] & 0x000FU);
        card_status->VideoSpeedClass = (uint8_t)((sd_status[4] & 0xFF000000U) >> 24U);
    }

    /* Set Block Size for Card */
    errorstate = SDMMC_CmdBlockLength(FURI_SDMMC_BLOCK, SD_BLOCKSIZE);
    if(errorstate != FuriHalSdErrorNone) {
        /* Clear all the static flags */
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "SDMMC_CmdBlockLength failed with error 0x%x", errorstate);
        status = false;
    }

    return status;
}

static FuriHalSdError sdmmc_find_scr(uint32_t* pSCR) {
    SDMMC_DataInitTypeDef config = {0};
    FuriHalSdError errorstate;
    FuriHalCortexTimer timer = furi_hal_cortex_timer_get(FURI_SDMMC_SWDATATIMEOUT * 1000U);
    uint32_t index = 0U;
    uint32_t tempscr[2U] = {0};
    uint32_t* scr = pSCR;

    /* Set Block Size To 8 Bytes */
    errorstate = SDMMC_CmdBlockLength(FURI_SDMMC_BLOCK, 8U);
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdBlockLength failed with error 0x%x", errorstate);
        return errorstate;
    }

    /* Send CMD55 APP_CMD with argument as card's RCA */
    errorstate = SDMMC_CmdAppCommand(FURI_SDMMC_BLOCK, (uint32_t)((sdmmc1.card_rca) << 16U));
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdAppCommand failed with error 0x%x", errorstate);
        return errorstate;
    }

    config.DataTimeOut = SDMMC_DATATIMEOUT;
    config.DataLength = 8U;
    config.DataBlockSize = SDMMC_DATABLOCK_SIZE_8B;
    config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
    config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
    config.DPSM = SDMMC_DPSM_ENABLE;
    SDMMC_ConfigData(FURI_SDMMC_BLOCK, &config);

    /* Send ACMD51 SD_APP_SEND_SCR with argument as 0 */
    errorstate = SDMMC_CmdSendSCR(FURI_SDMMC_BLOCK);
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdSendSCR failed with error 0x%x", errorstate);
        return errorstate;
    }

    while(!sdmmc_get_flags(
        SDMMC_FLAG_RXOVERR | SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_DBCKEND |
        SDMMC_FLAG_DATAEND)) {
        if((!sdmmc_get_flags(SDMMC_FLAG_RXFIFOE)) && (index == 0U)) {
            tempscr[0] = SDMMC_ReadFIFO(FURI_SDMMC_BLOCK);
            tempscr[1] = SDMMC_ReadFIFO(FURI_SDMMC_BLOCK);
            index++;
        }

        if(furi_hal_cortex_timer_is_expired(timer)) {
            FURI_LOG_E(TAG, "SDMMC_ReadFIFO failed with timeout");
            return FuriHalSdErrorTimeout;
        }
    }

    if(sdmmc_get_flags(SDMMC_FLAG_DTIMEOUT)) {
        sdmmc_clear_flags(SDMMC_FLAG_DTIMEOUT);
        FURI_LOG_E(TAG, "SDTIMEOUT");
        return FuriHalSdErrorDataTimeout;
    } else if(sdmmc_get_flags(SDMMC_FLAG_DCRCFAIL)) {
        sdmmc_clear_flags(SDMMC_FLAG_DCRCFAIL);
        FURI_LOG_E(TAG, "SDCRCFAIL");
        return FuriHalSdErrorDataCrcFail;
    } else if(sdmmc_get_flags(SDMMC_FLAG_RXOVERR)) {
        sdmmc_clear_flags(SDMMC_FLAG_RXOVERR);
        FURI_LOG_E(TAG, "SDRXOVERR");
        return FuriHalSdErrorRxOverrun;
    } else {
        /* No error flag set */
        /* Clear all the static flags */
        sdmmc_clear_static_data_flags();

        *scr =
            (((tempscr[1] & SDMMC_0TO7BITS) << 24U) | ((tempscr[1] & SDMMC_8TO15BITS) << 8U) |
             ((tempscr[1] & SDMMC_16TO23BITS) >> 8U) | ((tempscr[1] & SDMMC_24TO31BITS) >> 24U));
        scr++;
        *scr =
            (((tempscr[0] & SDMMC_0TO7BITS) << 24U) | ((tempscr[0] & SDMMC_8TO15BITS) << 8U) |
             ((tempscr[0] & SDMMC_16TO23BITS) >> 8U) | ((tempscr[0] & SDMMC_24TO31BITS) >> 24U));
    }

    return FuriHalSdErrorNone;
}

static FuriHalSdError sdmmc_wide_bus_enable(void) {
    uint32_t scr[2U] = {0};
    FuriHalSdError errorstate;

    if((SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP1) & SDMMC_CARD_LOCKED) ==
       SDMMC_CARD_LOCKED) {
        FURI_LOG_E(TAG, "SD Card unlock failed");
        return FuriHalSdErrorLockUnlockFailed;
    }

    /* Get SCR Register */
    errorstate = sdmmc_find_scr(scr);
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "sdmmc_find_scr failed with error 0x%x", errorstate);
        return errorstate;
    }

    /* If requested card supports wide bus operation */
    if((scr[1U] & SDMMC_WIDE_BUS_SUPPORT) != SDMMC_ALLZERO) {
        /* Send CMD55 APP_CMD with argument as card's RCA.*/
        errorstate = SDMMC_CmdAppCommand(FURI_SDMMC_BLOCK, (uint32_t)(sdmmc1.card_rca << 16U));
        if(errorstate != FuriHalSdErrorNone) {
            FURI_LOG_E(TAG, "SDMMC_CmdAppCommand failed with error 0x%x", errorstate);
            return errorstate;
        }

        /* Send ACMD6 APP_CMD with argument as 2 for wide bus mode */
        errorstate = SDMMC_CmdBusWidth(FURI_SDMMC_BLOCK, 2U);
        if(errorstate != FuriHalSdErrorNone) {
            FURI_LOG_E(TAG, "SDMMC_CmdBusWidth failed with error 0x%x", errorstate);
            return errorstate;
        }

        return FuriHalSdErrorNone;
    } else {
        return FuriHalSdErrorRequestNotApplicable;
    }
}

static bool sdmmc_config_wide_bus_operation(uint32_t sdmmc_clk) {
    furi_assert(sdmmc_clk != 0U);

    SDMMC_InitTypeDef init = {0};
    FuriHalSdError errorstate;

    bool status = true;

    errorstate = sdmmc_wide_bus_enable();
    if(errorstate != FuriHalSdErrorNone) {
        /* Clear all the static flags */
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "sdmmc_wide_bus_enable failed with error 0x%x", errorstate);
        status = false;
    }

    /* Configure the SDMMC peripheral */
    init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_ENABLE;
    init.BusWide = SDMMC_BUS_WIDE_4B;
    init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
    init.ClockDiv = 0U;

    /* Check if user Clock div < Normal speed 25Mhz, no change in Clockdiv */
    if(init.ClockDiv >= (sdmmc_clk / (2U * SD_NORMAL_SPEED_FREQ))) {
        init.ClockDiv = init.ClockDiv;
    } else if(sdmmc1.info.speed == FuriHalSdSpeedUltraHigh) {
        /* UltraHigh speed SD card, user Clock div */
        init.ClockDiv = init.ClockDiv;
    } else if(sdmmc1.info.speed == FuriHalSdSpeedHigh) {
        /* High speed SD card, Max Frequency = 50Mhz */
        if(init.ClockDiv == 0U) {
            if(sdmmc_clk > SD_HIGH_SPEED_FREQ) {
                init.ClockDiv = sdmmc_clk / (2U * SD_HIGH_SPEED_FREQ);
            } else {
                init.ClockDiv = init.ClockDiv;
            }
        } else {
            if((sdmmc_clk / (2U * init.ClockDiv)) > SD_HIGH_SPEED_FREQ) {
                init.ClockDiv = sdmmc_clk / (2U * SD_HIGH_SPEED_FREQ);
            } else {
                init.ClockDiv = init.ClockDiv;
            }
        }
    } else {
        /* No High speed SD card, Max Frequency = 25Mhz */
        if(init.ClockDiv == 0U) {
            if(sdmmc_clk > SD_NORMAL_SPEED_FREQ) {
                init.ClockDiv = sdmmc_clk / (2U * SD_NORMAL_SPEED_FREQ);
            } else {
                init.ClockDiv = init.ClockDiv;
            }
        } else {
            if((sdmmc_clk / (2U * init.ClockDiv)) > SD_NORMAL_SPEED_FREQ) {
                init.ClockDiv = sdmmc_clk / (2U * SD_NORMAL_SPEED_FREQ);
            } else {
                init.ClockDiv = init.ClockDiv;
            }
        }
    }

    SDMMC_Init(FURI_SDMMC_BLOCK, init);

    /* Set Block Size for Card */
    errorstate = SDMMC_CmdBlockLength(FURI_SDMMC_BLOCK, SD_BLOCKSIZE);
    if(errorstate != FuriHalSdErrorNone) {
        /* Clear all the static flags */
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "SDMMC_CmdBlockLength failed with error 0x%x", errorstate);
        status = false;
    }

    return status;
}

static FuriHalSdError sdmmc_send_status(uint32_t* pCardStatus) {
    FuriHalSdError errorstate;

    /* Send Status command */
    errorstate = SDMMC_CmdSendStatus(FURI_SDMMC_BLOCK, (uint32_t)(sdmmc1.card_rca << 16U));
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "SDMMC_CmdSendStatus failed with error 0x%x", errorstate);
        return errorstate;
    }

    /* Get SD card status */
    *pCardStatus = SDMMC_GetResponse(FURI_SDMMC_BLOCK, SDMMC_RESP1);

    return FuriHalSdErrorNone;
}

typedef enum {
    SdCardStateReady = 0x00000001U, /*!< Card state is ready */
    SdCardStateIdentification = 0x00000002U, /*!< Card is in identification state */
    SdCardStateStandby = 0x00000003U, /*!< Card is in standby state */
    SdCardStateTransfer = 0x00000004U, /*!< Card is in transfer state */
    SdCardStateSending = 0x00000005U, /*!< Card is sending an operation */
    SdCardStateReceiving = 0x00000006U, /*!< Card is receiving operation information */
    SdCardStateProgramming = 0x00000007U, /*!< Card is in programming state */
    SdCardStateDisconnected = 0x00000008U, /*!< Card is disconnected */
    SdCardStateError = 0x000000FFU, /*!< Card response Error */
} SdCardState;

SdCardState sdmmc_get_card_state(void) {
    SdCardState cardstate;
    FuriHalSdError errorstate;
    uint32_t resp1 = 0;

    errorstate = sdmmc_send_status(&resp1);
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "sdmmc_send_status failed with error 0x%x", errorstate);
    }

    cardstate = ((resp1 >> 9U) & 0x0FU);
    return cardstate;
}

static bool sdmmc_wait_for_transfer_state(size_t timeout_ms) {
    FuriHalCortexTimer timer = furi_hal_cortex_timer_get(timeout_ms * 1000U);
    SdCardState card_state = sdmmc_get_card_state();
    while(card_state != SdCardStateTransfer) {
        if(furi_hal_cortex_timer_is_expired(timer)) {
            FURI_LOG_E(TAG, "sdmmc_get_card_state failed");
            return false;
        }
        card_state = sdmmc_get_card_state();
    }

    return true;
}

static bool sdmmc_read_blocks_dma(uint8_t* data, uint32_t address, uint32_t block_count) {
    SDMMC_DataInitTypeDef config = {0};
    FuriHalSdError errorstate;

    /* Initialize data control register */
    FURI_SDMMC_BLOCK->DCTRL = 0U;

    /* Update the SD transfer context */
    sdmmc_dma_context.rx_buffer = data;
    sdmmc_dma_context.rx_size = SD_BLOCKSIZE * block_count;

    if(sdmmc1.info.type != FuriHalSdTypeHCXC) {
        address *= SD_BLOCKSIZE;
    }

    /* Configure the SD DPSM (Data Path State Machine) */
    config.DataTimeOut = SDMMC_DATATIMEOUT;
    config.DataLength = SD_BLOCKSIZE * block_count;
    config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
    config.TransferDir = SDMMC_TRANSFER_DIR_TO_SDMMC;
    config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
    config.DPSM = SDMMC_DPSM_DISABLE;
    SDMMC_ConfigData(FURI_SDMMC_BLOCK, &config);

    __SDMMC_CMDTRANS_ENABLE(FURI_SDMMC_BLOCK);
    FURI_SDMMC_BLOCK->IDMABASER = (uint32_t)data;
    FURI_SDMMC_BLOCK->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;

    /* Read Blocks in DMA mode */
    if(block_count > 1U) {
        sdmmc_dma_context.state = SdMmcDmaStateEnabled | SdMmcDmaStateRxMulti;

        /* Read Multi Block command */
        errorstate = SDMMC_CmdReadMultiBlock(FURI_SDMMC_BLOCK, address);
    } else {
        sdmmc_dma_context.state = SdMmcDmaStateEnabled | SdMmcDmaStateRxSingle;

        /* Read Single Block command */
        errorstate = SDMMC_CmdReadSingleBlock(FURI_SDMMC_BLOCK, address);
    }

    if(errorstate != FuriHalSdErrorNone) {
        /* Clear all the static flags */
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "SDMMC_CmdReadSingle/MultiBlock failed with error 0x%x", errorstate);
        return false;
    }

    /* Enable transfer interrupts */
    sdmmc_enable_it(SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_RXOVERR | SDMMC_IT_DATAEND);

    return true;
}

static bool sdmmc_write_blocks_dma(const uint8_t* data, uint32_t address, uint32_t block_count) {
    SDMMC_DataInitTypeDef config = {0};
    FuriHalSdError errorstate;

    /* Initialize data control register */
    FURI_SDMMC_BLOCK->DCTRL = 0U;

    sdmmc_dma_context.tx_buffer = data;
    sdmmc_dma_context.tx_size = SD_BLOCKSIZE * block_count;

    if(sdmmc1.info.type != FuriHalSdTypeHCXC) {
        address *= SD_BLOCKSIZE;
    }

    /* Configure the SD DPSM (Data Path State Machine) */
    config.DataTimeOut = SDMMC_DATATIMEOUT;
    config.DataLength = SD_BLOCKSIZE * block_count;
    config.DataBlockSize = SDMMC_DATABLOCK_SIZE_512B;
    config.TransferDir = SDMMC_TRANSFER_DIR_TO_CARD;
    config.TransferMode = SDMMC_TRANSFER_MODE_BLOCK;
    config.DPSM = SDMMC_DPSM_DISABLE;
    SDMMC_ConfigData(FURI_SDMMC_BLOCK, &config);

    __SDMMC_CMDTRANS_ENABLE(FURI_SDMMC_BLOCK);

    FURI_SDMMC_BLOCK->IDMABASER = (uint32_t)data;
    FURI_SDMMC_BLOCK->IDMACTRL = SDMMC_ENABLE_IDMA_SINGLE_BUFF;

    /* Write Blocks in Polling mode */
    if(block_count > 1U) {
        sdmmc_dma_context.state = SdMmcDmaStateEnabled | SdMmcDmaStateRxMulti;

        /* Write Multi Block command */
        errorstate = SDMMC_CmdWriteMultiBlock(FURI_SDMMC_BLOCK, address);
    } else {
        sdmmc_dma_context.state = SdMmcDmaStateEnabled | SdMmcDmaStateRxSingle;

        /* Write Single Block command */
        errorstate = SDMMC_CmdWriteSingleBlock(FURI_SDMMC_BLOCK, address);
    }
    if(errorstate != FuriHalSdErrorNone) {
        /* Clear all the static flags */
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "sdmmc_write_blocks_dma failed with error 0x%x", errorstate);
        return false;
    }

    /* Enable transfer interrupts */
    sdmmc_enable_it(SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR | SDMMC_IT_DATAEND);

    return true;
}

static void sdmmc_irq_handler(void* ctx) {
    UNUSED(ctx);

    uint32_t errorstate;

    if(sdmmc_get_flags(SDMMC_FLAG_DATAEND)) {
        sdmmc_clear_flags(SDMMC_FLAG_DATAEND);

        sdmmc_disable_it(
            SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
            SDMMC_IT_RXOVERR);

        __SDMMC_CMDTRANS_DISABLE(FURI_SDMMC_BLOCK);

        if((sdmmc_dma_context.state & SdMmcDmaStateEnabled) != 0U) {
            FURI_SDMMC_BLOCK->DLEN = 0;
            FURI_SDMMC_BLOCK->DCTRL = 0;
            FURI_SDMMC_BLOCK->IDMACTRL = SDMMC_DISABLE_IDMA;

            /* Stop Transfer for Write Multi blocks or Read Multi blocks */
            if(((sdmmc_dma_context.state & SdMmcDmaStateRxMulti) != 0U) ||
               ((sdmmc_dma_context.state & SdMmcDmaStateTxMulti) != 0U)) {
                errorstate = SDMMC_CmdStopTransfer(FURI_SDMMC_BLOCK);
                if(errorstate != FuriHalSdErrorNone) {
                    sdmmc_dma_context.error |= errorstate;

                    furi_event_flag_set(sdmmc_dma_context.event, SdMmcDmaEventError);
                }
            }

            furi_event_flag_set(sdmmc_dma_context.event, SdMmcDmaEventComplete);
        }
    } else if(sdmmc_get_flags(
                  SDMMC_FLAG_DCRCFAIL | SDMMC_FLAG_DTIMEOUT | SDMMC_FLAG_RXOVERR |
                  SDMMC_FLAG_TXUNDERR)) {
        /* Set Error code */
        if(sdmmc_get_flags(SDMMC_IT_DCRCFAIL)) {
            sdmmc_dma_context.error |= FuriHalSdErrorDataCrcFail;
        }
        if(sdmmc_get_flags(SDMMC_IT_DTIMEOUT)) {
            sdmmc_dma_context.error |= FuriHalSdErrorDataTimeout;
        }
        if(sdmmc_get_flags(SDMMC_IT_RXOVERR)) {
            sdmmc_dma_context.error |= FuriHalSdErrorRxOverrun;
        }
        if(sdmmc_get_flags(SDMMC_IT_TXUNDERR)) {
            sdmmc_dma_context.error |= FuriHalSdErrorTxUnderrun;
        }

        /* Clear All flags */
        sdmmc_clear_static_data_flags();

        /* Disable all interrupts */
        sdmmc_disable_it(
            SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
            SDMMC_IT_RXOVERR);

        __SDMMC_CMDTRANS_DISABLE(FURI_SDMMC_BLOCK);
        FURI_SDMMC_BLOCK->DCTRL |= SDMMC_DCTRL_FIFORST;
        FURI_SDMMC_BLOCK->CMD |= SDMMC_CMD_CMDSTOP;
        sdmmc_dma_context.error |= SDMMC_CmdStopTransfer(FURI_SDMMC_BLOCK);
        FURI_SDMMC_BLOCK->CMD &= ~(SDMMC_CMD_CMDSTOP);
        sdmmc_clear_flags(SDMMC_FLAG_DABORT);

        if((sdmmc_dma_context.state & SdMmcDmaStateEnabled) != 0U) {
            if(sdmmc_dma_context.error != FuriHalSdErrorNone) {
                /* Disable Internal DMA */
                FURI_SDMMC_BLOCK->IDMACTRL = SDMMC_DISABLE_IDMA;

                furi_event_flag_set(sdmmc_dma_context.event, SdMmcDmaEventError);
            }
        }
    }
}

static bool sdmmc_init_card_lowspeed(uint32_t sdmmc_clk) {
    furi_assert(sdmmc_clk != 0U);

    // init params
    SDMMC_InitTypeDef init = {0};
    init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_ENABLE;
    init.BusWide = SDMMC_BUS_WIDE_1B;
    init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;

    // set init clock (400 kHz)
    init.ClockDiv = sdmmc_clk / (2U * SD_INIT_FREQ);

    // init sdmmc block
    SDMMC_Init(FURI_SDMMC_BLOCK, init);

    // power on sdmmc block
    SDMMC_PowerState_ON(FURI_SDMMC_BLOCK);

    // wait 74 sd clock cycles
    furi_delay_us((1000000 * 74) / (sdmmc_clk / (2U * init.ClockDiv)));

    // identify card operating voltage
    FuriHalSdError errorstate = sdmmc_power_on();
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "sdmmc_power_on failed with error 0x%x", errorstate);
        return false;
    }

    // initialize card
    errorstate = sdmmc_init_card();
    if(errorstate != FuriHalSdErrorNone) {
        FURI_LOG_E(TAG, "sdmmc_init_card failed with error 0x%x", errorstate);
        return false;
    }

    // set block size for card
    errorstate = SDMMC_CmdBlockLength(FURI_SDMMC_BLOCK, SD_BLOCKSIZE);
    if(errorstate != FuriHalSdErrorNone) {
        sdmmc_clear_static_flags();
        FURI_LOG_E(TAG, "SDMMC_CmdBlockLength failed with error 0x%x", errorstate);
        return false;
    }

    return true;
}

static bool sdmmc_init_sdcard(uint32_t sdmmc_clk) {
    if(!sdmmc_init_card_lowspeed(sdmmc_clk)) {
        FURI_LOG_E(TAG, "sdmmc_init_card failed");
        return false;
    }

    if(sd_mmc_get_card_status(&sdmmc1.status) != true) {
        FURI_LOG_E(TAG, "sd_mmc_get_card_status failed");
        return false;
    }

    /* Get Initial Card Speed from Card Status */
    uint32_t speedgrade = sdmmc1.status.UhsSpeedGrade;
    uint32_t unitsize = sdmmc1.status.UhsAllocationUnitSize;
    if(sdmmc1.info.type == FuriHalSdTypeHCXC && ((speedgrade != 0U) || (unitsize != 0U))) {
        sdmmc1.info.speed = FuriHalSdSpeedUltraHigh;
    } else if(sdmmc1.info.type == FuriHalSdTypeHCXC) {
        sdmmc1.info.speed = FuriHalSdSpeedHigh;
    } else {
        sdmmc1.info.speed = FuriHalSdSpeedNormal;
    }

    /* Configure the bus wide */
    if(sdmmc_config_wide_bus_operation(sdmmc_clk) != true) {
        FURI_LOG_E(TAG, "sdmmc_config_wide_bus_operation failed");
        return false;
    }

    /* Verify that SD card is ready to use after Initialization */
    if(!sdmmc_wait_for_transfer_state(FURI_SDMMC_SWDATATIMEOUT)) {
        FURI_LOG_E(TAG, "sdmmc_wait_for_transfer_state failed");
        return false;
    }

    return true;
}

bool furi_hal_sdmmc_init_card(void) {
    sdmmc1.card_alive = false;

    // init sdmmc periph
    furi_hal_sdmmc_periph_init();

    // enable sdcard power
    furi_hal_sdmmc_card_enable_power();

    uint32_t sdmmc_clk = furi_hal_clock_get_freq(FuriHalClockHwSdMmc12);
    if(sdmmc_init_sdcard(sdmmc_clk)) {
        sdmmc1.card_alive = true;
        return true;
    } else {
        FURI_LOG_E(TAG, "SD card init failed");
        return false;
    }
}

void furi_hal_sdmmc_deinit_card(void) {
    furi_hal_sdmmc_card_disable_power();

    furi_hal_sdmmc_periph_deinit();

    FURI_LOG_I(TAG, "Card deinit OK");
    sdmmc1.card_alive = false;
}

bool furi_hal_sdmmc_read_blocks(
    uint8_t* buffer,
    uint32_t address,
    uint32_t count,
    size_t timeout_ms) {
    if((address + count) > (sdmmc1.info.logical_block_count)) {
        FURI_LOG_E(TAG, "Address out of range");
        return false;
    }

    sdmmc_disable_it(
        SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
        SDMMC_IT_RXOVERR);
    sdmmc_clear_static_flags();

    furi_event_flag_clear(sdmmc_dma_context.event, SdMmcDmaEventComplete | SdMmcDmaEventError);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdSdMmc1, sdmmc_irq_handler, &sdmmc_dma_context);

    bool status = sdmmc_read_blocks_dma(buffer, address, count);

    uint32_t flags = furi_event_flag_wait(
        sdmmc_dma_context.event,
        SdMmcDmaEventComplete | SdMmcDmaEventError,
        FuriFlagWaitAny,
        timeout_ms);

    furi_hal_interrupt_set_isr(FuriHalInterruptIdSdMmc1, NULL, NULL);

    if((flags == FuriFlagErrorTimeout) || (flags & SdMmcDmaEventError)) {
        if(flags == FuriFlagErrorTimeout) {
            FURI_LOG_E(TAG, "sdmmc_read_blocks_dma failed with timeout");
        } else {
            FURI_LOG_E(
                TAG, "sdmmc_read_blocks_dma failed with error 0x%x", sdmmc_dma_context.error);
        }
        sdmmc_dma_context.error = FuriHalSdErrorNone;
        status = false;
        sdmmc_disable_it(
            SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
            SDMMC_IT_RXOVERR);
    }

    if(status) {
        status = sdmmc_wait_for_transfer_state(timeout_ms);
        if(!status) {
            FURI_LOG_E(TAG, "sdmmc_wait_for_transfer_state failed");
        }
    }

    if(!status) {
        sdmmc1.card_alive = false;
    }

    return status;
}

bool furi_hal_sdmmc_write_blocks(
    const uint8_t* buffer,
    uint32_t address,
    uint32_t count,
    size_t timeout_ms) {
    if((address + count) > (sdmmc1.info.logical_block_count)) {
        FURI_LOG_E(TAG, "Address out of range");
        return false;
    }

    sdmmc_disable_it(
        SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
        SDMMC_IT_RXOVERR);
    sdmmc_clear_static_flags();

    furi_event_flag_clear(sdmmc_dma_context.event, SdMmcDmaEventComplete | SdMmcDmaEventError);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdSdMmc1, sdmmc_irq_handler, &sdmmc_dma_context);

    bool status = sdmmc_write_blocks_dma(buffer, address, count);

    uint32_t flags = furi_event_flag_wait(
        sdmmc_dma_context.event,
        SdMmcDmaEventComplete | SdMmcDmaEventError,
        FuriFlagWaitAny,
        timeout_ms);

    furi_hal_interrupt_set_isr(FuriHalInterruptIdSdMmc1, NULL, NULL);

    if((flags == FuriFlagErrorTimeout) || (flags & SdMmcDmaEventError)) {
        if(flags == FuriFlagErrorTimeout) {
            FURI_LOG_E(TAG, "sdmmc_write_blocks_dma failed with timeout");
        } else {
            FURI_LOG_E(
                TAG, "sdmmc_write_blocks_dma failed with error 0x%x", sdmmc_dma_context.error);
        }

        sdmmc_dma_context.error = FuriHalSdErrorNone;
        status = false;
        sdmmc_disable_it(
            SDMMC_IT_DATAEND | SDMMC_IT_DCRCFAIL | SDMMC_IT_DTIMEOUT | SDMMC_IT_TXUNDERR |
            SDMMC_IT_RXOVERR);
    }

    if(status) {
        status = sdmmc_wait_for_transfer_state(timeout_ms);
        if(!status) {
            FURI_LOG_E(TAG, "sdmmc_wait_for_transfer_state failed");
        }
    }

    if(!status) {
        sdmmc1.card_alive = false;
    }

    return status;
}

bool furi_hal_sdmmc_get_card_info(FuriHalSdInfo* info) {
    furi_check(info);
    memcpy(info, &sdmmc1.info, sizeof(FuriHalSdInfo));
    return true;
}

bool furi_hal_sd_alive(void) {
    return sdmmc1.card_alive;
}