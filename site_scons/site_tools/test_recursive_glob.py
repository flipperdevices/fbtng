"""Tests for recursive_glob.py — the improved GlobRecursive / GatherSources.

Run with:
    cd fbt_layers/fbtng
    python -m pytest site_scons/site_tools/test_recursive_glob.py -v
"""

import os
import tempfile
import pytest

# Import the module under test.  The functions we need are top-level in
# recursive_glob; we import them directly for unit testing the scandir
# walker (which does not need a real SCons environment).
from site_scons.site_tools.recursive_glob import (
    _walk_scandir,
    GlobRecursive,
    GatherSources,
    _scons_glob,
    has_glob_magic,
)


# ---------------------------------------------------------------------------
# Fixtures
# ---------------------------------------------------------------------------

def _touch(filepath):
    """Create an empty file (like the shell ``touch`` command)."""
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with open(filepath, "a"):
        pass


@pytest.fixture
def tmpdir():
    """Create a temporary directory tree for glob testing.

    Layout::

        tmp/
        ├── a.c
        ├── b.c
        ├── c.h
        ├── .hidden.c
        ├── backup.c~
        └── sub/
            ├── d.c
            ├── e.c
            └── deep/
                └── f.c
    """
    with tempfile.TemporaryDirectory() as td:
        # Root files
        for name in ("a.c", "b.c", "c.h", ".hidden.c", "backup.c~"):
            _touch(os.path.join(td, name))
        # Subdir
        for name in ("sub/d.c", "sub/e.c", "sub/deep/f.c"):
            _touch(os.path.join(td, name))
        yield td


# ---------------------------------------------------------------------------
# _walk_scandir unit tests
# ---------------------------------------------------------------------------

class TestWalkScandir:
    """Unit tests for the os.scandir-based recursive walker."""

    def test_simple_glob(self, tmpdir):
        """*.c at root matches .c files at ALL depths (recursive)."""
        results = sorted(_walk_scandir(tmpdir, "*.c", ["*~"]))
        expected = sorted([
            os.path.join(tmpdir, "a.c"),
            os.path.join(tmpdir, "b.c"),
            os.path.join(tmpdir, "sub/d.c"),
            os.path.join(tmpdir, "sub/e.c"),
            os.path.join(tmpdir, "sub/deep/f.c"),
        ])
        assert results == expected

    def test_recursive_glob(self, tmpdir):
        """*/*.c matches files at EXACTLY 1 directory level."""
        results = sorted(_walk_scandir(tmpdir, "*/*.c", ["*~"]))
        expected = sorted([
            os.path.join(tmpdir, "sub/d.c"),
            os.path.join(tmpdir, "sub/e.c"),
        ])
        assert results == expected

    def test_deep_glob(self, tmpdir):
        """*/*/*.c matches files two levels deep."""
        results = sorted(_walk_scandir(tmpdir, "*/*/*.c", ["*~"]))
        expected = sorted([
            os.path.join(tmpdir, "sub/deep/f.c"),
        ])
        assert results == expected

    def test_exclude_backups(self, tmpdir):
        """backup files (*~) are excluded by default."""
        results = list(_walk_scandir(tmpdir, "*.c", ["*~"]))
        paths = [os.path.basename(r) for r in results]
        assert "backup.c~" not in paths

    def test_exclude_pattern(self, tmpdir):
        """Custom exclude patterns work."""
        results = list(_walk_scandir(tmpdir, "*.c", ["*~", "a*"]))
        paths = [os.path.basename(r) for r in results]
        assert "a.c" not in paths
        assert "b.c" in paths

    def test_exclude_path_pattern(self, tmpdir):
        """Exclude patterns with '/' match against the full relative path."""
        # Exclude a specific file by relative path
        results = sorted(_walk_scandir(tmpdir, "*.c", ["sub/d.c"]))
        basenames = [os.path.basename(r) for r in results]
        assert "d.c" not in basenames
        assert "e.c" in basenames

    def test_exclude_path_pattern_deep(self, tmpdir):
        """Path-based exclusion matches deeply nested files."""
        results = sorted(_walk_scandir(tmpdir, "*.c", ["sub/deep/f.c"]))
        basenames = [os.path.basename(r) for r in results]
        assert "f.c" not in basenames

    def test_exclude_directory_by_path(self, tmpdir):
        """Excluding a directory prunes the entire subtree."""
        results = sorted(_walk_scandir(tmpdir, "*.c", ["sub/deep"]))
        basenames = [os.path.basename(r) for r in results]
        assert "f.c" not in basenames
        assert "d.c" in basenames  # sub/d.c still matches

    def test_exclude_wildcard_path(self, tmpdir):
        """Wildcard in path exclusion matches directory components."""
        results = sorted(_walk_scandir(tmpdir, "*.c", ["*/d.c"]))
        basenames = [os.path.basename(r) for r in results]
        assert "d.c" not in basenames  # sub/d.c excluded
        assert "e.c" in basenames      # sub/e.c still there
        assert "f.c" in basenames      # sub/deep/f.c has different path

    def test_hidden_files_skipped(self, tmpdir):
        """Files starting with '.' are skipped (matching SCons behavior)."""
        results = list(_walk_scandir(tmpdir, "*.c", ["*~"]))
        paths = [os.path.basename(r) for r in results]
        assert ".hidden.c" not in paths

    def test_no_match(self, tmpdir):
        """Non-matching pattern returns empty list."""
        results = list(_walk_scandir(tmpdir, "*.py", ["*~"]))
        assert results == []

    def test_empty_directory(self):
        """Empty directory returns empty list."""
        with tempfile.TemporaryDirectory() as td:
            results = list(_walk_scandir(td, "*.c", ["*~"]))
            assert results == []

    def test_nonexistent_directory(self):
        """Non-existent directory returns empty list (no crash)."""
        results = list(_walk_scandir("/nonexistent/path/12345", "*.c", ["*~"]))
        assert results == []

    def test_subdir_only_file_match(self, tmpdir):
        """*.h matches c.h — recursive, but only .h file exists at root."""
        results = sorted(_walk_scandir(tmpdir, "*.h", ["*~"]))
        expected = sorted([os.path.join(tmpdir, "c.h")])
        assert results == expected

    def test_deep_subdir_file(self, tmpdir):
        """sub/deep/f.c is found with explicit path pattern."""
        results = sorted(_walk_scandir(tmpdir, "sub/deep/f.c", ["*~"]))
        expected = sorted([os.path.join(tmpdir, "sub/deep/f.c")])
        assert results == expected


