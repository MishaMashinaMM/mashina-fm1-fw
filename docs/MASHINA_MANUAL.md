# MASHINA — FM1 Performance Firmware User Manual

MASHINA turns the M-VAVE FM1 into a **six-track sound module for external MIDI keyboards, pads and controllers**. This guide explains how to select sounds, route MIDI, build splits and layers, control synthesis and drums, and save your live setup.

## 1. Getting around the FM1

| Control | What it does |
| --- | --- |
| **HOME** | Return to the home/sound view. **Hold HOME** to open the settings menu. |
| **GLO** | Mixer, MIDI zones, tuning, sends, global EQ and system settings. |
| **EDIT** | Edit the selected track's sound engine. |
| **ENV** | Envelope and modulation destinations. |
| **LFO** | LFO, modulation and motion controls. |
| **FX** | Track effects and global effect buses. |
| **SCL / ARP** | Scale/chord and arpeggiator settings. |
| **SAVE** | Store sounds, presets and projects; manage the library. |
| **SEQ** | Drum MIDI note, pan, pitch and timing mapping. It does **not** start a song sequencer. |
| **PLAY** | Drum performance settings: velocity, choke and retrigger. |
| **REC** | Live MIDI CC macros, up to 32 per project. |
| **ALGORITHM** | Select a track (except on pages where it has a dedicated action). |
| **SELECT** | Move between the pages in the current section; on drum pages, select a pad. |
| **K1–K4** | Edit the four corresponding controls, where available. |
| **PRESETS** | Browse sounds, except when a page gives it another role. |
| **OCT− / OCT+** | Change the FM1 keyboard's playing octave. Together they reset the octave offset. |

On most sections, **press a section button once** to open it, turn **SELECT** to browse pages, and turn the relevant **K** knob to edit a displayed value. The currently selected track is shown in the interface. Settings changed on a track normally take effect immediately; **save your project** to keep them.

### Settings menu

**Hold HOME**, then turn **SELECT** to navigate between groups. The menu pages are:

| Page | Controls |
| --- | --- |
| **SCREEN** | Scope, screen sleep, LED glow, scale LEDs |
| **AUDIO** | Speaker EQ, active tracks (4/6), load, BPM lock |
| **MIDI** | MIDI output, MIDI Learn, clear learned controls |
| **USB** | USB audio, USB level |
| **CONTROLS** | Hold time, knob speedup, FX latch, calibration |
| **SYSTEM** | Restore last, CPU cores, CPU clock, ABOUT |

Use the displayed K knob to adjust a setting. HOME exits the menu. **LEARN CLEAR** removes MIDI mappings; confirm only if you intend to erase them.

## 2. External MIDI and the six-track mixer

Connect a compatible USB or TRS MIDI source. Open **GLO → MIX → MIXER**.

| Control | Function |
| --- | --- |
| **K1** | Receive (RX) MIDI channel for the selected track, **1–16** |
| **K2** | Track volume |
| **K3** | Track pan |
| **K4** | Not assigned on MIXER |
| **PRESETS** | Master DJ Filter, on MIXER only |

Use **ALGORITHM** to select T1–T6. Tracks may share a MIDI channel: this is how you layer several instruments from one keyboard. You can set RX channels for T5/T6 even when only four tracks are active.

Select the active track count under **HOME (hold) → AUDIO → TRACKS**.

### MIDI routing modes

Open **GLO → SYSTEM → ROUTE**:

- **1-4** — route incoming MIDI to matching assigned channels on tracks 1–4.
- **1-6** — route to matching channels on tracks 1–6 (subject to active track count).
- **SEL** — play the currently selected track.

For example, set T1 and T2 both to **RX 1**, use **ROUTE = 1-6**, and send notes on MIDI channel 1. Both tracks respond, each with its own sound and pan. Save the project to retain assignments.

### Note and velocity zones

Open **GLO → MIX → ZONES**. For each selected track:

