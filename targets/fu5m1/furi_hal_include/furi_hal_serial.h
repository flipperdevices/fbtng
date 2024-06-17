#pragma once

#include "stm32u5xx.h"
#include "stddef.h"
#include "furi_hal_serial_types.h"
#include "furi_hal_gpio.h"

typedef enum {
    FuriHalSerialEventData = (1 << 0), /**< Data: new data available */
    FuriHalSerialEventIdle = (1 << 1), /**< Idle: bus idle detected */
    FuriHalSerialEventFrameError = (1 << 2), /**< Framing Error: incorrect frame detected */
    FuriHalSerialEventNoiseError = (1 << 3), /**< Noise Error: noise on the line detected */
    FuriHalSerialEventParityError = (1 << 4), /**< Parity Error: incorrect parity detected */
    FuriHalSerialEventOverrunRxFifoError =
        (1 << 5), /**< Overrun Error: no space for received data */
    FuriHalSerialEventOverrunRxFifoDmaError =
        (1 << 6), /**< Overrun Error: no space for received data */
} FuriHalSerialEvent;

typedef enum {
    FuriHalSerialDmaTxEventComplete = (1 << 0), /**< Transmission complete */
} FuriHalSerialDmaTxEvent;

typedef void (*FuriHalSerialDmaAsyncRxCallback)(
    FuriHalSerialHandle* handle,
    FuriHalSerialEvent event,
    size_t data_len,
    void* context);
typedef void (*FuriHalSerialDmaAsyncTxCallback)(
    FuriHalSerialHandle* handle,
    FuriHalSerialDmaTxEvent event,
    void* context);

/**
 * Initialize the serial interface.
 *
 * @param handle Pointer to the serial handle.
 * @param baud_rate Baud rate.
 *      @arg min: 10UL
 *      @arg max: 20000000UL
 * @return true if the initialization was successful, false otherwise.
 */
bool furi_hal_serial_init(FuriHalSerialHandle* handle, uint32_t baud_rate);

/**
 * Deinitialize the serial interface.
 *
 * @param handle Pointer to the serial handle.
 */
void furi_hal_serial_deinit(FuriHalSerialHandle* handle);

/**
 * Transmit data over the serial interface.
 *
 * @param handle Pointer to the serial handle.
 * @param buffer Pointer to the data buffer.
 * @param buffer_size Size of the data buffer.
 */
void furi_hal_serial_tx(FuriHalSerialHandle* handle, const uint8_t* buffer, size_t buffer_size);

/**
 * Transmit data over the serial interface using DMA.
 *
 * @param handle Pointer to the serial handle.
 * @param buffer Pointer to the data buffer.
 * @param buffer_size Size of the data buffer.
 */
void furi_hal_serial_dma_tx(FuriHalSerialHandle* handle, const uint8_t* buffer, size_t buffer_size);

/**
 * Set the callback functions for the serial interface.
 *
 * @param handle Pointer to the serial handle.
 * @param tx_callback Pointer to the transmit callback function.
 * @param rx_callback Pointer to the receive callback function.
 * @param context Pointer to the context object.
 */
void furi_hal_serial_set_callback(
    FuriHalSerialHandle* handle,
    FuriHalSerialDmaAsyncTxCallback tx_callback,
    FuriHalSerialDmaAsyncRxCallback rx_callback,
    void* context);

/**
 * Get the received data from the serial interface.
 * @param handle Pointer to the serial handle.
 * @param data Pointer to the data buffer.
 * @param len Size of the data buffer.
 * @return The number of bytes received.
 */
size_t furi_hal_serial_get_rx_data(FuriHalSerialHandle* handle, uint8_t* data, size_t len);

/**
 * Set the baud rate for the serial interface.
 *
 * @param handle Pointer to the serial handle.
 * @param baud_rate Baud rate.
 *      @arg min: 10UL
 *      @arg max: 20000000UL
 */
void furi_hal_serial_set_baudrate(FuriHalSerialHandle* handle, uint32_t baud_rate);

/**
 * Enable the DMA transmit mode.
 *
 * @param handle Pointer to the serial handle.
 * @return true if the DMA transmit channel was enabled, false error DMA transmit channel.
 */
bool furi_hal_serial_dma_tx_enable(FuriHalSerialHandle* handle);

/**
 * Set the transfer direction for the serial interface.
 *
 * @param handle Pointer to the serial handle.
 * @param dir Transfer direction.
 *      @arg @ref FuriHalSerialTransferDirectionNone
 *      @arg @ref FuriHalSerialTransferDirectionTx
 *      @arg @ref FuriHalSerialTransferDirectionRx
 *      @arg @ref FuriHalSerialTransferDirectionTxRx
 */
