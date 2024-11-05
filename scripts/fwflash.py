#!/usr/bin/env python3

import argparse
import signal
import subprocess
from typing import Iterable

from flipper.app import App
from flipper.debug import BaseDebugExtension, GdbConfigurationManager, GdbParam
from flipper.debug.extensions import (
    CoreConfigurationExtension,
    RemoteParametesExtension,
)


class FlashExtension(BaseDebugExtension):

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "--verify",
            "-v",
            action="store_true",
            help="Verify flash after programming",
            default=False,
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        yield GdbParam("-batch", True)

        yield GdbParam(f"load")
        if args.verify:
            yield GdbParam(f"compare-sections")
        yield GdbParam(f"quit")


GdbConfigurationManager.register_extension(CoreConfigurationExtension)
GdbConfigurationManager.register_extension(RemoteParametesExtension)
GdbConfigurationManager.register_extension(FlashExtension)


class Main(App):
    GDB_BIN = "arm-none-eabi-gdb-py3"

    def init(self):
        GdbConfigurationManager.configure_arg_parser(self.parser)
        self.parser.set_defaults(func=self.run)

    def run(self):
        signal.signal(signal.SIGINT, signal.SIG_IGN)

        mgr = GdbConfigurationManager()
        proc = None
        try:
            gdb_args = [self.GDB_BIN]
            gdb_args.extend(mgr.get_gdb_args(self.args))
            self.logger.debug(f"Running: {gdb_args}")
            proc = subprocess.run(gdb_args)
            return 0
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
