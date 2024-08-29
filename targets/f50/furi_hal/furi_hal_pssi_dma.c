#include "furi_hal_pssi_dma.h"
#include "stm32u5xx_ll_dma.h"
#include "furi_hal_bus.h"
#include "furi_hal_resources.h"
#include "furi_hal_gpio.h"
#include "furi_hal_interrupt.h"
#include "furi_hal_dma.h"

#include "furi_hal_pssi_def.h"
#include <stm32u5xx_safe.h>

#define FURI_HAL_PSSI_DATA_WIDTH           (FURI_HAL_PSSI_8BITS)
#define FURI_HAL_PSSI_BUS_DATA             (FURI_HAL_PSSI_8LINES)
#define FURI_HAL_PSSI_CONTROL_SIGNAL       (FURI_HAL_PSSI_DE_ENABLE)
#define FURI_HAL_PSSI_CLOCK_POLARITY       (FURI_HAL_PSSI_FALLING_EDGE)
#define FURI_HAL_PSSI_DATA_ENABLE_POLARITY (FURI_HAL_PSSI_DEPOL_ACTIVE_LOW)
#define FURI_HAL_PSSI_READY_POLARITY       (FURI_HAL_PSSI_RDYPOL_ACTIVE_LOW)

typedef struct {
    uint8_t* buf;
    uint16_t buf_size;
    uint32_t dma_link_reg[8];
    uint32_t dma_channel;
    FuriHalPssiRxCallback callback;
    void* context;
} FuriHalPssi;

static FuriHalPssi furi_hal_pssi = {0};

