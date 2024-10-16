#!/usr/bin/env python3

import argparse
import pathlib
import shlex
import signal
import subprocess
from collections.abc import Iterable
from dataclasses import dataclass
from itertools import chain
from typing import Optional

from flipper.app import App
from fwinterface import TARGETS, OpenOCDCommandLineParameter, discover_probes, INTERFACES

"""
This script is a wrapper for gdb that will autodetect current debug adapter and
set up gdb for debugging with it. It will also load the correct gdbinit file and supplementary scripts.
"""


@dataclass
class GdbParam:
    command: str
    is_file: bool = False

    def __next__(self):
        while self.__items:
            return self.__items.pop(0)
        raise StopIteration

    def __iter__(self):
        self.__items = []
        if not self.is_file:
            self.__items.append("-ex")
        self.__items.append(self.command)
        return self


class BaseDebugExtension:
    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        raise NotImplementedError

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        raise NotImplementedError


class RemoteParametesExtension(BaseDebugExtension):
    DEFAULT_ADAPTER_SERIAL = "auto"
    DEFAULT_ADAPTER_INTERFACE = "auto"

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "--serial",
            help="Serial number of the debug adapter",
            nargs=1,
            default=[RemoteParametesExtension.DEFAULT_ADAPTER_SERIAL],
        )
        parser.add_argument(
            "-i",
            "--interface",
            choices=(
                RemoteParametesExtension.DEFAULT_ADAPTER_INTERFACE,
                INTERFACES.keys()
            ),
            default=RemoteParametesExtension.DEFAULT_ADAPTER_INTERFACE,
            help="Interface to use",
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        self._debug_root = args.root
        for arg in self._get_remote(args):
            yield GdbParam(arg, True)

    def _get_remote(self, args: argparse.Namespace):
        serial = args.serial[0]
        interface = args.interface

        if serial.upper() == self.DEFAULT_ADAPTER_SERIAL.upper():
            serial = None

        return self._build_connection_args(args.target, serial, interface)

    def _build_connection_args(self, target: str, adapter_sn: Optional[str], interface: str | None):
        adapter = self._discover_adapter(target, adapter_sn, interface)
        return adapter.to_connection_args()

    def _discover_adapter(self, target: str, adapter_sn: Optional[str], interface: str):
        if target not in TARGETS:
            raise Exception(
                f"Unknown target: {target}. Available targets: {list(TARGETS.keys())}"
            )

        if interface.upper() == self.DEFAULT_ADAPTER_INTERFACE.upper():
            interface = None
        elif interface in INTERFACES:
            interface = INTERFACES[interface]
        else:
            raise Exception(
                f"Unknown interface: {interface}. Available interfaces: {list(INTERFACES.keys())}"
            )

        adapters = discover_probes(TARGETS.get(target), adapter_sn, interface)
        if not adapters:
            raise Exception("No debug adapter found")
        if len(adapters) > 1:
            raise Exception(
                f"Multiple debug adapters found: {adapters}, specify one with --serial"
            )
        return adapters[0]


class CoreConfigurationExtension(BaseDebugExtension):
    GDBINIT = "gdbinit"

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "--init",
            help="Execute the gdbinit file",
            action="store_true",
            default=False,
        )
        parser.add_argument(
            "-t",
            "--target",
            help="Target device",
            required=True,
            choices=TARGETS.keys(),
        )
        parser.add_argument(
            "file",
            help="Path to the ELF file to debug",
            nargs="?",
        )
        parser.add_argument(
            "--root",
            help="Path to the root of the debugging configuration",
            default="scripts/debug",
            type=pathlib.Path,
        )
        parser.add_argument(
            "--compare",
            help="Compare the ELF file with the target",
            action="store_true",
            default=False,
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        yield GdbParam(f"--quiet", True)  # Suppress the welcome message
        if args.init:
            yield GdbParam(f"source {args.root / self.GDBINIT}")
        if args.file:
            yield GdbParam(args.file, is_file=True)
        if args.compare:
            yield GdbParam("compare-sections")


class ExtraCommandsExtension(BaseDebugExtension):
    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "-ex",
            help="Execute a gdb command",
            action="append",
            dest="extra_commands",
            default=[],
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        for command in args.extra_commands:
            yield GdbParam(command)


class RTOSExtension(BaseDebugExtension):
    RTOS_SCRIPT = "FreeRTOS/FreeRTOS.py"

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "--with-rtos",
            help="Enable RTOS support",
            action="store_true",
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        if args.with_rtos:
            yield GdbParam(f"source {args.root / self.RTOS_SCRIPT}")


class SVDLoaderExtension(BaseDebugExtension):
    CORTEX_DEBUG_SCRIPT = pathlib.Path("PyCortexMDebug/PyCortexMDebug.py")

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "--with-svd",
            help="Enable SVD loader",
            dest="svd_file",
            type=pathlib.Path,
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        if args.svd_file:
            yield GdbParam(f"source {args.root / self.CORTEX_DEBUG_SCRIPT}")
            yield GdbParam(f"svd_load {args.svd_file}")


class FlipperScriptsExtension(BaseDebugExtension):
    APPS_SCRIPT = "flipperapps.py"
    FIRMWARE_SCRIPT = "flipperversion.py"

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        parser.add_argument(
            "--with-apps",
            help="Enable support for apps debugging",
            dest="apps_root",
            nargs=1,
            type=pathlib.Path,
        )
        parser.add_argument(
            "--with-firmware",
            help="Enable support for firmware versioning",
            action="store_true",
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        if args.apps_root:
            yield GdbParam(f"source {args.root/self.APPS_SCRIPT}")
            yield GdbParam(f"fap-set-debug-elf-root {args.apps_root[0]}")
        if args.with_firmware:
            yield GdbParam(f"source {args.root/self.FIRMWARE_SCRIPT}")
            yield GdbParam("fw-version")


## Debug extensions:
# - Debug interface: CMSIS-DAP, ST-Link, J-Link, ... (G,O)
# - Target device configuration: STM32U5x, STM32F4x, STM32WBx, ... (G,O)
# - SVD loader (G)
# - FBT apps support (G)
# - FBT firmware version (G)
# - ELF file (G)
# - RTOS support (G)


class GdbConfigurationManager:
    extension_classes = []

    @classmethod
    def register_extension(cls, extension_kls: BaseDebugExtension):
        cls.extension_classes.append(extension_kls)

    def __init__(self):
        self.extensions = [extension() for extension in self.extension_classes]

    @classmethod
    def configure_arg_parser(cls, parser: argparse.ArgumentParser):
        for extension_cls in cls.extension_classes:
            extension_cls.configure_arg_parser(parser)

    def get_gdb_args(self, args: argparse.Namespace) -> Iterable[str]:
        ext_arg_iterables = chain.from_iterable(
            extension.append_gdb_args(args) for extension in self.extensions
        )
        # print(ext_arg_iterables)
        return chain.from_iterable(ext_arg_iterables)


GdbConfigurationManager.register_extension(CoreConfigurationExtension)
GdbConfigurationManager.register_extension(RemoteParametesExtension)
# GdbConfigurationManager.register_extension(FlipperScriptsExtension)
# GdbConfigurationManager.register_extension(SVDLoaderExtension)
# GdbConfigurationManager.register_extension(RTOSExtension)
# GdbConfigurationManager.register_extension(ExtraCommandsExtension)


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
            return 1
        except (KeyboardInterrupt, SystemExit):
            if proc:
                proc.terminate()
            return 1


if __name__ == "__main__":
    Main()()
