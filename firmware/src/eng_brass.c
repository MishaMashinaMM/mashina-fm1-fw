/* SPDX-License-Identifier: GPL-3.0-only
 * BRASS: compact playable synthetic wind engine, SAX and TRUMPET modes.
 *
 * Uses the existing VOICE Klatt-formant coefficient builder (fres_coef),
 * ANALOG band-limited oscillators and shared softclip/voice ADSR.
 * Two resonators instead of VOICE's four; no sample data, additional
 * RAM buffers or additional effect chain. Pitch remains the played note.
 *
 * This is an expressive electronic wind approximation, not a physical
 * reed/lip/bore waveguide simulation. VOICE's vowel and WORDS systems
 * remain unchanged.
 */
/* Extra knobs occupy existing persisted lane slots, exactly as INDUSTRIAL.
 * Neutral defaults intentionally leave the original resonant instrument dry. */
enum { BR_FAT = P_LN8, BR_GRIND = P_LN9, BR_METAL = P_LN10, BR_CRUSH = P_LN11 };
static const param_desc_t BRASS_EXTRA[4] = {
    {"FAT", F_INT, 0, 127, 0, 0, "ct"},
    {"GRIND", F_PCT, 0, 127, 0, 0, 0},
    {"METAL", F_PCT, 0, 127, 0, 0, 0},
    {"CRUSH", F_PCT, 0, 127, 0, 0, 0}
};
static const uint8_t BRASS_EXTRA_PRESETS[][4] = {
    {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0},
    {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
};
static int32_t brass_lerp(int32_t sax, int32_t trumpet, int32_t morph)
{
    return sax + (int32_t)(((int64_t)(trumpet - sax) * morph + 63) / 127);
}

static void brass_note_on(track_t *t, voice_t *v)
{
    uint32_t k;
    (void)t;
    v->ph[0] = 0;                         /* clean pitch-period start */
    v->ph[1] = 0;                         /* growl sub-oscillator */
    v->ph[2] = 0x40000000u;               /* second oscillator: fat/metal */
    for (k = 0; k < 7u; k++) v->s[k] = 0; /* two formants, breath seed, pressure */
    v->s[6] = 0x35AB731 + (int32_t)v->note * 9619;
}

static void brass_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    const int16_t *p = t->p;
    const int32_t morph = clamp(p[P_E0], 0, 127);
    const int32_t fat = clamp(p[BR_FAT], 0, 127);
    const int32_t grind = clamp(p[BR_GRIND], 0, 127);
    const int32_t metal = clamp(p[BR_METAL], 0, 127);
    const int32_t crush = clamp(p[BR_CRUSH], 0, 127);
    const int32_t pressure = clamp(p[P_E1], 0, 127);
    const int32_t buzz = clamp(p[P_E2] * 256 + m->shape - (64 << 8), 0, 127 << 8);
    const int32_t breath = clamp(p[P_E3], 0, 127);
    const int32_t body = clamp(p[P_E4], 0, 127);
    const int32_t form = clamp(p[P_E5], 0, 127);
    const int32_t growl = clamp(p[P_E6], 0, 127);
    const int32_t attack = clamp(p[P_E7], 0, 127);
    const uint32_t inc = m->inc;
    uint32_t ph = v->ph[0], subph = v->ph[1], fatph = v->ph[2], i;
    const uint32_t fatinc = fat ? fine_pitch_inc(cents_inc(m->pitch16, fat, 0), m->fine) : inc;
    const int32_t fatmix = fat * 105;  /* up to 40% detuned layer */
    const int32_t metalmix = metal * 192;
    const int32_t crushbits = crush ? 1 + crush / 20 : 0;
    int32_t y1 = v->s[0], y2 = v->s[1], y3 = v->s[2], y4 = v->s[3];
    int32_t airlp = v->s[4], pressure_env = v->s[5], nst = v->s[6];
    /* Pressure ramps toward the played breath level, rather than changing pitch.
     * ATTACK selects ~3..45 ms of smoothing at 44.1kHz. */
    const int32_t breath_target = clamp((pressure * (85 + v->vel) * 32767) / (127 * 212), 0, 32767);
    const int32_t attack_shift = 6 + attack / 24;
    const uint32_t oq = (uint32_t)brass_lerp(0x69000000, 0x58000000, morph);
    int32_t f1 = brass_lerp(780, 520, morph) +
                 (body - 64) * brass_lerp(4, 3, morph);
    int32_t f2 = brass_lerp(2180, 1390, morph) +
                 (form - 64) * brass_lerp(13, 9, morph);
    int32_t bw1 = brass_lerp(260, 190, morph);
    int32_t bw2 = brass_lerp(480, 340, morph);
    const int32_t sourcegain = brass_lerp(4, 3, morph);
    fres_t r1, r2;

    /* Fixed resonant regions with modest tracking: the harmonic excitation
     * carries the note's pitch; formant movement adds instrumental colour. */
    f1 += (m->pitch16 - 60 * 16) / 7;
    f2 += (m->pitch16 - 60 * 16) / 4;
    f1 = clamp(f1, 200, 1800);
    f2 = clamp(f2, 500, 4800);
    fres_coef(&r1, (uint32_t)f1 * 16u, (uint32_t)bw1 * 16u);
    fres_coef(&r2, (uint32_t)f2 * 16u, (uint32_t)bw2 * 16u);
    for (i = 0; i < n; i++) {
        int32_t saw, pulse, src, air, s, a, b;
        /* Breath swell is intentionally slower than the envelope's key-on,
         * while never leaving the note detuned or changing its fundamental. */
        pressure_env += (breath_target - pressure_env) >> attack_shift;
        saw = osc_saw(ph, inc);
        pulse = osc_pulse(ph, inc, (uint32_t)oq);
        subph += inc >> 1;
        fatph += fatinc;
        ph += inc;
        /* GROWL introduces gentle subharmonic roughness, without turning the
         * instrument into a noise generator at moderate knob positions. */
        src = ((saw * (32767 - (buzz >> 1))) >> 15) +
              ((pulse * (buzz >> 1)) >> 15);
        if (growl)
            src += mulq15(osc_sine(subph), growl * 75);
        src = (src * sourcegain) >> 2;
        src = mulq15(src, pressure_env);
        /* Mild air noise follows breath pressure and is smoothed slightly. */
        if (breath) {
            int32_t nz = (int32_t)(noise32(&nst) >> 17) - 16384;
            airlp += (nz - airlp) >> 2;
            air = mulq15(airlp, (breath * 215));
            src += mulq15(air, pressure_env);
        }
        src = clamp(src, -16000, 16000);
        /* VOICE's two-pole resonator recurrence, but two independent formants
         * in parallel rather than four vocal-tract filters in cascade. */
        a = (int32_t)(((int64_t)r1.a * (src << 6) +
                        (int64_t)r1.b * y1 + (int64_t)r1.c * y2 +
                        (1ll << 29)) >> 30);
        y2 = y1;
        y1 = clamp(a, -(1 << 25), (1 << 25));
        b = (int32_t)(((int64_t)r2.a * (src << 6) +
                        (int64_t)r2.b * y3 + (int64_t)r2.c * y4 +
                        (1ll << 29)) >> 30);
        y4 = y3;
        y3 = clamp(b, -(1 << 25), (1 << 25));
        /* A dry core protects fundamental/body, resonant highlights add
         * reed/brass identity. Voiced harmonics remain playable. */
        s = (src >> 1) + (y1 >> 8) + (y3 >> 8);
        /* INDUSTRIAL-inspired optional processing AFTER the instrument body.
         * All zero leaves the original SAX/TRUMPET voice path untouched. */
        if (fat)
            s += mulq15(osc_saw(fatph, fatinc) - (s >> 1), fatmix);
        if (metal)
            s += mulq15(mulq15(s, osc_sine(fatph)) - s, metalmix);
        if (grind) {
            const int32_t drive = 32768 + grind * 680;
            s = softclip(((clamp(s, -32767, 32767) >> 2) * (drive >> 2)) >> 11);
        }
        if (crushbits)
            s &= ~((1 << crushbits) - 1); /* signed-safe quantization, no extra RAM */
        s = softclip(s);
        out[i] += voice_amp(s, m, i) << 1;
    }
    v->ph[0] = ph;
    v->ph[1] = subph;
    v->ph[2] = fatph;
    v->s[0] = y1;
    v->s[1] = y2;
    v->s[2] = y3;
    v->s[3] = y4;
    v->s[4] = airlp;
    v->s[5] = pressure_env;
    v->s[6] = nst;
}

