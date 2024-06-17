#include "furi_hal_serial.h"
#include "furi_hal_serial_types_i.h"

#include "furi_hal_resources.h"
#include "furi_hal_interrupt.h"
#include "furi_hal_bus.h"
#include "furi_hal_dma.h"

#include "stm32u5xx_ll_rcc.h"
#include "stm32u5xx_ll_usart.h"
#include "stm32u5xx_ll_dma.h"

#define TAG "Serial"

#define FURI_HAL_SERIAL_DEFALUT_DMA_RX_BUF_SIZE (256UL)

typedef struct {
    uint32_t baudrate;
    uint32_t dma_rx_channel;
    uint32_t dma_tx_channel;

    bool dma_tx_use;

    bool dma_rx_node_half_buf;
    LL_DMA_LinkNodeTypeDef dma_rx_link_node[2];
    uint16_t rx_dma_buf_index_write;
    uint16_t rx_dma_buf_index_read;
    uint16_t rx_dma_buf_count_bytes;
    uint16_t rx_dma_buf_size;
    uint16_t rx_dma_buf_size_half;
    uint16_t rx_dma_buf_size_free;
    uint8_t* rx_dma_buf_ptr;

    FuriHalSerialHandle* handle;
    USART_TypeDef* periph_ptr;
    FuriHalSerialDmaAsyncRxCallback rx_callback;
    FuriHalSerialDmaAsyncTxCallback tx_callback;
    void* context;
} FuriHalSerial;

typedef struct {
    USART_TypeDef* periph;
    GpioAltFn alt_fn;
    const GpioPin* gpio[FuriHalSerialPinMax];
    uint32_t tx_dma_request;
    uint32_t rx_dma_request;
} FuriHalSerialConfig;

static const FuriHalSerialConfig furi_hal_serial_config[FuriHalSerialIdMax] = {
    [FuriHalSerialIdUsart1] =
        {
            .periph = USART1,
            .alt_fn = GpioAltFn7USART1,
            .gpio =
                {
                    [FuriHalSerialPinTx] = &gpio_log_usart_tx,
                    [FuriHalSerialPinRx] = &gpio_log_usart_rx,
                },
            .tx_dma_request = LL_GPDMA1_REQUEST_USART1_TX,
            .rx_dma_request = LL_GPDMA1_REQUEST_USART1_RX,
        },
    [FuriHalSerialIdUsart2] =
        {
            .periph = USART2,
            .alt_fn = GpioAltFn7USART2,
            .gpio =
                {
                    [FuriHalSerialPinTx] = NULL,
                    [FuriHalSerialPinRx] = NULL,
                },
            .tx_dma_request = LL_GPDMA1_REQUEST_USART2_TX,
            .rx_dma_request = LL_GPDMA1_REQUEST_USART2_RX,
        },
    [FuriHalSerialIdUsart3] =
        {
            .periph = USART3,
            .alt_fn = GpioAltFn7USART3,
            .gpio =
                {
                    [FuriHalSerialPinTx] = NULL,
                    [FuriHalSerialPinRx] = NULL,
                },
            .tx_dma_request = LL_GPDMA1_REQUEST_USART3_TX,
            .rx_dma_request = LL_GPDMA1_REQUEST_USART3_RX,
        },
    [FuriHalSerialIdUart4] =
        {
            .periph = UART4,
            .alt_fn = GpioAltFn8UART4,
            .gpio =
                {
                    [FuriHalSerialPinTx] = NULL,
                    [FuriHalSerialPinRx] = NULL,
                },
            .tx_dma_request = LL_GPDMA1_REQUEST_UART4_TX,
            .rx_dma_request = LL_GPDMA1_REQUEST_UART4_RX,
        },
    [FuriHalSerialIdUart5] =
        {
            .periph = UART5,
            .alt_fn = GpioAltFn8UART5,
            .gpio =
                {
                    [FuriHalSerialPinTx] = &gpio_uart5_tx,
                    [FuriHalSerialPinRx] = &gpio_uart5_rx,
                },
            .tx_dma_request = LL_GPDMA1_REQUEST_UART5_TX,
            .rx_dma_request = LL_GPDMA1_REQUEST_UART5_RX,
        },
    [FuriHalSerialIdUsart6] =
        {
            .periph = USART6,
            .alt_fn = GpioAltFn7USART6,
            .gpio =
                {
                    [FuriHalSerialPinTx] = NULL,
                    [FuriHalSerialPinRx] = NULL,
                },
            .tx_dma_request = LL_GPDMA1_REQUEST_USART6_TX,
            .rx_dma_request = LL_GPDMA1_REQUEST_USART6_RX,
        },
};

static FuriHalSerial* furi_hal_serial[FuriHalSerialIdMax] = {0};

static inline void furi_hal_serial_check(FuriHalSerialHandle* handle) {
    furi_check(handle, "Serial: handle is NULL");
    furi_assert(furi_hal_serial[handle->id], "Serial: handle is not initialized");
}

static size_t fufuri_hal_serial_dma_bytes_available(FuriHalSerialId ch) {
    furi_assert(ch);
    size_t rx_dma_buf_count_bytes = 0;
    size_t dma_remain = LL_DMA_GetBlkDataLength(GPDMA1, furi_hal_serial[ch]->dma_rx_channel);

    furi_hal_serial[ch]->rx_dma_buf_index_write =
        (furi_hal_serial[ch]->rx_dma_buf_size_half - dma_remain);
    if(furi_hal_serial[ch]->dma_rx_node_half_buf) {
        furi_hal_serial[ch]->rx_dma_buf_index_write += furi_hal_serial[ch]->rx_dma_buf_size_half;
    }
    if(furi_hal_serial[ch]->rx_dma_buf_index_write >= furi_hal_serial[ch]->rx_dma_buf_index_read) {
        rx_dma_buf_count_bytes = furi_hal_serial[ch]->rx_dma_buf_index_write -
                                 furi_hal_serial[ch]->rx_dma_buf_index_read;
    } else {
        rx_dma_buf_count_bytes = furi_hal_serial[ch]->rx_dma_buf_size -
                                 furi_hal_serial[ch]->rx_dma_buf_index_read +
                                 furi_hal_serial[ch]->rx_dma_buf_index_write;
    }

    if(furi_hal_serial[ch]->rx_dma_buf_count_bytes > rx_dma_buf_count_bytes) {
        if(furi_hal_serial[ch]->rx_callback) {
            furi_hal_serial[ch]->rx_callback(
                furi_hal_serial[ch]->handle,
                FuriHalSerialEventOverrunRxFifoDmaError,
                0,
                furi_hal_serial[ch]->context);
        } else {
            furi_crash("Serial: RX DMA buffer overrun");
        }
    }

    furi_hal_serial[ch]->rx_dma_buf_count_bytes = rx_dma_buf_count_bytes;
    return rx_dma_buf_count_bytes;
}

