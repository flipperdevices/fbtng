#include "furi_hal_ospi.h"
#include "furi_hal_cortex.h"
#include "core/check.h"
#include "core/log.h"
#include "furi_hal_ospi_assert.h"

#include "stm32u5xx_ll_rcc.h"
#include "stm32u5xx_ll_system.h"

#include "furi_hal_bus.h"

#define TAG "OSPI"

#define FURI_HAL_OSPI_FIFOTHRESHOLD 1
#define FURI_HAL_OSPI_DUALQUAD FURI_HAL_OSPI_DUALQUAD_DISABLE
#define FURI_HAL_OSPI_MEMORYTYPE FURI_HAL_OSPI_MEMTYPE_APMEMORY
#define FURI_HAL_OSPI_DEVICESIZE 24 //128Mbit
#define FURI_HAL_OSPI_CHIPSELECTHIGHTIME 2
#define FURI_HAL_OSPI_FREERUNNINGCLOCK FURI_HAL_OSPI_FREERUNCLK_DISABLE
#define FURI_HAL_OSPI_CLOCKMODE FURI_HAL_OSPI_CLOCK_MODE_0
#define FURI_HAL_OSPI_WRAPSIZE FURI_HAL_OSPI_WRAP_NOT_SUPPORTED
#define FURI_HAL_OSPI_CLOCKPRESCALER 2 //FreqClock 160Mhz/2 = 80Mhz
#define FURI_HAL_OSPI_SAMPLESHIFTING FURI_HAL_OSPI_SAMPLE_SHIFTING_NONE
#define FURI_HAL_OSPI_DELAYHOLDQUARTERCYCLE FURI_HAL_OSPI_DHQC_ENABLE
#define FURI_HAL_OSPI_CHIPSELECTBOUNDARY 10 //boundary 1k
#define FURI_HAL_OSPI_DELAYBLOCKBYPASS FURI_HAL_OSPI_DELAY_BLOCK_USED
#define FURI_HAL_OSPI_MAXTRAN 0
#define FURI_HAL_OSPI_REFRESH 320 // 4us 320clock freq 80Mhz

#define OSPI_IOM_PORT_MASK 0x1U

#define FURI_HAL_OSPI OCTOSPI1

typedef enum {
    FuriHalOspiStateReset = ((uint32_t)0x00000000U), /*!< Initial state */
    FuriHalOspiStateInit =
        ((uint32_t)0x00000001U), /*!< Initialization done in but timing configuration not done */
    FuriHalOspiStateReady = ((uint32_t)0x00000002U), /*!< Driver ready to be used */
    FuriHalOspiStateCmdCfg = ((
        uint32_t)0x00000004U), /*!< Command (regular or hyperbus) configured, ready for an action */
    FuriHalOspiStateReadCmdCfg = ((
        uint32_t)0x00000014U), /*!< Read command configuration done, not the write command configuration */
    FuriHalOspiStateWriteCmdCfg = ((
        uint32_t)0x00000024U), /*!< Write command configuration done, not the read command configuration */
    FuriHalOspiStateBusyCmd = ((uint32_t)0x00000008U), /*!< Command without data on-going */
    FuriHalOspiStateBusyTx = ((uint32_t)0x00000018U), /*!< Indirect Tx on-going */
    FuriHalOspiStateBusyRx = ((uint32_t)0x00000028U), /*!< Indirect Rx on-going */
    FuriHalOspiStateBusyAutoPolling = ((uint32_t)0x00000048U), /*!< Auto-polling on-going */
    FuriHalOspiStateBusyMemMapped = ((uint32_t)0x00000088U), /*!< Memory-mapped on-going */
    FuriHalOspiStateAbort = ((uint32_t)0x00000100U), /*!< Abort on-going */
    FuriHalOspiStateError =
        ((uint32_t)0x00000200U) /*!< Blocking error, driver should be re-initialized */
} FuriHalOspiState;

FuriHalOspiState furi_hal_ospi_state = FuriHalOspiStateReset;

#define __FURI_HAL_OSPI_GET_FLAG(__FLAG__) \
    ((READ_BIT(FURI_HAL_OSPI->SR, (__FLAG__)) != 0U) ? SET : RESET)

static bool
    furi_hal_osp_wait_flag_until_timeout(uint32_t flag, FlagStatus state, uint32_t timeout) {
    FuriHalCortexTimer timer = furi_hal_cortex_timer_get(timeout * 1000);
    while(__FURI_HAL_OSPI_GET_FLAG(flag) != state) {
        if(furi_hal_cortex_timer_is_expired(timer)) {
            if(__FURI_HAL_OSPI_GET_FLAG(flag) != state) {
                return false;
                FURI_LOG_W(TAG, "Timeout waiting for flag 0x%08lX", flag);
            } else {
                return true;
            }
        }
    }
    return true;
}

