# MASHINA Library Manager — web UI prototype

This directory contains an interactive **browser-only** preview for the planned MASHINA 300-slot writable sound library. Open `index.html` in your browser (or serve the directory with `python -m http.server`).

**Safety:** There is no WebMIDI or device-flash code in this prototype. **READ / WRITE buttons are disabled.** No action can affect a connected FM1. The first 100 slots and the project references are **illustrative demo content**, not the device's actual saved data.

## Three sections

- **FM1 Slots:** fixed slots 001–300 (no engine grouping); drag from a filled slot to an occupied slot to swap, or an empty slot to move; search and filter; stage changes; undo; export/import local 300-slot drafts. The change counter is relative to the initial browser demo, not the FM1.
- **Sound Collections:** import multiple MASHINA/Felucca bank JSON files (with `patches[]`), combine and filter sounds without a 300-sound limit, export all or selected sounds, or explicitly stage a selected sound into one of the 300 local slots. FM6 sound import checks for a 128-byte `fm6[]` patch.
- **Projects & Backups:** conceptual eight projects × four lettered sections (A, B, C, D) × six tracks, reference-based slots; import/export illustrative project templates and bundled setup JSON containing referenced sounds once each. **These demo imports and exports operate only on browser drafts, and are not real firmware backup files.**

The slot list uses a **project-use badge** (e.g. `P2:A,B +1`), and the three-dot control opens all referencing projects and lettered sections, e.g. `[P2:A,B] - [P4:B] - [P8:A,C,D]`.

## Implementation stages still required

1. Audit SPI NOR layout and allocate safe A/B transactional 300-slot library storage.
2. Replace compiled factory/user preset selection with universal writable slot references; keep engine-specific data such as FM6 voice patches.
3. Refactor persistent project records to reference slots only, preserving track-specific settings, and wire usage indicators plus SELECT/OCT+ reference popup.
4. Add WebMIDI read, validated staged writes, verification, and explicit confirmation. Never write on import, rearrangement, page close or connection.
5. Add real project/section/template/full-setup import/export; stage sound and project restoration as a recoverable transaction.
6. Validate with automated flash-interruption tests and physical FM1 hardware before allowing write controls.

This branch is intentionally **not merged into main**. It is for UI review before firmware/storage work.
