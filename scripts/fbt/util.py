import os
import re
import subprocess
import sys
import webbrowser
from pathlib import Path, PurePosixPath

import SCons
from SCons.Errors import StopError
from SCons.Script import AddOption
from SCons.Subst import quote_spaces

WINPATHSEP_RE = re.compile(r"\\([^\"'\\]|$)")

# Used by default when globbing for files with GlobRecursive
# Excludes all files ending with ~, usually created by editors as backup files
GLOB_FILE_EXCLUSION = ["*~"]

# List of environment variables to proxy to child processes
FORWARDED_ENV_VARIABLES = [
    # CI/CD variables
    "WORKFLOW_BRANCH_OR_TAG",
    "DIST_SUFFIX",
    # Python & other tools
    "HOME",
    "APPDATA",
    "PYTHONHOME",
    "PYTHONPATH",
    "PYTHONNOUSERSITE",
    "REQUESTS_CA_BUNDLE",
    "SSL_CERT_FILE",
    # Temp & user dirs
    "TMP",
    "TEMP",
    "USERPROFILE",
    "LOCALAPPDATA",
    # ccache
    "CCACHE_DISABLE",
    # Colors for tools
    "TERM",
    "TERMINFO_DIRS",
    # Toolchain
    "FBT_TOOLCHAIN_PATH",
    "UFBT_HOME",
]


def tempfile_arg_esc_func(arg):
    arg = quote_spaces(arg)
    if SCons.Platform.platform_default() != "win32":
        return arg
    # GCC requires double Windows slashes, let's use UNIX separator
    return WINPATHSEP_RE.sub(r"/\1", arg)


def wrap_tempfile(env, command):
    env[command] = '${TEMPFILE("' + env[command] + '","$' + command + 'STR")}'


def link_dir(target_path, source_path, is_windows):
    # print(f"link_dir: {target_path} -> {source_path}")
    if os.path.lexists(target_path) or os.path.exists(target_path):
        os.unlink(target_path)
    if is_windows:
        # Crete junction
        import _winapi

        if not os.path.isdir(source_path):
            raise StopError(f"Source directory {source_path} is not a directory")

        if not os.path.exists(target_path):
            _winapi.CreateJunction(source_path, target_path)
    else:
        os.symlink(source_path, target_path)


def resolve_real_dir_node(node):
    if isinstance(node, SCons.Node.FS.EntryProxy):
        node = node.get()

    for repo_dir in node.get_all_rdirs():
        if os.path.exists(repo_dir.abspath):
            return repo_dir

    raise StopError(f"Can't find absolute path for {node.name} ({node})")


def expand_paths_over_repositories(env, entries):
    """Resolve repo-relative entries to existing absolute paths across all SCons repositories."""
    roots = []
    seen_roots = set()
    for repo_dir in env.Dir("#").get_all_rdirs():
        abspath = repo_dir.get_abspath()
        if abspath not in seen_roots:
            seen_roots.add(abspath)
            roots.append(abspath)

    results = []
    seen = set()

    def _add(path):
        if os.path.exists(path) and path not in seen:
            seen.add(path)
            results.append(path)

    for entry in entries:
        if not entry:
            continue
        if hasattr(entry, "get_abspath"):
            if isinstance(entry, SCons.Node.FS.EntryProxy):
                entry = entry.get()
            _add(entry.srcnode().get_abspath())
            continue
        path = os.fspath(entry)
        if os.path.isabs(path):
            _add(path)
            continue
        for root in roots:
            _add(os.path.join(root, path))

    return results


class PosixPathWrapper:
    def __init__(self, pathobj):
        self.pathobj = pathobj

    @staticmethod
    def fixup_separators(path):
        if SCons.Platform.platform_default() == "win32":
            return path.replace(os.path.sep, os.path.altsep)
        return path

    @staticmethod
    def fix_path(path):
        return str(PurePosixPath(Path(path).as_posix()))

    def __call__(self, target, source, env, for_signature):
        if for_signature:
            return self.pathobj

        return self.fix_path(env.subst(self.pathobj))


def open_browser_action(target, source, env):
    if sys.platform == "darwin":
        subprocess.run(["open", source[0].abspath])
    else:
        webbrowser.open(source[0].abspath)


def safe_add_option(*args, **kwargs):
    """Add an option to the command line parser, ensuring it doesn't conflict with existing options."""
    # if (optdest := kwargs.get("dest")) and optdest in OptionsParser.values.__defaults__:
    #     print(f"Option {optdest} already exists, skipping addition.")
    #     return

    try:
        AddOption(*args, **kwargs)
    except Exception as e:
        # print(f"Error adding option {args[0]}: {e}")
        pass
