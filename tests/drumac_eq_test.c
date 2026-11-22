/* SPDX-License-Identifier: GPL-3.0-only
 * Regression for the lightweight, invisible DRUM-AC tonal contour.
 * Compare the full signal's measured RMS frequency response with GLO EQ
 * at LOW +10 / MID -7 / HIGH +3, with 20% additional MID component
 * reduction, using the actual host DSP implementations.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int eq_check(const char *name, int good)
{
    printf("drumac tone: %-68s %s\\n", name, good ? "ok" : "FAIL");
    return !good;
}
static double sine_rms_at(uint32_t hz, int contour, int reference)
{
    int32_t audio[CTL];
    eq_dsp_t global = {0};
    track_t *t = &trk[0];
    uint32_t block, k;
    double sum = 0;
    uint32_t count = 0;
    ac_tone_forget(0);
    eq_dsp_init(&global);
    global.gain[0] = 10;
    global.gain[1] = -7;
    global.gain[2] = 3;
    t->engine = t->eng_req = ENGI_DRUMAC;
    /* Run long enough to settle the 50-Hz GLO shelf as well as the
     * lightweight 171-Hz pole. Accumulate the final ~200 ms. */
    for (block = 0; block < 900u; block++) {
        for (k = 0; k < CTL; k++) {
            uint32_t i = block * CTL + k;
            audio[k] = (int32_t)(10000.0 * sin(6.283185307179586 *
                                                  (double)hz * (double)i / FS));
        }
        if (contour) ac_tone_process(t, audio, CTL);
        if (reference) eq_dsp_process(&global, 0, audio, CTL);
        if (block >= 625u)
            for (k = 0; k < CTL; k++) {
                sum += (double)audio[k] * audio[k];
                count++;
            }
    }
    return sqrt(sum / count);
}
int main(void)
{
    int bad = 0, changed = 0, untouched = 1;
    int32_t a[CTL], b[CTL], c[CTL];
    track_t *t0 = &trk[0], *t1 = &trk[1];
    int16_t global_before[3], enabled_before;
    const uint32_t frequencies[] = {30, 50, 100, 200, 400, 700, 1000, 2000, 5000, 10000, 15000};
    ui_power_on();
    memcpy(global_before, global_eq_gain, sizeof global_before);
    enabled_before = global_eq_on;
    bad += eq_check("AC tonal contour uses just two filter states per track",
                    sizeof(ac_tone_t) <= 24u);
    bad += eq_check("no new user-adjustable kit parameters", P_COUNT == 127u);
    bad += eq_check("mid-band gain 20% lower, bass/treble identical to prior filter",
                    AC_TONE_HIGH_Q12 == 5297 &&
                    AC_TONE_MID_MINUS_HIGH_Q12 == -4409 &&
                    AC_TONE_LOW_MINUS_MID_Q12 == 6647 &&
                    AC_TONE_HIGH_Q12 + AC_TONE_MID_MINUS_HIGH_Q12 + AC_TONE_LOW_MINUS_MID_Q12 == 7535);
    for (uint32_t f = 0; f < sizeof frequencies / sizeof frequencies[0]; f++) {
        double actual = sine_rms_at(frequencies[f], 1, 0);
        double target = sine_rms_at(frequencies[f], 0, 1);
        double db = 20.0 * log10(actual / target);
        int close = db > -2.8 && db < 2.8;
        char label[104];
        snprintf(label, sizeof label, "%5u Hz vs GLO +10/-7/+3: error %+.2f dB",
                 frequencies[f], db);
        bad += eq_check(label, close);
    }
    t0->engine = t0->eng_req = ENGI_DRUMAC;
    t1->engine = t1->eng_req = ENGI_DRUMLAB;
    ac_tone_forget(0); ac_tone_forget(1);
    for (uint32_t i = 0; i < CTL; i++)
        a[i] = b[i] = c[i] = (int32_t)((int32_t)i - 12) * 3100;
    ac_tone_process(t0, a, CTL);
    ac_tone_process(t1, b, CTL);
    for (uint32_t i = 0; i < CTL; i++)
        changed |= a[i] != c[i];
    untouched &= !memcmp(b, c, sizeof b);
    bad += eq_check("DRUM-AC output is audibly and numerically reshaped", changed);
    bad += eq_check("DRUM-LAB output is bit-for-bit untouched", untouched);
    bad += eq_check("non-AC track creates no tonal filter state", !ac_tone[1].ready);
    t1->engine = t1->eng_req = ENGI_DRUMAC;
    memcpy(b, c, sizeof b);
    ac_tone_process(t1, b, CTL);
    bad += eq_check("second DRUM-AC track has independent initial state",
                    !memcmp(a, b, sizeof a) && ac_tone[0].ready && ac_tone[1].ready);
    memset(b, 0, sizeof b);
    ac_tone_process(t1, b, CTL);
    bad += eq_check("silence advances the filter's residual low-frequency tail",
                    ac_tone[1].low_q12 != 0 && ac_tone[1].upper_q12 != 0);
    t1->engine = t1->eng_req = ENGI_ANALOG;
    ac_tone_process(t1, b, CTL);
    bad += eq_check("switching engines discards old tonal filter state",
                    !ac_tone[1].ready && !ac_tone[1].low_q12 &&
                    !ac_tone[1].upper_q12);
    bad += eq_check("GLO EQ is left completely independent",
                    global_eq_on == enabled_before &&
                    !memcmp(global_before, global_eq_gain, sizeof global_before));
    printf("DRUM-AC LIGHT TONAL FILTER %s (%d failures)\\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