# ---------------------------------------------------------------------------
# has_glob_magic tests
# ---------------------------------------------------------------------------

class TestHasGlobMagic:
    def test_star(self):
        assert has_glob_magic("*.c") is True

    def test_question(self):
        assert has_glob_magic("file?.c") is True

    def test_bracket(self):
        assert has_glob_magic("file[0-9].c") is True

    def test_plain(self):
        assert has_glob_magic("file.c") is False

    def test_path_with_glob(self):
        assert has_glob_magic("sub/*.c") is True

    def test_path_without_glob(self):
        assert has_glob_magic("sub/file.c") is False


# ---------------------------------------------------------------------------
# GlobRecursive integration tests (require SCons)
# ---------------------------------------------------------------------------

@pytest.fixture
def scons_env():
    """Create a minimal SCons environment with GlobRecursive registered."""
    from SCons.Environment import Environment
    env = Environment()
    from site_scons.site_tools.recursive_glob import generate
    generate(env)
    return env


class TestGlobRecursive:
    """Integration tests for GlobRecursive with a real SCons environment."""

    def test_simple_glob(self, scons_env, tmpdir):
        """GlobRecursive finds *.c files at ALL depths (recursive)."""
        results = scons_env.GlobRecursive("*.c", tmpdir, exclude=["*~"])
        paths = sorted(str(f) for f in results)
        expected = sorted([
            os.path.join(tmpdir, "a.c"),
            os.path.join(tmpdir, "b.c"),
            os.path.join(tmpdir, "sub/d.c"),
            os.path.join(tmpdir, "sub/e.c"),
            os.path.join(tmpdir, "sub/deep/f.c"),
        ])
        assert paths == expected

    def test_recursive_glob(self, scons_env, tmpdir):
        """GlobRecursive */*.c matches files at EXACTLY 1 dir level."""
        results = scons_env.GlobRecursive("*/*.c", tmpdir, exclude=["*~"])
        paths = sorted(str(f) for f in results)
        expected = sorted([
            os.path.join(tmpdir, "sub/d.c"),
            os.path.join(tmpdir, "sub/e.c"),
        ])
        assert paths == expected

    def test_returns_file_nodes(self, scons_env, tmpdir):
        """GlobRecursive returns SCons File nodes, not strings."""
        results = scons_env.GlobRecursive("*.c", tmpdir, exclude=["*~"])
        from SCons.Node.FS import File as FSFile
        for r in results:
            assert isinstance(r, FSFile)

    def test_no_glob_magic_returns_file(self, scons_env, tmpdir):
        """A path without glob magic returns a single File node."""
        results = scons_env.GlobRecursive("a.c", tmpdir, exclude=["*~"])
        assert len(results) == 1
        assert str(results[0]) == os.path.join(tmpdir, "a.c")

    def test_exclude_applied(self, scons_env, tmpdir):
        """Custom exclude patterns remove matching files."""
        results = scons_env.GlobRecursive("*.c", tmpdir, exclude=["a*"])
        paths = [os.path.basename(str(f)) for f in results]
        assert "a.c" not in paths
        assert "b.c" in paths

    def test_matches_scons_fallback(self, scons_env, tmpdir):
        """New implementation returns a subset of the old SCons fallback.
        (The old fallback is overly broad for patterns like */*.c — it
        matches files at greater depth than the pattern specifies — so
        we only assert the new results are contained in the old ones.)"""
        patterns = ["*.c", "*/*.c", "*.h"]
        for pat in patterns:
            new_results = scons_env.GlobRecursive(pat, tmpdir, exclude=["*~"])
            old_results = _scons_glob(scons_env, pat,
                                      scons_env.Dir(tmpdir), ["*~"])
            new_paths = set(str(f) for f in new_results)
            old_paths = set(str(f) for f in old_results)
            extra = new_paths - old_paths
            assert not extra, \
                f"New returns files old does not for {pat!r}: {extra}"

    def test_strings_fallback(self, scons_env, tmpdir):
        """strings=True triggers the SCons fallback."""
        results = scons_env.GlobRecursive("*.c", tmpdir, exclude=["*~"],
                                          strings=True)
        assert all(isinstance(r, str) for r in results)

    def test_source_fallback(self, scons_env, tmpdir):
        """source=True triggers the SCons fallback."""
        results = scons_env.GlobRecursive("*.c", tmpdir, exclude=["*~"],
                                          source=True)
        from SCons.Node.FS import File as FSFile
        assert all(isinstance(r, FSFile) for r in results)


