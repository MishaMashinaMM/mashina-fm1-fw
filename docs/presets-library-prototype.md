# Presets Library: 32 editable + 32 web-replaceable (experimental)

This branch is isolated from `main`; **do not flash to a physical device yet**.

## Intended behavior

- USER slots **001–032** are the *existing* 32 fully editable, persistent sounds. Their stored sector addresses and formats remain unchanged.
- LIBRARY slots **033–064** are intended to be load-only on the FM1, independently replaceable by the web library via USB/WebMIDI, without reflashing the firmware.
- The eventual first-install package should populate 32 LIBRARY slots with a curated cross-engine assortment; USER slots initially empty.
- JSON is a web/interchange format, not a JSON parser on the FM1. The browser sends compact, validated binary records (including FM6/drum sidecars).
- Program Change **0..31 → USER 001..032** and **32..63 → LIBRARY 033..064**, with empty USER slots preserving their PC numbers. MIDI channel 16 keeps its existing project/section command reservation.
- Projects remain full independent snapshots and must not change when preset slots are replaced.

## Code implemented so far

The `ui.c` browsing and MIDI program index are prototyped as **fixed USER-first positions followed by 32 read-only factory selections**. These 32 selections are presently compiled, selected round-robin across all visible engines (provisional rather than a musically curated pack); they are **not yet writable or uploadable**. The PRESETS knob skips empty USER positions while Program Change retains the fixed position (PC to an empty slot does not load a sound). The existing SAVE > USER functions, their storage, and the sound engine have not been changed.

## Measured storage feasibility and stop condition

`python3 tools/presets_library_capacity.py` is a read-only, source-verified layout check. Baseline:

- 32 × 192-byte existing-format sound records = **6,144 bytes**
- 32 × 112-byte FM6/drum auxiliary records = **3,584 bytes**
- **9,728 bytes minimum raw payload** for 32 arbitrary full records, before metadata and atomicity.
- For A/B bank copying with 4 KiB erase sectors and 3,840-byte payloads, the current-style layout needs **4 sectors** for the sound records and **2 sectors** for companion FM6/drum data: **24,576 bytes / 24 KiB** additional flash.
- Only **8 KiB** (`0xDA000–0xDBFFF`) is clearly unused in the current normal storage area. The **0xEA000–0xFBFFF** range (72 KiB, marked `USR` in `fm1pkg_make.py`) remains unverified as safe for nonvolatile firmware-controlled writes and not allowed by `FL_STORE_OK`. No writes to that region are authorized.
- The 32 existing USER presets already live in RAM. Additional LIBRARY sound records can be streamed individually from flash, so this concept does **not** require another 32-entry RAM bank.

**Stop condition:** Do not wire up an actual WebMIDI upload/write handler or mark an installer device-ready without a tested and explicitly allocated non-overlapping persistent region, A/B commit protocol, full FM6/drum isolation, power-loss recovery, and old-firmware preservation checks. This is a storage feasibility prototype, not the promised end-to-end transfer.

## Next implementation gates

1. Determine whether any part of the `USR` region can be used legitimately, including stock loader, firmware update, VM and manufacturer-data behavior, on real device and emulator.
2. If not, explore *lossless bounded compression and smaller records* but **prove** every allowable worst-case 32-sound set fits in 8 KiB (including redundant writes). If neither route passes, direct WebMIDI replacement of 32 arbitrary sounds is not safely implementable with the existing partitioning.
3. Only then implement persistent `LIB_GET/LIB_PUT/LIB_LIST` versioned SysEx and web library JSON→binary transfer, plus boot-time validation, atomic commit, error handling and integration tests.
4. Audit presets list, favorites, filter, SAVE navigation, and the fixed PC map with device/emulator tests. Stage a dedicated browser preview; never merge before physical testing.

The earlier `presets-editable` proposal remains a separate branch and is intentionally not merged here.
