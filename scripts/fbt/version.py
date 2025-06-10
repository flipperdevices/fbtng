import datetime
import subprocess
from functools import cache


@cache
def get_git_commit_unix_timestamp(target_dir=None):
    return int(
        subprocess.check_output(["git", "show", "-s", "--format=%ct"], cwd=target_dir)
        .strip()
        .decode()
    )


@cache
def get_fast_git_version_id(target_dir=None):
    try:
        version = (
            subprocess.check_output(
                [
                    "git",
                    "describe",
                    "--always",
                    "--dirty",
                    "--all",
                    "--long",
                ],
                cwd=target_dir,
            )
            .strip()
            .decode()
        )
        return (version, datetime.date.today())
    except Exception as e:
        print("Failed to check for git changes", e)
