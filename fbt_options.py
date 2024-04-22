from pathlib import Path
import posixpath

# For more details on these options, run 'fbt -h'

FIRMWARE_ORIGIN = "Official"

# Default hardware target
TARGET_HW = "u5m1"

# Optimization flags
## Optimize for size
COMPACT = 0
## Optimize for debugging experience
DEBUG = 1

# Suffix to add to files when building distribution
# If OS environment has DIST_SUFFIX set, it will be used instead
DIST_SUFFIX = "local"

# Supported toolchain versions
# Also specify in scripts/ufbt/SConstruct
FBT_TOOLCHAIN_VERSIONS = (" 12.3.", " 13.2.")

OPENOCD_OPTS = [
    "-f",
    "interface/cmsis-dap.cfg",
    # "-c",
    # "transport select hla_swd",
    "-f",
    "${FBT_DEBUG_DIR}/stm32u5x.cfg",
    "-c",
    "stm32u5x.cpu configure -rtos auto",
]

SVD_FILE = "${FBT_DEBUG_DIR}/STM32WB55_CM4.svd"

# Look for blackmagic probe on serial ports and local network
BLACKMAGIC = "auto"

FIRMWARE_APPS = {
    "default": [
        # Svc
        "basic_services",
        # Apps
        "main_apps",
        # "system_apps",
        # Settings
        # "settings_apps",
    ],
    "unit_tests": [
        "basic_services",
        "updater_app",
        "radio_device_cc1101_ext",
        "unit_tests",
    ],
}

FIRMWARE_APP_SET = "default"

custom_options_fn = "fbt_options_local.py"

if Path(custom_options_fn).exists():
    exec(compile(Path(custom_options_fn).read_text(), custom_options_fn, "exec"))
