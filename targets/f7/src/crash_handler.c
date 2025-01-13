#include <furi/core/check.h>
#include <furi/core/log.h>
#include <furi_hal_bt.h>

void furi_crash_handler() {
    furi_hal_bt_log_c2_state();
}
