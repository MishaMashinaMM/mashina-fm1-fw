# One-device 32+32 first-install trial — staged, NOT flashable

**Branch:** `presets-library`. **Scope:** one sacrificial FM1 only; the second stays on the stable firmware.

**Owner intent (24 November 2026):** deliberate fresh installation on ONE FM1, discarding all old USER sounds/projects/settings. The second FM1 remains unchanged. An experimental first-install `.fwsc` may be packaged only via one exact explicitly authorized `presets-library` build and must pass all host tests. **No flashing has been performed by ChatGPT**, and this is not a public release.

## Known verified foundations

- Emulator: the owner's nine original USER sounds auditioned as separate sounds; editing and saving one did not change others.
- Experimental project reference/storage, 32 LIBRARY storage, torn-program/erase fault-injection, and native firmware pipeline: passed through GitHub Actions run [45477998616](https://github.com/MishaMashinaMM/mashina-fm1-fw/actions/runs/45477998616).
- Physical read-only diagnostic: 53/53 candidate state-sector fingerprints matched across a tested MASHINA diagnostic update; 52 legacy CRC-valid and one erased. This proves preservation for that particular update, **not** a successful reset or recovery from an interrupted reset.
- The legacy 43-object archive and original USER sound pack remain separate offline assets; do not commit these to the public repository.

## Trial release blockers, in order

1. **Artifact protection:** ordinary builds with `FELUCCA_CLEAN_INSTALL_LAYOUT=1` remain compile-only and produce no `.fwsc`. The one strict exception requires a `presets-library`-only GitHub Actions authorization commit (`AUTHORIZE ONE FM1 CLEAN TRIAL`) or a supported equivalent manual confirmation, plus an exact `0.13-cleantrial` ID and all storage flags. The candidate remains an experimental device-risk trial, never a public release. `tests/clean_install_release_gate_test.mjs` checks the default gate and exception.
2. **Formatter integration:** the 53-sector one-shot formatter exists in `project_refs_init.c`. An experimental two-phase WebMIDI formatter exists: command 101 performs a read-only complete preflight and issues a 15-second nonce; command 102 requires the fresh nonce plus `MASHINA32`, then repeats preflight before any erase. The matching `web/clean-install-trial.html` requires re-import and CRC verification of the original 43-object backup, explicit one-device/rollback confirmations, typed `ERASE THIS FM1`, and a final confirmation dialog. This protocol is compiled OUT of all ordinary public firmware and exists only in the separately authorized one-device trial image; it is not a public installer. Physical restart and failure-recovery verification still block an actual trial release.
3. **Cold boot after formatting:** verify that RAM mirrors for USER, project sections and project settings are rebuilt from empty NOR (not kept from the old session). Confirm exactly eight projects × A–D begin empty, 32 USER slots are logically empty, and 32 LIBRARY factory fallback slots play.
4. **Interrupted-format recovery:** inject interruption at every destructive sector erase and the last marker commit in a simulated NOR run. Restart must never silently re-erase unknown, partially written or new-layout sectors. A fully erased/validated legacy-prefix recovery must be deterministic. Verify a retry is not destructive once the MPR1 marker is committed.
5. **Optional original USER restore remains hardware-unverified and NOT required for this fresh start:** the experimental WebMIDI helper web/legacy-user-bank-restore.mjs now stages **only original archive object IDs 6, 7 and 9** (two USER banks and their FM6/drum companion bank), checks all 43 archive CRCs and per-sound sidecar tags first, requires write-capable MPR1/LIB32 firmware, a CRC-verified committed MPR1 first bank plus 15 fully erased project banks and four fully erased LIBRARY banks (read-only probes 97/98), and 32 empty USER slots. It refuses old FUN projects/settings, nonempty or unknown source sectors, performs no write retries, and captures 32 USER positions for 304-byte readback. Host mock tests must pass. This is **not yet proven on physical hardware**, is not atomic across three banks, and must not be treated as a guaranteed physical rollback.
6. **Recovery, not migration:** original USER/project/settings data is intentionally discarded, so legacy JSON and two full-flash dumps are optional recovery insurance, NOT prerequisites to begin with 32 empty USER slots. Retain access to known-working firmware and accept that merely reinstalling it cannot recover erased USER data. A restore to physical hardware has not been demonstrated.
7. **One-device trial:** after separate explicit permission, never target both devices. Keep external power/USB stable. Check all sound and project settings, one independent USER edit, one LIBRARY replacement, controlled cold restart, backup/export and restore, then subsequent MASHINA update persistence.
8. **Stop conditions:** any unknown/unreadable/foreign 53-sector classification, source address mismatch, bootloader/recovery uncertainty not consciously accepted for this trial, missing backup, failed build/headroom/OTA test, or unexpected post-install state blocks release.

## First-install transition write interlock

A newly installed experimental MPR1 firmware initially sees the old FUN/USER storage. On this special clean-install build, all normal USER/settings/autosave, MPR1 and LIBRARY erase/program operations are now **locked at boot** until an entire CRC-valid `MPS1` first project bank is observed. The deliberate, synchronously confirmed command 102 temporarily unlocks the 53-sector formatter; it locks normal saves again even on success or failure. The owner must power-cycle, at which time the firmware checks the new committed bank before re-enabling normal writes. A mid-format interruption leaves ordinary writes locked; automatic retry remains forbidden. The build gate remains non-flashable.

The intended physical procedure and the Windows 10 **ordinary USB / M-Flasher read-only double backup** (with XIAO reserved for recovery only) are in [first FM1 32+32 installation runbook](first-fm1-32x32-install-runbook.md).

## Physical recovery evidence, newly documented

The supported FM-1 Transporter recovery route using an actual **Seeed XIAO RP2040** and an externally wired USB-C data breakout was identified. See [the read-only physical recovery preflight](fm1-physical-recovery-preflight.md) for identification of the precise RP2040 board, safe D+/D−/ground wiring and VBUS isolation, and the independent dump requirements. The owner's USB-C **five-core** cable must be electrically identified before using it; colors are not reliable proof. The nRF52840 Sense variants are not interchangeable with the documented RP2040 Transporter image.

The command `python3 tools/verify_fm1_recovery_dumps.py first.bin second.bin` now checks two independent 1 MiB dumps, rejecting mismatches even inside user data above 0x93000; this is **offline and read-only**. Both full binary dumps must remain private. Neither an untested RP2040 setup nor a byte-exact backup is proof that a physical rollback will work. Firmware package/build release gates remain unchanged.

## What can be issued without further approval

A **non-flashable review bundle** containing source commit, read-only 53-sector diagnostic, regression logs, build-size report, and testing instructions. A separate explicitly authorized one-FM1 build is required for any actual installation package.

## What must never be inferred

A passing host build cannot prove source-region ownership or guarantee rollback. CRC32 fingerprints are not byte-exact full-device image backups. The original legacy archive cannot be loaded as MPR1 project records. A source-level formatter does not become safely installable merely because the owner has agreed to trial preparation.

## Source-only trial interface

The trial page is `web/clean-install-trial.html` (included in non-flashable reviews and any separately authorized one-unit package). It is **not** linked from the public MASHINA homepage or public firmware installer. The full firmware pipeline compiles the reset-capable path for size/type checks but `tools/build.py` refuses a `.fwsc` for destructive formatter flags by default, except under the exact manually/commit-authorized one-device trial gate. Ordinary firmware returns status 2 to commands 101/102; the UI refuses to proceed without both write-enabled MPR1/LIB32 capabilities and a fresh valid preflight. The ordinary source build cannot enable the formatter over USB. Only an explicitly authorized, separate 0.13-cleantrial device package can do so.

The C-level simulated NOR regression `tests/clean_layout_formatter_host_test.c` exercises the ACTUAL preflight/initialization code using known old objects, a foreign byte in the last sector, an interrupted sector erase, an interrupted final marker write, successful one-time initialize, and refusal to reinitialize. Simulated NOR safety is necessary but not a substitute for a controlled device recovery demonstration.

## Post-reset original sound restore (not yet device-tested)

The isolated trial page allows a fully clean initialization **without requiring a legacy backup**. Its USER-only restore option is a separate, entirely optional operation after a power cycle, with typed confirmation and firmware capability checks. The original 43-object archive must pass complete CRC and companion verification. The target must show the committed first MPR1 bank, 15 erased project banks, four erased LIBRARY banks and all 32 USER slots empty; this is a one-time first-boot restore, not an overwrite of a working device. The browser restores only objects 6, 7 and 9, then captures each complete 304-byte sound for exact comparison. It does not transfer old FUN project sections or old settings. A failed write can leave a partially restored bank; do not automatically retry. The portable nine-sound JSON pack by itself is not an adequate physical restore source: retain the original full 43-object archive.

## Independent post-restore cold-boot verification

The initial 304-byte USER preset readback occurs in the same boot session as the writes and can reflect RAM mirrors rather than durable NOR. That result is **NOT** proof that restoring survived power loss. After the three USER-only backup objects (6/7/9) are accepted, the owner must power down the FM1, power it up afresh, reconnect WebMIDI and use the new **read-only post-reboot verification** button. It compares every populated original USER slot and all 304 bytes, including drum/FM6 companions, against the unchanged offline archive. No PUT, erase, retry or automatic repair is performed by this verifier. The independent cold-start check is exercised against simulated WebMIDI in `tests/legacy_user_bank_restore_test.mjs`; physical hardware remains untested until a separately approved trial. Even byte-exact cold-start readback does not independently prove audible behavior or physical rollback.
