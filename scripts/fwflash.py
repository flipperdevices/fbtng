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
from fwinterface import OpenOCDAdapter, OpenOCDCommandLineParameter


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

        yield GdbParam(f"set confirm off")
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

    def get_faster_flash_cmdline(self, adapter) -> list[str]:
        if isinstance(adapter, OpenOCDAdapter):
            flash_cmd = OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.COMMAND,
                " ".join(
                    [
                        "program",
                        f'"{self.args.file}"',
                        "verify" if self.args.verify else "",
                        "reset",
                        "exit",
                    ]
                ),
            )

            return [*adapter.to_args(), *flash_cmd.to_str_args()]
        return []

    def run(self):
        signal.signal(signal.SIGINT, signal.SIG_IGN)

        mgr = GdbConfigurationManager()
        proc = None
        try:
            remote_mgr = mgr.get_extension(RemoteParametesExtension)
            print(remote_mgr._dicover_adapter_for_args(self.args))
            if flash_cmd := self.get_faster_flash_cmdline(
                remote_mgr._dicover_adapter_for_args(self.args)
            ):
                self.logger.info(f"Using fast flash command line: {flash_cmd}")
            else:
                flash_cmd = [self.GDB_BIN]
                flash_cmd.extend(mgr.get_gdb_args(self.args))

            self.logger.debug(f"Running: {flash_cmd}")
            proc = subprocess.run(flash_cmd, check=True)
            return proc.returncode
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