| Knob | Value | Range |
| --- | --- | --- |
| **K1** | MIN — lowest accepted MIDI note | 0–127 |
| **K2** | MAX — highest accepted MIDI note | 0–127 |
| **K3** | V.MIN — lowest accepted velocity | 1–127 |
| **K4** | V.MAX — highest accepted velocity | 1–127 |

Both endpoints are inclusive. **Non-overlapping note ranges** create keyboard splits. **Overlapping note ranges** make layers. Different velocity ranges create soft/hard playing layers.

Example: Give T1 the lower keyboard range and T2 the upper range, while both use RX 1. Or give both the same note range with T1 velocity 1–80 and T2 velocity 81–127.

**Important:** Zones filter incoming **external** MIDI; the FM1's built-in mini-keyboard still addresses only the selected track. Zones are evaluated before transposition.

### Per-track transpose, fine tuning and timing

Open **GLO → MIX → TUNING**:

- **K1 TRN:** −36 to +36 semitones (three octaves in either direction).
- **K2 FINE:** −50 to +50 cents (subtle detuning).
- **K3 DLY:** universal per-track note-onset delay, 0–50 ms, including melodic engines. On drum tracks it adds to the selected pad's K4 delay.
- **K4:** reserved.

Use TRN to shift one layer up or down and FINE to thicken two sounds played in unison. A note already sounding keeps its current pitch when TRN changes; later notes use the new tuning. Note Off and sustain still release the correct voices. DRUM-X and DRUM-LAB retain their drum pitches and MIDI note maps regardless of TRN/FINE.

## 3. Sound engines and polyphony

Select the track, choose a sound/engine with the preset controls, then open **EDIT**. Turn **SELECT** to access the available EDIT pages; K1–K4 adjust the labels shown on each page. Use ENV, LFO, FX, SCL and ARP for further shaping.

### Existing JIANT sound engines

These engines are available alongside MASHINA's additions. Each has its own factory presets and EDIT parameters.

| Engine | Sound and main controls |
| --- | --- |
| **ANALOG** | Classic subtractive sounds: warm basses, sharp leads, plucks and pads. Mix oscillators, detune them, then use drive and the filter for tone shaping. |
| **PHASE** | Phase-distortion synthesis for brassy stabs, glassy bells, resonant keys and growling bass. DCW and its envelope reshape the waveform while the note plays. |
| **LOFI** | Chip-style tones, wave-table sounds and bytebeat textures, from clean pulse leads to gritty game-console effects. Explore CHIP, WAVE, DUTY, sweep and bit reduction. |
| **VOICE** | Sung vowels, choirs, speech-like leads and robot-voice sounds. VOWEL, VOWEL2 and TALK move between formants; some presets articulate words. |
| **WHEEL** | Tonewheel-style additive organ with drawbar registrations, percussion, key click, drive and rotary-speaker color. Useful for organs, reedy bass and sustained chords. |
| **NOISE** | Filtered noise, crackles, wind, rain, metallic hits and retro shift-register effects. MODE chooses the noise character; COLOR, FREQ and PEAK shape the result. |
| **FM6** | Six-operator FM for electric pianos, bells, mallets, basses, complex pads and digital leads. Choose factory patches first; algorithms and operator envelopes provide deeper editing. |

**DRUM-X** is the expanded version of JIANT's drum engine. It is described together with the other drum kits in section 4.

### Additional MASHINA melodic engines

**ATMOS — moving pads and evolving textures.** Start with **KANT** for a subtle layered pad, **BURKE** for slow swells or **JEFFERSON** for a brighter texture. Its basic EDIT controls include **WAVE** (oscillator shape), **MORPH**, **DENSITY**, **DRIFT**, **CUT**, **RES**, **TEXTURE** and **SHIMMER**. The additional **BREATH** control adds volume movement, **MOTION** sets its pace, **FLOW** moves the tone over time and **SPREAD** separates the oscillators. Increase these gradually to keep stacked chords clear.

