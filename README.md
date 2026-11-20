# MASHINA — FM1 Performance Firmware v0.10

**MASHINA** turns the **M-VAVE FM1** into a flexible, six-track sound module for **external MIDI keyboards, pad controllers and live performance**. Assign sounds to MIDI channels, split or layer a keyboard, reshape parameters while playing, and recall projects without relying on a song sequencer.

Developed by **Misha Mashina**, based on [JIANT by Juan Ignacio Antonucci (Juan Jiant)](https://github.com/juanjiant-bit/JIANT) and the [Felucca firmware](https://github.com/hugelton/Felucca). Free software under **GPL-3.0-only**.

![MASHINA FM1 Performance Firmware — MIDI routing, synths, drums and live controls](docs/mashina-overview.svg)

### MASHINA online

[Project homepage](https://mishamashinamm.github.io/mashina-fm1-fw/) · [Play in emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/) · [FM1 web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/) · [Web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/) · [User manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/)

## What MASHINA does

| Feature | Description |
| --- | --- |
| **Six independent tracks** | Assign MIDI receive channels 1–16 to T1–T6; duplicate channels to layer multiple instruments. Per-track volume, pan and project-persistent routing. |
| **Keyboard zones** | Independent MIDI note and velocity ranges per track. Build splits, overlapping layers or velocity-switched sounds from an external controller. |
| **Pitch control** | Per-track transpose **−36 to +36 semitones** and fine tuning **−50 to +50 cents** on **GLO → MIX → TUNING**. Drum kits keep their original note mapping. |
| **Expressive melodic engines** | Inherited JIANT synthesizers plus **ATMOS** (evolving pads), **BRASS** (synthetic winds) and **INDUSTRIAL** (heavy basses and leads). |
| **Four 12-sound drum engines** | **DRUM-X** extends the original eight sounds with B2, S2, O2 and T2. **DRUM-LAB** provides independent synthesized percussion; **DRUM-AC** is an acoustic-style kit; **DRUM-IND** adds 12 noise-based, deliberately non-melodic industrial textures for MIDI-channel layering. |
| **32 MIDI CC macros** | Learn an incoming CC and bind it to a track's parameter. Choose range or toggle, endpoints, reverse and bypass. Stored with each project. |
| **MIDI Learn and live commands** | Additional learned destinations, MIDI routing overrides and MIDI-triggered sound randomization/restoration for live control. |
| **Effects and EQ** | Inherited effects, plus shared **three-band EQ** before effects: **50 Hz**, **700 Hz** and **10 kHz**, each **±15 dB**. |
| **Mixer and note activity** | Six-row mixer with per-track MIDI ownership/activity indicators and direct access to routing, zones and tuning. |
| **Live project management** | Save, recall and clear project slots; use Live Recall shortcuts to switch projects during a performance. |
| **Panic mode** | Immediately stop all voices and effects, clearing held-note and sustain state. |
| **Performance UI** | Black, white and gray with **orange #FF9900** accents, static MASHINA boot cog and streamlined held-button layers; no decorative animations. |

MASHINA uses **eight shared synthesis voices** across its tracks and engines; it does not provide eight voices per track. The original song sequencer is disabled and its former hardware buttons have live-performance functions.

## DRUM-AC acoustic-style kit

**DRUM-AC** is the third percussion engine alongside DRUM-X and DRUM-LAB. It synthesizes all sounds directly in fixed-point DSP; **it does not use drum samples**. The 12 independently editable pads are **kick, snare, crash, closed hi-hat, open hi-hat, floor tom, low tom, mid tom, high tom, clap, ride bell and ride splash**. Kick, snare and clap use noise-excited impact models; toms add an independently adjustable short drumhead resonance; metallic cymbals use noise-excited damped resonant bands. The cymbal modelling remains an acoustic-style approximation, not a recording of a real cymbal.

The instrument layout covers **all 12 semitones and repeats in every MIDI octave**, so the emulator keyboard or an external controller can play the whole kit without switching down to one specific octave:

| Note name (any octave) | DRUM-AC pad | Note name (any octave) | DRUM-AC pad |
| --- | --- | --- | --- |
| C | Kick | F♯ | Closed hi-hat |
| C♯ | Crash | G | Low tom |
| D | Snare | G♯ | Ride splash |
| D♯ | Clap | A | Mid tom |
| E | Ride bell | A♯ | Open hi-hat |
| F | Floor tom | B | High tom |

**Closed hi-hat chokes open hi-hat**, even across octaves, without stopping crash or ride tails. The repeating note-to-pad layout is fixed in DRUM-AC; do not assume DRUM-X/DRUM-LAB's custom SEQ mapping features apply to it.

### Editing DRUM-AC

Select a track running DRUM-AC, then press **EDIT** and turn **SELECT** to move through pages.

| EDIT pages | Purpose |
| --- | --- |
| **First two kit-wide pages** | Eight independent global sound controls: **TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL**. Each is **0–200%**; **100% is neutral**. They act consistently across the whole kit and do **not** overwrite or scale individual pad parameter values. |
| **SOUND 1–3** | Use **K1** to choose one of the 12 instruments; **K2–K4** change the selected pad's own **0–127** sound parameters. The eight pad fields are TUNE, DAMP, BODY, HIT, a pad-specific strike control, BRIGHT, a pad-specific timbre control and RING (or DECAY/TAIL). |
| **LEVELS 1–3** | Independent **0–127** volume for all 12 pads, with the same level taper as DRUM-LAB; project-persistent. |

The pad-specific field names tell you what is being changed: **BEATER / TEXTURE** on kick; **WIRES / TEXTURE** on snare; **SPLASH / METAL** on crash; **CHICK / DECAY** on hi-hats; **STICK / SHELL** on toms; **SNAP / TEXTURE / TAIL** on clap; **STRIKE** on ride bell; and **SPLASH** on ride splash. Not every field produces the same synthesis effect on every instrument. In contrast, the eight **kit-wide** controls above have a common musical purpose across all 12 pads.

For each tom, **TUNE = 0** removes the pitched drumhead resonance, while **TUNE = 1–127** adjusts that resonance from low to high, independently for the floor, low, mid and high toms. Changing TUNE does not retune the noise-based thud. **SPLASH** affects the noisy strike and metallic wash of the crash and ride splash.

DRUM-AC includes **ACOUSTIC**, **DRY KIT** and **BIG ROOM** factory presets. Its independent pad edits, levels and kit-wide values are stored with a project. The kit shares the firmware's existing **eight-voice total polyphony**; it does not add twelve simultaneous voices or a separate CPU allocation.

**Compatibility:** DRUM-AC is a new engine. Back up existing projects and presets before upgrading. Projects made with this engine may not be understood by older firmware. Older experimental DRUM-AC projects with non-neutral global multipliers may sound different because the eight global controls now use independent acoustic kit shaping.

## New in v0.10: DRUM-IND and flicker-free EDIT controls

**DRUM-IND** is the fourth 12-pad synthesized percussion engine, alongside DRUM-X, DRUM-LAB and DRUM-AC. It uses **no samples** and no fixed-pitch musical oscillators. Its noise-excited impacts, moving filtered bands, irregular gated bursts and sweeping textures are designed to **layer with other kits or melodic instruments**. Its name is separate from the older **INDUSTRIAL** melodic bass/lead engine.

**DRUM-IND keyboard mapping:** the twelve notes **C through B repeat in every MIDI octave**. MIDI 36–47 are one example octave:

| Key (example MIDI) | Sound | Character |
| --- | --- | --- |
| C (36) | **CLANG** | Dense metal-on-metal strike |
| C♯ (37) | **ANVIL** | Heavy hammer/anvil impact |
| D (38) | **METAL** | Bright, irregular sheet-metal resonance |
| D♯ (39) | **SCRAPE** | Metal dragged over a rough surface |
| E (40) | **WHISTLE** | Moving narrow-band noise whistle |
| F (41) | **SCREECH** | Unstable friction squeal |
| F♯ (42) | **BUZZ** | Abrasive electrical buzzing |
| G (43) | **LIGHTNING** | Sharp electrical crack with rolling noise |
| G♯ (44) | **ENGINE** | Slow uneven mechanical chug |
| A (45) | **WHOOSH** | Swelling air rush |
| A♯ (46) | **MOTOR** | Fast rotary mechanical noise |
| B (47) | **EXPLOSION** | Broad blast and turbulent tail |

These are synthetic interpretations, not recordings of machinery. WHISTLE, SCREECH and METAL use noise-excited resonant filtering; ENGINE and MOTOR use irregular noise pulses rather than pitched carriers. A sound can still have a recognizable spectral center, but none is deliberately tuned to a musical note. The note mapping is fixed, unlike the customizable DRUM-X/DRUM-LAB MIDI-map screens.

**DRUM-IND editing:** press **EDIT**, then use **SELECT** to navigate. The first two kit-wide pages expose **TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL**, each **0–200%** with **100% neutral**. These eight controls act independently of the pad values. **ATTACK** strongly shapes roughly the first 80 ms of a hit; **BRIGHT** applies broader spectral shaping after the original sound's limiting, so its effect is audible across all twelve models. SOUND 1–3 lets **K1** choose a pad and **K2–K4** edit its eight 0–127 settings across the pages: **GRAIN, DECAY, WEIGHT, STRIKE, MOTION, AIR, CRUNCH, TAIL**. **LEVELS 1–3** adjust the twelve individual pad volumes (0–127). Parameters may act differently on different models; all 96 pad/parameter combinations have been tested for audio response.

The factory sound presets are **STEEL YARD**, **PRESSURE** and **SCRAPHEAP**. Individual pad edits, levels, and independent kit-wide controls persist in projects. DRUM-IND shares MASHINA's existing **eight synthesis voices**; twelve pads do not mean twelve extra voices. Use duplicate MIDI RX channels across tracks to layer DRUM-IND with another kit.

**EDIT screen fix:** DRUM-LAB, DRUM-AC and DRUM-IND no longer trigger an unnecessary full-screen LCD redraw on every knob movement. The four-cell EDIT view repaints only changed cells during normal editing, while full redraw remains available for navigation and wake-up. This UI fix does **not** change the existing drum sounds or stored parameter values. The corrected DRUM-IND and EDIT screen were **successfully tested on a physical FM1**; automated firmware, emulator, LCD regression and installer browser checks also passed. This is not a claim of exhaustive endurance or recovery testing.

**Upgrading:** back up projects, user presets and settings first. Firmware before v0.10 does not include DRUM-IND and may not understand projects that use it. Keep a known-working recovery method available.

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

See the **[MASHINA user manual](docs/MASHINA_MANUAL.md)** for further control mappings, project behavior, engine details, and MIDI command values. The online version contains the same current controls and instructions.

## Install or try in a browser

The **[web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/)** flashes MASHINA to the physical FM1 using a compatible desktop browser. The **[emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/)** runs the firmware UI and sound engines in a browser without flashing.

**Before flashing, back up your projects, user presets and settings**, and keep a known-working recovery firmware available. Projects saved by newer firmware may not be understood by older versions.

The [web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/) supports device management and backup/restore; some recent live controls are not yet exposed in its editing interface.

**Version:** v0.10 · **FM1 firmware identity:** `FM-1_910`. DRUM-IND and the EDIT screen no-flicker fix have been tested successfully on a physical FM1. Firmware/emulator and automated UI/audio tests also passed; this does not constitute exhaustive endurance or recovery testing.

## Build from source

This repository's **`main` branch** is the MASHINA source of truth. GitHub Actions builds the browser emulator, real FM1 firmware package and Pages site from the same revision.

- [Build and deployment workflow](.github/workflows/emulator.yml)
- [Firmware build script](tools/build.py)
- [Firmware and live regression tests](tests/run_live_drum_tests.sh)
- [Online MASHINA manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/) — operating instructions for MIDI, sounds, drums and live controls
- [MASHINA manual](docs/MASHINA_MANUAL.md) — describes its current live controls

## Origins, authors and licensing

**MASHINA:** Misha Mashina. Originally developed as *JIANT LIVE DRUMS*, then expanded into an externally controlled live-performance firmware. The [earlier development repository](https://github.com/MishaMashinaMM/jiant-live-drums) remains available as a historical archive.

**Original JIANT:** Juan Ignacio Antonucci (Juan Jiant) — [source repository](https://github.com/juanjiant-bit/JIANT) and [project/manual](https://juanjiant-bit.github.io/JIANT/).

**Felucca:** Hügelton Instruments, including Leo Kuroshita and other contributors. Further work by SKIFF, Melodee, fm1-x0x, sound-engine contributors, fonts and third-party projects is credited in the source headers, firmware ABOUT credits and bundled license notices.

This project is distributed under **[GPL-3.0-only](LICENSE)**. Original author credits and third-party licenses remain in place.
