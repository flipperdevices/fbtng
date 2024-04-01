#include <furi.h>
#include <furi_hal.h>
#include "fatfs.h"

#define TAG "Storage"

typedef struct {
    FuriEventFlag* event_flag;
    bool sd_present;
} Storage;

typedef enum {
    StorageEventPresenceChanged = 1 << 0,

    StorageEventAll = StorageEventPresenceChanged,
} StorageEvent;

static void storage_sdmmc_present_callback(void* context) {
    Storage* storage = (Storage*)context;
    furi_event_flag_set(storage->event_flag, StorageEventPresenceChanged);
}

static bool storage_init_card(size_t retries) {
    while(retries--) {
        if(furi_hal_sdmmc_init_card()) {
            return true;
        }
        furi_delay_ms(100);
    }

    return false;
}

FATFS* fs = &fatfs_object;
const char* path = "0:/";
const char* filename = "benchmark.bin";

static void storage_benchmark_write(size_t blocks) {
    size_t buffer_size = blocks * 512;

    FIL file;
    FRESULT status = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if(status != FR_OK) {
        FURI_LOG_E(TAG, "Failed to open file: %d", status);
        return;
    }

    uint8_t* buffer = malloc(buffer_size);
    for(size_t i = 0; i < buffer_size; i++) {
        buffer[i] = i % 256;
    }

    uint32_t start = DWT->CYCCNT;
    uint32_t end;
    bool error = false;
    const size_t iterations = 10;

    for(size_t i = 0; i < iterations; i++) {
        UINT bytes_written;
        status = f_write(&file, buffer, buffer_size, &bytes_written);
        if(status != FR_OK) {
            FURI_LOG_E(TAG, "Failed to write: %d", status);
            error = true;
            break;
        }

        if(bytes_written != buffer_size) {
            FURI_LOG_E(TAG, "Tried to write %zu bytes, wrote %lu", buffer_size, bytes_written);
            error = true;
            break;
        }
    }

    end = DWT->CYCCNT;

    if(error) {
        FURI_LOG_E(TAG, "Failed to write %zu blocks", blocks);
    } else {
        float seconds = (float)(end - start) / furi_hal_cortex_instructions_per_microsecond() /
                        1000000 / iterations;
        float speed_kb = (float)(buffer_size) / seconds / 1024;
        FURI_LOG_I(
            TAG,
            "Write %zu bytes took %f s, speed %0.2f kb/s (%0.2f Mbit/s)",
            buffer_size,
            (double)seconds,
            (double)speed_kb,
            (double)speed_kb * 8 / 1024);
    }

    f_close(&file);

    free(buffer);
}

static void storage_benchmark_read(size_t blocks) {
    size_t buffer_size = blocks * 512;

    FIL file;
    FRESULT status = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
    if(status != FR_OK) {
        FURI_LOG_E(TAG, "Failed to open file: %d", status);
        return;
    }

    uint8_t* buffer = malloc(buffer_size);

    uint32_t start = DWT->CYCCNT;
    uint32_t end;
    bool error = false;
    const size_t iterations = 10;

    for(size_t i = 0; i < iterations; i++) {
        UINT bytes_read;
        status = f_read(&file, buffer, buffer_size, &bytes_read);
        if(status != FR_OK) {
            FURI_LOG_E(TAG, "Failed to read: %d", status);
            error = true;
            break;
        }

        if(bytes_read != buffer_size) {
            FURI_LOG_E(TAG, "Tried to read %zu bytes, read %lu", buffer_size, bytes_read);
            error = true;
            break;
        }
    }

    end = DWT->CYCCNT;

    if(error) {
        FURI_LOG_E(TAG, "Failed to read %zu blocks", blocks);
    } else {
        float seconds = (float)(end - start) / furi_hal_cortex_instructions_per_microsecond() /
                        1000000 / iterations;
        float speed_kb = (float)(buffer_size) / seconds / 1024;
        FURI_LOG_I(
            TAG,
            "Read %zu bytes took %f s, speed %0.2f kb/s (%0.2f Mbit/s)",
            buffer_size,
            (double)seconds,
            (double)speed_kb,
            (double)speed_kb * 8 / 1024);

        for(size_t i = 0; i < buffer_size; i++) {
            if(buffer[i] != i % 256) {
                FURI_LOG_E(TAG, "Data mismatch at address %zu: %u != %u", i, buffer[i], i % 256);
                break;
            }
        }
    }

    f_close(&file);
    free(buffer);
}

static void storage_benchmark(size_t blocks) {
    storage_benchmark_write(blocks);
    storage_benchmark_read(blocks);
}

