# MASHINA — FM1 Performance Firmware v0.15

**MASHINA** turns the **M-VAVE FM1** into a six-track sound module designed for **external MIDI keyboards, pad controllers and live performance**. Play multiple sounds together, split a keyboard into zones, shape sounds while performing and recall projects quickly.

Developed by **Misha Mashina**, based on [JIANT by Juan Ignacio Antonucci (Juan Jiant)](https://github.com/juanjiant-bit/JIANT) and [Felucca](https://github.com/hugelton/Felucca). Free software under **GPL-3.0-only**.

![MASHINA v0.15 — 128 presets, Web Library, MIDI routing and live controls](docs/mashina-overview.svg?v=0.15)

### MASHINA online

[Project homepage](https://mishamashinamm.github.io/mashina-fm1-fw/) · [Play in emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/) · [FM1 web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/) · [Web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/) · [Web Library](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/) · [User manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/)

## MASHINA offers:

MASHINA is a **live-performance instrument** for the FM1: six independently configured tracks, playable synths and drum kits, extensive external MIDI control, and fast access to sounds and projects without a song sequencer.

| Feature | What's included |
| --- | --- |
| **10 melodic sound engines** | **ANALOG** (subtractive synthesis), **PHASE** (phase distortion), **LOFI** (chip, wavetable and bytebeat tones), **VOICE** (vowels, formants and speech), **WHEEL** (tonewheel organ), **NOISE** (noise and textures), **FM6** (six-operator FM), **INDUSTRIAL** (hard-edged basses and leads), **BRASS** (synthetic horns and reeds), and **ATMOS** (evolving pads and soundscapes). |
| **4 drum engines — 48 editable pads** | **DRUM-X** (expanded classic/electronic kit), **DRUM-LAB** (electronic percussion), **DRUM-AC** (synthesized acoustic-style drums), and **DRUM-IND** (metallic, mechanical and industrial percussion). Each engine has **12 independently editable sounds**, individual levels and live pad controls. |
| **Six-track MIDI performance** | Assign each track its own **MIDI receive channel (1–16)** or share channels for layered sounds. Choose routing for tracks 1–4, tracks 1–6 or only the selected track; select four or six active tracks as needed. |
| **Keyboard splits and velocity layers** | Independent **minimum/maximum MIDI note and velocity zones** for each track. Split a keyboard, layer several synths on the same notes, or switch sounds with playing velocity. |
| **Independent track tuning** | **TRN ±36 semitones** and **FINE ±50 cents** per track for octave shifts, detuned layers and custom voicings. |
| **Universal live timing** | Delay the onset of **any melodic or drum track by 0–50 ms** using **GLO → MIX → TUNING → K3 DLY**. Melodic notes retain their duration even when triggered late. |
| **Additive drum-pad timing** | Set **0–50 ms** of additional delay on each drum pad with **SEQ → MIDI MAP → K4 DLY**. Track and pad offsets add together (up to **100 ms**), allowing independent timing across kits. |
| **Custom drum MIDI maps** | On all four kits, the 12 pads repeat in every octave by default. Switch to **CUSTOM** for exact per-pad MIDI notes (**0–127**), including multiple pads on one note; reset to DEFAULT per track. |
| **Live drum performance controls** | Per-pad **PAN**, **PITCH ±24 semitones**, velocity curve and response, choke groups, and overlap/restart retrigger behavior on all four drum engines. |
| **Hands-on MIDI control** | **32 project-specific CC macros** alongside MIDI Learn; assign controller knobs, pads and pedals to sound parameters. Dedicated MIDI CC commands provide sound randomization/reset and live channel switching for T1–T3. |
| **Sound shaping and effects** | Engine-specific EDIT controls, envelopes, LFO and modulation, track FX/sends, a **shared three-band EQ before effects**, and a master DJ Filter on the mixer. |
| **128 replaceable presets** | **32 USER (001–032)**, editable on the FM1, and **96 LIBRARY (033–128)** factory-backed sounds, replaceable without reflashing. Full 107-sound catalog in Web Library, including FM6 and all drum kits. MIDI PC 0–127. |
| **8 projects × 4 sections (A–D)** | Each section references up to six preset slots and stores independent routing, zones, tuning, timing, mixer, effects and macros. Replace a preset once and every referencing project uses the new sound. Instant recall via **REC + FX / SEL / ENV / LFO**. |
| **Streamlined performance UI** | High-contrast orange-and-gray interface, clear track activity indicators, dedicated **SEQ** and **PLAY** drum pages, and a **GLO + OCT− panic shortcut** for immediate silence. No song sequencer or decorative UI animations. |
| **Web Library and complete performances** | Drag, multi-select, swap, rename and replace presets and projects. Build reusable offline sound collections and complete performance files, then write them to FM1. Automatic device reading and verification; save backups only when desired. |
| **USB identity** | Appears as **MASHINA** in MIDI devices and DAWs. |

MASHINA **inherits selected sound and drum synthesis engines from JIANT**, built on Felucca, while combining them with its own engines, MIDI performance controls, drum features, project workflow and redesigned interface. The result is a substantially different firmware rather than a collection of additions to JIANT.

All six tracks share **eight synthesis voices in total**, not eight per track; **BRASS** also has a four-voice limit. The firmware is designed for real-time playing rather than song sequencing.

For detailed engine parameters, drum layouts, MIDI controls and operating instructions, see the **[User Manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/)** (English, Serbian, Spanish, Russian, Japanese and Hindi).

## Drum MIDI mapping: DEFAULT and CUSTOM

Open **SEQ → MIDI MAP** on any DRUM-X, DRUM-LAB, DRUM-AC or DRUM-IND track. The **DEFAULT / CUSTOM** indicator at the upper right describes the selected track's MIDI mode:

- **DEFAULT:** the 12 factory pads repeat in every octave (for example C at MIDI 36, 48, 60, 72 and 84 triggers the same drum). The incoming octave does not transpose the drum sound. 
- **CUSTOM:** changing any pad note with **K1** activates an exact, **per-track** 12-pad map. The edited pad sounds only on its assigned note; all other pads retain their original MIDI-note assignments but no longer repeat in other octaves. Multiple pads may intentionally share a note and sound together.
- **RESET MIDI MAP:** hold the physical **SEL button** (not the SELECT rotary knob) for about **0.7 seconds** on SEQ → MIDI MAP. The confirmation applies only to the selected track: **OCT− cancels**, **OCT+ resets** that track to DEFAULT and restores octave repetition. Sound parameters, pan/pitch/timing edits and the other tracks are unchanged.

Custom MIDI maps are saved with the project. Returning the visible note values to their factory numbers does **not** exit CUSTOM mode; use **RESET MIDI MAP** to restore DEFAULT.

## Universal live timing

Adjust **GLO → MIX → TUNING → K3 DLY** to delay the selected track's note onset by **0–50 ms** on **every engine** (melodic and drum). This makes layered sounds hit at slightly different times while sharing the same MIDI channel. The control affects note triggering, not the already-generated audio; melodic Note On and Note Off use the same captured delay, preserving note duration.

For **DRUM-X, DRUM-LAB, DRUM-AC and DRUM-IND**, select an individual pad in **SEQ → MIDI MAP**, then adjust **K4 DLY** for an additional **0–50 ms**. The track and pad values are **additive**: a 12 ms track delay plus a 7 ms pad delay triggers that pad **19 ms** late. Both default to **0 ms** and are saved with projects. Existing drum MIDI note, pan and pitch controls are unchanged.

## Preset banks and Web Library

**USER** stores 32 editable sounds; **LIBRARY** stores 96 factory-backed positions, all replaceable through the browser. **SAVE → PRESETS → BANK** selects ALL, USER or LIB. Projects store slot references; save edited sounds to USER before saving projects.

The [Web Library](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/) opens on BACKUP. Click **Connect FM1** to read and verify the device automatically. In USER, LIBRARY and PROJECTS, drag individual or Ctrl/Shift-selected groups, swap positions and rename directly in the list. Review the pending changes and click the corresponding **Write** button. Build your own offline collections: import, open, remove and rename sounds, then save a named JSON file. The same sound collection works in both USER and LIBRARY. Save a complete backup or a complete performance, including pending changes, whenever needed. See the [Web Library guide](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/guide.html) and [user manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/?lang=en#en-7-preset-management-and-web-library).

The offline melodic catalog contains 20 ANALOG presets (including CICERO and PLUTARCH), 12 INDUSTRIAL, 5 BRASS and 8 ATMOS, alongside custom PHASE, WHEEL, NOISE, VOICE, LOFI and FM6 sounds. The four drum engines retain their synthesis and have renamed factory kits: **BUTTLER-1–9** (DRUM-X), **ZIZEK-1–3** (DRUM-LAB), **KRIPKE-1–3** (DRUM-AC) and **BADIOU-1–3** (DRUM-IND).

Many factory presets are named after philosophers and thinkers from different periods of history.

## Made for live performance

**MASHINA puts the FM1 under your hands instead of behind menus.** Use a MIDI keyboard or pad controller to play several tracks at once, build splits and layers, and change the sound as you perform. Assign knobs, sliders or pedals to the parameters that matter to you, then reshape tones, trigger variations and balance tracks without interrupting the music.

**Your setup stays flexible from song to song.** Combine melodic and percussion engines, give each track its own MIDI channel and playing range, and save complete configurations as projects for quick recall on stage. The mixer, streamlined controls and panic shortcut keep essential performance functions close at hand. MASHINA works equally well as the centerpiece of a compact MIDI rig or as an additional sound module in a larger setup.

## Install or try in a browser

Use the **[FM1 web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/)** to install MASHINA with a compatible desktop browser, or **[try the emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/)** without flashing your device. The **[web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/)** supports live sound editing; use Web Library for complete device backups and preset/project management.

**Installation:** flash MASHINA using the web installer and keep FM1 powered until it starts. Installing from another firmware or an incompatible older MASHINA version (including v0.14) clears presets, projects and settings automatically. Save anything you want to keep first. Compatible updates retain existing data. No separate initialization is required.

**Current version:** v0.15.

## Credits and licensing

**MASHINA:** Misha Mashina.

**JIANT:** Juan Ignacio Antonucci (Juan Jiant) — [source repository](https://github.com/juanjiant-bit/JIANT) and [project/manual](https://juanjiant-bit.github.io/JIANT/).

**Felucca:** Hügelton Instruments, including Leo Kuroshita and other contributors. Additional credits are preserved in source headers, firmware ABOUT information and bundled license notices.

Distributed under **[GPL-3.0-only](LICENSE)**. Original author credits and third-party licenses are retained.
