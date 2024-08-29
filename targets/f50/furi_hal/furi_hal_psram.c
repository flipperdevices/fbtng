#include "furi_hal_psram.h"
#include "apsxx08l.h"

#include "stm32u5xx_ll_rcc.h"
#include "stm32u5xx_ll_system.h"

#include "furi_hal_ospi.h"
#include "furi_hal_bus.h"
#include "furi_hal_resources.h"
#include "furi_hal_gpio.h"
#include "furi_hal_cortex.h"

//todo remove LL_PWR_EnableVddIO2();
#include "stm32u5xx_ll_pwr.h"

#define TAG "PSRAM"

//https://www.st.com/resource/en/application_note/an5050-getting-started-with-octospi-and-hexadecaspi-interface-on-stm32-microcontrollers-stmicroelectronics.pdf

#define FURI_HAL_PSRAM_READ_LATENCY 6

static uint8_t furi_hal_psram_start(void);
static uint32_t furi_hal_psram_reset(void);
static uint8_t furi_hal_psram_apsxx08l_read_reg(uint32_t address);
static void furi_hal_psram_apsxx08l_write_reg(uint32_t address, uint8_t value);
static void furi_hal_psram_disable_compensation_cell(void);

static void furi_hal_psram_init_pin(void) {
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io0,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io1,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io2,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io3,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io4,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn3OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io5,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io6,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_io7,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_clk,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_ncs,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10OCTOSPI1);
    furi_hal_gpio_init_ex(
        &gpio_octospi1_psram_dqs,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn3OCTOSPI1);
}

