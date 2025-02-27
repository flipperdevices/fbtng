from SCons.Builder import Builder
from SCons.Defaults import Touch


def AddFwFlashTarget(env, targetenv, **kw):
    fwflash_target = env.FwFlash(
        targetenv.File("${BUILD_DIR}/../${TARGET_HW}_${FIRMWARE_BUILD_CFG}_flash.flag"),
        targetenv["FW_ELF"],
        **kw,
    )
    env.Alias(targetenv.subst("${FIRMWARE_BUILD_CFG}_flash"), fwflash_target)
    if env["FORCE"]:
        env.AlwaysBuild(fwflash_target)
    return fwflash_target


def generate(env):
    env.SetDefault(
        FW_FLASH_SCRIPT="${FBT_SCRIPT_DIR}/fwflash.py",
        DEBUG_INTERFACE_SERIAL="auto",
        FW_FLASH_EXTRA_COMMANDS="",
    )

    env.AddMethod(AddFwFlashTarget)

    env.Append(
        BUILDERS={
            "FwFlash": Builder(
                action=[
                    [
                        "${PYTHON3}",
                        "${FW_FLASH_SCRIPT}",
                        "-d" if env["VERBOSE"] else "",
                        "--interface=${DEBUG_INTERFACE}",
                        "--serial=${DEBUG_INTERFACE_SERIAL}",
                        "--platform=${HW_CONFIG_FILE}",
                        "-ex",
                        "${FW_FLASH_EXTRA_COMMANDS}",
                        "${SOURCE}",
                        "${ARGS}",
                    ],
                    Touch("${TARGET}"),
                ]
            ),
        }
    )


def exists(env):
    return True