**BRASS — playable synthetic sax, horn and trumpet.** This engine is intentionally electronic rather than a realistic acoustic imitation. **TYPE** sweeps between sax-like (**0**) and trumpet-like (**127**) character, with hybrid sounds between them. **PRESS** changes playing intensity, **BUZZ** adds a reed/lip-like edge, **BREATH** adds air, and **BODY/FORMNT** control resonant color. Use **GROWL** for rougher tone and **ATTACK** for the start of a note. A third EDIT page provides **FAT**, **GRIND**, **METAL** and **CRUSH** for detuned or aggressive synthetic brass. Try **HUME**, **DIDEROT** or **HOLBACH** as starting points.

**INDUSTRIAL — powerful melodic basses and leads.** Unlike the DRUM-IND percussion kit, this engine follows the notes you play. **WAVE** chooses the main oscillator; **FAT** thickens it, **GRIND/METAL/CRUSH** add increasingly hard-edged color, and **FEED** introduces additional grit. **CUT/RES** shape the filter. On the third EDIT page **WAVE2**, **MIX**, **SUB** and **MODE** combine a second oscillator and low-end weight. For tight bass begin with **PASCAL** or **NEWTON**; for sharp leads try **CAVENDISH** or **LOCKE**.

MASHINA has **eight voices shared across all active tracks and drum engines**. Dense layered chords can exhaust the voice budget. BRASS also has a four-voice-per-track limit. Simplify layers if notes are being cut off.

### Factory sounds in v0.14

The refreshed factory bank includes 20 ANALOG, 12 INDUSTRIAL, 5 BRASS and 8 ATMOS sounds, plus updated PHASE, WHEEL, NOISE, FM6, VOICE and LOFI banks. Many presets are named after philosophers and thinkers from different periods of history. DRUM-X uses BUTTLER-1…9, DRUM-LAB ZIZEK-1…3, DRUM-AC KRIPKE-1…3 and DRUM-IND BADIOU-1…3.


## 4. Drum engines and live pad controls

### DRUM-X: 12-pad percussion engine

**DRUM-X** contains twelve independent sounds. **B2, S2, O2 and T2** are individually editable pads with the same controls as their matching drum sounds.

| Pad | MIDI note | Pad | MIDI note |
| --- | --- | --- | --- |
| BD | 36 | RM | 37 |
| SD | 38 | BL | 47 |
| CP | 39 | B2 | 41 |
| CH | 42 | S2 | 40 |
| OH | 46 | O2 | 43 |
| TM | 45 | T2 | 44 |

Select a DRUM-X track and open EDIT to adjust kit, individual sound controls and **LEVELS 1–3**, covering all twelve pads.

### DRUM-LAB: electronic percussion engine

**DRUM-LAB** also provides twelve individually editable synthesized pads, initially MIDI notes **36–47**. In EDIT, use:

| Page | What you can edit |
| --- | --- |
| **STRIKE** | Kit-wide PITCH, SWEEP, BODY and SNAP |
| **BODY** | Kit-wide NOISE, TONE, DRIVE and WIRES |
| **SOUND 1–4** | Selected pad's synthesis parameters and type |
| **LEVELS 1–3** | Individual levels for all twelve pads |

Kit-wide multipliers scale individual pad settings without replacing them. At 100% they are neutral; reducing a multiplier scales that aspect of the sound.

### DRUM-AC: acoustic-style 12-pad engine

**DRUM-AC** is a third, separately synthesized drum engine (no samples). Its twelve pads use acoustic-style impact, tom-head and metallic-plate models. The DRUM-AC kit has three presets: **KRIPKE-1**, **KRIPKE-2** and **KRIPKE-3**.

**Chromatic layout, repeating in every octave:** each note name plays the same pad in any octave. 

| Key | Pad | Key | Pad |
| --- | --- | --- | --- |
| C (36) | Kick | F♯ (42) | Closed hi-hat |
| C♯ (37) | Crash | G (43) | Low tom |
| D (38) | Snare | G♯ (44) | Ride splash |
| D♯ (39) | Clap | A (45) | Mid tom |
| E (40) | Ride bell | A♯ (46) | Open hi-hat |
| F (41) | Floor tom | B (47) | High tom |

