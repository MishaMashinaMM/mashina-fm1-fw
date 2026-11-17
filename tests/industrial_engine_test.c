/* SPDX-License-Identifier: GPL-3.0-only
 * INDUSTRIAL engine: new third edit page's stored parameters must change audio.
 * No extra per-track memory and no changes to the legacy eight edit slots. */
#define main hostsim_main
#include "hostsim.c"
#undef main

static int check(const char *msg, int yes)
{
    printf("INDUSTRIAL %-55s %s\n", msg, yes ? "PASS" : "FAIL");
    return !yes;
}
static uint64_t digest(uint32_t slot, int32_t value, uint64_t *energy)
{
    track_t *t = &trk[0];
    int32_t buf[CTL];
    uint64_t hash = 1469598103934665603ull;
    *energy = 0;
    memset(t, 0, sizeof *t);
    host_tracks_init();
    host_preset(t, ENGI_INDUSTRIAL, 0u);
    t->p[P_ATK] = 0;
    t->p[P_SUS] = 127;
    t->p[slot] = (int16_t)value;
    trk_note_on(t, 45u, 110u);
    for (uint32_t b = 0; b < 28u; b++) {
        track_render(t, buf, CTL);
        for (uint32_t i = 0; i < CTL; i++) {
            uint32_t v = (uint32_t)buf[i];
            hash = (hash ^ v) * 1099511628211ull;
            *energy += (uint64_t)(buf[i] < 0 ? -(int64_t)buf[i] : buf[i]);
        }
    }
    return hash;
}
int main(void)
{
    int bad = 0;
    uint64_t ea, eb, a, b;
    bad += check("append-only ID 16", ENGI_INDUSTRIAL == 16u &&
                 ENGINES[16] == &ENG_INDUSTRIAL);
    bad += check("eight standard + four independent stored parameters",
                 P_LN11 == P_COUNT - 1 && NELEM(INDUSTRIAL_EXTRA) == 4u &&
                 ENG_INDUSTRIAL.npresets == NELEM(INDUSTRIAL_EXTRA_PRESETS));
    bad += check("new descriptors fit existing stored slots",
                 INDUSTRIAL_EXTRA[0].max == 5 && INDUSTRIAL_EXTRA[3].max == 2);
    a = digest(IND_WAVE2, 0, &ea);
    b = digest(IND_WAVE2, 4, &eb);
    bad += check("WAVE2 changes playable oscillator timbre", ea > 10000 && eb > 10000 && a != b);
    a = digest(IND_MIX, 0, &ea);
    b = digest(IND_MIX, 127, &eb);
    bad += check("MIX controls oscillator balance", ea > 10000 && eb > 10000 && a != b);
    a = digest(IND_SUB, 0, &ea);
    b = digest(IND_SUB, 127, &eb);
    bad += check("SUB independently controls low octave", ea > 10000 && eb > 10000 && a != b);
    a = digest(IND_MODE, 0, &ea);
    b = digest(IND_MODE, 2, &eb);
    bad += check("MODE switches low-pass and band-pass", ea > 10000 && eb > 10000 && a != b);
    printf("INDUSTRIAL 12-PARAMETER TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
