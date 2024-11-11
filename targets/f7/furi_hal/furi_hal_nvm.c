#include <furi_hal_rtc.h>
#include <furi_hal_nvm.h>
#include <furi_hal_debug.h>
#include <furi_hal_serial_control.h>

#include <stm32wbxx_ll_rtc.h>

#define TAG "FuriHalNvm"

#define FURI_HAL_RTC_HEADER_MAGIC   0x10F1
#define FURI_HAL_RTC_HEADER_VERSION 0

typedef struct {
    uint16_t magic;
    uint8_t version;
    uint8_t unused;
} FuriHalRtcHeader;

typedef struct {
    uint8_t log_level    : 4;
    uint8_t log_reserved : 4;
    uint8_t flags;
    FuriHalRtcBootMode boot_mode                 : 4;
    FuriHalRtcHeapTrackMode heap_track_mode      : 2;
    FuriHalRtcLocaleUnits locale_units           : 1;
    FuriHalRtcLocaleTimeFormat locale_timeformat : 1;
    FuriHalRtcLocaleDateFormat locale_dateformat : 2;
    FuriHalRtcLogDevice log_device               : 2;
    FuriHalRtcLogBaudRate log_baud_rate          : 3;
    uint8_t reserved                             : 1;
} SystemReg;

_Static_assert(sizeof(SystemReg) == 4, "SystemReg size mismatch");

static const FuriHalSerialId furi_hal_rtc_log_devices[] = {
    [FuriHalRtcLogDeviceUsart] = FuriHalSerialIdUsart,
    [FuriHalRtcLogDeviceLpuart] = FuriHalSerialIdLpuart,
    [FuriHalRtcLogDeviceReserved] = FuriHalSerialIdMax,
    [FuriHalRtcLogDeviceNone] = FuriHalSerialIdMax,
};

static const uint32_t furi_hal_rtc_log_baud_rates[] = {
    [FuriHalRtcLogBaudRate230400] = 230400,
    [FuriHalRtcLogBaudRate9600] = 9600,
    [FuriHalRtcLogBaudRate38400] = 38400,
    [FuriHalRtcLogBaudRate57600] = 57600,
    [FuriHalRtcLogBaudRate115200] = 115200,
    [FuriHalRtcLogBaudRate460800] = 460800,
    [FuriHalRtcLogBaudRate921600] = 921600,
    [FuriHalRtcLogBaudRate1843200] = 1843200,
};

void furi_hal_nvm_init_early(void) {
    // Verify header register
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterHeader);
    FuriHalRtcHeader* data = (FuriHalRtcHeader*)&data_reg;
    if(data->magic != FURI_HAL_RTC_HEADER_MAGIC || data->version != FURI_HAL_RTC_HEADER_VERSION) {
        furi_hal_rtc_reset_registers();
    }

    if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagDebug)) {
        furi_hal_debug_enable();
    } else {
        furi_hal_debug_disable();
    }
}

void furi_hal_nvm_init(void) {
    furi_log_set_level(furi_hal_rtc_get_log_level());
    furi_hal_serial_control_set_logging_config(
        furi_hal_rtc_log_devices[furi_hal_rtc_get_log_device()],
        furi_hal_rtc_log_baud_rates[furi_hal_rtc_get_log_baud_rate()]);

    FURI_LOG_I(TAG, "Init OK");
}

void furi_hal_rtc_sync_shadow(void) {
    if(!LL_RTC_IsShadowRegBypassEnabled(RTC)) {
        LL_RTC_ClearFlag_RS(RTC);
        while(!LL_RTC_IsActiveFlag_RS(RTC)) {
        };
    }
}

void furi_hal_rtc_reset_registers(void) {
    for(size_t i = 0; i < RTC_BKP_NUMBER; i++) {
        furi_hal_rtc_set_register(i, 0);
    }

    uint32_t data_reg = 0;
    FuriHalRtcHeader* data = (FuriHalRtcHeader*)&data_reg;
    data->magic = FURI_HAL_RTC_HEADER_MAGIC;
    data->version = FURI_HAL_RTC_HEADER_VERSION;
    furi_hal_rtc_set_register(FuriHalRtcRegisterHeader, data_reg);
}

uint32_t furi_hal_rtc_get_register(FuriHalRtcRegister reg) {
    return LL_RTC_BAK_GetRegister(RTC, reg);
}