static void furi_hal_serial_irq(void* context) {
    FuriHalSerialHandle* handle = context;
    FuriHalSerialEvent event = 0;

    // Notification flags
    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_RXNE_RXFNE) {
        event |= FuriHalSerialEventData;
    }

    //won't turn off IDLE interrupt
    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_IDLE) {
        furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_IDLECF;
        //event |= FuriHalSerialEventIdle;
    }

    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_RTOF) {
        furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_RTOCF;
        event |= FuriHalSerialEventIdle;
    }

    // Error flags
    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_ORE) {
        furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_ORECF;
        event |= FuriHalSerialEventOverrunRxFifoError;
    }

    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_NE) {
        furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_NECF;
        event |= FuriHalSerialEventNoiseError;
    }

    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_PE) {
        furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_PECF;
        event |= FuriHalSerialEventParityError;
    }

    if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_FE) {
        furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_FECF;
        event |= FuriHalSerialEventFrameError;
    }

    if(event) {
        size_t len = fufuri_hal_serial_dma_bytes_available(handle->id);
        if(len && furi_hal_serial[handle->id]->rx_callback) {
            furi_hal_serial[handle->id]->rx_callback(
                furi_hal_serial[handle->id]->handle,
                event,
                len,
                furi_hal_serial[handle->id]->context);
        } else {
            //clear USART_ISR_RXNE_RXFNE flag
            LL_USART_ReceiveData8(furi_hal_serial[handle->id]->periph_ptr);
        }
    } else {
        if(furi_hal_serial[handle->id]->periph_ptr->ISR & USART_ISR_TC) {
            furi_hal_serial[handle->id]->periph_ptr->ICR = USART_ICR_TCCF;
            LL_USART_DisableIT_TC(furi_hal_serial[handle->id]->periph_ptr);
            LL_DMA_DisableChannel(GPDMA1, furi_hal_serial[handle->id]->dma_tx_channel);
            if(furi_hal_serial[handle->id]->tx_callback) {
                furi_hal_serial[handle->id]->tx_callback(
                    furi_hal_serial[handle->id]->handle,
                    FuriHalSerialDmaTxEventComplete,
                    furi_hal_serial[handle->id]->context);
            }
        }
    }
}

static void furi_hal_serial_dma_tx_init(FuriHalSerialHandle* handle) {
    furi_assert(handle);
    LL_DMA_InitTypeDef dma_init_strust = {0};

    dma_init_strust.SrcAddress = 0x00000000U;
    dma_init_strust.DestAddress = LL_USART_DMA_GetRegAddr(
        furi_hal_serial[handle->id]->periph_ptr, LL_USART_DMA_REG_DATA_TRANSMIT);
    dma_init_strust.BlkDataLength = 0x00000000U;
    dma_init_strust.Request = furi_hal_serial_config[handle->id].tx_dma_request;

    dma_init_strust.Direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH;
    dma_init_strust.BlkHWRequest = LL_DMA_HWREQUEST_SINGLEBURST;
    dma_init_strust.DataAlignment = LL_DMA_DATA_ALIGN_ZEROPADD;

    dma_init_strust.SrcAllocatedPort = LL_DMA_SRC_ALLOCATED_PORT1;
    dma_init_strust.SrcBurstLength = 1;
    dma_init_strust.SrcIncMode = LL_DMA_SRC_INCREMENT;
    dma_init_strust.SrcDataWidth = LL_DMA_SRC_DATAWIDTH_BYTE;

    dma_init_strust.DestAllocatedPort = LL_DMA_DEST_ALLOCATED_PORT0;
    dma_init_strust.DestBurstLength = 1;
    dma_init_strust.DestIncMode = LL_DMA_DEST_FIXED;
    dma_init_strust.DestDataWidth = LL_DMA_DEST_DATAWIDTH_BYTE;

    dma_init_strust.TriggerMode = LL_DMA_TRIGM_BLK_TRANSFER;
    dma_init_strust.TriggerPolarity = LL_DMA_TRIG_POLARITY_MASKED;
    dma_init_strust.TriggerSelection = 0x00000000U;

    dma_init_strust.TransferEventMode = LL_DMA_TCEM_BLK_TRANSFER;

    dma_init_strust.Priority = LL_DMA_LOW_PRIORITY_MID_WEIGHT;
    dma_init_strust.LinkAllocatedPort = LL_DMA_LINK_ALLOCATED_PORT1;
    dma_init_strust.LinkStepMode = LL_DMA_LSM_FULL_EXECUTION;
    dma_init_strust.LinkedListBaseAddr = 0x00000000U;
    dma_init_strust.LinkedListAddrOffset = 0x00000000U;

    LL_DMA_Init(GPDMA1, furi_hal_serial[handle->id]->dma_tx_channel, &dma_init_strust);
}

