#
# Main Flipper Build Tool entry point
#
# This file is evaluated by scons (the build system) every time fbt is invoked.
#

import os

from SCons.Errors import StopError
from SCons.Script.SConsOptions import SConsBadOptionError

DefaultEnvironment(tools=[])

EnsurePythonVersion(3, 8)

fbt_variables = SConscript("site_scons/commandline.scons")

# This environment is created only for loading options & validating file/dir existence
cmd_environment = Environment(tools=[], variables=fbt_variables)

target_bootstrap_env = cmd_environment.Clone(
    tools=["fbt_hwtarget", "fbt_repos"],
    TARGETS_ROOT=Dir("#/targets"),
)
target_bootstrap_env.InitializeRepositories()
target_bootstrap_env.ConfigureForTarget(lightweight=True)
target_bootstrap_env.ConfigureCommandlineVariables(fbt_variables)

fbt_variables.Update(target_bootstrap_env)
if fbt_variables.UnknownVariables():
    print("Unrecognized command-line variables:")
    for key, value in fbt_variables.UnknownVariables().items():
        print(f"  {key} = {value}")
    raise StopError("Please check your command line.")

try:
    ValidateOptions(throw_exception=True)
except SConsBadOptionError as e:
    print(f"Option validation failure: ", e.opt_str)
    print("See --help and documentation for details on available options.")
    Exit(1)

fbt_variables.Update(cmd_environment)


# Building basic environment - tools, utility methods, cross-compilation settings,
# basic builders, discover and register components, and more
coreenv = SConscript(
    "site_scons/environ.scons",
    exports={
        "VAR_ENV": cmd_environment,
        "COMPONENT_SCRIPTS": target_bootstrap_env["FBT_ENV_SETUP_SCRIPTS"],
        "EXTRA_TOOLPATHS": target_bootstrap_env.GetAdditionalToolPaths(),
    },
)
# fbt_variables.Save("fbt_options_auto.py", coreenv)


# Create a separate "dist" environment and add construction envs to it
distenv = coreenv.Clone(
    tools=["fbt_dist"],
    # ENV=os.environ,
    CORE_ENV=coreenv,
)

firmware_env = distenv.AddFwProject(
    base_env=coreenv,
    fw_type="firmware",
    fw_env_key="FW_ENV",
)

distenv.Default(firmware_env["FW_ARTIFACTS"])

firmware_env.ConfigureDistTargets(distenv)


# Return a path with script to source for enabling build tools in the shell
distenv.PhonyTarget(
    "env",
    "@echo $( ${FBT_SCRIPT_DIR.abspath}/toolchain/fbtenv.sh $)",
)
