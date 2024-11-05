#include <furi_hal_interrupt.h>
#include <furi_hal_rtc.h>
#include <furi_hal_nvm.h>
#include <furi_hal_light.h>

#include <stm32wbxx_ll_pwr.h>
#include <stm32wbxx_ll_bus.h>
#include <stm32wbxx_ll_rcc.h>
#include <stm32wbxx_ll_rtc.h>
#include <stm32wbxx_ll_utils.h>

#include <furi.h>

#define TAG "FuriHalRtc"

#define FURI_HAL_RTC_LSE_STARTUP_TIME 300

#define FURI_HAL_RTC_CLOCK_IS_READY() (LL_RCC_LSE_IsReady() && LL_RCC_LSI1_IsReady())

typedef struct {
    FuriHalRtcAlarmCallback alarm_callback;
    void* alarm_callback_context;
} FuriHalRtc;

static FuriHalRtc furi_hal_rtc = {};

static void furi_hal_rtc_enter_init_mode(void) {
    LL_RTC_EnableInitMode(RTC);
    while(LL_RTC_IsActiveFlag_INIT(RTC) != 1)
        ;
}

static void furi_hal_rtc_exit_init_mode(void) {
    LL_RTC_DisableInitMode(RTC);
    furi_hal_rtc_sync_shadow();
}

static void furi_hal_rtc_reset(void) {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
}

static bool furi_hal_rtc_start_clock_and_switch(void) {
    // Clock operation require access to Backup Domain
    LL_PWR_EnableBkUpAccess();

    // Enable LSI and LSE
    LL_RCC_LSI1_Enable();
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_HIGH);
    LL_RCC_LSE_Enable();

    // Wait for LSI and LSE startup
    uint32_t c = 0;
    while(!FURI_HAL_RTC_CLOCK_IS_READY() && c < FURI_HAL_RTC_LSE_STARTUP_TIME) {
        LL_mDelay(1);
        c++;
    }

    if(FURI_HAL_RTC_CLOCK_IS_READY()) {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
        LL_RCC_EnableRTC();
        return LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSE;
    } else {
        return false;
    }
}

static void furi_hal_rtc_recover(void) {
    DateTime datetime = {0};

    // Handle fixable LSE failure
    if(LL_RCC_LSE_IsCSSDetected()) {
        furi_hal_light_sequence("rgb B");
        // Shutdown LSE and LSECSS
        LL_RCC_LSE_DisableCSS();
        LL_RCC_LSE_Disable();
    } else {
        furi_hal_light_sequence("rgb R");
    }

    // Temporary switch to LSI
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    if(LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSI) {
        // Get datetime before RTC Domain reset
        furi_hal_rtc_get_datetime(&datetime);
    }

    // Reset RTC Domain
    furi_hal_rtc_reset();

    // Start Clock
    if(!furi_hal_rtc_start_clock_and_switch()) {
        // Plan C: reset RTC and restart
        furi_hal_light_sequence("rgb R.r.R.r.R.r");
        furi_hal_rtc_reset();
        NVIC_SystemReset();
    }

    // Set date if it valid
    if(datetime.year != 0) {
        furi_hal_rtc_set_datetime(&datetime);
    }
}

static void furi_hal_rtc_alarm_handler(void* context) {
    UNUSED(context);

    if(LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
        /* Clear the Alarm interrupt pending bit */
        LL_RTC_ClearFlag_ALRA(RTC);

        /* Alarm callback */
        furi_check(furi_hal_rtc.alarm_callback);
        furi_hal_rtc.alarm_callback(furi_hal_rtc.alarm_callback_context);
    }
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
}

static void furi_hal_rtc_set_alarm_out(bool enable) {
    FURI_CRITICAL_ENTER();
    LL_RTC_DisableWriteProtection(RTC);
    if(enable) {
        LL_RTC_SetAlarmOutEvent(RTC, LL_RTC_ALARMOUT_ALMA);
        LL_RTC_SetOutputPolarity(RTC, LL_RTC_OUTPUTPOLARITY_PIN_LOW);
        LL_RTC_SetAlarmOutputType(RTC, LL_RTC_ALARM_OUTPUTTYPE_OPENDRAIN);
    } else {
        LL_RTC_SetAlarmOutEvent(RTC, LL_RTC_ALARMOUT_DISABLE);
        LL_RTC_SetOutputPolarity(RTC, LL_RTC_OUTPUTPOLARITY_PIN_LOW);
        LL_RTC_SetAlarmOutputType(RTC, LL_RTC_ALARM_OUTPUTTYPE_OPENDRAIN);
    }
    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

void furi_hal_rtc_init_early(void) {
    // Enable RTCAPB clock
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTCAPB);

    // Prepare clock
    if(!furi_hal_rtc_start_clock_and_switch()) {
        // Plan B: try to recover
        furi_hal_rtc_recover();
    }
}

void furi_hal_rtc_deinit_early(void) {
}

void furi_hal_rtc_init(void) {
    LL_RTC_InitTypeDef RTC_InitStruct;
    RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
    RTC_InitStruct.AsynchPrescaler = 127;
    RTC_InitStruct.SynchPrescaler = 255;
    LL_RTC_Init(RTC, &RTC_InitStruct);

    FURI_LOG_I(TAG, "Init OK");
    furi_hal_rtc_set_alarm_out(false);
}

void furi_hal_rtc_prepare_for_shutdown(void) {
    furi_hal_rtc_set_alarm_out(true);
}

