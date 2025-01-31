import json
from dataclasses import dataclass
from pathlib import Path
from typing import Optional


@dataclass(frozen=True)
class FbtHardwarePlatform:
    name: str
    openocd_interface_file: Optional[Path]
    openocd_init_commands: tuple[str]
    flash_address: Optional[int]
    ram_address: Optional[int]
    svd_file: Optional[Path]

    @staticmethod
    def from_dict(param_dict: dict, rel_path_root: Path) -> "FbtHardwarePlatform":
        rel_path_root = rel_path_root.resolve()
        oocd_params = param_dict.get("openocd", {})
        return FbtHardwarePlatform(
            param_dict["name"],
            (
                rel_path_root / cfg
                if (cfg := oocd_params.get("chip_config", None))
                else None
            ),
            tuple(oocd_params.get("init_commands", [])),
            flash_address=int(param_dict.get("flash_address", "0x0"), 16),
            ram_address=int(param_dict.get("ram_address", "0x0"), 16),
            svd_file=(
                rel_path_root / svd
                if (svd := param_dict.get("svd_file", None))
                else None
            ),
        )

    @staticmethod
    def from_file(hw_platform_file: str) -> "FbtHardwarePlatform":
        with open(hw_platform_file, "r") as f:
            hw_platform_data = json.load(f)

        return FbtHardwarePlatform.from_dict(
            hw_platform_data, Path(hw_platform_file).parent
        )
