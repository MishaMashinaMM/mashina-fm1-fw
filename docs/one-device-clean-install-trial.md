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
2. **Formatter integration:** the 53-sector one-shot formatter currently exists as a compile-time prototype in `project_refs_init.c`; no safe callable physical installation procedure is exposed. It cannot be considered a completed installer. The physical first-install action needs explicit operator confirmation, verified 53-sector preflight, and deterministic restart/failed-step reporting before write enablement.
3. **Cold boot after formatting:** verify that RAM mirrors for USER, project sections and project settings are rebuilt from empty NOR (not kept from the old session). Confirm exactly eight projects × A–D begin empty, 32 USER slots are logically empty, and 32 LIBRARY factory fallback slots play.
4. **Interrupted-format recovery:** inject interruption at every destructive sector erase and the last marker commit in a simulated NOR run. Restart must never silently re-erase unknown, partially written or new-layout sectors. A fully erased/validated legacy-prefix recovery must be deterministic. Verify a retry is not destructive once the MPR1 marker is committed.
5. **First-device backup and rollback:** the owner must retain two independent offline copies of the original 43-object archive, verify a second parse/checksum and retain the known-working v0.13 installation route. A restore to physical hardware is not yet demonstrated. Do not infer restore-readiness from a JSON readback match.
6. **One-device trial:** after separate explicit permission, never target both devices. Keep external power/USB stable. Check all sound and project settings, one independent USER edit, one LIBRARY replacement, controlled cold restart, backup/export and restore, then subsequent MASHINA update persistence.
7. **Stop conditions:** any unknown/unreadable/foreign 53-sector classification, source address mismatch, bootloader/recovery uncertainty not consciously accepted for this trial, missing backup, failed build/headroom/OTA test, or unexpected post-install state blocks release.

## What can be issued without further approval

A **non-flashable review bundle** containing source commit, read-only 53-sector diagnostic, preflight report, regression logs, build-size report, and testing instructions. Do not label this bundle “installer.”

## What must never be inferred

A passing host build cannot prove source-region ownership or guarantee rollback. CRC32 fingerprints are not byte-exact full-device image backups. The original legacy archive cannot be loaded as MPR1 project records. A source-level formatter does not become safely installable merely because the owner has agreed to trial preparation.
