# Unified 128-slot editable presets — revised design (discussion draft)

Branch: `presets-editable`. **Design only**; firmware storage and UI have not been modified.

## Agreed direction

- Exactly **128 numbered slots**, displayed **001–128** on the device and addressed by incoming **MIDI Program Change 0–127**, with the exact mapping `PC 0 → slot 001`, `PC 127 → slot 128`. No bank switching is needed.
- **No factory versus USER distinction** in the playable library. Every slot is equally editable, renameable, overwriteable, moveable and **deletable**, even if it originally held a factory sound.
- A **fresh/empty install seeds** the 128 slots with a curated selection of available factory presets. Any factory sounds beyond the initial 128 are accessible in the web library, not permanently reserved on the FM1.
- The **web library** provides downloadable/reimportable factory preset packs, user exports/imports and full library backups. Restoring a factory sound is an explicit import, **not an automatic fallback** after deleting a preset.
- **No special protected factory region** in device preset storage. The firmware may still need compiled defaults for sound-engine initialization, hardware recovery or the seed *delivery* mechanism; these are not addressable writable preset slots and do not make deleted sounds reappear.
- Each project continues to save its **own complete sound snapshots**. Changing or deleting a library preset must never mutate the existing eight projects with four sections A–D.
- Stop at **design, storage audit and UI proposal** until approved. Do not modify real user data or flash maps until migration, downgrade and backup behavior is proven.

## Findings from existing source (v0.13)

The code currently has (a) engine-defined factory presets and import arrays, (b) 32 USER flash records, two banks of 16. Each USER `up_rec_t` is 192 bytes and has sidecar support for FM6 patches and full drum-engine kit state. Existing `UP_PUT`, `UP_STORE`, `UP_LOAD`, `UP_ERASE` and factory `PRESET` operations are distinct. Projects embed their actual engine parameters and drum/FM6 state and clear USER association upon load.

The current 1 MiB SPI NOR mapping has dedicated A/B sectors for 8 × 4 project sections, USER banks, FM6/drum companion records, autosave, OTA, preferences and the voice bank. Explicitly unused 0xDA000–0xDBFFF is just **8 KiB**, insufficient for 128 independent complete sounds. **We must audit firmware/update/bootloader regions and flash usage before reserving new sectors.** Do not repurpose project, OTA, or existing backup data.

A baseline of 128 × 192 = **24,576 bytes** covers only the current USER-type records, excluding full drum kits, FM6 patches, names/index metadata, CRC/transaction structures and write redundancy. At an indicative 192+112 auxiliary bytes per slot, 128 entries are about **38,912 bytes per copy** (77,824 bytes for two copies), before sector overhead; the actual worst case must be calculated from concrete serializations. The existing auxiliary formats are capped at 32 slots, and `up_fm6.c` currently occupies almost a whole flash payload, so simply increasing `UP_SLOTS` from 32 to 128 will **not work**.

## Factory initialization — crucial distinction

There are **two** safe cases, with different behavior:

1. **Completely new library:** all new-library headers invalid/absent **and** no legacy USER data detected. Install/seed the preset bank once, possibly by a first-run web-installer provisioning package, with an interrupted-provisioning restart mechanism. If a factory pack has fewer than 128 sounds, remaining slots are **empty**.
2. **Existing FM1 / firmware upgrade:** detect and preserve the current 32 USER slots, FM6 and drum sidecars, user projects and backups. Do **not** automatically seed or overwrite them. Require a backup and a preview of slot assignments/migration conflicts; commit only after a validated migration path or a user-initiated provisioning/import. A simple 'new format absent' check would falsely treat existing v0.13 data as a new installation and must not initialize over it.

If it is not possible to distinguish a truly empty store from a legacy installation reliably, use **explicit provisioning on the web installer** rather than attempting to guess. A firmware reflash/update should **never reset the user's 128-slot library**.

A deleted factory-origin slot stays **EMPTY after reboot**, even if that slot was in the first-install factory assortment.

## Firmware performance and sound behavior

