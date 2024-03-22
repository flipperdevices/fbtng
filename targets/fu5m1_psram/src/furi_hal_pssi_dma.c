#include "furi_hal_pssi_dma.h"
#include "stm32u5xx_ll_gpio.h"
#include "stm32u5xx_ll_dma.h"
#include "stm32u5xx_ll_bus.h"

#include <stdlib.h>

#include "furi_hal_pssi_def.h"

#define FURI_HAL_PSSI_DATA_WIDTH FURI_HAL_PSSI_8BITS
#define FURI_HAL_PSSI_BUS_DATA FURI_HAL_PSSI_8LINES
#define FURI_HAL_PSSI_CONTROL_SIGNAL FURI_HAL_PSSI_DE_ENABLE
#define FURI_HAL_PSSI_CLOCK_POLARITY FURI_HAL_PSSI_FALLING_EDGE
#define FURI_HAL_PSSI_DATA_ENABLE_POLARITY FURI_HAL_PSSI_DEPOL_ACTIVE_LOW
#define FURI_HAL_PSSI_READY_POLARITY FURI_HAL_PSSI_RDYPOL_ACTIVE_LOW

typedef struct {
    uint8_t* buf;
    uint16_t buf_size;
    FuriHalPssiRxCallback callback;
    void* context;
} FuriHalPssi;

static FuriHalPssi furi_hal_pssi = {0};

void GPDMA1_Channel1_IRQHandler() {
    //	 /* Check if User Setting Error flag is active */
    //	  if (LL_DMA_IsActiveFlag_USE(GPDMA1, LL_DMA_CHANNEL_1) != 0U)
    //	  {
    //	    /* Clear User Setting Error flag */
    //	    LL_DMA_ClearFlag_USE(GPDMA1, LL_DMA_CHANNEL_1);
    //	  }
    //
    //	  /* Check if Update Link Error flag is active */
    //	  if (LL_DMA_IsActiveFlag_ULE(GPDMA1, LL_DMA_CHANNEL_1) != 0U)
    //	  {
    //	    /* Clear Update Link Error flag */
    //	    LL_DMA_ClearFlag_ULE(GPDMA1, LL_DMA_CHANNEL_1);
    //	  }
    //
    //	  /* Check if Data Transfer Error flag is active */
    //	  if (LL_DMA_IsActiveFlag_DTE(GPDMA1, LL_DMA_CHANNEL_1) != 0U)
    //	  {
    //	    /* Clear Data Transfer Error flag */
    //	    LL_DMA_ClearFlag_DTE(GPDMA1, LL_DMA_CHANNEL_1);
    //	  }

    /* Check if Transfer Complete flag is active */
    if(LL_DMA_IsActiveFlag_TC(GPDMA1, LL_DMA_CHANNEL_1) != 0U) {
        /* Clear Transfer Complete flag */
        LL_DMA_ClearFlag_TC(GPDMA1, LL_DMA_CHANNEL_1);
        if(furi_hal_pssi.callback) {
            furi_hal_pssi.callback(
                furi_hal_pssi.buf + furi_hal_pssi.buf_size / 2,
                furi_hal_pssi.buf_size / 2,
                furi_hal_pssi.context);
        }
    }

    /* Check if Transfer Complete flag is active */
    if(LL_DMA_IsActiveFlag_HT(GPDMA1, LL_DMA_CHANNEL_1) != 0U) {
        /* Clear Half Transmit flag */
        LL_DMA_ClearFlag_HT(GPDMA1, LL_DMA_CHANNEL_1);
        if(furi_hal_pssi.callback) {
            furi_hal_pssi.callback(
                furi_hal_pssi.buf, furi_hal_pssi.buf_size / 2, furi_hal_pssi.context);
        }
    }
}
static void furi_hal_pssi_dma_init(void) {
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPDMA1);

    LL_DMA_InitTypeDef DMA_InitStruct = {0};
    static __IO uint32_t LinkRegisters[8];
    LinkRegisters[0] = (uint32_t)furi_hal_pssi.buf;

    DMA_InitStruct.SrcAddress = (uint32_t)&PSSI->DR;
    DMA_InitStruct.DestAddress = (uint32_t)furi_hal_pssi.buf;
    DMA_InitStruct.BlkDataLength = furi_hal_pssi.buf_size;
    DMA_InitStruct.Request = LL_GPDMA1_REQUEST_DCMI_PSSI;

    DMA_InitStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
    DMA_InitStruct.BlkHWRequest = LL_DMA_HWREQUEST_SINGLEBURST;
    DMA_InitStruct.DataAlignment = LL_DMA_DATA_ALIGN_ZEROPADD;

    DMA_InitStruct.SrcAllocatedPort = LL_DMA_SRC_ALLOCATED_PORT1;
    DMA_InitStruct.SrcBurstLength = 1;
    DMA_InitStruct.SrcIncMode = LL_DMA_SRC_FIXED;
    DMA_InitStruct.SrcDataWidth = LL_DMA_SRC_DATAWIDTH_BYTE;

    DMA_InitStruct.DestAllocatedPort = LL_DMA_DEST_ALLOCATED_PORT0;
    DMA_InitStruct.DestBurstLength = 1;
    DMA_InitStruct.DestIncMode = LL_DMA_DEST_INCREMENT;
    DMA_InitStruct.DestDataWidth = LL_DMA_DEST_DATAWIDTH_BYTE;

    DMA_InitStruct.TriggerMode = LL_DMA_TRIGM_BLK_TRANSFER;
    DMA_InitStruct.TriggerPolarity = LL_DMA_TRIG_POLARITY_MASKED;
    DMA_InitStruct.TriggerSelection = 0x00000000U;

    DMA_InitStruct.TransferEventMode = LL_DMA_TCEM_BLK_TRANSFER;

    DMA_InitStruct.Priority = LL_DMA_LOW_PRIORITY_LOW_WEIGHT;
    DMA_InitStruct.LinkAllocatedPort = LL_DMA_LINK_ALLOCATED_PORT0;
    DMA_InitStruct.LinkStepMode = LL_DMA_LSM_FULL_EXECUTION;
    DMA_InitStruct.LinkedListBaseAddr = (uint32_t)&LinkRegisters;
    DMA_InitStruct.LinkedListAddrOffset = (uint32_t)&LinkRegisters;
    LL_DMA_Init(GPDMA1, LL_DMA_CHANNEL_1, &DMA_InitStruct);
    LL_DMA_EnableCDARUpdate(GPDMA1, LL_DMA_CHANNEL_1);
    //LL_DMA_EnableCSARUpdate(GPDMA1,LL_DMA_CHANNEL_1);

    /* Enable DMA interrupts */
    //	  LL_DMA_EnableIT_USE(GPDMA1, LL_DMA_CHANNEL_1);
    //	  LL_DMA_EnableIT_ULE(GPDMA1, LL_DMA_CHANNEL_1);
    //	  LL_DMA_EnableIT_DTE(GPDMA1, LL_DMA_CHANNEL_1);
    LL_DMA_EnableIT_TC(GPDMA1, LL_DMA_CHANNEL_1);
    LL_DMA_EnableIT_HT(GPDMA1, LL_DMA_CHANNEL_1);

    /* DMA channel 0 interrupt init */
    NVIC_SetPriority(GPDMA1_Channel1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);

    //Start DMA Channel
    LL_DMA_EnableChannel(GPDMA1, LL_DMA_CHANNEL_1);
}

