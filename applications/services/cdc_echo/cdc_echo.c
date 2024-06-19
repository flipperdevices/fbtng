#include <furi.h>
#include <furi_hal_gpio.h>
#include <furi_hal_resources.h>
#include "furi_hal_usb_cdc.h"

#define TAG "CDC test"

static void cdc_tx_cb(uint8_t itf, void* context) {
    UNUSED(itf);
    UNUSED(context);
    // FURI_LOG_I(TAG, "tx_cb");
}

static void cdc_rx_cb(uint8_t itf, void* context) {
    UNUSED(itf);
    FuriThreadId thread_id = context;
    furi_thread_flags_set(thread_id, 1);
    // FURI_LOG_I(TAG, "rx_cb");
}

static void cdc_ctr_line(uint8_t itf, bool dtr, bool rts, void* context) {
    UNUSED(itf);
    UNUSED(context);
    FURI_LOG_I(TAG, "ctr_line dtr:%u rts:%u", dtr, rts);
}

static const char parity_name[] = {'N', 'O', 'E', 'M', 'S'};
static const char* stop_name[] = {"1", "1.5", "2"};

static void cdc_config(uint8_t itf, CdcLineCoding* config, void* context) {
    UNUSED(itf);
    UNUSED(context);
    furi_assert(config->parity <= 4);
    furi_assert(config->stop_bits <= 2);
    FURI_LOG_I(
        TAG,
        "config %lu/%u%c%s",
        config->bit_rate,
        config->data_bits,
        parity_name[config->parity],
        stop_name[config->stop_bits]);
}

static const CdcCallbacks cdc_cb = {
    .rx_callback = cdc_rx_cb,
    .tx_done_callback = cdc_tx_cb,
    .ctrl_line_callback = cdc_ctr_line,
    .config_callback = cdc_config,
};

#define RX_BUF_LEN 512
int32_t cdc_echo(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Started");
    uint8_t* rx_buf = malloc(RX_BUF_LEN);

    while(1) {
        CdcContext cdc_cfg = {
            .callbacks = &cdc_cb,
            .context = furi_thread_get_current_id(),
        };
        furi_hal_usb_set_config(&usb_cdc, &cdc_cfg);

        while(1) {
            furi_thread_flags_wait(1, FuriFlagWaitAny, FuriWaitForever);
            while(furi_hal_cdc_available(0)) {
                uint32_t count = furi_hal_cdc_read(0, rx_buf, RX_BUF_LEN);

                furi_hal_cdc_write(0, rx_buf, count);
                furi_hal_cdc_write_flush(0);
                // if(buf[0] == 'q') {
                //     FURI_LOG_I(TAG, "Reconnect");
                //     break;
                // }
            }
        }
        furi_hal_usb_set_config(NULL, NULL);
    }

    free(rx_buf);

    return 0;
}