The MIDI numbers in parentheses are examples in the **36–47** octave. Every note 12 semitones above or below selects the same pad. A closed-hi-hat hit chokes an open-hi-hat tail even across octaves, but does not choke crash or ride. To reassign the notes for individual pads, use SEQ → MIDI MAP.

**EDIT pages on a DRUM-AC track:**

| Pages | Controls |
| --- | --- |
| **First two kit-wide EDIT pages** | **TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL**. Eight independent effects across all twelve drums, **0–200%**, with **100% neutral**. They do not modify individual pad values. |
| **SOUND 1–3** | Turn **K1** to select a pad; use **K2–K4** for its own eight **0–127** parameters, spread over three pages: TUNE, DAMP, BODY, HIT, instrument-specific strike, BRIGHT, instrument-specific character, RING/DECAY/TAIL. |
| **LEVELS 1–3** | Independent pad volumes **0–127** for all twelve instruments. |

The per-pad labels depend on the chosen instrument: **BEATER** and **TEXTURE** (kick); **WIRES** and **TEXTURE** (snare); **SPLASH** and **METAL** (crash); **CHICK** and **DECAY** (hi-hats); **STICK** and **SHELL** (all four toms); **SNAP**, **TEXTURE** and **TAIL** (clap); **STRIKE** (ride bell); **SPLASH** (ride splash). Unlike the kit-wide controls, an individual field may have a different sonic function on different pads.

**Tom tuning:** each tom is tuned separately. **TUNE = 0** disables pitched drumhead resonance (untuned thud); **1–127** introduces a clear head resonance that rises from low to high pitch, leaving the noise-based impact intact. **Crash/ride SPLASH** controls the noisy attack and cymbal wash.

The twelve individual sounds, pad levels and eight kit-wide settings are retained with the project. DRUM-AC uses the same **eight shared synthesis voices** as the other engines, not an independent twelve-voice sampler. The cymbal sounds are synthesized acoustic approximations, not recorded cymbals.


### DRUM-IND: non-tonal industrial percussion

**DRUM-IND** is a fourth, independent, sample-free 12-pad engine, separate from the melodic **INDUSTRIAL** synthesizer. It uses noise excitation, moving resonant filters, rough transients and irregular mechanical pulses rather than tuned note oscillators. It is intended to layer with acoustic or electronic drum kits using the same incoming MIDI channel.

**Default chromatic layout — repeats every octave:** the MIDI numbers below are examples from 36–47.

| Key (MIDI) | Sound / character | Key (MIDI) | Sound / character |
| --- | --- | --- | --- |
| C (36) | **CLANG** — metal impact | F♯ (42) | **BUZZ** — abrasive electrical buzz |
| C♯ (37) | **ANVIL** — heavy hammer | G (43) | **LIGHTNING** — crack and rolling noise |
| D (38) | **METAL** — sheet-metal resonance | G♯ (44) | **ENGINE** — mechanical chug |
| D♯ (39) | **SCRAPE** — metal friction | A (45) | **WHOOSH** — air sweep |
| E (40) | **WHISTLE** — noisy jet whistle | A♯ (46) | **MOTOR** — rotary mechanical noise |
| F (41) | **SCREECH** — unstable squeal | B (47) | **EXPLOSION** — blast and turbulent tail |

**EDIT structure:** the first two pages control the whole kit: **TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL** (each **0–200%**, **100% neutral**). ATTACK changes roughly the first 80 ms of the hit; BRIGHT changes its high-frequency balance after the sound's limiting. The individual SOUND 1–3 pages use **K1** for pad selection and **K2–K4** for the eight **0–127** parameters **GRAIN, DECAY, WEIGHT, STRIKE, MOTION, AIR, CRUNCH, TAIL**. **LEVELS 1–3** offer twelve independent pad volumes (**0–127**). The same parameter may act differently on different models.

Three presets: **BADIOU-1**, **BADIOU-2**, **BADIOU-3**. Pad edits, levels and independent kit-wide settings save with the project. DRUM-IND shares the firmware's **eight total voices**. 


### SEQ: drum MIDI mapping

Press **SEQ** on a drum track. **ALGORITHM** selects the track, **SELECT** selects the pad, **OCT−/OCT+** switch pad banks.

