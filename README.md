# JIANT LIVE DRUMS v0.9

**Performance-focused firmware for the M-VAVE FM1**, modified by **Misha Mashina** from **JIANT by Juan Jiant** (based on Felucca).

[![License: GPL-3.0-only](https://img.shields.io/badge/license-GPL--3.0--only-777777.svg)](LICENSE)
[![UI v0.9: FM1 tested](https://img.shields.io/badge/v0.9_UI-FM1_device_tested-ff9900.svg)](#v09-orange-ui-device-tested)

![JIANT LIVE DRUMS v0.9: six-track zones, transpose ±36 semitones and fine tuning ±50 cents, BRASS and INDUSTRIAL, two 12-pad drum engines and 32 MIDI macros](docs/jiant-live-drums-overview-32-macros.svg?v=0.9-ui)

**[Project homepage](https://mishamashinamm.github.io/jiant-live-drums/)** · **[Emulator](https://mishamashinamm.github.io/jiant-live-drums/emu/)** · **[Drums installer](https://mishamashinamm.github.io/jiant-live-drums/webapp/installer/)** · **[Drums editor](https://mishamashinamm.github.io/jiant-live-drums/webapp/editor/)** · **[Interactive manual (inherited)](https://mishamashinamm.github.io/jiant-live-drums/manual/)**

### Panic Mode — device-tested (v0.9)

**Physical FM1 test passed on November 18, 2026.** Hold **GLO** to display the performance map, then tap **OCT−** while continuing to hold GLO. The screen shows **PANIC MODE** in white on black until GLO is released, then restores the previously displayed page. The action immediately silences all six tracks, including delay/reverb tails, and clears held-note and sustain state. It does not change the selected project, stored presets, MIDI macros or settings. **OCT− alone** retains its normal octave function. The owner installed and tested this feature successfully on a physical FM1 after emulator and automated regression checks. This is part of **v0.9**; there is no version bump.

### v0.9 orange UI — device-tested

**Physical FM1 test passed on November 18, 2026.** The owner successfully installed this orange-accent UI build on an FM1 and reported that everything tested worked correctly, including the updated interface. This follows the separate v0.9 MIDI Zones/TUNING test with a Korg X5D earlier the same day. This release remains **v0.9**, firmware identity **`FM-1_909`**, with no `rc1` suffix. The owner’s report confirms a successful installation and functional check, not exhaustive endurance or recovery testing.

The display now uses a **black / white / grayscale palette with orange `#FF9900`**. Editable values, page titles and breadcrumbs are orange; labels, backgrounds and gauge/intensity ramps remain grayscale; selected mixer tracks are white with black lettering. Occupied SAVE section chips are orange, empty ones dark gray, and saved projects have orange borders (current project white). The HOME-over-GLO rendering bug is fixed. The original UI animations remain disabled.

**Installer:** after the merged `main` firmware build and GitHub Pages deployment complete, use the [public Drums installer](https://mishamashinamm.github.io/jiant-live-drums/webapp/installer/). The [UI v0.9 test installer workflow](https://github.com/MishaMashinaMM/jiant-live-drums/actions/workflows/ui-changes-installer.yml) remains as the historical pre-merge build used for hardware verification. Back up FM1 projects, user presets and settings before updating.

**Physical test report:** the orange-accent v0.9 firmware flashed successfully to an FM1 on November 18, 2026, and the owner reported everything working properly. The release changes include the splash title, HOME ↔ GLO redraw fix, mixer selection contrast, editable-value/breadcrumb coloring, and occupied/empty SAVE slot colors. For a future update, continue checking audio, MIDI, recalls, persistence and display readability.

**[JIANT LIVE DRUMS v0.9 user manual](docs/JLD_MANUAL_V0.9.md)** — covers MIXER, ZONES and TUNING, independent per-track ±36-semitone transpose and ±50-cent fine tuning, splits/layers, MIDI routing, BRASS, INDUSTRIAL, EQ, live recall, both drum engines and REC macros. The separate interactive manual linked above is inherited from original JIANT and has not yet been fully adapted to Drums.

The links above are the GitHub Pages routes for this separate Drums repository. The repository includes the Drums-specific browser emulator, firmware installer, editor, and an inherited interactive manual. The inherited manual still describes some original sequencer behavior and is **not a fully updated Drums control reference**.

JIANT LIVE DRUMS is performance-oriented firmware for the FM1 with **six independently routed MIDI tracks**, expressive melodic engines, two 12-pad drum kits, live project recall and 32 project MIDI CC macros. **v0.9 adds a dedicated six-track TUNING page** with **TRN −36…+36 semitones** and **FINE −50…+50 cents**, moving TRN out of MIXER. **v0.8 introduced Keyboard Zones** for per-track note/velocity splits and overlaps; **v0.7 introduced BRASS and INDUSTRIAL**. Existing live channel overrides, global pre-effects 50 Hz / 700 Hz / 10 kHz three-band EQ and the 12-pad drum engines remain available. The sequencer is disabled; SEQ, PLAY and REC serve live controls instead.

> **v0.9 hardware testing (November 18, 2026):** The prior feature build was flashed and tested with a Korg X5D MIDI keyboard, including TUNING and existing features. Later the same day, the new black/gray/orange v0.9 UI build was flashed successfully to a physical FM1, and the owner reported all tested functions working. Automated firmware, MIDI, UI, project and WebAssembly checks also passed. Hardware validation is a functional test, not comprehensive stress, upgrade-path or recovery certification.

## What's different from JIANT / Jiant Live

| Feature | JIANT LIVE DRUMS v0.9 |
| --- | --- |
| **Panic Mode (v0.9, device-tested)** | Hold **GLO**, tap **OCT−**: immediately silence all tracks and effects, show white **PANIC MODE** on black while GLO remains held, then return to the previous screen on release. Does not modify project data or macros. Tested on a physical FM1 on November 18, 2026. |
| **Keyboard Zones (introduced in v0.8)** | **GLO → MIX → ZONES**, immediately after MIXER. Six identical track rows: static track number/name; **K1 MIN**, **K2 MAX** (note 0–127, shown as note names), **K3 V.MIN**, **K4 V.MAX** (velocity 1–127). Each track independently filters incoming USB/TRS MIDI after RX-channel selection and **before transpose**. Ranges are inclusive; overlapping note/velocity areas layer sounds. Saved **per project**; old projects default to full ranges. FM1 built-in keyboard still plays only the selected track. Confirmed on a physical FM1 with a Korg X5D in v0.9 hardware testing. |
| **BRASS engine (new in v0.7)** | Dedicated synth for synthetic saxophone and trumpet leads, with **continuous TYPE morph (0–127)** and PRESS, BUZZ, BREATH, BODY, FORMNT, GROWL, ATTACK. **EDIT 3** adds neutral-by-default FAT, GRIND, METAL and CRUSH. Six factory presets; up to four voices for this engine. |
| **INDUSTRIAL engine (new in v0.7)** | Heavy playable synth basses and leads. **EDIT 1** WAVE/FAT/GRIND/METAL, **EDIT 2** CRUSH/FEED/CUT/RES, **EDIT 3** WAVE2/MIX/SUB/MODE; six factory presets. Both engines share the existing global voice budget and effect routing. |
| **Per-track tuning (new v0.9 page)** | **GLO → MIX → TUNING → K1** adjusts **TRN −36 to +36 semitones**; **K2 FINE** adjusts **−50 to +50 cents** to enrich layers without changing MIDI note numbers. Both external MIDI and the FM1 keyboard transpose; **DRUM-X and DRUM-LAB remain unaffected by per-track TRN/FINE**. Values save per project; older projects default to zero. Note Off and sustain follow the original note owner, including if TRN changes while held. Device-tested November 18, 2026. |
| **MIXER and GLO navigation (v0.6)** | Pressing GLO from elsewhere opens **MIX → MIXER**, with **K1 MIDI RX, K2 volume, K3 pan, K4 unused**. The PRESETS rotary adjusts the master **DJ Filter on MIXER only**; it keeps its normal preset function on other pages. GLO has MIX (MIXER, ZONES, TUNING, SENDS, EQ), GLOBAL and SYSTEM groups; unused MANO/RIELES pages are absent from navigation. |
| Live MIDI channel selection (v0.5) | **CC88 / CC89 / CC90** select MIDI **CH1** (value 64–127) or **CH2** (value 0–63) for **T1 / T2 / T3** respectively, independently and from any incoming MIDI channel. Other tracks retain their existing RX assignments. **GLO → MIXER → K1** remains editable and clears only the selected track's temporary override. Tested in emulator and on physical FM1 (November 16, 2026). |
| Live MIDI sound randomization | **CC85** slight, **CC86** hard, **CC87** restore last saved sound parameters without cutting held notes. Dedicated MIDI channels **13/14/15** control **T1/T2/T3**, independent of track RX assignments. Successfully tested on a physical FM1 via TRS MIDI (November 15, 2026). |
| Live project recall | Long-press **REC** to enter Live Recall mode; **FX**, **SEL**, **ENV** and **LFO** then recall project slots **1–4**, respectively. Previous notes stop while delay/reverb tails decay naturally. Tested and approved on a physical FM1, as well as verified in the browser emulator. |
| CLEAR SLOT | **SAVE → LIBRARY → TOOLS → CLEAR SLOT** permanently clears the selected saved project slot, with confirmation. Live TOOLS displays four actions mapped directly to **K1–K4**: INIT SOUND, CLEAR SLOT, NEW PROJECT and CPU TEST. Tested and approved on a physical FM1; emulator and automated tests also passed. |
| Global 3-band EQ | **GLO → MIX → EQ**: LOW 50 Hz shelf, MID 700 Hz peak (Q=1), HIGH 10 kHz shelf; each −15 to +15 dB, plus ON/bypass. One shared EQ for all tracks, before effects, saved per project. |
| MIDI input per track | Independent **RX channel 1–16** for each of six tracks. Two or more tracks may share a channel and sound together. |
| MIDI routing | **1-4**, **1-6** (assigned-channel routing), or **SEL** (selected-track routing). TRACKS 4/6 still determines how many tracks are active. |
| MIDI LEARN | **32 parameter destinations** rather than 10. One incoming CC can control several parameters on different tracks at once; MIDI Learn remains available alongside macros. |
| Project MIDI macros | Up to **32** per project, with captured MIDI CC/channel and track-engine-parameter destination; RNG/TGL behavior, editable 0–127 endpoints, reverse, and independent ON/OFF. |
| Project receive channels | The six **GLO → MIXER** RX channel assignments are saved and loaded with each project (legacy projects retain existing channels). |
| DRUM-X | **12 independent sounds** over one chromatic octave, including extra B2, S2, O2 and T2 instances with the original editing controls. |
| DRUM-LAB | **12 independent synthesized pads** with selectable sound types, ten per-pad sound parameters, three LEVELS pages, and eight kit-wide proportional controls. |
| Repurposed SEQ, PLAY and REC buttons | **SEQ** opens **DRUM MIDI MAP**, **PLAY** opens **DRUM PERFORMANCE**, and **REC** opens **MIDI CC MACROS**. The three buttons illuminate with their active live pages; none starts the removed sequencer/transport. |
| Polyphony | **Eight shared voices** across all tracks and both drum engines. |
| Display (v0.9 UI, device-tested) | Static **JIANT LIVE DRUMS v0.9** startup splash; black/white/gray UI with **orange #FF9900** values and breadcrumbs, solid-white selected mixer rows, orange occupied SAVE slots and gray empty slots. Decorative animations remain disabled. |
| ABOUT | JIANT LIVE DRUMS identification and purpose, while retaining original author and license credits. |
| Browser emulator | Can load an existing JIANT **backup JSON** into an isolated session to audition saved sounds/projects without changing the physical FM1. |
| Sequencer / sound engines | **Sequencer disabled** for live use; the inherited melodic engines and effects remain. |

### Keyboard Zones: six independent note and velocity ranges (new in v0.8)

Open **GLO → MIX → ZONES**, the six-row page immediately after MIXER. Its track number and current sound name are static; the selected track's four controls are:

| Knob | Column | Allowed range | Default |
| --- | --- | --- | --- |
| **K1** | **MIN** | Lowest MIDI note, 0–127 | 0 (C−1) |
| **K2** | **MAX** | Highest MIDI note, 0–127 | 127 (G9) |
| **K3** | **V.MIN** | Lowest Note On velocity, 1–127 | 1 |
| **K4** | **V.MAX** | Highest Note On velocity, 1–127 | 127 |

Use **ALGORITHM** to select a track exactly as on MIXER. Both ends are inclusive. The UI keeps MIN ≤ MAX and V.MIN ≤ V.MAX. Note numbers display as musical names (MIDI 48 = C3, MIDI 60 = C4). The settings apply independently to T1–T6, even when several tracks share the same MIDI RX channel.

**How routing works:** external MIDI RX channel selects candidate tracks → each track checks its original source **note and velocity** → matching tracks play with their own **TRN transpose**. Overlapping zones trigger multiple instruments; non-overlapping zones create keyboard splits. Held notes still release correctly if the zones are edited before Note Off. The FM1's own small keyboard (and the emulator's built-in FM1 keybed) still targets the selected track only; use **external USB/TRS MIDI** or the emulator's MIDI-input controls for multitrack splits.

**Example:** assign T1, T2 and T3 to CH1 on MIXER; on ZONES set T1=C−1…B2, T2=C3…B4, T3=C5…G9 with V.MIN=1 and V.MAX=127. For a velocity split, instead give T1 and T2 the same note range, with T1 V.MAX=80 and T2 V.MIN=81. Overlap those boundaries to play both.

**Persistence:** All 24 zone boundaries save with the project (compact block stored only when customized); loading older projects restores full ranges. NEW PROJECT resets them. The owner tested keyboard zones and the newer tuning controls with a Korg X5D attached to a physical FM1, with successful results reported November 18, 2026.

### Per-track TUNING: TRN and FINE (new in v0.9)

Open **GLO → MIX → TUNING**, immediately after ZONES. It displays six tracks, selected with **ALGORITHM**. **K1 TRN** changes the pitch by **−36…+36 semitones** (up to three octaves up or down). **K2 FINE** adjusts **−50…+50 cents** (100 cents = one semitone), enabling subtle detuned layers without changing the musical note. **K3 and K4 are reserved**, and MIXER K4 is now unused.

**Layer example:** T1 FINE = −8, T2 = 0, T3 = +8 cents, with matching RX channels and overlapping ZONES. TRN applies after source note/velocity zone filtering, and FINE changes the synthesizer's sounding pitch without changing incoming MIDI note numbers. Melodic engines are affected, while DRUM-X and DRUM-LAB keep their mapped drum pitches. Transposed MIDI notes outside 0–127 are discarded; Note Off and sustain use the originally captured owners.

All six tracks' TRN/FINE settings are saved per project and participate in autosave; NEW PROJECT resets them to zero. The firmware reads legacy project TR values (±24 semitones); the extended ±36 range and cents tuning have compact new project blocks. **Back up projects before downgrading**, since older firmware may not understand the new blocks.

### BRASS and INDUSTRIAL synthesizers (v0.7 — device-tested)

The **BRASS** engine is a playable, intentionally synthetic wind-instrument approximation built from existing VOICE-style formant resonators and band-limited oscillators. Its **TYPE** control morphs continuously from saxophone character at **0** through hybrid sounds around **64** to trumpet character at **127**. EDIT 1 holds TYPE, PRESS, BUZZ and BREATH; EDIT 2 holds BODY, FORMNT, GROWL and ATTACK; EDIT 3 adds FAT (detuned oscillator), GRIND (saturation), METAL (ring-modulated harmonics) and CRUSH (bit-depth reduction). All four EDIT 3 effects start at **0 on factory presets**, preserving the clean wind character until edited. Six factory sounds: **TENOR SAX, BARITONE SAX, SAX GROWL, SOFT HORN, BRASS LEAD, TRUMPET**. BRASS is capped at **four simultaneous voices**, within the existing pool of eight shared across all tracks.

The **INDUSTRIAL** engine is intended for heavy, saturated, playable bass and lead sounds. EDIT 1: WAVE, FAT, GRIND, METAL. EDIT 2: CRUSH, FEED, CUT, RES. EDIT 3: WAVE2 (independent secondary oscillator waveform), MIX (oscillator balance), SUB (low octave) and MODE (low/high/band-pass). Factory sounds: **IRON BASS, STEEL LEAD, GRIND BASS, GEAR LEAD, BROKEN AMP, SUB MACHINE**.

For either engine, press **EDIT** and navigate through **EDIT 1 → EDIT 2 → EDIT 3**; the third page is available on the actual FM1 navigation chain. Its four controls use existing saved parameter slots, so sound and project data persist without a project-format expansion. The third page's parameters are **not yet direct MOD-matrix destinations**. Factory Favorites for engines added after the original 16 engine slots are unavailable in the existing fixed Favorites format; save your customized sound in a project/user preset instead.

**Physical testing (November 17, 2026):** v0.7 was tested on the FM1 with **three melodic tracks receiving the same MIDI channel**, EQ enabled, and chords and melodies together. The owner reported very good performance. Automated tests also covered high control settings, rapid retriggers, the four-voice cap and BRASS/INDUSTRIAL/DRUM-LAB project serialization. Physical testing does not constitute exhaustive worst-case CPU qualification.

### GLO MIXER and per-track MIDI transpose (MIXER introduced in v0.6; TUNING updated in v0.9)

Open **GLO → MIX → MIXER**. The MIXER is now the first GLO page whenever you enter GLO from elsewhere, even after visiting GLOBAL or SYSTEM. The old **LIVE → MANO** and **LIVE → RIELES** GLO pages are removed from navigation, but their underlying project data is preserved. The current page chain is **MIXER → ZONES → TUNING → SENDS → EQ → GLOBAL → SYSTEM**.

In the six-row MIXER, select a track with **ALGORITHM** and use **K1** to set its MIDI RX channel, **K2** for level and **K3** for pan; **K4 is unused**. Transpose is now on **TUNING K1**, with **−36…+36 semitones**, independently of the other tracks. Transpose affects both incoming USB/TRS MIDI and the FM1's own keyboard (including the browser emulator keyboard); **DRUM-X and DRUM-LAB pad mappings are unchanged**. The **PRESETS rotary** becomes the master DJ Filter control on this MIXER screen only; its ordinary preset browsing is unchanged elsewhere.

Each project's six transpose values are stored and restored with that project. Loading a legacy project with no transpose data starts the tracks at zero transpose. For a held note, changing **TUNING K1** affects **subsequent note-ons**, not the currently sounding pitch; note-off and sustain release still end the originally sounding note. Out-of-range transposed MIDI notes are ignored rather than wrapped. Layered tracks can share a MIDI channel and use different transpose values.

**Device verification:** the original v0.6 transpose control was checked on an FM1 on **November 16, 2026**. Its updated v0.9 TUNING placement, expanded ±36 range and FINE parameter were subsequently tested with a Korg X5D on **November 18, 2026**. The firmware build passed flash layout, RAM budget, and automated regression checks. This does not substitute for exhaustive long-duration stress testing.

### Live MIDI channel selection (v0.5 — device-tested)

Use **CC88**, **CC89** and **CC90** to select the receiving MIDI channel of **track 1**, **track 2** and **track 3**, respectively, while performing. These commands are accepted from **any incoming MIDI channel**, via USB or TRS MIDI, and are reserved from MIDI Learn and REC macros.

| Command | A CC value of 64–127 | A CC value of 0–63 |
| --- | --- | --- |
| **CC88** | T1 receives on **CH1** | T1 receives on **CH2** |
| **CC89** | T2 receives on **CH1** | T2 receives on **CH2** |
| **CC90** | T3 receives on **CH1** | T3 receives on **CH2** |

**This is explicit channel selection, not a press-to-toggle command:** sending CC88/127 repeatedly leaves T1 on CH1; sending CC88/0 selects CH2. Each CC affects **only its assigned track**, so T1–T3 can be combined freely on CH1 or CH2. Other tracks (including T4–T6) retain their previously assigned channels. The commands are temporary and **do not overwrite the saved project RX assignments**.

**GLO → MIXER** shows the currently effective MIDI channels (the **LIVE** label indicates temporary routing). **K1** still edits the selected track's MIDI RX channel, even after receiving a live CC command; turning K1 also clears that track's temporary override. Loading a project or restarting clears all live overrides and restores the project's saved RX settings. Incoming notes are rerouted to the selected channel; existing notes on removed destinations are released, while unrelated effects tails continue.

**Example:** Send CC88/127 and CC89/127 to put both T1 and T2 on CH1, leaving T3 on its previous channel. Send CC88/0 to move only T1 to CH2. Use the mixer K1 to select another channel for a track manually.

The v0.5 routing behavior, including mixer K1 operation, was **tested in the browser emulator and after flashing a physical FM1** on November 16, 2026. See [MIDI implementation notes](docs/randomization-midi.md).

### Live MIDI sound randomization (v0.4 — device-tested)

The dedicated **CC85/86/87** performance controls change a saved melodic sound live, without stopping held notes or retriggering envelopes. The feature was successfully flashed and **tested on a physical FM1 via TRS MIDI using a JamJum JP1 controller on November 15, 2026**. The non-interrupting CC87 reset was confirmed working on the device after the branch validation [run #33](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/43761151245) passed. This is owner-confirmed feature testing, not exhaustive hardware qualification.

| MIDI channel (1-based) | Target track | CC85 | CC86 | CC87 |
| --- | --- | --- | --- | --- |
| **13** | **T1** | Slight randomization | Hard randomization | Restore saved sound parameters |
| **14** | **T2** | Slight randomization | Hard randomization | Restore saved sound parameters |
| **15** | **T3** | Slight randomization | Hard randomization | Restore saved sound parameters |

Send a **Control Change value of 127 on press and 0 on release** (any value ≥64 triggers; a value below 64 re-arms the button). Repeated high-only presses are accepted after a 120 ms debounce. These three CC numbers are reserved on all MIDI channels and bypass ordinary RX routing, MIDI Learn and REC macros; **only channels 13–15 actually execute the commands**. Some controllers label channels **0–15**, so their displayed channel **12** corresponds to MIDI channel **13** (T1).

- **CC85:** changes up to two eligible continuous sound parameters by approximately 20–60 steps relative to the last saved baseline.
- **CC86:** changes up to three eligible continuous parameters by approximately 40–90 steps relative to that baseline.
- **CC87:** restores the **last explicitly saved** sound parameters **in place**. Unlike project/sound recall, it does **not** reload the engine or stop playing voices. Held notes and effects tails continue naturally. Track LEVEL and PAN, and the shared global EQ, remain unchanged.
- **Prerequisites:** save a project with a melodic sound on T1, T2 or T3. DRUM-X and DRUM-LAB are excluded. Randomization does not alter pitch, envelopes, categorical controls or timing parameters, and does not write to the saved project unless you explicitly save.
- **Engine changes:** CC87 does not switch an unsaved engine back to a different saved engine. Re-select/reload the saved sound first; CC87 is designed for uninterrupted parameter restoration within the same engine.

For example, save an ANALOG sound on T1, then send **channel 13 / CC86 / 127** to randomize it. Send **channel 13 / CC87 / 127** to restore the saved parameters while continuing to hold a note. A subsequent **0** value releases each button for the next press. These controls work through both USB and TRS MIDI input paths.

See [MIDI randomization technical notes](docs/randomization-midi.md) for eligibility details and implementation constraints.

### Live project recall and slot management (v0.4)

**Why Live Recall exists:** Original JIANT's keyboard-based shortcut is still available: hold **SAVE**, then press a mapped keyboard key to recall or store a section. For performance, v0.4 offers a simpler alternative: **long-press REC once to enable Live Recall mode and leave it active**. From then on, a **single tap** on **FX**, **SEL**, **ENV** or **LFO** recalls project slot **1**, **2**, **3** or **4**, respectively—without holding SAVE or reaching for a keyboard key each time. **Long-press REC again to exit Live Recall mode.** The inherited SAVE + keyboard shortcuts remain available; neither method replaces the other. On physical FM1 hardware and in the emulator, switching between saved sounds worked on the first note; the previous sound's active notes stopped while existing delay/reverb tails continued naturally. This feature and CLEAR SLOT were merged into `main` in [PR #18](https://github.com/MishaMashinaMM/jiant-live-drums/pull/18), merge commit [`43b2e9a`](https://github.com/MishaMashinaMM/jiant-live-drums/commit/43b2e9ad207cf62370e9e2aa6c132887caa9bfdf). The 11 live-drum regression suites and firmware build passed in the feature-branch CI run [#100](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/43690733723); live recall and slot management have also been validated on the physical FM1.

In **SAVE → LIBRARY → TOOLS**, the four visible actions are **INIT SOUND**, **CLEAR SLOT**, **NEW PROJECT** and **CPU TEST**, selected by turning **K1**, **K2**, **K3** and **K4** respectively. CLEAR SLOT requires confirmation; clearing a saved slot persists across reloads. The owner confirms that the v0.4 firmware was successfully flashed, tested and approved on a physical FM1.

**LIBRARY display fix (device-tested):** The SAVE → LIBRARY → PROJECTS overview now validates saved sections using the same project importer as project selection. Previously, some empty sections appeared occupied until their project was selected; the correction was merged in [PR #21](https://github.com/MishaMashinaMM/jiant-live-drums/pull/21). The owner confirmed the fix on a physical FM1 running v0.4.

### Global three-band EQ (introduced in v0.3)

Open **GLO → MIX → EQ** to edit the **single shared EQ**. This replaces the earlier experimental idea of one EQ per track, which was too CPU-intensive. The three bands are **LOW 50 Hz**, **MID 700 Hz** and **HIGH 10 kHz**, each adjustable from **−15 to +15 dB**; **ON** enables or bypasses the EQ. The MID band is a peaking filter (Q=1); LOW/HIGH are broad shelves. Bypass and gain adjustments are smoothed to avoid abrupt switching artifacts.

The EQ processes the **shared stereo dry buses, chorus/delay/reverb input sends, and SLICER input before effects processing**. It does **not** re-filter wet effects returns or the final output. It processes seven shared streams at most, rather than allocating six separate track EQs. All six tracks share the same settings.

**Saved with projects:** LOW, MID, HIGH and ON are stored in an optional EQ block in the project file. Older projects without the block load with flat EQ (0 dB) and ON. The EQ is **not** an independently saved device-wide setting: save your project to retain edits. The browser editor does **not yet** expose the new EQ parameters. Exported individual stems are captured before the shared EQ and therefore may not reconstruct the EQ-filtered master.

**Validation:** The Global EQ preview was successfully flashed to a physical FM1 on **November 14, 2026**, and the owner reported that it booted and the tested functions worked well. The emulator, DSP frequency tests, 4,096-block stress test, project compatibility tests, sanitizers and real firmware build also passed. **The Global EQ and the current feature set have been tested and approved on physical hardware.** This does not claim exhaustive long-duration stress certification. See [Global EQ technical notes](docs/JLD_GLOBAL_EQ_PREVIEW.md) for the signal flow and test limitations.

### Six receive channels

Each track has an independently saved RX channel. The default assignments are T1=1, T2=2, T3=3, T4=4, T5=5 and T6=6.

1. On the FM1, open **GLO → MIXER** and find the **T1–T6 RX channel** controls. Choose a MIDI channel from 1 to 16 for each track.
2. In **GLO → SYSTEM**, choose **ROUTE**:
   - **1-4:** route notes to matching tracks 1–4 (within the active TRACKS setting).
   - **1-6:** route notes to matching tracks 1–6 (within the active TRACKS setting).
   - **SEL:** incoming MIDI follows the selected track, as in JIANT.
3. Use **MENU → AUDIO → TRACKS** for 4 or 6 active tracks. RX assignments for T5 and T6 are retained even when TRACKS=4.

**Example:** assign both T1 and T2 to channel 1, choose ROUTE=1-6 with TRACKS=6, and play channel 1 from a keyboard. Both tracks receive the notes. Duplicate RX channels are intentional. Switching routing, assignments or active track count releases owned notes to avoid stuck notes.

The six RX settings are **saved with each project**. Loading a newly saved project restores its RX channels; older project files without RX data retain the current channel assignments. The original global settings remain available as the fallback for older projects.

**Project MIDI CC macros:** each project can hold up to **32** assignments (224 bytes for the rows, plus a three-byte serialized header). Active macros respond to incoming CC messages: linear range or two-value toggle, with optional reverse and independent bypass. Each row targets a saved track/engine/parameter, automatically suspending when the track uses another engine without losing its enabled setting. Multiple rows may share a CC and channel. The original 32-destination MIDI Learn remains available (active macros take priority on a matching CC). Older four-macro projects still load; projects containing more than four macros cannot be opened by earlier JLD versions without conversion. Very full projects may report `SECTION FULL` when saving additional macro data.

**REC macro editor (live build):** tap **REC** to view the macro table. Press **OCT+** to add: send a CC from your external controller (its incoming channel is captured automatically), then navigate to the desired track and sound page and turn the target parameter's knob. The macro is added with **RNG 0–127, REV OFF, ON** and the REC table reopens. **SELECT** chooses an existing row; **ALGORITHM** turns it ON/OFF; **K1** chooses RNG/TGL, **K2/K3** set the two endpoints, and **K4** toggles reverse. Turn ALGORITHM/K1/K4 clockwise for ON/TGL/REV and counterclockwise for OFF/RNG/no reverse. To delete, select a row, press **OCT−**, then **OCT+** to confirm or **OCT−** to cancel. REC cancels an unfinished assignment. The captured **CC/CH/SRC** are intentionally read-only; delete and recreate a row to change them. The ON column shows **Y** for active, **N** for disabled, and **S** when an enabled assignment is temporarily suspended by an incompatible engine. The assignment is stored with its project when saved. The original JIANT M1–M4 sound-engine macros are unchanged; the existing 32-destination MIDI Learn remains independently available.

SELECT scrolls the REC list in four-row pages. A small indicator shows the selected position and total (for example, `13/32`). The same knob and delete controls work on every row. Value edits redraw only their affected cells, avoiding whole-screen flicker on the FM1 (verified after flashing the `rec-partial-redraw` preview). The 32-row UI, paging, editing and persistence have been tested and approved on physical FM1 hardware, as well as exercised in the emulator and host tests. Exhaustive long-duration stress qualification is not claimed.

After a new assignment, REC briefly ignores residual encoder detents until the knobs settle. This prevents the knob movement used to select the destination from also changing the new macro's default RNG type or endpoints.

**Hardware testing:** On November 10, 2026, the published **JLD v0.3** firmware flashed and booted on **two FM1 devices**. One installation upgraded directly from **official V15**, and the other upgraded from **original JIANT**; **both paths succeeded** and both devices were reported to operate as expected. The first unit's **ABOUT v0.3** display and smooth REC macro editing without full-screen redraw were explicitly checked; the second also passed the owner's functional checks. Earlier test rounds verified external MIDI macros, REC/SEQ/PLAY LEDs and pages, editing, project save/load, and emulator paging.  The isolated preview workflow and [device-test checklist](docs/macro-control-device-test.md) remain available for future development.

### MIDI LEARN: 32 destinations and shared CC

1. Open **MENU → MIDI → LEARN** and switch it **ON**.
2. Visit a track's editable parameter (for example, T1 filter cutoff) and turn its corresponding FM1 knob to select that destination.
3. Send a CC from your external MIDI controller. JIANT LIVE DRUMS learns the number for that parameter.
4. To share the same CC, select another parameter (for example, T2 resonance) and send **the same CC number** again.
5. Turn LEARN **OFF** when finished. **LEARN CLEAR** removes all learned assignments.

**Example:** CC40 → T1 cutoff **and** CC40 → T2 resonance. One movement of your controller's CC40 knob changes both simultaneously.

- A single parameter can have **one learned CC** at a time. Relearning it replaces that parameter's former assignment **without removing the same CC from other destinations**.
- The **32-entry limit counts destinations**, not distinct CC numbers. A CC assigned to five parameters uses five entries.
- System-reserved CCs and CCs already handled by the standard MIDI map cannot be learned; the firmware displays **RESERVED**. When all 32 destinations are occupied, it reports **LEARN FULL**.
- Learn assignments are **global settings**, saved persistently; settings from earlier JIANT formats are migrated.
- Normal MIDI handling (notes, sustain, pitch bend and other standard controllers) continues to work. USB-MIDI and TRS-MIDI inputs are supported.

### DRUM-X: 12 editable sounds

The original eight kit sounds remain. Four additional **independent instances** are provided:

| New sound | Duplicates the original sound type | Editing |
| --- | --- | --- |
| **B2** | BD (bass drum) | Same parameters and controls as BD; independent values |
| **S2** | SD (snare) | Same parameters and controls as SD; independent values |
| **O2** | OH (open hi-hat) | Same parameters and controls as OH; independent values |
| **T2** | TM (tom) | Same parameters and controls as TM; independent values |

DRUM-X has 12 pads but **retains the original eight drum-note assignments** (for example kick 36, snare 38 and open hat 46) rather than replacing them with a chromatic map. The four additional default notes are B2=41, S2=40, O2=43 and T2=44. All 12 notes can be reassigned individually on the **SEQ / DRUM MIDI MAP** page. Its original per-sound editing pages have not been redesigned.

### DRUM-LAB: 12 independently editable drum sounds

DRUM-LAB is a **second drum synthesis engine**, separate from DRUM-X. Each track assigned DRUM-LAB has its own kit of 12 editable pads. The default note assignments are **MIDI 36–47** (C2 through B2); the notes can be changed per pad on the SEQ page. DRUM-LAB has kick, snare, clap, closed/open hat, tom, metallic and crash sound types, including a second tom variant. You can change a pad's sound type without changing the other pads.

Open **EDIT** while a DRUM-LAB track is selected:

| EDIT pages | Controls |
| --- | --- |
| **STRIKE** (page 1) | Kit-wide **PITCH, SWEEP, BODY, SNAP** |
| **BODY** (page 2) | Kit-wide **NOISE, TONE, DRIVE, WIRES** |
| **SOUND** (page 1) | K1 chooses the pad; K2–K4 edit its **PITCH, SWEEP, BODY** |
| **SOUND 2** | K1 chooses the pad; K2–K4 edit its **SNAP, NOISE, TONE** |
| **SOUND 3** | K1 chooses the pad; K2–K4 edit its **DRIVE, WIRES, WEIGHT** |
| **SOUND 4** | K1 chooses the pad; K2 edits **METAL**, K3 changes its **TYPE**; K4 is unused |
| **LEVELS, LEVELS 2, LEVELS 3** | Independent output levels for all 12 pads |

The ten per-pad sound parameters have values **0–127**. The eight kit controls on STRIKE and BODY are a separate **0–100% multiplier layer**:

- **100%** plays the pad's own saved parameter value unchanged.
- **50%** halves its effective value *when played* (for example, pad PITCH 18 becomes 9).
- **0%** makes the effective parameter value zero across all 12 pads.

Changing a kit-wide percentage **does not overwrite individual pad settings**. All eight multipliers default to **100%** for new and migrated projects, are saved with the project, and can be assigned to incoming MIDI CCs through the existing **32-destination MIDI LEARN** mechanism. This makes them useful for shaping an entire kit live while retaining every pad's programmed sound.

### Live drum buttons: SEQ and PLAY

With the sequencer disabled, these buttons open two dedicated **per-track, per-pad** pages for **both DRUM-X and DRUM-LAB**. They do not trigger playback, recording, a pattern or a transport command.

| Button / page | What the controls do |
| --- | --- |
| **SEQ → DRUM MIDI MAP** | **ALGO** chooses the track; **SELECT** chooses a pad; **OCT− / OCT+** jump between pads 1–6 and 7–12. **K1** edits that pad's incoming MIDI note (**0–127**), **K2** its pan (**L64–R64**, C=center), and **K3** its pitch offset (**−24 to +24 semitones**). K4 is unused. |
| **PLAY → DRUM PERFORMANCE** | Same track, pad and six-pad-bank selection. **K1 CURVE** selects velocity curve **LIN, SOFT, HARD or FIX**; **K2 V>S** sets velocity-to-sound response (**0–127**); **K3 CH** assigns a choke group (**--**, G1–G4); **K4 RETR** chooses **OVR** (OVERLAP) or **RST** (RESTART). |

Six pads are visible at a time, and the selected row is highlighted. Each page uses the full screen. Press **SEQ** or **PLAY** again to close its page; pressing the other button switches directly between them. Tapping a regular page-family button (such as **EDIT** or **ENV**) or **HOME** leaves the drum page immediately. **REC** opens the project-based MIDI CC MACROS page, as described above.

On the tested FM1, these pages open with a single press and remain stable while idle. The SEQ, PLAY and REC LEDs illuminate when their live pages are active. Turning a K knob updates the affected pad row without blinking or clearing the entire screen (Stage 29C, `b9654ee`).

- **OVR / OVERLAP** is the default: a repeated hit may overlap the previous hit, subject to the eight-voice limit.
- **RST / RESTART** retriggers the selected pad instead of letting the previous hit continue.
- Pads assigned to the same **G1–G4 choke group** can cut one another; **--** means no assigned group.
- These settings belong to the selected track and pad, so different kits and pads can use different note maps, curves, choke groups and retrigger policies.

**Polyphony is eight voices total**, not eight per kit: simultaneous hits on several tracks or both drum engines compete for the same eight-voice pool. Stage 27B host stress tests passed for 12-pad and combined 24-pad bursts; the current drum features have also been tested and approved on physical FM1 hardware. The host stress test is additional automated coverage, not a substitute for the reported device tests.

### Display and startup

The startup screen uses a black background with **JIANT LIVE DRUMS**, author attribution and the current **build-version label** (for example **v0.9**). It no longer hard-codes **v0.5**. Decorative display animations and the startup LED sweep are disabled; informative UI and sound feedback remain.

## Try JIANT LIVE DRUMS on your computer

**[▶ Launch JIANT LIVE DRUMS in your browser](https://mishamashinamm.github.io/jiant-live-drums/emu/)** — no installation or FM1 hardware is required to use the emulator. The public emulator follows the current v0.9 `main` build once GitHub Pages deployment completes; the v0.9 release candidate was already tested on a physical FM1 with a Korg X5D.

The browser tab is titled **JIANT LIVE DRUMS**. It includes the virtual FM1, computer-keyboard controls, MIDI CC testing, and an optional JIANT backup importer. Credits and licenses remain in the repository.

The Drums site's GitHub Actions workflow builds the emulator under `emu/` and the dedicated installer and editor under `webapp/`. Their source files are `web/emu/index.html`, `web/index_pkg.html`, and `web/editor.html`. The installer is separate from the emulator.

### Dedicated web installer, editor and documentation

The Drums site has the same core web-page options as Jiant Live:

| Page | Route | Purpose |
| --- | --- | --- |
| Home | `/` | Drums overview, links, authorship and risk notice |
| Emulator | `/emu/` | Live audition, MIDI test input, CC sender and backup importer |
| Installer | `/webapp/installer/` | USB-MIDI update, device/package validation, backup/return-to-stock and progress report |
| Editor | `/webapp/editor/` | Sound/project editing, banks, settings, complete backup/restore |
| Interactive manual | `/manual/` | Inherited JIANT navigation and control reference (Drums differences noted) |

The site generator (`web/make_site.py`) embeds the tested update libraries and packages only the Drums build passed to it. The GitHub Pages workflow builds both hardware firmware and emulator in the default live-only configuration from the current `main` commit; it does not select older JIANT `STABLE`, `PREVIEW`, or `CLASSIC` builds. Publish the site only after its workflow succeeds and the displayed version and SHA-256 are verified.

### Run the emulator locally

You can also build and serve the emulator on your own computer.

Requirements: this source checkout, the original build dependencies, Python 3, and **Emscripten** (`emcc`) for WebAssembly builds.

```sh
bash build.sh
bash web/emu/build.sh
python3 -m http.server 8000 --directory build/emu
```

Open **http://localhost:8000/** in a desktop browser and click **Switch on**. The computer keyboard and on-screen controls can play it. The page also has a **MIDI test input** panel for selecting channels 1–16 and auditioning 12 DRUM-X notes.

### Audition an existing JIANT backup

Before clicking **Switch on**:

1. Use the **Jiant backup (.json)** picker above the emulator to select a backup made with the JIANT backup tool.
2. Click **Switch on**. The file is validated and its saved objects are loaded into an **isolated browser session**.
3. Open your sounds, user banks and projects in the emulator and verify them by ear and by their parameter settings.

The original backup file and the FM1 are not changed. An imported backup is **not silently written over an existing browser flash session**. The browser backup-import path was verified with a real JIANT v0.9.17 backup, including user banks and a project. This describes a browser-only operation, not a firmware feature awaiting FM1 testing; it does not establish compatibility with every possible backup version.

The Drums emulator does not load a sample bank or offer sample-kit playback. It retains the hardware controls, backup import, and MIDI test tools.

## Status and safety

**Current hardware validation:** On November 18, 2026, JLD v0.9 TUNING and zones were successfully tested on a physical FM1 with a Korg X5D. The orange-accent UI version was subsequently flashed successfully onto an FM1, and the owner confirmed everything was working properly. The updated UI is therefore **device-tested**, while long-duration stress and recovery behavior remain outside this report.

- **v0.4 device-tested release:** Live project recall, CLEAR SLOT and direct K1–K4 TOOLS navigation merged in [PR #18](https://github.com/MishaMashinaMM/jiant-live-drums/pull/18). The project LIBRARY occupancy discrepancy was corrected in [PR #21](https://github.com/MishaMashinaMM/jiant-live-drums/pull/21). **The owner successfully flashed v0.4 on a physical FM1 and confirmed live recall, slot management, LIBRARY indicators, startup/About version display and the current feature set work on the device (November 15, 2026).** Firmware, host regression and emulator checks also passed. Device approval does not imply exhaustive long-duration stress qualification.

- **Previous v0.3 release (historical):** [GitHub Actions build and deployment](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/42855710544) completed successfully for `v0.3-1956d21`. Both physical FM1 units were flashed and checked successfully on November 10, 2026: **official V15 → JLD v0.3** and **original JIANT → JLD v0.3**. The [public installer](https://mishamashinamm.github.io/jiant-live-drums/webapp/installer/) serves the current v0.7 release, not this historical package.
- **Host regression:** the live-drum host regression suite covers drum engines, MIDI routing, project serialization, macro control, and Stage 27B stress. The `macro-32` firmware and emulator build jobs, including all 11 live-drum host regression suites, passed on November 10, 2026.
- **Historical 32-macro build:** source commit [`5c79b3b`](https://github.com/MishaMashinaMM/jiant-live-drums/commit/5c79b3b7b394c9d2a6ac75d4847aef9ef81e44da) produced [successful firmware and emulator preview artifacts](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/42836474817). The earlier Stage 29C package `felucca-v0.2-b9654ee.fwsc` is also historical; neither package is the current v0.4 release.
- **REC partial redraw:** the [firmware and emulator CI run](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/42849592862) passed all 11 live-drum host regression suites. The new installer was successfully flashed to an FM1 on November 10, 2026; K-knob macro edits no longer cause full-screen flickering on the device.
- **Update simulation:** Stage 29C OTA staging, CRC validation, allowed flash boundaries and the Stage 29B → 29C loader update passed host tests. These checks do **not** guarantee recovery from a failed physical update.
- **Physical FM1 testing (November 9–10, 2026):** **v0.3 (`1956d21`) was successfully flashed and booted on two FM1 units**—one starting from **official V15** and one from **original JIANT**—and the owner reported **all performed device checks passed**. The first device confirmed ABOUT v0.3 and smooth REC partial updates without flicker; the second also functioned as expected. Earlier physical tests confirmed DRUM-LAB sound/editing, SEQ/PLAY navigation, LEDs, and external-controller MIDI CC macros. Physical FM1 testing now covers the current macro feature set, including paging, editing, deletion and save/reload (owner-confirmed for v0.4). Emulator tests also exercised ten macros over three pages, and host tests exercised all 32. Exhaustive long-duration reliability qualification is not claimed.
- **Safety:** user projects/sound banks/settings were backed up externally before installation. A fully verified physical recovery procedure is still needed. The dedicated GitHub Pages installer is deployed. **Official V15 → JLD v0.3** and **original JIANT → JLD v0.3** were tested successfully, but other firmware variants, devices, and recovery paths are not thereby verified.

**v0.9 verification:** The v0.9 FM1 installer passed [firmware compilation and package checks, project persistence, MIDI routing, UI regression, pitch audio tests and WebAssembly smoke tests](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/44314672326). The owner confirmed successful on-device operation with a Korg X5D on November 18, 2026. The official main deployment rebuilds the signed-off implementation as v0.9 without RC labeling.

**v0.8 verification:** The v0.8 keyboard-zones development branch passed [FM1 firmware, project persistence, MIDI routing, UI and WebAssembly checks](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/44275406783). The v0.8 candidate was then integrated into the v0.9 release, and its physical external-MIDI behavior was confirmed with a Korg X5D on November 18, 2026. The corrected splash version display carries through to v0.9.

**v0.7 verification:** The user-approved BRASS/INDUSTRIAL device-test package passed [hardware firmware and complete CI regression checks](https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/44168705612) and was flashed to an FM1. The measured firmware image was 574,196 / 584,336 bytes (10,140 bytes free), with unchanged static RAM. The final public installer rebuilds the same source as v0.7 and should be verified independently after deployment.

For regression testing in the development environment:

```sh
bash tests/run_live_drum_tests.sh
```

## Upstream documentation and references

JIANT LIVE DRUMS retains many JIANT synthesis engines, interfaces, effects and editing workflows, but removes the sequencer and substantially changes drum pages. For unchanged controls, refer to the original manual:

- [Original JIANT project](https://github.com/juanjiant-bit/JIANT) and [original README](https://github.com/juanjiant-bit/JIANT/blob/main/README.md)
- [JIANT English manual](https://github.com/juanjiant-bit/JIANT/blob/main/docs/MANUAL.md) and [Spanish manual](https://github.com/juanjiant-bit/JIANT/blob/main/docs/MANUAL.es.md)
- [Upstream interactive manual](https://juanjiant-bit.github.io/JIANT/manual/)
- [Felucca](https://github.com/hugelton/Felucca), the firmware on which JIANT is based
- [SLOOP](https://github.com/isod89/sloop-fm1), source of JIANT's song-system lineage

**Important:** the inherited manual, navigation map, screenshots and cheat sheets describe upstream JIANT revisions; they are **not exact Jiant Live Drums v0.9 control maps**. Original illustrations under [`docs/`](docs/) are historical upstream assets.

## Authors, credits and license

- **Original JIANT firmware:** Juan Ignacio Antonucci (**Juan Jiant**).
- **JIANT LIVE DRUMS modification:** **Misha Mashina**.
- **Felucca firmware:** Hügelton Instruments, including Leo Kuroshita and other credited contributors.
- Additional contributors, predecessor projects, third-party tools, and font licenses retain their original attribution in source headers and accompanying license files.

**License: [GNU General Public License v3.0 only](LICENSE) (GPL-3.0-only).** This fork remains free software under the original copyleft license; modifications are identified as such. Original authors' credits and rights are preserved.

For the original author's project and support options, see [JIANT](https://github.com/juanjiant-bit/JIANT).
