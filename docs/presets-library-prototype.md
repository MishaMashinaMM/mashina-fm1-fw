# Presets Library — experimental 32 USER + 32 LIBRARY

**Branch:** `presets-library`, branched from `main`. Not merged, not released, not yet tested on a physical FM1. Do **not** flash this experimental build until its full read/write and recovery tests are approved.

## Agreed workflow

- **001–032**: the existing 32 fully editable USER presets, in fixed positions even when empty, `PC 0–31`.
- **033–064**: 32 fixed LIBRARY positions, `PC 32–63`, read-only on the device but replaceable individually or as a batch from the web library. Fresh/uninitialized positions play 32 compiled factory sounds spanning the available engines; selection is still provisional, not a final curated bank.
- User may fill all 32 USER slots, capture their complete presets in the browser, save an offline **JSON backup**, then upload those sounds to the 32 LIBRARY positions **without reflashing firmware**.
- A firmware update **may erase LIBRARY data**; this is accepted by the user, since offline JSON makes restoration possible. Never assume a firmware update safely preserves USER or project data without a separate complete device backup.
- The browser sends compact 304-byte binary sound records over 7-bit-safe WebMIDI; the FM1 does **not** parse JSON.
- All six live tracks play independent sound snapshots. A library load changes only its target track; it never stages a preset in a USER slot. Eight projects × four sections (A–D) remain snapshots, not references into the library.

## Flash design and measured headroom

- 32 library records × (192-byte USER sound record + 112-byte complete FM6 or drum companion) = **9,728 bytes raw payload**.
- Four banks of eight records; two alternating CRC-protected 4,096-byte sectors per bank, header committed **last**. **32,768 bytes** flash assigned in the experiment to `0xEA000–0xF1FFF`.
- The FM1 package labels `0xEA000–0xFBFFF` as **USR** (72 KiB). The eight candidate sectors are within it and do not overlap MASHINA's existing USER (`0xDC000–0xDFFFF`), 8-project section data, settings, FM6 voice bank or OTA staging. The package/loader's *actual* erase and retention behavior on physical FM1 devices remains unverified.
- Normal firmware still **refuses** writes outside its existing flash regions. This experimental module uses its own bounded checks for the candidate 32 KiB **only when separately compiled for writing**. A bank may initialize only when **both complete 4 KiB sectors are erased (all bytes `0xFF`)**. Blank headers alone are insufficient: foreign payload bytes, damaged records or interrupted first writes cause a fail-closed storage error rather than automatic erasure. Manual recovery is required in those cases.
- On a successful library-enabled build **with writes locked**, image **509,144 / 584,336 bytes**, pool **308,432 bytes used / 11,040 spare**, main RAM **95,936 / 98,304**. This is **within** the firmware's 8 KiB pool safety headroom. These numbers were measured in CI run 45205479394.

## Clean-break project references — development in progress

The new requirement is **references only**: 8 projects, each with sections A–D,
and each section with six references to one of 64 USER/LIBRARY presets. A preset
replacement intentionally changes every section using that slot. Project-owned
mixer, routing, global FX, EQ, macros, zoning, tuning, pad-performance controls
and other section settings remain separately stored. Loading a section must not
write or overwrite any preset. No automatic migration from previous FUN*
snapshot formats is required.

The first host-only modules are:

- `firmware/src/project_refs.c`: versioned `MPR1` record with six references,
  12-byte section name, up to 1,792 bytes of project-only payload, and CRC32.
  References 0–31 select USER, 32–63 select LIBRARY, 255 means unassigned.
- `firmware/src/project_refs_store.c`: sixteen atomic paired-section banks,
  each with two alternating 4 KiB flash sectors. Maximum records are
  1,824 bytes; two occupy 3,648 bytes in a 3,840-byte available sector payload.
  Candidate allocation: `0xA0000..0xBFFFF`, **128 KiB** instead of the old
  project's **256 KiB** allocated sectors. Fail-closed initialization protects
  unknown non-erased sectors.
- Host-only regression tests in `tests/project_refs_test.c` and
  `tests/project_refs_store_test.c`.
- `web/projects-library-codec.mjs`: matching versioned project JSON encoder,
  decoder and 8 × 4 section backup structure. Round-trip, checksum and
  reference validation is covered by `tests/projects_library_codec_test.mjs`.

The first firmware-side reference resolver now also exists in the
experimental `FELUCCA_LIB32` path (`ui.c`):

- `project_reference_load(track, ref)` loads one USER 0–31 or LIBRARY 32–63
  sound into the requested track without switching the player's selected track.
