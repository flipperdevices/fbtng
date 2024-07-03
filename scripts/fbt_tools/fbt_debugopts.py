from SCons.Errors import UserError


def generate(env, **kw):
    env.SetDefault(
        FBT_DEBUG_DIR="${FBT_SCRIPT_DIR}/debug",
        FBT_DEBUG_SCRIPT="${FBT_SCRIPT_DIR}/fwdebug.py",
    )


def exists(env):
    return True
