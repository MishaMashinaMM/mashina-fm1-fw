# One-device 32+32 first-install trial — staged, NOT flashable

**Branch:** `presets-library`. **Scope:** one sacrificial FM1 only; the second stays on the stable firmware.

**Owner approval:** preparation of a destructive test package was approved on 24 November 2026. This is **not** approval to perform a device write, and does not supersede the physical safety gates. Do not distribute or upload a flashable clean-layout `.fwsc` until separately reviewed and explicitly approved.

## Known verified foundations

- Emulator: the owner's nine original USER sounds auditioned as separate sounds; editing and saving one did not change others.
- Experimental project reference/storage, 32 LIBRARY storage, torn-program/erase fault-injection, and native firmware pipeline: passed through GitHub Actions run [45477998616](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45477998616).
- Physical read-only diagnostic: 53/53 candidate state-sector fingerprints matched across a tested MASHINA diagnostic update; 52 legacy CRC-valid and one erased. This proves preservation for that particular update, **not** a successful reset or recovery from an interrupted reset.
- The legacy 43-object archive and original USER sound pack remain separate offline assets; do not commit these to the public repository.

## Trial release blockers, in order

1. **Artifact protection:** `tools/build.py` deliberately builds `FELUCCA_CLEAN_INSTALL_LAYOUT=1` without packaging a flashable `.fwsc`; command 96 explicitly rejects combined initialization. This must remain so until all below are tested and approved. `tests/clean_install_release_gate_test.mjs` asserts this boundary.
2. **Formatter integration:** the 53-sector one-shot formatter exists in `project_refs_init.c`. A **source-only, non-distributable** two-phase WebMIDI prototype now exists: command 101 performs a read-only complete preflight and issues a 15-second nonce; command 102 requires the fresh nonce plus `MASHINA32`, then repeats preflight before any erase. The matching `web/clean-install-trial.html` requires re-import and CRC verification of the original 43-object backup, explicit one-device/rollback confirmations, typed `ERASE THIS FM1`, and a final confirmation dialog. This protocol is deliberately compiled OUT of all distributable firmware and is NOT a completed/approved installer. Physical restart and failure-recovery verification still block an actual trial release.
3. **Cold boot after formatting:** verify that RAM mirrors for USER, project sections and project settings are rebuilt from empty NOR (not kept from the old session). Confirm exactly eight projects × A–D begin empty, 32 USER slots are logically empty, and 32 LIBRARY factory fallback slots play.
4. **Interrupted-format recovery:** inject interruption at every destructive sector erase and the last marker commit in a simulated NOR run. Restart must never silently re-erase unknown, partially written or new-layout sectors. A fully erased/validated legacy-prefix recovery must be deterministic. Verify a retry is not destructive once the MPR1 marker is committed.
5. **Addressed original USER restore is not yet proven:** exporting the nine original sounds and successfully booting them in the emulator does **not** yet establish a safe, individually addressed physical USER 001–032 restore of the 192-byte records plus mandatory 112-byte FM6/drum companions. A hardware trial cannot be recommended as reversible until a complete post-reset USER restore, readback/sidecar verification and project-reference round-trip have been exercised. Do not treat the portable 304-byte JSON pack as a flashable device backup.
6. **First-device backup and rollback:** the owner must retain two independent offline copies of the original 43-object archive, verify a second parse/checksum and retain the known-working v0.13 installation route. A restore to physical hardware is not yet demonstrated. Do not infer restore-readiness from a JSON readback match.
7. **One-device trial:** after separate explicit permission, never target both devices. Keep external power/USB stable. Check all sound and project settings, one independent USER edit, one LIBRARY replacement, controlled cold restart, backup/export and restore, then subsequent MASHINA update persistence.
8. **Stop conditions:** any unknown/unreadable/foreign 53-sector classification, source address mismatch, bootloader/recovery uncertainty not consciously accepted for this trial, missing backup, failed build/headroom/OTA test, or unexpected post-install state blocks release.

## What can be issued without further approval

A **non-flashable review bundle** containing source commit, read-only 53-sector diagnostic, preflight report, regression logs, build-size report, and testing instructions. Do not label this bundle “installer.”

## What must never be inferred

A passing host build cannot prove source-region ownership or guarantee rollback. CRC32 fingerprints are not byte-exact full-device image backups. The original legacy archive cannot be loaded as MPR1 project records. A source-level formatter does not become safely installable merely because the owner has agreed to trial preparation.

## Source-only trial interface

The trial page is `web/clean-install-trial.html` (copied with its two JavaScript dependencies into the NON-FLASHABLE review dossier). It is **not** linked from the public MASHINA homepage or public firmware installer. The full firmware pipeline compiles the reset-capable path for size/type checks but `tools/build.py` explicitly refuses to emit a `.fwsc` when `FELUCCA_CLEAN_INSTALL_LAYOUT=1` or `FELUCCA_PROJECT_REFS_FORMAT=1`. Ordinary firmware returns status 2 to commands 101/102; the UI refuses to proceed without both write-enabled MPR1/LIB32 capabilities and a fresh valid preflight. No formatter can be physically invoked from the downloadable source build alone.

The C-level simulated NOR regression `tests/clean_layout_formatter_host_test.c` exercises the ACTUAL preflight/initialization code using known old objects, a foreign byte in the last sector, an interrupted sector erase, an interrupted final marker write, successful one-time initialize, and refusal to reinitialize. Simulated NOR safety is necessary but not a substitute for a controlled device recovery demonstration.
