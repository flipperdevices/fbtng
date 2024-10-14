import enum
import itertools
import os
import shlex
import subprocess
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Iterable, Optional, Callable
import socket
import serial.tools.list_ports as list_ports

from logging import getLogger

logger = getLogger(__name__)


class SocketService:
    @staticmethod
    def valid_ip(address) -> bool:
        try:
            socket.inet_aton(address)
            return True
        except Exception:
            return False

    @staticmethod
    def resolve_hostname(hostname: str) -> str | None:
        try:
            return socket.gethostbyname(hostname)
        except socket.gaierror:
            return None


@dataclass(frozen=True)
class OpenOCDCommandLineParameter:
    class Type(enum.Enum):
        COMMAND = "-c"
        FILE = "-f"

    parameter_type: Type
    value: str

    def to_str_args(self) -> Iterable[str]:
        return self.parameter_type.value, self.value

    @staticmethod
    def to_args(parameters: Iterable["OpenOCDCommandLineParameter"]) -> Iterable[str]:
        return itertools.chain.from_iterable(p.to_str_args() for p in parameters)


@dataclass(frozen=True)
class OpenOCDTarget:
    name: str
    config_file: str
    config_command: str

    def to_args(self) -> list[str]:
        params = [
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.FILE,
                self.config_file,
            ),
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.COMMAND,
                self.config_command,
            ),
        ]
        return list(OpenOCDCommandLineParameter.to_args(params))


__STM32U5X = OpenOCDTarget(
    "stm32u5",
    "scripts/debug/stm32u5x.cfg",
    "stm32u5x.cpu configure -rtos auto",
)

__STM32WBX = OpenOCDTarget(
    "stm32wb",
    "scripts/debug/stm32wbx.cfg",
    "stm32wbx.cpu configure -rtos auto",
)

__SI917 = OpenOCDTarget(
    "si917",
    "scripts/debug/siw917.cfg",
    "siw917.cpu configure -rtos auto",
)


TARGETS = dict((t.name, t) for t in (__STM32U5X, __STM32WBX, __SI917))


class BaseInterface(ABC):
    COMMAND_NAME = 'COMMAND_NAME'

    def __init__(self, name):
        self.name = name
        self._serial = None
        self.supported_targets = (None, None)

    @abstractmethod
    def base_args(self) -> list[str]:
        pass

    @abstractmethod
    def initial_args(self) -> list[str]:
        pass

    @abstractmethod
    def connection_args(self) -> list[str]:
        pass

    def is_target_supported(self, target):
        return target in self.supported_targets

    def init_serial(self, serial):
        self._serial = serial

    def set_serial(self, serial):
        self._serial = serial


class OpenOCDInterface(BaseInterface):
    COMMAND_NAME = 'openocd'

    def __init__(self, name, config_file, serial_command, transport_mode):
        super().__init__(name)
        self.config_file = config_file
        self.serial_command = serial_command
        self.transport_mode = transport_mode
        self.supported_targets = TARGETS.values()

    def base_args(self) -> list[str]:
        params = [
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.FILE,
                self.config_file,
            ),
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.COMMAND,
                f"transport select {self.transport_mode}",
            ),
        ]
        if self._serial:
            params.append(
                OpenOCDCommandLineParameter(
                    OpenOCDCommandLineParameter.Type.COMMAND,
                    f"{self.serial_command} {self._serial}",
                )
            )
        return list(OpenOCDCommandLineParameter.to_args(params))

    def initial_args(self) -> list[str]:
        params = [OpenOCDCommandLineParameter(
            OpenOCDCommandLineParameter.Type.COMMAND,
            "init",
        ),
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.COMMAND,
                "exit",
            ), ]

        return list(OpenOCDCommandLineParameter.to_args(params))

    def connection_args(self) -> list[str]:
        connect_args = [
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.COMMAND,
                param,
            ) for param in (
                "gdb_port pipe",
                # f"log_output {self._debug_root / 'openocd.log'}",  # TODO: add debug support
                "telnet_port disabled",
                "tcl_port disabled",
                "init",
            )
        ]
        return list(OpenOCDCommandLineParameter.to_args(connect_args))


class BlackmagicInterface(BaseInterface):

    COMMAND_NAME = 'arm-none-eabi-gdb'

    def __init__(self, name):
        super().__init__(name)

    def base_args(self) -> list[str]:
        gdb_launch_params = []
        self._add_command(gdb_launch_params, "set pagination off")
        self._add_command(gdb_launch_params, "set confirm off")
        self._add_command(gdb_launch_params, "monitor swdp_scan")
        return gdb_launch_params

    def initial_args(self) -> list[str]:
        args = []
        self._add_command(args, "quit")
        return args

    def connection_args(self) -> list[str]:
        base_args = self.base_args()
        self._add_command(base_args, "attach 1")
        self._add_command(base_args, "set mem inaccessible-by-default off")
        # self._add_command(base_args, "load")  # TODO: add load support
        # self._add_command(base_args, "quit")
        # base_args.append('/Users/gleb/PycharmProjects/flipperzero-firmware/build/f7-firmware-D/firmware.elf')
        return base_args

    def is_target_supported(self, target):
        return False

    def init_serial(self, serial: str) -> None:
        if SocketService.valid_ip(serial):
            self._serial = f"{serial}:2345"
        elif ip := SocketService.resolve_hostname(serial):
            self._serial = f"{ip}:2345"
        else:
            self._serial = serial

    def _add_command(self, params: list[str], command: str) -> None:
        params.append("-ex")
        params.append(command)


