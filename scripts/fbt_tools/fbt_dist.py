from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Defaults import Touch


def GetProjetDirName(env, project=None):
    parts = [env.subst("f${TARGET_HW}")]
    if project:
        parts.append(project)

    suffix = ""
    if env["DEBUG"]:
        suffix += "D"
    if env["COMPACT"]:
        suffix += "C"
    if suffix:
        parts.append(suffix)

    return "-".join(parts)


def create_fw_build_targets(env, configuration_name):
    flavor = GetProjetDirName(env, configuration_name)
    build_dir = env.Dir("build").Dir(flavor)
    return env.SConscript(
        "firmware.scons",
        variant_dir=build_dir,
        duplicate=0,
        exports={
            "ENV": env,
            "fw_build_meta": {
                "type": configuration_name,
                "flavor": flavor,
                "build_dir": build_dir,
            },
        },
    )


def AddFwProject(env, base_env, fw_type, fw_env_key):
    project_env = env[fw_env_key] = create_fw_build_targets(base_env, fw_type)
    env.Append(
        DIST_PROJECTS=[
            project_env["FW_FLAVOR"],
        ],
        DIST_DEPENDS=[
            project_env["FW_ARTIFACTS"],
        ],
    )

    env.Replace(DIST_DIR=env.GetProjetDirName())
    return project_env


def AddFwFlashTarget(env, targetenv, **kw):
    fwflash_target = env.FwFlash(
        "#build/${FIRMWARE_BUILD_CFG}_flash.flag",
        targetenv["FW_ELF"],
        **kw,
    )
    env.Alias(targetenv.subst("${FIRMWARE_BUILD_CFG}_flash"), fwflash_target)
    if env["FORCE"]:
        env.AlwaysBuild(fwflash_target)
    return fwflash_target


def generate(env):
    if not env["VERBOSE"]:
        env.SetDefault(
            DISTCOMSTR="\tDIST\t${TARGET}",
        )
    env.AddMethod(AddFwProject)
    env.AddMethod(AddFwFlashTarget)
    env.AddMethod(GetProjetDirName)

    env.SetDefault(
        FW_FLASH_SCRIPT="${FBT_SCRIPT_DIR}/fwflash.py",
        FW_FLASH_TARGET_INTEFACE="auto",
        DEBUG_INTERFACE_SERIAL="auto",
        FW_FLASH_EXTRA_COMMANDS="",
    )

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
                        "--target=${FW_FLASH_TARGET_INTEFACE}",
                        '--extra-commands="${FW_FLASH_EXTRA_COMMANDS}"',
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