static void furi_hal_psram_deinit_pin(void) {
    furi_hal_gpio_init(&gpio_octospi1_psram_io0, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io1, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io2, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io3, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io4, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io5, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io6, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_io7, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_clk, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_octospi1_psram_ncs, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
}

void furi_hal_psram_init(void) {
    //Designed for SYSclock frequency 160Mhz

    furi_hal_psram_init_pin();

    furi_hal_ospi_init();

    FuriHalOspiConfig sOspiManagerCfg = {0};
    sOspiManagerCfg.clk_port = 1;
    sOspiManagerCfg.dqs_port = 1;
    sOspiManagerCfg.ncs_port = 1;
    sOspiManagerCfg.io_low_port = FURI_HAL_OSPIM_IOPORT_1_LOW;
    sOspiManagerCfg.io_high_port = FURI_HAL_OSPIM_IOPORT_1_HIGH;
    furi_hal_ospi_config_no_mux_ospi1(&sOspiManagerCfg);

    LL_DLYB_CfgTypeDef dlyb_cfg = {0};
    dlyb_cfg.PhaseSel = 0;
    dlyb_cfg.Units = 0;
    furi_hal_ospi_dlyb_set_config(&dlyb_cfg);

    if(!furi_hal_psram_start()) {
        furi_crash("PSRAM: InitError");
    }
    FURI_LOG_I(TAG, "Init OK");
}

void furi_hal_psram_deinit(void) {
    /* Disable OSPI1 */
    furi_hal_ospi_abort();

    /* Reset PSRAM */
    furi_hal_psram_reset();
    /* Enables Slow Refresh and None update */
    furi_hal_psram_apsxx08l_write_reg(
        APSXX08L_MR4_ADDRESS, APSXX08L_MR4_PASR_NONE | APSXX08L_MR4_RF_SR);

    /* Disable Compensation cell */
    furi_hal_psram_disable_compensation_cell();

    /* Disable OctoSPI */
    furi_hal_ospi_deint();

    /* De-initialize pin */
    furi_hal_psram_deinit_pin();

    FURI_LOG_I(TAG, "Deinit OK");
}

static uint8_t furi_hal_psram_test(void) {
    const uint32_t buf_size = 1024 * 10;
    uint8_t* check_buf = malloc(buf_size * sizeof(uint8_t));

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
    free(check_buf);
    if(!error_buf) {
        return 1;
    }
    return 0;
}

static void furi_hal_psram_disable_compensation_cell(void) {
    LL_SYSCFG_DisableVddCompensationCell();
    furi_hal_bus_disable(FuriHalBusSYSCFG);
}

static void furi_hal_psram_enable_compensation_cell(void) {
    furi_hal_bus_enable(FuriHalBusSYSCFG);
    LL_SYSCFG_EnableVddCompensationCell();
}

static void furi_hal_psram_delay_configuration(void) {
    LL_DLYB_CfgTypeDef dlyb_cfg, dlyb_cfg_test;

    if(furi_hal_ospi_dlyb_get_clock_period(&dlyb_cfg) != true) {
        furi_crash("PSRAM: Delay block configuration error");
    }

    /*when DTR, PhaseSel is divided by 4 (empiric value)*/
    dlyb_cfg.PhaseSel /= 4;

    /* save the present configuration for check*/
    dlyb_cfg_test = dlyb_cfg;

    /*set delay block configuration*/
    furi_hal_ospi_dlyb_set_config(&dlyb_cfg);

    /*check the set value*/
    LL_DLYB_GetDelay(DLYB_OCTOSPI1, &dlyb_cfg);

    if((dlyb_cfg.PhaseSel != dlyb_cfg_test.PhaseSel) || (dlyb_cfg.Units != dlyb_cfg_test.Units)) {
        furi_crash("PSRAM: Delay block configuration error");
    }
}

static bool furi_hal_psram_apsxx08l_write_2reg(uint32_t address, uint8_t* value) {
    // According to the specification, 2 bytes are written, the recording is configured to mask 2 bytes

    FuriHalOspiCommand cmd = {0};

    /* Initialize the write register command */
    cmd.operation_type = FURI_HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.instruction_mode = FURI_HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.instruction_size = FURI_HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.instruction_dtr_mode = FURI_HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.instruction = APSXX08L_WRITE_REG_CMD;
    cmd.address_mode = FURI_HAL_OSPI_ADDRESS_8_LINES;
    cmd.address_size = FURI_HAL_OSPI_ADDRESS_32_BITS;
    cmd.address_dtr_mode = FURI_HAL_OSPI_ADDRESS_DTR_ENABLE;
    cmd.address = address;
    cmd.alternate_bytes_mode = FURI_HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.data_mode = FURI_HAL_OSPI_DATA_8_LINES;
    cmd.data_dtr_mode = FURI_HAL_OSPI_DATA_DTR_ENABLE;
    cmd.nb_data = 2;
    cmd.dummy_cycles = 0;
    cmd.dqs_mode = FURI_HAL_OSPI_DQS_DISABLE;
    cmd.sdio_mode = FURI_HAL_OSPI_SIOO_INST_EVERY_CMD;

    if(!furi_hal_ospi_command(&cmd)) {
        return false;
    }

    if(!furi_hal_ospi_transmit((uint8_t*)(value))) {
        return false;
    }
    return true;
}

static uint32_t
    furi_hal_psram_apsxx08l_read_2reg(uint32_t address, uint8_t* value, uint32_t latency) {
    // According to the specification, 2 bytes are read, reading is configured to mask 2 bytes

    FuriHalOspiCommand cmd = {0};

    /* Initialize the read register command */
    cmd.operation_type = FURI_HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.instruction_mode = FURI_HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.instruction_size = FURI_HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.instruction_dtr_mode = FURI_HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.instruction = APSXX08L_READ_REG_CMD;
    cmd.address_mode = FURI_HAL_OSPI_ADDRESS_8_LINES;
    cmd.address_size = FURI_HAL_OSPI_ADDRESS_32_BITS;
    cmd.address_dtr_mode = FURI_HAL_OSPI_ADDRESS_DTR_ENABLE;
    cmd.address = address;
    cmd.alternate_bytes_mode = FURI_HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.data_mode = FURI_HAL_OSPI_DATA_8_LINES;
    cmd.data_dtr_mode = FURI_HAL_OSPI_DATA_DTR_DISABLE;
    cmd.nb_data = 2;
    cmd.dummy_cycles = (latency - 1U);
    cmd.dqs_mode = FURI_HAL_OSPI_DQS_ENABLE;
    cmd.sdio_mode = FURI_HAL_OSPI_SIOO_INST_EVERY_CMD;

    if(!furi_hal_ospi_command(&cmd)) {
        return false;
    }

    if(!furi_hal_ospi_receive((uint8_t*)value)) {
        return false;
    }
    return true;
}

static uint32_t furi_hal_psram_reset(void) {
    FuriHalOspiCommand cmd = {0};

    /* Initialize the command */
    cmd.operation_type = FURI_HAL_OSPI_OPTYPE_COMMON_CFG;
    cmd.flash_id = FURI_HAL_OSPI_FLASH_ID_1;
    cmd.instruction_mode = FURI_HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.instruction_size = FURI_HAL_OSPI_INSTRUCTION_8_BITS;
    cmd.instruction_dtr_mode = FURI_HAL_OSPI_INSTRUCTION_DTR_DISABLE;
    cmd.instruction = APSXX08L_RESET_CMD;
    cmd.address_mode = FURI_HAL_OSPI_ADDRESS_8_LINES;
    cmd.address_size = FURI_HAL_OSPI_ADDRESS_32_BITS;
    cmd.address_dtr_mode = FURI_HAL_OSPI_ADDRESS_DTR_DISABLE;
    cmd.address = 0;
    cmd.alternate_bytes_mode = FURI_HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.data_mode = FURI_HAL_OSPI_DATA_NONE;
    cmd.nb_data = 0;
    cmd.dummy_cycles = 0;
    cmd.dqs_mode = FURI_HAL_OSPI_DQS_DISABLE;
    cmd.sdio_mode = FURI_HAL_OSPI_SIOO_INST_EVERY_CMD;

    if(!furi_hal_ospi_command(&cmd)) {
        return false;
    }

    /* Need to wait tRST */
    FuriHalCortexTimer timer = furi_hal_cortex_timer_get(1 * 1000);
    while(!furi_hal_cortex_timer_is_expired(timer)) {
    };

    return true;
}

static void furi_hal_psram_apsxx08l_write_reg(uint32_t address, uint8_t value) {
    uint8_t reg_temp[2] = {0};

    reg_temp[0] = value;
    if(!furi_hal_psram_apsxx08l_write_2reg(address, reg_temp)) {
        furi_crash("PSRAM: Not able to write register");
    }
}

static uint8_t furi_hal_psram_apsxx08l_read_reg(uint32_t address) {
    uint8_t reg_temp[2] = {0};

    if(!furi_hal_psram_apsxx08l_read_2reg(address, reg_temp, FURI_HAL_PSRAM_READ_LATENCY)) {
        furi_crash("PSRAM: Not able to read register");
    }

    return reg_temp[0];
}

static void furi_hal_psram_apmemory_configuration(void) {
    uint8_t regW_MR0 =
        APSXX08L_MR0_RLC_8 |
        APSXX08L_MR0_DS_HALF; /* To configure AP memory Latency Type and drive Strength */
    uint8_t regW_MR8 = APSXX08L_MR8_RBX_ON | APSXX08L_MR8_BT_OFF |
                       APSXX08L_MR8_BL_1K_BYTES; /* To configure AP memory Burst Type */

    /* Reset PSRAM */
    furi_hal_psram_reset();

    furi_hal_psram_apsxx08l_write_reg(APSXX08L_MR0_ADDRESS, regW_MR0);
    if(furi_hal_psram_apsxx08l_read_reg(APSXX08L_MR0_ADDRESS) != regW_MR0) {
        furi_crash("PSRAM: Case check error regW_MR0");
    }

    furi_hal_psram_apsxx08l_write_reg(APSXX08L_MR8_ADDRESS, regW_MR8);
    if(furi_hal_psram_apsxx08l_read_reg(APSXX08L_MR8_ADDRESS) != regW_MR8) {
        furi_crash("PSRAM: Case check error regW_MR8");
    }
}

static void furi_hal_psram_memory_mapped(void) {
    FuriHalOspiCommand cmd = {0};

    cmd.operation_type = FURI_HAL_OSPI_OPTYPE_WRITE_CFG;
    cmd.flash_id = FURI_HAL_OSPI_FLASH_ID_1;
    cmd.instruction = APSXX08L_WRITE_CMD;
    cmd.instruction_mode = FURI_HAL_OSPI_INSTRUCTION_8_LINES;
    cmd.instruction_size = FURI_HAL_OSPI_INSTRUCTION_16_BITS;
    cmd.instruction_dtr_mode = FURI_HAL_OSPI_INSTRUCTION_DTR_ENABLE;
    cmd.address_mode = FURI_HAL_OSPI_ADDRESS_8_LINES;
    cmd.address_size = FURI_HAL_OSPI_ADDRESS_32_BITS;
    cmd.address_dtr_mode = FURI_HAL_OSPI_ADDRESS_DTR_ENABLE;
    cmd.alternate_bytes_mode = FURI_HAL_OSPI_ALTERNATE_BYTES_NONE;
    cmd.data_mode = FURI_HAL_OSPI_DATA_8_LINES;
    cmd.data_dtr_mode = FURI_HAL_OSPI_DATA_DTR_ENABLE;
    cmd.dummy_cycles = APSXX08L_DUMMY_CLOCK_CYCLES_WRITE;
    cmd.dqs_mode = FURI_HAL_OSPI_DQS_ENABLE;
    cmd.sdio_mode = FURI_HAL_OSPI_SIOO_INST_EVERY_CMD;

    if(!furi_hal_ospi_command(&cmd)) {
        furi_crash("PSRAM: Not able to write command");
    }

    cmd.operation_type = FURI_HAL_OSPI_OPTYPE_READ_CFG;
    cmd.instruction = APSXX08L_READ_CMD;
    cmd.dummy_cycles = APSXX08L_DUMMY_CLOCK_CYCLES_READ;

    if(!furi_hal_ospi_command(&cmd)) {
        furi_crash("PSRAM: Not able to write command");
    }

    /* Configure the memory mapped mode */
    furi_hal_ospi_memory_mapped(FURI_HAL_OSPI_TIMEOUT_COUNTER_ENABLE, 0x34);
}

static uint8_t furi_hal_psram_start(void) {
    /* Enable Compensation cell */
    furi_hal_psram_enable_compensation_cell();

    /* Delay block configuration*/
    furi_hal_psram_delay_configuration();

    /*Configure AP memory register */
    furi_hal_psram_apmemory_configuration();

    /*Configure Memory Mapped mode*/
    furi_hal_psram_memory_mapped();

    return furi_hal_psram_test();
}
