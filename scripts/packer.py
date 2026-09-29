#!/usr/bin/env python3
"""Craft a .mod container: [64-byte header][info JSON][code].

Header layout (little-endian):
    0   4  magic    "LMOD"
    4   2  version
    6   2  flags
    8   4  info_off
    12  4  info_len
    16  4  code_off
    20  4  code_len
    24  32 sha256 over [info..end]
    56  8  reserved (zero)

usage: packer.py <module.bin> <info> <args> [output]
"""

import hashlib
import json
import os
import struct
import sys

MAGIC = b"LMOD"
VERSION = 1
FLAGS = 0
HEADER_SIZE = 64


def main():
    if len(sys.argv) not in (4, 5):
        print(f"usage: {sys.argv[0]} <module.bin> <info> <args> [output]", file=sys.stderr)
        return 1

    bin_path, info, args = sys.argv[1], sys.argv[2], sys.argv[3]

    with open(bin_path, "rb") as f:
        code = f.read()
    if not code:
        print(f"[-] {bin_path} is empty", file=sys.stderr)
        return 1

    name = os.path.splitext(os.path.basename(bin_path))[0]
    if len(sys.argv) == 5:
        out_path = sys.argv[4]
    else:
        out_path = os.path.join(os.path.dirname(bin_path), "output", name + ".mod")
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)

    info_bytes = json.dumps(
        {"name": name, "info": info, "args": args},
        separators=(",", ":"),
    ).encode("utf-8")

    info_off = HEADER_SIZE
    info_len = len(info_bytes)
    code_off = info_off + info_len
    code_len = len(code)

    digest = hashlib.sha256(info_bytes + code).digest()

    header = struct.pack(
        "<4sHHIIII32s8s",
        MAGIC,
        VERSION,
        FLAGS,
        info_off,
        info_len,
        code_off,
        code_len,
        digest,
        b"\x00" * 8,
    )
    assert len(header) == HEADER_SIZE

    with open(out_path, "wb") as f:
        f.write(header + info_bytes + code)

    print(f"    name: {name}")
    print(f"    info: {info}")
    print(f"    args: {args}")
    print(f"    json: {info_len:,} bytes @ {info_off:#x}")
    print(f"    code: {code_len:,} bytes @ {code_off:#x}")
    print(f"    out:  {out_path} ({HEADER_SIZE + info_len + code_len:,} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
