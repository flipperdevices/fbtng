from flipper.utils.templite import Templite

from SCons.Action import Action
from SCons.Builder import Builder
from SCons.Errors import UserError
from SCons.Util import is_Dict, is_Sequence, is_String
from SCons.Warnings import WarningOnByDefault, warn


def format_templite(target, source, env):
    source_path = source[0].abspath
    # source_tmpl_contents = source[0].get_text_contents()
    target_path = target[0].abspath

    if "SUBST_DICT" not in env:
        subs = {}  # no substitutions
        warn(
            WarningOnByDefault,
            "No substitutions provided for template file %s" % source_path,
        )
    else:
        subst_dict = env["SUBST_DICT"]

    template = Templite(filename=source_path)
    output = template.render(**subst_dict)
    with open(target_path, "w") as f:
        f.write(output)


def generate(env):
    env.SetDefault(TEMPLITECOMSTR="\tTMPLITE\t$TARGET")
    env.Append(
        BUILDERS={
            "TempliteFile": Builder(
                action=Action(
                    format_templite,
                    "${TEMPLITECOMSTR}",
                ),
            ),
        }
    )


def exists(env):
    return True