_BLACKMAGIC_USB = BlackmagicInterface(
    'blackmagic_usb',
)
_BLACKMAGIC_WIFI = BlackmagicInterface(
    'blackmagic_wifi',
)
__DAPLINK = OpenOCDInterface(
    "daplink",
    "interface/cmsis-dap.cfg",
    "adapter serial",
    "swd",
)

__STLINK = OpenOCDInterface(
    "stlink",
    "interface/stlink.cfg",
    "adapter serial",
    "hla_swd",
)

INTERFACES = dict((i.name, i) for i in (__DAPLINK, __STLINK, _BLACKMAGIC_USB, _BLACKMAGIC_WIFI))


class BaseAdapter(ABC):

    def __init__(self, interface: BaseInterface, target: OpenOCDTarget, serial: str):
        self.interface = interface
        self.serial = serial
        self.target = target

        if self.serial is not None:
            self.interface.init_serial(self.serial)

    def __repr__(self) -> str:
        return f"<{self.target.name} via {self.interface.name} {self.serial if self.serial else ''}>"

    def to_args(self) -> list[str]:
        args = [self.interface.COMMAND_NAME]
        args.extend(self.interface.base_args())
        if self.interface.is_target_supported(target=self.target):
            args.extend(self.target.to_args())

        return args

    def to_probe_args(self) -> list[str]:
        return self.to_args() + self.interface.initial_args()

    def execute_with_args(self, args: list[str]) -> bool:
        try:
            process = subprocess.run(
                args,
                check=False,
                capture_output=True,
                timeout=2,
            )
            logger.debug("Return code %d", process.returncode)
            logger.debug(process.stdout)
            if process.returncode == 0:
                return True

        except subprocess.TimeoutExpired:
            logger.debug("Timeout")
        except subprocess.CalledProcessError:
            logger.debug("Error")

        return False

    @abstractmethod
    def to_connection_args(self) -> list[str]:
        pass

    @abstractmethod
    def probe(self) -> bool:
        pass


class OpenOCDAdapter(BaseAdapter):

    def probe(self) -> bool:
        return self.execute_with_args(args=self.to_probe_args())

    def to_connection_args(self):
        args = self.to_args() + self.interface.connection_args()
        return ['-ex', f"target extended-remote | {shlex.join(args)}"]


class BaseBlackmagicAdapter(BaseAdapter):

    @abstractmethod
    def _port_resolver(self, serial: str) -> str | None:
        pass

    def to_connection_args(self) -> list[str]:
        return ['-ex', f'target extended-remote {self.interface._serial}'] + self.interface.connection_args()

    def probe(self) -> bool:
        if not (port := self._port_resolver(self.serial)):
            return False

        self.interface.set_serial(port)
        return self.execute_with_args(args=self.to_probe_args())


class BlackmagicUSBAdapter(BaseBlackmagicAdapter):

    def _port_resolver(self, serial: str) -> str | None:
        if serial and os.name == "nt":
            if not serial.startswith("\\\\.\\"):
                serial = f"\\\\.\\{serial}"

        # idk why, but python thinks that list_ports.grep returns tuple[str, str, str]
        ports: list[ListPortInfo] = list(list_ports.grep("blackmagic"))  # type: ignore

        if len(ports) == 0:
            return None
        elif len(ports) > 2:
            if serial:
                ports = list(
                    filter(
                        lambda p: p.serial_number == serial
                                  or p.name == serial
                                  or p.device == serial,
                        ports,
                    )
                )
                if len(ports) == 0:
                    return None

            if len(ports) > 2:
                raise Exception("More than one Blackmagic probe found")

        # If you're getting any issues with auto lookup, uncomment this
        # print("\n".join([f"{p.device} {vars(p)}" for p in ports]))
        port = sorted(ports, key=lambda p: f"{p.location}_{p.name}")[0]

        if serial:
            if (
                serial != port.serial_number
                and serial != port.name
                and serial != port.device
            ):
                return None

        if os.name == "nt":
            port.device = f"\\\\.\\{port.device}"
        return port.device


class BlackmagicNetAdapter(BaseBlackmagicAdapter):

    def _port_resolver(self, serial: str) -> str | None:
        if not serial or serial == "auto":
            serial = "blackmagic.local"

        # remove the tcp: prefix if it's there
        if serial.startswith("tcp:"):
            serial = serial[4:]

        # remove the port if it's there
        if ":" in serial:
            serial = serial.split(":")[0]

        if not (probe := SocketService.resolve_hostname(serial)):
            return None

        return f"tcp:{probe}:2345"



def discover_probes(
    target: OpenOCDTarget, serial_hint: Optional[str] = None
) -> list[BaseAdapter]:
    interface_adapters = {
        __DAPLINK: OpenOCDAdapter,
        __STLINK: OpenOCDAdapter,
        _BLACKMAGIC_USB: BlackmagicUSBAdapter,
        _BLACKMAGIC_WIFI: BlackmagicNetAdapter,
    }
    adapters = []

    for interface in INTERFACES.values():
        logger.debug(f"Checking {interface.name}, sn {serial_hint}")
        adapter_class = interface_adapters.get(interface)
        adapter_to_check = adapter_class(interface, target, serial_hint)

        if adapter_to_check.probe() is True:
            adapters.append(adapter_to_check)

    return adapters


def main():
    for target in (
        # STM32WBX,
        __STM32U5X,
    ):

        bs = discover_probes(target)
        print(bs)
    # bs = discover_probes(STM32U5X)
    # print(bs)


if __name__ == "__main__":
    main()
