# Presets Library — experimental 32 USER + 32 LIBRARY

## Current acceptance status — 25 November 2026

The v0.14 release candidate integrates the tested 32+32 layout, USB MASHINA identity and public Web Library. First-install is an explicit user-confirmed action over the fixed owned user-data sectors; existing committed MPS1 layouts are not eligible. This is a clean install, not a legacy-format migration. The experimental trial and earlier build history below are retained for engineering reference.

The following is the current branch status; the dated development notes below are historical and do not describe the present physical test boundary.

- One FM1 was deliberately initialized using the separately authorized clean trial, then updated with the initialized-device-only image. The second unit has not been changed.
- Physical acceptance covers USER save/persistence, individual and batch LIBRARY overrides, factory reset, USER sound recovery including complete drum companions, project section restoration, and complete logical backup restoration. The last live-only restore returned SUCCESS with full-state read-back verified by the Web Library. An earlier intermittent live-performance mismatch was not reproduced on the last trial; its cause remains unconfirmed. A previous intermittent backup CRC issue also has a dedicated regression.
- Desktop manager: USER, LIBRARY, PROJECTS and BACKUP, 32 positions in two columns, centered blocking operation modal. Chrome acceptance at 1536×864 and 1536×728 passed in quick host run [45625871904](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45625871904); full experimental run [45625871571](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45625871571) passed.
- The standalone package includes a [Web Library guide](../web/presets-library-guide.html). It describes the actual initialized-device workflow and backup boundaries, not the previous prototype interface.
- Release boundary: no merge, public installer, default build-flag change, general-purpose destructive initialization, or second-unit flash has been authorized. Public distribution still requires an explicit decision about the first-install experience, protection of existing firmware data, a version and packaging review, and matching published manuals. The current test artifacts remain separate from the public stable installer.

## SAVE > PRESETS browser (24 November 2026)

The old Jiant sound-type LIST on K4 has been removed. K4 now selects BANK: ALL (001-064), USER (001-032), or LIB (033-064). K1 and the PRESETS encoder navigate actual physical positions. K2 remains ENG; K3 remains FAV.

The highlighted slot is independent of the loaded sound. An unavailable LIBRARY entry shows LIBRARY UNAVAILABLE but leaves the cursor at that position; it cannot wrap prematurely or load an unrelated sound. Host regressions cover 046 to 047 and the full 033-064 range (wrapping at 064 to 033). Empty USER addresses keep their numbers but are skipped when auditioning. Bank view is temporary and does not write flash.


**2026-11-24 relocation prototype:** Active LIB1 is now allocated to *retired FUN project sectors* `0xC0000..0xC7FFF`, not the old `0xEA000..0xF1FFF` USR proposal. No LIB1 physical writes are approved. Prior exported emulator JSON with LIB1 at the old USR offsets remains accepted by the offline inspector and can be copied bank-by-bank into the new layout without changing the original JSON. The first physical diagnostic captured the *old* USR range; the new candidate sectors still need read-only device inspection. Earlier history below includes pre-relocation measurements and experiments.


**Branch:** `presets-library`, branched from `main`. Not merged, not released, not yet tested on a physical FM1. Do **not** flash this experimental build until its full read/write and recovery tests are approved.

## Emulator acceptance and release boundary (24 November 2026)

- **User-confirmed audition PASS:** the nine complete, individually recovered original USER sounds load in the experimental 32+32 emulator; they sound distinct, and editing/saving one does not change the others. This is manual **emulator** acceptance only; it does not verify device flash behavior.
- **Full experimental CI PASS:** GitHub Actions run [45473587005](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45473587005) completed successfully at commit `cd60c40b9c66368e755b554c4212932ebe0867e7`. It covers host and WebAssembly/storage regressions, native firmware compilation, source flash-map guards and OTA upgrade simulations. The previously prepared [browser emulator](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45472775127) was built from an earlier commit and passed its own workflow.
- **Outstanding, blocking physical release:** read-only physical inspection of the **relocated** `0xA0000..0xC7FFF` project/LIBRARY range; vendor bootloader/firmware update ownership verification; controlled blank-layout initialization, power-loss recovery, independent-bank persistence and full backup/restore on real hardware. Automated tests or a passing emulator audition cannot replace these checks.
- **Policy:** keep write-capable clean-layout installer unavailable, do not merge into `main`, and do not ask for a physical flash of the experimental layout until physical safety review and an explicit user decision. Preserve the original sound JSON and full snapshots throughout.

