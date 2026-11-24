# MASHINA FM1 — Physical storage acceptance gate (experimental)

**Branch:** `presets-library` only. **No experimental physical writes are authorized by this document.** The source-level layout and host/emulator tests are evidence of implementation correctness, not proof that a stock FM1's bootloader and update/recovery tools respect the proposed reservations.

## Clean-slate requirement and first physical observations

The final experimental firmware is a fresh start: **do not migrate old FUN projects, USER presets or settings**. Eight projects × A–D start empty, USER 001–032 start empty, and LIBRARY 033–064 start with 32 selected compiled factory fallback sounds. The original 107-preset catalog and older user/emulator backup files stay safely offline. See [clean-slate first-install plan](clean-slate-first-install.md).

The November 24, 2026 physical read-only diagnostic reported 16 legacy FUN project banks and **four unknown LIBRARY banks**. All 40 sectors were CRC32-fingerprinted. The eight library CRCs repeat after 16 KiB and none equals the CRC32 of an erased 4 KiB NOR sector. This does not prove an address alias or identify the storage owner. Unknown LIBRARY data is an absolute stop for physical writes **even though old app data may be discarded**.

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
3. The future **new-format** device has a fully validated native-to-native backup, restore and update path. The original firmware's data is intentionally disposable at first install; old FUN-to-MPR1 migration is not required.
4. The FM1 can complete the first write/erase to proposed experimental sectors without unexpected side effects. No real-hardware erase/program test has been conducted.
5. CRC32 equality is not a cryptographic proof of data identity and does not establish authorization to use a region.

## Required evidence before any **write-enabled** firmware

**A. Existing-device recovery.** The owner has confirmed a full offline physical backup and explicitly accepts a clean slate for old USER presets, FUN project sections and settings. Keep the backup and a working firmware recovery method available to recover from a failed install; there is no requirement to migrate legacy records. Never overwrite original emulator exportA/exportB archives or the protected 107-preset catalog.

**B. Explicit decision about a diagnostic reflash.** The diagnostic image is an **installer**: even though its new probe commands do not program flash, installing it still rewrites the application/loader. Once the diagnostic application is running, its HAL blocks all persistent USER/project/settings/autosave writes, while OTA staging remains writable for reinstalling stable firmware; stock bootloader or recovery operations are outside that HAL gate. No ChatGPT action is allowed to flash the FM1 autonomously. User approval and a verified backup are prerequisites even for diagnostic installation.

**C. Read-only physical evidence.** If diagnostic installation is accepted, on only **one** FM1, run the Web Library's *Inspect flash (read-only)*. Retain its `mashina-flash-diagnostic.json`, now including status classifications, candidate addresses and optional 40-sector CRC fingerprints. Share the report for independent review. **Unknown, unreadable, mixed or corrupt banks are an absolute stop**, not blank/free space. A valid retired FUN project is expected and can be deliberately replaced by a separately authorized fresh-format operation; **do not** automatically format it.

**D. Upgrade preservation evidence.** A second complete physical diagnostic from the same FM1 can be compared offline to the first. **No difference** across the 40 sector CRCs is useful evidence of preservation *for that tested update path only*. Differences must be investigated before any further storage work. This comparison neither enables writes nor automatically certifies the vendor/stock updater.

**E. First experimental device write.** Only after the above, and an explicit new user decision, prepare a distinct build with experimental writes deliberately enabled. Never initialize/format a region automatically at boot. Take a fresh target backup and verify its exact offline copy before any operation. First test a minimal isolated LIBRARY write with readback, confirm USER/FM6/project/settings preservation, and keep the old app installable. Do not expand from one FM1 to the second until recovery and persistence across normal reboots/upgrades are demonstrated.

## Stop/go decision

The Web Library's flash-readiness assessment intentionally always returns:

`PHYSICAL_WRITES_BLOCKED`

Even an entirely blank flash probe has **no authority** to change this. Hardware write permission is a separate explicit engineering and user decision backed by the physical evidence above. Never infer a green light from a successful emulator, headless browser, host-NOR test, package compilation, or diagnostic-only CRC comparison.