static void storage_print_card_info(void) {
    FuriHalSdInfo info;
    if(furi_hal_sdmmc_get_card_info(&info)) {
        const char* version = "Unknown";
        switch(info.version) {
        case FuriHalSdVersion1:
            version = "1.0";
            break;
        case FuriHalSdVersion2:
            version = "2.0";
            break;
        }

        const char* type = "Unknown";
        switch(info.type) {
        case FuriHalSdTypeSC:
            type = "Standard Capacity";
            break;
        case FuriHalSdTypeHCXC:
            type = "High Capacity or Extended Capacity";
            break;
        }

        const char* speed = "Unknown";
        switch(info.speed) {
        case FuriHalSdSpeedNormal:
            speed = "Normal";
            break;
        case FuriHalSdSpeedHigh:
            speed = "High";
            break;
        case FuriHalSdSpeedUltraHigh:
            speed = "Ultra High";
            break;
        }

        uint64_t card_size_mb =
            (uint64_t)info.logical_block_count * info.logical_block_size / 1024 / 1024;
        FURI_LOG_D(TAG, "Card size %llu MB", card_size_mb);
        FURI_LOG_D(TAG, "Block count: %lu", info.logical_block_count);
        FURI_LOG_D(TAG, "Block size: %lu", info.logical_block_size);
        FURI_LOG_D(TAG, "Version: %s", version);
        FURI_LOG_D(TAG, "Type: %s", type);
        FURI_LOG_D(TAG, "Speed: %s", speed);
        FURI_LOG_D(TAG, "Manufacturer ID: 0x%02x", info.manufacturer_id);
        FURI_LOG_D(TAG, "OEM ID: %s", info.oem_id);
        FURI_LOG_D(TAG, "Product name: %s", info.product_name);
        FURI_LOG_D(
            TAG,
            "Product revision: %u.%u",
            info.product_revision_major,
            info.product_revision_minor);
        FURI_LOG_D(TAG, "Product serial number: %lu", info.product_serial_number);
        FURI_LOG_D(
            TAG,
            "Manufacturing date: %02u.%04u",
            info.manufacturing_month,
            info.manufacturing_year);

    } else {
        FURI_LOG_E(TAG, "Failed to get card info");
    }
}

static void storage_check_presence(Storage* storage) {
    bool sd_present = furi_hal_is_sd_present();
    if(sd_present != storage->sd_present) {
        storage->sd_present = sd_present;
        if(storage->sd_present) {
            FURI_LOG_I(TAG, "SD card is inserted");
            if(storage_init_card(10)) {
                FURI_LOG_I(TAG, "SD card initialized");

                storage_print_card_info();

                fatfs_init();
                FRESULT status;
                status = f_mount(fs, path, 1);

                if(status == FR_OK) {
                    FURI_LOG_I(TAG, "SD card mounted");
                } else {
                    FURI_LOG_E(TAG, "Failed to mount: %d", status);
                }

                if(status == FR_NO_FILESYSTEM) {
                    FURI_LOG_I(TAG, "Creating filesystem");
                    const size_t buffer_size = 512 * 8;

                    uint8_t* buffer = malloc(buffer_size);
                    status = f_mkfs(path, FM_ANY, 0, buffer, buffer_size);
                    free(buffer);

                    if(status == FR_OK) {
                        FURI_LOG_I(TAG, "Filesystem created");
                        status = f_setlabel("Flipper SD");
                        if(status == FR_OK) {
                            FURI_LOG_I(TAG, "Volume label set");
                            status = f_mount(fs, path, 1);
                            if(status == FR_OK) {
                                FURI_LOG_I(TAG, "SD card mounted");
                            } else {
                                FURI_LOG_E(TAG, "Failed to mount: %d", status);
                            }
                        } else {
                            FURI_LOG_E(TAG, "Failed to set volume label: %d", status);
                        }

                    } else {
                        FURI_LOG_E(TAG, "Failed to create filesystem: %d", status);
                    }
                }

                {
                    DIR dir;
                    FILINFO fno;
                    FRESULT res;

                    res = f_opendir(&dir, path);
                    if(res == FR_OK) {
                        FURI_LOG_D(TAG, "Directory listing:");
                        for(;;) {
                            res = f_readdir(&dir, &fno);
                            if(res != FR_OK || fno.fname[0] == 0) {
                                break;
                            }
                            FURI_LOG_D(TAG, "%s", fno.fname);
                        }
                        f_closedir(&dir);
                    } else {
                        FURI_LOG_E(TAG, "Failed to open directory: %d", res);
                    }

                    storage_benchmark(1);
                    storage_benchmark(2);
                    storage_benchmark(5);
                    storage_benchmark(10);
                    storage_benchmark(50);
                    storage_benchmark(64);
                    storage_benchmark(128);
                    storage_benchmark(256);
                    FURI_LOG_I(TAG, "Benchmark finished");
                }

            } else {
                FURI_LOG_E(TAG, "Failed to initialize SD card");
            }
        } else {
            FURI_LOG_I(TAG, "SD card is removed");
            furi_hal_sdmmc_deinit_card();
            f_mount(NULL, path, 0);
        }
    }
}

int32_t storage(void* p) {
    UNUSED(p);

    Storage* storage = malloc(sizeof(Storage));
    storage->event_flag = furi_event_flag_alloc();

    FURI_LOG_I(TAG, "Started");

    storage->sd_present = false;
    storage_check_presence(storage);

    furi_hal_sdmmc_set_presence_callback(storage_sdmmc_present_callback, storage);

    while(true) {
        uint32_t flags = furi_event_flag_wait(
            storage->event_flag, StorageEventAll, FuriFlagWaitAny, FuriWaitForever);
        if((flags & FuriFlagError) == 0) {
            if(flags & StorageEventPresenceChanged) {
                // TODO: add debounce circuit?
                furi_delay_ms(5);
                storage_check_presence(storage);
            }
        }
    }

    return 0;
}