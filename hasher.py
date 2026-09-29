#!/usr/bin/env python3
"""djb2 (seed 5381) ANSI hash — matches HashStringA in src/rLdr/includes/nt.h."""

import sys


def hash_string_a(s: str) -> int:
    h = 5381
    for ch in s:
        b = ord(ch) & 0xFF
        h = ((h << 5) + h + b) & 0xFFFFFFFF
    return h


def main() -> int:
    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <string> [...]", file=sys.stderr)
        return 1
    for arg in sys.argv[1:]:
        print(f"0x{hash_string_a(arg):08x}  {arg}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