void furi_hal_rtc_set_register(FuriHalRtcRegister reg, uint32_t value) {
    LL_RTC_BAK_SetRegister(RTC, reg, value);
}

void furi_hal_rtc_set_log_level(uint8_t level) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->log_level = level;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
    furi_log_set_level(level);
}

uint8_t furi_hal_rtc_get_log_level(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->log_level;
}

void furi_hal_rtc_set_log_device(FuriHalRtcLogDevice device) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->log_device = device;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);

    furi_hal_serial_control_set_logging_config(
        furi_hal_rtc_log_devices[furi_hal_rtc_get_log_device()],
        furi_hal_rtc_log_baud_rates[furi_hal_rtc_get_log_baud_rate()]);
}

FuriHalRtcLogDevice furi_hal_rtc_get_log_device(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->log_device;
}

void furi_hal_rtc_set_log_baud_rate(FuriHalRtcLogBaudRate baud_rate) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->log_baud_rate = baud_rate;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);

    furi_hal_serial_control_set_logging_config(
        furi_hal_rtc_log_devices[furi_hal_rtc_get_log_device()],
        furi_hal_rtc_log_baud_rates[furi_hal_rtc_get_log_baud_rate()]);
}

FuriHalRtcLogBaudRate furi_hal_rtc_get_log_baud_rate(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->log_baud_rate;
}

void furi_hal_rtc_set_flag(FuriHalRtcFlag flag) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->flags |= flag;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);

    if(flag & FuriHalRtcFlagDebug) {
        furi_hal_debug_enable();
    }
}

void furi_hal_rtc_reset_flag(FuriHalRtcFlag flag) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->flags &= ~flag;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);

    if(flag & FuriHalRtcFlagDebug) {
        furi_hal_debug_disable();
    }
}

bool furi_hal_rtc_is_flag_set(FuriHalRtcFlag flag) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->flags & flag;
}

void furi_hal_rtc_set_boot_mode(FuriHalRtcBootMode mode) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->boot_mode = mode;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcBootMode furi_hal_rtc_get_boot_mode(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->boot_mode;
}

void furi_hal_rtc_set_heap_track_mode(FuriHalRtcHeapTrackMode mode) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->heap_track_mode = mode;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcHeapTrackMode furi_hal_rtc_get_heap_track_mode(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->heap_track_mode;
}

void furi_hal_rtc_set_locale_units(FuriHalRtcLocaleUnits value) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->locale_units = value;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcLocaleUnits furi_hal_rtc_get_locale_units(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->locale_units;
}

void furi_hal_rtc_set_locale_timeformat(FuriHalRtcLocaleTimeFormat value) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->locale_timeformat = value;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcLocaleTimeFormat furi_hal_rtc_get_locale_timeformat(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->locale_timeformat;
}

void furi_hal_rtc_set_locale_dateformat(FuriHalRtcLocaleDateFormat value) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    data->locale_dateformat = value;
    furi_hal_rtc_set_register(FuriHalRtcRegisterSystem, data_reg);
}

FuriHalRtcLocaleDateFormat furi_hal_rtc_get_locale_dateformat(void) {
    uint32_t data_reg = furi_hal_rtc_get_register(FuriHalRtcRegisterSystem);
    SystemReg* data = (SystemReg*)&data_reg;
    return data->locale_dateformat;
}

void furi_hal_rtc_set_fault_data(uint32_t value) {
    furi_hal_rtc_set_register(FuriHalRtcRegisterFaultData, value);
}

uint32_t furi_hal_rtc_get_fault_data(void) {
    return furi_hal_rtc_get_register(FuriHalRtcRegisterFaultData);
}

void furi_hal_rtc_set_pin_fails(uint32_t value) {
    furi_hal_rtc_set_register(FuriHalRtcRegisterPinFails, value);
}

uint32_t furi_hal_rtc_get_pin_fails(void) {
    return furi_hal_rtc_get_register(FuriHalRtcRegisterPinFails);
}

void furi_hal_rtc_set_pin_value(uint32_t value) {
    furi_hal_rtc_set_register(FuriHalRtcRegisterPinValue, value);
}

uint32_t furi_hal_rtc_get_pin_value(void) {
    return furi_hal_rtc_get_register(FuriHalRtcRegisterPinValue);
}
