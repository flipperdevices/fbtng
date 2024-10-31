/**
 * @file furi_hal_rtc.h
 * Furi Hal RTC API
 */

#pragma once

#include <stdint.h>
#include <datetime/datetime.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Early initialization */
void furi_hal_rtc_init_early(void);

/** Early de-initialization */
void furi_hal_rtc_deinit_early(void);

/** Initialize RTC subsystem */
void furi_hal_rtc_init(void);

/** Prepare system for shutdown
 *
 * This function must be called before system sent to transport mode(power off).
 * FlipperZero implementation configures and enables ALARM output on pin PC13
 * (Back button). This allows the system to wake-up charger from transport mode.
 */
void furi_hal_rtc_prepare_for_shutdown(void);

/** Set RTC Date Time
 *
 * @param      datetime  The date time to set
 */
void furi_hal_rtc_set_datetime(DateTime* datetime);

/** Get RTC Date Time
 *
 * @param      datetime  The datetime
 */
void furi_hal_rtc_get_datetime(DateTime* datetime);

/** Get UNIX Timestamp
 *
 * @return     Unix Timestamp in seconds from UNIX epoch start
 */
uint32_t furi_hal_rtc_get_timestamp(void);

/** Set alarm
 *
 * @param[in]  datetime  The date time to set or NULL if time change is not needed
 * @param[in]  enabled   Indicates if alarm must be enabled or disabled
 */
void furi_hal_rtc_set_alarm(const DateTime* datetime, bool enabled);

/** Get alarm
 *
 * @param      datetime  Pointer to DateTime object
 *
 * @return     true if alarm was set, false otherwise
 */
bool furi_hal_rtc_get_alarm(DateTime* datetime);

/** Furi HAL RTC alarm callback signature */
typedef void (*FuriHalRtcAlarmCallback)(void* context);

/** Set alarm callback
 *
 * Use it to subscribe to alarm trigger event. Setting alarm callback is
 * independent from setting alarm.
 *
 * @warning    Normally this callback will be delivered from the ISR, however we may
 *             deliver it while this function is called. This happens when
 *             the alarm has already triggered, but there was no ISR set.
 *
 * @param[in]  callback  The callback
 * @param      context   The context
 */
void furi_hal_rtc_set_alarm_callback(FuriHalRtcAlarmCallback callback, void* context);

#ifdef __cplusplus
}
#endif
