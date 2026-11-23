# Unified 128-slot editable presets — revised design (discussion draft)

Branch: `presets-editable`. **Design only**; firmware storage and UI have not been modified.

## Agreed direction

- Exactly **128 numbered slots**, displayed **001–128** on the device and addressed by incoming **MIDI Program Change 0–127**, with the exact mapping `PC 0 → slot 001`, `PC 127 → slot 128`. No bank switching is needed.
- **No factory versus USER distinction** in the playable library. Every slot is equally editable, renameable, overwriteable, moveable and **deletable**, even if it originally held a factory sound.
- A **fresh/empty install seeds** the 128 slots with **100 curated factory sounds covering all 14 playable engines**, leaving **28 empty slots**. Any factory sounds beyond the initial 128 are accessible in the web library, not permanently reserved on the FM1.
- The **web library** provides downloadable/reimportable factory preset packs, user exports/imports and full library backups. Restoring a factory sound is an explicit import, **not an automatic fallback** after deleting a preset.
- **No special protected factory region** in device preset storage. The firmware may still need compiled defaults for sound-engine initialization, hardware recovery or the seed *delivery* mechanism; these are not addressable writable preset slots and do not make deleted sounds reappear.
- Each project continues to save its **own complete sound snapshots**. Changing or deleting a library preset must never mutate the existing eight projects with four sections A–D.
- Stop at **design, storage audit and UI proposal** until approved. Do not modify real user data or flash maps until migration, downgrade and backup behavior is proven.

## Findings from existing source (v0.13)

The code currently has (a) engine-defined factory presets and import arrays, (b) 32 USER flash records, two banks of 16. Each USER `up_rec_t` is 192 bytes and has sidecar support for FM6 patches and full drum-engine kit state. Existing `UP_PUT`, `UP_STORE`, `UP_LOAD`, `UP_ERASE` and factory `PRESET` operations are distinct. Projects embed their actual engine parameters and drum/FM6 state and clear USER association upon load.

The current 1 MiB SPI NOR mapping has dedicated A/B sectors for 8 × 4 project sections, USER banks, FM6/drum companion records, autosave, OTA, preferences and the voice bank. Explicitly unused 0xDA000–0xDBFFF is just **8 KiB**, insufficient for 128 independent complete sounds. **We must audit firmware/update/bootloader regions and flash usage before reserving new sectors.** Do not repurpose project, OTA, or existing backup data.

A baseline of 128 × 192 = **24,576 bytes** covers only the current USER-type records, excluding full drum kits, FM6 patches, names/index metadata, CRC/transaction structures and write redundancy. The current sidecar allocates **112 bytes per slot** for FM6 or drum data, so a 128-slot full-equivalent estimate is **38,912 bytes of record+sidecar payload per complete copy**, or **77,824 bytes for two copies**, before sector rounding, indexing and transaction overhead. Example: if the current A/B 16-record/sector organization is kept, eight record banks × two 4-KiB sectors = **64 KiB**; eight 16-sidecar banks × two sectors = another **64 KiB**; **128 KiB in total**, larger than the payload-only estimate. A more compact integrated format may reduce this, but needs a worst-case capacity proof. The existing auxiliary formats are capped at 32 slots, and `up_fm6.c` currently occupies almost a whole 3,840-byte flash payload, so simply increasing `UP_SLOTS` from 32 to 128 will **not work**.

### Measured v0.13 release memory budget (successful GitHub Actions build 45176097640)

From the actual linker/build log:

| Resource | Current occupancy | Available |
| --- | ---: | ---: |
| Main RAM (.data + .bss) | 95,952 / 98,304 B | **2,352 B** |
| Large RAM pool | 305,316 B used | **14,144 B spare** |
| Required pool safety headroom | 8,192 B | **5,952 B additional pool allocation at most** |
| Firmware application slot (XIP flash code/constants) | 507,020 / 584,336 B | 77,316 B **firmware build slack, not writable-presets storage** |

The current 32-record mirror already uses **6,144 bytes** in POOL, plus **3,728 bytes** for the FM6/drum sidecar mirror. Expanding these exact full mirrors to 128 slots would need **18,432 + 11,184 = 29,616 more bytes**, exceeding the additional safe pool budget by **23,664 bytes**. Main RAM has just 2,352 bytes free and is also not suitable for a full bank. **Verdict: a full 128-preset RAM mirror is not viable.**

**Flash-backed approach remains technically plausible**, provided storage can be allocated: keep only a 128-entry tiny metadata directory (e.g. fixed-width slot status/engine/name summary, preferably compact), a one-sector working buffer or smaller record cache reused with existing buffer paths, and the active sound in its existing track state. Avoid loading full 128 sounds into RAM at boot.

### Actual flash map and reservation warning

The packaged firmware uses flash image **0x00000–0x92FFF**, which is wholly rewritten by the installer. The documented safe persistent-data region starts at **0x97000**. The current eight projects / 32 sections occupy 0x97000–0x9EFFF plus 0xA0000–0xD7FFF. Existing user-bank flash: 0xDC000–0xDFFFF; sidecars use 0x9F000 and 0xFE000. Other sectors contain song index 0xD8000–0xD9FFF, autosave 0xE5000–0xE6FFF, FM6 voice bank 0xE7000, and settings 0xFC000–0xFDFFF. **Only 0xDA000–0xDBFFF (8 KiB) is clearly unused by the current MASHINA storage map.**

