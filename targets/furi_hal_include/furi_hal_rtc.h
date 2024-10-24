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

#ifdef __cplusplus
}
#endif