static void furi_hal_serial_dma_rx_irq(void* context) {
    FuriHalSerialHandle* handle = context;
    //Check if User Setting Error flag is active
    if(LL_DMA_IsActiveFlag_USE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel) != 0U) {
        //Clear User Setting Error flag
        LL_DMA_ClearFlag_USE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
        furi_crash("Serial: GPDMA User Setting Error");
    }

    //Check if Update Link Error flag is active
    if(LL_DMA_IsActiveFlag_ULE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel) != 0U) {
        //Clear Update Link Error flag
        LL_DMA_ClearFlag_ULE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
        furi_crash("Serial: GPDMA Update Link Error");
    }

    //Check if Data Transfer Error flag is active
    if(LL_DMA_IsActiveFlag_DTE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel) != 0U) {
        //Clear Data Transfer Error flag
        LL_DMA_ClearFlag_DTE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
        furi_crash("Serial: GPDMA Data Transfer Error");
    }

    //Check if Transfer Complete flag is active
    if(LL_DMA_IsActiveFlag_TC(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel) != 0U) {
        //Clear Transfer Complete flag
        LL_DMA_ClearFlag_TC(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);

        furi_hal_serial[handle->id]->dma_rx_node_half_buf =
            !furi_hal_serial[handle->id]->dma_rx_node_half_buf;

        size_t len_tc = fufuri_hal_serial_dma_bytes_available(handle->id);
        if(((furi_hal_serial[handle->id]->rx_dma_buf_size - len_tc) <=
            furi_hal_serial[handle->id]->rx_dma_buf_size_free) &&
           furi_hal_serial[handle->id]->rx_callback) {
            furi_hal_serial[handle->id]->rx_callback(
                furi_hal_serial[handle->id]->handle,
                FuriHalSerialEventData,
                len_tc,
                furi_hal_serial[handle->id]->context);
        }
    }

    //Check if Transfer Complete flag is active
    if(LL_DMA_IsActiveFlag_HT(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel) != 0U) {
        //Clear Half Transmit flag
        LL_DMA_ClearFlag_HT(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
        size_t len_ht = fufuri_hal_serial_dma_bytes_available(handle->id);
        if(((furi_hal_serial[handle->id]->rx_dma_buf_size - len_ht) <=
            furi_hal_serial[handle->id]->rx_dma_buf_size_free) &&
           furi_hal_serial[handle->id]->rx_callback) {
            furi_hal_serial[handle->id]->rx_callback(
                furi_hal_serial[handle->id]->handle,
                FuriHalSerialEventData,
                len_ht,
                furi_hal_serial[handle->id]->context);
        }
    }
}

