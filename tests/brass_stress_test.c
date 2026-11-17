/* SPDX-License-Identifier: GPL-3.0-only
 * BRASS release-candidate stress: 4-note cap, extreme knobs, repeated notes.
 * Performance timings are host diagnostics, NOT measurements of FM1 CPU.
 * Neither original synthesis files nor factory presets are changed.
 */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int failures;
static void result(const char *name, int pass)
{
    printf("brass_stress: %-48s %s\n", name, pass ? "PASS" : "FAIL");
    failures += !pass;
}
static void run_case(const char *name, int extreme, uint32_t morph, uint32_t repeat)
{
    track_t *t = &trk[0];
    static const uint8_t notes[6] = {45, 52, 57, 64, 69, 76};
    int32_t buffer[CTL * 2];
    uint64_t peak = 0, activity = 0, start, elapsed;
    uint32_t voices = 0, largest = 0;
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    host_tracks_init();
    host_preset(t, ENGI_BRASS, 0);
    t->p[P_MUTE] = 0;
    t->p[P_VOICE] = V_POLY;
    t->p[P_SUS] = 127;
    t->p[P_E0] = (int16_t)morph;
    if (extreme) {
        for (uint32_t j = 1; j < 8u; j++)
            t->p[P_E0 + j] = 127;
        t->p[BR_FAT] = t->p[BR_GRIND] = 127;
        t->p[BR_METAL] = t->p[BR_CRUSH] = 127;
    }
    start = now_ns();
    for (uint32_t b = 0; b < 512u; b++) {
        if (b == 0u || (repeat && !(b % 15u)))
            for (uint32_t k = 0; k < 6u; k++)
                trk_note_on(t, notes[k], 100u);
        if (repeat && !(b % 15u))
            t->p[P_E0] = (int16_t)((morph + b) % 128u);
        mix_block(buffer, CTL);
        for (uint32_t k = 0; k < CTL * 2u; k++) {
            int64_t v = buffer[k], mag = v < 0 ? -v : v;
            if ((uint64_t)mag > peak) peak = (uint64_t)mag;
            activity += mag != 0;
        }
        voices = 0;
        for (uint32_t k = 0; k < NVOICE; k++)
            voices += t->v[k].active && t->v[k].stage != 4u;
        if (voices > largest) largest = voices;
    }
    elapsed = now_ns() - start;
    printf("brass_stress: %s: %u active voices, %llu active samples, peak %llu, host %.2f ms\n",
           name, largest, (unsigned long long)activity, (unsigned long long)peak,
           (double)elapsed / 1e6);
    result(name, largest > 0 && largest <= ENG_BRASS.poly &&
           activity > 0 && peak > 0 && elapsed > 0);
}
int main(void)
{
    run_case("SAX clean chord", 0, 0, 0);
    run_case("SAX/TRUMPET midpoint clean chord", 0, 64, 0);
    run_case("TRUMPET clean chord", 0, 127, 0);
    run_case("SAX extreme 12-knob chord", 1, 0, 0);
    run_case("TRUMPET extreme 12-knob chord", 1, 127, 0);
    run_case("Morph change under rapid repeated chords", 1, 64, 1);
    printf("BRASS STRESS %s (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return !!failures;
}
