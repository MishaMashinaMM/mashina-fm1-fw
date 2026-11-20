# MASHINA — FM1 Performance Firmware v0.10

**MASHINA** turns the **M-VAVE FM1** into a six-track sound module designed for **external MIDI keyboards, pad controllers and live performance**. Play multiple sounds together, split a keyboard into zones, shape sounds while performing and recall projects quickly.

Developed by **Misha Mashina**, based on [JIANT by Juan Ignacio Antonucci (Juan Jiant)](https://github.com/juanjiant-bit/JIANT) and [Felucca](https://github.com/hugelton/Felucca). Free software under **GPL-3.0-only**.

![MASHINA FM1 Performance Firmware — MIDI routing, synths, drums and live controls](docs/mashina-overview.svg)

### MASHINA online

[Project homepage](https://mishamashinamm.github.io/mashina-fm1-fw/) · [Play in emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/) · [FM1 web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/) · [Web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/) · [User manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/)

## What MASHINA adds to JIANT

MASHINA retains JIANT's sound-making capabilities while expanding the FM1 for hands-on MIDI performance:

| Feature | What's included |
| --- | --- |
| **Additional melodic engines** | **ATMOS** for evolving pads, **BRASS** for synthetic wind sounds and **INDUSTRIAL** for aggressive basses and leads. |
| **Expanded drums** | **DRUM-X** with 12 sounds, plus three additional 12-sound kits: **DRUM-LAB** (electronic percussion), **DRUM-AC** (acoustic-style drums) and **DRUM-IND** (metallic, mechanical and industrial effects). |
| **Flexible MIDI routing** | Assign independent MIDI receive channels to six tracks; layer sounds, build keyboard splits and set note and velocity zones. |
| **Track tuning** | Transpose each track by up to three octaves and adjust fine tuning independently. |
| **Live MIDI control** | 32 assignable MIDI CC macros, MIDI Learn, performance commands and MIDI-triggered sound variations. |
| **Live mixer and sound shaping** | Six-track mixer with note activity indicators, individual track controls and shared three-band EQ. |
| **Project management** | Save and clear project slots, retain performance settings and use quick project recall during a set. |
| **Performance interface** | Streamlined, animation-free orange-and-gray UI, repurposed hardware controls and panic mode for immediate silence. |

MASHINA has **eight shared synthesis voices in total**, not eight per track. It is focused on live playing; the original song sequencer is disabled.

For drum note layouts, sound parameters, MIDI mappings, hardware controls and complete operating instructions, see the **[User Manual](https://mishamashinamm.github.io/mashina-fm1-fw/manual/)** (available in English, Serbian, Spanish, Russian, Japanese and Hindi).

## Install or try in a browser

Use the **[FM1 web installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/)** to install MASHINA with a compatible desktop browser, or **[try the emulator](https://mishamashinamm.github.io/mashina-fm1-fw/emu/)** without flashing your device. The **[web editor](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/editor/)** supports device management and backup/restore.

**Before flashing:** back up projects, user presets and settings, and keep a known-working recovery firmware available. Projects saved with newer firmware may not be compatible with older versions.

**Current version:** v0.10 · **FM1 firmware identity:** `FM-1_910`.

## Credits and licensing

**MASHINA:** Misha Mashina.

**JIANT:** Juan Ignacio Antonucci (Juan Jiant) — [source repository](https://github.com/juanjiant-bit/JIANT) and [project/manual](https://juanjiant-bit.github.io/JIANT/).

**Felucca:** Hügelton Instruments, including Leo Kuroshita and other contributors. Additional credits are preserved in source headers, firmware ABOUT information and bundled license notices.

Distributed under **[GPL-3.0-only](LICENSE)**. Original author credits and third-party licenses are retained.
