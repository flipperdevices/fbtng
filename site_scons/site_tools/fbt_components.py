import os
import posixpath

from SCons.Errors import StopError


def FindComponentSconscript(env, module, dir_node=None):
    if env["VERBOSE"]:
        print(f"FindComponentSconscript: {module} {dir_node}")
    if not dir_node:
        dir_node = env.Dir(".")

    # We cannot operate on scons nodes here, because mere construction of the
    # node will leave a "virtual" node that will be later globbable.
    paths_to_check = [f"{module}/SConscript", f"{module}.scons"]
    for repo_dir in dir_node.get_all_rdirs():
        # print(f"FindComponentSconscript: {module} {dir_node} {repo_dir}")
        for path in paths_to_check:
            # print(f"Checking {posixpath.join(repo_dir.abspath, path)}")
            if os.path.exists(candidate_path := posixpath.join(repo_dir.abspath, path)):
                if env["VERBOSE"]:
                    print(f"FindComponentSconscript: {module} -> {candidate_path}")
                return candidate_path

    # return module_sconscript


def RegisterComponent(env, component_name, node_or_path):
    if env["VERBOSE"]:
        print(f"RegisterComponent: {component_name} -> {node_or_path}")
    if component_name in env["FBT_SCRIPT_LIB"]:
        raise StopError(f"Component {component_name} already registered")
    env["FBT_SCRIPT_LIB"][component_name] = node_or_path


def GetComponent(env, component_name):
    script = env["FBT_SCRIPT_LIB"].get(component_name)
    if not script:
        raise StopError(f"Script lookup failed: {component_name}")
    if env["VERBOSE"]:
        print(f"GetComponent: {component_name} -> {script}")
    return script


def _run_component_registrations(env):
    for component_script in env["FBT_COMPONENT_SCRIPTS"]:
        if env["VERBOSE"]:
            print(f"Registering components from {component_script}")
        env.SConscript(component_script, exports={"env": env})


def generate(env):
    env.SetDefault(
        FBT_SCRIPT_LIB=dict(),
        FBT_COMPONENT_SCRIPTS=[],
    )

    env.AddMethod(FindComponentSconscript)
    env.AddMethod(RegisterComponent)
    env.AddMethod(GetComponent)

    _run_component_registrations(env)


def exists(env):
    return True
