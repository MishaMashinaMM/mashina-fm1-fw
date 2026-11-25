# DRUM-AC cymbal sample experiment

Round 1: listening only. No firmware integration or project format changes yet.

The existing 520,148-byte application has 64,188 bytes spare; retain 32,768 bytes engineering headroom, leaving 31,420 bytes for sample data, descriptors and playback code. The sample choices may mix formats and rates. Start with crash and ride bow, not the hi-hats.

Pinned real CC0 recordings: Virtuosity Drums (Versilian Studios / Karoryfer Samples), mid mic, velocity layer 3, round robin 1. The script verifies original Git blob hashes, normalizes each source once, trims, applies a short fade, encodes and decodes to WAV. The CI artifact includes all candidates, reference recordings, exact BIN payloads and manifest. No DSP performance or audio-quality claim before listening/device tests.

Implementation after audition: immutable XIP data, shared across kits; independent per-voice predictor, nibble position and phase; fixed point resampling and bounded pitch; preserve all non-metal pad paths and record layouts. Consider using ride bow sample for RIDE SPLASH, keep synthesized RIDE BELL until separately auditioned. Test polyphony, choke/retrigger, delay, pan, pitch and all pad/global knobs. Never duplicate samples per preset or write sample data into preset flash.
