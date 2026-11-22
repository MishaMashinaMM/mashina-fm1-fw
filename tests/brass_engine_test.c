/* SPDX-License-Identifier: GPL-3.0-only
 * BRASS synth regression: SAX/TRUMPET and each key control must affect sound.
 * Includes all existing engines; no new static voice memory is allocated.
 */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int check(const char *label, int ok)
{
    printf("BRASS %-51s %s\n", label, ok ? "PASS" : "FAIL");
    return !ok;
}
static uint64_t digest(uint32_t param, int16_t value, uint64_t *energy)
{
    track_t *t = &trk[0];
    int32_t audio[CTL];
    uint64_t hash = 1469598103934665603ull;
    *energy = 0;
    memset(t, 0, sizeof *t);
    host_tracks_init();
    host_preset(t, ENGI_BRASS, 0u);
    t->p[P_ATK] = 0;
    t->p[P_SUS] = 127;
    if (param < P_COUNT) t->p[param] = value;
    trk_note_on(t, 62, 105);
    for (uint32_t b = 0; b < 38u; b++) {
        track_render(t, audio, CTL);
        for (uint32_t i = 0; i < CTL; i++) {
            int32_t v = audio[i];
            hash = (hash ^ (uint32_t)v) * 1099511628211ull;
            *energy += v < 0 ? (uint64_t)-(int64_t)v : (uint64_t)v;
        }
    }
    return hash;
}
int main(void)
{
    int bad = 0;
    uint64_t ea, eb, a, b;
    bad += check("append-only engine ID 17", ENGI_BRASS == 17u &&
                 ENGINES[ENGI_BRASS] == &ENG_BRASS);
    bad += check("VOICE resonators reused, 4-voice cap, five presets",
                 ENG_BRASS.poly == 4 && ENG_BRASS.npresets == 5u);
    a = digest(P_E0, 0, &ea);
    b = digest(P_E0, 127, &eb);
    bad += check("SAX/TRUMPET endpoints have distinct timbres", a != b && ea > 100000 && eb > 100000);
    {
        uint64_t mid = digest(P_E0, 64, &eb);
        bad += check("TYPE midpoint is a genuinely distinct tone", mid != a && mid != b);
    }
    a = digest(P_E1, 35, &ea);
    b = digest(P_E1, 115, &eb);
    bad += check("PRESS changes playing intensity", a != b && eb > ea);
    a = digest(P_E2, 15, &ea);
    b = digest(P_E2, 120, &eb);
    bad += check("BUZZ changes harmonic excitation", a != b);
    a = digest(P_E3, 0, &ea);
    b = digest(P_E3, 100, &eb);
    bad += check("BREATH affects audio", a != b);
    a = digest(P_E4, 0, &ea);
    b = digest(P_E4, 127, &eb);
    bad += check("BODY moves lower resonant formant", a != b);
    a = digest(P_E5, 0, &ea);
    b = digest(P_E5, 127, &eb);
    bad += check("FORMNT moves upper resonant formant", a != b);
    a = digest(P_E6, 0, &ea);
    b = digest(P_E6, 100, &eb);
    bad += check("GROWL introduces subharmonic", a != b);
    a = digest(P_E7, 0, &ea);
    b = digest(P_E7, 127, &eb);
    bad += check("ATTACK changes breath swell", a != b);
    a = digest(BR_FAT, 0, &ea);
    b = digest(BR_FAT, 100, &eb);
    bad += check("EDIT 3 FAT changes oscillator weight", a != b);
    a = digest(BR_GRIND, 0, &ea);
    b = digest(BR_GRIND, 100, &eb);
    bad += check("EDIT 3 GRIND changes saturated timbre", a != b);
    a = digest(BR_METAL, 0, &ea);
    b = digest(BR_METAL, 100, &eb);
    bad += check("EDIT 3 METAL changes ring harmonics", a != b);
    a = digest(BR_CRUSH, 0, &ea);
    b = digest(BR_CRUSH, 100, &eb);
    bad += check("EDIT 3 CRUSH changes quantized output", a != b);
    printf("BRASS ENGINE TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
