import itertools
import json
import os
from flipper.utils.hw_platform import FbtHardwarePlatform

from SCons.Errors import StopError


class TargetLoaderError(Exception):
    pass


class HardwareTargetLoader:
    TARGET_FILE_NAME = "target.json"

    def __init__(self, env, root_target_scons_dir, target_id):
        self.env = env
        self.all_targets_root_dir = root_target_scons_dir
        self.target_dir = self._getTargetDir(target_id)
        self.target_id = target_id

        self.include_paths = []
        self.sdk_headers = []
        self.linker_script_flash = None
        self.linker_script_ram = None
        self.linker_script_app = None
        self.sdk_symbols = None
        self.platform_desc = None
        self.rtos_flavor = None
        self.lib_modules = []
        self.fw_modules = []
        self.variables_sconscript = None
        self.target_sconscript = None
        self.dist_modules = []
        self.sources = []
        self.extra_target_meta = {}

        self.env_setup_scripts = []
        self.extra_tool_paths = []

        self._processTargetDefinitions(target_id)
        self._checkEffectiveConfig()
        # print(f"Config for {target_id} : {self.__dict__}")

    def _checkEffectiveConfig(self):
        for prop_name in (
            "platform_desc",
            "linker_script_flash",
            "fw_modules",
            "lib_modules",
        ):
            if not getattr(self, prop_name):
                raise TargetLoaderError(
                    f"Property {prop_name} is not set for target {self.target_id}"
                )

    def _getTargetDir(self, target_id):
        return self.all_targets_root_dir.Dir(f"{target_id}")

    def _loadDescription(self, target_id):
        target_json_file = self._getTargetDir(target_id).File(self.TARGET_FILE_NAME)
        if not target_json_file.rexists():
            raise TargetLoaderError(
                f"Target specification file {target_json_file} does not exist"
            )

        try:
            return json.loads(target_json_file.get_text_contents())
        except json.JSONDecodeError as e:
            raise TargetLoaderError(
                f"Failed to parse target file {target_json_file}: {e}"
            )

    def _mergeList(self, target_list, new_list, converter=lambda x: x):
        actions = {
            "-": lambda lst, elem: (
                lst.remove(elem)
                if elem in lst
                else self._raise_error(elem, lst, "remove")
            ),
            "^": lambda lst, elem: (
                lst.insert(0, elem)
                if elem not in lst
                else self._raise_error(elem, lst, "insert")
            ),
            "": lambda lst, elem: (
                lst.append(elem)
                if elem not in lst
                else self._raise_error(elem, lst, "append")
            ),
        }

        for elem in new_list:
            prefix = elem[0] if elem[0] in actions else ""
            elem = converter(elem[1:] if prefix else elem)
            actions[prefix](target_list, elem)

        return target_list

    def _raise_error(self, elem, lst, action):
        raise TargetLoaderError(f"Cannot {action} element {elem} in the list {lst}")

    def _path_spec_to_dir_node_pair(self, curr_target_dir, path_spec):
        if ":" in path_spec:
            target_id, path_spec = path_spec.split(":")
            target_dir = self._getTargetDir(target_id)
            if not target_dir:
                raise TargetLoaderError(f"Unknown target {target_id}")
        else:
            target_dir = curr_target_dir

        return (target_dir, path_spec)

    def _processTargetDefinitions(self, target_id):
        target_dir = self._getTargetDir(target_id)

        config = self._loadDescription(target_id)
        if inherited_target := config.get("inherit", None):
            self._processTargetDefinitions(inherited_target)

        for dir_list_name in ("env_setup_scripts",):
            self._mergeList(
                getattr(self, dir_list_name),
                config.get(dir_list_name, []),
                lambda n: target_dir.File(n).srcnode(),
            )

        for file_list_name in (
            "include_paths",
            "extra_tool_paths",
        ):
            self._mergeList(
                getattr(self, file_list_name),
                config.get(file_list_name, []),
                lambda n: target_dir.Dir(n).rdir(),
            )

        # We store these path globs in conjunction with the target directory, so we can
        # resolve them later in the context of that directory
        for file_glob_list_name in (
            "sdk_headers",
            "sources",
        ):
            self._mergeList(
                getattr(self, file_glob_list_name),
                config.get(file_glob_list_name, []),
                lambda pathspec: self._path_spec_to_dir_node_pair(target_dir, pathspec),
            )

        for prop_name in (
            "lib_modules",
            "dist_modules",
            "fw_modules",
        ):
            self._mergeList(getattr(self, prop_name), config.get(prop_name, []))

        for dict_prop_name in ("extra_target_meta",):
            self.extra_target_meta.update(config.get(dict_prop_name, {}))

        file_attrs = (
            ## (name, is_target_file_node)
            ("linker_script_flash", True),
            ("linker_script_ram", True),
            ("linker_script_app", True),
            ("sdk_symbols", True),
            ("platform_desc", True),
            ("rtos_flavor", False),
            ("variables_sconscript", True),
            ("target_sconscript", True),
        )

        for attr_name, is_target_file_node in file_attrs:
            if val := config.get(attr_name):  # and not getattr(self, attr_name):
                node = target_dir.File(val).rfile() if is_target_file_node else val
                # print(f"Got node {node} for {attr_name}")
                setattr(self, attr_name, node)

        cpu_flags = config.get("cpu_flags", [])
        flags_pairs = (
            # (scons_env_var_name, target_json_name, append_cpu_flags)
            ("CCFLAGS", "c_cpp_flags", True),
            ("CXXFLAGS", "cpp_flags", False),
            ("CFLAGS", "c_flags", False),
            ("ASFLAGS", "asm_flags", True),
            ("LINKFLAGS", "linker_flags", True),
            ("CPPDEFINES", "defines", False),
        )
        for env_var, json_name, append_cpu_flags in flags_pairs:
            flags = list(cpu_flags) if append_cpu_flags else []
            flags.extend(config.get(json_name, []))
            self.env.AppendUnique(**{env_var: flags})

        # print(f"Processed target {target_id} with config: {config}")

    def _processTargetGlobs(
        self, target_globs, *, filter_same_paths=False, strings=False
    ):
        collected_paths = []
        seen_rel_paths = set()

        # Group (target, source_glob) pairs by target directory
        grouped_sources = itertools.groupby(
            sorted(target_globs, key=lambda x: x[0]),
            key=lambda x: x[0],
        )

        for target_dir, source_glob_group in grouped_sources:
            # print(f"Seen sources: {seen_rel_paths}")
            for src in self.env.GatherSources(
                list(g[1] for g in source_glob_group),
                target_dir,
            ):
                relpath = os.path.relpath(src.get_abspath(), target_dir.get_abspath())
                # print(
                #     f"Candidate source: {src.path}, tdir: {target_dir}, seen: {seen_sources_rel_paths}"
                # )
                if filter_same_paths and relpath in seen_rel_paths:
                    continue
                # print(f"Adding source: {src.path}, rel: {relpath}")
                seen_rel_paths.add(relpath)
                collected_paths.append(src)

        # print(
        #     f"Found {len(collected_paths)} sources: {list(f.path for f in collected_paths)}"
        # )

        return (
            list(f.get_path(self.all_targets_root_dir) for f in collected_paths)
            if strings
            else collected_paths
        )

    def gatherTargetSources(self):
        sources = self._processTargetGlobs(
            self.sources, filter_same_paths=True, strings=True
        )
        if self.env["VERBOSE"]:
            print(f"Sources for target {self.env['TARGET_HW']}: {sources}")
        return sources

    def gatherTargetSdkHeaders(self):
        return self._processTargetGlobs(self.sdk_headers)

    def resolveTargetFilePath(self, path_spec):
        path = self._path_spec_to_dir_node_pair(self.target_dir, path_spec)
        return path[0].File(path[1]).rfile()