The package descriptors additionally reserve **0xEA000–0xFBFFF as 'USR' (72 KiB)** and refer to VM/backup space near 0xE8000–0xE9FFF. Those addresses are not currently admitted by `FL_STORE_OK` for normal writes. **We cannot yet count the 72 KiB as free**, nor erase them without proving loader/stock compatibility and ownership, testing firmware updates, and checking manufacturer data. If verified usable, this region could materially improve feasibility; if not, supporting 128 writable slots may require changing flash partitioning or removing other existing features, which has not been authorized.

**Current answer:** 128 editable slots are **not feasible by just increasing array sizes**, but are **potentially feasible with a flash-backed design**. A definitive yes is contingent on proving enough non-overlapping nonvolatile sectors and a power-safe storage format, as well as emulating flash/power-loss behavior. RAM itself is solvable via streaming/caching rather than full mirrors.

## Factory initialization — crucial distinction

There are **two** safe cases, with different behavior:

1. **Completely new library:** all new-library headers invalid/absent **and** no legacy USER data detected. Install/seed the preset bank once, possibly by a first-run web-installer provisioning package, with an interrupted-provisioning restart mechanism. The other **28 slots stay empty**.
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

- **Seeding selection:** current firmware may contain more than 100 factory sounds. **Agreed: curate 100 starter sounds spanning all 14 playable engines and leave 28 empty slots.** Remaining originals are available in Factory Collection online.
- **Provisioning responsibility:** safest proposal is web-installer-driven first-install seeding, with a firmware fallback only if provably pristine. Offline first boot must have predictable behavior.
- **MIDI PC precedence:** existing mappings may use PC to recall projects/sections. Define whether a special channel or configured mode preserves these commands while regular track channels select slots.

Status: agreed library model specified; storage architecture and migration **not yet implemented or validated**.


## Alternative under review: 32 USER-first + 96 load-only replaceable preset slots

This section records a *new candidate*, not an approved replacement of the 128-fully-writable specification above.

### Proposed addresses and navigation

- Positions **001–032** = existing 32 fully editable USER slots, including empty slots; `PC 0..31` always addresses those exact slot numbers.
- Positions **033–128** = 96 factory-style slots, load-only on the device, but optionally replaceable using the web library; `PC 32..127` addresses these fixed positions.
- Unlike current `up_count()` / `up_nth()` logic, empty USER slots must occupy their fixed place in the 128-slot directory. Browsing may skip empties, but PC numbering must *not* collapse around them.
- The original engine and sound identity of factory-style slots may change when the library is replaced; the slot number remains fixed. The 32 USER slots keep their existing A/B flash objects, FM6/drum sidecars, and RAM footprint.
- Current channel 16 Program Change 0–3 and 16–23 commands have special project/section behavior. Preserve their semantics or define an explicit mode/channel policy before asserting all 128 PCs directly select sounds.

### Two radically different meanings of 'replaceable from web'

**Option 1 — Build a personalized firmware image (strongest RAM/flash-data safety).** Keep the 96 factory-style sounds as read-only **XIP program/constant data** just as current factory presets and imports are. When the web library changes any slot, generate a *new firmware package* containing all 96 selected sounds and flash it via the existing installer. It is a full firmware rewrite, not a live preset upload. It needs a trusted firmware build environment / service (the current static browser page alone cannot compile the native firmware), backup/restore guarantees, repeatable builds, exact engine-specific kit/FM6 serialization, and full regression tests. The v0.13 firmware uses **507,020 of 584,336 B XIP image capacity; 77,316 B free** on the measured build. Whether 96 arbitrary full sounds and their metadata fit within that headroom has not been measured; the existing factory data already occupies some XIP, so a replace-not-add design should be assessed. This option requires **essentially no new 96-slot RAM mirror** and **no new dedicated writable data partition**. Program-flashing is heavier/slower and has firmware-update compatibility implications.

**Option 2 — 96 flash-data slots uploaded over MIDI/WebMIDI (more convenient live updates but flash risk).** A `96 × (192+112)` payload estimate is **29,184 B per unmirrored copy**, or **58,368 B for two copies**, before sector overhead and index/journaling. This is smaller than the 128-slot plan, but current verified unused normal data storage is only **8 KiB** (0xDA000–0xDBFFF). The firmware packager describes a **72 KiB 'USR' region** (0xEA000–0xFBFFF), but `fm1_flash.h` does not allow ordinary writes there and ownership/OTA interaction is unproven. Thus **do not treat Option 2 as safe or ready**. A small RAM index plus streaming would avoid a full 96-preset RAM cache, but the writable-storage safety problem remains.

### Immediate feasibility conclusion

Reordering existing USER entries ahead of factory entries is a **low-risk UI/index change** and does not require moving the 32 USER flash records. Restricting full local editing to those existing 32 also **avoids the ~29.6 KiB RAM increase** of multiplying the full current RAM mirror fourfold. But the requirement for replacing 96 sound slots *individually via WebMIDI without reflashing* is distinct: it requires trusted writable flash and robust storage management. Treat personalized firmware rebuilding and flashing as a viable alternative, **not as an invisible one-click preset upload**.

No runtime code, flash map, or firmware binary has been changed by this note.
