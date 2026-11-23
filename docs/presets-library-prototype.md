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
- Normal firmware still **refuses** writes outside its existing flash regions. This experimental module uses its own bounded checks for the candidate 32 KiB **only when separately compiled for writing**. A blank/uncommitted bank can be initialized; if both copies contain unreadable nonblank headers it fails closed, rather than erasing unknown data.
- On a successful library-enabled build **with writes locked**, image **509,144 / 584,336 bytes**, pool **308,432 bytes used / 11,040 spare**, main RAM **95,936 / 98,304**. This is **within** the firmware's 8 KiB pool safety headroom. These numbers were measured in CI run 45205479394.

## Compile-time safety switches

The firmware's `tools/build.py` now accepts two independent flags:

- `FELUCCA_LIB32=0` (default): no library code or extra RAM in the firmware; existing USER-first browse prototype remains.
- `FELUCCA_LIB32=1 FELUCCA_LIB32_WRITE=0`: supports reading library records and exposes the experimental WebMIDI protocol in **read-only** mode. The browser must refuse uploads.
- `FELUCCA_LIB32=1 FELUCCA_LIB32_WRITE=1`: **experimental write implementation**, only compiled for explicit write-enabled testing. No write-enabled firmware is published or automatically installed.

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

Web backup/restore code:

- `web/presets-library-codec.mjs`: validates 127 sound parameters, complete binary records and FM6 operator data. **Refuses** a drum-kit JSON that lacks the full 12-pad sidecar.
- `web/presets-library-sync.mjs`: capture 32 USER slots; read 32 library slots; export/import offline JSON; upload selected records and verify every upload with GET.
- `web/presets-library-midi.mjs`: explicit WebMIDI connection, serialized requests and timeouts.
- `web/presets-library-lab.html`: experimental import, slot grid, USER capture, device-library backup and upload. Upload is disabled unless CAP reports writable and the user has first backed up the current device library. The normal public editor site is unchanged.

**Important limitations:** Existing online user-preset JSON that lacks an independent drum-kit sidecar is **not** a complete drum preset. Use the new CAPTURE_USER operation (or a complete binary library JSON backup) to preserve all twelve pad sounds. Factory selection, favorites and LIST tag filtering over overwritten presets need further user-interface tests.

## Verification and pending gates

Passing host tests:
- Flash simulator: complete 304-byte independent records, slot isolation, CRC fallback, interrupted write, corrupt-copy handling and strict sector bounds.
- WebMIDI simulator: CAP, LIST, GET, PUT, ERASE and malformed record rejection.
- Browser fixtures: real melodic presets, FM6 full voice and full raw kit round-trips, USER capture, offline JSON backup and GET-verified restore.
- Standard firmware/UI/live-drum regressions; library-enabled firmware with writes locked compiles and meets linker budgets.

The library-enabled firmware **with hardware flash writes explicitly compiled ON** also passed its code/link test (CI 45206578319): image **509,992 / 584,336 B**, pool **308,432 B used / 11,040 B spare**. No device was flashed. The actual sound-loader host integration also passed (CI 45207741439), including melodic restoration, PC32 targeting another track, full DRUM-AC pad restoration, identical USER data after load, and simulated writes confined to the assigned library sectors.

The CI workflow bundles **`mashina-presets-library-32-preview`**, containing an `index.html` and required local ES modules; no installer/firmware is attached as a public release. Serve the directory from `localhost` (for instance `python -m http.server 8000`) in Chrome/Edge to inspect the 32/32 layout. A physical FM1 is usable only with an explicitly enabled experimental LIB32 build; regular v0.13 does not support the new SysEx operations.

Still required: true device-side SysEx integration tests, real-device verification of the `USR` region and update/restore behavior, emulator playback testing under continuous load, final curated factory selection, and web-library localization. Once complete, the user can explicitly authorize an experimental device installer. **Not yet device-ready.**
