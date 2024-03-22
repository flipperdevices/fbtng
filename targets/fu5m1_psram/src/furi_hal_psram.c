#include "furi_hal_psram.h"
#include "apsxx08l.h"
#include "cachel1_armv7.h"
#include "dbg_log.h"

#define TAG "PSRAM"

//https://www.st.com/resource/en/application_note/an5050-getting-started-with-octospi-and-hexadecaspi-interface-on-stm32-microcontrollers-stmicroelectronics.pdf

#define FURI_HAL_PSRAM_READ_LATENCY 6

static OSPI_HandleTypeDef furi_hal_psram_ospi1;
static uint8_t furi_hal_psram_start(OSPI_HandleTypeDef* ctx);
static uint32_t furi_hal_psram_reset(OSPI_HandleTypeDef* Ctx);
static uint8_t furi_hal_psram_apsxx08l_read_reg(OSPI_HandleTypeDef* ctx, uint32_t address);
static void
    furi_hal_psram_apsxx08l_write_reg(OSPI_HandleTypeDef* ctx, uint32_t address, uint8_t value);
static void furi_hal_psram_disable_compensation_cell(void);

static void Error_Handler(void) {
    DBG_LOG_E(TAG, "Error_Handler");
    while(1)
        ;
}

void HAL_OSPI_MspInit(OSPI_HandleTypeDef* ospiHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    if(ospiHandle->Instance == OCTOSPI1) {
        /** Initializes the peripherals clock
		 */
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
        PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_SYSCLK;
        if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        /* OCTOSPI1 clock enable */
        __HAL_RCC_OSPIM_CLK_ENABLE();
        __HAL_RCC_OSPI1_CLK_ENABLE();

        __HAL_RCC_GPIOD_CLK_ENABLE();
        __HAL_RCC_GPIOH_CLK_ENABLE();
        __HAL_RCC_GPIOG_CLK_ENABLE();
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**OCTOSPI1 GPIO Configuration
		 PD5     ------> OCTOSPIM_P1_IO5
		 PH2     ------> OCTOSPIM_P1_IO4
		 PG6     ------> OCTOSPIM_P1_DQS
		 PE13     ------> OCTOSPIM_P1_IO1
		 PC3     ------> OCTOSPIM_P1_IO6
		 PE15     ------> OCTOSPIM_P1_IO3
		 PE12     ------> OCTOSPIM_P1_IO0
		 PE10     ------> OCTOSPIM_P1_CLK
		 PE11     ------> OCTOSPIM_P1_NCS
		 PC4     ------> OCTOSPIM_P1_IO7
		 PE14     ------> OCTOSPIM_P1_IO2
		 */
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF3_OCTOSPI1;
        HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF3_OCTOSPI1;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_15 | GPIO_PIN_12 | GPIO_PIN_10 | GPIO_PIN_11 |
                              GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF10_OCTOSPI1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    }
}

void HAL_OSPI_MspDeInit(OSPI_HandleTypeDef* ospiHandle) {
    if(ospiHandle->Instance == OCTOSPI1) {
        /* Peripheral clock disable */
        __HAL_RCC_OSPIM_CLK_DISABLE();
        __HAL_RCC_OSPI1_CLK_DISABLE();

        /**OCTOSPI1 GPIO Configuration
		 PD5     ------> OCTOSPIM_P1_IO5
		 PH2     ------> OCTOSPIM_P1_IO4
		 PG6     ------> OCTOSPIM_P1_DQS
		 PE13     ------> OCTOSPIM_P1_IO1
		 PC3     ------> OCTOSPIM_P1_IO6
		 PE15     ------> OCTOSPIM_P1_IO3
		 PE12     ------> OCTOSPIM_P1_IO0
		 PE10     ------> OCTOSPIM_P1_CLK
		 PE11     ------> OCTOSPIM_P1_NCS
		 PC4     ------> OCTOSPIM_P1_IO7
		 PE14     ------> OCTOSPIM_P1_IO2
		 */
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5);

        HAL_GPIO_DeInit(GPIOH, GPIO_PIN_2);

        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_6);

        HAL_GPIO_DeInit(
            GPIOE,
            GPIO_PIN_13 | GPIO_PIN_15 | GPIO_PIN_12 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_14);

        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_3 | GPIO_PIN_4);
    }
}

