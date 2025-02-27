import json
import os.path
import pathlib
import posixpath
import shutil

from fbt.sdk.cache import SdkCache
from fbt.sdk.collector import SdkCollector
from fbt.util import PosixPathWrapper
from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Errors import UserError

# from SCons.Scanner import C
from SCons.Script import Entry
from SCons.Util import LogicalLines


def ProcessSdkDepends(env, filename):
    try:
        with open(filename, "r") as fin:
            lines = LogicalLines(fin).readlines()
    except IOError:
        return []

    _, depends = lines[0].split(":", 1)
    depends = depends.split()
    depends.pop(0)  # remove the .c file
    depends = list(
        # Don't create dependency on non-existing files
        # (e.g. when they were renamed since last build)
        filter(
            lambda file: file.exists(),
            (env.File(f"#{path}") for path in depends),
        )
    )
    return depends


def _api_amalgam_emitter(target, source, env):
    target.append(env.ChangeFileExtension(target[0], ".d"))
    target.append(env.ChangeFileExtension(target[0], ".i.c"))
    return target, source


def _api_amalgam_gen_origin_header(target, source, env):
    mega_file = env.subst("${TARGET}.c", target=target[0])
    with open(mega_file, "wt") as sdk_c:
        sdk_c.write(
            "\n".join(f"#include <{h.srcnode().path}>" for h in env["SDK_HEADERS"])
        )


class SdkMeta:
    MAP_FILE_SUBST = "SDK_MAP_FILE_SUBST"

    def __init__(self, env, tree_builder: "SdkTreeBuilder"):
        self.env = env
        self.treebuilder = tree_builder

    def save_to(self, json_manifest_path: str):
        meta_contents = {
            "sdk_symbols": self.treebuilder.build_sdk_file_path(
                self.env["SDK_DEFINITION"].path
            ),
            "cc_args": self._wrap_scons_vars("$CCFLAGS $_CCCOMCOM"),
            "cpp_args": self._wrap_scons_vars("$CXXFLAGS $CCFLAGS $_CCCOMCOM"),
            "linker_args": self._wrap_scons_vars("$LINKFLAGS"),
            "linker_libs": self.env.subst("${LIBS}"),
            "app_ep_subst": self.env.subst("${APP_ENTRY}"),
            "sdk_path_subst": self.env.subst("${SDK_DIR_SUBST}"),
            "map_file_subst": self.MAP_FILE_SUBST,
            "hardware": self.env.subst("${TARGET_HW}"),
        }
        with open(json_manifest_path, "wt") as f:
            json.dump(meta_contents, f, indent=4)

    def _wrap_scons_vars(self, vars: str):
        expanded_vars = self.env.subst(
            vars,
            target=Entry(self.MAP_FILE_SUBST),
        )
        return PosixPathWrapper.fixup_separators(expanded_vars)


class SdkTreeBuilder:
    SDK_DIR_SUBST = "SDK_ROOT_DIR"
    SDK_APP_EP_SUBST = "SDK_APP_EP_SUBST"
    HEADER_EXTENSIONS = [".h", ".hpp"]

    def __init__(self, env, target, source) -> None:
        self.env = env
        self.target = target
        self.source = source

        self.header_depends = []
        self.header_dirs = []

        self.target_sdk_dir_name = env.subst("${F_TARGET_HW}_sdk")
        self.sdk_root_dir = target[0].Dir(".")
        self.sdk_deploy_dir = self.sdk_root_dir.Dir(self.target_sdk_dir_name)

        self.sdk_env = self.env.Clone(
            APP_ENTRY=self.SDK_APP_EP_SUBST,
            SDK_DIR_SUBST=self.SDK_DIR_SUBST,
        )

    def _parse_sdk_depends(self):
        deps_file = self.source[0]
        with open(deps_file.path, "rt") as deps_f:
            lines = LogicalLines(deps_f).readlines()
            _, depends = lines[0].split(":", 1)
            self.header_depends = list(
                filter(
                    lambda fname: any(map(fname.endswith, self.HEADER_EXTENSIONS)),
                    depends.split(),
                ),
            )
            self.header_depends.append(self.sdk_env.subst("${LINKER_SCRIPT_PATH}"))
            self.header_depends.append(self.sdk_env.subst("${SDK_DEFINITION}"))
            self.header_dirs = sorted(
                set(map(os.path.normpath, map(os.path.dirname, self.header_depends)))
            )

    def _generate_sdk_meta(self):
        filtered_paths = ["."]
        full_fw_paths = list(
            map(
                os.path.normpath,
                (
                    self.sdk_env.Dir(inc_dir).relpath
                    for inc_dir in self.sdk_env["CPPPATH"]
                ),
            )
        )

        sdk_dirs = ", ".join(f"'{dir}'" for dir in self.header_dirs)
        filtered_paths.extend(
            filter(lambda path: path in sdk_dirs, full_fw_paths),
        )
        filtered_paths = list(map(self.build_sdk_file_path, filtered_paths))

        self.sdk_env.Replace(
            CPPPATH=filtered_paths,
            ORIG_LINKER_SCRIPT_PATH=self.env["LINKER_SCRIPT_PATH"],
            LINKER_SCRIPT_PATH=self.build_sdk_file_path("${ORIG_LINKER_SCRIPT_PATH}"),
        )
        meta = SdkMeta(self.sdk_env, self)
        meta.save_to(self.target[0].path)

    def build_sdk_file_path(self, orig_path: str) -> str:
        return PosixPathWrapper.fix_path(
            posixpath.normpath(
                posixpath.join(
                    self.SDK_DIR_SUBST,
                    self.target_sdk_dir_name,
                    orig_path,
                )
            )
        )

    def emitter(self, target, source, env):
        target_folder = target[0]
        target = [target_folder.File("sdk.opts")]
        return target, source

    def _run_deploy_commands(self):
        dirs_to_create = set(
            self.sdk_deploy_dir.Dir(dirpath).path for dirpath in self.header_dirs
        )

        shutil.rmtree(self.sdk_root_dir.path, ignore_errors=False)

        for sdkdir in dirs_to_create:
            os.makedirs(sdkdir, exist_ok=True)

        for header in self.header_depends:
            shutil.copy2(header, self.sdk_deploy_dir.File(header).path)

    def deploy_action(self):
        self._parse_sdk_depends()
        # FIXME: disabled for now until proper flattening is implemented
        # self._run_deploy_commands()
        # self._generate_sdk_meta()


