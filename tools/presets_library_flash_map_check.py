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
    ref_store = "firmware/src/project_refs_store.c"
    projects = (macro(ref_store, "MPRS_BASE"), macro(ref_store, "MPRS_BASE") + 16 * 2 * sector)
    assert projects == (0xA0000, 0xC0000), f"unexpected new project region: {projects}"
    assert separated(projects, library), "compact project and LIB32 banks overlap"
    assert projects[0] % sector == 0 and projects[1] % sector == 0
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
        if name == "existing USER/project store":
            assert region[0] <= projects[0] and projects[1] <= region[1], "new projects outside original data region"
            print("CAUTION: compact projects REPLACE sectors of old project storage; fresh initialization required")
        else:
            assert separated(projects, region), f"MPR1 projects overlap {name}: {region}"
    assert projects[1] <= 0xDC000, "MPR1 projects must stay below USER preset storage"
    assert projects[0] >= 0xA0000, "MPR1 projects must not overlap legacy first 4 sections"
    # The loader's update-record sweep can scan this range. It erases only
    # sectors whose trailing 80 bytes validate as stock update records.
    # Thus we cannot claim collision-free operation merely from range checks.
    rec = (macro(loader, "LDR_REC_LO"), macro(loader, "LDR_REC_HI"))
    assert not separated(library, rec)
    assert "ota_rd16(r + 6) == 0x5441u" in src(loader), "record signature guard changed"
    assert "ota_crc16(r + 2, 78, 0)" in src(loader), "record CRC guard changed"
    assert "s - 0x1000u >= 0xEA000u" in src(loader) and "s - 0x1000u < 0xF2000u" in src(loader), (
        "MASHINA update loader no longer excludes LIBRARY flash sectors from cleanup")
    # A successful build is NOT proof opt-in code was actually compiled:
    # every feature flag must be forwarded through the native AC79 builder.
    build = src("tools/build.py")
    for flag in ("FELUCCA_LIB32", "FELUCCA_LIB32_WRITE",
                 "FELUCCA_PROJECT_REFS", "FELUCCA_PROJECT_REFS_WRITE",
                 "FELUCCA_PROJECT_REFS_FORMAT"):
        assert '"' + flag + '"' in build, f"tools/build.py no longer forwards {flag}"
    hal = src(flash)
    assert "#if defined(FELUCCA_LIB32_WRITE) && FELUCCA_LIB32_WRITE" in hal
    assert "FL_LIB32_OK(off,n)" in hal and "|| FL_LIB32_OK(off,n)" in hal, (
        "LIB32 writes must be bounded by an explicit flash HAL gate")
    # The opt-in physical diagnostic MUST be read-only for every persistent
    # sound/project/settings sector, not just MPR1 and LIB32. The only
    # allowed write range is OTA loader staging so stable firmware remains
    # reinstallable. Both erase and program go through FL_RANGE_OK.
    assert re.search(
        r"#ifndef FL_RANGE_OK\s*#if defined\(FELUCCA_FLASH_DIAG\) && FELUCCA_FLASH_DIAG\s*"
        r"#define FL_RANGE_OK\(off, n\) FL_IN\(off, n, FL_OTA_LO, FL_OTA_HI\)",
        hal), "Diagnostic firmware HAL must deny all data-flash writes"
    assert "if (!FL_RANGE_OK(off, 0x1000u)" in hal and \
           "if (!FL_RANGE_OK(off, n)" in hal, (
        "Both 4KiB erase and page program must enforce the diagnostic write gate")
    print("PASS: diagnostic app HAL blocks USER/MPR1/LIBRARY/globals/autosave writes; only OTA staging remains writable")
    fw = src("firmware/src/felucca.c")
    for flag in ("FELUCCA_PROJECT_REFS", "FELUCCA_PROJECT_REFS_WRITE",
                 "FELUCCA_PROJECT_REFS_FORMAT"):
        assert "#define " + flag + " 0" in fw, f"unsafe default for {flag}"
    print("PASS: feature flags reach native compiler; hardware writes remain gated OFF by default")
    print("PASS: MASHINA loader explicitly skips LIBRARY flash sectors while clearing update records")
    print("CAUTION: stock bootloader/vendor update tools are still unverified on physical hardware")
    print("PASS: source-level LIB32 + compact MPR1 geometry and known app partitions")
if __name__ == "__main__":
    check()
