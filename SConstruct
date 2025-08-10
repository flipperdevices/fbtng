#
# Main Flipper Build Tool entry point
#
# This file is evaluated by scons (the build system) every time fbt is invoked.
#

DefaultEnvironment(tools=[])

EnsurePythonVersion(3, 8)

SConscript("#project.scons")

# # Return a path with script to source for enabling build tools in the shell
# distenv.PhonyTarget(
#     "env",
#     "@echo $( ${FBT_SCRIPT_DIR.abspath}/toolchain/fbtenv.sh $)",
# )
