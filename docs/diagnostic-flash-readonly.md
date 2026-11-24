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
- **Only the diagnostic operations are read-only.** USB commands 97/98 inspect
  flash without erase/program operations. Experimental LIB32 writes and MPR1
  project writes/formatting are compile-disabled. **Existing v0.13 USER,
  legacy project, settings and automatic autosave paths are not globally
  write-protected by these flags.** Even avoiding the SAVE button does not
  prove the firmware will perform no writes. Do not treat this temporary
  image as a write-protected recovery environment.
- **Installing any firmware still flashes the application/loader.** This
  diagnostic does not prove that the installation/bootloader will preserve
  every data sector, and it cannot make a failed install harmless.
- When you explicitly invoke the probe, it inspects the 16 proposed MPR1 banks in
  `0xA0000..0xBFFFF` and four LIBRARY banks in `0xEA000..0xF1FFF`.
  It does not initialize or erase those regions.
- The 16-bank probe checks both sectors per bank for erased data, fully
  CRC-verified legacy `FELU` project records, or CRC-verified `MPS1`
  reference records. Mixed, unreadable or unknown content is flagged.
  The separate LIBRARY probe checks blank or committed LIBRARY banks.
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
   The diagnostic firmware advertises experimental LIBRARY and MPR1 write
   capability as **disabled**, but existing firmware save paths remain capable
   of writing flash.
5. After inspection, return to your known working v0.13 image if appropriate.

Status codes: **blank**, **committed**, **retired project**, **unknown** or
**unavailable**. An unknown/unavailable bank means no write-enabled experiment.
A bank of retired project sections must only be cleared after a complete
verified offline backup and an explicit future formatting decision.

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
