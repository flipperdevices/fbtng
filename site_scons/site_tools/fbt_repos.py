from SCons.Errors import StopError
from SCons.Script import Action
from SCons.Defaults import Copy
import os
import multiprocessing


def initialize_repo_dir(env, repo_dir):
    if not repo_dir.exists():
        raise StopError(f"Repository directory does not exist: {repo_dir}")

    git_env = env.Clone(ENV=os.environ)
    if not os.environ.get("FBT_NO_SYNC"):
        if git_env.Execute(
            Action(
                [
                    [
                        "git",
                        "-C",
                        repo_dir.abspath,
                        "submodule",
                        "update",
                        "--init",
                        "--recursive",
                        "--jobs",
                        multiprocessing.cpu_count(),
                    ]
                ],
                f"Updating submodules for {repo_dir}" if env["VERBOSE"] else None,
            ),
            target=repo_dir,
        ):
            raise StopError(f"Failed to update submodules for {repo_dir}")
    for config_file_name in env["FBT_REPO_CONFIG_FILES"]:
        if not repo_dir.File(config_file_name).exists():
            git_env.Execute(
                Copy(repo_dir.File(config_file_name), env.File(config_file_name)),
            )

    # All good, add the repository to the environment
    env.Repository(repo_dir)


def InitializeRepositories(env):
    # TODO: also iterate over repos supplied via command line (-Y)
    for repo_path in env.get("FBT_EXTRA_REPOS", []):
        initialize_repo_dir(env, env.Dir(repo_path))


def generate(env):
    env.SetDefault(
        FBT_EXTRA_REPOS=[],
        FBT_REPO_CONFIG_FILES=[".clang-format", ".clangd"],
    )
    env.AddMethod(InitializeRepositories, "InitializeRepositories")


def exists(env):
    return True