## Agreed workflow

- **001–032**: the existing 32 fully editable USER presets, in fixed positions even when empty, `PC 0–31`.
- **033–064**: 32 fixed LIBRARY positions, `PC 32–63`, read-only on the device but replaceable individually or as a batch from the web library. Fresh/uninitialized positions play 32 compiled factory sounds spanning the available engines; selection is still provisional, not a final curated bank.
- User may fill all 32 USER slots, capture their complete presets in the browser, save an offline **JSON backup**, then upload those sounds to the 32 LIBRARY positions **without reflashing firmware**.
- A firmware update **may erase LIBRARY data**; this is accepted by the user, since offline JSON makes restoration possible. Never assume a firmware update safely preserves USER or project data without a separate complete device backup.
- The browser sends compact 304-byte binary sound records over 7-bit-safe WebMIDI; the FM1 does **not** parse JSON.
- Each live track loads its sound independently from the shared USER/LIBRARY bank. Under the experimental MPR1 project format, all 8 projects × 4 sections A–D store only preset references, so replacing one preset intentionally changes all sections that reference it. Project-specific mixer, tuning, routing, effects, macros, zones and pad controls remain independent.

## Flash design and measured headroom

- 32 library records × (192-byte USER sound record + 112-byte complete FM6 or drum companion) = **9,728 bytes raw payload**.
- Four banks of eight records; two alternating CRC-protected 4,096-byte sectors per bank, header committed **last**. **32,768 bytes** flash assigned in the relocation prototype to `0xC0000–0xC7FFF`.
- These eight sectors were the legacy project's FUN sections at old bank indices 16–19, within MASHINA's existing data range. The new MPR1 project store ends at `0xBFFFF`; the LIB1 region ends at `0xC7FFF`, before the existing USER banks (`0xDC000–0xDFFFF`). The old `USR` candidate remains excluded and unowned. Hardware validation of the replacement region is still required.
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
`0xC0000..0xC7FFF` reclaimed LIBRARY region is separate from active MPR1 and USER and does not overlap known MASHINA firmware app,
USER/project store, autosave, FM6 voice bank, OTA staging or global settings.
It checks constants in the actual firmware and loader sources, rather than
depending only on a hand-maintained diagram.

The MASHINA-owned updater (`firmware/loader/ldr_core.c`) writes app sectors
`0x4000..0x92FFF` and skips the existing USER/project store in its update-record
sweep. Its record-cleanup logic now **explicitly skips all
`0xC0000..0xC7FFF` LIBRARY sectors**, even if the contents resemble a
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

**Important limitations:** Existing online user-preset JSON that lacks an independent drum-kit sidecar is **not** a complete drum preset. Use the new CAPTURE_USER operation (or a complete binary library JSON backup) to preserve all twelve pad sounds. Factory selection and favorites over overwritten presets still need hardware testing. The old LIST tag filtering has been removed.

## Verification and pending gates

Passing host tests:
- Flash simulator: complete 304-byte independent records, slot isolation, CRC fallback, interrupted write, corrupt-copy handling and strict sector bounds. New regression checks additionally prevent first-use erasure of sectors containing foreign data, even with blank headers (CI verification pending).
- WebMIDI simulator: CAP, LIST, GET, PUT, ERASE and malformed record rejection.
- Browser fixtures: real melodic presets, FM6 full voice and full raw kit round-trips, USER capture, offline JSON backup and GET-verified restore.
- Standard firmware/UI/live-drum regressions; library-enabled firmware with writes locked compiles and meets linker budgets.

The library-enabled firmware **with hardware flash writes explicitly compiled ON** also passed its code/link test (CI 45206578319): image **509,992 / 584,336 B**, pool **308,432 B used / 11,040 B spare**. No device was flashed. The actual sound-loader host integration also passed (CI 45207741439), including melodic restoration, PC32 targeting another track, full DRUM-AC pad restoration, identical USER data after load, and simulated writes confined to the assigned library sectors.

The CI workflow bundles **`mashina-presets-library-32-preview`**, containing an `index.html` and required local ES modules; no installer/firmware is attached as a public release. Serve the directory from `localhost` (for instance `python -m http.server 8000`) in Chrome/Edge to inspect the 32/32 layout. A physical FM1 is usable only with an explicitly enabled experimental LIB32 build; regular v0.13 does not support the new SysEx operations.

