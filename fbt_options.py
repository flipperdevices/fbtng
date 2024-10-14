from pathlib import Path

# For details on available options, run 'fbt -h'

# Default hardware target
TARGET_HW = "7"


custom_options_fn = "fbt_options_local.py"

if Path(custom_options_fn).exists():
    exec(compile(Path(custom_options_fn).read_text(), custom_options_fn, "exec"))
