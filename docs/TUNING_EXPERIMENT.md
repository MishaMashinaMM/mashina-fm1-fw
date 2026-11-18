# v0.9 TUNING technical notes — TRN ±36 / FINE ±50

This document describes TUNING as shipped in **JIANT LIVE DRUMS v0.9**, built on the v0.8 Keyboard Zones feature. The release candidate was successfully tested on FM1 hardware using a Korg X5D MIDI keyboard on November 18, 2026.

## GLO navigation

`MIXER → ZONES → TUNING → SENDS → EQ → GLOBAL → SYSTEM`.

The first three pages share the same full-height six-row view. Select T1–T6
with the **ALGORITHM** control (including T5/T6 while sleeping).

| Page | K1 | K2 | K3 | K4 |
| --- | --- | --- | --- | --- |
| MIXER | RX MIDI channel (1–16) | Volume | Pan | Unused |
| ZONES | Note MIN | Note MAX | Velocity MIN | Velocity MAX |
| TUNING | TRN (−36 to +36 semitones) | FINE (−50 to +50 cents) | Unused | Unused |

TRN is measured in **semitones** (±36 = ±3 octaves).
FINE uses **cents** (100 cents = 1 semitone) and is neutral at zero.
Both apply independently to each melodic track. **Drum tracks retain
their MIDI note mapping and ignore per-track TRN and FINE.**

### Example layer

Assign T1/T2/T3 the same RX MIDI channel and overlapping ZONES, then:

| Track | TRN | FINE |
| --- | ---: | ---: |
| T1 | 0 | −8 cents |
| T2 | 0 | 0 cents |
| T3 | 0 | +8 cents |

This creates a subtle beating effect while all three instruments still
play the same notes. Use different PAN positions for stereo width.

TRN changes incoming notes **after zone matching** and **before** voice
allocation. Transposed notes outside MIDI 0–127 are not played.
MIDI Note Off/sustain ownership is captured at the original Note On,
so changing TRN or FINE during a held note should not leave a stuck note.

FINE changes synthesized oscillator frequencies during rendering, not
the MIDI note number or Keyboard Zones. Separately synthesized oscillators
retain their internal detune relationships. The FM1's internal keyboard
still targets only the selected track, as in v0.8.

### Storage and compatibility

Both settings save **per project** (all six tracks); NEW PROJECT resets
them to zero. A previous project containing old `TR` data (±24)
loads unchanged. Values outside that range use an extended `T2`
serialization block; nonzero fine tuning uses an optional `FN` block
(biased signed cents). Autosave signatures include both arrays.

Because old firmware does not understand new `T2`/`FN` blocks,
**back up projects before downgrading**. v0.9 was approved after physical FM1 testing with a Korg X5D.

## Testing

The v0.9 release build serves the main browser emulator and public FM1 installer through GitHub Pages. The dedicated live regression suite includes pitch audio comparisons, MIDI routing boundaries, navigation tests and project-format compatibility. Physical testing with a Korg X5D was completed successfully.