Still required: true device-side SysEx integration tests, physical read-only verification of reclaimed FUN storage and update/restore behavior, emulator playback testing under continuous load, final curated factory selection, and web-library localization. Once complete, the user can explicitly authorize an experimental device installer. **Not yet device-ready.**

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

### JavaScript regression execution (23 November 2026)

The project/export audit found that `projectsBackup()` wrongly interpreted a
canonical raw MPR1 byte array as a project object, causing
`captureProjectsOnly()` to fail with **invalid section**. The JSON codec now
accepts raw arrays, structured project values and `{recordBytes}` wrappers.
A permanent test covers the raw-array path. Two additional test-harness fixes
remove a dangling undefined variable and correct an invalid regular expression
in the full-device regression.

Four source-based JavaScript regression files have been executed successfully
in an isolated V8 environment with assertion/Node API compatibility shims:
`projects_library_codec_test.mjs`, `full_device_backup_test.mjs`,
`performance_backup_test.mjs`, and `presets_library_sync_test.mjs`.
This verifies their JavaScript assertions in that environment, **not** the
unmodified Node CI suite, native firmware build, or physical device behavior.
The experimental firmware write paths remain gated off; do not infer flash
safety or release readiness from these checks.

### Fast host verification pipeline

A separate branch-only GitHub Actions workflow at
`.github/workflows/presets-library-host-smoke.yml` now provides quick
verification **without** the JieLi cross-compiler, generated firmware assets,
or physical-device writes. It checks JavaScript syntax, the existing WebMIDI
and JSON backup regressions, the protected 107-preset catalog and selection,
flash partition boundaries/capacity, and pure C host simulations for MPR1
records, paired project flash, 32-slot library flash, wire protocol and
read-only diagnostics. The full `presets-library-check.yml` continues to own
firmware compilation and the six-track audio/project integration regressions.

Passing the quick pipeline will **not** mean the MPR1 firmware is safe to
flash: full CI, toolchain/linker budgets, emulator playback and physical
flash-region ownership must still be validated. The GitHub connector's
commit-workflow lookup is limited to pull-request-triggered runs, so an
empty result for these push-triggered workflows is not evidence of success
or failure. Read the actual job conclusion on GitHub Actions before treating
it as a CI pass.

### Experimental emulator flash preservation (23 November 2026)

The browser AudioWorklet now includes the experimental LIBRARY bank region
`0xC0000..0xC7FFF` among the flash sectors it persists across a page
reload. The existing `0x97000..0xDFFFF` range already covers the new MPR1
project sectors `0xA0000..0xBFFFF`. OTA, unknown and reserved sectors stay
excluded. An erased LIBRARY sector is not serialized, and restoration
requires exactly 4 KiB at a permitted sector boundary.

The standalone `tests/emu_library_flash_persistence_test.mjs` checks all
8 LIB32 sectors, MPR1 sector boundaries, erased-state behavior, and rejection
of malformed/outside flash snapshots. It ran successfully against the actual
branch AudioWorklet source in an isolated JavaScript runtime and is now part
of both experimental GitHub CI workflows.

**Important:** The normal `web/emu/build.sh` currently builds the legacy
emulator configuration and `web/emu/felucca_web.c` does not yet wire a
LIB32/MPR1-enabled browser preview or USB WebMIDI commands 89–98. This
persistence fix is a prerequisite, **not** evidence that experimental project
references or uploaded LIBRARY sounds can already be exercised through the
browser. Enable the simulator-specific feature integration and validate the
WASM build/audio/UI separately; never substitute a hardware write-enabled
firmware image for an emulator artifact.

### Branch-only MPR1/LIB32 WebAssembly preview (23 November 2026 testing gate)

The default emulator build remains unchanged. To compile the opt-in browser
simulation with reference-only MPR1 projects and the 32 LIBRARY slots, set
`MASHINA_EMU_REFS=1` when running `sh web/emu/build.sh` after normal generated
headers are prepared. `web/emu/felucca_web.c` then includes the experimental
LIBRARY runtime and project-reference modules, using bounded reads/writes
against its **1 MiB in-memory NOR array** only. The simulation flags do not
activate any physical FM1 SPI driver. The browser AudioWorklet now preserves
LIB32 sectors between reloads.