static void furi_hal_serial_dma_rx_init(FuriHalSerialHandle* handle) {
    furi_assert(handle);
    LL_DMA_InitNodeTypeDef DMA_InitNodeStruct = {0};
    LL_DMA_InitLinkedListTypeDef DMA_InitLinkedListStruct = {0};

    /* Set node type */
    DMA_InitNodeStruct.NodeType = LL_DMA_GPDMA_LINEAR_NODE;

    /* Set node configuration */
    DMA_InitNodeStruct.BlkHWRequest = LL_DMA_HWREQUEST_SINGLEBURST;
    DMA_InitNodeStruct.Direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY;

    /* Where we write */
    DMA_InitNodeStruct.DestAllocatedPort = LL_DMA_DEST_ALLOCATED_PORT0;
    DMA_InitNodeStruct.DestBurstLength = 1U;
    DMA_InitNodeStruct.DestIncMode = LL_DMA_DEST_INCREMENT;
    DMA_InitNodeStruct.DestDataWidth = LL_DMA_DEST_DATAWIDTH_BYTE;

    /* Where do we read from */
    DMA_InitNodeStruct.SrcAllocatedPort = LL_DMA_SRC_ALLOCATED_PORT1;
    DMA_InitNodeStruct.SrcBurstLength = 1U;
    DMA_InitNodeStruct.SrcIncMode = LL_DMA_SRC_FIXED;
    DMA_InitNodeStruct.SrcDataWidth = LL_DMA_SRC_DATAWIDTH_BYTE;

    /* Setting interrupt at half and end of each block */
    DMA_InitNodeStruct.TransferEventMode = LL_DMA_TCEM_BLK_TRANSFER;

    /* Set node data handling parameters */
    DMA_InitNodeStruct.DataAlignment = LL_DMA_DATA_ALIGN_ZEROPADD;
    DMA_InitNodeStruct.DestHWordExchange = LL_DMA_DEST_HALFWORD_PRESERVE;
    DMA_InitNodeStruct.DestByteExchange = LL_DMA_DEST_BYTE_PRESERVE;
    DMA_InitNodeStruct.SrcByteExchange = LL_DMA_SRC_BYTE_PRESERVE;

    /* Set node trigger parameters */
    DMA_InitNodeStruct.TriggerPolarity = LL_DMA_TRIG_POLARITY_MASKED;

    /* Set repeated block parameters */
    DMA_InitNodeStruct.BlkRptDestAddrUpdateMode = LL_DMA_BLKRPT_DEST_ADDR_INCREMENT;
    DMA_InitNodeStruct.BlkRptSrcAddrUpdateMode = LL_DMA_BLKRPT_SRC_ADDR_INCREMENT;
    DMA_InitNodeStruct.DestAddrUpdateMode = LL_DMA_BURST_DEST_ADDR_INCREMENT;
    DMA_InitNodeStruct.SrcAddrUpdateMode = LL_DMA_BURST_SRC_ADDR_INCREMENT;
    DMA_InitNodeStruct.BlkRptCount = 0U;
    DMA_InitNodeStruct.DestAddrOffset = 0U;
    DMA_InitNodeStruct.SrcAddrOffset = 0U;
    DMA_InitNodeStruct.BlkRptDestAddrOffset = 0U;
    DMA_InitNodeStruct.BlkRptSrcAddrOffset = 0U;

    /* Set registers to be updated */
    DMA_InitNodeStruct.UpdateRegisters =
        (LL_DMA_UPDATE_CTR1 | LL_DMA_UPDATE_CTR2 | LL_DMA_UPDATE_CBR1 | LL_DMA_UPDATE_CSAR |
         LL_DMA_UPDATE_CDAR | LL_DMA_UPDATE_CLLR);

    /* Set transfer parameters */
    DMA_InitNodeStruct.SrcAddress = LL_USART_DMA_GetRegAddr(
        furi_hal_serial[handle->id]->periph_ptr, LL_USART_DMA_REG_DATA_RECEIVE);
    DMA_InitNodeStruct.DestAddress = (uint32_t)furi_hal_serial[handle->id]->rx_dma_buf_ptr;
    DMA_InitNodeStruct.BlkDataLength = furi_hal_serial[handle->id]->rx_dma_buf_size_half;

    /* Request which we will update the data on */
    DMA_InitNodeStruct.Request = furi_hal_serial_config[handle->id].rx_dma_request;

    /* Initializes DMA linked list node */
    LL_DMA_CreateLinkNode(&DMA_InitNodeStruct, &furi_hal_serial[handle->id]->dma_rx_link_node[0]);

    /* Set transfer parameters */
    DMA_InitNodeStruct.SrcAddress = LL_USART_DMA_GetRegAddr(
        furi_hal_serial[handle->id]->periph_ptr, LL_USART_DMA_REG_DATA_RECEIVE);
    DMA_InitNodeStruct.DestAddress = (uint32_t)furi_hal_serial[handle->id]->rx_dma_buf_ptr +
                                     furi_hal_serial[handle->id]->rx_dma_buf_size_half;
    DMA_InitNodeStruct.BlkDataLength = furi_hal_serial[handle->id]->rx_dma_buf_size_half;

    LL_DMA_CreateLinkNode(&DMA_InitNodeStruct, &furi_hal_serial[handle->id]->dma_rx_link_node[1]);

    /* Connect Node1 to Node2 */
    LL_DMA_ConnectLinkNode(
        &furi_hal_serial[handle->id]->dma_rx_link_node[0],
        LL_DMA_CLLR_OFFSET5,
        &furi_hal_serial[handle->id]->dma_rx_link_node[1],
        LL_DMA_CLLR_OFFSET5);
    /* Connect Node2 to Node1 */
    LL_DMA_ConnectLinkNode(
        &furi_hal_serial[handle->id]->dma_rx_link_node[1],
        LL_DMA_CLLR_OFFSET5,
        &furi_hal_serial[handle->id]->dma_rx_link_node[0],
        LL_DMA_CLLR_OFFSET5);

    /* Set DMA channel parameter to be configured */
    DMA_InitLinkedListStruct.Priority = LL_DMA_HIGH_PRIORITY;
    DMA_InitLinkedListStruct.TransferEventMode = LL_DMA_TCEM_LAST_LLITEM_TRANSFER;
    DMA_InitLinkedListStruct.LinkStepMode = LL_DMA_LSM_FULL_EXECUTION;
    DMA_InitLinkedListStruct.LinkAllocatedPort = LL_DMA_LINK_ALLOCATED_PORT0;

    /* Initialize the DMA linked list */
    LL_DMA_List_Init(
        GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel, &DMA_InitLinkedListStruct);

    /* Link created queue to DMA channel */
    LL_DMA_SetLinkedListBaseAddr(
        GPDMA1,
        furi_hal_serial[handle->id]->dma_rx_channel,
        (uint32_t)&furi_hal_serial[handle->id]->dma_rx_link_node[0]);
    LL_DMA_ConfigLinkUpdate(
        GPDMA1,
        furi_hal_serial[handle->id]->dma_rx_channel,
        (LL_DMA_UPDATE_CTR1 | LL_DMA_UPDATE_CTR2 | LL_DMA_UPDATE_CBR1 | LL_DMA_UPDATE_CSAR |
         LL_DMA_UPDATE_CDAR | LL_DMA_UPDATE_CTR3 | LL_DMA_UPDATE_CBR2 | LL_DMA_UPDATE_CLLR),
        (uint32_t)&furi_hal_serial[handle->id]->dma_rx_link_node[0]);

    furi_hal_interrupt_set_isr(
        furi_hal_dma_get_gpdma_interrupt_id(furi_hal_serial[handle->id]->dma_rx_channel),
        furi_hal_serial_dma_rx_irq,
        handle);

    //if needed to enable DMA interrupts
    LL_DMA_EnableIT_USE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
    LL_DMA_EnableIT_ULE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
    LL_DMA_EnableIT_DTE(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);

    LL_DMA_EnableIT_TC(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
    LL_DMA_EnableIT_HT(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);

    LL_USART_EnableDMAReq_RX(furi_hal_serial[handle->id]->periph_ptr);
    LL_DMA_EnableChannel(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
}

bool furi_hal_serial_init(FuriHalSerialHandle* handle, uint32_t baud) {
    furi_check(handle, "Serial: handle is NULL");
    furi_check(furi_hal_serial[handle->id] == NULL, "Serial: handle is already initialized");
    furi_hal_serial[handle->id] = malloc(sizeof(FuriHalSerial));

    //ToDo: Check return value furi_hal_dma_allocate_gpdma_channel
    if(!furi_hal_dma_allocate_gpdma_channel(&furi_hal_serial[handle->id]->dma_rx_channel)) {
        furi_assert("Serial: DMA RX channel allocation failed");
        return false;
    }

    furi_hal_serial[handle->id]->baudrate = baud;
    furi_hal_serial[handle->id]->handle = handle;
    furi_hal_serial[handle->id]->rx_dma_buf_size = FURI_HAL_SERIAL_DEFALUT_DMA_RX_BUF_SIZE;
    furi_hal_serial[handle->id]->rx_dma_buf_size_half =
        furi_hal_serial[handle->id]->rx_dma_buf_size / 2;
    furi_hal_serial[handle->id]->rx_dma_buf_size_free =
        furi_hal_serial[handle->id]->rx_dma_buf_size_half / 2;
    furi_hal_serial[handle->id]->rx_dma_buf_ptr =
        malloc(furi_hal_serial[handle->id]->rx_dma_buf_size);
    furi_hal_serial[handle->id]->periph_ptr = furi_hal_serial_config[handle->id].periph;

    switch(handle->id) {
    case FuriHalSerialIdUsart1:
        LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_SYSCLK);
        furi_hal_bus_enable(FuriHalBusUSART1);
        break;
    case FuriHalSerialIdUsart2:
        LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_SYSCLK);
        furi_hal_bus_enable(FuriHalBusUSART2);
        break;
    case FuriHalSerialIdUsart3:
        LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_SYSCLK);
        furi_hal_bus_enable(FuriHalBusUSART3);
        break;
    case FuriHalSerialIdUart4:
        LL_RCC_SetUARTClockSource(LL_RCC_UART4_CLKSOURCE_SYSCLK);
        furi_hal_bus_enable(FuriHalBusUART4);
        break;
    case FuriHalSerialIdUart5:
        LL_RCC_SetUARTClockSource(LL_RCC_UART5_CLKSOURCE_SYSCLK);
        furi_hal_bus_enable(FuriHalBusUART5);
        break;
    case FuriHalSerialIdUsart6:
        LL_RCC_SetUSARTClockSource(LL_RCC_USART6_CLKSOURCE_SYSCLK);
        furi_hal_bus_enable(FuriHalBusUSART6);
        break;

    default:
        furi_crash("Invalid serial id");
        break;
    }

    furi_hal_serial_set_transfer_direction(handle, FuriHalSerialTransferDirectionTxRx);

    furi_hal_serial_set_config(
        handle,
        FuriHalSerialConfigDataBits8,
        FuriHalSerialConfigParityNone,
        FuriHalSerialConfigStopBits_1);

    LL_USART_SetHWFlowCtrl(furi_hal_serial[handle->id]->periph_ptr, LL_USART_HWCONTROL_NONE);

    furi_hal_serial_set_baudrate(handle, baud);

    LL_USART_SetTXFIFOThreshold(
        furi_hal_serial[handle->id]->periph_ptr, LL_USART_FIFOTHRESHOLD_8_8);
    LL_USART_SetRXFIFOThreshold(
        furi_hal_serial[handle->id]->periph_ptr, LL_USART_FIFOTHRESHOLD_8_8);

    LL_USART_EnableFIFO(furi_hal_serial[handle->id]->periph_ptr);

    /* UART5 interrupt Init */
    furi_hal_interrupt_set_isr_ex(
        FuriHalInterruptIdUart5, FuriHalInterruptPriorityLowest, furi_hal_serial_irq, handle);
    LL_USART_EnableOverrunDetect(furi_hal_serial[handle->id]->periph_ptr);
    LL_USART_EnableDMADeactOnRxErr(furi_hal_serial[handle->id]->periph_ptr);

    LL_USART_DisableIT_RXNE_RXFNE(furi_hal_serial[handle->id]->periph_ptr);
    LL_USART_DisableIT_IDLE(furi_hal_serial[handle->id]->periph_ptr);

    LL_USART_SetRxTimeout(
        furi_hal_serial[handle->id]->periph_ptr,
        70); // count byte * count bot in 1 byte  (8n1 = 10bit)
    LL_USART_EnableRxTimeout(furi_hal_serial[handle->id]->periph_ptr);
    LL_USART_EnableIT_RTO(furi_hal_serial[handle->id]->periph_ptr);

    furi_hal_serial_dma_rx_init(handle);

    LL_USART_Enable(furi_hal_serial[handle->id]->periph_ptr);

    // Clear TC flag
    if(LL_USART_IsActiveFlag_TC(furi_hal_serial[handle->id]->periph_ptr)) {
        LL_USART_ClearFlag_TC(furi_hal_serial[handle->id]->periph_ptr);
    }

    while(!LL_USART_IsActiveFlag_TEACK(furi_hal_serial[handle->id]->periph_ptr) ||
          !LL_USART_IsActiveFlag_REACK(furi_hal_serial[handle->id]->periph_ptr));

    return true;
}