void furi_hal_pssi_set_rx_callback(FuriHalPssiRxCallback callback, void* context) {
    furi_hal_pssi.callback = callback;
    furi_hal_pssi.context = context;
}

void furi_hal_pssi_init_bus8line(uint16_t buf_size) {
    furi_hal_pssi.buf = malloc(buf_size);
    furi_hal_pssi.buf_size = buf_size;

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOH);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOI);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOF);

    /**PSSI GPIO Configuration
        PH5     ------> PSSI_PDCK
        PI5     ------> PSSI_RDY
        PI7     ------> PSSI_D7
        PI6     ------> PSSI_D6
        PH8     ------> PSSI_DE
        PI4     ------> PSSI_D5
        PI1     ------> PSSI_D8
        PI2     ------> PSSI_D9
        PI3     ------> PSSI_D10
        PH14     ------> PSSI_D4
        PH15     ------> PSSI_D11
        PI0     ------> PSSI_D13
        PF9     ------> PSSI_D15
        PH10     ------> PSSI_D1
        PH11     ------> PSSI_D2
        PF6     ------> PSSI_D12
        PH9     ------> PSSI_D0
        PH12     ------> PSSI_D3
        */
    LL_GPIO_InitTypeDef pssi_gpio = {0};
    pssi_gpio.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_8 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15 |
                    LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_9 | LL_GPIO_PIN_12;
    pssi_gpio.Mode = LL_GPIO_MODE_ALTERNATE;
    pssi_gpio.Pull = LL_GPIO_PULL_NO;
    pssi_gpio.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    pssi_gpio.Alternate = LL_GPIO_AF_10;
    LL_GPIO_Init(GPIOH, &pssi_gpio);

    pssi_gpio.Pin = LL_GPIO_PIN_5 | LL_GPIO_PIN_7 | LL_GPIO_PIN_6 | LL_GPIO_PIN_4 | LL_GPIO_PIN_1 |
                    LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_0;
    pssi_gpio.Mode = LL_GPIO_MODE_ALTERNATE;
    pssi_gpio.Pull = LL_GPIO_PULL_NO;
    pssi_gpio.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    pssi_gpio.Alternate = LL_GPIO_AF_10;
    LL_GPIO_Init(GPIOI, &pssi_gpio);

    pssi_gpio.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_6;
    pssi_gpio.Mode = LL_GPIO_MODE_ALTERNATE;
    pssi_gpio.Pull = LL_GPIO_PULL_NO;
    pssi_gpio.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    pssi_gpio.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOF, &pssi_gpio);

    //PSSI Init
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_DCMI_PSSI);

    // HAL_PSSI_DISABLE
    PSSI->CR &= (~PSSI_CR_ENABLE);
    /*---------------------------- PSSIx CR Configuration ----------------------*/
    /* Configure PSSIx: Control Signal and Bus Width*/
    MODIFY_REG(
        PSSI->CR,
        PSSI_CR_DERDYCFG | PSSI_CR_EDM | PSSI_CR_DEPOL | PSSI_CR_RDYPOL,
        FURI_HAL_PSSI_CONTROL_SIGNAL | FURI_HAL_PSSI_DATA_ENABLE_POLARITY |
            FURI_HAL_PSSI_READY_POLARITY | FURI_HAL_PSSI_BUS_DATA);
}

