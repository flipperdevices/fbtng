#include "furi_hal_serial_control.h"
#include "furi_hal_serial_types_i.h"
#include "furi_hal_serial.h"

#include <furi.h>

#define TAG "FuriHalSerialControl"

typedef struct {
    FuriHalSerialHandle handles[FuriHalSerialIdMax];
} FuriHalSerialControl;

FuriHalSerialControl* furi_hal_serial_control = NULL;

void furi_hal_serial_control_init(void) {
    furi_check(!furi_hal_serial_control, "FuriHalSerialControl already initialized");
    furi_hal_serial_control = malloc(sizeof(FuriHalSerialControl));

    for(size_t i = 0; i < FuriHalSerialIdMax; i++) {
        furi_hal_serial_control->handles[i].id = i;
        furi_hal_serial_control->handles[i].in_use = false;
    }
}

void furi_hal_serial_control_deinit(void) {
    furi_check(furi_hal_serial_control, "FuriHalSerialControl not initialized");

    for(size_t i = 0; i < FuriHalSerialIdMax; i++) {
        furi_hal_serial_deinit(&furi_hal_serial_control->handles[i]);
    }

    free(furi_hal_serial_control);
    furi_hal_serial_control = NULL;
}

void furi_hal_serial_control_suspend(void) {
    furi_check(furi_hal_serial_control, "FuriHalSerialControl not initialized");

    for(size_t i = 0; i < FuriHalSerialIdMax; i++) {
        if(furi_hal_serial_control->handles[i].in_use) {
            furi_hal_serial_tx_wait_complete(&furi_hal_serial_control->handles[i]);
            furi_hal_serial_suspend(&furi_hal_serial_control->handles[i]);
        }
    }
}

void furi_hal_serial_control_resume(void) {
    furi_check(furi_hal_serial_control, "FuriHalSerialControl not initialized");

    for(size_t i = 0; i < FuriHalSerialIdMax; i++) {
        if(furi_hal_serial_control->handles[i].in_use) {
            furi_hal_serial_resume(&furi_hal_serial_control->handles[i]);
        }
    }
}

FuriHalSerialHandle* furi_hal_serial_control_acquire(FuriHalSerialId serial_id) {
    furi_check(furi_hal_serial_control, "FuriHalSerialControl not initialized");

    FuriHalSerialHandle* output = NULL;
    if(serial_id <= FuriHalSerialIdMax) {
        FuriHalSerialHandle* handle = &furi_hal_serial_control->handles[serial_id];
        if(!handle->in_use) {
            handle->in_use = true;
            output = handle;
            //forced deinit if someone used the device to bypass control
            furi_hal_serial_deinit(handle);
        }
    }
    return output;
}

void furi_hal_serial_control_release(FuriHalSerialHandle* handle) {
    furi_check(furi_hal_serial_control, "FuriHalSerialControl not initialized");
    furi_check(handle, "FuriHalSerialHandle is null");
    furi_check(
        furi_hal_serial_control->handles[handle->id].in_use, "FuriHalSerialHandle not in use");

    furi_hal_serial_control->handles[handle->id].in_use = false;
    //forced deinit if the user forgot
    furi_hal_serial_deinit(handle);
}

bool furi_hal_serial_control_is_busy(FuriHalSerialId serial_id) {
    furi_check(furi_hal_serial_control, "FuriHalSerialControl not initialized");
    return furi_hal_serial_control->handles[serial_id].in_use;
}