void furi_hal_serial_deinit(FuriHalSerialHandle* handle) {
    furi_check(handle, "Serial: handle is NULL");
    if(furi_hal_serial[handle->id] == NULL) {
        return;
    }
    LL_USART_Disable(furi_hal_serial[handle->id]->periph_ptr);
    furi_hal_interrupt_set_isr(FuriHalInterruptIdUart5, NULL, NULL);

    LL_DMA_DisableChannel(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
    LL_USART_DisableDMAReq_RX(furi_hal_serial[handle->id]->periph_ptr);

    furi_hal_interrupt_set_isr(
        furi_hal_dma_get_gpdma_interrupt_id(furi_hal_serial[handle->id]->dma_rx_channel),
        NULL,
        NULL);

    furi_hal_serial_set_transfer_direction(handle, FuriHalSerialTransferDirectionNone);

    switch(handle->id) {
    case FuriHalSerialIdUsart1:
        furi_hal_bus_disable(FuriHalBusUSART1);
        break;
    case FuriHalSerialIdUsart2:
        furi_hal_bus_disable(FuriHalBusUSART2);
        break;
    case FuriHalSerialIdUsart3:
        furi_hal_bus_disable(FuriHalBusUSART3);
        break;
    case FuriHalSerialIdUart4:
        furi_hal_bus_disable(FuriHalBusUART4);
        break;
    case FuriHalSerialIdUart5:
        furi_hal_bus_disable(FuriHalBusUART5);
        break;
    case FuriHalSerialIdUsart6:
        furi_hal_bus_disable(FuriHalBusUSART6);
        break;

    default:
        furi_crash("Invalid serial id");
        break;
    }

    furi_hal_dma_free_gpdma_channel(furi_hal_serial[handle->id]->dma_rx_channel);

    if(furi_hal_serial[handle->id]->dma_tx_use) {
        // Wait Tx Complete previous transmission
        while(LL_USART_IsEnabledIT_TC(furi_hal_serial[handle->id]->periph_ptr)) {
            furi_delay_ms(1);
        }
        furi_hal_dma_free_gpdma_channel(furi_hal_serial[handle->id]->dma_tx_channel);
    }
    free(furi_hal_serial[handle->id]->rx_dma_buf_ptr);
    free(furi_hal_serial[handle->id]);
    furi_hal_serial[handle->id] = NULL;
}

void furi_hal_serial_tx(FuriHalSerialHandle* handle, const uint8_t* buffer, size_t buffer_size) {
    furi_hal_serial_check(handle);
    furi_check(
        LL_USART_IsEnabled(furi_hal_serial[handle->id]->periph_ptr), "Serial: is not enabled");

    while(buffer_size > 0) {
        while(!LL_USART_IsActiveFlag_TXE(furi_hal_serial[handle->id]->periph_ptr));

        LL_USART_TransmitData8(furi_hal_serial[handle->id]->periph_ptr, *buffer);

        buffer++;
        buffer_size--;
    }
    // Wait Tx Complete
    while(!LL_USART_IsActiveFlag_TC(furi_hal_serial[handle->id]->periph_ptr));
    // Clear TC flag
    if(LL_USART_IsActiveFlag_TC(furi_hal_serial[handle->id]->periph_ptr)) {
        LL_USART_ClearFlag_TC(furi_hal_serial[handle->id]->periph_ptr);
    }
}

void furi_hal_serial_dma_tx(FuriHalSerialHandle* handle, const uint8_t* buffer, size_t buffer_size) {
    furi_check(furi_hal_serial[handle->id]->dma_tx_use, "Serial: DMA TX is not enabled");
    furi_hal_serial_check(handle);
    furi_check(
        LL_USART_IsEnabled(furi_hal_serial[handle->id]->periph_ptr), "Serial: is not enabled");

    // Wait Tx Complete previous transmission
    while(LL_USART_IsEnabledIT_TC(furi_hal_serial[handle->id]->periph_ptr)) {
        furi_delay_ms(1);
    }

    FURI_CRITICAL_ENTER();
    LL_DMA_SetBlkDataLength(GPDMA1, furi_hal_serial[handle->id]->dma_tx_channel, buffer_size);
    LL_DMA_SetSrcAddress(GPDMA1, furi_hal_serial[handle->id]->dma_tx_channel, (uint32_t)buffer);
    LL_USART_EnableIT_TC(furi_hal_serial[handle->id]->periph_ptr);
    LL_USART_EnableDMAReq_TX(furi_hal_serial[handle->id]->periph_ptr);
    LL_DMA_EnableChannel(GPDMA1, furi_hal_serial[handle->id]->dma_tx_channel);
    FURI_CRITICAL_EXIT();
}

void furi_hal_serial_set_callback(
    FuriHalSerialHandle* handle,
    FuriHalSerialDmaAsyncTxCallback tx_callback,
    FuriHalSerialDmaAsyncRxCallback rx_callback,
    void* context) {
    furi_hal_serial_check(handle);
    furi_hal_serial[handle->id]->tx_callback = tx_callback;
    furi_hal_serial[handle->id]->rx_callback = rx_callback;
    furi_hal_serial[handle->id]->context = context;
}

size_t furi_hal_serial_get_rx_data(FuriHalSerialHandle* handle, uint8_t* data, size_t len) {
    furi_assert(handle, "Serial: handle is NULL");
    size_t available = fufuri_hal_serial_dma_bytes_available(handle->id);
    if(available < len) {
        len = available;
    }
    furi_hal_serial[handle->id]->rx_dma_buf_count_bytes -= len;
    if((furi_hal_serial[handle->id]->rx_dma_buf_index_read + len) <=
       furi_hal_serial[handle->id]->rx_dma_buf_size) {
        memcpy(
            data,
            furi_hal_serial[handle->id]->rx_dma_buf_ptr +
                furi_hal_serial[handle->id]->rx_dma_buf_index_read,
            len);
        furi_hal_serial[handle->id]->rx_dma_buf_index_read += len;
        if(furi_hal_serial[handle->id]->rx_dma_buf_index_read >=
           furi_hal_serial[handle->id]->rx_dma_buf_size) {
            furi_hal_serial[handle->id]->rx_dma_buf_index_read = 0;
        }
    } else {
        size_t len_tmp = furi_hal_serial[handle->id]->rx_dma_buf_size -
                         furi_hal_serial[handle->id]->rx_dma_buf_index_read;
        memcpy(
            data,
            furi_hal_serial[handle->id]->rx_dma_buf_ptr +
                furi_hal_serial[handle->id]->rx_dma_buf_index_read,
            len_tmp);
        furi_hal_serial[handle->id]->rx_dma_buf_index_read = len - len_tmp;
        memcpy(
            data + len_tmp,
            furi_hal_serial[handle->id]->rx_dma_buf_ptr,
            furi_hal_serial[handle->id]->rx_dma_buf_index_read);
    }
    return len;
}

void furi_hal_serial_set_baudrate(FuriHalSerialHandle* handle, uint32_t baud_rate) {
    furi_hal_serial_check(handle);
    furi_check(baud_rate >= 10 && baud_rate <= 20000000, "Serial: invalid baud rate");

    uint32_t divisor = (SystemCoreClock / baud_rate);
    uint32_t prescaler = 0;
    uint32_t over_sampling = 0;

    if(baud_rate > 10000000) {
        over_sampling = LL_USART_OVERSAMPLING_8;
        divisor = (divisor / 8) >> 12;
    } else {
        over_sampling = LL_USART_OVERSAMPLING_16;
        divisor = (divisor / 16) >> 12;
    }

    if(divisor < 1) {
        prescaler = LL_USART_PRESCALER_DIV1;
    } else if(divisor < 2) {
        prescaler = LL_USART_PRESCALER_DIV2;
    } else if(divisor < 4) {
        prescaler = LL_USART_PRESCALER_DIV4;
    } else if(divisor < 6) {
        prescaler = LL_USART_PRESCALER_DIV6;
    } else if(divisor < 8) {
        prescaler = LL_USART_PRESCALER_DIV8;
    } else if(divisor < 10) {
        prescaler = LL_USART_PRESCALER_DIV10;
    } else if(divisor < 12) {
        prescaler = LL_USART_PRESCALER_DIV12;
    } else if(divisor < 16) {
        prescaler = LL_USART_PRESCALER_DIV16;
    } else if(divisor < 32) {
        prescaler = LL_USART_PRESCALER_DIV32;
    } else if(divisor < 64) {
        prescaler = LL_USART_PRESCALER_DIV64;
    } else if(divisor < 128) {
        prescaler = LL_USART_PRESCALER_DIV128;
    } else {
        prescaler = LL_USART_PRESCALER_DIV256;
    }

    LL_USART_SetOverSampling(furi_hal_serial[handle->id]->periph_ptr, over_sampling);
    LL_USART_SetPrescaler(furi_hal_serial[handle->id]->periph_ptr, prescaler);
    LL_USART_SetBaudRate(
        furi_hal_serial[handle->id]->periph_ptr,
        SystemCoreClock,
        prescaler,
        over_sampling,
        baud_rate);
}

bool furi_hal_serial_dma_tx_enable(FuriHalSerialHandle* handle) {
    furi_hal_serial_check(handle);

    if(!furi_hal_dma_allocate_gpdma_channel(&furi_hal_serial[handle->id]->dma_tx_channel)) {
        furi_assert("Serial: DMA TX channel allocation failed");
        return false;
    }
    furi_hal_serial[handle->id]->dma_tx_use = true;
    furi_hal_serial_dma_tx_init(handle);

    return true;
}

void furi_hal_serial_set_transfer_direction(
    FuriHalSerialHandle* handle,
    FuriHalSerialTransferDirection dir) {
    furi_hal_serial_check(handle);
    uint32_t direction = LL_USART_DIRECTION_NONE;
    const GpioPin* gpio_tx = furi_hal_serial_config[handle->id].gpio[FuriHalSerialPinTx];
    const GpioPin* gpio_rx = furi_hal_serial_config[handle->id].gpio[FuriHalSerialPinRx];
    const GpioAltFn alt_fn = furi_hal_serial_config[handle->id].alt_fn;

    switch(dir) {
    case FuriHalSerialTransferDirectionNone:
        direction = LL_USART_DIRECTION_NONE;
        furi_hal_gpio_init(gpio_tx, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
        furi_hal_gpio_init(gpio_rx, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
        break;
    case FuriHalSerialTransferDirectionTx:
        direction = LL_USART_DIRECTION_TX;
        furi_hal_gpio_init_ex(
            gpio_tx, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedHigh, alt_fn);
        furi_hal_gpio_init(gpio_rx, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
        break;
    case FuriHalSerialTransferDirectionRx:
        direction = LL_USART_DIRECTION_RX;
        furi_hal_gpio_init(gpio_tx, GpioModeAnalog, GpioPullNo, GpioSpeedLow);
        furi_hal_gpio_init_ex(
            gpio_rx, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedHigh, alt_fn);
        break;
    case FuriHalSerialTransferDirectionTxRx:
        direction = LL_USART_DIRECTION_TX_RX;
        furi_hal_gpio_init_ex(
            gpio_tx, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedHigh, alt_fn);
        furi_hal_gpio_init_ex(
            gpio_rx, GpioModeAltFunctionPushPull, GpioPullNo, GpioSpeedHigh, alt_fn);
        break;
    default:
        furi_crash("Serial: Invalid transfer direction");
        break;
    }
    LL_USART_SetTransferDirection(furi_hal_serial[handle->id]->periph_ptr, direction);
}

const GpioPin* furi_hal_serial_gpio_get_pin(FuriHalSerialHandle* handle, FuriHalSerialPin pin) {
    furi_hal_serial_check(handle);
    return furi_hal_serial_config[handle->id].gpio[pin];
}

void furi_hal_serial_set_config(
    FuriHalSerialHandle* handle,
    FuriHalSerialConfigDataBits data_bits,
    FuriHalSerialConfigParity parity,
    FuriHalSerialConfigStopBits stop_bits) {
    furi_hal_serial_check(handle);
    uint32_t data_width = LL_USART_DATAWIDTH_8B;
    uint32_t parity_mode = LL_USART_PARITY_NONE;
    uint32_t stop_bits_mode = LL_USART_STOPBITS_1;
    bool is_enabled = furi_hal_serial_is_enabled(handle);

    switch(data_bits) {
    case FuriHalSerialConfigDataBits7:
        data_width = LL_USART_DATAWIDTH_7B;
        break;
    case FuriHalSerialConfigDataBits8:
        data_width = LL_USART_DATAWIDTH_8B;
        break;
    case FuriHalSerialConfigDataBits9:
        data_width = LL_USART_DATAWIDTH_9B;
        break;
    default:
        furi_crash("Serial: Invalid data bits");
        break;
    }

    switch(parity) {
    case FuriHalSerialConfigParityNone:
        parity_mode = LL_USART_PARITY_NONE;
        break;
    case FuriHalSerialConfigParityEven:
        parity_mode = LL_USART_PARITY_EVEN;
        break;
    case FuriHalSerialConfigParityOdd:
        parity_mode = LL_USART_PARITY_ODD;
        break;
    default:
        furi_crash("Serial: Invalid parity");
        break;
    }

    switch(stop_bits) {
    case FuriHalSerialConfigStopBits_0_5:
        stop_bits_mode = LL_USART_STOPBITS_0_5;
        break;
    case FuriHalSerialConfigStopBits_1:
        stop_bits_mode = LL_USART_STOPBITS_1;
        break;
    case FuriHalSerialConfigStopBits_1_5:
        stop_bits_mode = LL_USART_STOPBITS_1_5;
        break;
    case FuriHalSerialConfigStopBits_2:
        stop_bits_mode = LL_USART_STOPBITS_2;
        break;
    default:
        furi_crash("Serial: Invalid stop bits");
        break;
    }

    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    if(parity != FuriHalSerialConfigParityNone) {
        LL_USART_EnableIT_PE(furi_hal_serial[handle->id]->periph_ptr);
    } else {
        LL_USART_DisableIT_PE(furi_hal_serial[handle->id]->periph_ptr);
    }

    LL_USART_ConfigCharacter(
        furi_hal_serial[handle->id]->periph_ptr, data_width, parity_mode, stop_bits_mode);
    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

void furi_hal_serial_tx_rx_swap(FuriHalSerialHandle* handle, bool enable) {
    furi_hal_serial_check(handle);

    bool is_enabled = furi_hal_serial_is_enabled(handle);
    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    if(enable) {
        LL_USART_SetTXRXSwap(furi_hal_serial[handle->id]->periph_ptr, LL_USART_TXRX_SWAPPED);
    } else {
        LL_USART_SetTXRXSwap(furi_hal_serial[handle->id]->periph_ptr, LL_USART_TXRX_STANDARD);
    }

    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

void furi_hal_serial_rx_level_inverted(FuriHalSerialHandle* handle, bool enable) {
    furi_hal_serial_check(handle);

    bool is_enabled = furi_hal_serial_is_enabled(handle);
    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    if(enable) {
        LL_USART_SetRXPinLevel(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_RXPIN_LEVEL_INVERTED);
    } else {
        LL_USART_SetRXPinLevel(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_RXPIN_LEVEL_STANDARD);
    }

    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

void furi_hal_serial_tx_level_inverted(FuriHalSerialHandle* handle, bool enable) {
    furi_hal_serial_check(handle);

    bool is_enabled = furi_hal_serial_is_enabled(handle);
    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    if(enable) {
        LL_USART_SetTXPinLevel(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_TXPIN_LEVEL_INVERTED);
    } else {
        LL_USART_SetTXPinLevel(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_TXPIN_LEVEL_STANDARD);
    }

    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

void furi_hal_serial_set_transfer_bit_order(
    FuriHalSerialHandle* handle,
    FuriHalSerialTransferBitOrder bit_order) {
    furi_hal_serial_check(handle);

    bool is_enabled = furi_hal_serial_is_enabled(handle);
    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    switch(bit_order) {
    case FuriHalSerialTransferBitOrderLsbFirst:
        LL_USART_SetTransferBitOrder(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_BITORDER_LSBFIRST);
        break;
    case FuriHalSerialTransferBitOrderMsbFirst:
        LL_USART_SetTransferBitOrder(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_BITORDER_MSBFIRST);
        break;

    default:
        furi_crash("Serial: Invalid transfer bit order");
        break;
    }

    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

void furi_hal_serial_set_binary_data_logic(
    FuriHalSerialHandle* handle,
    FuriHalSerialBinaryDataLogic binary_data_logic) {
    furi_hal_serial_check(handle);

    bool is_enabled = furi_hal_serial_is_enabled(handle);
    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    switch(binary_data_logic) {
    case FuriHalSerialBinaryDataLogicPositive:
        LL_USART_SetBinaryDataLogic(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_BINARY_LOGIC_POSITIVE);
        break;
    case FuriHalSerialBinaryDataLogicNegative:
        LL_USART_SetBinaryDataLogic(
            furi_hal_serial[handle->id]->periph_ptr, LL_USART_BINARY_LOGIC_NEGATIVE);
        break;

    default:
        furi_crash("Serial: Invalid binary data logic");
        break;
    }

    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

void furi_hal_serial_set_rx_dma_buffer_size(FuriHalSerialHandle* handle, uint16_t size) {
    furi_hal_serial_check(handle);
    furi_check(size % 4 == 0, "Serial: size must be multiple of 4");

    bool is_enabled = furi_hal_serial_is_enabled(handle);
    if(is_enabled) {
        furi_hal_serial_suspend(handle);
    }

    FURI_CRITICAL_ENTER();
    LL_DMA_DisableChannel(GPDMA1, furi_hal_serial[handle->id]->dma_rx_channel);
    LL_USART_DisableDMAReq_RX(furi_hal_serial[handle->id]->periph_ptr);

    furi_hal_interrupt_set_isr(
        furi_hal_dma_get_gpdma_interrupt_id(furi_hal_serial[handle->id]->dma_rx_channel),
        NULL,
        NULL);

    furi_hal_serial[handle->id]->rx_dma_buf_size = size;
    furi_hal_serial[handle->id]->rx_dma_buf_size_half = size / 2;
    furi_hal_serial[handle->id]->rx_dma_buf_size_free = size / 4;
    free(furi_hal_serial[handle->id]->rx_dma_buf_ptr);
    furi_hal_serial[handle->id]->rx_dma_buf_ptr = malloc(size);

    furi_hal_serial_dma_rx_init(handle);
    FURI_CRITICAL_EXIT();

    if(is_enabled) {
        furi_hal_serial_resume(handle);
    }
}

inline void furi_hal_serial_suspend(FuriHalSerialHandle* handle) {
    furi_hal_serial_check(handle);
    LL_USART_Disable(furi_hal_serial[handle->id]->periph_ptr);
}

inline void furi_hal_serial_resume(FuriHalSerialHandle* handle) {
    furi_hal_serial_check(handle);
    LL_USART_Enable(furi_hal_serial[handle->id]->periph_ptr);
}

inline bool furi_hal_serial_is_enabled(FuriHalSerialHandle* handle) {
    furi_hal_serial_check(handle);
    return LL_USART_IsEnabled(furi_hal_serial[handle->id]->periph_ptr);
}

void furi_hal_serial_tx_wait_complete(FuriHalSerialHandle* handle) {
    furi_hal_serial_check(handle);

    // Wait Tx Complete if dma transmission is not enabled
    while(LL_USART_IsEnabledIT_TC(furi_hal_serial[handle->id]->periph_ptr)) {
        furi_delay_ms(1);
    }
    // Wait Tx Complete if simple transfer
    while(!LL_USART_IsActiveFlag_TC(furi_hal_serial[handle->id]->periph_ptr));
    LL_USART_ClearFlag_TC(furi_hal_serial[handle->id]->periph_ptr);
}