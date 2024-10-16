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
                # self.AUTO_INTERFACE,
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
            help="Path to the ELF file to flash",
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
        # TODO: add verify logic
        # self.parser.add_argument(
        #     "--verify",
        #     "-v",
        #     action="store_true",
        #     help="Verify flash after programming",
        #     default=False,
        # )

        # self.parser.add_argument(
        #     "--extra-commands",
        #     action="append",
        #     default=[],
        # )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        yield GdbParam(f"--quiet", True)  # Suppress the welcome message
        if args.init:
            yield GdbParam(f"source {args.root / self.GDBINIT}")
        if args.file:
            yield GdbParam(args.file, is_file=True)
        if args.compare:
            yield GdbParam("compare-sections")


class FlashExtension(BaseDebugExtension):

    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        pass

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        yield GdbParam(f"load")
        yield GdbParam(f"quit")


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
        return chain.from_iterable(ext_arg_iterables)


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
            return 1
        except (KeyboardInterrupt, SystemExit):
            if proc:
                proc.terminate()
            return 1


if __name__ == "__main__":
    Main()()

#
# class Main(App):
#     AUTO_INTERFACE = "auto"
#
#     def init(self):
#         Programmer.root_logger = self.logger
#
#         self.parser.add_argument(
#             "filename",
#             type=str,
#             help="File to flash",
#         )
#         self.parser.add_argument(
#             "--verify",
#             "-v",
#             action="store_true",
#             help="Verify flash after programming",
#             default=False,
#         )
#         self.parser.add_argument(
#             "--interface",
#             choices=(
#                 self.AUTO_INTERFACE,
#                 *[i.get_name() for i in all_flash_interfaces],
#             ),
#             type=str,
#             default=self.AUTO_INTERFACE,
#             help="Interface to use",
#         )
#         self.parser.add_argument(
#             "--serial",
#             type=str,
#             default=self.AUTO_INTERFACE,
#             help="Serial number or port of the programmer",
#         )
#         self.parser.add_argument(
#             "-t",
#             "--target",
#             help="Target device",
#             required=True,
#             choices=TARGETS.keys(),
#         )
#         # self.parser.add_argument(
#         #     "--target",
#         #     type=str,
#         #     default="target/stm32u5x.cfg",
#         #     help="OpenOCD interface file for acessing target MPU",
#         # )
#         self.parser.add_argument(
#             "--extra-commands",
#             action="append",
#             default=[],
#         )
#         self.parser.set_defaults(func=self.flash)
#
#     def _search_interface(self, interface_list: list[Programmer]) -> list[Programmer]:
#         # target = self.args.target
#         # self.logger.error(f"target: {target}, from dict: {TARGETS.get(target)}")
#         # self.logger.error(f"serial: {self.args.serial}")
#         #
#         # if target not in TARGETS:
#         #     raise Exception(
#         #         f"Unknown target: {target}. Available targets: {list(TARGETS.keys())}"
#         #     )
#         # adapters = discover_probes(TARGETS.get(target), None)
#         # if not adapters:
#         #     raise Exception("No debug adapter found")
#         # if len(adapters) > 1:
#         #     raise Exception(
#         #         f"Multiple debug adapters found: {adapters}, specify one with --serial"
#         #     )
#         # print('hhh')
#         # print(f"Using {adapters[0]}")
#         # return adapters[0]
#
#         found_programmers = []
#
#         for p in interface_list:
#             name = p.get_name()
#             if (serial := self.args.serial) != self.AUTO_INTERFACE:
#                 p.set_serial(serial)
#                 self.logger.debug(f"Trying {name} with {serial}")
#             else:
#                 self.logger.debug(f"Trying {name}")
#
#             p.set_mcu_interface_config(self.args.target)
#             p.add_programming_commands(self.args.extra_commands)
#
#             if p.probe():
#                 self.logger.debug(f"Found {name}")
#                 found_programmers.append(p)
#             else:
#                 self.logger.debug(f"Failed to probe {name}")
#
#         return found_programmers
#
#     def flash(self):
#         start_time = time.time()
#         file_path = os.path.abspath(self.args.filename)
#
#         if not os.path.exists(file_path):
#             self.logger.error(f"Binary file not found: {file_path}")
#             return 1
#
#         if self.args.interface != self.AUTO_INTERFACE:
#             available_interfaces = list(
#                 filter(
#                     lambda p: p.get_name() == self.args.interface,
#                     all_flash_interfaces,
#                 )
#             )
#
#         else:
#             self.logger.info("Probing for local interfaces...")
#             available_interfaces = self._search_interface(local_flash_interfaces)
#             self.logger.error(f"interfaces: {available_interfaces}")
#
#             if not available_interfaces:
#                 # Probe network blackmagic
#                 self.logger.info("Probing for network interfaces...")
#                 available_interfaces = self._search_interface(network_flash_interfaces)
#
#             if not available_interfaces:
#                 self.logger.error("No available interfaces")
#                 return 1
#             elif len(available_interfaces) > 1:
#                 self.logger.error("Multiple interfaces found:")
#                 self.logger.error(
#                     f"Please specify '--interface={[i.get_name() for i in available_interfaces]}'"
#                 )
#                 return 1
#
#         interface = available_interfaces.pop(0)
#         interface.set_mcu_interface_config(self.args.target)
#
#         if self.args.serial != self.AUTO_INTERFACE:
#             interface.set_serial(self.args.serial)
#             self.logger.info(f"Using {interface.get_name()} with {self.args.serial}")
#         else:
#             self.logger.info(f"Using {interface.get_name()}")
#         self.logger.info(f"Flashing {file_path}")
#
#         if not interface.flash(file_path, self.args.verify):
#             self.logger.error(f"Failed to flash via {interface.get_name()}")
#             return 1
#
#         flash_time = time.time() - start_time
#         self.logger.info(f"Flashed successfully in {flash_time:.2f}s")
#         if file_path.endswith(".bin"):
#             bin_size = os.path.getsize(file_path)
#             self.logger.info(
#                 f"Effective speed: {bin_size / flash_time / 1024:.2f} KiB/s"
#             )
#         return 0
#
#
# if __name__ == "__main__":
#     Main()()
