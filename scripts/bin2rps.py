#!/usr/bin/env python3

import os
import crc
import struct
import argparse

"""
RPS file format:
    https://www.silabs.com/documents/public/reference-manuals/siw917x-family-rm.pdf (pg. 856)
"""


class RpsFile:
    def __init__(self, address: int, source: str, dest: str):
        self.address = address
        self.source = source
        self.dest = dest
        self.data = bytearray()

        self.__add_header()
        self.__add_firmware()
        self.__add_crc()

    def save(self):
        dest_file = open(self.dest, "wb")
        dest_file.write(self.data)
        dest_file.close()

    def __add_header(self):
        # Header
        self.data += struct.pack(
            "<HHIIIII16sI16sI",
            0x01,  # flags
            0x0,  # sha_type
            0x900D900D,  # magic
            os.path.getsize(self.source) + 0xFC0,  # image_size
            0x1,  # firmware_version
            (self.address - 0x08001000) & 0xFFFFFFFF,  # flash_address
            0x0,  # crc (placeholder)
            bytearray(16),  # mic
            0x0,  # pubkey_reference
            bytearray(16),  # unknown
            0x900D900D,  # magic 2
        )

        # Boot descriptor
        self.data += struct.pack(
            "<HHI",
            0x5AA5,  # magic
            0x0,  # offset
            self.address,  # ivt_offset
        )

        # Last boot descriptor entry
        self.data += struct.pack(
            "<II",
            0x80000000,
            0x00,
        )

        # Unused boot descriptor entries
        self.data += bytearray(0x30)
        # Padding?
        self.data += bytearray(0xF80)
        pass

    def __add_firmware(self):
        source_file = open(self.source, "rb")
        self.data += source_file.read()

    def __add_crc(self):
        # Configure CRC generator
        crc_config = crc.Configuration(
            width=32,
            polynomial=0xD95EAAE5,
            init_value=0x0,
            final_xor_value=0x0,
            reverse_input=True,
            reverse_output=True,
        )

        # Calculate CRC
        crc_calculator = crc.Calculator(crc_config)
        crc_calculated = crc_calculator.checksum(self.data)

        # Set CRC
        self.data[0x14:0x18] = struct.pack("I", crc_calculated)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-a",
        dest="address",
        metavar="ADDRESS",
        default=0x08012000,
        type=lambda x: int(x, 16),
        help="Target flash memory address",
    )

    parser.add_argument(
        "-i",
        required=True,
        dest="source",
        metavar="SOURCE",
        type=str,
        help="The path to the source (.bin) file",
    )

    parser.add_argument(
        "-o",
        required=True,
        dest="dest",
        metavar="DEST",
        type=str,
        help="The path to the destination (.rps) file",
    )

    args = parser.parse_args()

    rps_file = RpsFile(args.address, args.source, args.dest)
    rps_file.save()


if __name__ == "__main__":
    main()
