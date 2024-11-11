from ansi.color import fg
from fbt.appmanifest import (
    AppManager,
    AppBuildset,
    FlipperApplication,
    FlipperAppType,
    FlipperManifestException,
)
from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Errors import StopError
from SCons.Script import GetOption
from SCons.Warnings import WarningOnByDefault, warn

# Adding objects for application management to env
#  AppManager env["APPMGR"] - loads all manifests; manages list of known apps
#  AppBuildset env["APPBUILD"] - contains subset of apps, filtered for current config


def LoadAppManifest(env, entry):
    try:
        manifest_glob = entry.glob(FlipperApplication.APP_MANIFEST_DEFAULT_NAME)
        if len(manifest_glob) == 0:
            try:
                disk_node = next(filter(lambda d: d.exists(), entry.get_all_rdirs()))
            except Exception:
                disk_node = entry

            raise FlipperManifestException(
                f"App folder '{disk_node.abspath}': missing manifest ({FlipperApplication.APP_MANIFEST_DEFAULT_NAME})"
            )

        for manifest in manifest_glob:
            app_manifest_file_path = manifest.rfile().abspath
            env["APPMGR"].load_manifest(
                app_manifest_file_path, entry, target_hw=env.subst("${F_TARGET_HW}")
            )
    except FlipperManifestException as e:
        if not GetOption("silent"):
            warn(WarningOnByDefault, str(e))


def PrepareApplicationsBuild(env):
    try:
        extra_apps_list = []
        if extra_ext_apps := GetOption("extra_ext_apps"):
            extra_apps_list = extra_ext_apps.split(",")
        appbuild = env["APPBUILD"] = env["APPMGR"].filter_apps(
            applist=env["APPS"],
            ext_applist=extra_apps_list,
            hw_target=env.subst("${F_TARGET_HW}"),
        )
    except Exception as e:
        raise StopError(e)

    # At this point, the app env owns the SDK header list (it cloned from fwenv before)
    if env.get("APPENV"):
        env["APPENV"].Append(
            SDK_HEADERS=appbuild.get_sdk_headers(),
        )


def DumpApplicationConfig(target, source, env):
    print(f"Loaded {len(env['APPMGR'].known_apps)} app definitions.")
    if "APPBUILD" not in env:
        print(fg.red("No apps selected for the build."))
        return
    print(fg.boldgreen("Firmware modules configuration:"))
    for apptype in FlipperAppType:
        app_sublist = env["APPBUILD"].get_apps_of_type(apptype)
        if app_sublist:
            print(
                fg.green(f"{apptype.value}:\n\t"),
                ", ".join(app.appid for app in app_sublist),
            )
    if incompatible_ext_apps := env["APPBUILD"].get_incompatible_ext_apps():
        print(
            fg.blue("Incompatible apps (skipped):\n\t"),
            ", ".join(app.appid for app in incompatible_ext_apps),
        )


def generate(env):
    env.AddMethod(LoadAppManifest)
    env.AddMethod(PrepareApplicationsBuild)
    env.SetDefault(
        APPMGR=AppManager(bool(GetOption("silent"))),
        APPBUILD_DUMP=env.Action(
            DumpApplicationConfig,
            "\tINFO\t",
        ),
    )


def exists(env):
    return True