void furi_hal_rtc_set_datetime(DateTime* datetime) {
    furi_check(!FURI_IS_IRQ_MODE());
    furi_check(datetime);

    FURI_CRITICAL_ENTER();
    /* Disable write protection */
    LL_RTC_DisableWriteProtection(RTC);

    /* Enter Initialization mode and wait for INIT flag to be set */
    furi_hal_rtc_enter_init_mode();

    /* Set time */
    LL_RTC_TIME_Config(
        RTC,
        LL_RTC_TIME_FORMAT_AM_OR_24,
        __LL_RTC_CONVERT_BIN2BCD(datetime->hour),
        __LL_RTC_CONVERT_BIN2BCD(datetime->minute),
        __LL_RTC_CONVERT_BIN2BCD(datetime->second));

    /* Set date */
    LL_RTC_DATE_Config(
        RTC,
        datetime->weekday,
        __LL_RTC_CONVERT_BIN2BCD(datetime->day),
        __LL_RTC_CONVERT_BIN2BCD(datetime->month),
        __LL_RTC_CONVERT_BIN2BCD(datetime->year - 2000));

    /* Exit Initialization mode */
    furi_hal_rtc_exit_init_mode();

    /* Enable write protection */
    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

void furi_hal_rtc_get_datetime(DateTime* datetime) {
    furi_check(!FURI_IS_IRQ_MODE());
    furi_check(datetime);

    FURI_CRITICAL_ENTER();
    uint32_t time = LL_RTC_TIME_Get(RTC); // 0x00HHMMSS
    uint32_t date = LL_RTC_DATE_Get(RTC); // 0xWWDDMMYY
    FURI_CRITICAL_EXIT();

    datetime->second = __LL_RTC_CONVERT_BCD2BIN((time >> 0) & 0xFF);
    datetime->minute = __LL_RTC_CONVERT_BCD2BIN((time >> 8) & 0xFF);
    datetime->hour = __LL_RTC_CONVERT_BCD2BIN((time >> 16) & 0xFF);
    datetime->year = __LL_RTC_CONVERT_BCD2BIN((date >> 0) & 0xFF) + 2000;
    datetime->month = __LL_RTC_CONVERT_BCD2BIN((date >> 8) & 0xFF);
    datetime->day = __LL_RTC_CONVERT_BCD2BIN((date >> 16) & 0xFF);
    datetime->weekday = __LL_RTC_CONVERT_BCD2BIN((date >> 24) & 0xFF);
}

void furi_hal_rtc_set_alarm(const DateTime* datetime, bool enabled) {
    furi_check(!FURI_IS_IRQ_MODE());

    FURI_CRITICAL_ENTER();
    LL_RTC_DisableWriteProtection(RTC);

    if(datetime) {
        LL_RTC_ALMA_ConfigTime(
            RTC,
            LL_RTC_ALMA_TIME_FORMAT_AM,
            __LL_RTC_CONVERT_BIN2BCD(datetime->hour),
            __LL_RTC_CONVERT_BIN2BCD(datetime->minute),
            __LL_RTC_CONVERT_BIN2BCD(datetime->second));
        LL_RTC_ALMA_SetMask(RTC, LL_RTC_ALMA_MASK_DATEWEEKDAY);
    }

    if(enabled) {
        LL_RTC_ClearFlag_ALRA(RTC);
        LL_RTC_ALMA_Enable(RTC);
    } else {
        LL_RTC_ALMA_Disable(RTC);
        LL_RTC_ClearFlag_ALRA(RTC);
    }

    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

bool furi_hal_rtc_get_alarm(DateTime* datetime) {
    furi_check(datetime);

    memset(datetime, 0, sizeof(DateTime));

    datetime->hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_ALMA_GetHour(RTC));
    datetime->minute = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_ALMA_GetMinute(RTC));
    datetime->second = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_ALMA_GetSecond(RTC));

    return READ_BIT(RTC->CR, RTC_CR_ALRAE);
}

void furi_hal_rtc_set_alarm_callback(FuriHalRtcAlarmCallback callback, void* context) {
    FURI_CRITICAL_ENTER();
    LL_RTC_DisableWriteProtection(RTC);
    if(callback) {
        furi_check(!furi_hal_rtc.alarm_callback);
        // Set our callbacks
        furi_hal_rtc.alarm_callback = callback;
        furi_hal_rtc.alarm_callback_context = context;
        // Enable RTC ISR
        furi_hal_interrupt_set_isr(FuriHalInterruptIdRtcAlarm, furi_hal_rtc_alarm_handler, NULL);
        // Hello EXTI my old friend
        // Chain: RTC->LINE-17->EXTI->NVIC->FuriHalInterruptIdRtcAlarm
        LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);
        LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
        // Enable alarm interrupt
        LL_RTC_EnableIT_ALRA(RTC);
        // Force trigger
        furi_hal_rtc_alarm_handler(NULL);
    } else {
        furi_check(furi_hal_rtc.alarm_callback);
        // Cleanup EXTI flags and config
        LL_EXTI_DisableIT_0_31(LL_EXTI_LINE_17);
        LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_17);
        // Cleanup NVIC flags and config
        furi_hal_interrupt_set_isr(FuriHalInterruptIdRtcAlarm, NULL, NULL);
        // Disable alarm interrupt
        LL_RTC_DisableIT_ALRA(RTC);

        furi_hal_rtc.alarm_callback = NULL;
        furi_hal_rtc.alarm_callback_context = NULL;
    }
    LL_RTC_EnableWriteProtection(RTC);
    FURI_CRITICAL_EXIT();
}

uint32_t furi_hal_rtc_get_timestamp(void) {
    DateTime datetime = {0};
    furi_hal_rtc_get_datetime(&datetime);
    return datetime_datetime_to_timestamp(&datetime);
}
