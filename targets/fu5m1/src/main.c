#include "st_hal.h"
#include "dbg_log.h"
#include "test_tim1_gdma_ch0.h"
#include "furi_hal_psram.h"
#include "furi_hal_button.h"
#include "furi_hal_pssi_dma.h"
#include "furi_hal_mem.h"
#include "furi_hal_cortex.h"

#define TAG "main"

static void clock_init(void);
static void system_power_config(void);
static void cache_init(void);
static void led_gpio_init(void);
void print_pssi(uint8_t* buf, uint16_t size, void* context);

// TODO: get rid of these
void _close(void) {
}
void _lseek(void) {
}
void _read(void) {
}
void _write(void) {
}

uint32_t memory_latency_write_test(uint32_t* addr, size_t cycles) {
    uint32_t start = DWT->CYCCNT;
    for(size_t i = 0; i < cycles; i++) {
        *addr = i;
    }
    uint32_t end = DWT->CYCCNT;

    return end - start;
}

uint32_t memory_latency_read_test(uint32_t* addr, size_t cycles) {
    uint32_t start = DWT->CYCCNT;
    for(size_t i = 0; i < cycles; i++) {
        volatile uint32_t val = *addr;
        UNUSED(val);
    }
    uint32_t end = DWT->CYCCNT;

    return end - start;
}

int main(void) {
    furi_hal_cortex_init_early();

    HAL_Init();

    LL_AHB3_GRP1_EnableClock(LL_AHB3_GRP1_PERIPH_PWR);

    clock_init();
    system_power_config();
    cache_init();

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);

    led_gpio_init();
    test_pa7_tim1_gdma_ch0_init();
    dbg_log_init(DBG_LOG_LVL_INFO);
    DBG_LOG_I(TAG, "Start");

    furi_hal_psram_init();
    DBG_LOG_I(TAG, "PSRAM init done");

    furi_hal_button_init();
    DBG_LOG_I(TAG, "Button init done");

    furi_hal_pssi_init_bus8line(512);
    furi_hal_pssi_set_rx_callback(print_pssi, NULL);
    furi_hal_pssi_dma_receve_start();
    DBG_LOG_I(TAG, "PSSI init done");

    furi_hal_mem_print_memory_layout();

    uint32_t sram1_var;
    uint32_t* psram_addr = (uint32_t*)furi_hal_mem_pool_get_addr(FuriHalMemPoolPSram);
    uint32_t cycles = 10000000;
    uint32_t latency;

    latency = memory_latency_write_test(psram_addr, cycles);
    DBG_LOG_I(TAG, "PSRAM write latency: %lu", latency);

    latency = memory_latency_write_test(&sram1_var, cycles);
    DBG_LOG_I(TAG, "SRAM1 write latency: %lu", latency);

    latency = memory_latency_read_test(psram_addr, cycles);
    DBG_LOG_I(TAG, "PSRAM read latency: %lu", latency);

    latency = memory_latency_read_test(&sram1_var, cycles);
    DBG_LOG_I(TAG, "SRAM1 read latency: %lu", latency);

    while(1) {
        // LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7);
        // LL_mDelay(500);
        // LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
        // LL_mDelay(500);
    }
}

static void clock_init(void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
    while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_4) {
    }

    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    while(LL_PWR_IsActiveFlag_VOS() == 0) {
    }

    LL_RCC_HSE_Enable();
    while(LL_RCC_HSE_IsReady() != 1) {
    }

    LL_RCC_PLL1_ConfigDomain_SYS(LL_RCC_PLL1SOURCE_HSE, 1, 10, 1);
    LL_RCC_PLL1_EnableDomain_SYS();
    LL_RCC_SetPll1EPodPrescaler(LL_RCC_PLL1MBOOST_DIV_1);
    LL_RCC_PLL1_SetVCOInputRange(LL_RCC_PLLINPUTRANGE_8_16);

    LL_RCC_PLL1_Enable();
    while(LL_RCC_PLL1_IsReady() != 1) {
    }

    /* Intermediate AHB prescaler 2 when target frequency clock is higher than 80 MHz */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_2);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL1);
    while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL1) {
    }

    /* Insure 1us transition state at intermediate medium speed clock*/
    for(__IO uint32_t i = (160 >> 1); i != 0; i--)
        ;

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetAPB3Prescaler(LL_RCC_APB3_DIV_1);

    // LL_Init1msTick(160000000);
    LL_SetSystemCoreClock(160000000);

    /* Update the time base */
    if(HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) {
        //Error_Handler();
    }
}

static void system_power_config(void) {
    LL_PWR_EnableVddIO2();

    /*
   * Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
   */
    LL_PWR_DisableUCPDDeadBattery();

    /*
   * Switch to SMPS regulator instead of LDO
   */
    LL_PWR_SetRegulatorSupply(LL_PWR_SMPS_SUPPLY);

    while(LL_PWR_IsActiveFlag_REGULATOR() != 1) {
    }
}

static void cache_init(void) {
    LL_ICACHE_SetMode(LL_ICACHE_1WAY);
    LL_ICACHE_Enable();

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DCACHE1);
    LL_DCACHE_SetReadBurstType(DCACHE1, LL_DCACHE_READ_BURST_WRAP);
    LL_DCACHE_Enable(DCACHE1);
}

static void led_gpio_init(void) {
    LL_GPIO_InitTypeDef led_gpio = {};

    led_gpio.Pin = LL_GPIO_PIN_7;
    led_gpio.Mode = LL_GPIO_MODE_OUTPUT;
    led_gpio.Speed = LL_GPIO_SPEED_FREQ_LOW;
    led_gpio.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    led_gpio.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &led_gpio);
}

void print_pssi(uint8_t* buf, uint16_t size, void* context) {
    DBG_DUMP(TAG, buf, size);
}

void SysTick_Handler(void) {
    HAL_IncTick();
}
