#include "test_tim1_gdma_ch0.h"
#include "stm32u5xx_ll_dma.h"
#include "stm32u5xx_ll_bus.h"
#include "stm32u5xx_ll_tim.h"

//Example of initializing direct recording to port (PA7) in circular mode GDMA 1 channel 0, based on timer 1 overflow event. only LL is used

#define BUFF_SIZE 32
//TEST_PIN - PA7
uint32_t buff[BUFF_SIZE] = {0x00000080, 0x00800000, 0x00000080, 0x00800000, 0x00000080, 0x00800000,
                            0x00000080, 0x00800000, 0x00000080, 0x00800000, 0x00000080, 0x00800000,
                            0x00000080, 0x00800000, 0x00000080, 0x00800000, 0x00000080, 0x00800000,
                            0x00000080, 0x00800000, 0x00000080, 0x00800000, 0x00000080, 0x00800000,
                            0x00000080, 0x00800000, 0x00000080, 0x00800000, 0x00000080, 0x00800000,
                            0x00000080, 0x00800000};
#define TEST_PIN_BSRR (uint32_t) & (GPIOA->BSRR)

void TIM1_UP_IRQHandler(void) {
    /* Check whether update interrupt is pending */
    if(LL_TIM_IsActiveFlag_UPDATE(TIM1) == 1) {
        /* Clear the update interrupt flag */
        LL_TIM_ClearFlag_UPDATE(TIM1);
    }
}

void GPDMA1_Channel0_IRQHandler() {
    //	 /* Check if User Setting Error flag is active */
    //	  if (LL_DMA_IsActiveFlag_USE(GPDMA1, LL_DMA_CHANNEL_0) != 0U)
    //	  {
    //	    /* Clear User Setting Error flag */
    //	    LL_DMA_ClearFlag_USE(GPDMA1, LL_DMA_CHANNEL_0);
    //	  }
    //
    //	  /* Check if Update Link Error flag is active */
    //	  if (LL_DMA_IsActiveFlag_ULE(GPDMA1, LL_DMA_CHANNEL_0) != 0U)
    //	  {
    //	    /* Clear Update Link Error flag */
    //	    LL_DMA_ClearFlag_ULE(GPDMA1, LL_DMA_CHANNEL_0);
    //	  }
    //
    //	  /* Check if Data Transfer Error flag is active */
    //	  if (LL_DMA_IsActiveFlag_DTE(GPDMA1, LL_DMA_CHANNEL_0) != 0U)
    //	  {
    //	    /* Clear Data Transfer Error flag */
    //	    LL_DMA_ClearFlag_DTE(GPDMA1, LL_DMA_CHANNEL_0);
    //	  }

    /* Check if Transfer Complete flag is active */
    if(LL_DMA_IsActiveFlag_TC(GPDMA1, LL_DMA_CHANNEL_0) != 0U) {
        /* Clear Transfer Complete flag */
        LL_DMA_ClearFlag_TC(GPDMA1, LL_DMA_CHANNEL_0);
    }

    /* Check if Transfer Complete flag is active */
    if(LL_DMA_IsActiveFlag_HT(GPDMA1, LL_DMA_CHANNEL_0) != 0U) {
        /* Clear Half Transmit flag */
        LL_DMA_ClearFlag_HT(GPDMA1, LL_DMA_CHANNEL_0);
    }
}
static void tim1_dma_init(void) {
    /*
	   * ATTENTION!!!! Try to use PORT0 to work with IO ports, as PORT0 is directly connected to them bypassing the
	   * data bus. For GDMA to work to transfer data from one place to another, it is better to configure PORT1 and PORT0
	   *  to fit into 1 transaction. but all this is not necessary if you are not interested in speed
	   */

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPDMA1);
    LL_DMA_InitTypeDef DMA_InitStruct = {0};
    /*
	  * ATTENTION!!!! LinkRegis must be declared as a global variable. since GDMA constantly uses the data stored in it.
	  * 8 elements are enough for any GDMA operating mode, but sometimes less is possible depending on the node settings
	  */
    static __IO uint32_t LinkRegis[8];
    LinkRegis[0] = (uint32_t)&buff;

    DMA_InitStruct.SrcAddress = (uint32_t)buff;
    DMA_InitStruct.DestAddress = TEST_PIN_BSRR;
    DMA_InitStruct.BlkDataLength = (BUFF_SIZE * 4U);

    //Set REQUEST
    DMA_InitStruct.Request = LL_GPDMA1_REQUEST_TIM1_UP;

    DMA_InitStruct.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    DMA_InitStruct.BlkHWRequest = LL_DMA_HWREQUEST_SINGLEBURST;
    DMA_InitStruct.DataAlignment = LL_DMA_DATA_ALIGN_ZEROPADD;

    //Where do we read
    DMA_InitStruct.SrcAllocatedPort = LL_DMA_SRC_ALLOCATED_PORT1;
    DMA_InitStruct.SrcBurstLength = 1;
    DMA_InitStruct.SrcIncMode = LL_DMA_SRC_INCREMENT;
    DMA_InitStruct.SrcDataWidth = LL_DMA_SRC_DATAWIDTH_WORD;

    //Where do we write
    DMA_InitStruct.DestAllocatedPort = LL_DMA_DEST_ALLOCATED_PORT0;
    DMA_InitStruct.DestBurstLength = 1;
    DMA_InitStruct.DestIncMode = LL_DMA_DEST_FIXED;
    DMA_InitStruct.DestDataWidth = LL_DMA_DEST_DATAWIDTH_WORD;

    DMA_InitStruct.TriggerMode = LL_DMA_TRIGM_BLK_TRANSFER;
    DMA_InitStruct.TriggerPolarity = LL_DMA_TRIG_POLARITY_MASKED;
    DMA_InitStruct.TriggerSelection = 0x00000000U;

    //Interrupt at half and end of each block
    DMA_InitStruct.TransferEventMode = LL_DMA_TCEM_BLK_TRANSFER;

    DMA_InitStruct.Priority = LL_DMA_LOW_PRIORITY_LOW_WEIGHT;
    DMA_InitStruct.LinkAllocatedPort = LL_DMA_LINK_ALLOCATED_PORT1;
    DMA_InitStruct.LinkStepMode = LL_DMA_LSM_FULL_EXECUTION;
    DMA_InitStruct.LinkedListBaseAddr = (uint32_t)&LinkRegis;
    DMA_InitStruct.LinkedListAddrOffset = (uint32_t)&LinkRegis;
    LL_DMA_Init(GPDMA1, LL_DMA_CHANNEL_0, &DMA_InitStruct);
    LL_DMA_EnableCSARUpdate(GPDMA1, LL_DMA_CHANNEL_0);

    /* Enable DMA interrupts */
    //	  LL_DMA_EnableIT_USE(GPDMA1, LL_DMA_CHANNEL_0);
    //	  LL_DMA_EnableIT_ULE(GPDMA1, LL_DMA_CHANNEL_0);
    //	  LL_DMA_EnableIT_DTE(GPDMA1, LL_DMA_CHANNEL_0);
    LL_DMA_EnableIT_TC(GPDMA1, LL_DMA_CHANNEL_0);
    LL_DMA_EnableIT_HT(GPDMA1, LL_DMA_CHANNEL_0);

    /* DMA channel 0 interrupt init */
    NVIC_SetPriority(GPDMA1_Channel0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    /* Enable DMA channel */
    LL_DMA_EnableChannel(GPDMA1, LL_DMA_CHANNEL_0);
}

