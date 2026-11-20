/* SPDX-License-Identifier: GPL-3.0-only
 * ATMOS: engine index, sound generation and parameter isolation regression.
 */
#define main hostsim_main
#include "hostsim.c"
#undef main
static int atmos_check(const char *msg, int yes)
{
    printf("ATMOS %-55s %s\n", msg, yes ? "PASS" : "FAIL");
    return !yes;
}
static uint64_t atmos_digest(uint32_t slot, int32_t value, uint64_t *energy)
{
    track_t *t = &trk[0];
    int32_t buf[CTL];
    uint64_t hash = 1469598103934665603ull;
    *energy = 0;
    memset(t, 0, sizeof *t);
    host_tracks_init();
    host_preset(t, ENGI_ATMOS, 0u);
    t->p[P_ATK] = 0;
    t->p[P_SUS] = 127;
    t->p[slot] = (int16_t)value;
    trk_note_on(t, 60u, 110u);
    for (uint32_t b = 0; b < 35u; b++) {
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
    uint64_t a, b, ea, eb;
    bad += atmos_check("append-only engine IDs: ATMOS 18, DRUM-AC 19, DRUM-IND 20",
                       ENGI_ATMOS == 18u && ENGI_DRUMAC == 19u &&
                       ENGI_DRUMIND == 20u && NENGINES == 21u &&
                       ENGINES[18] == &ENG_ATMOS &&
                       ENGINES[19] == &ENG_DRUMAC &&
                       ENGINES[20] == &ENG_DRUMIND);
    bad += atmos_check("12 edit controls and six presets", NELEM(ATMOS_EXTRA) == 4u &&
                       ENG_ATMOS.npresets == 6u &&
                       NELEM(ATMOS_EXTRA_PRESETS) == ENG_ATMOS.npresets);
    const uint32_t slots[] = {P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
                              AT_BREATH, AT_MOTION, AT_FLOW, AT_SPREAD};
    const int32_t lows[] = {0, 0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0};
    const int32_t highs[] = {3, 127, 127, 127, 100, 100, 127, 127, 127, 127, 127, 127};
    for (uint32_t i = 0; i < NELEM(slots); i++) {
        a = atmos_digest(slots[i], lows[i], &ea);
        b = atmos_digest(slots[i], highs[i], &eb);
        char msg[96];
        snprintf(msg, sizeof msg, "control %u changes non-silent audio", (unsigned)i);
        bad += atmos_check(msg, ea > 10000 && eb > 10000 && a != b);
    }
    printf("ATMOS ENGINE TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
