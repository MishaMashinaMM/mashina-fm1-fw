/* SPDX-License-Identifier: GPL-3.0-only
 * DRUM-AC's invisible built-in EQ is a single mono processing stage per
 * track, using precisely the global EQ's +10/-7/+4 coefficient tables.
 * All other engines must bypass it, and GLO EQ is independent.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void ac_test_signal(int32_t *b, uint32_t block)
{
    for (uint32_t i = 0; i < CTL; i++) {
        uint32_t n = block * CTL + i;
        b[i] = (int32_t)(22000.0 * sin(6.283185307179586 * 50.0 * n / FS) +
                         9000.0 * sin(6.283185307179586 * 700.0 * n / FS) +
                         4000.0 * sin(6.283185307179586 * 10000.0 * n / FS));
    }
}
int main(void)
{
    int bad = 0;
    const int8_t expected_gain[3] = {10, -7, 4};
    int32_t ac[CTL], reference[CTL], untouched[CTL], other[CTL];
    track_t *t0 = &trk[0], *t1 = &trk[1];
    eq_dsp_t reference_eq = {0};
    int match = 1, changed = 0, bypass = 1;
    int8_t global_before[3]; uint8_t enabled_before;

    ui_power_on();
    t0->engine = t0->eng_req = ENGI_DRUMAC;
    t1->engine = t1->eng_req = ENGI_DRUMLAB;
    eq_dsp_init(&reference_eq);
    memcpy(reference_eq.gain, expected_gain, sizeof expected_gain);
    bad += check("fixed AC EQ adds no kit controls", P_COUNT == 127u);
    memcpy(global_before, global_eq_gain, sizeof global_before);
    enabled_before = global_eq_on;

    for (uint32_t block = 0; block < 350u; block++) {
        ac_test_signal(ac, block);
        memcpy(reference, ac, sizeof ac);
        memcpy(untouched, ac, sizeof ac);
        ac_fixed_eq_process(t0, ac, CTL);
        eq_dsp_process(&reference_eq, 0u, reference, CTL);
        ac_fixed_eq_process(t1, untouched, CTL);
        match &= !memcmp(ac, reference, sizeof ac);
        changed |= memcmp(ac, untouched, sizeof ac) != 0;
        ac_test_signal(other, block);
        bypass &= !memcmp(other, untouched, sizeof other);
    }
    bad += check("built-in AC output matches global EQ's three fixed bands exactly", match);
    bad += check("AC tone actually changes with internal EQ", changed);
    bad += check("non-AC engine leaves audio bit-for-bit unchanged", bypass);
    bad += check("GLO EQ remains unaffected",
                 global_eq_on == enabled_before &&
                 !memcmp(global_before, global_eq_gain, sizeof global_before));
    bad += check("EQ gains are exactly LOW +10, MID -7, HIGH +4",
                 ac_fixed_eq[0].gain[0] == 10 &&
                 ac_fixed_eq[0].gain[1] == -7 &&
                 ac_fixed_eq[0].gain[2] == 4);
    bad += check("DRUM-LAB receives no built-in AC processing",
                 ac_fixed_eq_ready[1] == 0u);

    /* Two simultaneous DRUM-AC tracks must have independent filter histories. */
    t1->engine = t1->eng_req = ENGI_DRUMAC;
    ac_test_signal(other, 0u);
    memcpy(reference, other, sizeof other);
    eq_dsp_init(&reference_eq);
    memcpy(reference_eq.gain, expected_gain, sizeof expected_gain);
    ac_fixed_eq_process(t1, other, CTL);
    eq_dsp_process(&reference_eq, 0u, reference, CTL);
    bad += check("a second DRUM-AC track starts with independent history",
                 !memcmp(other, reference, sizeof other) &&
                 ac_fixed_eq_ready[0] && ac_fixed_eq_ready[1]);

    t1->engine = t1->eng_req = ENGI_ANALOG;
    ac_fixed_eq_process(t1, other, CTL);
    bad += check("switching to another engine releases AC filter history",
                 !ac_fixed_eq_ready[1]);
    t1->engine = t1->eng_req = ENGI_DRUMAC;
    ac_test_signal(other, 0u);
    ac_fixed_eq_process(t1, other, CTL);
    bad += check("switching back restarts the AC EQ without stale audio",
                 !memcmp(other, reference, sizeof other));

    printf("DRUM-AC FIXED EQ %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
