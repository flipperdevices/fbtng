#!/usr/bin/env python3

import signal
import subprocess
from pathlib import Path
from typing import Iterator, Type

from flipper.app import App
from flipper.debug import GdbConfigurationManager
from flipper.debug.extensions import (
    BaseDebugExtension,
    CoreConfigurationExtension,
    ExtraCommandsExtension,
    RemoteParametersExtension,
    RTOSExtension,
    SVDLoaderExtension,
)
import flipper.debug.extensions as flipper_debug_extensions

GdbConfigurationManager.register_extension(CoreConfigurationExtension)
GdbConfigurationManager.register_extension(RemoteParametersExtension)
GdbConfigurationManager.register_extension(SVDLoaderExtension)
GdbConfigurationManager.register_extension(RTOSExtension)
GdbConfigurationManager.register_extension(ExtraCommandsExtension)

def enumerate_user_extensions(extensions_dir: Path) -> Iterator[Type]:
    from importlib.util import spec_from_file_location, module_from_spec
    from inspect import isclass

    for py_file in extensions_dir.glob("*.py"):
        if py_file.name.startswith("_"):
            continue

        module_name = f"_debug_extension_{py_file.stem}"

        spec = spec_from_file_location(module_name, py_file)
        if spec is None or spec.loader is None:
            continue

        module = module_from_spec(spec)
        spec.loader.exec_module(module)

        extension_fn = getattr(module, "extensions", None)
        if extension_fn is None or not callable(extension_fn):
            continue

        yield from extension_fn(flipper_debug_extensions)


class Main(App):
    GDB_BIN = "arm-none-eabi-gdb-py3"

    def init(self):
        GdbConfigurationManager.configure_arg_parser(self.parser)
        self.parser.add_argument(
            "--batch",
            action="store_true",
            help="Run GDB in batch (non-interactive) mode",
        )
        self.parser.add_argument(
            "--user-extensions",
            type=Path,
            help="Path to user extensions folder")
        self.parser.set_defaults(func=self.run)

        known_args, _ = parser.parse_known_args()
        if known_args.user_extensions:
            self.logger.debug(f"Loading user extensions from {known_args.user_extensions}")
            for ext in enumerate_user_extensions(known_args.user_extensions):
                GdbConfigurationManager.register_extension(ext)

    def run(self):
        signal.signal(signal.SIGINT, signal.SIG_IGN)

        mgr = GdbConfigurationManager()
        proc = None
        try:
            gdb_args = [self.GDB_BIN]
            if self.args.batch:
                gdb_args.append("-batch")
            gdb_args.extend(mgr.get_gdb_args(self.args))
            self.logger.debug(f"Running: {gdb_args}")
            proc = subprocess.run(gdb_args)
            return proc.returncode if self.args.batch else 0
        except Exception as e:
            self.logger.error(f"Error: {e}")
            if self.args.debug:
                raise
            return 1
        except (KeyboardInterrupt, SystemExit):
            if proc:
                proc.terminate()
            return 1


if __name__ == "__main__":
    Main()()
