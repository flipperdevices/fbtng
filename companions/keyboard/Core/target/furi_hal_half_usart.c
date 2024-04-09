#include "furi_hal_half_usart.h"

#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_gpio.h>
#include <stm32g0xx_ll_usart.h>

typedef struct {
	FuriHalfUsartRxCallback callback;
    void* context;
} FuriHalHalfUsart;

static FuriHalHalfUsart furi_hal_half_usart ={0};

void furi_hal_half_usart_init(void) {

	LL_USART_InitTypeDef USART_InitStruct = { 0 };
	LL_GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* Peripheral clock enable */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	/**USART2 GPIO Configuration
	 PA14-BOOT0   ------> USART2_TX
	 */
	GPIO_InitStruct.Pin = LL_GPIO_PIN_14;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
	LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/* USART2 interrupt Init */
	NVIC_SetPriority(USART2_IRQn, 0);
	NVIC_EnableIRQ(USART2_IRQn);

	USART_InitStruct.PrescalerValue = LL_USART_PRESCALER_DIV1;
	USART_InitStruct.BaudRate = 115200;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	LL_USART_Init(USART2, &USART_InitStruct);
	LL_USART_DisableOverrunDetect(USART2);
	LL_USART_DisableDMADeactOnRxErr(USART2);
	LL_USART_ConfigHalfDuplexMode(USART2);

	LL_USART_EnableIT_RXNE_RXFNE(USART2);
	LL_USART_Enable(USART2);

	/* Polling USART2 initialisation */
	while ((!(LL_USART_IsActiveFlag_TEACK(USART2)))
			|| (!(LL_USART_IsActiveFlag_REACK(USART2)))) {
	}

}

void furi_hal_half_usart_tx(uint8_t *buffer, uint32_t size) {
	if (LL_USART_IsEnabled(USART2) == 0)
		return;
	//tx data
	LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_TX);
	while (size > 0) {
		while (!LL_USART_IsActiveFlag_TXE(USART2))
			;
		LL_USART_TransmitData8(USART2, *buffer);
		buffer++;
		size--;
	}
	//wait end tx data
	while (!LL_USART_IsActiveFlag_TC(USART2))
		;
	LL_USART_SetTransferDirection(USART2, LL_USART_DIRECTION_RX);
}

void furi_hal_half_usart_rx(void) {
	if (LL_USART_IsActiveFlag_RXNE_RXFNE(USART2)) {
		if(furi_hal_half_usart.callback){
			furi_hal_half_usart.callback(LL_USART_ReceiveData8(USART2));
		}
	} else if (LL_USART_IsActiveFlag_ORE(USART2)) {
		LL_USART_ClearFlag_ORE(USART2);
	}

//	if (rx_buf_index > 2) {
//		func_set_rgb(rx_buf_data[0], rx_buf_data[1], rx_buf_data[2]);
//		func_tx(rx_buf_data, rx_buf_index);
//		rx_buf_index = 0;
//	}
}

void USART2_IRQHandler(void)
{
	furi_hal_half_usart_rx();
}
