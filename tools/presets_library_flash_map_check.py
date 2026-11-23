#!/usr/bin/env python3
"""Source-level LIB32 flash map regression gate (NOT proof of vendor bootloader ownership)."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
def src(path):
    return (ROOT / path).read_text(encoding="utf-8")

def macro(path, name):
    match = re.search(r"^#define\s+" + re.escape(name) + r"\s+(0x[0-9A-Fa-f]+)u?\b", src(path), re.M)
    if not match:
        raise AssertionError(f"missing or changed numeric macro {name} in {path}")
    return int(match.group(1), 16)

def separated(a, b):
    return a[1] <= b[0] or b[1] <= a[0]

def check():
    store = "firmware/src/preset_library_store.c"
    flash = "firmware/hal/fm1_flash.h"
    loader = "firmware/loader/ldr_core.c"
    ota = "firmware/src/ota.c"
    source = src(store)
    start = macro(store, "LIB_BASE")
    sector = 4096
    # These layout constants are deliberately checked exactly; if changed,
    # review the reservation rather than accepting an arbitrary new address.
    assert "LIB_BANKS = 4" in source and "LIB_SECTOR = 4096" in source
    library = (start, start + 4 * 2 * sector)
    assert library == (0xEA000, 0xF2000), f"unexpected library region: {library}"
    regions = {
        "firmware app": (macro(loader, "LDR_APP_LO"), macro(loader, "LDR_APP_HI")),
        "existing USER/project store": (macro(flash, "FL_DATA_LO"), macro(flash, "FL_DATA_HI")),
        "OTA staging": (macro(flash, "FL_OTA_LO"), macro(flash, "FL_OTA_HI")),
        "autosave": (macro(flash, "FL_AUTO_LO"), macro(flash, "FL_AUTO_HI")),
        "FM6 voice bank": (macro(flash, "FL_VBANK_LO"), macro(flash, "FL_VBANK_HI")),
        "global settings": (macro(flash, "FL_GLOB_LO"), macro(flash, "FL_GLOB_HI")),
    }
    assert macro(ota, "OTA_AREA") == regions["OTA staging"][0]
    assert "OTA_AREA_LEN 0x5000u" in src(ota)
    assert "s - 0x1000u >= LDR_DATA_LO" in src(loader), "loader user data exclusion changed"
    assert "s - 0x1000u < LDR_DATA_HI" in src(loader), "loader user data exclusion changed"
    for name, region in regions.items():
        assert separated(library, region), f"LIBRARY overlaps {name}: {region}"
        print(f"PASS: LIBRARY {library[0]:#x}..{library[1]-1:#x} avoids {name} {region[0]:#x}..{region[1]-1:#x}")
    # The loader's update-record sweep can scan this range. It erases only
    # sectors whose trailing 80 bytes validate as stock update records.
    # Thus we cannot claim collision-free operation merely from range checks.
    rec = (macro(loader, "LDR_REC_LO"), macro(loader, "LDR_REC_HI"))
    assert not separated(library, rec)
    assert "ota_rd16(r + 6) == 0x5441u" in src(loader), "record signature guard changed"
    assert "ota_crc16(r + 2, 78, 0)" in src(loader), "record CRC guard changed"
    print("CAUTION: loader scans LIBRARY region for possible update records (conditional erase); device verification still required.")
    print("PASS: source-level LIB32 geometry and known app partitions")
if __name__ == "__main__":
    check()