A dedicated `.github/workflows/presets-library-emu-preview.yml` workflow
builds the opt-in WASM variant using Emscripten, checks its required runtime
exports and packages **only browser assets**, under the artifact name
`mashina-mpr1-lib32-browser-emulator-experimental`. A separate
`EXPERIMENTAL-README.txt` explicitly forbids using it as a device installer.
Until this exact workflow finishes successfully, compilation of the new
simulator variant is **not confirmed**.

**Emulator verification sequence after a successful build:** boot with blank
simulated NOR, load and save independent USER sounds, assign USER/LIBRARY slot
references across tracks, SAVE a project section B, modify a referenced USER
sound, recall B and check the updated sound plus unchanged project tuning/mix
settings. Then reload the page to check that saved USER/projects persist,
exercise the other A–D sections and eight project indices, and test acoustic
and industrial drum engines for full-kit retention. Browser MIDI/editor USB
commands 89–98 are still absent, so uploading a 32-slot JSON selection to the
WASM emulator is not currently a valid test. That operation must await a
separate simulated WebMIDI interface or the later authorized physical-device
phase. Even a successful emulator pass does not prove flash bootloader ownership
on real FM1 hardware.

### Physical-emulator feedback: mixer name and project-save feedback (23 November 2026)

The first user-tested MPR1/LIB32 WASM build boots, renders the orange/gray UI
and plays all six tracks. Two user-visible issues were traced and patched:

1. A newly selected built-in LIBRARY sound could play while GLO > MIXER
   displayed a blank VOICE name. Its fallback path assigned the LIBRARY slot
   number but forgot to populate the per-track name cache used by GLO.
   Uploaded records and compiled-factory fallbacks now share the same bounded
   name-cache setter. The simulated-NOR sound integration test asserts the
   expected factory name, including 12-character truncation.
2. Saving a section could appear to do nothing because an active track
   without a USER/LIBRARY preset reference is rejected (by design: a
   reference-only project cannot capture an unassigned raw sound), or because
   the sound had unsaved edits. The error now identifies the offending track
   (for example, **T2: ASSIGN PRESET** or **T1: SAVE SOUND FIRST**). The
   project NAME editor stays open after a rejected save rather than returning
   to a still-EMPTY section without explanation. Host regressions cover both
   rejection reasons and editor retention.

**Retest after a fresh WASM build:** select a built-in factory LIBRARY preset
and verify its name on GLO > MIXER. For the simplest project test, assign a
saved USER/LIBRARY preset to Track 1 and mute any remaining active tracks
that have no assigned preset reference (or assign a valid preset to each).
Navigate to Project 1 Section B, select SAVE, then confirm the NAME editor
with **OCT+**. Section B should become occupied. Modify and overwrite USER 001,
then recall B: its referenced sound must update, but project mixer/MIDI/tuning
settings must remain independent. Test an emulator reload after the save.

These fixes have been committed to `presets-library`; no new CI or emulator
runtime result has yet been confirmed for the patched build, and no physical
FM1 was flashed.

### First four startup tracks use referenceable factory sounds

For the experimental MPR1/LIB32 boot path, Tracks **1–4** now load the first
four **LIBRARY factory positions** in order: **033, 034, 035, 036**.
These are the first four slots of the curated 32-position LIBRARY map,
not necessarily entries 1–4 of the separate full 107-sound factory catalog.
On a blank LIBRARY bank they use their compiled fallback sounds; if the
corresponding LIBRARY record has later been replaced, the track loads that
current replacement instead (the reference remains valid). Each loaded
sound receives a real per-track source reference and name cache, so a
four-track project can be saved immediately without manually selecting
presets on all four tracks. Tracks 5–6 keep their existing defaults, the
selected track remains T1, and the ordinary/legacy build path is unchanged.
Damaged LIBRARY slots are not silently substituted or marked assigned.

The MPR1 host integration regression now checks all four initial identities,
their names, and an immediate Project Section A save. The code is committed
but still requires fresh native CI and emulator verification.

### Startup T2 fingerprint correction (23 November 2026)

The first browser test of startup-assigned LIBRARY 033..036 reported
`T2: SAVE SOUND F...` when saving a project before touching any sound.
The startup source assignment had run **inside the outer load-depth scope**,
so individual LIBRARY sound loads could exit their `load_end()` early,
without normal motion-rebase/finalization before capturing the reference
fingerprint. The four LIBRARY startup loads now run **after** the outer
`power_on_parts()` loading scope closes, using the same complete sound-load
sequence as ordinary preset selection. No protection against genuinely
unsaved user edits was disabled.

