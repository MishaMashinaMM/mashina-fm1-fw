# MASHINA FM1 — Physical storage acceptance gate (experimental)

**Branch:** `presets-library` only. **No experimental physical writes are authorized by this document.** The source-level layout and host/emulator tests are evidence of implementation correctness, not proof that a stock FM1's bootloader and update/recovery tools respect the proposed reservations.

## What is verified without a physical FM1

- `MPR1` project records occupy 16 paired A/B flash banks, `0xA0000..0xBFFFF` (128 KiB), with 8 projects and four A–D sections. Each saved project references presets rather than duplicating sound parameters.
- `LIB1` contains 32 full 304-byte preset records in four A/B banks, `0xEA000..0xF1FFF` (32 KiB), and compiled factory fallbacks. USER bank and separate FM6/drum companions retain independent storage.
- Host NOR tests exercise CRC validation, sector bounds, interrupted saves, damaged bank handling, full six-track project loading, and complete sound/kit records.
- The branch's MASHINA OTA loader explicitly excludes the proposed MPR1 and LIB1 ranges from its update-record cleanup. A simulated real-package upgrade now checks **all eight LIBRARY sectors**, including forged stock updater signatures, as well as the remainder of the data-flash window.
- The experimental Web Library's exported emulator NOR JSON was physically-independent-browser tested and loaded/played by the user in the MPR1/LIB32 browser emulator. Shared preset changes were reflected in projects without losing project-specific settings.
- A diagnostic-only, **read-only** protocol can classify 16 project and four library banks and fingerprint every one of the **40 underlying 4 KiB sectors** by CRC32 (5 WebMIDI requests × 8 sectors). An offline browser comparison reports any differences between two captures. It transmits checksums, not raw preset bytes.

## What is **not** established

1. The specific physical FM1's entire `0xEA000..0xF1FFF` USR region is safely owned by MASHINA. The packager's `USR` descriptor alone is *not* proof that every stock firmware, rescue procedure or bootloader will preserve it.
2. A stock installer, factory reset, vendor recovery and all future flash paths preserve both MPR1/LIB1 regions.
3. The user's current physical device has a complete, verified **native-to-native backup and restore** path for its existing firmware. The experimental MPR1 full-device backup cannot be assumed compatible with pre-MPR1 FUN project storage.
4. The FM1 can complete the first write/erase to proposed experimental sectors without unexpected side effects. No real-hardware erase/program test has been conducted.
5. CRC32 equality is not a cryptographic proof of data identity and does not establish authorization to use a region.

## Required evidence before any **write-enabled** firmware

**A. Existing-device recovery.** Back up all currently stored USER sound banks, FM6/drum companions, all existing project sections, current performance and global settings using the editor appropriate for the *currently installed* stable firmware. Keep the backup offline, check that the file opens and has the expected records, and retain a working stable firmware/recovery method. Never overwrite the original emulator exportA/exportB archives or the protected 107-preset catalog.

**B. Explicit decision about a diagnostic reflash.** The diagnostic image is an **installer**: even though its new probe commands do not program flash, installing it still rewrites the application/loader and the existing stable firmware's normal save/autosave paths can still write. No ChatGPT action is allowed to flash the FM1 autonomously. User approval and a verified backup are prerequisites even for diagnostic installation.

**C. Read-only physical evidence.** If diagnostic installation is accepted, on only **one** FM1, run the Web Library's *Inspect flash (read-only)*. Retain its `mashina-flash-diagnostic.json`, now including status classifications, candidate addresses and optional 40-sector CRC fingerprints. Share the report for independent review. **Unknown, unreadable, mixed or corrupt banks are an absolute stop**, not blank/free space. A valid retired FUN project means old project records must be backed up and a deliberate migration/formatting plan agreed upon.

**D. Upgrade preservation evidence.** A second complete physical diagnostic from the same FM1 can be compared offline to the first. **No difference** across the 40 sector CRCs is useful evidence of preservation *for that tested update path only*. Differences must be investigated before any further storage work. This comparison neither enables writes nor automatically certifies the vendor/stock updater.

**E. First experimental device write.** Only after the above, and an explicit new user decision, prepare a distinct build with experimental writes deliberately enabled. Never initialize/format a region automatically at boot. Take a fresh target backup and verify its exact offline copy before any operation. First test a minimal isolated LIBRARY write with readback, confirm USER/FM6/project/settings preservation, and keep the old app installable. Do not expand from one FM1 to the second until recovery and persistence across normal reboots/upgrades are demonstrated.

## Stop/go decision

The Web Library's flash-readiness assessment intentionally always returns:

`PHYSICAL_WRITES_BLOCKED`

Even an entirely blank flash probe has **no authority** to change this. Hardware write permission is a separate explicit engineering and user decision backed by the physical evidence above. Never infer a green light from a successful emulator, headless browser, host-NOR test, package compilation, or diagnostic-only CRC comparison.
