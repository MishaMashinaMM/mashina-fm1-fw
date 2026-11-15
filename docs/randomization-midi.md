# MIDI sound randomization — implementation specification

Status: design agreed; implementation pending. This document is intentionally not a claim that the feature is wired or tested.

## Commands
- Incoming MIDI channel 13 targets physical track T1; channel 14 T2; channel 15 T3, independent of each track's note RX channel.
- CC85: Slight, select up to two distinct eligible parameters; each independently gets a random signed 15–30 point offset.
- CC86: Hard, select up to three distinct eligible parameters; each independently gets a random signed 30–60 point offset.
- CC87: reset only the targeted track's sound to the project's saved baseline.
- Commands are for active melodic engines only; DRUM-X and DRUM-LAB must be ignored.
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
