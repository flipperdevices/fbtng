from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Defaults import Touch


def AddJFlashTarget(env, targetenv, **kw):
    jflash_target = env.JFlash(
        "#build/jflash-${BUILD_CFG}-flash.flag",
        targetenv["FW_BIN"],
        JFLASHADDR=targetenv.subst("$IMAGE_BASE_ADDRESS"),
        BUILD_CFG=targetenv.subst("${FIRMWARE_BUILD_CFG}"),
        **kw,
    )
    env.Alias(targetenv.subst("${FIRMWARE_BUILD_CFG}_jflash"), jflash_target)
    if env["FORCE"]:
        env.AlwaysBuild(jflash_target)
    return jflash_target


def generate(env):
    env.AddMethod(AddJFlashTarget)

    env.SetDefault(
        JFLASH="JFlash" if env.subst("$PLATFORM") == "win32" else "JFlashExe",
        JFLASHFLAGS=[
            "-auto",
            "-exit",
        ],
    )
    env.Append(
        BUILDERS={
            "JFlash": Builder(
                action=[
                    Action(
                        [
                            [
                                "${JFLASH}",
                                "-openprj${JFLASHPROJECT}",
                                "-open${SOURCE},${JFLASHADDR}",
                                "${JFLASHFLAGS}",
                            ]
                        ]
                    ),
                    Touch("${TARGET}"),
                ],
            ),
        }
    )


def exists(env):
    return True
