#!/usr/bin/env python3

import signal
import subprocess

from flipper.app import App
from flipper.debug import GdbConfigurationManager
from flipper.debug.extensions import (
    CoreConfigurationExtension,
    ExtraCommandsExtension,
    FlipperScriptsExtension,
    RemoteParametersExtension,
    RTOSExtension,
    SVDLoaderExtension,
)

GdbConfigurationManager.register_extension(CoreConfigurationExtension)
GdbConfigurationManager.register_extension(RemoteParametersExtension)
GdbConfigurationManager.register_extension(FlipperScriptsExtension)
GdbConfigurationManager.register_extension(SVDLoaderExtension)
GdbConfigurationManager.register_extension(RTOSExtension)
GdbConfigurationManager.register_extension(ExtraCommandsExtension)


class Main(App):
    GDB_BIN = "arm-none-eabi-gdb-py3"

    def init(self):
        GdbConfigurationManager.configure_arg_parser(self.parser)
        self.parser.add_argument(
            "--batch",
            action="store_true",
            help="Run GDB in batch (non-interactive) mode",
        )
        self.parser.set_defaults(func=self.run)

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
