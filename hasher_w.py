#!/usr/bin/env python3
"""djb2 (seed 5381) WIDE hash — matches HashStringW in src/rLdr/includes/nt.h.

Takes the low byte of each wchar_t and lowercases ASCII 'A'..'Z' (the same
case-folding HashStringW does for PEB module names).
"""

import sys


def hash_string_w(s: str) -> int:
    h = 5381
    for ch in s:
        b = ord(ch) & 0xFF
        if 0x41 <= b <= 0x5A:  # 'A'..'Z'
            b += 0x20
        h = ((h << 5) + h + b) & 0xFFFFFFFF
    return h


def main() -> int:
    if len(sys.argv) < 2:
        print(f"usage: {sys.argv[0]} <string> [...]", file=sys.stderr)
        return 1
    for arg in sys.argv[1:]:
        print(f"0x{hash_string_w(arg):08x}  {arg}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
