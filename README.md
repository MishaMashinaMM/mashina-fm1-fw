# MASHINA — FM1 Performance Firmware v0.9

**MASHINA** turns the **M-VAVE FM1** into a flexible, six-track sound module for **external MIDI keyboards, pad controllers and live performance**. Assign sounds to MIDI channels, split or layer a keyboard, reshape parameters while playing, and recall projects without relying on a song sequencer.

Developed by **Misha Mashina**, based on [JIANT by Juan Ignacio Antonucci (Juan Jiant)](https://github.com/juanjiant-bit/JIANT) and the [Felucca firmware](https://github.com/hugelton/Felucca). Free software under **GPL-3.0-only**.

![MASHINA FM1 Performance Firmware — MIDI routing, synths, drums and live controls](docs/mashina-overview.svg)

**[Project homepage](https://mishamashinamm.github.io/mashina-fm1-fw/)** · **[Emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/)** · **[Web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/)** · **[Web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/)** · **[User manual](docs/JLD_MANUAL_V0.9.md)**

## What MASHINA does

| Feature | Description |
| --- | --- |
| **Six independent tracks** | Assign MIDI receive channels 1–16 to T1–T6; duplicate channels to layer multiple instruments. Per-track volume, pan and project-persistent routing. |
| **Keyboard zones** | Independent MIDI note and velocity ranges per track. Build splits, overlapping layers or velocity-switched sounds from an external controller. |
| **Pitch control** | Per-track transpose **−36 to +36 semitones** and fine tuning **−50 to +50 cents** on **GLO → MIX → TUNING**. Drum kits keep their original note mapping. |
| **Expressive melodic engines** | Inherited JIANT synthesizers plus **ATMOS** (evolving pads), **BRASS** (synthetic winds) and **INDUSTRIAL** (heavy basses and leads). |
| **Two 12-sound drum engines** | **DRUM-X** extends the original eight sounds with B2, S2, O2 and T2. **DRUM-LAB** offers twelve separately editable synthesized pads. |
| **32 MIDI CC macros** | Learn an incoming CC and bind it to a track's parameter. Choose range or toggle, endpoints, reverse and bypass. Stored with each project. |
| **MIDI Learn and live commands** | Additional learned destinations, MIDI routing overrides and MIDI-triggered sound randomization/restoration for live control. |
| **Effects and EQ** | Inherited effects, plus shared **three-band EQ** before effects: **50 Hz**, **700 Hz** and **10 kHz**, each **±15 dB**. |
| **Mixer and note activity** | Six-row mixer with per-track MIDI ownership/activity indicators and direct access to routing, zones and tuning. |
| **Live project management** | Save, recall and clear project slots; use Live Recall shortcuts to switch projects during a performance. |
| **Panic mode** | Immediately stop all voices and effects, clearing held-note and sustain state. |
| **Performance UI** | Black, white and gray with **orange #FF9900** accents, static MASHINA boot cog and streamlined held-button layers; no decorative animations. |

MASHINA uses **eight shared synthesis voices** across its tracks and engines; it does not provide eight voices per track. The original song sequencer is disabled and its former hardware buttons have live-performance functions.

## External MIDI setup

1. Connect a suitable **USB or TRS MIDI controller** to the FM1.
2. Open **GLO → MIX → MIXER**. Set each track's **RX channel (K1)**, **volume (K2)** and **pan (K3)**.
3. In **GLO → SYSTEM**, choose **ROUTE = 1-6** for assigned-channel routing across six active tracks. **ROUTE = 1-4** uses the first four; **SEL** follows the selected track. Set the active track count under **MENU → AUDIO → TRACKS**.
4. Open **GLO → MIX → ZONES** to define note/velocity ranges and **TUNING** for per-track transpose and fine detuning.
5. Save the project to retain its routing, zones, tuning and other project-specific settings.

For example, assign T1 and T2 to the same incoming MIDI channel and give them overlapping zones: a single keyboard can then play two layered instruments. Assign non-overlapping zones to create a split.

### Hardware controls for live use

| Control | Main live function |
| --- | --- |
| **GLO** | Six-track mixer, zones, tuning, sends, EQ and global settings. |
| **REC** | MIDI CC macro table: learn, edit, enable/bypass and delete assignments. |
| **SEQ** | Drum MIDI note mapping, not a song sequencer. |
| **PLAY** | Drum performance settings, not sequencer transport. |
| **SAVE** | Sound, preset, project and library operations. |
| **HOME (hold)** | Settings menu, including MIDI/USB options and the ABOUT page. |
| **GLO (hold) + OCT−** | Panic: silence tracks, effects and held-note state. |

**Live Recall:** long-press **REC** to enable this mode, then tap **FX / SEL / ENV / LFO** to recall project slots **1 / 2 / 3 / 4**. Long-press REC again to exit the mode.

**CC macros:** open **REC**, press **OCT+**, send a CC from the controller, then navigate to the track/parameter you want to control and turn its knob. Back in REC, edit its range/toggle endpoints and other options; save the project afterward.

See the **[MASHINA user manual](docs/JLD_MANUAL_V0.9.md)** for further control mappings, project behavior, engine details, and MIDI command values. Its historical `JLD_` filename is retained so existing links continue to work.

## Install or try in a browser

The **[web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/)** flashes MASHINA to the physical FM1 using a compatible desktop browser. The **[emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/)** runs the firmware UI and sound engines in a browser without flashing.

**Before flashing, back up your projects, user presets and settings**, and keep a known-working recovery firmware available. Projects saved by newer firmware may not be understood by older versions.

The [web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/) supports device management and backup/restore; some recent live controls are not yet exposed in its editing interface.

**Version:** v0.9 · **FM1 firmware identity:** `FM-1_909`. The MASHINA build has been flashed and functionally checked on a physical FM1, and the firmware regression and flash/RAM checks pass. This is not a claim of exhaustive endurance or recovery testing.

## Build from source

This repository's **`main` branch** is the MASHINA source of truth. GitHub Actions builds the browser emulator, real FM1 firmware package and Pages site from the same revision.

- [Build and deployment workflow](.github/workflows/emulator.yml)
- [Firmware build script](tools/build.py)
- [Firmware and live regression tests](tests/run_live_drum_tests.sh)
- [Interactive upstream manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/) — **inherited JIANT reference**, not an exact guide to MASHINA's revised pages
- [MASHINA manual](docs/JLD_MANUAL_V0.9.md) — describes its current live controls

## Origins, authors and licensing

**MASHINA:** Misha Mashina. Originally developed as *JIANT LIVE DRUMS*, then expanded into an externally controlled live-performance firmware. The [earlier development repository](https://github.com/MishaMashinaMM/jiant-live-drums) remains available as a historical archive.

**Original JIANT:** Juan Ignacio Antonucci (Juan Jiant) — [source repository](https://github.com/juanjiant-bit/JIANT) and [project/manual](https://juanjiant-bit.github.io/JIANT/).

**Felucca:** Hügelton Instruments, including Leo Kuroshita and other contributors. Further work by SKIFF, Melodee, fm1-x0x, sound-engine contributors, fonts and third-party projects is credited in the source headers, firmware ABOUT credits and bundled license notices.

This project is distributed under **[GPL-3.0-only](LICENSE)**. Original author credits and third-party licenses remain in place.
