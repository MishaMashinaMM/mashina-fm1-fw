# Presets Library — experimental 32 USER + 32 LIBRARY

**Branch:** `presets-library`, branched from `main`. Not merged, not released, not yet tested on a physical FM1. Do **not** flash this experimental build until its full read/write and recovery tests are approved.

## Agreed workflow

- **001–032**: the existing 32 fully editable USER presets, in fixed positions even when empty, `PC 0–31`.
- **033–064**: 32 fixed LIBRARY positions, `PC 32–63`, read-only on the device but replaceable individually or as a batch from the web library. Fresh/uninitialized positions play 32 compiled factory sounds spanning the available engines; selection is still provisional, not a final curated bank.
- User may fill all 32 USER slots, capture their complete presets in the browser, save an offline **JSON backup**, then upload those sounds to the 32 LIBRARY positions **without reflashing firmware**.
- A firmware update **may erase LIBRARY data**; this is accepted by the user, since offline JSON makes restoration possible. Never assume a firmware update safely preserves USER or project data without a separate complete device backup.
- The browser sends compact 304-byte binary sound records over 7-bit-safe WebMIDI; the FM1 does **not** parse JSON.
- Each live track loads its sound independently from the shared USER/LIBRARY bank. Under the experimental MPR1 project format, all 8 projects × 4 sections A–D store only preset references, so replacing one preset intentionally changes all sections that reference it. Project-specific mixer, tuning, routing, effects, macros, zones and pad controls remain independent.

## Flash design and measured headroom

- 32 library records × (192-byte USER sound record + 112-byte complete FM6 or drum companion) = **9,728 bytes raw payload**.
- Four banks of eight records; two alternating CRC-protected 4,096-byte sectors per bank, header committed **last**. **32,768 bytes** flash assigned in the experiment to `0xEA000–0xF1FFF`.
- The FM1 package labels `0xEA000–0xFBFFF` as **USR** (72 KiB). The eight candidate sectors are within it and do not overlap MASHINA's existing USER (`0xDC000–0xDFFFF`), 8-project section data, settings, FM6 voice bank or OTA staging. The package/loader's *actual* erase and retention behavior on physical FM1 devices remains unverified.
- Normal firmware still **refuses** writes outside its existing flash regions. This experimental module uses its own bounded checks for the candidate 32 KiB **only when separately compiled for writing**. A bank may initialize only when **both complete 4 KiB sectors are erased (all bytes `0xFF`)**. Blank headers alone are insufficient: foreign payload bytes, damaged records or interrupted first writes cause a fail-closed storage error rather than automatic erasure. Manual recovery is required in those cases.
- On a successful library-enabled build **with writes locked**, image **509,144 / 584,336 bytes**, pool **308,432 bytes used / 11,040 spare**, main RAM **95,936 / 98,304**. This is **within** the firmware's 8 KiB pool safety headroom. These numbers were measured in CI run 45205479394.

## Offline factory preset catalog extracted from firmware code

The file [`web/factory-presets-from-code.json`](../web/factory-presets-from-code.json)
collects **all 107 compiled factory presets** in firmware browsing order
from the current MASHINA engine tables, without reading a connected FM1.
It includes the names, source engine numbers and source C file, factory
ordinal, eight EDIT values, engine envelope/effects/pattern metadata and
an `initialLibrarySlot` for the 32 curated factory fallbacks.

- **89 melodic presets** contain all 127 parameter positions reconstructed
  from the exact factory-import arrays and preset declarations; all ten FM6
  presets also include their complete 128-byte operator voices.
- **18 drum presets** are present with their exact factory values and
  factory-kit indices. These require their full synthesized kit sidecars
  to be serialized separately before the browser can issue a 304-byte
  WebMIDI LIB_PUT for them. The catalog intentionally marks this requirement
  rather than presenting incomplete drum sounds as loadable records.
- A **107-entry offline catalog is not a 64-slot FM1 device snapshot**.
  MASHINA holds 32 USER plus 32 LIBRARY positions simultaneously; the
  browser can eventually choose which 64 sounds to install from this larger
  source catalog. The catalog cannot contain sounds that users have
  edited only on a physical FM1; those require WebMIDI export.
- A source-audit check at `tests/factory_presets_catalog_test.mjs`
  guards naming, engine ordering, parameter lengths, FM6 voices and
  the default 32-slot library map.
