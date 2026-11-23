#!/usr/bin/env python3
"""Read-only flash-capacity gate for the proposed 32 USER + 32 WebMIDI library.

This is intentionally NOT a flash writer. Fails closed: before implementing
writes, a proven, non-overlapping, resilient persistent region is required.
Run: python3 tools/presets_library_capacity.py
"""
from pathlib import Path
import math
import re

ROOT = Path(__file__).resolve().parents[1]
read = lambda p: (ROOT / p).read_text(encoding="utf-8")
up = read("firmware/src/upreset.c")
side = read("firmware/src/up_fm6.c")
storage = read("firmware/src/storage.c")
flash = read("firmware/hal/fm1_flash.h")
pack = read("tools/fm1pkg_make.py")
core = read("firmware/src/core.h")
def expect(pattern, source, what):
    m = re.search(pattern, source)
    if not m:
        raise AssertionError(f"Storage layout changed: review {what}")
    return m
assert expect(r"#define UP_SLOTS\s+32u", core, "existing user count")
assert expect(r"sizeof\(up_rec_t\) == 192", up, "USER record size")
assert expect(r"#define UP_PER_BANK\s+16u", up, "USER bank count")
assert expect(r"#define UPF_PK\s+112u", side, "FM6/drum auxiliary record")
assert expect(r"#define ST_PAYLOAD_OFF\s+256u", storage, "flash payload offset")
assert expect(r"0xDA000\.\.0xDBFFF free", storage, "documented free flash range")
assert expect(r"0xEA000, 0x12000.*\"USR\"", pack, "reserved USR region")
assert expect(r"#define FL_DATA_HI\s+0x000E0000u", flash, "permitted data upper limit")
SECTOR = 4096
PAYLOAD = 4096-256
SLOTS = 32
BASIC = 192
AUX = 112
FREE_SECTORS = 2
# Conservative power-loss resilient copy-on-write banks using current layout:
# 16 USER records/bank, two A/B copies; 32 aux records fit in one A/B pair.
basic_banks = math.ceil(SLOTS / 16)
aux_banks = math.ceil(SLOTS * AUX / PAYLOAD)
basic_sectors = basic_banks * 2
aux_sectors = aux_banks * 2
required_sectors = basic_sectors + aux_sectors
print("Proposed additional library: 32 independent load-only replaceable presets")
print(f"Full existing-format payload: 32 x ({BASIC}+{AUX}) = {SLOTS*(BASIC+AUX):,} B")
print(f"Documented free space: {FREE_SECTORS} x 4096 = {FREE_SECTORS*SECTOR:,} B")
print(f"Power-safe bank layout: {basic_sectors} basic + {aux_sectors} sidecar sectors = "
      f"{required_sectors} sectors ({required_sectors*SECTOR:,} B)")
print(f"Unproven capacity gap: {required_sectors-FREE_SECTORS} sectors "
      f"({(required_sectors-FREE_SECTORS)*SECTOR:,} B)")
print("PASS: existing USER bank untouched, existing storage definitions verified.")
integrated_sectors = math.ceil(SLOTS / 8) * 2
print(f"Integrated 304-byte sound+sidecar record, 8 records/copy: "
      f"{integrated_sectors} sectors = {integrated_sectors*SECTOR:,} B "
      "(one atomic commit updates both pieces)")
print("GATE BLOCKED: 8 KiB is insufficient for arbitrary 32 full sounds with A/B writes.")
print("Do not use 0xEA000..0xFBFFF USR: ownership and update safety unverified.")
print("No firmware flash writes are enabled by this prototype.")
