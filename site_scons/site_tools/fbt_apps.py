from ansi.color import fg
from fbt.appmanifest import (
    AppManager,
    FlipperApplication,
    FlipperAppType,
    FlipperManifestException,
)
from SCons.Errors import StopError
from SCons.Script import GetOption
from SCons.Warnings import WarningOnByDefault, warn

# Adding objects for application management to env
#  AppManager env["APPMGR"] - loads all manifests; manages list of known apps
#  AppBuildset env["APPBUILD"] - contains subset of apps, filtered for current config


def LoadAppManifest(env, entry):
    try:
        # print(env["APPDIRS"], env["TARGET_HW"])
        # print(f"Loading app manifest for {entry.abspath} in {env['TARGET_HW']}...")
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
    print(
        fg.boldgreen(
            f"Firmware modules configuration for {env.subst('${F_TARGET_HW}')}:"
        )
    )
    for apptype in FlipperAppType:
        app_sublist = env["APPBUILD"].get_apps_of_type(apptype)
        # Print a warning if any apps in the list have same .order value
        unique_order_values = set(app.order for app in app_sublist)
        if len(app_sublist) != len(unique_order_values) and max(unique_order_values):
            print(
                fg.red(f"{apptype.value}: ")
                + fg.yellow(
                    "Duplicate .order values in group:\n\t"
                    + ", ".join(f"{app.appid} ({app.order})" for app in app_sublist)
                )
            )
        elif app_sublist:
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
