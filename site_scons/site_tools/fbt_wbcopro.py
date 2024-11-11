from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Defaults import Touch


def AddUsbFlashTarget(env, file_flag, extra_deps, **kw):
    usb_update = env.UsbInstall(
        file_flag,
        (
            env["DIST_DEPENDS"],
            *extra_deps,
        ),
    )
    if env["FORCE"]:
        env.AlwaysBuild(usb_update)
    return usb_update


def DistCommand(env, name, source, **kw):
    target = f"dist_{name}"
    command = env.Command(
        target,
        source,
        action=Action(
            [
                [
                    "${PYTHON3}",
                    "${DIST_SCRIPT}",
                    "copy",
                    "-p",
                    "${DIST_PROJECTS}",
                    "-s",
                    "${DIST_SUFFIX}",
                    "${DIST_EXTRA}",
                ]
            ],
            "${DISTCOMSTR}",
        ),
        **kw,
    )
    env.Pseudo(target)
    env.Alias(name, command)
    return command


def generate(env):
    if not env["VERBOSE"]:
        env.SetDefault(
            COPROCOMSTR="\tCOPRO\t${TARGET}",
            DISTCOMSTR="\tDIST\t${TARGET}",
        )
    env.AddMethod(DistCommand)
    env.AddMethod(AddUsbFlashTarget)

    env.SetDefault(
        COPRO_MCU_FAMILY="STM32WB5x",
        SELFUPDATE_SCRIPT="${FBT_SCRIPT_DIR}/selfupdate.py",
        DIST_SCRIPT="${FBT_SCRIPT_DIR}/sconsdist.py",
        COPRO_ASSETS_SCRIPT="${FBT_SCRIPT_DIR}/assets.py",
    )

    env.Append(
        BUILDERS={
            "UsbInstall": Builder(
                action=[
                    [
                        "${PYTHON3}",
                        "${SELFUPDATE_SCRIPT}",
                        "-p",
                        "${FLIP_PORT}",
                        "${UPDATE_BUNDLE_DIR}/update.fuf",
                        "${ARGS}",
                    ],
                    Touch("${TARGET}"),
                ]
            ),
            "CoproBuilder": Builder(
                action=Action(
                    [
                        [
                            "${PYTHON3}",
                            "${COPRO_ASSETS_SCRIPT}",
                            "copro",
                            "${COPRO_CUBE_DIR}",
                            "${TARGET}",
                            "${COPRO_MCU_FAMILY}",
                            "--cube_ver=${COPRO_CUBE_VERSION}",
                            "--stack_type=${COPRO_STACK_TYPE}",
                            "--stack_file=${COPRO_STACK_BIN}",
                            "--stack_addr=${COPRO_STACK_ADDR}",
                            "${ARGS}",
                        ]
                    ],
                    "${COPROCOMSTR}",
                )
            ),
        }
    )


def exists(env):
    return True