void furi_hal_pssi_deinit(void) {
    free(furi_hal_pssi.buf);

    //Stop GDMA
    LL_DMA_DisableChannel(GPDMA1, LL_DMA_CHANNEL_1);

    LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_DCMI_PSSI);

    /**PSSI GPIO Configuration
    PH5     ------> PSSI_PDCK
    PI5     ------> PSSI_RDY
    PI7     ------> PSSI_D7
    PI6     ------> PSSI_D6
    PH8     ------> PSSI_DE
    PI4     ------> PSSI_D5
    PI1     ------> PSSI_D8
    PI2     ------> PSSI_D9
    PI3     ------> PSSI_D10
    PH14     ------> PSSI_D4
    PH15     ------> PSSI_D11
    PI0     ------> PSSI_D13
    PF9     ------> PSSI_D15
    PH10     ------> PSSI_D1
    PH11     ------> PSSI_D2
    PF6     ------> PSSI_D12
    PH9     ------> PSSI_D0
    PH12     ------> PSSI_D3
    */
    //    HAL_GPIO_DeInit(GPIOH, GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_14|GPIO_PIN_15
    //                          |GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_9|GPIO_PIN_12);
    //
    //    HAL_GPIO_DeInit(GPIOI, GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_6|GPIO_PIN_4
    //                          |GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_0);
    //
    //    HAL_GPIO_DeInit(GPIOF, GPIO_PIN_9|GPIO_PIN_6);
}

void furi_hal_pssi_dma_receve_stop(void) {
    /* Diabse DMA Request */
    PSSI->CR |= PSSI_CR_DMA_ENABLE;

    //Stop GDMA
    LL_DMA_DisableChannel(GPDMA1, LL_DMA_CHANNEL_1);

    // HAL_PSSI_DISABLE
    PSSI->CR &= (~PSSI_CR_ENABLE);
}

void furi_hal_pssi_dma_receve_start(void) {
    /* Disable the selected PSSI peripheral */
    // HAL_PSSI_DISABLE
    PSSI->CR &= (~PSSI_CR_ENABLE);

#if(FURI_HAL_PSSI_BUS_DATA == FURI_HAL_PSSI_8LINES)
    MODIFY_REG(
        PSSI->CR,
        PSSI_CR_DMAEN | PSSI_CR_OUTEN | PSSI_CR_CKPOL,
        PSSI_CR_DMA_ENABLE |
            ((FURI_HAL_PSSI_CLOCK_POLARITY == HAL_PSSI_RISING_EDGE) ? PSSI_CR_CKPOL : 0U));
#else
    MODIFY_REG(
        PSSI->CR,
        PSSI_CR_DMAEN | PSSI_CR_OUTEN | PSSI_CR_CKPOL,
        PSSI_CR_DMA_ENABLE | FURI_HAL_PSSI_BUS_DATA |
            ((FURI_HAL_PSSI_CLOCK_POLARITY == HAL_PSSI_RISING_EDGE) ? PSSI_CR_CKPOL : 0U));
#endif

    furi_hal_pssi_dma_init();

    /* Enable ERR  interrupt */
    //HAL_PSSI_ENABLE_IT(hpssi, PSSI_FLAG_OVR_RIS);
    //PSSI->IER |=PSSI_FLAG_OVR_RIS;

    /* Enable DMA Request */
    PSSI->CR |= PSSI_CR_DMA_ENABLE;

    /* Enable the selected PSSI peripheral */
    //HAL_PSSI_ENABLE(hpssi);
    PSSI->CR |= PSSI_CR_ENABLE;
}
