/* SPDX-License-Identifier: GPL-3.0-only
 * INDUSTRIAL: playable dual-oscillator bass and lead synth.
 * Reuses Felucca's band-limited oscillators, tanh softclip and trapezoidal SVF.
 * No dynamically allocated state, samples, additional voices or effect buses.
 * Feedback is bounded, and drive uses a pre-scaled 32-bit multiply.
 */
static const char *const N_IND_WAVE[] = {"SAW", "PULSE", "SAW+SQR", "RING", "TRI", "SQR"};
/* Extra controls share four existing DRUM-only parameter IDs, never used for
 * industrial synthesis by other engines. No persistent data layout changes. */
enum { IND_WAVE2 = P_LN8, IND_MIX = P_LN9, IND_SUB = P_LN10, IND_MODE = P_LN11 };
static const char *const N_IND_WAVE2[] = {"FOLLOW", "SAW", "SQR", "TRI", "SIN", "PULSE"};
static const char *const N_IND_MODE[] = {"LP", "HP", "BP"};
static const param_desc_t INDUSTRIAL_EXTRA[4] = {
    {"WAVE2", F_ENUM, 0, 5, 0, N_IND_WAVE2, 0},
    {"MIX", F_PCT, 0, 127, 25, 0, 0},
    {"SUB", F_PCT, 0, 127, 32, 0, 0},
    {"MODE", F_ENUM, 0, 2, 0, N_IND_MODE, 0},
};
/* Corresponding factory preset defaults in unchanged preset index order. */
static const uint8_t INDUSTRIAL_EXTRA_PRESETS[][4] = {
    {0, 21, 32, 0}, {0, 17, 32, 0}, {0, 31, 32, 0},
    {0, 28, 32, 0}, {0, 38, 32, 0}, {0, 10, 32, 0},
};


static void industrial_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[1] = v->ph[0] + 0x40000000u; /* fat oscillator starts out of phase */
    v->ph[2] = v->ph[0] >> 1;
    v->s[0] = v->s[1] = 0;             /* SVF integrators */
    v->s[2] = 0;                       /* bounded previous sample feedback */
    v->s[3] = 0;                       /* sample hold for CRUSH */
    v->s[4] = 0;                       /* decimation counter */
}

static void industrial_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t wave = (uint32_t)p[P_E0], inc = m->inc, i;
    /* FAT is 0..127 cents. A nonzero FAT adds a second band-limited oscillator.
     * METAL progressively adds a pitch-related ring component rather than noise. */
    uint32_t inc2 = p[P_E1] ? cents_inc(m->pitch16, p[P_E1], 0) : inc;
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], subph = v->ph[2];
    int32_t ic1 = v->s[0], ic2 = v->s[1], prev = v->s[2];
    int32_t held = v->s[3], count = v->s[4];
    int32_t mix = (int32_t)p[IND_MIX] * 258;       /* oscillator 2 balance, 0..32766 Q15 */
    int32_t metal = (int32_t)p[P_E3] * 220;           /* 0..27940 Q15 */
    int32_t feed = (int32_t)p[P_E5] * 160;            /* 0..20320 Q15 */
    int32_t gain = 32768 + (int32_t)p[P_E2] * 700;   /* 1..3.7x, applied to half-scale */
    int32_t holdstep = 1 + (int32_t)p[P_E4] / 18;    /* 1..8 samples, bounded */
    int32_t submix = (int32_t)p[IND_SUB] * 258;
    uint32_t w2 = (uint32_t)clamp(p[IND_WAVE2], 0, 5);
    uint32_t mode = (uint32_t)clamp(p[IND_MODE], 0, 2);
    int32_t fk = 8192 - (int32_t)p[P_E7] * 7600 / 127;
    int32_t cut = ((int32_t)p[P_E6] << 8) + m->cutoff;
    tsvf_t flt;
    tsvf_coef(&flt, cut, p[P_E7]);
    for (i = 0; i < n; i++) {
        int32_t a, b, s, ring;
        switch (wave) {
        case 1: a = osc_pulse(ph0, inc, 0x50000000u); break;
        case 2: a = osc_saw(ph0, inc); break;
        case 3: a = osc_saw(ph0, inc); break;
        case 4: a = osc_tri(ph0); break;
        case 5: a = osc_pulse(ph0, inc, 0x80000000u); break;
        default: a = osc_saw(ph0, inc); break;
        }
        switch (w2) {
        case 1: b = osc_saw(ph1, inc2); break;
        case 2: b = osc_pulse(ph1, inc2, 0x80000000u); break;
        case 3: b = osc_tri(ph1); break;
        case 4: b = osc_sine(ph1); break;
        case 5: b = osc_pulse(ph1, inc2, 0x50000000u); break;
        default: /* FOLLOW reproduces the previous engine's paired waveform. */
            b = wave == 1 ? osc_pulse(ph1, inc2, 0x50000000u) :
                wave == 2 ? osc_pulse(ph1, inc2, 0x80000000u) :
                wave == 3 ? osc_sine(ph1) :
                wave == 4 ? osc_tri(ph1) :
                wave == 5 ? osc_pulse(ph1, inc2, 0x80000000u) :
                osc_saw(ph1, inc2);
            break;
        }
        ph0 += inc;
        ph1 += inc2;
        subph += inc >> 1;
        /* Main oscillator stays dominant. A quiet sub keeps the low octave present. */
        s = a + mulq15(b - a, mix);
        s = mulq15(s, 32767 - submix) + mulq15(osc_sine(subph), submix);
        ring = mulq15(a, b);
        s += mulq15(ring - s, metal);
        /* Feedback is pre-filtered and limited so that it cannot grow unbounded. */
        s = clamp(s + mulq15(prev, feed), -32767, 32767);
        s = softclip(((s >> 2) * (gain >> 2)) >> 11);
        prev = s;
        if (count-- <= 0) {
            held = s;
            count = holdstep - 1;
        }
        s = held;
        /* Existing trapezoidal SVF plus post-filter soft saturation. */
        s = svf_mode(&flt, s >> 1, &ic1, &ic2, mode, fk) << 1;
        s = softclip(s);
        out[i] += voice_amp(s, m, i) << 1;
    }
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->ph[2] = subph;
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = prev;
    v->s[3] = held;
    v->s[4] = count;
}