The on-device message is now the shorter `T2: SAVE PRESET` (with the offending
track number substituted), fitting the screen better. The simulated NOR
integration test now reproduces the nested boot load and checks all four
sound fingerprints, plus T2 after a motion rebase, before attempting a
fresh project SAVE. This source change still requires a new successful CI
build and browser retest; it is not yet confirmed on the emulator.

### Corrected startup reference fingerprinting; real WASM SAVE gate (23 November 2026)

A subsequent browser retest still rejected an untouched initial four-track
project with `T2: SAVE PRESET`. The earlier change to finish the outer load
scope first was necessary but insufficient. The second factory position is a
DRUM-X kit, and DRUM-X maintains a **shared kit** rather than independent
per-track kit storage. Fingerprinting one track in the middle of a multi-track
load is unsafe: later loads can change shared sound state. The experimental
startup now refreshes all four loaded preset-source fingerprints only **after
all four tracks have finished loading**. Multi-track project recalls likewise
refresh their referenced sources only after the entire batch loads. This is
not done at SAVE time and does not waive the check for edits after loading.

Most importantly, the dedicated WebAssembly emulator workflow now **runs the
compiled browser firmware** after the normal boot and roughly five seconds of
simulated audio/UI frames. The new `tests/emu_mpr1_fresh_save_test.mjs`
requires all four startup references 033–036, all four sound fingerprints to
remain unchanged, and Section A to save successfully without touching a
preset. It then deliberately edits Track 2 and requires the exact same SAVE
to be rejected while the earlier section remains present. These checks use
simulator-only exported probes and an in-memory NOR flash, never the physical
FM1 flash. The artifact-upload step is withheld if this integration test
fails. **Do not claim the bug resolved until this workflow and a browser
retest both pass.**

### Root cause of the T2 startup-save failure (23 November 2026)

The real WASM integration test proved that **T2, T3 and T4 were already marked
modified immediately after `web_boot()`**, before any simulated audio
rendering. T1 stayed clean. Investigation found a second boot initializer:
`session_dice()` calls `macro_dice(0)` **after** referenceable presets are
loaded and their source hashes captured. That code populates vacant M1–M4
macro parameters (`P_M1SRC` etc.) and thus silently changes sound-owned
parameters in memory. This was the actual explanation for the false
`T2: SAVE PRESET` rejection; it was not caused by user edits.

For `FELUCCA_PROJECT_REFS=1` only, startup no longer generates random macro
assignments. Tracks retain exactly the macro parameters from their current
factory or uploaded sound records. **Explicit MACRO DICE commands still work**
and will correctly make an unsaved sound dirty until that preset is saved.
Legacy/non-reference firmware keeps its previous session behavior. The
existing running-WASM regression is required to pass initial Section A SAVE
and to reject a genuine later unsaved sound edit before any emulator artifact
is published. The previous host test was also fixed to stop calling the
NAME UI enum that its isolated harness does not include; a separate factory
fallback regression now resets its deliberately corrupted simulated flash.

Await new GitHub Actions results before considering this fix verified.

### RESTORE LAST reference-only boot behavior (23 November 2026)

The experimental MPR1 emulator retains its legacy safe-idle autosave policy:
it writes after **10 seconds without input, voices or transport activity**;
reloading the page immediately after switching projects is not equivalent to
a successful autosave. A project recall is not itself a destructive flash
save. The autosave is a reference-only copy of the *currently live* preset
identities plus project-owned settings; it does not duplicate sound patches.

Two bugs in the MPR1 autosave change detector were corrected on this branch:
(1) switching between preset sources was not part of the signature, even
though the reference identity is essential to a snapshot; (2) enabling
RESTORE LAST did not itself schedule any snapshot if the live patch had
not otherwise changed. The signature now includes all track source IDs and
active project/section context. The disabled-to-enabled transition forces
one save **after the same quiet period**, and a first boot with RESTORE LAST
ON but no valid previous autosave also schedules a new safe-idle snapshot.
The existing sound-change integrity guard remains active; unsaved sound
edits still cannot silently overwrite the last good snapshot.

An actual-WASM regression in `tests/emu_mpr1_restore_last_test.mjs` boots
one emulator against empty NOR, turns RESTORE LAST on, changes a LIBRARY
reference, advances enough simulated time for safe idle, copies the full
simulated NOR, boots an independent emulator instance, and verifies settings
and the precise source reference are restored. A second scenario enables
RESTORE LAST without any other live change and checks that a new autosave
appears after idle. CI has to pass before treating this as emulator-verified;
physical-device flash safety is still unverified.

