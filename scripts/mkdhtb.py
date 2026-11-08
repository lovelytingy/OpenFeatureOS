#!/usr/bin/env python3
"""
mkdhtb.py - Prepend Spreadtrum/Unisoc DHTB header to raw binary payload.

The Unisoc BootROM/FDL1 expects downloaded images to have a 512-byte (0x200)
header beginning with the 'DHTB' magic word. The execution entry point is
located at load_addr + 0x200 (i.e. 0x80100200 when loaded at 0x80100000).

Header format (observed in FDL1/FDL2/BOOT0 and Nokia-2660-RE docs):
  Offset 0x00: 'DHTB' (0x42544844 little-endian)
  Offset 0x04: Version uint32_t (1)
  Offset 0x30: Payload size uint32_t (size of executable after 512-byte header)
  Total header size: 512 bytes (0x200), padded with zeros.
"""

import sys
import struct
import os

def create_dhtb(raw_path, out_path):
    if not os.path.exists(raw_path):
        sys.stderr.write(f"Error: input file '{raw_path}' not found\n")
        sys.exit(1)

    with open(raw_path, "rb") as f:
        payload = f.read()

    payload_size = len(payload)

    # 512-byte DHTB header
    header = bytearray(512)
    header[0:4] = b"DHTB"
    struct.pack_into("<I", header, 4, 1)              # Version 1
    struct.pack_into("<I", header, 0x30, payload_size) # Payload size at offset 0x30

    with open(out_path, "wb") as f:
        f.write(header)
        f.write(payload)

    print(f"[mkdhtb] Built '{out_path}': header=512 B, payload={payload_size} B, total={512 + payload_size} B")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.stderr.write(f"Usage: {sys.argv[0]} <input.raw> <output.bin>\n")
        sys.exit(1)

    create_dhtb(sys.argv[1], sys.argv[2])