void test_pa7_tim1_gdma_ch0_init(void) {
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

    /* TIM1 interrupt Init */
    NVIC_SetPriority(TIM1_UP_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(TIM1_UP_IRQn);

    //In this example TIM1 input clock TIM1CLK is set to APB2 clock PCLK2 (TIM1CLK = PCLK2).
    //	As APB2 pre-scaler is equal to 1 PCLK2 = HCLK, and since AHB pre-scaler is equal to 1,
    //      => TIM1CLK = SystemCoreClock (160 MHz)
    TIM_InitStruct.Prescaler = __LL_TIM_CALC_PSC(SystemCoreClock, 10000);
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload =
        __LL_TIM_CALC_ARR(SystemCoreClock / 1, __LL_TIM_CALC_PSC(SystemCoreClock, 10000), 4);
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM1, &TIM_InitStruct);
    LL_TIM_DisableARRPreload(TIM1);
    LL_TIM_SetClockSource(TIM1, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM1, LL_TIM_TRGO_RESET);
    LL_TIM_SetTriggerOutput2(TIM1, LL_TIM_TRGO2_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM1);

    /* Clear the update flag */
    LL_TIM_ClearFlag_UPDATE(TIM1);

    /* Enable the update interrupt */
    //LL_TIM_EnableIT_UPDATE(TIM1);

    tim1_dma_init();
    //Enable the update Request
    LL_TIM_EnableDMAReq_UPDATE(TIM1);

    //LL_TIM_GenerateEvent_UPDATE(TIM1);

    /* Enable counter */
    LL_TIM_EnableCounter(TIM1);
    /* USER CODE END 2 */
}