void furi_hal_serial_set_transfer_direction(
    FuriHalSerialHandle* handle,
    FuriHalSerialTransferDirection dir);

/**
 * Get the GPIO pin for the serial interface.
 * @param handle Pointer to the serial handle.
 * @param pin Pin.
 *     @arg @ref FuriHalSerialPinTx
 *     @arg @ref FuriHalSerialPinRx
 * @return Pointer to the GPIO pin.
 */
const GpioPin* furi_hal_serial_gpio_get_pin(FuriHalSerialHandle* handle, FuriHalSerialPin pin);

/**
 * Set the configuration for the serial interface.
 * @param handle Pointer to the serial handle.
 * @param data_bits Number of data bits.
 *      @arg @ref FuriHalSerialConfigDataBits7
 *      @arg @ref FuriHalSerialConfigDataBits8
 *      @arg @ref FuriHalSerialConfigDataBits9
 * @param parity Parity.
 *      @arg @ref FuriHalSerialConfigParityNone
 *      @arg @ref FuriHalSerialConfigParityEven
 *      @arg @ref FuriHalSerialConfigParityOdd
 * @param stop_bits Number of stop bits.
 *      @arg @ref FuriHalSerialConfigStopBits_0_5
 *      @arg @ref FuriHalSerialConfigStopBits_1
 *      @arg @ref FuriHalSerialConfigStopBits_1_5
 *      @arg @ref FuriHalSerialConfigStopBits_2
 */
void furi_hal_serial_set_config(
    FuriHalSerialHandle* handle,
    FuriHalSerialConfigDataBits data_bits,
    FuriHalSerialConfigParity parity,
    FuriHalSerialConfigStopBits stop_bits);

/**
 * Swap the transmit and receive pins.
 * @param handle Pointer to the serial handle.
 * @param enable true to swap the pins, false normal mode.
 */
void furi_hal_serial_tx_rx_swap(FuriHalSerialHandle* handle, bool enable);

/**
 * Invert logic the receive level.
 * @param handle Pointer to the serial handle.
 * @param enable true to invert the level, false normal mode.
 */
void furi_hal_serial_rx_level_inverted(FuriHalSerialHandle* handle, bool enable);

/**
 * Invert logic the transmit level.
 * @param handle Pointer to the serial handle.
 * @param enable true to invert the level, false normal mode.
 */
void furi_hal_serial_tx_level_inverted(FuriHalSerialHandle* handle, bool enable);

/**
 * Set the transfer bit order for the serial interface.
 * @param handle Pointer to the serial handle.
 * @param bit_order Transfer bit order.
 *      @arg @ref FuriHalSerialTransferBitOrderLsbFirst
 *      @arg @ref FuriHalSerialTransferBitOrderMsbFirst
 */
void furi_hal_serial_set_transfer_bit_order(
    FuriHalSerialHandle* handle,
    FuriHalSerialTransferBitOrder bit_order);

/**
 * Set the binary data logic for the serial interface.
 * @param handle Pointer to the serial handle.
 * @param binary_data_logic Binary data logic.
 *      @arg @ref FuriHalSerialBinaryDataLogicPositive
 *      @arg @ref FuriHalSerialBinaryDataLogicNegative
 */
void furi_hal_serial_set_binary_data_logic(
    FuriHalSerialHandle* handle,
    FuriHalSerialBinaryDataLogic binary_data_logic);

/**
 * Set the DMA buffer size for the transmit channel.
 * @warning The buffer size must be a multiple of 4.
 * @param handle Pointer to the serial handle.
 * @param size Size of the DMA buffer.
 */
void furi_hal_serial_set_rx_dma_buffer_size(FuriHalSerialHandle* handle, uint16_t size);

/**
 * Suspend the serial interface.
 * @param handle Pointer to the serial handle.
 */
void furi_hal_serial_suspend(FuriHalSerialHandle* handle);

/**
 * Resume the serial interface.
 * @param handle Pointer to the serial handle.
 */
void furi_hal_serial_resume(FuriHalSerialHandle* handle);

/**
 * Check if the serial interface is enabled.
 * @param handle Pointer to the serial handle.
 * @return true if the serial interface is enabled, false disabled.
 */
bool furi_hal_serial_is_enabled(FuriHalSerialHandle* handle);

/**
 * Wait for the transmission to complete.
 * @param handle Pointer to the serial handle.
 */
void furi_hal_serial_tx_wait_complete(FuriHalSerialHandle* handle);