void furi_hal_ospi_init(void) {
    //Todo check system clock frequency
    //Designed for SYSclock frequency 160Mhz
    LL_RCC_SetOCTOSPIClockSource(LL_RCC_OCTOSPI_CLKSOURCE_SYSCLK);
    furi_hal_bus_enable(FuriHalBusOCTOSPIM);
    furi_hal_bus_enable(FuriHalBusOCTOSPI1);

    /* Configure memory type, device size, chip select high time, delay block bypass,
         free running clock, clock mode */
    MODIFY_REG(
        FURI_HAL_OSPI->DCR1,
        (OCTOSPI_DCR1_MTYP | OCTOSPI_DCR1_DEVSIZE | OCTOSPI_DCR1_CSHT | OCTOSPI_DCR1_DLYBYP |
         OCTOSPI_DCR1_FRCK | OCTOSPI_DCR1_CKMODE),
        (FURI_HAL_OSPI_MEMORYTYPE | ((FURI_HAL_OSPI_DEVICESIZE - 1U) << OCTOSPI_DCR1_DEVSIZE_Pos) |
         ((FURI_HAL_OSPI_CHIPSELECTHIGHTIME - 1U) << OCTOSPI_DCR1_CSHT_Pos) |
         FURI_HAL_OSPI_DELAYBLOCKBYPASS | FURI_HAL_OSPI_CLOCKMODE));

    /* Configure wrap size */
    MODIFY_REG(FURI_HAL_OSPI->DCR2, OCTOSPI_DCR2_WRAPSIZE, FURI_HAL_OSPI_WRAPSIZE);

    /* Configure chip select boundary and maximum transfer */
    FURI_HAL_OSPI->DCR3 =
        ((FURI_HAL_OSPI_CHIPSELECTBOUNDARY << OCTOSPI_DCR3_CSBOUND_Pos) |
         (FURI_HAL_OSPI_MAXTRAN << OCTOSPI_DCR3_MAXTRAN_Pos));

    /* Configure refresh */
    FURI_HAL_OSPI->DCR4 = FURI_HAL_OSPI_REFRESH;

    /* Configure FIFO threshold */
    MODIFY_REG(
        FURI_HAL_OSPI->CR,
        OCTOSPI_CR_FTHRES,
        ((FURI_HAL_OSPI_FIFOTHRESHOLD - 1U) << OCTOSPI_CR_FTHRES_Pos));

    /* Wait till busy flag is reset */
    if(furi_hal_osp_wait_flag_until_timeout(
           OCTOSPI_SR_BUSY, RESET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
        /* Configure clock prescaler */
        MODIFY_REG(
            FURI_HAL_OSPI->DCR2,
            OCTOSPI_DCR2_PRESCALER,
            ((FURI_HAL_OSPI_CLOCKPRESCALER - 1U) << OCTOSPI_DCR2_PRESCALER_Pos));

        /* Configure Dual Quad mode */
        MODIFY_REG(FURI_HAL_OSPI->CR, OCTOSPI_CR_DMM, FURI_HAL_OSPI_DUALQUAD);

        /* Configure sample shifting and delay hold quarter cycle */
        MODIFY_REG(
            FURI_HAL_OSPI->TCR,
            (OCTOSPI_TCR_SSHIFT | OCTOSPI_TCR_DHQC),
            (FURI_HAL_OSPI_SAMPLESHIFTING | FURI_HAL_OSPI_DELAYHOLDQUARTERCYCLE));

        /* Enable OctoSPI */
        SET_BIT(FURI_HAL_OSPI->CR, OCTOSPI_CR_EN);

        /* Enable free running clock if needed : must be done after OSPI enable */
        if(FURI_HAL_OSPI_FREERUNNINGCLOCK == FURI_HAL_OSPI_FREERUNCLK_ENABLE) {
            SET_BIT(FURI_HAL_OSPI->DCR1, OCTOSPI_DCR1_FRCK);
        }
    } else {
        furi_crash("OSPI: Init timeout");
    }
    furi_hal_ospi_state = FuriHalOspiStateInit;
}

void furi_hal_ospi_deint(void) {
    /* Disable OctoSPI */
    furi_hal_bus_disable(FuriHalBusOCTOSPIM);
    furi_hal_bus_disable(FuriHalBusOCTOSPI1);
}

bool furi_hal_ospi_abort(void) {
    bool status = false;
    /* Check if the DMA is enabled */
    if((FURI_HAL_OSPI->CR & OCTOSPI_CR_DMAEN) != 0U) {
        /* Disable the DMA transfer on the OctoSPI side */
        CLEAR_BIT(FURI_HAL_OSPI->CR, OCTOSPI_CR_DMAEN);

        //Todo NO DMA check
        //   /* Disable the DMA transfer on the DMA side */
        //   status = HAL_DMA_Abort(hospi->hdma);
        //   if (status != HAL_OK)
        //   {
        //     hospi->ErrorCode = HAL_OSPI_ERROR_DMA;
        //   }
    }

    if(READ_BIT(FURI_HAL_OSPI->SR, OCTOSPI_SR_BUSY) != RESET) {
        /* Perform an abort of the OctoSPI */
        SET_BIT(FURI_HAL_OSPI->CR, OCTOSPI_CR_ABORT);

        /* Wait until the transfer complete flag is set to go back in idle state */
        if(furi_hal_osp_wait_flag_until_timeout(
               OCTOSPI_SR_TCF, SET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
            /* Clear transfer complete flag */
            WRITE_REG(FURI_HAL_OSPI->FCR, OCTOSPI_SR_TCF);

            /* Wait until the busy flag is reset to go back in idle state */
            if(furi_hal_osp_wait_flag_until_timeout(
                   OCTOSPI_SR_BUSY, RESET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
                /* Update state */
                furi_hal_ospi_state = FuriHalOspiStateAbort;
                status = true;
            }
        }

    } else {
        /* Update state */
        furi_hal_ospi_state = FuriHalOspiStateAbort;
        status = true;
    }
    /* Return function status */
    return status;
}

void furi_hal_ospi_dlyb_set_config(LL_DLYB_CfgTypeDef* pdlyb_cfg) {
    furi_assert(pdlyb_cfg != NULL);
    /* Enable OCTOSPI Free Running Clock (mandatory) */
    SET_BIT(FURI_HAL_OSPI->DCR1, OCTOSPI_DCR1_FRCK);

    /* Enable the DelayBlock */
    LL_DLYB_Enable(DLYB_OCTOSPI1);

    /* Set the Delay Block configuration */
    LL_DLYB_SetDelay(DLYB_OCTOSPI1, pdlyb_cfg);

    /* Abort the current OCTOSPI operation if exist */
    furi_hal_ospi_abort();

    /* Disable Free Running Clock */
    CLEAR_BIT(FURI_HAL_OSPI->DCR1, OCTOSPI_DCR1_FRCK);
}

bool furi_hal_ospi_dlyb_get_clock_period(LL_DLYB_CfgTypeDef* pdlyb_cfg) {
    furi_assert(pdlyb_cfg != NULL);
    bool status = false;

    /* Enable OCTOSPI Free Running Clock (mandatory) */
    SET_BIT(FURI_HAL_OSPI->DCR1, OCTOSPI_DCR1_FRCK);
    /* Enable the DelayBlock */
    LL_DLYB_Enable(DLYB_OCTOSPI1);

    /* try to detect Period */
    if(LL_DLYB_GetClockPeriod(DLYB_OCTOSPI1, pdlyb_cfg) == (uint32_t)SUCCESS) {
        status = true;
    }

    /* Disable the DelayBlock */
    LL_DLYB_Disable(DLYB_OCTOSPI1);

    /* Abort the current OctoSPI operation if exist */
    furi_hal_ospi_abort();

    /* Disable Free Running Clock */
    CLEAR_BIT(FURI_HAL_OSPI->DCR1, OCTOSPI_DCR1_FRCK);

    /* Return function status */
    return status;
}

static void furi_hal_ospi_get_config_ospi1(FuriHalOspiConfig* cfg) {
    uint32_t value = 0U;

    /* Initialize the structure */
    cfg->clk_port = 0U;
    cfg->dqs_port = 0U;
    cfg->ncs_port = 0U;
    cfg->io_low_port = 0U;
    cfg->io_high_port = 0U;
    cfg->req2_ask_time = 0U;

    uint32_t reg = OCTOSPIM->PCR[0];

    if((reg & OCTOSPIM_PCR_CLKEN) != 0U) {
        /* The clock is enabled on this port */
        if((reg & OCTOSPIM_PCR_CLKSRC) == (value & OCTOSPIM_PCR_CLKSRC)) {
            /* The clock correspond to the instance passed as parameter */
            cfg->clk_port = 1U;
        }
    }

    if((reg & OCTOSPIM_PCR_DQSEN) != 0U) {
        /* The DQS is enabled on this port */
        if((reg & OCTOSPIM_PCR_DQSSRC) == (value & OCTOSPIM_PCR_DQSSRC)) {
            /* The DQS correspond to the instance passed as parameter */
            cfg->dqs_port = 1U;
        }
    }

    if((reg & OCTOSPIM_PCR_NCSEN) != 0U) {
        /* The nCS is enabled on this port */
        if((reg & OCTOSPIM_PCR_NCSSRC) == (value & OCTOSPIM_PCR_NCSSRC)) {
            /* The nCS correspond to the instance passed as parameter */
            cfg->ncs_port = 1U;
        }
    }

    if((reg & OCTOSPIM_PCR_IOLEN) != 0U) {
        /* The IO Low is enabled on this port */
        if((reg & OCTOSPIM_PCR_IOLSRC_1) == (value & OCTOSPIM_PCR_IOLSRC_1)) {
            /* The IO Low correspond to the instance passed as parameter */
            if((reg & OCTOSPIM_PCR_IOLSRC_0) == 0U) {
                cfg->io_low_port = (OCTOSPIM_PCR_IOLEN | (1U));
            } else {
                cfg->io_low_port = (OCTOSPIM_PCR_IOHEN | (1U));
            }
        }
    }

    if((reg & OCTOSPIM_PCR_IOHEN) != 0U) {
        /* The IO High is enabled on this port */
        if((reg & OCTOSPIM_PCR_IOHSRC_1) == (value & OCTOSPIM_PCR_IOHSRC_1)) {
            /* The IO High correspond to the instance passed as parameter */
            if((reg & OCTOSPIM_PCR_IOHSRC_0) == 0U) {
                cfg->io_high_port = (OCTOSPIM_PCR_IOLEN | (1U));
            } else {
                cfg->io_high_port = (OCTOSPIM_PCR_IOHEN | (1U));
            }
        }
    }
}

void furi_hal_ospi_config_no_mux_ospi1(FuriHalOspiConfig* cfg) {
    furi_assert(cfg != NULL);
    uint32_t instance;
    uint8_t ospi_enabled = 0U;
    FuriHalOspiConfig iom_cfg;

    instance = 0U;

    furi_hal_ospi_get_config_ospi1(&(iom_cfg));

    /********** Disable both OctoSPI to configure OctoSPI IO Manager **********/
    if((OCTOSPI1->CR & OCTOSPI_CR_EN) != 0U) {
        CLEAR_BIT(OCTOSPI1->CR, OCTOSPI_CR_EN);
        ospi_enabled |= 0x1U;
    }

    /***************** Deactivation of previous configuration *****************/
    CLEAR_BIT(OCTOSPIM->PCR[(iom_cfg.ncs_port - 1U)], OCTOSPIM_PCR_NCSEN);

    if(iom_cfg.clk_port != 0U) {
        CLEAR_BIT(OCTOSPIM->PCR[(iom_cfg.clk_port - 1U)], OCTOSPIM_PCR_CLKEN);
        if(iom_cfg.dqs_port != 0U) {
            CLEAR_BIT(OCTOSPIM->PCR[(iom_cfg.dqs_port - 1U)], OCTOSPIM_PCR_DQSEN);
        }
        if(iom_cfg.io_low_port != FURI_HAL_OSPIM_IOPORT_NONE) {
            CLEAR_BIT(
                OCTOSPIM->PCR[((iom_cfg.io_low_port - 1U) & OSPI_IOM_PORT_MASK)],
                OCTOSPIM_PCR_IOLEN);
        }
        if(iom_cfg.io_high_port != FURI_HAL_OSPIM_IOPORT_NONE) {
            CLEAR_BIT(
                OCTOSPIM->PCR[((iom_cfg.io_high_port - 1U) & OSPI_IOM_PORT_MASK)],
                OCTOSPIM_PCR_IOHEN);
        }
    }

    /******************** Activation of new configuration *********************/
    MODIFY_REG(
        OCTOSPIM->PCR[(cfg->ncs_port - 1U)],
        (OCTOSPIM_PCR_NCSEN | OCTOSPIM_PCR_NCSSRC),
        (OCTOSPIM_PCR_NCSEN | (instance << OCTOSPIM_PCR_NCSSRC_Pos)));

    if(((cfg->req2_ask_time) >= 1U) && ((cfg->req2_ask_time) <= 256U)) {
        if((cfg->req2_ask_time - 1U) >
           ((OCTOSPIM->CR & OCTOSPIM_CR_REQ2ACK_TIME) >> OCTOSPIM_CR_REQ2ACK_TIME_Pos)) {
            MODIFY_REG(
                OCTOSPIM->CR,
                OCTOSPIM_CR_REQ2ACK_TIME,
                ((cfg->req2_ask_time - 1U) << OCTOSPIM_CR_REQ2ACK_TIME_Pos));
        } else {
            /* Nothing to do */
        }
    }

    MODIFY_REG(
        OCTOSPIM->PCR[(cfg->clk_port - 1U)],
        (OCTOSPIM_PCR_CLKEN | OCTOSPIM_PCR_CLKSRC),
        (OCTOSPIM_PCR_CLKEN | (instance << OCTOSPIM_PCR_CLKSRC_Pos)));
    if(cfg->dqs_port != 0U) {
        MODIFY_REG(
            OCTOSPIM->PCR[(cfg->dqs_port - 1U)],
            (OCTOSPIM_PCR_DQSEN | OCTOSPIM_PCR_DQSSRC),
            (OCTOSPIM_PCR_DQSEN | (instance << OCTOSPIM_PCR_DQSSRC_Pos)));
    }

    if((cfg->io_low_port & OCTOSPIM_PCR_IOLEN) != 0U) {
        MODIFY_REG(
            OCTOSPIM->PCR[((cfg->io_low_port - 1U) & OSPI_IOM_PORT_MASK)],
            (OCTOSPIM_PCR_IOLEN | OCTOSPIM_PCR_IOLSRC),
            (OCTOSPIM_PCR_IOLEN | (instance << (OCTOSPIM_PCR_IOLSRC_Pos + 1U))));
    } else if(cfg->io_low_port != FURI_HAL_OSPIM_IOPORT_NONE) {
        MODIFY_REG(
            OCTOSPIM->PCR[((cfg->io_low_port - 1U) & OSPI_IOM_PORT_MASK)],
            (OCTOSPIM_PCR_IOHEN | OCTOSPIM_PCR_IOHSRC),
            (OCTOSPIM_PCR_IOHEN | (instance << (OCTOSPIM_PCR_IOHSRC_Pos + 1U))));
    } else {
        /* Nothing to do */
    }

    if((cfg->io_high_port & OCTOSPIM_PCR_IOLEN) != 0U) {
        MODIFY_REG(
            OCTOSPIM->PCR[((cfg->io_high_port - 1U) & OSPI_IOM_PORT_MASK)],
            (OCTOSPIM_PCR_IOLEN | OCTOSPIM_PCR_IOLSRC),
            (OCTOSPIM_PCR_IOLEN | OCTOSPIM_PCR_IOLSRC_0 |
             (instance << (OCTOSPIM_PCR_IOLSRC_Pos + 1U))));
    } else if(cfg->io_high_port != FURI_HAL_OSPIM_IOPORT_NONE) {
        MODIFY_REG(
            OCTOSPIM->PCR[((cfg->io_high_port - 1U) & OSPI_IOM_PORT_MASK)],
            (OCTOSPIM_PCR_IOHEN | OCTOSPIM_PCR_IOHSRC),
            (OCTOSPIM_PCR_IOHEN | OCTOSPIM_PCR_IOHSRC_0 |
             (instance << (OCTOSPIM_PCR_IOHSRC_Pos + 1U))));
    } else {
        /* Nothing to do */
    }

    /******* Re-enable both OctoSPI after configure OctoSPI IO Manager ********/
    if((ospi_enabled & 0x1U) != 0U) {
        SET_BIT(OCTOSPI1->CR, OCTOSPI_CR_EN);
    }
}

static bool furi_hal_ospi_config_cmd(FuriHalOspiCommand* cmd) {
    bool status = true;
    __IO uint32_t* ccr_reg;
    __IO uint32_t* tcr_reg;
    __IO uint32_t* ir_reg;
    __IO uint32_t* abr_reg;

    /* Re-initialize the value of the functional mode */
    MODIFY_REG(FURI_HAL_OSPI->CR, OCTOSPI_CR_FMODE, 0U);

    /* Configure the flash ID */
    if(FURI_HAL_OSPI_DUALQUAD == FURI_HAL_OSPI_DUALQUAD_DISABLE) {
        MODIFY_REG(FURI_HAL_OSPI->CR, OCTOSPI_CR_MSEL, cmd->flash_id);
    }

    if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_WRITE_CFG) {
        ccr_reg = &(FURI_HAL_OSPI->WCCR);
        tcr_reg = &(FURI_HAL_OSPI->WTCR);
        ir_reg = &(FURI_HAL_OSPI->WIR);
        abr_reg = &(FURI_HAL_OSPI->WABR);
    } else if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_WRAP_CFG) {
        ccr_reg = &(FURI_HAL_OSPI->WPCCR);
        tcr_reg = &(FURI_HAL_OSPI->WPTCR);
        ir_reg = &(FURI_HAL_OSPI->WPIR);
        abr_reg = &(FURI_HAL_OSPI->WPABR);
    } else {
        ccr_reg = &(FURI_HAL_OSPI->CCR);
        tcr_reg = &(FURI_HAL_OSPI->TCR);
        ir_reg = &(FURI_HAL_OSPI->IR);
        abr_reg = &(FURI_HAL_OSPI->ABR);
    }

    /* Configure the CCR register with DQS and SIOO modes */
    *ccr_reg = (cmd->dqs_mode | cmd->sdio_mode);

    if(cmd->alternate_bytes_mode != FURI_HAL_OSPI_ALTERNATE_BYTES_NONE) {
        /* Configure the ABR register with alternate bytes value */
        *abr_reg = cmd->alternate_bytes;

        /* Configure the CCR register with alternate bytes communication parameters */
        MODIFY_REG(
            (*ccr_reg),
            (OCTOSPI_CCR_ABMODE | OCTOSPI_CCR_ABDTR | OCTOSPI_CCR_ABSIZE),
            (cmd->alternate_bytes_mode | cmd->alternate_bytes_dtr_mode |
             cmd->alternate_bytes_size));
    }

    /* Configure the TCR register with the number of dummy cycles */
    MODIFY_REG((*tcr_reg), OCTOSPI_TCR_DCYC, cmd->dummy_cycles);

    if(cmd->data_mode != FURI_HAL_OSPI_DATA_NONE) {
        if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_COMMON_CFG) {
            /* Configure the DLR register with the number of data */
            FURI_HAL_OSPI->DLR = (cmd->nb_data - 1U);
        }
    }

    if(cmd->instruction_mode != FURI_HAL_OSPI_INSTRUCTION_NONE) {
        if(cmd->address_mode != FURI_HAL_OSPI_ADDRESS_NONE) {
            if(cmd->data_mode != FURI_HAL_OSPI_DATA_NONE) {
                /* ---- Command with instruction, address and data ---- */

                /* Configure the CCR register with all communication parameters */
                MODIFY_REG(
                    (*ccr_reg),
                    (OCTOSPI_CCR_IMODE | OCTOSPI_CCR_IDTR | OCTOSPI_CCR_ISIZE |
                     OCTOSPI_CCR_ADMODE | OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_ADSIZE |
                     OCTOSPI_CCR_DMODE | OCTOSPI_CCR_DDTR),
                    (cmd->instruction_mode | cmd->instruction_dtr_mode | cmd->instruction_size |
                     cmd->address_mode | cmd->address_dtr_mode | cmd->address_size |
                     cmd->data_mode | cmd->data_dtr_mode));
            } else {
                /* ---- Command with instruction and address ---- */

                /* Configure the CCR register with all communication parameters */
                MODIFY_REG(
                    (*ccr_reg),
                    (OCTOSPI_CCR_IMODE | OCTOSPI_CCR_IDTR | OCTOSPI_CCR_ISIZE |
                     OCTOSPI_CCR_ADMODE | OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_ADSIZE),
                    (cmd->instruction_mode | cmd->instruction_dtr_mode | cmd->instruction_size |
                     cmd->address_mode | cmd->address_dtr_mode | cmd->address_size));

                /* The DHQC bit is linked with DDTR bit which should be activated */
                if((FURI_HAL_OSPI_DELAYHOLDQUARTERCYCLE == FURI_HAL_OSPI_DHQC_ENABLE) &&
                   (cmd->instruction_dtr_mode == FURI_HAL_OSPI_INSTRUCTION_DTR_ENABLE)) {
                    MODIFY_REG((*ccr_reg), OCTOSPI_CCR_DDTR, FURI_HAL_OSPI_DATA_DTR_ENABLE);
                }
            }

            /* Configure the IR register with the instruction value */
            *ir_reg = cmd->instruction;

            /* Configure the AR register with the address value */
            FURI_HAL_OSPI->AR = cmd->address;
        } else {
            if(cmd->data_mode != FURI_HAL_OSPI_DATA_NONE) {
                /* ---- Command with instruction and data ---- */

                /* Configure the CCR register with all communication parameters */
                MODIFY_REG(
                    (*ccr_reg),
                    (OCTOSPI_CCR_IMODE | OCTOSPI_CCR_IDTR | OCTOSPI_CCR_ISIZE | OCTOSPI_CCR_DMODE |
                     OCTOSPI_CCR_DDTR),
                    (cmd->instruction_mode | cmd->instruction_dtr_mode | cmd->instruction_size |
                     cmd->data_mode | cmd->data_dtr_mode));
            } else {
                /* ---- Command with only instruction ---- */

                /* Configure the CCR register with all communication parameters */
                MODIFY_REG(
                    (*ccr_reg),
                    (OCTOSPI_CCR_IMODE | OCTOSPI_CCR_IDTR | OCTOSPI_CCR_ISIZE),
                    (cmd->instruction_mode | cmd->instruction_dtr_mode | cmd->instruction_size));

                /* The DHQC bit is linked with DDTR bit which should be activated */
                if((FURI_HAL_OSPI_DELAYHOLDQUARTERCYCLE == FURI_HAL_OSPI_DHQC_ENABLE) &&
                   (cmd->instruction_dtr_mode == FURI_HAL_OSPI_INSTRUCTION_DTR_ENABLE)) {
                    MODIFY_REG((*ccr_reg), OCTOSPI_CCR_DDTR, FURI_HAL_OSPI_DATA_DTR_ENABLE);
                }
            }

            /* Configure the IR register with the instruction value */
            *ir_reg = cmd->instruction;
        }
    } else {
        if(cmd->address_mode != FURI_HAL_OSPI_ADDRESS_NONE) {
            if(cmd->data_mode != FURI_HAL_OSPI_DATA_NONE) {
                /* ---- Command with address and data ---- */

                /* Configure the CCR register with all communication parameters */
                MODIFY_REG(
                    (*ccr_reg),
                    (OCTOSPI_CCR_ADMODE | OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_ADSIZE |
                     OCTOSPI_CCR_DMODE | OCTOSPI_CCR_DDTR),
                    (cmd->address_mode | cmd->address_dtr_mode | cmd->address_size |
                     cmd->data_mode | cmd->data_dtr_mode));
            } else {
                /* ---- Command with only address ---- */

                /* Configure the CCR register with all communication parameters */
                MODIFY_REG(
                    (*ccr_reg),
                    (OCTOSPI_CCR_ADMODE | OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_ADSIZE),
                    (cmd->address_mode | cmd->address_dtr_mode | cmd->address_size));
            }

            /* Configure the AR register with the instruction value */
            FURI_HAL_OSPI->AR = cmd->address;
        } else {
            /* ---- Invalid command configuration (no instruction, no address) ---- */
            status = false;
        }
    }

    /* Return function status */
    return status;
}

