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

# Building basic environment - tools, utility methods, cross-compilation
# settings, gcc flags for Cortex-M4, basic builders and more
coreenv = SConscript(
    "site_scons/environ.scons",
    exports={"VAR_ENV": cmd_environment},
    toolpath=["#/scripts/fbt_tools"],
)
SConscript("site_scons/cc.scons", exports={"ENV": coreenv})

# Create a separate "dist" environment and add construction envs to it
distenv = coreenv.Clone(
    tools=[
        "fbt_dist",
        "fbt_debugopts",
        "openocd",
        "blackmagic",
    ],
    ENV=os.environ,
    UPDATE_BUNDLE_DIR="dist/${DIST_DIR}/f${TARGET_HW}-update-${DIST_SUFFIX}",
    VSCODE_LANG_SERVER=ARGUMENTS.get("LANG_SERVER", "cpptools"),
)

firmware_env = distenv.AddFwProject(
    base_env=coreenv,
    fw_type="firmware",
    fw_env_key="FW_ENV",
)

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

# Copy all faps to device
fap_deploy = distenv.PhonyTarget(
    "fap_deploy",
    Action(
        [
            [
                "${PYTHON3}",
                "${FBT_SCRIPT_DIR}/storage.py",
                "-p",
                "${FLIP_PORT}",
                "send",
                "${SOURCE}",
                "/ext/apps",
                "${ARGS}",
            ]
        ]
    ),
    source=firmware_env.Dir(("${RESOURCES_ROOT}/apps")),
)
Depends(fap_deploy, firmware_env["FW_RESOURCES_MANIFEST"])

firmware_flash = distenv.AddFwFlashTarget(firmware_env)
distenv.Alias("flash", firmware_flash)

distenv.PhonyTarget(
    "gdb_trace_all",
    [["${GDB}", "${GDBOPTS}", "${SOURCES}", "${GDBFLASH}"]],
    source=firmware_env["FW_ELF"],
    GDBOPTS="${GDBOPTS_BASE}",
    GDBREMOTE="${OPENOCD_GDB_PIPE}",
    GDBFLASH=[
        "-ex",
        "thread apply all bt",
        "-ex",
        "quit",
    ],
)

# Debugging firmware
firmware_debug = distenv.PhonyTarget(
    "debug",
    "${GDBPYCOM}",
    source=firmware_env["FW_ELF"],
    GDBOPTS="${GDBOPTS_BASE}",
    GDBREMOTE="${OPENOCD_GDB_PIPE}",
)
distenv.Depends(firmware_debug, firmware_flash)

distenv.PhonyTarget(
    "blackmagic",
    "${GDBPYCOM}",
    source=firmware_env["FW_ELF"],
    GDBOPTS="${GDBOPTS_BASE} ${GDBOPTS_BLACKMAGIC}",
    GDBREMOTE="${BLACKMAGIC_ADDR}",
)

# Debug alien elf
debug_other_opts = [
    "-ex",
    "source ${FBT_DEBUG_DIR}/PyCortexMDebug/PyCortexMDebug.py",
    # "-ex",
    # "source ${FBT_DEBUG_DIR}/FreeRTOS/FreeRTOS.py",
    "-ex",
    "source ${FBT_DEBUG_DIR}/flipperversion.py",
    "-ex",
    "fw-version",
]

distenv.PhonyTarget(
    "debug_other",
    "${GDBPYCOM}",
    GDBOPTS="${GDBOPTS_BASE}",
    GDBREMOTE="${OPENOCD_GDB_PIPE}",
    GDBPYOPTS=debug_other_opts,
)

distenv.PhonyTarget(
    "debug_other_blackmagic",
    "${GDBPYCOM}",
    GDBOPTS="${GDBOPTS_BASE} ${GDBOPTS_BLACKMAGIC}",
    GDBREMOTE="${BLACKMAGIC_ADDR}",
    GDBPYOPTS=debug_other_opts,
)

# Just start OpenOCD
distenv.PhonyTarget(
    "openocd",
    [["${OPENOCDCOM}", "${ARGS}"]],
)

# Linter
distenv.PhonyTarget(
    "lint",
    [
        [
            "${PYTHON3}",
            "${FBT_SCRIPT_DIR}/lint.py",
            "check",
            "${LINT_SOURCES}",
            "${ARGS}",
        ]
    ],
    LINT_SOURCES=[n.srcnode() for n in firmware_env["LINT_SOURCES"]],
)

distenv.PhonyTarget(
    "format",
    [
        [
            "${PYTHON3}",
            "${FBT_SCRIPT_DIR}/lint.py",
            "format",
            "${LINT_SOURCES}",
            "${ARGS}",
        ]
    ],
    LINT_SOURCES=[n.srcnode() for n in firmware_env["LINT_SOURCES"]],
)

# PY_LINT_SOURCES contains recursively-built modules' SConscript files
# Here we add additional Python files residing in repo root
firmware_env.Append(
    PY_LINT_SOURCES=[
        # Py code folders
        "site_scons",
        "scripts",
        "applications",
        "applications_user",
        "assets",
        "targets",
        # Extra files
        "SConstruct",
        "firmware.scons",
        "fbt_options.py",
    ]
)


black_commandline = [
    [
        "@${PYTHON3}",
        "-m",
        "black",
        "${PY_BLACK_ARGS}",
        "${PY_LINT_SOURCES}",
        "${ARGS}",
    ]
]
black_base_args = [
    "--include",
    '"(\\.scons|\\.py|SConscript|SConstruct|\\.fam)$"',
]

distenv.PhonyTarget(
    "lint_py",
    black_commandline,
    PY_BLACK_ARGS=[
        "--check",
        "--diff",
        *black_base_args,
    ],
    PY_LINT_SOURCES=firmware_env["PY_LINT_SOURCES"],
)

distenv.PhonyTarget(
    "format_py",
    black_commandline,
    PY_BLACK_ARGS=black_base_args,
    PY_LINT_SOURCES=firmware_env["PY_LINT_SOURCES"],
)

# Start Flipper CLI via PySerial's miniterm
distenv.PhonyTarget(
    "cli",
    [
        [
            "${PYTHON3}",
            "${FBT_SCRIPT_DIR}/serial_cli.py",
            "-p",
            "${FLIP_PORT}",
            "${ARGS}",
        ]
    ],
)

# Update WiFi devboard firmware with release channel
distenv.PhonyTarget(
    "devboard_flash",
    [
        [
            "${PYTHON3}",
            "${FBT_SCRIPT_DIR}/wifi_board.py",
            "${ARGS}",
        ]
    ],
)


# Find blackmagic probe
distenv.PhonyTarget(
    "get_blackmagic",
    "@echo $( ${BLACKMAGIC_ADDR} $)",
)


# Find STLink probe ids
distenv.PhonyTarget(
    "get_stlink",
    distenv.Action(
        lambda **_: distenv.GetDevices(),
        None,
    ),
)

# Prepare vscode environment
VSCODE_LANG_SERVER = cmd_environment["LANG_SERVER"]
vscode_dist = distenv.Install(
    "#.vscode",
    [
        distenv.Glob("#.vscode/example/*.json"),
        distenv.Glob(f"#.vscode/example/{VSCODE_LANG_SERVER}/*.json"),
    ],
)
distenv.Precious(vscode_dist)
distenv.NoClean(vscode_dist)
distenv.Alias("vscode_dist", (vscode_dist, firmware_env["FW_CDB"]))

# Configure shell with build tools
distenv.PhonyTarget(
    "env",
    "@echo $( ${FBT_SCRIPT_DIR.abspath}/toolchain/fbtenv.sh $)",
)