static const preset_t BRASS_PRESETS[] = {
    {"TENOR SAX", {0, 94, 78, 14, 72, 62, 14, 28}, {12, 72, 117, 48}, 4, 1 | TAG(LEAD), FX(0, 3, 24, 17), PAT(4)},
    {"BARITONE SAX", {0, 107, 67, 9, 91, 43, 18, 38}, {15, 82, 117, 57}, -2, 1 | TAG(LEAD), FX(0, 1, 13, 13), PAT(4)},
    {"SAX GROWL", {0, 108, 101, 22, 75, 70, 63, 22}, {7, 76, 120, 50}, 5, 1 | TAG(LEAD), FX(10, 0, 18, 9), PAT(4)},
    {"SOFT HORN", {127, 65, 46, 7, 51, 50, 4, 54}, {25, 80, 107, 65}, 1, 1 | TAG(LEAD), FX(0, 6, 12, 26), PAT(4)},
    {"BRASS LEAD", {127, 105, 108, 5, 72, 78, 9, 16}, {8, 75, 122, 41}, 8, 1 | TAG(LEAD), FX(4, 0, 17, 15), PAT(4)},
    {"TRUMPET", {127, 93, 86, 8, 58, 68, 5, 24}, {10, 70, 116, 44}, 7, 1 | TAG(LEAD), FX(0, 1, 17, 17), PAT(4)},
};
static const engine_t ENG_BRASS = {
    .name = "BRASS",
    .page_title = {"HORN", "BODY"},
    .edit = {
        {"TYPE", F_PCT, 0, 127, 0, 0, 0},
        {"PRESS", F_PCT, 0, 127, 94, 0, 0},
        {"BUZZ", F_PCT, 0, 127, 78, 0, 0},
        {"BREATH", F_PCT, 0, 127, 14, 0, 0},
        {"BODY", F_PCT, 0, 127, 72, 0, 0},
        {"FORMNT", F_PCT, 0, 127, 62, 0, 0},
        {"GROWL", F_PCT, 0, 127, 14, 0, 0},
        {"ATTACK", F_PCT, 0, 127, 28, 0, 0},
    },
    .presets = BRASS_PRESETS,
    .npresets = NELEM(BRASS_PRESETS),
    .note_on = brass_note_on,
    .render = brass_render,
    .knob = {P_E1, P_E2, P_E3, P_E6},
    .poly = 4,
};
