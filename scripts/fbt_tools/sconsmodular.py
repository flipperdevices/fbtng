import os
import posixpath

from SCons.Errors import UserError, StopError


# class ComponentLibrary:
#     def __init__(self):
#         pass

#     def register(self, component_name, node_or_path):
#         pass

#     def get(self, component_name):
#         pass


def FindModuleSconscript(env, module):
    # We cannot operate on scons nodes here, because mere construction of the
    # node will leave a "virtual" node that will be later globbable.
    src_dir = str(env.Dir(".").srcdir or os.getcwd())
    module_sconscript = posixpath.join(src_dir, module, "SConscript")
    if not os.path.exists(module_sconscript):
        module_sconscript = posixpath.join(src_dir, f"{module}.scons")
        if not os.path.exists(module_sconscript):
            raise UserError(f"Cannot build module {module}: scons file not found")
    print(f"FindModuleSconscript: {module}->{module_sconscript}")
    return module_sconscript


def BuildModule(env, module):
    module_sconscript = env.FindModuleSconscript(module)

    # print(posixpath.join(env.subst("$BUILD_DIR"), module))
    # print(env.Dir("$BUILD_DIR/" + module).path)
    # print()
    env.Append(PY_LINT_SOURCES=[module_sconscript])
    return env.SConscript(
        module_sconscript,
        variant_dir=env.Dir("$BUILD_DIR/" + module),
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


def PhonyTarget(env, name, action, source=None, **kw):
    if not source:
        source = []
    phony_name = "phony_" + name
    env.Pseudo(phony_name)
    command = env.Command(phony_name, source, action, **kw)
    env.AlwaysBuild(env.Alias(name, command))
    return command


def ChangeFileExtension(env, fnode, ext):
    return env.File(f"#{os.path.splitext(fnode.path)[0]}{ext}")


def RegisterComponent(env, component_name, node_or_path):
    print(f"RegisterComponent: {component_name} -> {node_or_path}")
    env["FBT_SCRIPT_LIB"][component_name] = node_or_path


def GetComponent(env, component_name):
    script = env["FBT_SCRIPT_LIB"].get(component_name)
    if not script:
        raise StopError(f"Script lookup failed: {component_name}")
    print(f"GetComponent: {component_name} -> {script}")
    return script


def generate(env):
    env.SetDefault(FBT_SCRIPT_LIB=dict())

    env.AddMethod(FindModuleSconscript)
    env.AddMethod(BuildModule)
    env.AddMethod(BuildModules)
    env.AddMethod(PhonyTarget)
    env.AddMethod(ChangeFileExtension)

    env.AddMethod(RegisterComponent)
    env.AddMethod(GetComponent)


def exists(env):
    return True