void furi_hal_psram_init(void) {
    //Designed for SYSclock frequency 160Mhz

    OSPIM_CfgTypeDef sOspiManagerCfg = {0};
    HAL_OSPI_DLYB_CfgTypeDef HAL_OSPI_DLYB_Cfg_Struct = {0};

    furi_hal_psram_ospi1.Instance = OCTOSPI1;
    furi_hal_psram_ospi1.Init.FifoThreshold = 1;
    furi_hal_psram_ospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    furi_hal_psram_ospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_APMEMORY;
    furi_hal_psram_ospi1.Init.DeviceSize = 24; //128Mbit
    furi_hal_psram_ospi1.Init.ChipSelectHighTime = 2;
    furi_hal_psram_ospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    furi_hal_psram_ospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    furi_hal_psram_ospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    furi_hal_psram_ospi1.Init.ClockPrescaler = 2; //FreqClock 160Mhz/2 = 80Mhz
    furi_hal_psram_ospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
    furi_hal_psram_ospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE;
    furi_hal_psram_ospi1.Init.ChipSelectBoundary = 10; //boundary 1k
    furi_hal_psram_ospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED;
    furi_hal_psram_ospi1.Init.MaxTran = 0;
    furi_hal_psram_ospi1.Init.Refresh = 320; // 4us 320clock freq 80Mhz
    if(HAL_OSPI_Init(&furi_hal_psram_ospi1) != HAL_OK) {
        Error_Handler();
    }
    sOspiManagerCfg.ClkPort = 1;
    sOspiManagerCfg.DQSPort = 1;
    sOspiManagerCfg.NCSPort = 1;
    sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
    sOspiManagerCfg.IOHighPort = HAL_OSPIM_IOPORT_1_HIGH;
    if(HAL_OSPIM_Config(&furi_hal_psram_ospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) !=
       HAL_OK) {
        Error_Handler();
    }
    HAL_OSPI_DLYB_Cfg_Struct.Units = 0;
    HAL_OSPI_DLYB_Cfg_Struct.PhaseSel = 0;
    if(HAL_OSPI_DLYB_SetConfig(&furi_hal_psram_ospi1, &HAL_OSPI_DLYB_Cfg_Struct) != HAL_OK) {
        Error_Handler();
    }

    if(!furi_hal_psram_start(&furi_hal_psram_ospi1)) {
        Error_Handler();
    }
}

void furi_hal_psram_deinit(void) {
    /* Disable OSPI1 */
    if(HAL_OSPI_Abort(&furi_hal_psram_ospi1) != HAL_OK) {
        Error_Handler();
    }
    /* Reset PSRAM */
    furi_hal_psram_reset(&furi_hal_psram_ospi1);
    /* Enables Slow Refresh and None update */
    furi_hal_psram_apsxx08l_write_reg(
        &furi_hal_psram_ospi1, APSXX08L_MR4_ADDRESS, APSXX08L_MR4_PASR_NONE | APSXX08L_MR4_RF_SR);

    /* Disable Compensation cell */
    furi_hal_psram_disable_compensation_cell();

    /* Deinit GPIO */
    if(HAL_OSPI_DeInit(&furi_hal_psram_ospi1) != HAL_OK) {
        Error_Handler();
    }
}

