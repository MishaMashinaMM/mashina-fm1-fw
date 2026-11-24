# MASHINA FM1 — clean-slate first installation (experimental specification)

**Decision (2026-11-24):** This firmware is a **new, incompatible installation**, not a migration from the current main-branch firmware. The owner's old USER presets, old project sections and old settings do **not** need to survive the intentional first-install reset. Existing offline backup files remain outside the device and are not part of the initial device state.

## Desired device state

- 32 USER positions, 001–032: logically empty and editable.
- 32 LIBRARY positions, 033–064: 32 deliberately selected compiled factory fallbacks, audible immediately; later replaceable from Web Library.
- Eight projects, each with sections A, B, C, D: initially empty; new MPR1 records reference shared USER/LIBRARY slots.
- Project settings: ordinary defaults, no FUN import or compatibility layer.
- 107-preset master catalog and protected user-created emulator JSON backups: remain immutable external sources; not all 107 are simultaneously stored in the FM1.

## Implementation status: 40-sector clean-layout preflight (24 November 2026)

An additional **compile-disabled-by-default** routine now exists in `project_refs_init.c` behind the independent `FELUCCA_CLEAN_INSTALL_LAYOUT=1` flag (also requires the MPR1 project, LIBRARY, and one-time formatter write gates).

It inspects **all 40 sector copies (160 KiB)** in `0xA0000..0xC7FFF` before the first destructive operation. A sector is accepted only if entirely erased or a valid, address-matched, fully CRC-verified retired FUN project record (including its extension when present). An unknown, damaged, unreadable, or new-format sector blocks **all** erasures. If the whole set passes, the routine erases the 32 MPR1 candidate sectors and 8 LIB1 candidate sectors, verifying each blank sector afterward. A verified interrupted erasure can be restarted while all sectors are old FUN or erased. No LIB1 record is programmed: 32 factory sounds are already compiled into firmware and appear as fallbacks after a genuinely blank LIB1 bank.

**This is not yet a complete device reset or user-facing installer.** It does not erase the four USER preset sectors, FM6/drum companion sectors, the old first-project and remaining legacy sections, song index, settings, autosave, or FM6 voice bank. A fresh 32-empty-USER-slot experience additionally requires an independently verified reset of those stores and RAM state. In the combined-layout prototype, the old project-only SysEx initializer is explicitly disabled to avoid a partially clean installation. The entire combined formatter is **build-only** and is never included in the downloadable read-only diagnostic.

The next engineering gate is a complete source-owned storage-partition inventory and restore/recovery validation. In particular, **do not automatically erase** the old `0xEA000..0xF1FFF` USR candidate, the FM6 voice-bank staging region, or any unverified bootloader/OTA scratch area.

## First-install ordering, after independent hardware approval

1. Reclaim eight old FUN project sectors at **0xC0000–0xC7FFF** for LIB1, rather than using the unknown **0xEA000–0xF1FFF USR** area. Verify that the exact physical sectors contain retired FUN records or are erased and that the loader and other stores cannot write them unexpectedly. The proposed physical reallocation is **not yet tested**.
2. Independently test the exact updater/recovery path on a single FM1 with an offline backup, and explicitly approve a destructive clean install. The app-level HAL read-only diagnostic does not govern stock SPL or bootloader operations.
3. Make the first-install formatting operation explicit and one-shot, never automatic on boot, and restrict writes to verified exact partitions. Do not erase all of USR or all of NOR.
4. Initialize the new MPR1 paired project-bank sectors (current candidates 0xA0000–0xBFFFF); deliberately erase legacy project records as part of the fresh start, only after the storage map is approved.
5. Explicitly initialize the NEW LIB1 paired-bank sectors only if independent physical evidence authorizes them (new candidates 0xC0000–0xC7FFF). Both sectors of a brand-new bank must be entirely erased before the current library code will allow a compiled factory fallback. A sector with an unrecognized header or payload is **not** an empty LIBRARY bank.
6. Independently initialize USER slots, their FM6/drum companions, settings and reference-only autosave; do not treat prior-format data as current records. Never erase loader, recovery or unknown storage as a side effect.
7. Validate all 32 factory fallback sounds and 32 empty USER slots on-device, all 8 × A–D project sections, section references and project-owned settings. Verify normal reboot persistence, a safe WebMIDI replacement of one LIBRARY sound, recovery from an interrupted write, and backup/restore under the new format.
8. Validate a subsequent normal MASHINA upgrade while preserving the **new** firmware's user-created records. The old firmware's data need not survive the initial installation, but subsequent upgrades must preserve the new format.

## What the first physical read-only probe showed

Source: `mashina-flash-diagnostic.json` captured 2026-11-24 08:19:12Z on one FM1.

- All 40 4-KiB sector CRC32s were readable; this is fingerprint-only evidence, not raw-flash inspection.
- All 16 proposed MPR1 paired banks report valid retired FUN project records. No FUN-to-MPR1 migration is required by product design.
- All four **old USR LIB1 candidates** reported unknown data. The eight CRC32s at 0xEA000–0xF1FFF show a repeated 16-KiB sequence (sector 0 equals 4, 1 equals 5, 2 equals 6, 3 equals 7).
- None of the eight library-sector CRC32s equals F154670A (the CRC32 of 4096 bytes of 0xFF).
- **Interpretation:** the **old USR** candidate sectors are occupied or otherwise inconsistent with an erased bank; repeated CRCs cannot distinguish repeated content, address aliasing or a read-path error. Do not wipe these sectors.
- **New candidate `0xC0000–0xC7FFF`:** present in source-level legacy FUN project allocations and not inspected by the old physical CRC diagnostic. A fresh read-only test is still needed before authorizing any writes.

## Current physical gate

`PHYSICAL_WRITES_BLOCKED`. No web UI unlock, installation, automatic format, project save, or experimental physical LIBRARY write is authorized. Keep the compiled factory fallbacks in firmware; do not depend on physical LIBRARY data being initialized until storage ownership is established.

This document is an engineering plan, **not** a runnable installer or hardware permission.
