#!/usr/bin/env python3
"""Check the linked experimental FM1 app against the real JieLi app-slot limit.

No hardware access. The padded firmware package is NOT the application size.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
def main():
    image = ROOT / (sys.argv[1] if len(sys.argv) > 1 else "build/felucca.bin")
    reserve = int(sys.argv[2]) if len(sys.argv) > 2 else 32768
    packer = (ROOT / "tools/fm1pkg_make.py").read_text()
    match = re.search(r"^APP_SLOT\s*=\s*(0x[0-9a-f]+)", packer, re.M | re.I)
    if not match:
        raise SystemExit("Packager application slot is unknown")
    size = int(match.group(1), 16)
    linker = (ROOT / "firmware/app.ld").read_text()
    if not re.search(r"XIP\s*\(rx\)\s*:\s*ORIGIN\s*=\s*0x[0-9a-f]+,\s*LENGTH\s*=\s*"
                     + re.escape(hex(size)), linker, re.I):
        raise SystemExit("Linker XIP app-slot disagrees with the packager")
    used = image.stat().st_size
    remaining = size - used
    print(f"APP FLASH: {used:,} / {size:,} bytes used, {remaining:,} bytes "
          f"({remaining / 1024:.1f} KiB) free; engineering reserve {reserve:,} bytes")
    if remaining < reserve:
        raise SystemExit("FAIL: Not enough application flash headroom for physical experiment")
    print("PASS: application flash reserve intact")

if __name__ == "__main__":
    main()