# ---------------------------------------------------------------------------
# GatherSources tests
# ---------------------------------------------------------------------------

class TestGatherSources:
    """Tests for the GatherSources helper."""

    def test_basic(self, scons_env, tmpdir):
        """GatherSources collects files from multiple patterns (recursive)."""
        results = scons_env.GatherSources(["*.c", "*.h"], tmpdir)
        paths = sorted(os.path.basename(str(f)) for f in results)
        expected = sorted(["a.c", "b.c", "c.h", "d.c", "e.c", "f.c"])
        assert paths == expected

    def test_exclusion(self, scons_env, tmpdir):
        """Patterns prefixed with ! are treated as exclusions."""
        results = scons_env.GatherSources(["*.c", "!a*"], tmpdir)
        paths = sorted(os.path.basename(str(f)) for f in results)
        assert paths == ["b.c", "d.c", "e.c", "f.c"]

    def test_deduplication(self, scons_env, tmpdir):
        """Duplicate patterns are deduplicated."""
        results = scons_env.GatherSources(["*.c", "*.c"], tmpdir)
        paths = sorted(os.path.basename(str(f)) for f in results)
        assert paths == ["a.c", "b.c", "d.c", "e.c", "f.c"]


# ---------------------------------------------------------------------------
# FS node pollution regression test
# ---------------------------------------------------------------------------

class TestFSNodePollution:
    """Verify that the improved GlobRecursive does not create excessive
    FS.Dir nodes, which was the original performance bug."""

    def test_no_fsdir_explosion(self, scons_env, tmpdir):
        """GlobRecursive should not create FS.Dir nodes for every visited
        directory — only for directories containing matching files.

        We verify this indirectly: calling GlobRecursive many times on a
        deep tree should complete quickly (not hang or take exponentially
        longer), and the result count should be proportional to actual
        files, not directories.
        """
        # Deeper tree to amplify any potential pollution
        deep = os.path.join(tmpdir, "a", "b", "c", "d")
        os.makedirs(deep)
        _touch(os.path.join(deep, "match.c"))
        # Add many empty directories to ensure they don't pollute results
        for i in range(20):
            os.makedirs(os.path.join(tmpdir, f"empty{i}", "sub"))
            _touch(os.path.join(tmpdir, f"empty{i}", "real.c"))

        # Glob for *.c — returns all .c files at all depths
        # 5 from fixture (a.c, b.c, sub/d.c, sub/e.c, sub/deep/f.c)
        # + 1 match.c in a/b/c/d + 20 real.c in emptyN = 26 total
        results = scons_env.GlobRecursive("*.c", tmpdir, exclude=["*~"])
        assert len(results) == 26, \
            f"Expected 26 files, got {len(results)}"

        # Glob for */*.c — matches files with EXACTLY 1 dir component
        # fixture: sub/d.c, sub/e.c (2 at depth 1), sub/deep/f.c (depth 2, SKIP)
        # a/b/c/d/match.c is at depth 4 → SKIP
        # 20 emptyN/real.c at depth 1 → included
        # Total: 2 + 20 = 22
        results2 = scons_env.GlobRecursive("*/*.c", tmpdir, exclude=["*~"])
        assert len(results2) == 22, \
            f"Expected 22 depth==1 files, got {len(results2)}"

        # Verify performance: many globs on the same tree should be fast
        import time
        start = time.time()
        for _ in range(10):
            scons_env.GlobRecursive("*.c", tmpdir, exclude=["*~"])
        elapsed = time.time() - start
        # 10 globs on a 20-dir tree should take well under 1 second
        assert elapsed < 1.0, \
            f"10 globs took {elapsed:.2f}s, expected < 1.0s"