bool furi_hal_ospi_command(FuriHalOspiCommand* cmd) {
    bool status = true;
    furi_assert(cmd != NULL);
    /* Check the parameters of the command structure */
    furi_assert(IS_FURI_HAL_OSPI_OPERATION_TYPE(cmd->operation_type));

    if(FURI_HAL_OSPI_DUALQUAD == FURI_HAL_OSPI_DUALQUAD_DISABLE) {
        furi_assert(IS_FURI_HAL_OSPI_FLASH_ID(cmd->flash_id));
    }

    furi_assert(IS_FURI_HAL_OSPI_INSTRUCTION_MODE(cmd->instruction_mode));
    if(cmd->instruction_mode != FURI_HAL_OSPI_INSTRUCTION_NONE) {
        furi_assert(IS_FURI_HAL_OSPI_INSTRUCTION_SIZE(cmd->instruction_size));
        furi_assert(IS_FURI_HAL_OSPI_INSTRUCTION_DTR_MODE(cmd->instruction_dtr_mode));
    }

    furi_assert(IS_FURI_HAL_OSPI_ADDRESS_MODE(cmd->address_mode));
    if(cmd->address_mode != FURI_HAL_OSPI_ADDRESS_NONE) {
        furi_assert(IS_FURI_HAL_OSPI_ADDRESS_SIZE(cmd->address_size));
        furi_assert(IS_FURI_HAL_OSPI_ADDRESS_DTR_MODE(cmd->address_dtr_mode));
    }

    furi_assert(IS_FURI_HAL_OSPI_ALT_BYTES_MODE(cmd->alternate_bytes_mode));
    if(cmd->alternate_bytes_mode != FURI_HAL_OSPI_ALTERNATE_BYTES_NONE) {
        furi_assert(IS_FURI_HAL_OSPI_ALT_BYTES_SIZE(cmd->alternate_bytes_size));
        furi_assert(IS_FURI_HAL_OSPI_ALT_BYTES_DTR_MODE(cmd->alternate_bytes_dtr_mode));
    }

    furi_assert(IS_FURI_HAL_OSPI_DATA_MODE(cmd->data_mode));
    if(cmd->data_mode != FURI_HAL_OSPI_DATA_NONE) {
        if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_COMMON_CFG) {
            furi_assert(IS_FURI_HAL_OSPI_NUMBER_DATA(cmd->nb_data));
        }
        furi_assert(IS_FURI_HAL_OSPI_DATA_DTR_MODE(cmd->data_dtr_mode));
        furi_assert(IS_FURI_HAL_OSPI_DUMMY_CYCLES(cmd->dummy_cycles));
    }

    furi_assert(IS_FURI_HAL_OSPI_DQS_MODE(cmd->dqs_mode));
    furi_assert(IS_FURI_HAL_OSPI_SIOO_MODE(cmd->sdio_mode));

    /* Check the state of the driver */
    FuriHalOspiState state = furi_hal_ospi_state;
    if(((state == FuriHalOspiStateReady) &&
        (FURI_HAL_OSPI_MEMORYTYPE != FURI_HAL_OSPI_MEMTYPE_HYPERBUS)) ||
       ((state == FuriHalOspiStateReadCmdCfg) &&
        ((cmd->operation_type == FURI_HAL_OSPI_OPTYPE_WRITE_CFG) ||
         (cmd->operation_type == FURI_HAL_OSPI_OPTYPE_WRAP_CFG))) ||
       ((state == FuriHalOspiStateWriteCmdCfg) &&
        ((cmd->operation_type == FURI_HAL_OSPI_OPTYPE_READ_CFG) ||
         (cmd->operation_type == FURI_HAL_OSPI_OPTYPE_WRAP_CFG))) ||
       (state == FuriHalOspiStateAbort)) {
        /* Wait till busy flag is reset */
        if(furi_hal_osp_wait_flag_until_timeout(
               OCTOSPI_SR_BUSY, RESET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
            /* Configure the registers */
            status = furi_hal_ospi_config_cmd(cmd);

            if(status == true) {
                if(cmd->data_mode == FURI_HAL_OSPI_DATA_NONE) {
                    /* When there is no data phase, the transfer start as soon as the configuration is done
             so wait until TC flag is set to go back in idle state */
                    if(furi_hal_osp_wait_flag_until_timeout(
                           OCTOSPI_SR_TCF, SET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
                        status = true;
                    } else {
                        status = false;
                    }

                    WRITE_REG(FURI_HAL_OSPI->FCR, OCTOSPI_SR_TCF);
                } else {
                    /* Update the state */
                    if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_COMMON_CFG) {
                        furi_hal_ospi_state = FuriHalOspiStateCmdCfg;
                    } else if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_READ_CFG) {
                        if(furi_hal_ospi_state == FuriHalOspiStateWriteCmdCfg) {
                            furi_hal_ospi_state = FuriHalOspiStateCmdCfg;
                        } else {
                            furi_hal_ospi_state = FuriHalOspiStateReadCmdCfg;
                        }
                    } else if(cmd->operation_type == FURI_HAL_OSPI_OPTYPE_WRITE_CFG) {
                        if(furi_hal_ospi_state == FuriHalOspiStateReadCmdCfg) {
                            furi_hal_ospi_state = FuriHalOspiStateCmdCfg;
                        } else {
                            furi_hal_ospi_state = FuriHalOspiStateWriteCmdCfg;
                        }
                    } else {
                        /* Wrap configuration, no state change */
                    }
                }
            }
        }
    } else {
        furi_crash("OSPI: Error queue command");
    }

    /* Return function status */
    return status;
}

