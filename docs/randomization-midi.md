# MIDI sound randomization — implementation specification

Status: MIDI dispatch, saved-project sound recall and dedicated regression tests implemented on this branch. The emulator build, smoke test, dedicated MIDI tests and existing live-drum tests have passed GitHub Actions. Physical FM1 firmware and hardware pad behavior have not yet been verified.

## Commands
- Incoming MIDI channel 13 targets physical track T1; channel 14 T2; channel 15 T3, independent of each track's note RX channel.
- CC85: Slight, select up to two distinct eligible parameters; each independently gets a random signed 15–30 point offset.
- CC86: Hard, select up to three distinct eligible parameters; each independently gets a random signed 30–60 point offset.
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
- No auto-save is requested by the MIDI command handler.

## Validation
- Branch-specific GitHub Actions workflow: `.github/workflows/randomization-midi.yml`.
- This workflow compiles the emulator, runs its smoke test, runs `tests/randomization_midi_test.c`, and runs the existing live-drum regression suite.
- Successful run #11: https://github.com/MishaMashinaMM/jiant-live-drums/actions/runs/43749851627
- Hardware behavior and native device firmware build still need verification before release.
