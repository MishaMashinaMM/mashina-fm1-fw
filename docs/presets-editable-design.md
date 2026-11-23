# Editable Presets — architecture and UI proposal (discussion draft)

Branch: `presets-editable`. This document is a proposal, not a firmware implementation or a release.

## Goal

Remove the *factory vs USER* distinction from daily operation. Every sound in one numbered preset library should be loadable, editable, renamed, saved back to its own slot, duplicated to another slot, and restored to its original default where one exists. No separate USER bank in the UI.

Preserve MASHINA's eight projects × four sections (A–D), current on-stage sounds, engine IDs, MIDI Program Change compatibility wherever possible, backups, FM6 patches, and complete per-kit edits.

## Verified current architecture

- Factory presets live in compiled `engine_t.presets` and engine-specific import arrays; `apply_preset_to` loads sound parameters plus engine special data, including FM6 imported patch and drum-kit resets.
- USER bank has exactly 32 slots in two flash objects, 16 records/bank. A record is 192 bytes (engine, 12-character name, parameter payload, optional 16-step pattern); FM6 patch and drum-engine state use auxiliary storage.
- PRESETS rotary and SAVE > PRESETS already browse factory entries followed by occupied USER entries, while SAVE > USER has separate LOAD / ERASE / SAVE operations.
- Projects serialize complete track settings and additional patch/kit data. Project restore explicitly clears `track.user`; it does not reload the sound from a library reference. Thus existing project sounds should remain unchanged when a library preset is later edited.
- Storage uses CRC-checked A/B sector pairs for power-loss resilience. Project and song storage occupies 8 projects × 4 sections; user bank and auxiliary kit/FM6 storage have specific allocated sectors. A flash-map audit is mandatory before allocating more writable sounds.
- The web editor uses distinct factory-PRESET and user-slot SysEx operations; changing the device UI alone does not unify external tools.
- A USER record can also store a 16-step pattern, but loading the sound does not load the pattern. A unified *sound preset* should not silently load or erase steps.

## Proposed model: fixed preset identities with writable overrides

Use one logical table of numbered slots. For a built-in slot, immutable factory data serves as the **reset default**, not a second category of preset. If a user saves edits in the slot, load the saved override instead. Empty additional slots behave exactly the same as occupied slots once saved. The UI should not label slots FACTORY or USER.

- **LOAD:** select a slot -> resolve writable override if present, otherwise its compiled default.
- **EDIT:** alter the selected track's working sound as today, independently of the library. Show an unsaved-change marker if the current sound differs from the loaded slot.
- **SAVE:** write the current sound into the selected slot. Existing occupied slots require a clear OVERWRITE confirmation.
- **SAVE AS / COPY:** choose any target slot (including an additional slot) while retaining the original. Confirmation on occupied slots.
- **RENAME:** any slot, saved in writable metadata; never changes an engine's public name.
- **RESTORE DEFAULT:** only slots that had a built-in original. Discards the persisted override after confirmation; keeps the factory contents in ROM.
- **DELETE/CLEAR:** additional slots can become empty. Built-in slots cannot become empty; clearing restores the default instead.
- **FAVORITES / filters:** operate on unified slot identities, not an engine+preset versus USER discriminator.
- **Projects:** keep independent embedded snapshots, by default. Editing a library entry never mutates saved projects or currently playing tracks. On an explicit preset load, a project receives that preset's current sound snapshot. Add a *source slot* label as non-authoritative metadata only if size allows.
- **PC/MIDI:** retain legacy factory numbering for existing entries and provide an explicit compatibility/migration plan for old USER presets and Program Changes. Do not renumber engine IDs.

Important: a limited override cache (e.g. only 32 edited factory sounds) would **not** meet the stated 'ALL presets editable' requirement if edits cannot coexist. Full-capacity persistent storage, with optional export/restore, must be designed and measured first.

## Proposed device UI

### PRESETS browsing (one list)

```text
PRESETS                         023/160
> 023  ANALOG   CICERO
  024  ANALOG   PLUTARCH
  025  DRUM-X   BUTTLER-1
  026  VOICE    SINGER EN
  ...
```

