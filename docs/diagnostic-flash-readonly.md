# MASHINA FM1 — read-only flash *probe* diagnostic

**Status:** Optional experimental diagnostic. Not a release, not merged into main.
**Firmware:** `v0.13-diag` — based on the v0.13 MASHINA firmware.
**USB:** Chrome/Edge WebMIDI with SysEx permission.

## Safety boundaries

- Firmware retains the **existing FUN project format**, rather than enabling
  the experimental MPR1 project layout; it does not migrate old projects.
- `FELUCCA_FLASH_DIAG=1`, `FELUCCA_LIB32=1`,
  `FELUCCA_LIB32_WRITE=0`, `FELUCCA_PROJECT_REFS=0`,
  `FELUCCA_PROJECT_REFS_WRITE=0`, `FELUCCA_PROJECT_REFS_FORMAT=0`.
- **All ordinary persistent-data flash writes are blocked in the diagnostic
  application at the lowest HAL range gate.** USB commands 97/98/99 are
  read-only. Experimental LIB32 and MPR1 writes/formatting are separately
  compile-disabled. The legacy USER, project, global-settings and automatic
  autosave paths may still *request* saves, but their erase/program attempts
  are rejected by the HAL. **OTA loader staging remains writable**, so the
  previously working firmware can be reinstalled. This is not a universal
  write-protected recovery environment: installing/reflashing the diagnostic
  still rewrites application/loader sectors, and stock SPL/bootloader/recovery
  behavior is not governed by the running application's data-flash gate.
- **Installing any firmware still flashes the application/loader.** This
  diagnostic does not prove that the installation/bootloader will preserve
  every data sector, and it cannot make a failed install harmless.
- When you explicitly invoke the probe, it inspects the 16 proposed MPR1 banks in
  `0xA0000..0xBFFFF` and four reclaimed LIBRARY candidate banks in `0xC0000..0xC7FFF`.
  It does not initialize or erase those regions.
- The 16-bank probe checks both sectors per bank for erased data, fully
  CRC-verified legacy `FELU` project records, or CRC-verified `MPS1`
  reference records. Mixed, unreadable or unknown content is flagged.
  The separate LIBRARY probe checks blank or committed LIBRARY banks and, in diagnostic mode, also recognizes CRC-validated retired FUN sectors in the reclaimed candidate area. Unknown/mixed data still blocks any format or write.
- This build is a **new candidate-region diagnostic**; the user's earlier November 24 JSON captured the older `0xEA000..0xF1FFF` proposal and cannot substitute for inspecting `0xC0000..0xC7FFF`. Both JSON layouts remain interpretable offline.
- Diagnostic-only WebMIDI command **99** reads CRC32 fingerprints of the 32
  candidate MPR1 sectors and eight LIBRARY sectors (40 × 4096 bytes, split
  into five read-only requests). Each sector is read in 256-byte pages with
  watchdog feeding; the browser receives **checksums only**, never private
  preset or project data. Busy or unreadable flash is reported as unavailable.
  This command is not present in ordinary firmware builds.
- A successful probe **does not** prove what every stock recovery tool,
  SPL/bootloader or future firmware installer will do. It is one physical
  memory-ownership check, not approval to enable destructive writes.

## Before installing

1. **Back up your currently running FM1 first**, including project sections,
   USER presets, FM6/drum-kit companions and global device settings. The
   experimental MPR1 full-backup option is **not compatible** with an existing
   v0.13 device; use the existing v0.13 backup/editor before switching.
2. Keep your existing working v0.13 firmware and recovery path available.
3. Use a reliable USB cable and keep the FM1 powered throughout firmware
   installation. Do not change USB cables during the process.
4. Use one FM1 unit for the diagnostic; do not install on both units until
   the first has been verified and its backup checked.

## Local browser installer (Windows / Chrome or Edge)

The GitHub Actions artifact `mashina-fm1-read-only-flash-diagnostic`
contains the `.fwsc` file, checksum, this README and a standalone
`webapp/installer/` web installer. Extract the complete ZIP, open PowerShell
in the extracted root and run:

```powershell
py -m http.server 8000
```

Open `http://localhost:8000/webapp/installer/` and follow the installer
instructions. Do **not** use the public MASHINA installer for this test: it
delivers the stable release, not the diagnostic package.

## Read the flash map (probe performs no writes)

From the same locally extracted artifact, open the experimental Web Library
preview in a second tab at `http://localhost:8000/web-library/`.

1. Connect the FM1 (allow WebMIDI SysEx).
2. Choose **Inspect flash (read-only)**.
3. Copy the status text or take a screenshot and share it here.
4. Do not click any restore/upload action or save using the FM1's own controls.
   Experimental LIBRARY/MPR1 writes are disabled, and the diagnostic app's
   flash HAL refuses persistent-data writes from legacy SAVE/autosave too.
   Avoid unnecessary SAVE operations because they can fail visibly; keep
   the diagnostic session restricted to read-only inspection.
5. After inspection, return to your known working v0.13 image if appropriate.

Status codes: **blank**, **committed**, **retired project**, **unknown** or
**unavailable**. An unknown/unavailable bank means no write-enabled experiment.
A bank of retired project sections must only be cleared after a complete
verified offline backup and an explicit future formatting decision.

The browser also embeds an offline `fingerprintAnalysis` summary when all 40 CRC32s are readable. It flags a repeated 16-KiB CRC sequence and checks for the erased-sector CRC32 (`F154670A`); these signals **cannot** establish ownership or address aliasing and do not authorize writes. See [clean-slate first-install plan](clean-slate-first-install.md).

The resulting `mashina-flash-diagnostic.json` (format version 2) now embeds
an independent, **read-only readiness audit** with the exact candidate bank
addresses, project-section/slot mapping, occupied/blank/corrupt counts,
reasons for blocking, and four explicit **unverified** conditions:

- physical sector ownership;
- stock bootloader/recovery/OTA preservation;
- verified complete offline backup;
- demonstrated restoration on a physical FM1.

The audit's decision is **always `PHYSICAL_WRITES_BLOCKED`**, even if every
sector is reported erased. This is deliberate: clean sector signatures only
tell us about *current contents*, not who is authorized to erase/program
them or how every firmware installer will handle those addresses.
The diagnostic artifact does not automatically enable an installer,
factory reset or write capability.

## Compare sector contents across two diagnostic sessions

A version-2 diagnostic JSON optionally includes `fingerprints` with 40
address-indexed CRC32 results. The Web Library now contains
**Compare two physical flash diagnostics offline (read-only)**. Import the
*before* and *after* diagnostic JSON files there. The browser validates that
each file contains all 40 readable sectors, then compares them and downloads
`mashina-flash-fingerprint-comparison.json` listing exactly which physical
4 KiB sectors changed. The source files are never modified, no FM1 connection
is needed for the comparison, and physical writes remain blocked even when
all 40 CRCs match.

CRC32 is useful for detecting unintentional changes but is not cryptographic
integrity or proof that firmware recovery will preserve these sectors.
A failed/unavailable checksum leaves the comparison incomplete and blocked.
Do not intentionally perform an experimental erase/update merely to produce
a second diagnostic file.

**No installer or device write was executed by ChatGPT.** The artifact is
prepared for user-controlled installation and inspection only.
