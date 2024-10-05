import os
import posixpath

from SCons.Errors import UserError, StopError


# TODO: move to a separate module
def PhonyTarget(env, name, action, source=None, **kw):
    if not source:
        source = []
    phony_name = "phony_" + name
    env.Pseudo(phony_name)
    command = env.Command(phony_name, source, action, **kw)
    env.AlwaysBuild(env.Alias(name, command))
    return command


# TODO: move to a separate module
def ChangeFileExtension(env, fnode, ext):
    return env.File(f"#{os.path.splitext(fnode.path)[0]}{ext}")


def FindModuleSconscript(env, module):
    # We cannot operate on scons nodes here, because mere construction of the
    # node will leave a "virtual" node that will be later globbable.
    src_dir = str(env.Dir(".").srcdir or os.getcwd())
    module_sconscript = posixpath.join(src_dir, module, "SConscript")
    if not os.path.exists(module_sconscript):
        module_sconscript = posixpath.join(src_dir, f"{module}.scons")
        if not os.path.exists(module_sconscript):
            raise UserError(f"Cannot build module {module}: scons file not found")
    # print(f"FindModuleSconscript: {module}->{module_sconscript}")
    return module_sconscript


def BuildModule(env, module):
    module_sconscript = env.FindModuleSconscript(module)

    # print(posixpath.join(env.subst("$BUILD_DIR"), module))
    # print(env.Dir("$BUILD_DIR/" + module).path)
    # print()
    env.AppendUnique(PY_LINT_SOURCES=[module_sconscript])
    return env.SConscript(
        module_sconscript,
        variant_dir=env.Dir("$BUILD_DIR/" + module),
        src_dir=env.Dir("#"),
        duplicate=0,
    )


def BuildModules(env, modules):
    result = []
    for module in modules:
        build_res = env.BuildModule(module)
        # print("module ", module, build_res)
        if build_res is None:
            continue
        result.append(build_res)
    return result


def RegisterComponent(env, component_name, node_or_path):
    # print(f"RegisterComponent: {component_name} -> {node_or_path}")
    if component_name in env["FBT_SCRIPT_LIB"]:
        raise StopError(f"Component {component_name} already registered")
    env["FBT_SCRIPT_LIB"][component_name] = node_or_path


def GetComponent(env, component_name):
    script = env["FBT_SCRIPT_LIB"].get(component_name)
    if not script:
        raise StopError(f"Script lookup failed: {component_name}")
    # print(f"GetComponent: {component_name} -> {script}")
    return script


def CheckEnvVariable(env, name):
    if name not in env or not env.subst("${" + name + "}"):
        raise UserError(
            f"Variable {name} not set in env, is module configuration wrong?"
        )


def generate(env):
    env.SetDefault(
        FBT_SCRIPT_LIB=dict(),
        FBT_COMPONENT_DIRS=[],
    )

    # TODO: move to a separate module
    env.AddMethod(PhonyTarget)
    env.AddMethod(ChangeFileExtension)

    env.AddMethod(FindModuleSconscript)
    env.AddMethod(BuildModule)
    env.AddMethod(BuildModules)
    env.AddMethod(RegisterComponent)
    env.AddMethod(GetComponent)
    env.AddMethod(CheckEnvVariable)

    for component_dir in env["FBT_COMPONENT_DIRS"]:
        # We eval the component dir and expect it to register components
        # print(f"Running reg for component dir: {component_dir}")
        env.SConscript(
            component_dir.File("SConscript"),  # FIXME
            exports={
                "env": env,
            },
        )


def exists(env):
    return True
