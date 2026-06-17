"""Recursive globbing for SCons build environments.

Uses ``os.scandir`` for the directory walk instead of SCons'
``node.glob()`` to avoid creating ``FS.Dir`` nodes for every visited
directory — critical when multiple build targets share one SCons process.
"""

import fnmatch
import itertools
import os

import SCons
from fbt.util import GLOB_FILE_EXCLUSION
from SCons.Node.FS import has_glob_magic
from SCons.Script import Flatten


def _walk_scandir(root_abspath, pattern, exclude_patterns):
    """Yield absolute paths of files under *root_abspath* matching *pattern*.

    Uses :func:`os.scandir` — no SCons FS nodes are created during the walk.
    """
    pattern_parts = pattern.split("/")
    dir_parts = pattern_parts[:-1]
    leaf_pattern = pattern_parts[-1]
    recursive = "**" in dir_parts
    min_depth = len(dir_parts)

    def _walk(abspath, depth):
        try:
            with os.scandir(abspath) as entries:
                for entry in entries:
                    if entry.name.startswith("."):
                        continue
                    # Exclude patterns with '/' must be matched against the
                    # full relative path (e.g. "furi_hal/version_device.c").
                    # Plain patterns (e.g. "*~") match against the entry name
                    # only — same semantics as SCons' native glob exclude.
                    relpath = os.path.relpath(entry.path, root_abspath)
                    if any(
                        fnmatch.fnmatch(relpath, p)
                        if "/" in p
                        else fnmatch.fnmatch(entry.name, p)
                        for p in exclude_patterns
                    ):
                        continue

                    if entry.is_dir(follow_symlinks=False):
                        yield from _walk(entry.path, depth + 1)
                    elif entry.is_file():
                        if depth < min_depth:
                            continue
                        if not fnmatch.fnmatch(entry.name, leaf_pattern):
                            continue
                        if dir_parts:
                            file_dir_parts = relpath.split(os.sep)[:-1]
                            if not recursive and len(file_dir_parts) != len(dir_parts):
                                continue
                            if len(file_dir_parts) < len(dir_parts):
                                continue
                            for i, dp in enumerate(dir_parts):
                                if not fnmatch.fnmatch(file_dir_parts[i], dp):
                                    break
                            else:
                                yield entry.path
                        else:
                            yield entry.path
        except (PermissionError, OSError):
            pass

    yield from _walk(root_abspath, 0)


def GlobRecursive(env, pattern, node=".", exclude=None, **kw):
    """Recursively collect files matching *pattern* starting from *node*.

    Falls back to SCons-native glob when ``source=True`` or ``strings=True``
    is passed (needed for variant-directory awareness).
    """
    if exclude is None:
        exclude = []
    exclude = list(set(Flatten(exclude) + GLOB_FILE_EXCLUSION))

    if isinstance(node, str):
        node = env.Dir(node)

    if not has_glob_magic(pattern):
        return [node.File(pattern)]

    if kw.get("source") or kw.get("strings"):
        return _scons_glob(env, pattern, node, exclude, **kw)

    # Walk all repository directories (from SCons -Y flags) to find files
    # that may only exist in a registered repo, not in the source tree.
    seen_abspaths = set()
    results = []
    for rdir in node.get_all_rdirs():
        rdir_abspath = rdir.get_abspath()
        if rdir_abspath in seen_abspaths:
            continue
        seen_abspaths.add(rdir_abspath)
        if not os.path.isdir(rdir_abspath):
            continue
        for filepath in _walk_scandir(rdir_abspath, pattern, exclude):
            relpath = os.path.relpath(filepath, rdir_abspath)
            results.append(node.File(relpath))

    results.sort(key=lambda f: str(f))
    return results


def GatherSources(env, sources_list, node="."):
    """Expand glob patterns into a flat list of source File nodes.

    Patterns prefixed with ``!`` are treated as exclusions.
    """
    sources_list = list(set(Flatten(sources_list)))
    include_sources = [x for x in sources_list if not x.startswith("!")]
    exclude_sources = [x[1:] for x in sources_list if x.startswith("!")]
    gathered_sources = list(
        itertools.chain.from_iterable(
            env.GlobRecursive(source_type, node, exclude=exclude_sources)
            for source_type in include_sources
        )
    )
    return gathered_sources


def _scons_glob(env, pattern, node, exclude, **kw):
    """SCons-native recursive glob fallback for source=True / strings=True."""
    source_flag = kw.pop("source", False)
    strings_flag = kw.pop("strings", False)
    results = []
    if has_glob_magic(pattern):
        for f in node.glob("*", source=source_flag, exclude=exclude):
            if isinstance(f, SCons.Node.FS.Dir):
                results += _scons_glob(env, pattern, f, exclude,
                                       source=source_flag, strings=strings_flag, **kw)
        results += node.glob(
            pattern, source=source_flag, strings=strings_flag,
            exclude=exclude, **kw
        )
    else:
        results.append(node.File(pattern))
    return results


def generate(env):
    env.AddMethod(GlobRecursive)
    env.AddMethod(GatherSources)


def exists(env):
    return True
