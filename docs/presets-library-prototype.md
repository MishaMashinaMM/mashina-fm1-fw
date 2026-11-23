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

## Follow-up measurements, 23 November 2026

- Experimental build from workflow 45194185147: **506,588 B** application image out of **584,336 B**, leaving **77,748 B**. RAM **95,952/98,304 B**; pool **305,316 B used / 14,144 B spare**. This confirms the fixed 32/32 browse scheme adds no meaningful RAM load.
- Fixed a browse correctness issue: a sound not among the curated 32 factory sounds must be shown as **off-list**, not misidentified as factory slot 033. The 32 USER slot positions remain reserved regardless of how many are occupied.
- `tools/presets_library_pack_experiment.py` measures real exported sound JSON using a lossless 127-signed-parameter encoding, plus synthetic random full FM6/drum sidecars; CI runs this in addition to the conservative 24 KiB A/B capacity gate. The real fixture compression ratio is **not** a guarantee of available flash capacity, and the experiment's binary format is **not yet a firmware serializer**.
- Current editor library exports **FM6 patches**, but the general JSON patch/fixture does **not** by itself encode every independent DRUM-AC/DRUM-IND/DRUM-LAB kit control in a compatible companion payload. Any end-to-end LIB_PUT implementation must explicitly capture and restore **full drum kit sidecars**. Do not use an incomplete JSON-to-binary conversion and claim full kit isolation.
- The app-image slack is **code/constant capacity, not spare persistent data storage**. The 8 KiB nonvolatile area would need all arbitrary 32 library sounds, metadata, and two safe A/B copies; the worst-case requirement is considerably greater. Native direct WebMIDI swapping therefore stays blocked without a safe storage redesign.

## Flash partition evidence and candidate (research only)

Our own `tools/fm1pkg_make.py` builds a JLFS `USR` descriptor at `0xEA000..0xFBFFF` (72 KiB); `VM` ends at `0xE8FFF`, and `BTIF` lies at `0xE9000`. `FL_STORE_OK` does **not** permit writes to the `USR` region today. Do not alter that protection without establishing loader ownership and persistence rules.

JieLi's AC79 SDK documentation explains that VM, reserved/protected partitions and updates obey flash partition configuration, and that application-defined user data partitions may be created. The presence of a `USR` descriptor is **supporting evidence** that the address range is set apart, not proof that all its sectors are blank, application-owned, or upgrade-safe.

References:
- https://doc.zh-jieli.com/SDKWiki/master/Jieli-Tech/fw-AC79_AIoT_SDK/1-overview/1.3-flashing-upgrade.html
- https://doc.zh-jieli.com/KDocs/master/%E9%9F%B3%E9%A2%91%E6%8A%80%E6%9C%AF%E5%BA%94%E7%94%A8%E6%96%87%E6%A1%A3%E5%85%A5%E5%8F%A3%E6%B1%87%E6%80%BB/%E8%80%B3%E6%9C%BA%E8%B5%84%E6%96%88%E6%A1%A3%E6%9C%89%E5%85%B3VM%E3%80%81%E8%93%9D%E7%89%99%E9%85%8D%E7%BD%AE%E5%8C%BA%E5%92%8C4K%E9%A2%84%E7%95%99%E5%8C%BA.html

A 32-slot, two-copy bank with the old 192-byte record and 112-byte sidecar fits within **24 KiB**, so a genuinely safe subset of the 72 KiB region could solve storage without shrinking the sound payload. This is conditional on real verification, not an allocation decision.

### Lossless compression test

CI run 45195603640 passed. 32 real factory sounds' 127 signed parameters + names encoded into an illustrative host-only record compress from **8,576 B to 1,377 B**. The same with zeroed 112-byte sidecars goes from **12,160 B to 1,423 B**. A deterministic full-data synthetic bank with independent sidecars goes from **12,160 B to 9,826 B**, which cannot fit either 3,840-byte per-copy payload. This is a strong reason **not** to use compression as an unbounded 8 KiB storage guarantee; real drum kit exports need dedicated sidecar support too.

## Host-only 32 KiB persistent store prototype

A new standalone module, `firmware/src/preset_library_store.c`, and simulator `tests/preset_library_storage_test.c` exercise a genuine lossless fixed-size **32-slot** data layout:

- Each record is exactly **304 bytes**: 192-byte existing USER format plus 112-byte FM6/DRUM companion payload. The two parts are written together in a single CRC-checked transaction, avoiding torn kit data.
- **8 slots × 304 = 2,432 B per bank**; four banks × two independently committed 4,096-byte sectors = **32,768 B** (eight sectors), tentatively mapped to `0xEA000–0xF1FFF` **in simulated NOR only**.
- Read and write operate on one **2,432-byte** scratch buffer, not a 32-sound RAM mirror. Zero records represent empty slots. Slot identities never shift.
- A/B copy-on-write commits the CRC-bearing header last. A failure during erase/write keeps the previous valid sector. Host tests cover 32 full records, individual replacement/clear, unrelated-slot isolation, CRC fallback, out-of-bounds addresses and interrupted updates.
- This storage module is **not included in the real firmware image**, and the real flash write permissions have **not** been widened. No production `LIB_PUT` command or web uploader is enabled.

This integrated design needs more sector space than the earlier split-format 24 KiB estimate, but it has simpler **atomic sound + sidecar** behavior and lower risk of desynchronizing complete kits. It fits well inside the nominal 72 KiB USR descriptor **if that descriptor can safely be used**. It does *not* fit the 8 KiB currently documented free outside that partition.

Before physical testing, audit the exact FM1 bootloader/updater handling of USR, protect any existing contents, verify retention across the official installer/firmware update, provide a backup/recovery path, and expose only read-only diagnostics until this is proven.
