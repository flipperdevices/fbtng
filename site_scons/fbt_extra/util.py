from ansi.color import fg
from fbt.util import link_dir
from SCons.Errors import StopError


def link_dirs(env, *, dst, src):
    print(fg.green(f"Linking {src} as {dst}"))
    return link_dir(dst.abspath, src.abspath, env["PLATFORM"] == "win32")


def should_gen_cdb_and_link_dir(env, requested_targets):
    # explicitly_building_updater = False
    # # Hacky way to check if updater-related targets were requested
    # for build_target in requested_targets:
    #     if "updater" in str(build_target) and "package" not in str(build_target):
    #         explicitly_building_updater = True

    is_updater = not env["IS_BASE_FIRMWARE"]
    # # If updater is explicitly requested, link to the latest updater
    # # Otherwise, link to firmware
    # return (is_updater and explicitly_building_updater) or (
    #     not is_updater and not explicitly_building_updater
    # )
    return not is_updater


def find_variable_config(variables_container, var_name):
    option = next(
        filter(lambda o: o.key == var_name, variables_container.options), None
    )
    if not option:
        raise StopError(f"Variable {var_name} not found in variables container")

    return option
