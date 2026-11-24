#!/usr/bin/env python3
"""Read-only verification of two independently captured 1 MiB FM-1 flash dumps.

Usage: python tools/verify_fm1_recovery_dumps.py first.bin second.bin
This NEVER opens a device, sends MIDI, erases or writes flash.
It cannot establish that a recovery write will work on real hardware.
"""
import argparse
import hashlib
from pathlib import Path
import sys

FLASH_BYTES = 0x100000
BOOT_END = 0x4000
APP_END = 0x93000


def verify(first: Path, second: Path):
    if first.resolve() == second.resolve():
        raise ValueError("Two distinct files from independent read sessions are required")
    images = []
    for path in (first, second):
        if not path.is_file():
            raise ValueError(f"Missing flash dump: {path}")
        blob = path.read_bytes()
        if len(blob) != FLASH_BYTES:
            raise ValueError(f"{path}: expected exactly {FLASH_BYTES} bytes, got {len(blob)}")
        if blob[:BOOT_END] in (bytes(BOOT_END), bytes([255])*BOOT_END):
            raise ValueError(f"{path}: boot head is entirely blank/erased")
        app = blob[BOOT_END:APP_END]
        if app in (bytes(len(app)), bytes([255])*len(app)):
            raise ValueError(f"{path}: application region is entirely blank/erased")
        images.append(blob)
    a, b = images
    if a != b:
        first_byte = next(i for i, (x, y) in enumerate(zip(a, b)) if x != y)
        raise ValueError(
            f"Full-flash dumps DO NOT MATCH; first difference at 0x{first_byte:05X}. "
            "Do not attempt installation or recovery."
        )
    digest = hashlib.sha256(a).hexdigest()
    return {
        "sha256": digest,
        "boot_sha256": hashlib.sha256(a[:BOOT_END]).hexdigest(),
        "app_sha256": hashlib.sha256(a[BOOT_END:APP_END]).hexdigest(),
        "data_sha256": hashlib.sha256(a[APP_END:]).hexdigest(),
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("first", type=Path, help="First full 1 MiB physical flash dump")
    parser.add_argument("second", type=Path, help="Second independent full 1 MiB flash dump")
    args = parser.parse_args(argv)
    try:
        result = verify(args.first, args.second)
    except (OSError, ValueError) as exc:
        print(f"STOP: {exc}", file=sys.stderr)
        return 2
    print("READ-ONLY CHECK PASSED: two full 1 MiB flash dumps match byte-for-byte.")
    print("Full flash SHA-256:   " + result["sha256"])
    print("Boot head SHA-256:    " + result["boot_sha256"])
    print("Application SHA-256:  " + result["app_sha256"])
    print("Device data SHA-256:  " + result["data_sha256"])
    print("Keep BOTH binary dumps privately and unchanged. This is NOT a recovery proof.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