### Emulator full-flash JSON export/import (experimental branch)

The browser emulator now offers **Export emulator data (.json)** and **Import emulator data (.json)** above its panel. Export while the emulator is running requests a fresh, read-only snapshot of the *actual WebAssembly NOR*, including recent saves; exporting before Switch on uses the existing browser-persisted sectors. The versioned `mashina-emulator-flash` JSON format records every non-erased owned 4-KiB flash sector, its offset, base64 bytes and checksum. This includes USER presets, LIBRARY banks, all 8 projects × A–D, settings and RESTORE LAST autosaves, but excludes bootloader/OTA/reserved sectors. Import validates the entire file (format/version, offsets, sizes, duplicate offsets, checksum) before boot.

**Import is isolated intentionally:** choose the exported JSON *before* clicking Switch on. The imported emulator uses the provided snapshot, and any changes made inside that session stay isolated; existing browser localStorage is not overwritten. Export those changes separately before closing the imported session. This is a byte-exact emulator recovery format, **not yet the Web Library's individual editable preset/project JSON** and not a hardware flashing or WebMIDI transfer mechanism.

Build artifact includes `web/emu/emulator-flash-backup.mjs` and the new UI. The `tests/emu_flash_backup_roundtrip_test.mjs` regression verifies identical flash round-trip, rejects corrupt/unsupported/reserved entries, and boots a second real WASM emulator from the JSON after the first saves a project. CI must pass before browser validation. The original 107-sound master catalog is untouched.


### Optional permanent emulator JSON restore

The browser emulator now offers **Restore permanently**, in addition to its existing read-only/isolated import. Choose a valid `mashina-emulator-flash` JSON before pressing **Switch on**. The validator checks the complete file first and enables the restore button only for a valid snapshot. Clicking **Restore permanently** displays a confirmation that the browser's existing saved emulator data will be replaced; cancel leaves everything untouched. Confirm writes the **complete sparse flash snapshot** to the existing `felucca-emu-flash` browser-localStorage key, replacing rather than merging older sectors, then reads it back and verifies the same bytes. On success, the file selection is cleared so **Switch on** loads the restored persistent state normally. The source JSON is never modified. The saved state survives subsequent reloads on the same browser origin unless browser site storage is cleared.

**Switch on with an imported file selected** continues to create an *isolated temporary session*; its edits are not saved to browser storage. Export that temporary session explicitly if you want to keep changes. Permanent restore is only available before the emulator is switched on and only for the dedicated emulator snapshot format, never for legacy device backups.

The CI emulator pipeline exercises successful permanent A → B → A replacement, invalid-file and failed-storage rejection, and a fresh real WASM emulator boot from the persisted snapshot. The experimental implementation does not change hardware firmware, flash memory layout, the 107 factory preset JSON, or the WebMIDI backup protocol.


### Inspect emulator backups in the Web Library (read-only)

The experimental Web Library preview now has an **EMULATOR SNAPSHOT · OFFLINE INSPECTION** panel. Select a JSON produced by **Export emulator data**. The inspector first checks the versioned 4-KiB sector/checksum archive, then validates committed MPR1 (16 A/B banks holding 8 × A–D), LIB1 (four A/B banks of eight 304-byte records), and normal ST/UPB1 USER preset banks (two A/B banks of 16 × 192-byte sound records). The newest valid committed A/B copy is selected; damaged/foreign banks with no valid copy are refused, never silently represented as empty. Empty LIBRARY overrides show **Factory fallback**, without guessing a factory name; the compiled fallback is not embedded in the flash JSON.

The Web Library displays all USER 001–032 names and engines, LIBRARY 033–064 overrides/fallbacks and project usage, and the 32 project sections with T1–T6 references. It can export **projects-only MPR1 JSON** (preserving opaque project settings) and **304-byte stored LIBRARY overrides JSON**, which are accepted by the existing library preview importer. It does **not** invent sound records for fallback slots, or silently treat USER's separate FM6/drum companion data as if the 192-byte USER record were a complete 304-byte upload. The source emulator JSON remains the lossless full-system backup. Importing it for inspection is read-only: no browser persistence changes, no WebMIDI request, and no USB/device write.