- Selecting a slot loads its **full independent sound**, not a reference to another slot; changing one drum kit must not modify any other kit.
- Saving a sound to a slot updates that **one slot only**, plus its dependent patch/kit data in a transaction-safe way.
- The engine may change freely: slot 017 could initially hold ANALOG and later hold DRUM-IND, or be EMPTY.
- Save/move/copy/rename/delete operations require confirmation where destructive, safe failure handling and an explicit unsaved-work indicator. Do not silently overwrite a library slot on each knob movement.
- Leave project A–D snapshots independent. For a recalled project with a preset now deleted, its saved sound still plays normally.
- Preserve a stable PC mapping (0→001 … 127→128) and make PC on **each track's configured MIDI channel** recall the corresponding sound to that track; ensure global live recall and existing PC functionality do not conflict.

## Proposed on-device UI (mockup)

**PRESETS browser: one numbered list, no factory/user split**

```text
SAVE > PRESETS                 017/128
 015  FM6       SPARK
 016  DRUM-AC   KRIPKE-1
>017  ANALOG    CICERO            •
 018  EMPTY     —
 019  ATMOS     KANT

 K1 SLOT   K2 LOAD   K3 SAVE   K4 MORE
```

Names and slot positions are illustrative; `•` denotes **unsaved changes on the current track**, not 'factory' or 'user'. Favorites and type/engine filtering may remain available. The slot number is the primary identity. The list must remain readable on the 240×240 FM1 display, keep MASHINA's six-row/alternating-shade visual style, and follow its orange/gray high-contrast palette.

**MORE** opens `RENAME`, `SAVE AS` / `COPY`, `MOVE`, `DELETE`, and `IMPORT` only where device transfer permits it. Avoid a hidden 'RESET TO FACTORY' action because no slot owns a permanent factory default. To restore a supplied factory sound, import/select it from the web library and save it to any chosen position.

**PRESETS rotary:** stays a sound browser for rapid live selection, including EMPTY slots only in library-management mode; ordinary playing should skip EMPTY. Current project and global MIDI controls remain available. Confirm whether a dedicated SAVE page has any controls that should be preserved before replacing it.

## Proposed web library

A 128-slot table with direct **PC number**, slot number, name, engine, favorites, and occupied/empty state. Actions: audition/load, rename, edit, duplicate, move, remove, export, import into any slot, select many and perform bulk operations, and backup/restore **all** 128 slots including full FM6 and drum data.

Provide an independent **Factory Collection** catalogue in the browser (not installed permanently on the FM1), with categorized presets for all 10 melodic and 4 drum engines, preview/audition where feasible, and a target-slot selector. The user can repopulate any cleared slot at any time. Factory packs should be downloadable/offline-capable and versioned; user collections must be exportable.

Support two distinct operations: **restore an entire 128-slot backup** (with explicit overwrite warning), and **import selected factory/user sounds into chosen slots**. Avoid silently shifting slot numbers and PC addresses.

## Flash architecture gates (must pass before implementation)

1. Map every 4 KiB flash sector, firmware image capacity, OTA staging region, all legacy project/USER/sidecar objects and both A/B copies. Determine a guaranteed **minimum** space budget for 128 complete presets, including worst-case 12-pad drum kits, FM6 patches and metadata.
2. Choose record layout/index format that allows independent per-slot writes with power-loss-safe commits, recovery and consistent companion payloads. Check wear/endurance and flash writing while audio is active.
3. Verify 128 distinct presets survive power cycling, especially **128 full drum kits**, and that saving slot A does not change B. Do not rely on optimistic compression ratios.
4. Specify backup format, migration from 32 USER slots, preservation of 8×4 projects, and rollback/downgrade behavior. Establish first-install provisioning vs firmware-upgrade detection.
5. Modify firmware SysEx protocol and **web editor** together; existing tools know separate factory PRESET and USER operations.
6. Check MIDI Program Change mapping on all 16 channels and per-track routing, and keep any previous project-recall PC behavior explicitly documented/handled.
7. Once implementation and host tests pass, **automatically build and publish an isolated web emulator**. Device flash testing and explicit approval precede merge/release.

## Decisions to explore next

- **Seeding selection:** current firmware may contain more than 128 factory sounds. Which 128 will populate an empty device? Proposal: curated broad coverage of all 14 engines, with remaining originals available in Factory Collection online.
- **Provisioning responsibility:** safest proposal is web-installer-driven first-install seeding, with a firmware fallback only if provably pristine. Offline first boot must have predictable behavior.
- **MIDI PC precedence:** existing mappings may use PC to recall projects/sections. Define whether a special channel or configured mode preserves these commands while regular track channels select slots.

Status: agreed library model specified; storage architecture and migration **not yet implemented or validated**.