static uint8_t furi_hal_psram_test(void) {
    const uint32_t buf_size = 1024 * 10;
    uint8_t check_buf[buf_size];
    uint32_t error_buf = 0;
    __IO uint8_t* mem_addr;

    //Create check buff
    for(uint32_t i = 0; i < buf_size; i++) {
        check_buf[i] = (i & 0xFF);
    }

    /* Writing Sequence */
    mem_addr = (__IO uint8_t*)(OCTOSPI1_BASE);
    for(uint32_t i = 0; i < buf_size; i++) {
        *mem_addr = check_buf[i];
        mem_addr++;
    }

    /* Reading Sequence */
    mem_addr = (__IO uint8_t*)(OCTOSPI1_BASE);
    for(uint32_t i = 0; i < buf_size; i++) {
        if(*mem_addr != check_buf[i]) {
            error_buf++;
        }
        mem_addr++;
    }

    if(!error_buf) {
        return 1;
    }
    return 0;
}

static void furi_hal_psram_disable_compensation_cell(void) {
    HAL_SYSCFG_DisableVddCompensationCell();
    __HAL_RCC_SYSCFG_CLK_DISABLE();
}

static void furi_hal_psram_enable_compensation_cell(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    HAL_SYSCFG_EnableVddCompensationCell();
}

static void furi_hal_psram_delay_configuration(OSPI_HandleTypeDef* ctx) {
    LL_DLYB_CfgTypeDef dlyb_cfg, dlyb_cfg_test;

    /* Delay block configuration*/
    if(HAL_OSPI_DLYB_GetClockPeriod(ctx, &dlyb_cfg) != HAL_OK) {
        Error_Handler();
    }

    /*when DTR, PhaseSel is divided by 4 (emperic value)*/
    dlyb_cfg.PhaseSel /= 4;

    /* save the present configuration for check*/
    dlyb_cfg_test = dlyb_cfg;

    /*set delay block configuration*/
    HAL_OSPI_DLYB_SetConfig(ctx, &dlyb_cfg);

    /*check the set value*/
    HAL_OSPI_DLYB_GetConfig(ctx, &dlyb_cfg);
    if((dlyb_cfg.PhaseSel != dlyb_cfg_test.PhaseSel) || (dlyb_cfg.Units != dlyb_cfg_test.Units)) {
        Error_Handler();
    }
}

