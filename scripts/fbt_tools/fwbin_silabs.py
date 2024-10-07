import SCons
from SCons.Action import Action
from SCons.Builder import Builder


def generate(env):
    env.SetDefault(
        BIN2RPS="${FBT_SCRIPT_DIR}/bin2rps.py",
    )

    if not env["VERBOSE"]:
        env.SetDefault(
            RPSCOMSTR="\tRPS\t${TARGET}",
        )

    env.Append(
        BUILDERS={
            "RPSBuilder": Builder(
                action=Action(
                    [
                        [
                            "${PYTHON3}",
                            "${BIN2RPS}",
                            "-i",
                            "${SOURCE}",
                            "-o",
                            "${TARGET}",
                            "-a",
                            "${HW_IMAGE_BASE_ADDRESS}",
                        ]
                    ],
                    "${RPSCOMSTR}",
                ),
                suffix=".rps",
                src_suffix=".bin",
            ),
        }
    )


def exists(env):
    return True
