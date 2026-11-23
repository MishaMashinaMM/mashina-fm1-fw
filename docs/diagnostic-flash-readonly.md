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
  legacy project, settings and other firmware save paths are not globally
  write-protected by these flags.** Do not save presets, projects or settings
  while using this temporary diagnostic image; do not regard it as a
  write-protected recovery environment.
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

**No installer or device write was executed by ChatGPT.** The artifact is
prepared for user-controlled installation and inspection only.