static uint32_t
    furi_hal_psram_apsxx08l_write_2reg(OSPI_HandleTypeDef* ctx, uint32_t address, uint8_t* value) {
    // According to the specification, 2 bytes are written, the recording is configured to mask 2 bytes

    OSPI_RegularCmdTypeDef cmd = {0};

    /* Initialize the write register command */
    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.Instruction = APSXX08L_WRITE_REG_CMD;
    cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
    cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
    cmd.Address = address;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_8_LINES;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
    cmd.NbData = 2;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if(HAL_OSPI_Command(ctx, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Transmission of the data */
    if(HAL_OSPI_Transmit(ctx, (uint8_t*)(value), HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint32_t furi_hal_psram_apsxx08l_read_2reg(
    OSPI_HandleTypeDef* ctx,
    uint32_t address,
    uint8_t* value,
    uint32_t latency) {
    // According to the specification, 2 bytes are read, reading is configured to mask 2 bytes

    OSPI_RegularCmdTypeDef cmd = {0};
    ;

    /* Initialize the read register command */
    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.Instruction = APSXX08L_READ_REG_CMD;
    cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
    cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
    cmd.Address = address;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_8_LINES;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE;
    cmd.NbData = 2;
    cmd.DummyCycles = (latency - 1U);
    cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if(HAL_OSPI_Command(ctx, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Reception of the data */
    if(HAL_OSPI_Receive(ctx, (uint8_t*)value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static uint32_t furi_hal_psram_reset(OSPI_HandleTypeDef* ctx) {
    OSPI_RegularCmdTypeDef cmd = {0};

    /* Initialize the command */
    cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.Instruction = APSXX08L_RESET_CMD;
    cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
    cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_DISABLE;
    cmd.Address = 0;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_NONE;
    cmd.NbData = 0;
    cmd.DummyCycles = 0;
    cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    /* Configure the command */
    if(HAL_OSPI_Command(ctx, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Need to wait tRST */
    HAL_Delay(1);

    return HAL_OK;
}

static void
    furi_hal_psram_apsxx08l_write_reg(OSPI_HandleTypeDef* ctx, uint32_t address, uint8_t value) {
    uint8_t reg_temp[2] = {0};

    reg_temp[0] = value;
    if(furi_hal_psram_apsxx08l_write_2reg(ctx, address, reg_temp) != HAL_OK) {
        Error_Handler();
    }
}

static uint8_t furi_hal_psram_apsxx08l_read_reg(OSPI_HandleTypeDef* ctx, uint32_t address) {
    uint8_t reg_temp[2] = {0};

    SCB_InvalidateDCache();
    if(furi_hal_psram_apsxx08l_read_2reg(ctx, address, reg_temp, FURI_HAL_PSRAM_READ_LATENCY) !=
       HAL_OK) {
        Error_Handler();
    }

    return reg_temp[0];
}

static void furi_hal_psram_apmemory_configuration(OSPI_HandleTypeDef* ctx) {
    uint8_t regW_MR0 =
        APSXX08L_MR0_RLC_8 |
        APSXX08L_MR0_DS_HALF; /* To configure AP memory Latency Type and drive Strength */
    uint8_t regW_MR8 = APSXX08L_MR8_RBX_ON | APSXX08L_MR8_BT_OFF |
                       APSXX08L_MR8_BL_1K_BYTES; /* To configure AP memory Burst Type */

    /* Reset PSRAM */
    furi_hal_psram_reset(&furi_hal_psram_ospi1);

    furi_hal_psram_apsxx08l_write_reg(ctx, APSXX08L_MR0_ADDRESS, regW_MR0);
    if(furi_hal_psram_apsxx08l_read_reg(ctx, APSXX08L_MR0_ADDRESS) != regW_MR0) {
        Error_Handler();
    }

    furi_hal_psram_apsxx08l_write_reg(ctx, APSXX08L_MR8_ADDRESS, regW_MR8);
    if(furi_hal_psram_apsxx08l_read_reg(ctx, APSXX08L_MR8_ADDRESS) != regW_MR8) {
        Error_Handler();
    }
}

static void furi_hal_psram_memory_mapped(OSPI_HandleTypeDef* ctx) {
    OSPI_RegularCmdTypeDef cmd = {0};
    OSPI_MemoryMappedTypeDef mem_mapped_cfg = {0};
    ;

    cmd.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
    cmd.FlashId = HAL_OSPI_FLASH_ID_1;
    cmd.Instruction = APSXX08L_WRITE_CMD;
    cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.InstructionSize = HAL_OSPI_INSTRUCTION_16_BITS;
    cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_ENABLE;
    cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
    cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
    cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
    cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.DataMode = HAL_OSPI_DATA_8_LINES;
    cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
    cmd.DummyCycles = APSXX08L_DUMMY_CLOCK_CYCLES_WRITE;
    cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
    cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

    if(HAL_OSPI_Command(ctx, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        Error_Handler();
    }

    cmd.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
    cmd.Instruction = APSXX08L_READ_CMD;
    cmd.DummyCycles = APSXX08L_DUMMY_CLOCK_CYCLES_READ;

    if(HAL_OSPI_Command(ctx, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        Error_Handler();
    }

    mem_mapped_cfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
    mem_mapped_cfg.TimeOutPeriod = 0x34;

    if(HAL_OSPI_MemoryMapped(ctx, &mem_mapped_cfg) != HAL_OK) {
        Error_Handler();
    }
}

static uint8_t furi_hal_psram_start(OSPI_HandleTypeDef* ctx) {
    /* Enable Compensation cell */
    furi_hal_psram_enable_compensation_cell();

    /* Delay block configuration*/
    furi_hal_psram_delay_configuration(ctx);

    /*Configure AP memory register */
    furi_hal_psram_apmemory_configuration(ctx);

    /*Configure Memory Mapped mode*/
    furi_hal_psram_memory_mapped(ctx);

    return furi_hal_psram_test();
}

void HAL_OSPI_ErrorCallback(OSPI_HandleTypeDef* hospi) {
    Error_Handler();
}
