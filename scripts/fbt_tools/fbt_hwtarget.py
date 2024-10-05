import json


class TargetLoaderError(Exception):
    pass


class HardwareTargetLoader:
    TARGET_FILE_NAME = "target.json"

    def __init__(self, env, root_target_scons_dir, target_id):
        self.env = env
        self.all_targets_root_dir = root_target_scons_dir
        self.target_dir = self._getTargetDir(target_id)
        # self.target_id = target_id
        self.layered_target_dirs = []

        self.include_paths = []
        self.sdk_header_paths = []
        self.startup_script = None
        self.linker_script_flash = None
        self.linker_script_ram = None
        self.linker_script_app = None
        self.sdk_symbols = None
        self.platform = None
        self.svd_file = None
        self.flash_address = None
        self.rtos_flavor = None
        self.included_sources = []
        self.excluded_sources = []
        self.excluded_headers = []
        self.env_modules = []
        self.fw_modules = []
        self.variables_sconscript = None
        self.target_sconscript = None
        self.dist_modules = []
        self.target_sources_paths = []
        # self.script_dir , tool_dir?
        self._processTargetDefinitions(target_id)

    def _getTargetDir(self, target_id):
        return self.all_targets_root_dir.Dir(f"{target_id}")

    def _loadDescription(self, target_id):
        target_json_file = self._getTargetDir(target_id).File(self.TARGET_FILE_NAME)
        if not target_json_file.exists():
            raise TargetLoaderError(
                f"Target specification file {target_json_file} does not exist"
            )

        with open(target_json_file.get_abspath(), "r") as f:
            try:
                return json.load(f)
            except json.JSONDecodeError as e:
                raise TargetLoaderError(
                    f"Failed to parse target file {target_json_file}: {e}"
                )

    def _processTargetDefinitions(self, target_id):
        target_dir = self._getTargetDir(target_id)

        config = self._loadDescription(target_id)

        for path_list in (
            "include_paths",
            "sdk_header_paths",
            "target_sources_paths",
        ):
            getattr(self, path_list).extend(
                target_dir.Dir(p) for p in config.get(path_list, [])
            )

        for dirpath in self.target_sources_paths:
            if not dirpath.exists():
                raise TargetLoaderError(f"Target sources path {dirpath} does not exist")
            self.layered_target_dirs.append(dirpath)

        for prop_name in (
            "included_sources",
            "excluded_sources",
            "excluded_headers",
            "env_modules",
            "dist_modules",
            "fw_modules",
        ):
            getattr(self, prop_name).extend(config.get(prop_name, []))

        file_attrs = (
            # (name, is_target_file_node)
            ("startup_script", True),
            ("linker_script_flash", True),
            ("linker_script_ram", True),
            ("linker_script_app", True),
            ("sdk_symbols", True),
            ("platform", False),
            ("svd_file", False),
            ("flash_address", False),
            ("rtos_flavor", False),
            ("variables_sconscript", True),
            ("target_sconscript", True),
        )

        for attr_name, is_target_file_node in file_attrs:
            if (val := config.get(attr_name)) and not getattr(self, attr_name):
                if is_target_file_node:
                    node = target_dir.File(val).srcnode()
                else:
                    node = val
                # print(f"Got node {node}, {node.path} for {attr_name}")
                setattr(self, attr_name, node)

        # for attr_name in ("linker_dependencies",):
        #     if (val := config.get(attr_name)) and not getattr(self, attr_name):
        #         setattr(self, attr_name, val)

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

        self.env.AppendUnique(LINKFLAGS="-T${LINKER_SCRIPT_PATH}")

        if inherited_target := config.get("inherit", None):
            self._processTargetDefinitions(inherited_target)

    def gatherSources(self):
        sources = []
        if self.startup_script:
            sources.append(self.startup_script)
        if self.included_sources:
            sources += list(self.target_dir.File(p) for p in self.included_sources)
            return list(f.get_path(self.all_targets_root_dir) for f in sources)

        seen_filenames = set(self.excluded_sources)
        # print("Layers: ", self.layered_target_dirs)
        for target_dir in self.layered_target_dirs:
            accepted_sources = list(
                filter(
                    lambda f: f.name not in seen_filenames,
                    self.env.GlobRecursive("*.c", target_dir),
                )
            )
            seen_filenames.update(f.name for f in accepted_sources)
            sources.extend(accepted_sources)
        # print(f"Found {len(sources)} sources: {list(f.path for f in sources)}")
        return list(f.get_path(self.all_targets_root_dir) for f in sources)

    def gatherSdkHeaders(self):
        sdk_headers = []
        seen_sdk_headers = set(self.excluded_headers)
        for sdk_path in self.sdk_header_paths:
            # dirty, but fast - exclude headers from overlayed targets by name
            # proper way would be to use relative paths, but names will do for now
            for header in self.env.GlobRecursive("*.h", sdk_path, "*_i.h"):
                if header.name not in seen_sdk_headers:
                    seen_sdk_headers.add(header.name)
                    sdk_headers.append(header)
        return sdk_headers


