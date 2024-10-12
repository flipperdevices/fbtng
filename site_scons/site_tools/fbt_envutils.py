import os

from SCons.Errors import StopError, UserError
from SCons.Node import FS


def PhonyTarget(env, name, action, source=None, **kw):
    if not source:
        source = []
    phony_name = "phony_" + name
    env.Pseudo(phony_name)
    command = env.Command(phony_name, source, action, **kw)
    env.AlwaysBuild(env.Alias(name, command))
    return command


def CheckEnvVariable(env, name):
    if name not in env or not env.subst("${" + name + "}"):
        raise UserError(
            f"Variable {name} not set in env, is module configuration wrong?"
        )


def Real(env, node, no_raise=False):
    if isinstance(node, FS.EntryProxy):
        node = node.get()

    for repo_dir in node.get_all_rdirs():
        if os.path.exists(repo_dir.abspath):
            print(f"Real: {node} -> {repo_dir.abspath}")
            return repo_dir

    if not no_raise:
        raise StopError(f"Can't find absolute path for {node.name} ({node})")


def ChangeFileExtension(env, fnode, ext):
    return env.File(f"#{os.path.splitext(fnode.path)[0]}{ext}")


def generate(env):
    env.AddMethod(PhonyTarget)
    env.AddMethod(ChangeFileExtension)
    env.AddMethod(CheckEnvVariable)
    env.AddMethod(Real)


def exists(env):
    return True