| Knob | Effect |
| --- | --- |
| **K1** | Pad MIDI note, 0–127 |
| **K2** | Pad pan |
| **K3** | Pad pitch offset, −24 to +24 semitones |
| **K4** | Extra pad onset delay, 0–50 ms |

**DEFAULT / CUSTOM — all four drum engines.** In **DEFAULT** mode DRUM-X, DRUM-LAB, DRUM-AC and DRUM-IND repeat all 12 pads across every octave: C at MIDI 36, 48, 60 and 72 triggers the same sound without transposing it. 

Changing any pad's MIDI note using **K1** activates **CUSTOM mode for the selected track only**. All 12 pads then respond exclusively to their exact assigned MIDI notes (0–127). Unchanged pads keep their original notes in 36–47 but stop repeating in other octaves. Multiple pads can share a note and play simultaneously. **DEFAULT / CUSTOM** appears in the screen's upper right.

To return to DEFAULT, hold the **physical SEL button** (not the rotary SELECT encoder) for about **0.7 seconds** to display **RESET MIDI MAP?**. Press **OCT− to cancel** or **OCT+ to reset only the selected track**. This restores octave repetition without altering sound parameters, pitch/pan offsets or other tracks. Returning MIDI note values manually to their defaults does not exit CUSTOM mode; use RESET. Custom maps are saved with projects. **K2 PAN** sets the selected pad's stereo position (L64–C–R64), and **K3 PITCH** adjusts its tuning by −24 to +24 semitones, on all four drum engines. On DRUM-AC and DRUM-IND, non-tonal sounds change spectral resonance/character rather than acquiring a melodic pitch. **K4 DLY** adds 0–50 ms to that pad's trigger. **GLO → MIX → TUNING → K3 DLY** applies another 0–50 ms to every drum pad on that track; both values are additive (for example 12 ms + 7 ms = 19 ms). They default to zero and are saved with the project. The track delay also applies to every melodic sound engine: for example, ANALOG at 0 ms, BRASS at 12 ms and ATMOS at 25 ms can be layered on one MIDI channel. Melodic Note On and Note Off are shifted by the same captured amount, preserving note duration; MIDI sustain ownership remains unchanged. Timing never plays early and does not buffer rendered audio. Existing FLAM behavior remains independent.


### PLAY: drum performance

Press **PLAY** on a drum track. Pad and bank selection work as on SEQ.

| Knob | Effect |
| --- | --- |
| **K1** | Velocity curve: LIN / SOFT / HARD / FIX |
| **K2** | Velocity response, 0–127 |
| **K3** | Choke group: none or G1–G4 |
| **K4** | OVR (overlap) / RST (restart) retrigger |

Pads in a common choke group can silence each other. Use RST if you want each hit to restart its pad rather than overlap the previous hit.

## 5. MIDI CC macros and MIDI Learn

Press **REC** to open MASHINA's project-specific macro table. A project can store **32 mappings** from external MIDI CC/channel messages to track/engine parameters.

### Create a macro

1. Press **REC**, then **OCT+** to add a mapping.
2. Send a MIDI CC from your external controller. Its number and channel are captured.
3. Navigate to the destination track and editable sound parameter.
4. Turn the matching K knob once to select the destination.
5. Return to REC to edit endpoints and save the project.

| REC control | Action |
| --- | --- |
| **SELECT** | Select a mapping, scrolling in four-row pages |
| **ALGORITHM** | Enable/disable selected mapping |
| **K1** | RNG (continuous range) or TGL (two-value toggle) |
| **K2/K3** | Set minimum/maximum or toggle A/B, 0–127 |
| **K4** | Reverse input mapping |
| **OCT−** | Delete prompt; **OCT+** confirms, OCT− cancels |

The table shows **Y** for enabled, **N** for disabled, and **S** when a mapping is temporarily suspended because its target engine is absent. Such a mapping resumes when the target engine is selected again.

The captured **CC, MIDI channel and destination** cannot be changed in place; delete and recreate that mapping. Several mappings may share the same CC and channel to control different parameters at once. **Save the project** to retain changes.

