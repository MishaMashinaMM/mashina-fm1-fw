/* SPDX-License-Identifier: GPL-3.0-only
 * Per-track FINE must audibly change melodic rendering while leaving
 * note ownership and DRUM-X/DRUM-LAB tuning unaffected. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int test_check(const char *name, int ok)
{
    printf("TUNING %-52s %s\n", name, ok ? "PASS" : "FAIL");
    return !ok;
}
static uint64_t fine_digest(uint32_t eng, int8_t cents, uint64_t *energy)
{
    track_t *t = &trk[0];
    int32_t audio[CTL];
    uint64_t h = 1469598103934665603ull;
    *energy = 0;
    memset(trk, 0, sizeof trk);
    memset(&song, 0, sizeof song);
    memset(midi_fine, 0, sizeof midi_fine);
    /* DRUM-LAB's voice/patch caches persist outside track_t; reset so
     * two renders differ only in FINE, never in a previous pad tail. */
    memset(drumlab_state, 0, sizeof drumlab_state);
    memset(drumlab_kit_ready, 0, sizeof drumlab_kit_ready);
    host_tracks_init();
    host_preset(t, eng, 0);
    t->p[P_ATK] = 0;
    t->p[P_SUS] = 127;
    t->p[P_DRIFT] = 127;
    midi_fine[0] = cents;
    trk_note_on(t, 60u, 110u);
    for (uint32_t b = 0; b < 48u; b++) {
        track_render(t, audio, CTL);
        for (uint32_t i = 0; i < CTL; i++) {
            int64_t sample = audio[i];
            h = (h ^ (uint32_t)sample) * 1099511628211ull;
            *energy += sample < 0 ? (uint64_t)-sample : (uint64_t)sample;
        }
    }
    return h;
}
int main(void)
{
    int bad = 0;
    uint64_t e0, ep, em;
    uint64_t a, b, c;
    const uint32_t melodic[3] = {ENGI_ANALOG, ENGI_INDUSTRIAL, ENGI_BRASS};
    for (uint32_t i = 0; i < 3u; i++) {
        a = fine_digest(melodic[i], 0, &e0);
        b = fine_digest(melodic[i], 50, &ep);
        c = fine_digest(melodic[i], -50, &em);
        bad += test_check(i == 0 ? "ANALOG: ±50 cents changes rendered samples" :
                          i == 1 ? "INDUSTRIAL: ±50 cents changes rendered samples" :
                                   "BRASS: ±50 cents changes rendered samples",
                          e0 > 1000 && ep > 1000 && em > 1000 && a != b && a != c && b != c);
    }
    a = fine_digest(ENGI_DRUM, 0, &e0);
    b = fine_digest(ENGI_DRUM, 50, &ep);
    bad += test_check("DRUM-X unaffected by per-track FINE", a == b);
    a = fine_digest(ENGI_DRUMLAB, 0, &e0);
    b = fine_digest(ENGI_DRUMLAB, -50, &em);
    bad += test_check("DRUM-LAB unaffected by per-track FINE", a == b);
    printf("TUNING PITCH TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
