import argparse
import pathlib
from collections.abc import Iterable
from typing import Optional

from flipper.utils.hw_platform import FbtHardwarePlatform
from fwinterface import INTERFACES, discover_probes

from . import BaseDebugExtension, GdbParam


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
                *INTERFACES.keys(),
            ),
            default=RemoteParametesExtension.DEFAULT_ADAPTER_INTERFACE,
            help="Interface to use",
        )
        parser.add_argument(
            "-p",
            "--platform",
            help="Configuration file for the target device's platform",
            required=True,
            type=FbtHardwarePlatform.from_file,
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

        return self._build_connection_args(args.platform, serial, interface)

    def _build_connection_args(
        self,
        platform: FbtHardwarePlatform,
        adapter_sn: Optional[str],
        interface: str | None,
    ):
        adapter = self._discover_adapter(platform, adapter_sn, interface)
        return adapter.to_connection_args()

    def _discover_adapter(
        self, platform: FbtHardwarePlatform, adapter_sn: Optional[str], interface: str
    ):
        if interface.upper() == self.DEFAULT_ADAPTER_INTERFACE.upper():
            interface = None
        elif interface in INTERFACES:
            interface = INTERFACES[interface]
        else:
            raise Exception(
                f"Unknown interface: {interface}. Available interfaces: {list(INTERFACES.keys())}"
            )

        adapters = discover_probes(platform, adapter_sn, interface)
        if not adapters:
            raise Exception("Debug adapter not found")
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

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        yield GdbParam(f"--quiet", is_file=True)  # Suppress the welcome message
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
            action="store_true",
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        if args.with_svd:
            yield GdbParam(f"source {args.root / self.CORTEX_DEBUG_SCRIPT}")
            yield GdbParam(f"svd_load {args.platform.svd_file}")


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
            "--with-fwversion",
            help="Enable support for firmware versioning",
            action="store_true",
        )

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        if args.apps_root:
            yield GdbParam(f"source {args.root / self.APPS_SCRIPT}")
            yield GdbParam(f"fap-set-debug-elf-root {args.apps_root[0]}")
        if args.with_fwversion:
            yield GdbParam(f"source {args.root / self.FIRMWARE_SCRIPT}")
            yield GdbParam("fw-version")
