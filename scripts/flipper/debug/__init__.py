import argparse
from collections.abc import Iterable
from dataclasses import dataclass
from itertools import chain


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


###############################################################################


class BaseDebugExtension:
    @staticmethod
    def configure_arg_parser(parser: argparse.ArgumentParser):
        raise NotImplementedError

    def append_gdb_args(self, args: argparse.Namespace) -> Iterable[GdbParam]:
        raise NotImplementedError


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
