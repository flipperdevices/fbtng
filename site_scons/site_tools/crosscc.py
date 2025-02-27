import subprocess

import gdb
import objdump
import strip
from SCons.Action import _subproc
from SCons.Errors import StopError
from SCons.Tool import ar, asm, gcc, gnulink, gxx


def prefix_commands(env, command_prefix, cmd_list):
    for command in cmd_list:
        if command in env:
            prefixed_binary = command_prefix + env[command]
            if not env.WhereIs(prefixed_binary):
                raise StopError(
                    f"Toolchain binary {prefixed_binary} not found in PATH."
                )
            env.Replace(**{command: prefixed_binary})


def _get_tool_version(env, tool):
    verstr = "version unknown"
    proc = _subproc(
        env,
        [env.subst("${%s}" % tool), "--version"],
        stdout=subprocess.PIPE,
        stderr="devnull",
        stdin="devnull",
        universal_newlines=True,
        error="raise",
        shell=False,
    )
    if proc:
        verstr = proc.stdout.readline()
        proc.communicate()
    return verstr


def generate(env, **kw):
    if not env.get("VERBOSE", False):
        env.SetDefault(
            CCCOMSTR="\tCC\t${SOURCE}",
            CXXCOMSTR="\tCPP\t${SOURCE}",
            ASCOMSTR="\tASM\t${SOURCE}",
            ARCOMSTR="\tAR\t${TARGET}",
            RANLIBCOMSTR="\tRANLIB\t${TARGET}",
            LINKCOMSTR="\tLINK\t${TARGET}",
            INSTALLSTR="\tINSTALL\t${TARGET}",
            APPSCOMSTR="\tAPPS\t${TARGET}",
            VERSIONCOMSTR="\tVERSION\t${TARGET}",
            STRIPCOMSTR="\tSTRIP\t${TARGET}",
            OBJDUMPCOMSTR="\tOBJDUMP\t${TARGET}",
        )

    for orig_tool in (asm, gcc, gxx, ar, gnulink, strip, gdb, objdump):
        orig_tool.generate(env)
    env.SetDefault(
        TOOLCHAIN_PREFIX=kw.get("toolchain_prefix"),
    )
    prefix_commands(
        env,
        env.subst("$TOOLCHAIN_PREFIX"),
        [
            "AR",
            "AS",
            "CC",
            "CXX",
            "OBJCOPY",
            "RANLIB",
            "STRIP",
            "GDB",
            "GDBPY",
            "OBJDUMP",
        ],
    )
    env.Replace(AS="arm-none-eabi-gcc")
    env.Replace(LINK="arm-none-eabi-gcc")
    env.Replace(CXX="arm-none-eabi-gcc")
    # Call CC to check version
    if whitelisted_versions := kw.get("versions", ()):
        cc_version = _get_tool_version(env, "CC")
        # print("CC version =", cc_version)
        # print(list(filter(lambda v: v in cc_version, whitelisted_versions)))
        if not any(filter(lambda v: v in cc_version, whitelisted_versions)):
            raise StopError(
                f"Toolchain version is not supported. Allowed: {whitelisted_versions}, toolchain: {cc_version} "
            )


def exists(env):
    return True
