# USER preset engine isolation — implementation and validation

**Status:** implemented and host/build validated on branch `fix/user-preset-engine-isolation`. Physical FM1 flash/save/reboot testing is still required. This is not merged or released.

## Root cause

`up_store()` originally captured only `P_COUNT` ordinary engine parameters. Drum engine pad patches and kit-wide controls live separately in RAM and were not included in the user preset. Changing the next kit therefore also changed what the previous user preset would play upon reload.

## What this branch changes

- `up_drum.c` adds independent compressed 32-slot user-kit serialization for **DRUM-AC, DRUM-IND, DRUM-LAB, and DRUM-X**.
- The existing `up_fm6.c` sidecar object holds 112 bytes per USER slot and tags them to that USER record. It was already included in complete exports/imports as backup object 9; a user slot holds either an FM6 patch or a drum kit. **No extra flash sector or USER record size increase.**
- Eight acoustic/industrial/lab global controls are stored in otherwise unused bytes of their original 192-byte USER record; DRUM-X's larger kit uses the sidecar plus unused record bytes and retired sequencer-pattern fields (not presented as a pattern).
- On USER load, the selected engine's pad patches are reset from the selected sound and then replaced by the recorded independent kit. Older records with no kit sidecar retain standard defaults.
- A rename preserves the sidecar tag (record names are excluded from its hash). Erasing and overwriting use the original USER bank and existing sidecar write paths.
- FM6 saves remain on the original FM6 codec, using the same tagged sidecar bank. Other melodic engines retain the ordinary parameter-only path.
- DRUM-X's encoded pattern bytes are excluded from legacy sequencer normalization during USER bank read and from the legacy pattern list.

## Validation results

Automated `tests/user_preset_isolation_test.c` and `tests/run_live_drum_tests.sh` pass on GitHub Actions, including:
- two divergent kits each for DRUM-AC, DRUM-IND, DRUM-LAB and DRUM-X, alternating load order
- per-kit global controls, per-pad values, type selection, DRUM-X additional patch data
- rename integrity and DRUM-X byte preservation during bank re-validation
- all 32 USER slots filled with mixed engine kits, simulated reboot/bank re-validation
- overwrite one slot and check all 31 others remain unchanged; erase behavior
- existing live-drum, UI, drum engine and project tests

The physical FM1 firmware was built successfully by the branch-only workflow with flash/RAM checks. Preview ZIP is attached to the successful workflow under **user-preset-isolation-device-test**.

## Before physical-device testing

Back up all existing data using the MASHINA web library/full backup facility. The preview installer is clearly marked DEVICE TEST ONLY. Flash this branch's candidate, not the public v0.11 installer. Save U01/U02/U03 as three clearly different DRUM-AC kits (including distinct per-pad EDIT settings), power-cycle, reload U01, U02, U03 in reverse order, and verify all are unchanged. Repeat for DRUM-IND, DRUM-LAB, DRUM-X; verify project recall and FM6 user presets.

**Do not merge to `main` until physical FM1 testing confirms these checks.**

## Remaining limits / compatibility

- Individual legacy editor UP_PUT/GET preset transfers contain only the standard preset record; transferring full drum kits requires **full backup/export-import** including backup object 9, not merely a legacy individual preset command.
- The DRUM-X kit remains engine-global at runtime as in v0.11; independent USER slots restore their own saved global kit when selected. This change does not introduce multiple simultaneous independent DRUM-X tracks.
- USER-bank and companion-bank commits are separate existing A/B flash objects. Like the pre-existing FM6 sidecar path, a power cut *between* the two writes could leave one slot's record and auxiliary payload temporarily out of sync; the companion tag prevents applying mismatched data. Further atomic multi-object journaling was not introduced.
