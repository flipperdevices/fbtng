#include <furi_hal_rtc.h>
#include <furi.h>

void furi_hal_rtc_reset_flag(FuriHalRtcFlag flag) {
    UNUSED(flag);
    // TODO: Implement furi_hal_rtc_reset_flag
}
bool furi_hal_rtc_is_flag_set(FuriHalRtcFlag flag) {
    UNUSED(flag);
    return false;
    // TODO: Implement furi_hal_rtc_is_flag_set
}

uint32_t furi_hal_rtc_get_timestamp(void) {
    // TODO: implement
    return 0;
}

FuriHalRtcBootMode furi_hal_rtc_get_boot_mode(void) {
    // TODO: implement
    return FuriHalRtcBootModeNormal;
}

void furi_hal_rtc_set_fault_data(uint32_t value) {
    UNUSED(value);
    // TODO: implement
}