**MIDI Learn** is a separate global feature with 32 learned destinations. Enable it through **HOME (hold) → MIDI → LEARN**, choose a parameter and move a controller. Its assignments are separate from project-specific REC macros.

### Dedicated MIDI CC shortcuts

These commands operate independently of the REC macro table:

| Command | Action |
| --- | --- |
| **CC88** | Temporarily switch T1 receive channel |
| **CC89** | Temporarily switch T2 receive channel |
| **CC90** | Temporarily switch T3 receive channel |
| **CC85** | Lightly randomize melodic sound parameters |
| **CC86** | Strongly randomize melodic sound parameters |
| **CC87** | Restore the current sound as it was before CC85/86 |

**CC88–90:** send value **0–63 for RX channel 2**, or **64–127 for RX channel 1**, on any incoming MIDI channel. This does not overwrite the project assignment. Loading a project restores the saved channels.

**CC85–87:** use incoming **channel 13 for T1, 14 for T2, 15 for T3**. Send **64–127** to trigger; send **0–63** to re-arm the command. CC85/86 vary the **currently loaded sound** on the addressed track: factory presets, user presets and sounds loaded from projects all work, without first saving them. The first randomization captures the current settings; later variations use the same starting point. CC87 restores that starting sound without interrupting playing notes. Loading another preset or changing a sound engine establishes a new starting point. All melodic engines, including ATMOS, BRASS, INDUSTRIAL and FM6, are supported; **DRUM-X, DRUM-LAB, DRUM-AC and DRUM-IND are excluded**. Waveform and patch selectors remain protected.

## 6. Global EQ, effects and project management

### Shared three-band EQ

Open **GLO → MIX → EQ**:

| Band | Frequency | Range |
| --- | --- | --- |
| LOW | 50 Hz | −15 to +15 dB |
| MID | 700 Hz | −15 to +15 dB |
| HIGH | 10 kHz | −15 to +15 dB |
| ON | EQ enable / bypass | — |

This is a **shared EQ before effects**, not one EQ per track. Save the project to retain its settings. Use FX and the track sends to shape the rest of the signal.

### Saving and loading

Use **SAVE** for edited sounds, project slots and the library. A project stores MIDI assignments, zones, tuning, macros, preset references and relevant effects. If you load a different project without saving, unsaved adjustments may be lost.

**SAVE → LIBRARY → TOOLS** provides four actions:

| Control | Action |
| --- | --- |
| K1 | INIT SOUND |
| K2 | CLEAR SLOT — permanently erase the selected project slot after confirmation |
| K3 | NEW PROJECT |
| K4 | CPU TEST |

### Live Recall

**Long-press REC** to enable Live Recall. Then tap **FX**, **SEL**, **ENV**, or **LFO** to recall saved project slots **A**, **B**, **C**, or **D**. Long-press REC again to leave Live Recall mode. Previously sounding notes stop when you change sections.

### Panic mode

**Hold GLO and press OCT−** to immediately stop sounding tracks and effects and clear held-note and sustain state. The display shows PANIC MODE while GLO is held. This action does **not** erase the project.

### Backup and restore



### 64 fixed preset positions

| Bank | Positions | MIDI Program Change (0-based) | Storage |
| --- | --- | --- | --- |
| USER | 001–032 | 0–31 | Edit and save sounds on FM1; replace, move or export them in Web Library |
| LIBRARY | 033–064 | 32–63 | 32 factory-backed sounds; replace or restore factory defaults in Web Library |

Open **SAVE → PRESETS**. K4 selects **BANK: ALL / USER / LIB**; K1 and the PRESETS encoder navigate positions; K2 filters engines, K3 favorites. USER positions stay numbered even when empty. LIBRARY sounds are loaded and played like USER sounds, but saving an edited sound requires a USER slot. The offline factory catalog contains 107 sounds, including complete FM6 voices and all four drum-kit types; choose any of them for a device slot. To keep edits to a sound, save the sound to USER first.

### Eight projects, four sections each (A–D)