def ConfigureForTarget(env, lightweight=False):
    env.SetDefault(
        F_TARGET_HW="f${TARGET_HW}",
        FBT_ENV_SETUP_SCRIPTS=[env.File("#/site_scons/register_components.scons")],
    )
    try:
        target_loader = HardwareTargetLoader(
            env, env["TARGETS_ROOT"], env.subst("${F_TARGET_HW}")
        )
    except TargetLoaderError as e:
        raise StopError(e)

    hw_target_obj = FbtHardwarePlatform.from_file(target_loader.platform_desc.abspath)
    env.Replace(
        TARGET_CFG=target_loader,
        SDK_DEFINITION=target_loader.sdk_symbols,
        HW_PLATFORM=hw_target_obj,
        HW_RTOS_FLAVOR=target_loader.rtos_flavor,
        LINKER_SCRIPT_PATH=target_loader.linker_script_flash,
        APP_LINKER_SCRIPT_PATH=target_loader.linker_script_app,
        # Extracting relevant properties from the target loader
        HW_CONFIG_FILE=target_loader.platform_desc,
        HW_SVD_FILE=env.File(hw_target_obj.svd_file).rfile(),
        HW_IMAGE_BASE_ADDRESS=f"{hw_target_obj.flash_address:#x}",
    )
    env.AppendUnique(
        FBT_ENV_SETUP_SCRIPTS=target_loader.env_setup_scripts,
        EXTRA_TOOL_PATHS=target_loader.extra_tool_paths,
    )

    if env["VERBOSE"]:
        print(f"Target include dirs: {[p.path for p in target_loader.include_paths]}")

    if lightweight:
        return

    env.AppendUnique(
        LINKFLAGS="-T${LINKER_SCRIPT_PATH}",
        CPPPATH=target_loader.include_paths,
        SDK_HEADERS=target_loader.gatherTargetSdkHeaders(),
    )