- `project_references_load_all(refs)` preflights six references before any
  track changes; 255 means unassigned. Missing USER presets and damaged
  LIBRARY sectors abort the entire recall before mutating any track.
- Built-in factory fallback sounds retain their physical LIBRARY slot identity,
  so their project references remain stable before/after WebMIDI replacement.
- Factory fallback is reserved for fully empty LIBRARY records. A corrupted
  occupied slot is never silently substituted with a different sound.

The clean-break project format is now integrated with **experimental,
compile-gated** project SAVE, LOAD, RENAME, CLEAR, and LIVE recall. The
`firmware/src/project_ref_settings.c` encoder saves project-owned mixer
parameters, MIDI channels, transposition, fine tuning, zones, EQ, macros,
pad note maps/pan/pitch/performance and six lyrics, without copying sound
parameters, engines, FM6 patches or full drum-kit voices. A worst-case
32-macro, six-full-lyric simulated section fits the 1,792-byte budget.
`firmware/src/project_refs_live.c` resolves all six preset IDs before
restoring these settings; a missing preset refuses the section instead of
partially loading it. New USER sound saves mark the USER slot as the
track's reference source.

Under these opt-in flags, the old FUN snapshot persistence is replaced by
the two-sections-per-bank MPR1 store, but legacy formats are intentionally
not imported. Legacy sound-snapshot autosave has been disabled; a compact
reference-only autosave replacement is **still pending**. Old sequencer
staging is removed in MASHINA's normal NO_SEQUENCER build and is not
a requirement for reference-based section switching.

`firmware/src/editor_backup.c` now uses the existing chunked WebMIDI
archive protocol **65 LIST / 66 GET / 67 PUT** for all 32 MPR1 project
sections (archive IDs 40–71) and for the current unsaved MPR1 performance
(ID 0); it retains existing settings, USER banks and FM6 companion archive
IDs. Ref-only project writes remain disabled unless the separate
`FELUCCA_PROJECT_REFS_WRITE` gate is enabled. WebMIDI command **95**
reports MPR1 protocol/version/32-section/write capability.
The browser module `web/full-device-backup.mjs` builds projects-only
and full-device JSON backups, including the 32 LIBRARY overrides, 32 USER
records, all 32 project sections, unsaved current performance, global
settings and existing user-bank archive. Restore validates the offline
records and both write capabilities before any destructive operation,
includes clearing empty slots, and verifies library/project/User records
after restoring. The new Web Library preview has UI controls for each
operation and requires the user to save an existing complete backup and
explicitly confirm before restoring.

**Remaining requirements:** validate physical flash ownership and
hardware persistence, exercise the actual browser against the FM1,
resolve any remaining legacy UI backup/slot behaviors, complete and test
the new reference-only RESTORE LAST/autosave behavior, and establish
safe initialization of the clean-break flash sectors on a first
installation. No write-enabled experimental image is approved for
hardware yet. All changes remain branch-local; no main merge.

The 128 KiB saving is a **planned flash-sector reservation reduction**,
not additional free CPU RAM until the old layout is actually replaced on
a clean installation.

The existing 32/32 preset loader remains experimental, and main is unchanged.

## Source-level flash ownership audit (23 November 2026)

A new CI check (`tools/presets_library_flash_map_check.py`) asserts that the proposed
`0xEA000..0xF1FFF` LIBRARY region does not overlap known MASHINA firmware app,
USER/project store, autosave, FM6 voice bank, OTA staging or global settings.
It checks constants in the actual firmware and loader sources, rather than
depending only on a hand-maintained diagram.

The MASHINA-owned updater (`firmware/loader/ldr_core.c`) writes app sectors
`0x4000..0x92FFF` and skips the existing USER/project store in its update-record
sweep. **Important:** its record sweep *does* inspect the area containing LIBRARY
sectors for special update-record signatures; an eligible signature and CRC could
trigger erasure of that sector. The LIBRARY format normally leaves the final
bytes of each sector erased and has no matching update-record footer, but the
partition is **not** formally reserved against future loaders.

This is a **source-level regression gate only**: it does not establish ownership
by the stock SPL/bootloader, every possible flashing path, or actual on-device
read/write persistence. The write-enabled firmware is not cleared for installation.
Physical validation and safe backup procedures remain mandatory.

## Compile-time safety switches

The firmware's `tools/build.py` now accepts two independent flags:

