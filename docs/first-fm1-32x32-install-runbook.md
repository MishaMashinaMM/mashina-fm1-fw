# First physical FM1 32+32 installation — staged execution plan

**Status: ONE-DEVICE EXPERIMENT PREPARATION.** A deliberately gated, explicitly authorized GitHub Actions build can package a destructive first-install trial **only after the full regression workflow completes successfully**. The ordinary installer and automatic builds remain non-destructive. This is not a public firmware release.
Branch: `presets-library`. Exactly **one** FM1 will be used when the owner separately authorizes the first-install test; the other is deliberately left unchanged. Keep the XIAO RP2040 as an emergency tool, not part of normal installation.

## Why this is NOT a normal firmware update

The new storage scheme uses 32 USER + 32 LIBRARY slots. Its 8 projects each have sections **A–D** referencing the 64 shared sounds. Old FUN song sections occupy the candidate MPR1/LIBRARY sectors. Installing the application image alone DOES NOT remove or translate that old data. The one-device transition has **two distinct actions**:

1. Flash the new application in the usual way (old data still physically present). The new program may temporarily show empty/invalid projects before initialization; **do not save projects, USER sounds, library sounds or settings in this intermediate state**.
2. Once running, explicitly invoke the one-time **53-sector clean initializer** through its separate, double-confirmed WebMIDI preflight/commit protocol. This destroys the old projects, USER banks, FM6/drum companions, settings and aux objects. The backup MUST precede this action.

Commands 101/102 are included only in the special first-install firmware. The build release gate prevents ordinary push/diagnostic/release builds from packaging destructive initialization; the single exception requires a human-triggered `presets-library` Actions workflow with the typed confirmation `ERASE ONE FM1`, plus an exact release ID and all six explicit storage flags. The resulting artifact is an **experimental one-device package**, not a public installer.

## A. Pre-flash evidence — no experimental writes

- Choose one unit and keep the second unplugged.
- **No preservation of the old USER presets/projects/settings is required** for this deliberate fresh start. An existing JSON preset pack can be imported later into whichever slots you choose. Keep access to a known-working MASHINA firmware image for emergency reinstallation.
- Download the official [M-Flasher for Windows 10](https://jvitkauskas.github.io/m-apps/flasher/) and read its [download-mode instructions](https://github.com/jvitkauskas/m-flasher/blob/main/docs/usage.md). Connect the first FM1 directly with a normal USB data cable.
- **Optional recovery insurance, not an installation prerequisite:** use **DOWNLOAD MODE** if M-Flasher recognizes the FM1, then **Back up the whole flash** twice as distinct 1 MiB read sessions. If you choose to skip this, accept that restoring the exact pre-install flash state may not be possible, even though firmware reinstallation remains available. Decline any offered bootloader installation.
- Only if making the optional full-flash backups, run `py tools/verify_fm1_recovery_dumps.py FIRST.bin SECOND.bin` and retain their matching checksums.
- If you used M-Flasher, use **Restart** (or the FM1 power switch) and verify that the original firmware still boots and plays. Keep the XIAO disconnected.

**Recovery limitation:** reinstalling working firmware cannot recreate erased user data; this is acceptable for a deliberate fresh install only if you really do not need that original data. Two matching optional full-flash backups strengthen rollback options but do not guarantee recovery.

## B. Controlled first application install — single-unit manually confirmed experiment

After the full regressions pass, the one-device trial can be issued by a single explicit commit with message **AUTHORIZE ONE FM1 CLEAN TRIAL** on `presets-library`; the CI checks then run before packaging. A manually dispatched workflow with confirmation **ERASE ONE FM1** is also supported where GitHub exposes that option. Only that action can publish `mashina-ONE-FM1-clean-start-32x32-DESTRUCTIVE-TRIAL`, containing the exact firmware `.fwsc`, SHA-256 manifest, dedicated `webapp/installer/`, and separate `first-install/` reset page. Extract the whole artifact, open a command prompt in the extracted root, run `py -m http.server 8000`, and open `http://localhost:8000/webapp/installer/` in Chrome or Edge. After firmware flashing and reconnecting, use `http://localhost:8000/first-install/` for the explicitly confirmed 53-sector initialization. **Never use the public `main` installer for this trial.**

The dedicated first-install build has an additional boot-time write lock: ordinary USER/settings/autosave, MPR1 and LIBRARY erase/program calls refuse writes while old storage remains. Only the confirmed, synchronous clean formatter temporarily unlocks writes; they lock again immediately afterward and are re-enabled only after the next cold boot validates a complete committed first MPS1 bank. This is a source-level safety feature, not physical validation. **Do not attempt to save anything between installation and format, even if the interface allows editing.**

After installing, confirm device boot, USB/WebMIDI connectivity and firmware capability flags BEFORE any format command. If boot or connection fails, **stop**; use the previously saved original full-flash backup and the recovery plan. Don't attempt an uninformed extra flash.

## C. Deliberate clean initialization — NOT YET AUTHORIZED

- The local experimental trial page **does not require any legacy archive**. It asks the owner to acknowledge that the chosen unit's existing sounds/projects/settings are intentionally discarded. An original archive is accepted only by the separate optional USER sound restoration workflow.
- Read-only **PREPARE** calls command 101; firmware checks ALL 53 legacy sectors and issues a short-lived one-time ticket only on exact valid original layout.
- After an additional typed phrase and owner confirmation, **COMMIT** command 102 repeats all preflight checks before the first erase. Never auto-repeat if interrupted; partial format is a STOP/recovery condition, not a routine retry.
- On success, power-cycle. Check **32 empty USER**, **32 LIBRARY factory fallbacks**, and **8 empty projects each with A–D**. Do not restore legacy project sections directly into MPR1.

## D. Restore and actual new workflow

- **For this clean-start scenario, skip old USER restoration entirely.** The device begins with 32 empty USER slots. If someday desired, a separately confirmed optional restore can import original archive objects **6, 7, 9** with FM6/drum sidecars.
- If optional original USER restoration was used, power-cycle **again** and verify the restored sounds read-only; otherwise proceed directly to importing new sounds or creating presets.
- Create or replace one LIBRARY sound from the offline JSON, save one reference-based project section, confirm that saving one sound doesn't overwrite other USER/LIBRARY presets, and that a project referencing it updates when its underlying preset is deliberately changed.
- Power-cycle and verify project, library, and USER persistence; export a complete NEW-format snapshot to local JSON.
- Only after these tests pass would we consider the new firmware ready for the second FM1 or future updates.

## If anything fails

Do not try to reset the device again or overwrite unrecognized flash sectors. Save status messages and stop. The primary path is M-Flasher/ROM access over ordinary USB if available. If the unit doesn't enter ROM download mode, the separately verified **XIAO RP2040 / FM-1 Transporter** is an emergency fallback. The original full-flash backup belongs to this exact FM1; never restore it to the other one.

## Current gate

The non-flashable default release gate is retained. An exact manual one-unit release path is staged but should be used only after the full CI completes successfully and the owner intentionally confirms the destructive test. Simulated host tests cannot prove the first physical erase/recovery works. If installation or first initialization fails, **stop** and use the XIAO only if ordinary USB recovery is unavailable.

## One-device trial artifact authorization

The owner explicitly requested a completely clean first installation on one sacrificial FM1, with no migration or preservation of old USER presets, projects or settings. The exact commit message on this documentation update authorizes **GitHub Actions to build and publish one experimental firmware trial artifact only if every full automated check passes**. It does **not** initiate flashing, reset, or any physical device operation. The second FM1 remains unchanged. This artifact is not a public release and must not be installed on both units.
