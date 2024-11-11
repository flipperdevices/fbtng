import os

from SCons.Errors import StopError, UserError
from SCons.Node import Node
from SCons.Node.FS import Entry, Dir, File


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


def Real(env, path, no_raise=False):
    if not isinstance(path, Entry):
        path = env.File(path)
    elif isinstance(path, Node.FS.EntryProxy):
        path = path.get()

    if isinstance(path, Dir):
        # may return wrong dir if there are multiple repos
        return path.srcnode()
    elif isinstance(path, File):
        return path.rfile()
    elif no_raise:
        return None
    raise StopError(f"Can't find absolute path for {path.name} ({path})")


def ChangeFileExtension(env, fnode, ext):
    return env.File(f"#{os.path.splitext(fnode.path)[0]}{ext}")


def generate(env):
    env.AddMethod(PhonyTarget)
    env.AddMethod(ChangeFileExtension)
    env.AddMethod(CheckEnvVariable)
    env.AddMethod(Real)


def exists(env):
    return True
