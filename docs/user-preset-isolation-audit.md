# User preset isolation audit (work branch; NOT A FIX)

Status: analysis completed, firmware correction pending. Do not merge or flash this branch as a bug fix.

## Reproduction
1. Edit a complete DRUM-AC kit and save in USER U01.
2. Edit different pads and the eight global controls; save as USER U02.
3. Reload U01; compare each pad's EDIT values, global controls and audio with saved U01, then repeat U02.
4. Repeat with U03, power-cycle, reload in reverse order, and test export/import.
5. Repeat for DRUM-IND, DRUM-LAB and DRUM-X; also verify ordinary engines and FM6.

## Confirmed architectural cause
- `upreset.c:up_store` saves `motion_base_value(TSEL, i)` for only P_COUNT parameters into `up_rec_t`; `up_load` only restores these values, with a special FM6 sidecar (`upf_track_load`).
- `eng_drumac.c` stores per-pad changes in `ac_patch[NPART][12][8]`, per-track dirty masks and override flags, separate from P_E*.
- `eng_drumind.c` similarly uses `ind_patch[NPART][12][8]` and separate flags.
- `eng_drumlab.c` stores 12 editable patches in `drumlab_kit[NPART][12]` as well as `drumlab_global`.
- The DRUM-X engine owns its kit (`dx_kit`/`dx_kit12`) outside the standard P_COUNT values.
- `project.c` already explicitly serializes and restores DRUM-AC, DRUM-IND and DRUM-LAB per-pad kits and global controls into project sections. The USER pathway does not.
- FM6 differs: `up_fm6.c` already has a dedicated tag-bound storage object and reload path.
Thus the confirmed code defect is missing USER serialization of auxiliary kit state, not proven aliasing of two stored records. The exact symptoms on hardware still require regression testing.

## Storage and compatibility constraints
- Existing USER record is 192 bytes, 16 records per bank, two banks in flash (0xDC000..0xDFFFF). Do not change its shape or version without a migration strategy.
- Existing `storage.c` supports atomic A/B object copies with up to 3840 payload bytes.
- 0xDA000..0xDBFFF is presently described as free in `storage.c`, exactly one A/B sector pair; verify flash-linker reservations before allocating it.
- Worst-case 32 editable DRUM-LAB kits are larger than one standard storage payload uncompressed. Do not silently truncate, share state between slots, or overwrite song/index/OTA sectors.
- Backups (`editor_backup.c`) must include any added storage, validate imports and handle older archives that lack the new object.
- Keep main and the published v0.11 installer untouched until tests and real-device verification.

## Required implementation and tests
1. Design an atomic, per-USER-slot auxiliary patch store covering DRUM-X, DRUM-LAB, DRUM-AC and DRUM-IND. Ensure overwrite, erase and rename preserve coherent tags/records, and handle interrupted writes.
2. Save the complete selected engine's kit, per-pad overrides and global controls on UP_STORE and restore them on UP_LOAD, resetting stale per-track auxiliary state first.
3. Use a versioned format with explicitly checked capacity and no silent loss. Preserve current USER sound bank and FM6 sidecar compatibility.
4. Integrate auxiliary store with full backup enumeration, export, restore, and migration of older backups.
5. Automated multi-slot tests for all four drum engines, in-place overwrite, rename, erase, power-cycle, backup/restore, mixed engine slots and a full 32-slot bank; verify FM6 and melodic engine behavior unchanged.
6. Build and run firmware / host and emulator checks, then test on physical FM1 before merge.