Projects store references to the numbered presets rather than copies of sounds. Each of the eight projects has four sections, A–D, with up to six track references and independent performance settings (MIDI routing, zones, tuning, delays, mixer, effects, macros). Replacing a sound in a numbered position changes it wherever that position is referenced. Save an edited sound to USER before saving a project; unsaved sound edits are not embedded in the project. If you need two versions of a sound, save them to two distinct USER slots. **REC (hold) + FX / SEL / ENV / LFO** recalls A / B / C / D of the selected project. Clearing a referenced USER slot in Web Library is blocked if it would leave a missing sound.

### Web Library: manage sounds, projects and complete performances

Open the [MASHINA Web Library](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/) in desktop Chrome or Edge; connect via USB MIDI and stop playing during transfers. The left side contains device positions; the right side is an independent offline collection.

1. In **BACKUP**, click **Backup complete FM1**, keep the downloaded JSON and select it under **Verify saved complete backup**. This unlocks writes for the session. After successful transfers the new verified backup is downloaded automatically; no repeated manual verification is needed. A failure, unexpected device change or disconnection locks writes again.
2. In **USER** or **LIBRARY**, import JSON, search and filter the source list, or choose factory/device sounds. Drag onto a numbered destination to replace it; drag between device slots to move or swap. LIBRARY overrides may be reset to their factory sound. The queue lists changes and affected project references. Applying changes to currently loaded sounds reloads the current performance. Stop playing before applying.
3. Click one row, Ctrl-click to toggle individual items, Shift-click to select a range, Ctrl+A to select all matching source items (even on other pages) or occupied device positions, Escape to clear. Drag any selected item to place a group in consecutive destinations. If it cannot fit, nothing is staged. Device groups swap displaced items into vacated positions. Drag selected device rows into the collection and **Save collection** to keep them in JSON. Collections may contain thousands of sounds or projects, independent of the FM1 capacity.
4. In **PROJECTS**, drag complete A–D projects or individual A–D chips into the destination project. References are numerical: ensure the corresponding sounds are installed. The saved project collection does not embed those sounds.
5. In **BACKUP**, **Save complete performance** includes USER, LIBRARY overrides, all eight projects, current performance and settings, including staged edits. Open a complete file offline to arrange it without FM1; later use **Restore complete FM1** to restore it. Only explicit Apply/Restore actions write to the device.

See the [Web Library guide](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/guide.html) for backup/restore details. A preset or project collection is not a full device backup. The USB device now appears as **MASHINA**; reselect MIDI ports in your DAW if necessary.

### First installation and updates

MASHINA v0.14 uses a new storage layout. Previous firmware's presets, projects and settings are not migrated; back up anything you want to keep before flashing. Use the [installer](https://mishamashinamm.github.io/mashina-fm1-fw/webapp/installer/), then [Initialize storage](https://mishamashinamm.github.io/mashina-fm1-fw/web-library/initialize.html) only if the device reports that first installation is required. Confirm the one-time erase, reboot, then open Web Library. Already initialized 32+32 devices must not be initialized again; compatible updates retain their data. The installer may require official V15 as an intermediate step when the current firmware's update protocol is not supported.

## 7. Example live configurations

**Two synths in unison:** T1 and T2 on RX 1, overlapping full-note zones; put a different engine on each. Adjust volume/pan on MIXER and use FINE to detune one layer.

**Split keyboard:** Set two tracks to RX 1, but put their ZONES note ranges on separate halves of the keyboard. The external keyboard then plays the appropriate instrument in each region.

**Velocity layers:** Use the same RX channel and note range on two tracks. Set T1 velocity 1–80 and T2 velocity 81–127 to swap timbres according to playing strength.

**One knob, several sounds:** Add two or more REC macros with the same incoming CC/channel but different track destinations. Change range/reverse independently for each row.

**External drum pads:** Assign a drum engine to a track, set its RX channel on MIXER, map incoming notes with SEQ and adjust curves, choke and retrigger in PLAY.

---

MASHINA v0.14 · M-VAVE FM1 · Usage reference