Both standalone experimental Web Library artifacts include the NOR decoder and the inspector's dependencies. The WASM emulator CI also tests inspection from real saved NOR, synthetic USER/LIB1 records, and CRC corruption refusal. Full individual USER conversion, device-side restoration and physical flash ownership checks remain separate future work. No changes are made to the original 107-preset factory catalog.


### Factory default labels in the emulator snapshot inspector

The inspector distinguishes an **unwritten LIBRARY flash override** (status `factory`) from a saved custom 304-byte override (status `overridden`). Factory fallback data lives in compiled firmware and therefore is **not** embedded in the exported NOR JSON. After validating an emulator snapshot, the Web Library loads the bundled, protected `factory-presets-from-code.json`, verifies the exact 107-preset fingerprint through `importFactoryCatalog`, and resolves all 32 `initialLibrarySlot` mappings to their original name/engine. These display-only labels appear as **Factory default**; saved flash records appear as **Custom override** and always take precedence.

If the protected catalog cannot be loaded or its fingerprint is wrong, snapshot inspection remains available, but the UI explicitly reports that factory names could not be resolved. This does not change the JSON archive, rewrite any browser or FM1 data, or pretend that compiled fallback sound bytes can be exported as standalone 304-byte records. Regression coverage verifies all 32 mappings, override precedence and rejection of a tampered catalog.


### Selectively compose a NEW complete emulator backup (offline only)

The Web Library snapshot inspector now includes **Selective USER & LIBRARY restore**, producing a new checked `mashina-emulator-flash` JSON without modifying the imported target file, browser persistence, physical FM1, or the protected factory source. Use **Inspect emulator snapshot (.json)** to pick the *target* flash state. Sound sources can be selected from the exact 107-preset factory master (all 18 DRUM kit companions must be generated from firmware code and validated), from a second snapshot's saved LIBRARY overrides, or from the target/second snapshot's USER sounds.

A USER source must contain its exact 192-byte stored record. FM6 and DRUM-X/LAB/AC/IND sounds additionally require a valid independent **UPF6** 112-byte companion tagged to that record by the firmware's FNV-1a hash (which excludes only the name). If no matching complete companion exists, the Web Library lists the source as unavailable; it does not invent default pads or synthetic voices. Factory default selections use complete compiled factory records, not just factory names.

**USER 001–032** and **LIBRARY 033–064** can both be destination slots. Replacing a destination changes the shared sound every project section referencing it will hear. The UI lists affected project tracks before export and permits multiple queued destination edits. Clearing a USER destination leaves it empty; clearing a LIBRARY override returns that slot to its compiled factory fallback. Only the *unmodified original* and the *new, downloaded JSON* represent the source and desired emulator states; no implicit browser write is performed.

The exported image stages each affected flash bank in the alternate A/B 4-KiB sector with the firmware's real CRC32 commit layout: USER banks (`UPB1`, 16 × 192 bytes), USER companions (`UPF6`, 32 × 116 bytes), LIBRARY banks (`LIB1`, eight × 304 bytes), and optionally MPR1 projects (two × 1824-byte records). The previous committed bank remains in the image. An unrecognized or damaged nonblank bank is refused, never treated as empty. A retired FM6B bank must be migrated by booting the emulator **before** selective USER editing, so adding a UPF6 commit cannot silently suppress firmware migration of older FM6 voices. Unselected records and sectors are preserved byte-identically.

Under **Optional: reassign project track preset references**, existing saved Project 1–8 sections A–D can have individual T1–T6 references changed to 001–064 or UNASSIGNED. This edits only the six reference bytes and checksum in that specific MPR1 section, preserving its name, all settings bytes and all sounds. Referencing a missing USER sound is prohibited unless it is also being installed in the same staged image. Empty project sections cannot be fabricated without their settings.

Use **Export modified complete emulator JSON** to download the resulting verified snapshot. Import it into a *temporary isolated emulator session first* and audition USER/LIBRARY sounds and all affected project sections before opting for permanent browser restore. Keep exportA/exportB and the new file as independent offline backups. The JSON is **emulator-only** and not directly flashable to a physical FM1.

CI tests stage multiple changes, preserve settings/unchanged sectors, exercise correct A/B selection, verify 192+112-byte FM6 and drum companions, block corrupt or legacy-migration-unsafe states, and cold-boot independent firmware WASM from the resulting edited flash image. The downloadable Web Library prototype ships complete 18 compiled drum-kit sidecars.
