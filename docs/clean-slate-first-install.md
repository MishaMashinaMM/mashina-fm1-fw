# MASHINA FM1 — clean-slate first installation (experimental specification)

**Decision (2026-11-24):** This firmware is a **new, incompatible installation**, not a migration from the current main-branch firmware. The owner's old USER presets, old project sections and old settings do **not** need to survive the intentional first-install reset. Existing offline backup files remain outside the device and are not part of the initial device state.

## Desired device state

- 32 USER positions, 001–032: logically empty and editable.
- 32 LIBRARY positions, 033–064: 32 deliberately selected compiled factory fallbacks, audible immediately; later replaceable from Web Library.
- Eight projects, each with sections A, B, C, D: initially empty; new MPR1 records reference shared USER/LIBRARY slots.
- Project settings: ordinary defaults, no FUN import or compatibility layer.
- 107-preset master catalog and protected user-created emulator JSON backups: remain immutable external sources; not all 107 are simultaneously stored in the FM1.

## 53-sector physical inspection and repeated CRC32 finding

The read-only browser report now includes 53 source-owned active-state sector CRC32 fingerprints. A repeated whole-sector checksum is **not** evidence that physical flash aliases itself. In particular, `0x127B0F88` is the reproducible CRC32 for an otherwise erased 4096-byte sector containing a valid, **zero-length committed FELU record**. The record stores its own header CRC32, so different object IDs, sequence numbers and A/B slots still yield the same whole-sector checksum. A standalone host regression now constructs different, CRC-valid FUN project records and verifies that their bytes differ while their whole-sector CRC32 is identical. **This pattern alone is not proof of an empty record:** header/payload and any extension still require independent CRC and address/type validation.

Diagnostic-only WebMIDI **command 100** therefore classifies **each** of the 53 exact candidate sectors without exposing raw sound/project bytes: 0 erased, 1 already-new-format MPR1, 2 expected source-owned CRC-validated old record, 3 unknown/corrupt or 4 unreadable. This classification uses the same legacy types and sector addresses as the compile-disabled clean installer. A matching report with only status 0 or 2 is supporting evidence for the proposed content layout, **not permission to write**; stock bootloader/recovery, verified backups and an explicitly gated installation still need independent checks.

The updated Web Library can also use the existing read-only backup protocol to export and re-import a byte-matched **legacy** archive covering the older projects (8 × A–D), USER sounds and settings. It must be kept for rollback to compatible legacy firmware; those old project snapshots must never be imported as MPR1 reference records. No destructive operation has been exposed by these additions.

## Implementation status: 53-sector clean-layout preflight (24 November 2026)

An additional **compile-disabled-by-default** routine now exists in `project_refs_init.c` behind the independent `FELUCCA_CLEAN_INSTALL_LAYOUT=1` flag (also requires the MPR1 project, LIBRARY, and one-time formatter write gates).

It inspects **all 53 sector copies (212 KiB total)** before the first destructive operation: 32 MPR1 sectors in `0xA0000..0xBFFFF`, 8 LIB1 sectors in `0xC0000..0xC7FFF`, 4 USER bank sectors in `0xDC000..0xDFFFF`, 2 FM6/drum companion sectors at `0x9F000` and `0xFE000`, 2 settings sectors at `0xFC000`/`0xFD000`, 2 song-index sectors at `0xD8000`/`0xD9000`, 2 autosave sectors at `0xE5000`/`0xE6000`, and the FM6 voice-bank sector at `0xE7000`. A sector is accepted only if entirely erased or a valid, address-matched, fully CRC-verified old storage record of the expected type (including its extension when present). For the shared FM6 companion sector the retired FM6 bank format is also accepted. An unknown, damaged, unreadable, or new-format sector blocks **all** erasures. If the whole set passes, the routine erases the 32 MPR1 candidate sectors, 8 LIB1 candidate sectors, 4 USER sectors, 2 companion sectors and 7 settings/index/autosave/voice sectors, verifying each blank sector afterward. **As its final operation**, it writes one verified MPR1 bank containing two logically empty sections. This first-bank marker prevents repeating the destructive reset later, even if no projects have been saved. An interrupted final commit refuses automatic reinitialization and requires explicit recovery. A verified interrupted erasure can be restarted while all sectors are old valid records or erased, before the final MPR1 marker commit. No LIB1 record is programmed: 32 factory sounds are already compiled into firmware and appear as fallbacks after a genuinely blank LIB1 bank.

**This is not yet a complete device reset or user-facing installer.** It deliberately leaves the old first-project and remaining **unreferenced** legacy song sections untouched. New MPR1 firmware does not use those old records. After successful formatting, a **reboot** must rebuild empty USER and project RAM mirrors and load default settings, song selection and RESTORE LAST state. A physical installer remains blocked until those boot/recovery behaviors are independently verified. In the combined-layout prototype, the old project-only SysEx initializer is explicitly disabled to avoid a partially clean installation. The entire combined formatter is **build-only** and is never included in the downloadable read-only diagnostic.

The read-only diagnostic has been extended to fingerprint all **53 active-state candidate sectors**, including the six USER/companion and seven settings/index/autosave/voice sectors. Before enabling the clean-install format path on real hardware, acquire a fresh physical 53-sector report, confirm the source-owned partition map and independently validate updater/recovery behavior. A CRC32 capture by itself cannot authorize writing. In particular, **do not automatically erase** the old `0xEA000..0xF1FFF` USR candidate, the FM6 voice-bank staging region, or any unverified bootloader/OTA scratch area.

## Latest actual FM1 preflight and owner backup (24 November 2026)

An FM1 flashed with the read-only diagnostic build `e646110ab1ed` successfully auditioned all 64 sound slots and retained its original project data. **Two separately uploaded, complete 53-sector captures**, made at 11:47:30 UTC and 12:18:18 UTC on 24 November 2026 (before and after the owner's diagnostic-firmware update), were independently compared offline: **53/53 indexed sector addresses and CRC32 fingerprints are identical; 0 changed**. This is strong, device-specific evidence that the MASHINA firmware update preserved the candidate data sectors. CRC agreement does not prove anything about unwitnessed interrupted updates, stock SPL vendor recovery, or future reset writes. The newer on-device read-only inspection reports **53/53 examined sectors**: 52 exact validated old source-owned records, 1 erased FM6 voice sector at `0xE7000`, 0 unknown/corrupt and 0 unreadable. All 16 proposed MPR1 project banks and all 4 proposed LIBRARY banks contain old CRC-validated FUN records. The 14 repeated CRCs `127B0F88` are compatible with zero-length committed FELU records and **are not flash aliasing evidence**.

The owner also successfully used the browser's read-only legacy archive function: it reported **43 objects** checked by individual CRC32, then the owner imported the saved file and received a byte-for-byte match against the same connection's captured data. The actual legacy archive remains with the owner, not in this repository; without that file or a physical restore test, do **not** claim a device recovery has been demonstrated. The diagnostic JSON's conservative `audit.physicalRestoreProven=false` is therefore correct; its `cleanLayoutPreflight.allowedContent=true` authorizes no writes.

**Outstanding hardware gate:** The MASHINA owned OTA loader's application-only write range and source-level user-data exclusions now include the three autosave/voice sectors. Host tests simulate real package upgrades and forged vendor-update signatures. Nevertheless, stock SPL/vendor recovery behavior and recovering a *failed* physical first-install format have **not** been validated. A staged physical installation must still require separate user authorization and a verified, retained original archive.

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