Exact names/numbers illustrative; current engine order and position must be retained. Orange selected row; small `*` beside a name when a saved override exists, a `•` when the loaded track has **unsaved** edits. No FACTORY or USER tags.

### SAVE > PRESETS (single library manager, replaces separate PRESETS and USER pages)

Keep the six-row, alternating-shade list and predictable rotary behavior. PRESETS or SELECT navigates slots (final control assignments to be decided, avoiding conflicts with current PRESETS behavior). Use a small action page tied to the highlighted slot:

```text
SAVE > PRESETS                  023/160
 023  ANALOG    CICERO       *
 K1 SLOT    K2 LOAD    K3 SAVE    K4 MORE
```

`MORE` opens a compact action list: **SAVE AS**, **RENAME**, **RESTORE**, **COPY**, **CLEAR** (as applicable). Confirm any destructive operation with existing OCT− cancel / OCT+ confirm. Preserve the ability to browse with minimal button presses. On the selected track's normal EDIT pages, modifications remain immediate but do not rewrite flash until SAVE.

### Web library

One grid/table with slot, name, engine, favorite and modified/default marker. Support audition/load, edit/rename, duplicate, restore default, and full backup/import. Include a migration report and preview before rewriting old user-bank content. The full backup must include all editable overrides and engine-specific payloads.

## Storage constraints and implementation alternatives to measure

**A. Preferred:** compressed/delta writable overlays (factory base + complete replacement data when changed). Factory defaults remain in ROM. Slot identity is stable. Must guarantee sufficient persisted entries for **all** factory slots plus any expansion slots, even in worst case, including full FM6 patch and 12-pad drum-engine state; do not rely on typical compression ratio.

**B. Full writable bank:** move preset data to flash. Simpler semantics but potentially large flash, RAM and backup cost; extra sidecars for large kits. Requires audited sector allocation and safety migration.

Existing USER `up_rec_t` is 192 bytes. At 160 entries this alone is 30,720 bytes before names/metadata/sidecars and A/B redundancy. The project map has only explicitly documented 0xDA000–0xDBFFF (8 KiB) free inside the old song region; do **not** assume unallocated-looking addresses elsewhere are safe without checking firmware, OTA and flash layout. Preserving 8×4 projects is non-negotiable.

## Migration and acceptance criteria

1. Prior to any write: read all existing 32 USER slots and auxiliary FM6/drum payloads; preserve them losslessly with original names. Never erase legacy objects until new format is verified and rollback/recovery is available.
2. Resolve the placement of 32 legacy USER sounds in the unified list without silently overwriting factory edits. Keep stable mapping to old Program Changes where possible, and document any unavoidable remapping.
3. Prove 160-or-more independently edited sound slots can persist across power cycles **including distinct full drum kits and FM6 patches**. This is the critical feasibility gate.
4. Confirm saving one DRUM-AC/IND/LAB/X entry never alters another (repeat previous isolation regression with full per-pad parameter data).
5. Confirm existing eight projects/four sections are bit-for-bit safe, and changing a sound in the library does not alter their stored sound snapshots.
6. Test favorites, ordering, filters, name editing, overwrite/restore prompts, device MIDI PC, web editor and full backup/restore.
7. Validate flash/RAM footprint and power-loss behavior, then publish isolated web emulator automatically when host/firmware checks pass. Physical-device tests before merge/release.

## Decision points before writing storage code

- Should the unified library keep **all current factory positions plus the 32 former USER slots** as extra slots, or fold the USER slots into a fixed 160-entry total? Safest default: keep existing factory positions and append the 32.
- Should an existing slot's name be editable immediately while its engine identity remains changeable only through SAVE AS? Proposed: saving any engine into any slot is allowed; confirmation protects replacement.
- Should project sounds auto-follow later changes to their source preset? Proposed: **no**, projects remain self-contained snapshots.
- Should presets store musical patterns/steps? Proposed: keep legacy pattern data for migration, but the future unified *sound* preset only loads sound, not steps; separate any pattern management.

Status: source review complete and proposal drafted; **no preset data format or device UI changed**.