def ConfigureForTarget(env, lightweight=False):
    env.SetDefault(
        F_TARGET_HW="f${TARGET_HW}",
    )
    target_loader = HardwareTargetLoader(
        env, env["TARGETS_ROOT"], env.subst("${F_TARGET_HW}")
    )
    env.Replace(
        TARGET_CFG=target_loader,
        SDK_DEFINITION=target_loader.sdk_symbols,
        HW_PLATFORM=target_loader.platform,
        HW_IMAGE_BASE_ADDRESS=target_loader.flash_address,
        HW_SVD_FILE="${FBT_DEBUG_DIR}/" + target_loader.svd_file,
        LINKER_SCRIPT_PATH=target_loader.linker_script_flash,
        APP_LINKER_SCRIPT_PATH=target_loader.linker_script_app,
        HW_RTOS_FLAVOR=target_loader.rtos_flavor,
    )

    if lightweight:
        return

    env.Append(
        CPPPATH=target_loader.include_paths,
        SDK_HEADERS=target_loader.gatherSdkHeaders(),
    )


def ApplyLibFlags(env, lib_name=None):
    if not lib_name:
        lib_name = env["FW_LIB_NAME"]
    flags_to_apply = env["FW_LIB_OPTS"].get(lib_name, env["FW_LIB_OPTS"]["Default"])
    if env["VERBOSE"]:
        print(f"Flags for {lib_name}: {flags_to_apply}")
    env.MergeFlags(flags_to_apply)


def ConfigureVariables(env, variables):
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
    for dep_lib in env["TARGET_CFG"].env_modules:
        # print("Dep lib: ", dep_lib)
        component_script = env.GetComponent(f"fwlib_{dep_lib}")
        script_eval_res = env.SConscript(
            component_script,
            # variant_dir=env.Dir("${BUILD_DIR}/" + dep_lib),
            variant_dir=env["BUILD_DIR"].Dir(dep_lib),
            src_dir=env.Dir("#"),
            duplicate=0,
        )

        if script_eval_res:
            static_libs.append(script_eval_res)
        # static_libs.append(env.
        # env.Depends(env["FW_LIB_NAME"], dep_lib)

    # print("Static libs: ", static_libs)
    env.AppendUnique(FW_LIBS=static_libs)


def ConfigureFwEnvComponents(env):
    for fw_module in env["TARGET_CFG"].fw_modules:
        fw_script = env.GetComponent(f"fwenv_{fw_module}")
        # print("Fw script: ", fw_script, "->", fw_script.abspath)
        env.SConscript(
            fw_script,
            # fw_script.abspath,
            exports={
                "FW_ENV": env,
            },
            variant_dir=env["BUILD_DIR"].Dir(fw_module),
            duplicate=0,
        )


def GetComponentDiscoveryDirs(env):
    return [
        env.Dir("#/lib"),
        env.Dir("#/targets"),
        env.Dir("#/furi"),
        env.Dir("#/site_scons/modules"),
        env.Dir("#/assets"),
    ]


def generate(env):
    env.AddMethod(ConfigureForTarget)
    env.AddMethod(ApplyLibFlags)
    env.AddMethod(ConfigureVariables)
    env.AddMethod(ConfigureDistTargets)
    env.AddMethod(ConfigureFwEnvWithLibraries)
    env.AddMethod(ConfigureFwEnvComponents)
    env.AddMethod(GetComponentDiscoveryDirs)


def exists(env):
    return True