- **Protected master:** the original `factory-presets-from-code.json`
  remains byte-for-byte unchanged by the Web Library. A separate SHA-locked
  regression in `tests/factory_catalog_selection_test.mjs` rejects silent
  changes to the original 107 hand-curated sounds. Never overwrite that
  JSON when exporting or importing a 32-slot selection.
- The experimental Web Library can **load the original 107-preset JSON**
  directly or fetch its packaged copy, filter by engine/name, assign any
  factory preset to any LIBRARY 033–064 slot, and export a standalone
  `mashina-factory-selection-32.json` with the **entire original catalog and
  the 32 positions**, without writing to FM1.
- Full synthesized **DRUM-X / DRUM-LAB / DRUM-AC / DRUM-IND factory kits**
  are generated **separately** by `tests/factory_drum_export.c` using
  actual firmware preset-loading and drum-sidecar capture in the host
  simulator. The build generates `factory-drum-records.json` containing
  18 complete 304-byte sound records, which is packaged alongside the Web
  Library preview. The master catalog is not modified by this process.
  Validation matches every kit by engine, factory index and name.
  For DRUM-X, all nine BUTTLER factory kits are captured through the
  **full preset-loader path**, not a parameter-only load; a regression
  checks their complete kit sidecars remain distinct instead of accidentally
  duplicating the previous kit. Each generated kit has its own FNV checksum
  checked again at browser import.
- Once the separate kit records are loaded, **all 107 factory sounds can
  be assigned**, and all 32 chosen sounds can become complete uploadable
  records (including FM6 voices and synthesized drum kits). Exporting
  the 32-slot selection now also embeds the 18 kit records, making the
  selection independently reloadable without the companion file.
- **No destructive surprise:** choosing a catalog preset never writes to
  FM1, and changing a selection disables uploading any previously prepared
  pack until the newly selected set is validated and prepared. USB upload
  requires a separate compatible write-enabled firmware build, a prior
  device backup and explicit confirmation. No write-enabled physical
  installer is approved yet.

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
not imported. Legacy sound-snapshot autosave has been replaced in the experimental
MPR1 build by a **reference-only RESTORE LAST** record in the existing
A/B autosave flash sectors. It writes only during safe idle periods, preserves
project-owned settings, refuses unsaved sound edits, and rejects incompatible
legacy FUN records at boot. It never embeds sound snapshots or modifies
any preset. Simulated NOR tests cover persistence, refusal and unsafe-boot
suppression; hardware validation is still pending. Old sequencer
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

A separate, disabled-by-default `FELUCCA_PROJECT_REFS_FORMAT=0`
flag reserves one-time first-install initialization of the new 128 KiB
project flash bank. It is available only with both experimental write
gates enabled, through WebMIDI command **96**, and accepts only an
explicit `MPR1` request. Its preflight recognizes completely blank
sectors or CRC-verified old project records in the exact corresponding
legacy positions. Unknown data and already-initialized MPR1 sectors
are refused. It never touches the USER bank or global settings.
**The browser does not expose an initialization button yet**: read-only
device diagnostics and physical flash ownership checks are required first.
WebMIDI commands **97/98** report per-bank status without changing flash,
and the experimental Web Library has a read-only inspection button.

**Remaining requirements:** validate physical flash ownership and
hardware persistence, exercise the actual browser against the FM1,
resolve any remaining legacy UI backup/slot behaviors, verify the new reference-only RESTORE LAST/autosave behavior on hardware, and establish
safe initialization of the clean-break flash sectors on a first
installation. No write-enabled experimental image is approved for
hardware yet. All changes remain branch-local; no main merge.