- `FELUCCA_LIB32=0` (default): no library code or extra RAM in the firmware; existing USER-first browse prototype remains.
- `FELUCCA_LIB32=1 FELUCCA_LIB32_WRITE=0`: supports reading library records and exposes the experimental WebMIDI protocol in **read-only** mode. The browser must refuse uploads.
- `FELUCCA_LIB32=1 FELUCCA_LIB32_WRITE=1`: **experimental write implementation**, only compiled for explicit write-enabled testing. No write-enabled firmware is published or automatically installed.

MPR1 references have separate `FELUCCA_PROJECT_REFS=0` (default) and
`FELUCCA_PROJECT_REFS_WRITE=0` (default) gates. Enabling reference writes
requires the explicit LIB32 write gate too. The branch CI compiles a
reference-enabled image with both physical write paths **disabled**.

Both read-only and write-enabled builds are compiled separately in the dedicated branch CI. Both preserve the exact existing USER 32-slot physical layout. Nothing from this branch is merged to `main`.

## WebMIDI protocol

F0 7D 46 4C `cmd` ... F7, distinct from existing FM6 68–71 and other commands:

| Command | Description |
| --- | --- |
| **89 CAP** | Return `[format_version=1, slots=32, write_enabled]`. Browser refuses unsupported firmware |
| **90 LIST** | Query fixed slot occupancy in groups of up to 16 |
| **91 GET** | Read one complete 304-byte record, in a 348-byte 7-bit-packed response, or empty |
| **92 PUT** | Upload one full record atomically, with strict record/slot checks; write must be enabled |
| **93 ERASE** | Clear one independent LIBRARY slot; does not affect USER |
| **94 CAPTURE_USER** | Read a complete existing USER record **plus its tagged FM6/drum companion**; no flash writes. Missing or incomplete kit sidecars are rejected |
| **95 MPR1_CAP** | Return `[version=1, sections=32, project_write_enabled]`; a separate gate from LIB32 sound writes |

Web backup/restore code:

- `web/presets-library-codec.mjs`: validates 127 sound parameters, complete binary records and FM6 operator data. **Refuses** a drum-kit JSON that lacks the full 12-pad sidecar.
- `web/presets-library-sync.mjs`: capture 32 USER slots; read 32 library slots; export/import offline JSON; upload selected records and verify every upload with GET.
- `web/presets-library-midi.mjs`: explicit WebMIDI connection, serialized requests and timeouts.
- `web/presets-library-lab.html`: experimental import, slot grid, USER capture, device-library backup and upload. Upload is disabled unless CAP reports writable and the user has first backed up the current device library. The normal public editor site is unchanged.

**Important limitations:** Existing online user-preset JSON that lacks an independent drum-kit sidecar is **not** a complete drum preset. Use the new CAPTURE_USER operation (or a complete binary library JSON backup) to preserve all twelve pad sounds. Factory selection, favorites and LIST tag filtering over overwritten presets need further user-interface tests.

## Verification and pending gates

Passing host tests:
- Flash simulator: complete 304-byte independent records, slot isolation, CRC fallback, interrupted write, corrupt-copy handling and strict sector bounds. New regression checks additionally prevent first-use erasure of sectors containing foreign data, even with blank headers (CI verification pending).
- WebMIDI simulator: CAP, LIST, GET, PUT, ERASE and malformed record rejection.
- Browser fixtures: real melodic presets, FM6 full voice and full raw kit round-trips, USER capture, offline JSON backup and GET-verified restore.
- Standard firmware/UI/live-drum regressions; library-enabled firmware with writes locked compiles and meets linker budgets.

The library-enabled firmware **with hardware flash writes explicitly compiled ON** also passed its code/link test (CI 45206578319): image **509,992 / 584,336 B**, pool **308,432 B used / 11,040 B spare**. No device was flashed. The actual sound-loader host integration also passed (CI 45207741439), including melodic restoration, PC32 targeting another track, full DRUM-AC pad restoration, identical USER data after load, and simulated writes confined to the assigned library sectors.

The CI workflow bundles **`mashina-presets-library-32-preview`**, containing an `index.html` and required local ES modules; no installer/firmware is attached as a public release. Serve the directory from `localhost` (for instance `python -m http.server 8000`) in Chrome/Edge to inspect the 32/32 layout. A physical FM1 is usable only with an explicitly enabled experimental LIB32 build; regular v0.13 does not support the new SysEx operations.

Still required: true device-side SysEx integration tests, real-device verification of the `USR` region and update/restore behavior, emulator playback testing under continuous load, final curated factory selection, and web-library localization. Once complete, the user can explicitly authorize an experimental device installer. **Not yet device-ready.**
