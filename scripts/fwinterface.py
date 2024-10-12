import enum
import itertools
import subprocess
from dataclasses import dataclass
from typing import Iterable, Optional

from logging import getLogger

logger = getLogger(__name__)


@dataclass(frozen=True)
class OpenOCDCommandLineParameter:
    class Type(enum.Enum):
        COMMAND = "-c"
        FILE = "-f"

    parameter_type: Type
    value: str

    def to_str_args(self) -> Iterable[str]:
        return (self.parameter_type.value, self.value)

    @staticmethod
    def to_args(parameters: Iterable["OpenOCDCommandLineParameter"]) -> Iterable[str]:
        return itertools.chain.from_iterable(p.to_str_args() for p in parameters)


@dataclass(frozen=True)
class OpenOCDInterface:
    name: str
    config_file: str
    serial_command: str
    transport_mode: str

    def to_openocd_args(
        self, serial: Optional[str] = None
    ) -> Iterable[OpenOCDCommandLineParameter]:
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
        if serial:
            params.append(
                OpenOCDCommandLineParameter(
                    OpenOCDCommandLineParameter.Type.COMMAND,
                    f"{self.serial_command} {serial}",
                )
            )
        return params


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

INTERFACES = dict((i.name, i) for i in (__DAPLINK, __STLINK))


@dataclass(frozen=True)
class OpenOCDTarget:
    name: str
    config_file: str
    config_command: str

    def to_openocd_args(self) -> Iterable[OpenOCDCommandLineParameter]:
        return [
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.FILE,
                self.config_file,
            ),
            OpenOCDCommandLineParameter(
                OpenOCDCommandLineParameter.Type.COMMAND,
                self.config_command,
            ),
        ]


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


@dataclass(frozen=True)
class BoardConnection:
    interface: OpenOCDInterface
    target: OpenOCDTarget
    serial: Optional[str] = None

    def __repr__(self) -> str:
        return f"<{self.target.name} via {self.interface.name} {self.serial if self.serial else ''}>"

    def to_openocd_args(self) -> Iterable[OpenOCDCommandLineParameter]:
        return itertools.chain(
            self.interface.to_openocd_args(self.serial),
            self.target.to_openocd_args(),
        )


def discover_probes(
    target: OpenOCDTarget, serial_hint: Optional[str] = None
) -> Iterable[BoardConnection]:
    boards: list[BoardConnection] = []
    for interface in INTERFACES.values():
        logger.debug(f"Checking {interface.name}, sn {serial_hint}")
        board_to_check = BoardConnection(interface, target, serial_hint)
        params = list(board_to_check.to_openocd_args())
        params.extend(
            (
                OpenOCDCommandLineParameter(
                    OpenOCDCommandLineParameter.Type.COMMAND,
                    "init",
                ),
                OpenOCDCommandLineParameter(
                    OpenOCDCommandLineParameter.Type.COMMAND,
                    "exit",
                ),
            )
        )

        logger.debug(list(OpenOCDCommandLineParameter.to_args(params)))

        try:
            process = subprocess.run(
                ["openocd", *OpenOCDCommandLineParameter.to_args(params)],
                check=False,
                capture_output=True,
                timeout=2,
            )
            logger.debug("Return code %d", process.returncode)
            logger.debug(process.stdout)
            if process.returncode == 0:
                boards.append(BoardConnection(interface, target, serial_hint))
        except subprocess.TimeoutExpired:
            logger.debug("Timeout")
        except subprocess.CalledProcessError:
            logger.debug("Error")

    return boards


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