bool furi_hal_ospi_transmit(uint8_t* p_data) {
    furi_assert(p_data != NULL);

    bool status = true;
    __IO uint32_t* data_reg = &FURI_HAL_OSPI->DR;
    uint32_t p_data_size = 0;

    if(furi_hal_ospi_state == FuriHalOspiStateCmdCfg) {
        /* Configure counters and size */
        p_data_size = READ_REG(FURI_HAL_OSPI->DLR) + 1U;

        /* Configure CR register with functional mode as indirect write */
        MODIFY_REG(
            FURI_HAL_OSPI->CR, OCTOSPI_CR_FMODE, FURI_HAL_OSPI_FUNCTIONAL_MODE_INDIRECT_WRITE);

        do {
            /* Wait till fifo threshold flag is set to send data */
            if(!furi_hal_osp_wait_flag_until_timeout(
                   OCTOSPI_SR_FTF, SET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
                break;
            }

            *((__IO uint8_t*)data_reg) = *p_data;
            p_data++;
            p_data_size--;
        } while(p_data_size > 0U);

        if(status == true) {
            /* Wait till transfer complete flag is set to go back in idle state */
            if(furi_hal_osp_wait_flag_until_timeout(
                   OCTOSPI_SR_TCF, SET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
                /* Clear transfer complete flag */
                WRITE_REG(FURI_HAL_OSPI->FCR, OCTOSPI_SR_TCF);

                /* Update state */
                furi_hal_ospi_state = FuriHalOspiStateReady;
            }
        } else {
            furi_crash("OSPI: Error transmit");
        }
    } else {
        furi_crash("OSPI: Error transmit");
    }

    /* Return function status */
    return status;
}

bool furi_hal_ospi_receive(uint8_t* p_data) {
    furi_assert(p_data != NULL);

    bool status = true;
    __IO uint32_t* data_reg = &FURI_HAL_OSPI->DR;
    uint32_t addr_reg = FURI_HAL_OSPI->AR;
    uint32_t ir_reg = FURI_HAL_OSPI->IR;
    uint32_t p_data_size = 0;

    if(furi_hal_ospi_state == FuriHalOspiStateCmdCfg) {
        /* Configure counters and size */
        p_data_size = READ_REG(FURI_HAL_OSPI->DLR) + 1U;

        /* Configure CR register with functional mode as indirect read */
        MODIFY_REG(
            FURI_HAL_OSPI->CR, OCTOSPI_CR_FMODE, FURI_HAL_OSPI_FUNCTIONAL_MODE_INDIRECT_READ);

        /* Trig the transfer by re-writing address or instruction register */
        if(FURI_HAL_OSPI_MEMORYTYPE == FURI_HAL_OSPI_MEMTYPE_HYPERBUS) {
            WRITE_REG(FURI_HAL_OSPI->AR, addr_reg);
        } else {
            if(READ_BIT(FURI_HAL_OSPI->CCR, OCTOSPI_CCR_ADMODE) != FURI_HAL_OSPI_ADDRESS_NONE) {
                WRITE_REG(FURI_HAL_OSPI->AR, addr_reg);
            } else {
                WRITE_REG(FURI_HAL_OSPI->IR, ir_reg);
            }
        }

        do {
            /* Wait till fifo threshold or transfer complete flags are set to read received data */
            if(!furi_hal_osp_wait_flag_until_timeout(
                   (OCTOSPI_SR_FTF | OCTOSPI_SR_TCF), SET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
                break;
            }

            *p_data = *((__IO uint8_t*)data_reg);
            p_data++;
            p_data_size--;
        } while(p_data_size > 0U);

        if(status == true) {
            /* Wait till transfer complete flag is set to go back in idle state */
            if(furi_hal_osp_wait_flag_until_timeout(
                   OCTOSPI_SR_TCF, SET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
                /* Clear transfer complete flag */
                WRITE_REG(FURI_HAL_OSPI->FCR, OCTOSPI_SR_TCF);

                /* Update state */
                furi_hal_ospi_state = FuriHalOspiStateReady;
            }
        } else {
            furi_crash("OSPI: Error receive");
        }
    } else {
        furi_crash("OSPI: Error receive");
    }

    /* Return function status */
    return status;
}

void furi_hal_ospi_memory_mapped(uint32_t time_out_activation, uint32_t time_out_period) {
    if(furi_hal_osp_wait_flag_until_timeout(
           OCTOSPI_SR_BUSY, RESET, FURI_HAL_OSPI_TIMEOUT_DEFAULT_VALUE)) {
        furi_hal_ospi_state = FuriHalOspiStateBusyMemMapped;
        if(time_out_activation == FURI_HAL_OSPI_TIMEOUT_COUNTER_ENABLE) {
            /* Configure register */
            WRITE_REG(FURI_HAL_OSPI->LPTR, time_out_period);

            /* Clear flags related to interrupt */
            WRITE_REG(FURI_HAL_OSPI->FCR, OCTOSPI_SR_TOF);

            /* Enable the timeout interrupt */
            SET_BIT(FURI_HAL_OSPI->CR, OCTOSPI_CR_TOIE);

            /* Configure CR register with functional mode as memory-mapped */
            MODIFY_REG(
                FURI_HAL_OSPI->CR,
                (OCTOSPI_CR_TCEN | OCTOSPI_CR_FMODE),
                (time_out_activation | FURI_HAL_OSPI_FUNCTIONAL_MODE_MEMORY_MAPPED));
        }
    } else {
        furi_crash("OSPI: Memory mapped failed");
    }
}