static void furi_hal_pssi_dma_pin_init(void) {
    furi_hal_gpio_init_ex(
        &gpio_pssi_pdck,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_rdy,
        GpioModeAltFunctionPushPull,
        GpioPullNo,
        GpioSpeedVeryHigh,
        GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_de, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d0, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d1, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d2, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d3, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d4, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d5, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d6, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
    furi_hal_gpio_init_ex(
        &gpio_pssi_d7, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedVeryHigh, GpioAltFn10PSSI);
}

static void furi_hal_pssi_dma_pin_deinit(void) {
    furi_hal_gpio_init(&gpio_pssi_pdck, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_rdy, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_de, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d0, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d1, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d2, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d3, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d4, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d5, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d6, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
    furi_hal_gpio_init(&gpio_pssi_d7, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
}

void furi_hal_pssi_dma_isr(void* context) {
    UNUSED(context);
    //Check if User Setting Error flag is active
    if(LL_DMA_IsActiveFlag_USE(GPDMA1, furi_hal_pssi.dma_channel) != 0U) {
        //Clear User Setting Error flag
        LL_DMA_ClearFlag_USE(GPDMA1, furi_hal_pssi.dma_channel);
        furi_crash("PSSI: GPDMA User Setting Error");
    }

    //Check if Update Link Error flag is active
    if(LL_DMA_IsActiveFlag_ULE(GPDMA1, furi_hal_pssi.dma_channel) != 0U) {
        //Clear Update Link Error flag
        LL_DMA_ClearFlag_ULE(GPDMA1, furi_hal_pssi.dma_channel);
        furi_crash("PSSI: GPDMA Update Link Error");
    }

    //Check if Data Transfer Error flag is active
    if(LL_DMA_IsActiveFlag_DTE(GPDMA1, furi_hal_pssi.dma_channel) != 0U) {
        //Clear Data Transfer Error flag
        LL_DMA_ClearFlag_DTE(GPDMA1, furi_hal_pssi.dma_channel);
        furi_crash("PSSI: GPDMA Data Transfer Error");
    }

    //Check if Transfer Complete flag is active
    if(LL_DMA_IsActiveFlag_TC(GPDMA1, furi_hal_pssi.dma_channel) != 0U) {
        //Clear Transfer Complete flag
        LL_DMA_ClearFlag_TC(GPDMA1, furi_hal_pssi.dma_channel);
        if(furi_hal_pssi.callback) {
            furi_hal_pssi.callback(
                furi_hal_pssi.buf + furi_hal_pssi.buf_size / 2,
                furi_hal_pssi.buf_size / 2,
                furi_hal_pssi.context);
        }
    }

    //Check if Transfer Complete flag is active
    if(LL_DMA_IsActiveFlag_HT(GPDMA1, furi_hal_pssi.dma_channel) != 0U) {
        //Clear Half Transmit flag
        LL_DMA_ClearFlag_HT(GPDMA1, furi_hal_pssi.dma_channel);
        if(furi_hal_pssi.callback) {
            furi_hal_pssi.callback(
                furi_hal_pssi.buf, furi_hal_pssi.buf_size / 2, furi_hal_pssi.context);
        }
    }
}

static void furi_hal_pssi_dma_init(void) {
    LL_DMA_InitTypeDef dma_init_strust = {0};

    furi_hal_pssi.dma_link_reg[0] = (uint32_t)furi_hal_pssi.buf;

    dma_init_strust.SrcAddress = (uint32_t)&PSSI->DR;
    dma_init_strust.DestAddress = (uint32_t)furi_hal_pssi.buf;
    dma_init_strust.BlkDataLength = furi_hal_pssi.buf_size;
    dma_init_strust.Request = LL_GPDMA1_REQUEST_DCMI_PSSI;

    dma_init_strust.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    dma_init_strust.BlkHWRequest = LL_DMA_HWREQUEST_SINGLEBURST;
    dma_init_strust.DataAlignment = LL_DMA_DATA_ALIGN_ZEROPADD;

    dma_init_strust.SrcAllocatedPort = LL_DMA_SRC_ALLOCATED_PORT1;
    dma_init_strust.SrcBurstLength = 1;
    dma_init_strust.SrcIncMode = LL_DMA_SRC_FIXED;
    dma_init_strust.SrcDataWidth = LL_DMA_SRC_DATAWIDTH_BYTE;

    dma_init_strust.DestAllocatedPort = LL_DMA_DEST_ALLOCATED_PORT0;
    dma_init_strust.DestBurstLength = 1;
    dma_init_strust.DestIncMode = LL_DMA_DEST_INCREMENT;
    dma_init_strust.DestDataWidth = LL_DMA_DEST_DATAWIDTH_BYTE;

    dma_init_strust.TriggerMode = LL_DMA_TRIGM_BLK_TRANSFER;
    dma_init_strust.TriggerPolarity = LL_DMA_TRIG_POLARITY_MASKED;
    dma_init_strust.TriggerSelection = 0x00000000U;

    dma_init_strust.TransferEventMode = LL_DMA_TCEM_BLK_TRANSFER;

    dma_init_strust.Priority = LL_DMA_LOW_PRIORITY_LOW_WEIGHT;
    dma_init_strust.LinkAllocatedPort = LL_DMA_LINK_ALLOCATED_PORT0;
    dma_init_strust.LinkStepMode = LL_DMA_LSM_FULL_EXECUTION;
    dma_init_strust.LinkedListBaseAddr = (uint32_t)&furi_hal_pssi.dma_link_reg[0];
    dma_init_strust.LinkedListAddrOffset = (uint32_t)&furi_hal_pssi.dma_link_reg[0];
    LL_DMA_Init(GPDMA1, furi_hal_pssi.dma_channel, &dma_init_strust);
    LL_DMA_EnableCDARUpdate(GPDMA1, furi_hal_pssi.dma_channel);
    //LL_DMA_EnableCSARUpdate(GPDMA1,furi_hal_pssi.dma_channel);

    //if needed to enable DMA interrupts
    LL_DMA_EnableIT_USE(GPDMA1, furi_hal_pssi.dma_channel);
    LL_DMA_EnableIT_ULE(GPDMA1, furi_hal_pssi.dma_channel);
    LL_DMA_EnableIT_DTE(GPDMA1, furi_hal_pssi.dma_channel);

    LL_DMA_EnableIT_TC(GPDMA1, furi_hal_pssi.dma_channel);
    LL_DMA_EnableIT_HT(GPDMA1, furi_hal_pssi.dma_channel);

    //DMA1_Channel1_IRQn interrupt configuration
    furi_hal_interrupt_set_isr(
        furi_hal_dma_get_gpdma_interrupt_id(furi_hal_pssi.dma_channel),
        furi_hal_pssi_dma_isr,
        NULL);

    //Start DMA Channel
    LL_DMA_EnableChannel(GPDMA1, furi_hal_pssi.dma_channel);
}

void furi_hal_pssi_set_rx_callback(FuriHalPssiRxCallback callback, void* context) {
    furi_hal_pssi.callback = callback;
    furi_hal_pssi.context = context;
}

bool furi_hal_pssi_init_bus8line(uint16_t buf_size) {
    furi_hal_pssi.buf = malloc(buf_size);

    if(!furi_hal_dma_allocate_gpdma_channel(&furi_hal_pssi.dma_channel)) {
        return false;
    }
    furi_hal_pssi.buf_size = buf_size;

    //PSSI Init pin
    furi_hal_pssi_dma_pin_init();

    //PSSI bus init
    furi_hal_bus_enable(FuriHalBusDCMI_PSSI);

    //PSSI Init DMA
    furi_hal_pssi_dma_init();

    //PSSI disable
    PSSI->CR &= (~PSSI_CR_ENABLE);

    //Configure PSSI Control Signal and Bus Width
    MODIFY_REG(
        PSSI->CR,
        PSSI_CR_DERDYCFG | PSSI_CR_EDM | PSSI_CR_DEPOL | PSSI_CR_RDYPOL,
        FURI_HAL_PSSI_CONTROL_SIGNAL | FURI_HAL_PSSI_DATA_ENABLE_POLARITY |
            FURI_HAL_PSSI_READY_POLARITY | FURI_HAL_PSSI_BUS_DATA);
    return true;
}

void furi_hal_pssi_deinit(void) {
    //Stop GDMA
    LL_DMA_DisableChannel(GPDMA1, furi_hal_pssi.dma_channel);
    furi_hal_interrupt_set_isr(
        furi_hal_dma_get_gpdma_interrupt_id(furi_hal_pssi.dma_channel), NULL, NULL);

    //PSSI bus disable
    furi_hal_bus_disable(FuriHalBusDCMI_PSSI);

    //PSSI deinit pin
    furi_hal_pssi_dma_pin_deinit();

    //Free GPDMA channel
    furi_hal_dma_free_gpdma_channel(furi_hal_pssi.dma_channel);

    free(furi_hal_pssi.buf);
}

void furi_hal_pssi_dma_receive_stop(void) {
    //PSSI disable DMA Request
    PSSI->CR |= FURI_HAL_PSSI_CR_DMA_ENABLE;

    //Stop GDMA Channel
    LL_DMA_DisableChannel(GPDMA1, furi_hal_pssi.dma_channel);

    //PSSI disable
    PSSI->CR &= (~PSSI_CR_ENABLE);
}

void furi_hal_pssi_dma_receive_start(void) {
    furi_check(!(READ_BIT(PSSI->CR, PSSI_CR_ENABLE)), "PSSI is already enabled");

#if(FURI_HAL_PSSI_BUS_DATA == FURI_HAL_PSSI_8LINES)
    MODIFY_REG(
        PSSI->CR,
        PSSI_CR_DMAEN | PSSI_CR_OUTEN | PSSI_CR_CKPOL,
        FURI_HAL_PSSI_CR_DMA_ENABLE |
            ((FURI_HAL_PSSI_CLOCK_POLARITY == FURI_HAL_PSSI_RISING_EDGE) ? PSSI_CR_CKPOL : 0U));
#else
    MODIFY_REG(
        PSSI->CR,
        PSSI_CR_DMAEN | PSSI_CR_OUTEN | PSSI_CR_CKPOL,
        FURI_HAL_PSSI_CR_DMA_ENABLE | FURI_HAL_PSSI_BUS_DATA |
            ((FURI_HAL_PSSI_CLOCK_POLARITY == FURI_HAL_PSSI_RISING_EDGE) ? PSSI_CR_CKPOL : 0U));
#endif

    //PSSI enable ERR interrupt (Overrun) if needed
    PSSI->IER |= FURI_HAL_PSSI_FLAG_OVR_RIS;

    //PSSI enable DMA Request
    //PSSI->CR |= FURI_HAL_PSSI_CR_DMA_ENABLE;

    //PSSI enable
    PSSI->CR |= PSSI_CR_ENABLE;
}
