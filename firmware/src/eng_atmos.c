/* SPDX-License-Identifier: GPL-3.0-only
 * ATMOS: lightweight, polyphonic harmonic-pad engine.
 * Two oscillators, one existing SVF per note, no buffers or dedicated effects.
 * First DSP implementation; performance verification is required before flashing.
 */
static const char *const N_ATMOS_WAVE[] = {"SINE", "TRI", "SAW", "PULSE"};
enum { AT_BREATH = P_LN8, AT_MOTION = P_LN9, AT_FLOW = P_LN10, AT_SPREAD = P_LN11 };
static const param_desc_t ATMOS_EXTRA[4] = {
    {"BREATH", F_PCT, 0, 127, 25, 0, 0},
    {"MOTION", F_PCT, 0, 127, 30, 0, 0},
    {"FLOW", F_PCT, 0, 127, 35, 0, 0},
    {"SPREAD", F_PCT, 0, 127, 20, 0, 0}
};
static const uint8_t ATMOS_EXTRA_PRESETS[][4] = {
    {18, 22, 26, 16}, {38, 17, 55, 35}, {30, 52, 75, 20},
    {30, 44, 58, 48}, {12, 30, 42, 25}, {55, 9, 72, 40}
};
static void atmos_note_on(track_t *t, voice_t *v)
{
    (void)t;
    v->ph[1] = v->ph[0] + 0x40000000u;
    v->ph[2] = 0;
    v->s[0] = v->s[1] = 0;
    v->s[2] = 0x13579BDF + (int32_t)v->note * 179;
}
static int32_t atmos_osc(uint32_t wave, uint32_t ph, uint32_t inc)
{
    switch (wave) {
    case 0: return osc_sine(ph);
    case 1: return osc_tri(ph);
    case 2: return osc_saw(ph, inc);
    default: return osc_pulse(ph, inc, 0x80000000u);
    }
}
static void atmos_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t ph = v->ph[0], ph2 = v->ph[1], slow = v->ph[2], i;
    uint32_t inc = m->inc;
    /* SPREAD adds a controlled stereo-like beating/dispersion without extra voices.
     * Actual left/right positioning remains under the track PAN and shared FX. */
    uint32_t inc2 = fine_pitch_inc(cents_inc(m->pitch16,
                                             1 + p[P_E3] / 4 + p[AT_SPREAD] / 6, 0), m->fine);
    uint32_t wave = (uint32_t)clamp(p[P_E0], 0, 3);
    int32_t ic1 = v->s[0], ic2 = v->s[1], seed = v->s[2];
    int32_t density = p[P_E2] * 258;
    int32_t morph = p[P_E1] * 258;
    int32_t texture = p[P_E6] * 95;
    int32_t shimmer = p[P_E7] * 120;
    /* BREATH: full-scale amplitude movement at maximum, with useful midrange.
     * The old 160 multiplier allowed only ~62% attenuation at maximum. */
    int32_t breath = (int32_t)p[AT_BREATH] * 258;
    /* FLOW: allow the morph to sweep its full range, not just ~62%. */
    int32_t flow = (int32_t)p[AT_FLOW] * 258;
    /* MOTION: old range (~1200..16 seconds per cycle at 44.1 kHz)
     * made normal knob changes almost imperceptible. New range ~90..2.5s,
     * still slow enough for pads but clearly audible on sustained chords. */
    uint32_t motion_inc = (uint32_t)(13000 + (uint32_t)p[AT_MOTION] * 5300u);
    tsvf_t flt;
    tsvf_coef(&flt, ((int32_t)p[P_E4] << 8) + m->cutoff, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t a = atmos_osc(wave, ph, inc);
        int32_t b = atmos_osc((wave + 1u) & 3u, ph2, inc2);
        /* Cheap triangle movement, no per-sample trigonometry or extra LFO buffers. */
        int32_t lfo = (int32_t)(slow >> 16) - 32768;
        lfo = 32767 - (lfo < 0 ? -lfo : lfo) * 2;
        int32_t moving = (int32_t)(((int64_t)lfo * flow) >> 15);
        int32_t mix = clamp(morph + moving, 0, 32767);
        int32_t s = mulq15(a, 32767 - mix) + mulq15(b, mix);
        s = mulq15(s, 32767 - density / 2) + mulq15(b, density / 2);
        if (texture) s += mulq15((int32_t)(noise32(&seed) >> 17) - 16384, texture);
        if (shimmer) s += mulq15(osc_sine(ph + (ph << 1)), shimmer);
        s = tsvf_lp(&flt, s >> 1, &ic1, &ic2) << 1;
        /* Unipolar amplitude sweep: at BREATH=0 unity, at 127 near silence
         * at the trough. Keep the upper end bounded to avoid sign inversion. */
        int32_t attenuation = ((lfo + 32767) * breath) >> 16;
        s = mulq15(s, 32767 - clamp(attenuation, 0, 32767));
        out[i] += voice_amp(s, m, i) << 1;
        ph += inc;
        ph2 += inc2;
        slow += motion_inc;
    }
    v->ph[0] = ph;
    v->ph[1] = ph2;
    v->ph[2] = slow;
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = seed;
}
static const preset_t ATMOS_PRESETS[] = {
    {"SOFT CLOUD", {0, 25, 45, 12, 83, 10, 0, 12}, {65, 90, 112, 95}, 10, TAG(PAD), FX(0, 30, 25, 55), PAT(5)},
    {"BREATHING", {1, 55, 55, 14, 70, 20, 12, 22}, {80, 90, 115, 110}, 10, TAG(PAD), FX(0, 35, 25, 60), PAT(5)},
    {"HARP PLUCK", {1, 34, 48, 7, 91, 12, 2, 37}, {0, 25, 12, 32}, 22, TAG(PLUCK), FX(0, 20, 24, 32), PAT(3)},
    {"GLASS TIDE", {2, 70, 40, 18, 96, 35, 10, 48}, {50, 85, 108, 95}, 12, TAG(PAD), FX(0, 45, 35, 70), PAT(5)},
    {"DUSK STRINGS", {1, 30, 80, 10, 60, 15, 4, 15}, {70, 90, 110, 105}, 8, TAG(PAD), FX(0, 40, 20, 60), PAT(5)},
    {"SLOW NEBULA", {3, 65, 60, 20, 75, 40, 20, 35}, {85, 100, 120, 115}, 12, TAG(PAD), FX(0, 40, 30, 75), PAT(5)}
};
static const engine_t ENG_ATMOS = {
    .name = "ATMOS",
    .page_title = {"HARM", "TONE"},
    .edit = {
        {"WAVE", F_ENUM, 0, 3, 0, N_ATMOS_WAVE, 0},
        {"MORPH", F_PCT, 0, 127, 25, 0, 0},
        {"DENSITY", F_PCT, 0, 127, 45, 0, 0},
        {"DRIFT", F_PCT, 0, 127, 12, 0, 0},
        {"CUT", F_CUTOFF, 0, 127, 83, 0, 0},
        {"RES", F_PCT, 0, 127, 10, 0, 0},
        {"TEXTURE", F_PCT, 0, 127, 0, 0, 0},
        {"SHIMMER", F_PCT, 0, 127, 12, 0, 0}
    },
    .presets = ATMOS_PRESETS,
    .npresets = NELEM(ATMOS_PRESETS),
    .note_on = atmos_note_on,
    .render = atmos_render,
    .knob = {P_E1, P_E4, P_LN8, P_LN9}
};