static const preset_t INDUSTRIAL_PRESETS[] = {
    {"IRON BASS", {0, 30, 70, 8, 0, 12, 72, 25}, {0, 62, 95, 24}, 18, 1 | TAG(BASS), FX(0, 0, 9, 7), PAT(2)},
    {"STEEL LEAD", {2, 24, 84, 25, 0, 14, 95, 18}, {3, 75, 110, 35}, 12, 1 | TAG(LEAD), FX(6, 0, 19, 14), PAT(4)},
    {"GRIND BASS", {0, 45, 108, 24, 0, 22, 66, 40}, {0, 68, 90, 26}, 20, 1 | TAG(BASS), FX(0, 0, 7, 5), PAT(2)},
    {"GEAR LEAD", {3, 40, 80, 64, 0, 12, 90, 30}, {2, 66, 100, 38}, 10, 1 | TAG(LEAD), FX(5, 0, 18, 15), PAT(4)},
    {"BROKEN AMP", {5, 55, 115, 20, 35, 40, 80, 20}, {2, 75, 95, 38}, 15, 1 | TAG(LEAD), FX(0, 0, 12, 8), PAT(4)},
    {"SUB MACHINE", {4, 15, 54, 0, 0, 0, 58, 10}, {0, 70, 110, 26}, 12, 1 | TAG(BASS), FX(0, 0, 5, 4), PAT(2)},
};

static const engine_t ENG_INDUSTRIAL = {
    .name = "INDUSTRIAL",
    .page_title = {"CORE", "GRIT"},
    .edit = {
        {"WAVE", F_ENUM, 0, 5, 0, N_IND_WAVE, 0},
        {"FAT", F_INT, 0, 127, 30, 0, "ct"},
        {"GRIND", F_PCT, 0, 127, 70, 0, 0},
        {"METAL", F_PCT, 0, 127, 8, 0, 0},
        {"CRUSH", F_PCT, 0, 127, 0, 0, 0},
        {"FEED", F_PCT, 0, 127, 12, 0, 0},
        {"CUT", F_CUTOFF, 0, 127, 72, 0, 0},
        {"RES", F_PCT, 0, 127, 25, 0, 0},
    },
    .presets = INDUSTRIAL_PRESETS,
    .npresets = NELEM(INDUSTRIAL_PRESETS),
    .note_on = industrial_note_on,
    .render = industrial_render,
    .knob = {P_E2, P_E6, P_E1, P_E3},
};