def _deploy_sdk_header_tree_action(target, source, env):
    sdk_tree = SdkTreeBuilder(env, target, source)
    return sdk_tree.deploy_action()


def _deploy_sdk_header_tree_emitter(target, source, env):
    sdk_tree = SdkTreeBuilder(env, target, source)
    return sdk_tree.emitter(target, source, env)


def gen_sdk_data(sdk_cache: SdkCache):
    api_def = []
    api_def.extend(
        (f"#include <{h.name}>" for h in sdk_cache.get_headers()),
    )
    api_def.append('#pragma GCC diagnostic ignored "-Wdeprecated-declarations"')

    api_def.append(f"const int elf_api_version = {sdk_cache.version.as_int()};")

    api_def.append(
        "static constexpr auto elf_api_table = sort(create_array_t<sym_entry>("
    )

    api_lines = []
    for fun_def in sdk_cache.get_functions():
        api_lines.append(
            f"API_METHOD({fun_def.name}, {fun_def.returns}, ({fun_def.params}))"
        )

    for var_def in sdk_cache.get_variables():
        api_lines.append(f"API_VARIABLE({var_def.name}, {var_def.var_type })")

    api_def.append(",\n".join(api_lines))

    api_def.append("));")
    return api_def


def _check_sdk_is_up2date(sdk_cache: SdkCache):
    if not sdk_cache.is_buildable():
        raise UserError(
            "SDK version is not finalized, please review changes and re-run operation. See AppsOnSDCard.md for more details"
        )


def _validate_api_cache(source, target, env):
    # print(f"Generating SDK for {source[0]} to {target[0]}")
    current_sdk = SdkCollector()
    current_sdk.process_source_file_for_sdk(source[0].path)
    for h in env["SDK_HEADERS"]:
        current_sdk.add_header_to_sdk(pathlib.Path(h.srcnode().tpath).as_posix())

    sdk_cache = SdkCache(target[0].rfile().path)
    sdk_cache.validate_api(current_sdk.get_api())
    sdk_cache.save()
    _check_sdk_is_up2date(sdk_cache)


def _generate_api_table(source, target, env):
    sdk_cache = SdkCache(source[0].path)
    _check_sdk_is_up2date(sdk_cache)

    api_def = gen_sdk_data(sdk_cache)
    with open(target[0].path, "wt") as f:
        f.write("\n".join(api_def))


def generate(env, **kw):
    if not env["VERBOSE"]:
        env.SetDefault(
            SDK_AMALGAMATE_HEADER_COMSTR="\tAPIPREP\t${TARGET}",
            SDK_AMALGAMATE_PP_COMSTR="\tAPIPP\t${TARGET}",
            SDKSYM_UPDATER_COMSTR="\tSDKCHK\t${TARGET}",
            APITABLE_GENERATOR_COMSTR="\tAPITBL\t${TARGET}",
            SDKTREE_COMSTR="\tSDKTREE\t${TARGET}",
        )

    # Filtering out things cxxheaderparser cannot handle
    env.SetDefault(
        SDK_PP_FLAGS=[
            '-D"_Static_assert(x,y)="',
            '-D"__asm__(x)="',
            '-D"__attribute__(x)="',
            "-Drestrict=",
            "-D_Noreturn=",
            "-D__restrict=",
            "-D__extension__=",
            "-D__inline=inline",
            "-D__inline__=inline",
        ]
    )

    env.AddMethod(ProcessSdkDepends)
    env.Append(
        BUILDERS={
            "ApiAmalgamator": Builder(
                emitter=_api_amalgam_emitter,
                action=[
                    Action(
                        _api_amalgam_gen_origin_header,
                        "$SDK_AMALGAMATE_HEADER_COMSTR",
                    ),
                    Action(
                        [
                            [
                                "$CC",
                                "-o",
                                "$TARGET",
                                "-E",
                                "-P",
                                "$CCFLAGS",
                                "$_CCCOMCOM",
                                "$SDK_PP_FLAGS",
                                "-MMD",
                                "${TARGET}.c",
                            ]
                        ],
                        "$SDK_AMALGAMATE_PP_COMSTR",
                    ),
                ],
                suffix=".i",
            ),
            "SDKHeaderTreeExtractor": Builder(
                action=Action(
                    _deploy_sdk_header_tree_action,
                    "$SDKTREE_COMSTR",
                ),
                emitter=_deploy_sdk_header_tree_emitter,
                src_suffix=".d",
            ),
            "ApiTableValidator": Builder(
                action=Action(
                    _validate_api_cache,
                    "$SDKSYM_UPDATER_COMSTR",
                ),
                suffix=".csv",
                src_suffix=".i",
            ),
            "ApiSymbolTable": Builder(
                action=Action(
                    _generate_api_table,
                    "$APITABLE_GENERATOR_COMSTR",
                ),
                suffix=".h",
                src_suffix=".csv",
            ),
        }
    )


def exists(env):
    return True
