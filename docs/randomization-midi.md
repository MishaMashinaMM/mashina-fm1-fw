# MIDI sound randomization — implementation specification

Status: MIDI dispatch, saved-project sound recall and dedicated regression tests implemented on this branch. The emulator build, smoke test, dedicated MIDI tests, existing live-drum tests and native firmware compilation/package have passed GitHub Actions. Physical FM1 hardware and pad behavior have not yet been verified.

## Commands
- Incoming MIDI channel 13 targets physical track T1; channel 14 T2; channel 15 T3, independent of each track's note RX channel.
- CC85: Slight, select up to two distinct eligible parameters; each independently gets a random signed 20–60 point offset.
- CC86: Hard, select up to three distinct eligible parameters; each independently gets a random signed 40–90 point offset.
- CC87: reload only the targeted track's sound and engine from its last explicitly saved project slot, including undoing all manual edits. Existing mix/performance settings and global effects stay untouched.
- CC85/86 operate only on eligible melodic engines. CC87 may restore a saved melodic engine after an unsaved engine change, even if that change was to a drum engine. A saved DRUM-X or DRUM-LAB track is excluded.
- Repeated mutation of a selected parameter always uses its saved baseline, NOT its already mutated live value. Parameters not selected by the current trigger retain their current live values.
- Clamp resulting values to 0..127. Never select more parameters than are eligible; an empty pool is a no-op.

## Eligibility
Use an explicit engine/mode-specific allowlist; do not infer eligibility merely from storage type or 0..127 representation.
Include suitable engine tonal parameters and shared FILTER CUT, FILTER RES, DIST where they actually affect the sound; consider MORPH for its supported engines.
Exclude all ADSR and envelope-like controls, timing/articulation, pitch/tuning/octave, waveforms, algorithms, modes, selectors, toggles, volume, pan, routing, FX sends, and globals.
Do not mutate parameters with non-0..127 native ranges. Mode-dependent descriptors must be checked using the active engine/mode.
Never use a disallowed parameter to fill a two- or three-parameter quota.

## Implementation safeguards / audit items
1. Inspect MIDI_CC_MAP, legacy MIDI LEARN, and REC macro CC capture/dispatch for conflicts with 85–87 on channels 13–15. Reserved commands must not trigger or be learned as macros on those channels.
2. Define a pad press edge policy to prevent release and repeated held values from triggering extra mutations.
3. Establish a per-track saved-sound baseline from the actual loaded project. Reset must not reload other tracks, macros, shared/global FX, MIDI routing, or kill unrelated voices.
4. Define unsaved/new-project behavior and whether explicit SAVE updates the baseline; test both.
5. Ensure parameter changes are applied through the same safety/update path as ordinary edits, not raw unvalidated writes.
6. Test all eight active melodic engines, mode-specific eligibility, limited pools, repeated mutations, shared parameters, reset isolation, USB and TRS MIDI, and no regression to normal MIDI CC/REC macros.
7. Keep this feature isolated to this branch until verified in host tests and emulator.

## Code locations identified
- firmware/src/engines.c: active engine registry (ANALOG, PHASE, LOFI, VOICE, WHEEL, NOISE, FM6, ANIMAL; two drum engines excluded).
- firmware/src/eng_*.c: engine edit descriptors and mode-dependent overrides.
- firmware/src/params.c: shared FILTER, DIST and MORPH descriptors.
- firmware/src/midi_control.c: USB/TRS channel controls, CC map, MIDI learn, REC macros.
- firmware/src/project.c: project capture and loading.

## Current trigger compatibility
- CC values 0..63 re-arm a command; values 64..127 activate it.
- A second high-only command is accepted after a 120 ms duplicate-message debounce window.
- There is no perfect way to distinguish a held controller's slow repeats from separate presses using CC messages alone.
- CC85..87 are reserved on every MIDI channel and never reach MIDI Learn, REC macros or normal CC mapping.

## Baseline integration
- Successful explicit project saves refresh the baseline for T1..T3.
- Explicit project load, nonempty Live Recall, and individual saved-sound recall establish per-track saved origins.
- No unsaved engine or preset change replaces the saved baseline. Randomization on a different unsaved engine is ignored until the saved sound returns or another explicit save occurs.
- CC87 queues a main-thread call to the existing `project_recall_sound` / `sound_put` code, restoring the saved engine, FM6 patch when applicable and sound parameters while retaining mix/performance settings.
- Clearing a saved slot invalidates any randomization baseline that references it. A project restored only from automatic unsaved recovery is not implicitly treated as an explicit save.
- No auto-save is requested by the MIDI command handler; its edits update the in-memory motion base, so a subsequent explicit SAVE records what the user hears.
- Without a saved project source, CC85/86 have no baseline and are ignored; CC87 reports that there is no saved sound.
- USB and TRS MIDI queues both dispatch reserved CCs before note-RX routing, modulation, MIDI Learn and REC macros.
- The first eligible command mixes human trigger timing into its pseudorandom seed so rebooting does not repeat an identical fixed sequence.

## Validation
- Branch-specific GitHub Actions workflow: `.github/workflows/randomization-midi.yml`.
- This workflow compiles the emulator, runs its smoke test, runs `tests/randomization_midi_test.c`, and runs the existing live-drum regression suite.
- Successful native firmware build run #17: https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/43750525254
- Dedicated regression coverage now includes both USB and TRS MIDI queues, per-track persisted sound reset, hard/slight ranges, Live Recall, drum exclusions, source deletion and 8-engine parameter whitelist.
- The workflow uploads a downloadable experimental emulator artifact named `randomization-midi-emulator`. It does not deploy to or alter the production GitHub Pages site.
- Native firmware is compiled and packaged but deliberately not deployed or advertised as device-tested. Hardware pad behavior still needs verification before merge/release.

## Temporary live routing toggles (CC88–90)

CC88 toggles T1 between CH1 and CH2; CC89 toggles T2; CC90 toggles T3. Each toggle is independent, giving eight CH1/CH2 combinations for T1–T3. T4–T6 always use CH2 while the live override is active. Initially all three toggles are CH2; the first high trigger of each CC moves its track to CH1.

Commands are received on any MIDI channel and intercepted before MIDI Learn, macros and modulation. A rising edge (value 0–63 followed by 64–127) triggers one toggle; repeated high messages do not retrigger until rearmed by a low value. The first toggle activates temporary live routing across all currently active tracks (up to six), overriding normal ROUTE and saved per-track RX assignments without changing them. Note ownership is released for destinations removed by a routing change; unrelated sound and FX remain intact. Loading a project or power cycling clears the override and all toggle states. CC88–90 are unavailable for new MIDI Learn and macro capture.
