#include <furi_hal_resources.h>
#include <furi_hal_bus.h>
#include <stm32u5xx_ll_rcc.h>

const GpioPin gpio_mco = {.port = GPIOA, .pin = LL_GPIO_PIN_8};
const GpioPin gpio_swdio = {.port = GPIOA, .pin = LL_GPIO_PIN_13};
const GpioPin gpio_swclk = {.port = GPIOA, .pin = LL_GPIO_PIN_14};
const GpioPin gpio_sd_card_power_switch = {.port = GPIOI, .pin = LL_GPIO_PIN_13};
const GpioPin gpio_sd_card_detect = {.port = GPIOI, .pin = LL_GPIO_PIN_14};
const GpioPin gpio_led = {.port = GPIOA, .pin = LL_GPIO_PIN_7};
const GpioPin gpio_log_usart_tx = {.port = GPIOA, .pin = LL_GPIO_PIN_9};

const GpioPin gpio_octospi1_psram_io0 = {.port = GPIOE, .pin = LL_GPIO_PIN_12};
const GpioPin gpio_octospi1_psram_io1 = {.port = GPIOE, .pin = LL_GPIO_PIN_13};
const GpioPin gpio_octospi1_psram_io2 = {.port = GPIOE, .pin = LL_GPIO_PIN_14};
const GpioPin gpio_octospi1_psram_io3 = {.port = GPIOE, .pin = LL_GPIO_PIN_15};
const GpioPin gpio_octospi1_psram_io4 = {.port = GPIOH, .pin = LL_GPIO_PIN_2};
const GpioPin gpio_octospi1_psram_io5 = {.port = GPIOD, .pin = LL_GPIO_PIN_5};
const GpioPin gpio_octospi1_psram_io6 = {.port = GPIOC, .pin = LL_GPIO_PIN_3};
const GpioPin gpio_octospi1_psram_io7 = {.port = GPIOC, .pin = LL_GPIO_PIN_4};
const GpioPin gpio_octospi1_psram_clk = {.port = GPIOE, .pin = LL_GPIO_PIN_10};
const GpioPin gpio_octospi1_psram_ncs = {.port = GPIOE, .pin = LL_GPIO_PIN_11};
const GpioPin gpio_octospi1_psram_dqs = {.port = GPIOG, .pin = LL_GPIO_PIN_6};

const GpioPin gpio_sd_card_d0 = {.port = GPIOC, .pin = LL_GPIO_PIN_8};
const GpioPin gpio_sd_card_d1 = {.port = GPIOC, .pin = LL_GPIO_PIN_9};
const GpioPin gpio_sd_card_d2 = {.port = GPIOC, .pin = LL_GPIO_PIN_10};
const GpioPin gpio_sd_card_d3 = {.port = GPIOC, .pin = LL_GPIO_PIN_11};
const GpioPin gpio_sd_card_ck = {.port = GPIOC, .pin = LL_GPIO_PIN_12};
const GpioPin gpio_sd_card_cmd = {.port = GPIOD, .pin = LL_GPIO_PIN_2};

static inline void furi_hal_nvic_enable(IRQn_Type irqn) {
    NVIC_SetPriority(irqn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 10, 0));
    NVIC_EnableIRQ(irqn);
}

void furi_hal_resources_init_early() {
    furi_hal_bus_enable(FuriHalBusGPIOA);
    furi_hal_bus_enable(FuriHalBusGPIOB);
    furi_hal_bus_enable(FuriHalBusGPIOC);
    furi_hal_bus_enable(FuriHalBusGPIOD);
    furi_hal_bus_enable(FuriHalBusGPIOE);
#if defined(GPIOF)
    furi_hal_bus_enable(FuriHalBusGPIOF);
#endif
    furi_hal_bus_enable(FuriHalBusGPIOG);
    furi_hal_bus_enable(FuriHalBusGPIOH);
#if defined(GPIOI)
    furi_hal_bus_enable(FuriHalBusGPIOI);
#endif
#if defined(GPIOJ)
    furi_hal_bus_enable(FuriHalBusGPIOJ);
#endif

    // SD Card stepdown control
    furi_hal_gpio_write(&gpio_sd_card_power_switch, 0);
    furi_hal_gpio_init(
        &gpio_sd_card_power_switch, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);

    // SD Card detect
    furi_hal_gpio_init(&gpio_sd_card_detect, GpioModeAnalog, GpioPullNo, GpioSpeedLow);

    // Master Clock Output
    // furi_hal_gpio_init_ex(
    //     &gpio_mco, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn0MCO);
    // LL_RCC_ConfigMCO(LL_RCC_MCO1SOURCE_SYSCLK, LL_RCC_MCO1_DIV_8);

    furi_hal_nvic_enable(EXTI0_IRQn);
    furi_hal_nvic_enable(EXTI1_IRQn);
    furi_hal_nvic_enable(EXTI2_IRQn);
    furi_hal_nvic_enable(EXTI3_IRQn);
    furi_hal_nvic_enable(EXTI4_IRQn);
    furi_hal_nvic_enable(EXTI5_IRQn);
    furi_hal_nvic_enable(EXTI6_IRQn);
    furi_hal_nvic_enable(EXTI7_IRQn);
    furi_hal_nvic_enable(EXTI8_IRQn);
    furi_hal_nvic_enable(EXTI9_IRQn);
    furi_hal_nvic_enable(EXTI10_IRQn);
    furi_hal_nvic_enable(EXTI11_IRQn);
    furi_hal_nvic_enable(EXTI12_IRQn);
    furi_hal_nvic_enable(EXTI13_IRQn);
    furi_hal_nvic_enable(EXTI14_IRQn);
    furi_hal_nvic_enable(EXTI15_IRQn);
}

void furi_hal_resources_deinit_early() {
    furi_hal_bus_disable(FuriHalBusGPIOA);
    furi_hal_bus_disable(FuriHalBusGPIOB);
    furi_hal_bus_disable(FuriHalBusGPIOC);
    furi_hal_bus_disable(FuriHalBusGPIOD);
    furi_hal_bus_disable(FuriHalBusGPIOE);
#if defined(GPIOF)
    furi_hal_bus_disable(FuriHalBusGPIOF);
#endif
    furi_hal_bus_disable(FuriHalBusGPIOG);
    furi_hal_bus_disable(FuriHalBusGPIOH);
#if defined(GPIOI)
    furi_hal_bus_disable(FuriHalBusGPIOI);
#endif
#if defined(GPIOJ)
    furi_hal_bus_disable(FuriHalBusGPIOJ);
#endif
}