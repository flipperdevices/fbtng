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


def create_fw_build_targets(env, configuration_name, extra_params):
    flavor = GetProjetDirName(env, configuration_name)
    build_dir = env.Dir("#build").Dir(flavor)
    fw_build_meta = dict(extra_params or {})
    fw_build_meta.update(
        {
            "type": configuration_name,
            "flavor": flavor,
            "build_dir": build_dir,
        }
    )

    return env.SConscript(
        "#firmware.scons",
        variant_dir=build_dir,  # FIXME
        duplicate=0,
        exports={
            "FW_ENV": env,
            "fw_build_meta": fw_build_meta,
        },
    )


def AddFwProject(env, base_env, fw_type, fw_env_key, extra_params=None):
    project_env = env[fw_env_key] = create_fw_build_targets(
        base_env, fw_type, extra_params
    )
    env.Append(
        DIST_PROJECTS=[
            project_env["FW_FLAVOR"],
        ],
        DIST_DEPENDS=[
            project_env["FW_ARTIFACTS"],
        ],
    )

    env.SetDefault(
        F_TARGET_HW=project_env["F_TARGET_HW"],
        DIST_DIR=env.GetProjetDirName(),
        UPDATE_BUNDLE_DIR="dist/${DIST_DIR}/${F_TARGET_HW}-update-${DIST_SUFFIX}",
    )

    return project_env


def generate(env):
    if not env["VERBOSE"]:
        env.SetDefault(
            DISTCOMSTR="\tDIST\t${TARGET}",
        )
    env.AddMethod(AddFwProject)
    env.AddMethod(GetProjetDirName)


def exists(env):
    return True
