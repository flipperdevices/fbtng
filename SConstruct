#
# Main Flipper Build System entry point
#
# This file is evaluated by scons (the build system) every time fbt is invoked.
# Scons constructs all referenced environments & their targets' dependency
# trees on startup. So, to keep startup time as low as possible, we're hiding
# construction of certain targets behind command-line options.

import os

DefaultEnvironment(tools=[])

EnsurePythonVersion(3, 8)

# Progress(["OwO\r", "owo\r", "uwu\r", "owo\r"], interval=15)

# This environment is created only for loading options & validating file/dir existence
fbt_variables = SConscript("site_scons/commandline.scons")
cmd_environment = Environment(
    toolpath=["#/scripts/fbt_tools"],
    tools=[
        ("fbt_help", {"vars": fbt_variables}),
    ],
    variables=fbt_variables,
)

target_bootstrap_env = cmd_environment.Clone(
    tools=[
        "sconsrecursiveglob",
        "fbt_hwtarget",
    ],
    F_TARGET_HW="f${TARGET_HW}",
    TARGETS_ROOT=Dir("#/targets"),
)
target_bootstrap_env.ConfigureForTarget(lightweight=True)
fbt_variables = target_bootstrap_env.ConfigureVariables(fbt_variables)

fbt_variables.Update(target_bootstrap_env)
if fbt_variables.UnknownVariables():
    print("Unrecognized command-line variables:")
    print(fbt_variables.UnknownVariables())

# print(fbt_variables)
# print()
# print(target_bootstrap_env.Dump())


# Building basic environment - tools, utility methods, cross-compilation
# settings, gcc flags for Cortex-M4, basic builders and more
coreenv = SConscript(
    "site_scons/environ.scons",
    exports={"VAR_ENV": cmd_environment},
    toolpath=["#/scripts/fbt_tools"],
)

keyboard_env = coreenv.Clone()
SConscript(
    "#companions/keyboard/SConscript",
    exports={"ENV": keyboard_env},
    variant_dir=Dir("#build/companions/keyboard"),
    duplicate=0,
)

# Create a separate "dist" environment and add construction envs to it
distenv = coreenv.Clone(
    tools=[
        "fbt_dist",
        "fbt_debugopts",
        "blackmagic",
        "doxygen",
        "textfile",
    ],
    ENV=os.environ,
    UPDATE_BUNDLE_DIR="dist/${DIST_DIR}/${F_TARGET_HW}-update-${DIST_SUFFIX}",
)

firmware_env = distenv.AddFwProject(
    base_env=coreenv,
    fw_type="firmware",
    fw_env_key="FW_ENV",
)

distenv.Default(firmware_env["FW_ARTIFACTS"])
# Target for copying & renaming binaries to dist folder
# basic_dist = distenv.DistCommand("fw_dist", distenv["DIST_DEPENDS"])
# distenv.Default(basic_dist)

dist_dir_name = distenv.GetProjetDirName()
dist_dir = distenv.Dir(f"#/dist/{dist_dir_name}")
external_apps_artifacts = firmware_env["FW_EXTAPPS"]
external_app_list = external_apps_artifacts.application_map.values()

fap_dist = [
    distenv.Install(
        dist_dir.Dir("debug_elf"),
        list(app_artifact.debug for app_artifact in external_app_list),
    ),
    *(
        distenv.Install(
            dist_dir.File(dist_entry[1]).dir,
            app_artifact.compact,
        )
        for app_artifact in external_app_list
        for dist_entry in app_artifact.dist_entries
    ),
]
Depends(
    fap_dist,
    list(app_artifact.validator for app_artifact in external_app_list),
)
Alias("fap_dist", fap_dist)

firmware_env.ConfigureDistTargets(distenv)


# Configure shell with build tools
distenv.PhonyTarget(
    "env",
    "@echo $( ${FBT_SCRIPT_DIR.abspath}/toolchain/fbtenv.sh $)",
)