The compact MPR1 flash writer now **streams 256-byte pages** rather than
holding a second whole 3,648-byte bank in memory. This restored the required
pool safety margin, without removing atomic A/B CRC validation or redundant
copy protection. Full experimental CI run
[45233452089](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45233452089)
passed host project/flash/power-loss regressions, the main and read-only
firmware builds, the separately gated write adapters and optional first-install
formatter *compile checks*, plus the standalone Web Library preview packaging.
Measured experimental MPR1+LIB32 write build:
**518,624 B / 584,336 B** image and **11,040 B pool spare**;
separately gated formatter build: **518,944 B** image, same pool spare.
These are build measurements only, NOT approval for physical flash writes.

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
sweep. Its record-cleanup logic now **explicitly skips all
`0xEA000..0xF1FFF` LIBRARY sectors**, even if the contents resemble a
valid update record. The source-level flash-map CI verifies this exclusion,
and the loader host test plants a forged update-record signature in LIBRARY
flash to confirm it survives upgrades. This protects the MASHINA update
path, **not every possible stock/vendor flashing method or bootloader**.

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
`FELUCCA_PROJECT_REFS_WRITE=0` (default) gates.
The destructive first-install formatter has an additional independent
`FELUCCA_PROJECT_REFS_FORMAT=0` gate, and is not enabled in any release. Enabling reference writes
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
| **95 MPR1_CAP** | Return `[version=1, sections=32, project_write_enabled, initialization_available]`; separate write gates |
| **96 MPR1_INIT** | Explicit one-time initialization of CRC-recognized retired project sectors; additionally gated, never enabled in normal firmware |
| **97 MPR1_PROBE** | Read-only status for 16 paired project banks: blank, MPR1, retired, unknown, unavailable |
| **98 LIB32_PROBE** | Read-only status for 4 paired 8-preset banks: blank, LIB32, unknown, unavailable |

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

## Browser restore hardening (23 November 2026)

The experimental Web Library now invalidates its prior-backup authorization **before** any
LIBRARY upload, projects-only restore or full-device restore begins. A failed or
interrupted write therefore cannot reuse that authorization for another destructive
operation. Keep the independently downloaded JSON: invalidating the authorization
does **not** delete or roll back an offline backup.

A full-device restore now reads back and checks **global settings** (archive ID 1)
and the **selected-project index** (ID 10), in addition to the existing USER,
LIBRARY, MPR1 project section and current-performance checks. A projects-only
restore now reads back all **32** sections, including empty positions, and
compares their canonical MPR1 records against the imported JSON before reporting
success. Browser regression fixtures simulate acknowledged writes with corrupted
settings or missing project data, which must fail verification. These changes are
committed on the experiment branch; the latest GitHub CI result has not yet been
confirmed. Neither a host simulation nor a successful compile authorizes physical
flash writes.

### Project dependency preflight

Before projects-only restoration writes its first section, it checks all
referenced USER presets against the connected FM1 and now also reads the
LIBRARY bank when any imported section references LIBRARY 033–064. A damaged
occupied LIBRARY record (as opposed to an actually empty factory-fallback
slot) aborts the operation **before any project PUT**. The host WebMIDI
regression simulates corruption and asserts zero project writes. The normal
read-only FM1 diagnostics and first-install flash ownership verification
remain prerequisites for any physical-device write test.

### Selected-project alias consistency

Projects-only restore now checks the current project's four archive aliases
(IDs 2–5) against their canonical global MPR1 sections (IDs 40–71), after
verifying all 32 restored positions. A stale current-project view therefore
fails verification even when the project bank's write acknowledgements and
read-back are otherwise correct. The browser regression fixture includes a
stale-alias failure case. This is host-side coverage only until the branch CI
and physical FM1 behavior are independently verified.

### Verified offline backup authorization

The experimental Web Library no longer treats clicking **Download** as proof
that a backup was actually saved. After a read-only LIBRARY or full-device
capture, the user must re-import the saved JSON via **Verify saved LIBRARY
backup** or **Verify saved complete backup**. The browser validates that
backup's records and requires its entire JSON content to match the most
recent capture from the current connected FM1. Only then can the corresponding
write buttons become available, still subject to explicit confirmation and
the firmware's independent write-capability gates. Reconnecting or beginning
any flash mutation clears the prior authorization. This does not guarantee
an external disk backup survives forever, but it catches cancelled downloads,
wrong files, stale backups, and edited backups before a destructive transfer.

The feature is branch-only and has not been physically exercised. Do not
publish or use a write-enabled experimental installer until CI, emulator and
flash-region ownership checks have passed.

### Projects-only export integrity

The projects-only WebMIDI capture now verifies that current-project archive
aliases (IDs 2–5) agree with the authoritative 8-project section records
(IDs 40–71) **before** exporting the JSON. An inconsistent live view must
produce an error, not a backup presented as valid. The JavaScript fixture
checks both a consistent capture and a deliberately mismatched alias.
Physical flash verification remains pending.
