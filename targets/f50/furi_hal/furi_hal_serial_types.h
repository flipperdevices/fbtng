#pragma once

#include <furi.h>

/**
 * UART channels
 */
typedef enum {
    FuriHalSerialIdUsart1,
    FuriHalSerialIdUsart2,
    FuriHalSerialIdUsart3,
    FuriHalSerialIdUart4,
    FuriHalSerialIdUart5,
    FuriHalSerialIdUsart6,

    FuriHalSerialIdMax,
} FuriHalSerialId;

typedef enum {
    FuriHalSerialPinTx,
    FuriHalSerialPinRx,

    FuriHalSerialPinMax,
} FuriHalSerialPin;

typedef enum {
    FuriHalSerialTransferDirectionNone,
    FuriHalSerialTransferDirectionTx,
    FuriHalSerialTransferDirectionRx,
    FuriHalSerialTransferDirectionTxRx,
} FuriHalSerialTransferDirection;

typedef enum {
    FuriHalSerialConfigDataBits7,
    FuriHalSerialConfigDataBits8,
    FuriHalSerialConfigDataBits9,
} FuriHalSerialConfigDataBits;

typedef enum {
    FuriHalSerialConfigParityNone,
    FuriHalSerialConfigParityEven,
    FuriHalSerialConfigParityOdd,
} FuriHalSerialConfigParity;

typedef enum {
    FuriHalSerialConfigStopBits_0_5,
    FuriHalSerialConfigStopBits_1,
    FuriHalSerialConfigStopBits_1_5,
    FuriHalSerialConfigStopBits_2,
} FuriHalSerialConfigStopBits;

typedef enum {
    FuriHalSerialTransferBitOrderLsbFirst,
    FuriHalSerialTransferBitOrderMsbFirst,
} FuriHalSerialTransferBitOrder;

typedef enum {
    FuriHalSerialBinaryDataLogicPositive,
    FuriHalSerialBinaryDataLogicNegative,
} FuriHalSerialBinaryDataLogic;

typedef struct FuriHalSerialHandle FuriHalSerialHandle;