def ApplyLibFlags(env, lib_name=None):
    if not lib_name:
        lib_name = env["FW_LIB_NAME"]
    flags_to_apply = env["FW_LIB_OPTS"].get(lib_name, env["FW_LIB_OPTS"]["Default"])
    if env["VERBOSE"]:
        print(
            f"Flags for {lib_name}: {flags_to_apply} -> {dict((k,env.subst(v)) for k,v in flags_to_apply.items())}"
        )
    env.MergeFlags(flags_to_apply)


def ConfigureCommandlineVariables(env, variables):
    loader = env["TARGET_CFG"]
    if loader.variables_sconscript:
        variables = env.SConscript(
            loader.variables_sconscript,
            exports={
                "ENV": env,
                "VARS": variables,
            },
        )
    return variables


def ConfigureDistTargets(env, distenv):
    # print("ConfigureDistTargets", env["TARGET_CFG"].dist_modules)
    for dist_module in env["TARGET_CFG"].dist_modules:
        dist_script = env.GetComponent(f"dist_{dist_module}")
        # print("Dist script: ", dist_script)
        env.SConscript(
            dist_script,
            # duplicate=0,
            exports={
                "DIST_ENV": distenv,
                "FW_ENV": env,
            },
        )


def ConfigureFwEnvWithLibraries(env):
    # print("ConfigureFwEnvWithLibraries")
    static_libs = []
    for dep_lib in env["TARGET_CFG"].lib_modules:
        if env["VERBOSE"]:
            print(f"Configuring lib module: {dep_lib}")
        component_script = env.GetComponent(f"fwlib_{dep_lib}")
        script_eval_res = env.SConscript(
            component_script,
            variant_dir=env["BUILD_DIR"].Dir(dep_lib),
            src_dir=env.File(component_script).dir,
            duplicate=0,
        )

        if script_eval_res:
            static_libs.append(script_eval_res)

    # print("Static libs: ", static_libs)
    env.AppendUnique(FW_LIBS=static_libs)


def ConfigureFwEnvComponents(env):
    for fw_module in env["TARGET_CFG"].fw_modules:
        fw_script = env.GetComponent(f"fwenv_{fw_module}")
        # print("Fw script: ", fw_script, "->", fw_script.abspath)
        env.SConscript(
            fw_script,
            exports={
                "FW_ENV": env,
            },
            variant_dir=env["BUILD_DIR"].Dir(fw_module),
            duplicate=0,
        )


def GetAdditionalToolPaths(env):
    return [d.srcnode().abspath for d in env.get("EXTRA_TOOL_PATHS", [])]


def generate(env):
    env.AddMethod(ConfigureForTarget)
    env.AddMethod(ApplyLibFlags)
    env.AddMethod(ConfigureCommandlineVariables)
    env.AddMethod(ConfigureDistTargets)
    env.AddMethod(ConfigureFwEnvWithLibraries)
    env.AddMethod(ConfigureFwEnvComponents)
    env.AddMethod(GetAdditionalToolPaths)


def exists(env):
    return True
