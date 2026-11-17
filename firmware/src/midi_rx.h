/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef JIANT_MIDI_RX_H
#define JIANT_MIDI_RX_H
/* Current receive-only setup. New projects save it; older projects without an RX
 * block keep the global fallback from settings. One-based channels. */
static uint8_t midi_rx[6] = {1, 2, 3, 4, 5, 6};
/* v0.6: project-specific incoming MIDI transpose, melodic tracks only. */
static int8_t midi_transpose[NTRK];
/* Per-track key/velocity zones (incoming MIDI BEFORE track transpose).
 * All endpoints inclusive. Velocity 0 means Note Off, never a Note On. */
enum { Z_NOTE_MIN, Z_NOTE_MAX, Z_VEL_MIN, Z_VEL_MAX, Z_FIELDS };
static uint8_t midi_zone[NTRK][Z_FIELDS] = {
    {0, 127, 1, 127}, {0, 127, 1, 127}, {0, 127, 1, 127},
    {0, 127, 1, 127}, {0, 127, 1, 127}, {0, 127, 1, 127}
};
static void midi_zone_reset(void)
{
    for (uint32_t i = 0; i < NTRK; i++) {
        midi_zone[i][Z_NOTE_MIN] = 0;
        midi_zone[i][Z_NOTE_MAX] = 127;
        midi_zone[i][Z_VEL_MIN] = 1;
        midi_zone[i][Z_VEL_MAX] = 127;
    }
}
static int midi_zone_accept(uint32_t track, uint32_t note, uint32_t vel)
{
    const uint8_t *z = midi_zone[track];
    return note >= z[Z_NOTE_MIN] && note <= z[Z_NOTE_MAX] &&
           vel >= z[Z_VEL_MIN] && vel <= z[Z_VEL_MAX];
}

static uint32_t midi_rx_channel(uint32_t track)
{
    uint32_t v = midi_rx[track];
    return v >= 1u && v <= 16u ? v : track + 1u;
}
#endif
