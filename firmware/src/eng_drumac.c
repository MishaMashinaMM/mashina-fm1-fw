/* SPDX-License-Identifier: GPL-3.0-only
 * MASHINA DRUM-AC: first acoustic percussion milestone.
 * Independent kick, snare and crash models, all generated in fixed point.
 * No sample data, no dependence on DRUM-X/DRUM-LAB patches.
 * Twelve individually editable sounds in one repeated chromatic octave.
 * Any MIDI note triggers the instrument at the same semitone in C2..B2.
 */
enum { AC_KICK, AC_SNARE, AC_CRASH, AC_HAT_C, AC_HAT_O,
       AC_FLOOR, AC_TOM_L, AC_TOM_M, AC_TOM_H, AC_CLAP,
       AC_RIDE_BELL, AC_RIDE_SPLASH, AC_NPAD };
/* Keep pad indices stable for project compatibility. The canonical octave
 * covers each semitone exactly once, so the keyboard can repeat the entire
 * 12-pad kit at every octave (the same behavior as DRUM-LAB). */
static const uint8_t AC_NOTES[AC_NPAD] = {
    36, 38, 37, 42, 46, 41, 43, 45, 47, 39, 40, 44
};
typedef struct {
    uint32_t phase[3], inc, seed, age;
    int32_t body, transient, noise_lp, noise_fast, wires_lp;
    uint8_t kind, active;
} ac_voice_t;
static ac_voice_t ac_state[NPART][NVOICE];
/* Three damped, independently noise-excited cymbal-plate bandpass modes.
 * State lives in POOL, not the FM1's tightly constrained audio .bss. */
static int32_t ac_plate_band[NPART][NVOICE][3] __attribute__((section(".pool")));
static uint8_t ac_patch[NPART][AC_NPAD][8] __attribute__((section(".pool")));
static uint8_t ac_ready[NPART];
static uint16_t ac_dirty[NPART]; /* 12 pad mask, sparse project storage */
static uint8_t ac_override[NPART][AC_NPAD] __attribute__((section(".pool"))); /* one bit per overridden field */

static void ac_patch_ensure(track_t *t)
{
    uint32_t ti = (uint32_t)(t - trk), k, j;
    if (ti >= NPART || ac_ready[ti]) return;
    for (k = 0; k < AC_NPAD; k++)
        for (j = 0; j < 8u; j++)
            ac_patch[ti][k][j] = (uint8_t)clamp(t->p[P_E0 + j], 0, 127);
    /* Give the acoustic snare a membrane-first default rather than a
     * maximum-rattle starting point. All other saved edits stay untouched. */
    ac_patch[ti][AC_SNARE][4] = 43u;
    ac_ready[ti] = 1u;
}
static uint8_t *ac_patch_of(track_t *t, uint32_t instrument)
{
    if ((uint32_t)(t - trk) >= NPART || instrument >= AC_NPAD) return 0;
    ac_patch_ensure(t);
    return ac_patch[t - trk][instrument];
}
static void ac_patch_reset(track_t *t)
{
    uint32_t ti = (uint32_t)(t - trk);
    if (ti >= NPART) return;
    ac_ready[ti] = 0;
    ac_dirty[ti] = 0;
    memset(ac_override[ti], 0, sizeof ac_override[ti]);
}
static uint8_t ac_param_value(track_t *t, uint32_t instrument, uint32_t field)
{
    uint8_t *patch = ac_patch_of(t, instrument);
    if (!patch || field >= 8u) return 0u;
    /* Kit-wide acoustic controls are a separate DSP stage. They must never
     * multiply or rewrite a pad's independently editable 0..127 fields. */
    return patch[field];
}
static void ac_patch_set(track_t *t, uint32_t instrument, uint32_t field, uint32_t value)
{
    uint8_t *p = ac_patch_of(t, instrument);
    if (!p || field >= 8u) return;
    /* Pad edits are always stored directly; global tone, damping etc.
     * are applied independently during synthesis. */
    p[field] = (uint8_t)clamp(value, 0, 127);
    ac_override[t - trk][instrument] |= (uint8_t)(1u << field);
    ac_dirty[t - trk] |= (uint16_t)(1u << instrument);
}


static uint32_t ac_random(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return x;
}
static uint32_t ac_kind(uint32_t note)
{
    uint32_t i, slot = 36u + note % 12u;
    for (i = 0; i < AC_NPAD; i++)
        if (slot == AC_NOTES[i]) return i;
    return AC_NPAD;
}
static uint32_t ac_is_tom(uint32_t kind)
{
    return kind >= AC_FLOOR && kind <= AC_TOM_H;
}
static uint32_t ac_is_metal(uint32_t kind)
{
    return kind == AC_CRASH || kind == AC_RIDE_BELL || kind == AC_RIDE_SPLASH;
}
/* Centre newly extended modulation at the preset's original field value.
 * Its 0 delta produces precisely the existing sound; projects and individual
 * pad overrides keep the same stored values and MIDI behaviour. */
static int32_t ac_mod_delta(track_t *t, uint32_t kind, uint32_t field, int32_t value)
{
    int32_t origin = kind == AC_SNARE && field == 4u ?
                     43 : clamp(t->p[P_E0 + field], 0, 127);
    return value - origin;
}
static void ac_note_on(track_t *t, voice_t *v)
{
    uint32_t slot = (uint32_t)(v - t->v);
    ac_voice_t *a;
    if (slot >= NVOICE) return;
    a = &ac_state[t - trk][slot];
    memset(a, 0, sizeof *a);
    if (ac_is_metal(ac_kind(v->note)))
        memset(ac_plate_band[t - trk][slot], 0, sizeof ac_plate_band[0][0]);
    if (ac_kind(v->note) == AC_NPAD) return;
    a->kind = (uint8_t)ac_kind(v->note);
    a->seed = 0xB5297A4Du ^ ((uint32_t)v->note << 18) ^ (uint32_t)v->age;
    if (!a->seed) a->seed = 1;
    /* Only CRASH needs oscillator phases. KICK/SNARE use phase[0..1]
     * as two signed, noise-driven filter memories; neither has a pitch. */
    a->inc = 0;  /* Plate filters are noise-excited, not pitched oscillators. */
    a->body = a->transient = 1 << 24;
    a->active = 1;
}
static int32_t ac_amp(track_t *t, voice_t *v, int32_t adsr)
{
    uint32_t slot = (uint32_t)(v - t->v);
    if (slot >= NVOICE || !ac_state[t - trk][slot].active) {
        v->active = v->gate = 0;
        v->stage = 0;
        v->env = 0;
        return 0;
    }
    if (v->stage == 4u) return adsr;
    v->env = 1 << 24;
    return 32767;
}
/* Noise-excited transient models for acoustic-style KICK and SNARE.
 * Their low and mid bodies are filtered aperiodic noise: no sine, pitch
 * sweep, repeating phase, or ring oscillator. TUNE sets body filter colour,
 * not a musical pitch. CRASH retains its separate metallic oscillators.
 * Decays are calculated once per audio block. */
static void ac_render(track_t *t, voice_t *v, int32_t *out,
                      uint32_t n, const vmod_t *m)
{
    uint32_t slot = (uint32_t)(v - t->v), i, ph0, ph1, ph2, rng, age;
    int32_t body, trans, step_b, step_t, lp, fast, wire;
    uint32_t tau_b, tau_t, mul_b, mul_t, kind, lid, tom_tone_inc = 0;
    int32_t *plate_bands = 0, plate_coef[3] = {0, 0, 0};
    int32_t plate_low[3] = {0, 0, 0}, plate_damp = 0;
    int32_t level_gain;
    int32_t tune, damp, strike, wire_amt, bright, metal, body_mix, ring_amt;
    int32_t mod_delta[8], kit[8];
    uint32_t mod_render, kit_timbre;
    ac_voice_t *a;
    if (slot >= NVOICE || !n) return;
    a = &ac_state[t - trk][slot];
    if (!a->active) return;
    kind = a->kind;
    /* All three instrument patches are independent; changing one pad never
     * changes the others. Initial values exactly match the old global kit. */
    if (!ac_patch_of(t, kind)) return;
    tune = ac_param_value(t, kind, 0);
    damp = ac_param_value(t, kind, 1);
    body_mix = ac_param_value(t, kind, 2);
    strike = ac_param_value(t, kind, 3);
    wire_amt = ac_param_value(t, kind, 4);
    bright = ac_param_value(t, kind, 5);
    metal = ac_param_value(t, kind, 6);
    ring_amt = ac_param_value(t, kind, 7);
    /* All eight kit controls are centred at 100. Old project defaults
     * therefore retain their sound exactly, while 0..200 permits either
     * a decrease or an increase. Order:
     * TONE, DAMP, BODY, ATTACK, BRIGHT, TEXTURE, RING, LEVEL. */
    drumlab_global_ensure((uint32_t)(t - trk));
    {
        uint32_t field;
        for (field = 0; field < 8u; field++)
            kit[field] = clamp((int32_t)drumlab_global[t - trk][field], 0, 200) - 100;
    }
    kit_timbre = kit[0] || kit[2] || kit[3] || kit[4] || kit[5] || kit[6];
    /* Precompute signed distances once per render block, not per sample. */
    {
        int32_t values[8] = {tune, damp, body_mix, strike,
                             wire_amt, bright, metal, ring_amt};
        uint32_t field;
        for (field = 0u; field < 8u; field++)
            mod_delta[field] = ac_mod_delta(t, kind, field, values[field]);
    }
    /* If no per-pad change is active, skip the additional DSP completely:
     * the approved default kit remains the original render path. */
    mod_render = (mod_delta[0] || mod_delta[2] || mod_delta[3] ||
                  mod_delta[4] || mod_delta[5] || mod_delta[6] ||
                  mod_delta[7]);
    lid = kind < 8u ? P_LN0 + kind : P_LN8 + kind - 8u;
    {   /* Quadratic pad LEVEL, identical to DRUM-LAB; calculated per block.
         * Global LEVEL is an independent 0..200% gain, 100% unchanged. */
        int32_t level = clamp(t->p[lid], 0, 127);
        int32_t pad_gain = level == 127 ? 32768 : level * level * 32768 / 16129;
        level_gain = (int32_t)(((int64_t)pad_gain * (kit[7] + 100)) / 100);
    }
    ph0 = a->phase[0]; ph1 = a->phase[1]; ph2 = a->phase[2];
    if (ac_is_metal(kind)) {
        /* Inharmonic plate modes: different fixed central bands for bow,
         * bell and crash. TUNE moves the bands, not a musical fundamental.
         * Coefficients run in a bounded, stable part of the SVF range. */
        int32_t colour = (tune - 64) * 18;
        plate_coef[0] = clamp(6000 + colour, 3000, 10500);
        plate_coef[1] = clamp(12200 + colour * 2, 6500, 19000);
        plate_coef[2] = clamp(19000 + colour * 2, 13000, 26000);
        plate_damp = kind == AC_RIDE_BELL ? 6800 : 15000;
        plate_bands = ac_plate_band[t - trk][slot];
        plate_low[0] = (int32_t)ph0;
        plate_low[1] = (int32_t)ph1;
        plate_low[2] = (int32_t)ph2;
    }
    if (ac_is_tom(kind)) {
        /* TUNE=0 removes all pitched resonance. TUNE=1 starts very low,
         * then rises continuously across nearly five octaves. A slight
         * per-tom offset preserves distinct natural sizes, but every tom
         * can be tuned independently across almost the entire range. */
        if (tune > 0) {
            int32_t pitch16 = clamp(27 * 16 + (tune - 1) * 6 +
                                  (int32_t)(kind - AC_FLOOR) * 32,
                                  26 * 16, 83 * 16);
            tom_tone_inc = pitch_inc((uint32_t)pitch16);
        }
    }
    tau_b = kind == AC_KICK ? 160000u + (uint32_t)body_mix * 2800u :
            kind == AC_SNARE ? 80000u + (uint32_t)body_mix * 2100u :
            kind == AC_HAT_C ? 13000u + (uint32_t)body_mix * 700u :
            kind == AC_HAT_O ? 150000u + (uint32_t)body_mix * 4300u :
            ac_is_tom(kind) ? 110000u + (uint32_t)body_mix * 3200u :
            kind == AC_CLAP ? 50000u + (uint32_t)body_mix * 1400u :
            kind == AC_RIDE_BELL ? 250000u + (uint32_t)body_mix * 11000u :
            kind == AC_RIDE_SPLASH ? 180000u + (uint32_t)body_mix * 8500u :
                               380000u + (uint32_t)body_mix * 16000u;
    tau_b = tau_b * (uint32_t)(180 - damp) / 116u;
    /* Kit DAMP shortens/extends the envelope, while kit RING adjusts the
     * lingering resonance separately; neutral 100% changes nothing. */
    {
        int32_t damp_factor = clamp(256 - kit[1] * 2 + kit[6], 48, 512);
        tau_b = (uint32_t)(((uint64_t)tau_b * (uint32_t)damp_factor) >> 8);
    }
    /* Stronger tail/damp shaping, centred on the current factory preset.
     * At a zero delta the original decay is bit-for-bit unchanged. */
    {
        int32_t decay_factor = clamp(256 - mod_delta[1] * 3 +
                                   mod_delta[7] * (ac_is_metal(kind) || kind == AC_HAT_O ? 2 : 1),
                                   48, 576);
        tau_b = (uint32_t)(((uint64_t)tau_b * (uint32_t)decay_factor) >> 8);
    }
    if (tau_b < 25000u) tau_b = 25000u;
    tau_t = ac_is_metal(kind) ? 85000u :
            kind == AC_SNARE ? 22000u : kind == AC_CLAP ? 30000u : 9500u;
    mul_b = dlab_decay_long(n, tau_b);
    mul_t = dlab_decay_long(n, tau_t);
    body = a->body; trans = a->transient;
    step_b = ((int32_t)(((int64_t)body * mul_b) >> 16) - body) / (int32_t)n;
    step_t = ((int32_t)(((int64_t)trans * mul_t) >> 16) - trans) / (int32_t)n;
    rng = a->seed; age = a->age;
    lp = a->noise_lp; fast = a->noise_fast; wire = a->wires_lp;
    for (i = 0; i < n; i++, body += step_b, trans += step_t, age++) {
        int32_t white = (int32_t)(ac_random(&rng) >> 16) - 32768;
        int32_t raw;
        /* Noise filters are bounded and share no state between drum voices. */
        lp += (int32_t)(((int64_t)(white - lp) * (800 + bright * 45)) >> 15);
        fast += (int32_t)(((int64_t)(white - fast) * (7800 + bright * 115)) >> 15);
        wire += (int32_t)(((int64_t)(white - wire) * (1800 + bright * 65)) >> 15);
        if (kind == AC_KICK) {
            /* Two cascaded low-pass integrators produce a short, dull thump.
             * Unlike a resonant sine, each hit is genuinely aperiodic. The
             * controls shift the noise bandwidth and envelope, never pitch. */
            int32_t deep = (int32_t)ph0, low = (int32_t)ph1;
            int32_t k_low = 1400 + tune * 16;       /* broad impact around low mids */
            int32_t k_deep = 220 + tune * 4;       /* sub/low-frequency weight */
            low += (int32_t)(((int64_t)(white - low) * k_low) >> 15);
            deep += (int32_t)(((int64_t)(low - deep) * k_deep) >> 15);
            int32_t thud = deep * (110 + body_mix) / 128 +
                           (low - deep) * (25 + ring_amt) / 128;
            int32_t beater = (white - fast) * (20 + strike) / 256;
            /* BEATER neutral at 92; TEXTURE neutral at 65.
             * Extra transient/low-mid colour only: no new pitched source. */
            int32_t texture = metal - 65, beater_delta = wire_amt - 92;
            raw = (int32_t)(((int64_t)thud * body) >> 23);
            raw += (int32_t)(((int64_t)beater * trans) >> 23);
            if (texture)
                raw += (int32_t)(((int64_t)(low - deep) * body * texture) >> 31);
            if (beater_delta)
                raw += (int32_t)(((int64_t)(white - fast) * trans * beater_delta) >> 30);
            ph0 = (uint32_t)deep;
            ph1 = (uint32_t)low;
        } else if (kind == AC_SNARE) {
            /* Broadband snare-wire rasp over a non-tonal, low-mid drumhead
             * thud. RING increases the mid-body component, not a sine. */
            int32_t low = (int32_t)ph0, mid = (int32_t)ph1;
            int32_t k_mid = 3500 + tune * 42;
            int32_t k_low = 650 + tune * 12;
            mid += (int32_t)(((int64_t)(white - mid) * k_mid) >> 15);
            low += (int32_t)(((int64_t)(mid - low) * k_low) >> 15);
            int32_t thud = low * (85 + body_mix) / 128 +
                           (mid - low) * (15 + ring_amt) / 128;
            int32_t rattle = (white - wire) * wire_amt / 127;
            /* Membrane-forward snare: the old rattle/attack was much louder
             * than the thud. Retain noise snap, but give the low-mid head a
             * clear and independent body instead of an extended hiss. */
            raw = (int32_t)(((int64_t)thud * body) >> 22);
            raw += (int32_t)(((int64_t)rattle * body * (40 + wire_amt)) >> 33);
            raw += (int32_t)(((int64_t)(white - fast) * trans * (35 + strike)) >> 33);
            if (metal != 65)
                raw += (int32_t)(((int64_t)(mid - low) * body * (metal - 65)) >> 31);
            /* Keep the approved snare exactly unchanged at WIRES=43 and
             * BRIGHT=87, but make editing either parameter clearly audible.
             * Wires adds/subtracts rattling noise rather than changing the
             * membrane; brightness changes the sharp high-frequency crack. */
            if (wire_amt != 43)
                raw += (int32_t)(((int64_t)(white - wire) * body *
                                  (wire_amt - 43)) >> 32);
            if (bright != 87)
                raw += (int32_t)(((int64_t)(white - fast) * trans *
                                  (bright - 87)) >> 32);
            ph0 = (uint32_t)low;
            ph1 = (uint32_t)mid;
        } else if (kind == AC_HAT_C || kind == AC_HAT_O) {
            /* High-pass noise and moving band, no sine fundamentals.
             * Short closed hat and ringing, wider open hat are separate pads. */
            int32_t high = white - fast, mid = fast - lp;
            int32_t shimmer = (high * (80 + bright) + mid * (30 + metal)) / 256;
            raw = (int32_t)(((int64_t)shimmer * body) >> 24);
            raw += (int32_t)(((int64_t)high * trans * (20 + strike)) >> 31);
        } else if (ac_is_tom(kind)) {
            /* Four distinct noise-excited drums: no musical boing or tuning sweep.
             * Each body filter starts in a different low-frequency region. */
            int32_t low = (int32_t)ph0, deep = (int32_t)ph1;
            uint32_t height = kind - AC_FLOOR;
            /* The noise-body colour is fixed for each tom. TUNE controls
             * only the independent resonating head so it is pitch, not thud. */
            int32_t k1 = 600 + (int32_t)height * 540 + 64 * 13;
            int32_t k2 = 150 + (int32_t)height * 130 + 64 * 3;
            low += (int32_t)(((int64_t)(white - low) * k1) >> 15);
            deep += (int32_t)(((int64_t)(low - deep) * k2) >> 15);
            int32_t thud = deep * (100 + body_mix) / 128 +
                           (low - deep) * (22 + ring_amt) / 128;
            raw = (int32_t)(((int64_t)thud * body) >> 23);
            raw += (int32_t)(((int64_t)(white - fast) * trans * (25 + strike)) >> 32);
            /* The noise impact stays exactly as before. At nonzero TUNE
             * a strong but time-limited membrane resonance is added. The
             * previous >>49 attenuation made pitch almost inaudible. */
            if (tom_tone_inc && age < 13000u) {
                int32_t head = sine_i(ph2);
                raw += (int32_t)(((int64_t)head * body *
                                  (13000u - age) * (75 + ring_amt)) >> 48);
                ph2 += tom_tone_inc;
            }
            ph0 = (uint32_t)low;
            ph1 = (uint32_t)deep;
        } else if (kind == AC_CLAP) {
            /* Several tightly spaced hand-clap reflections followed by a noisy tail. */
            uint32_t burst = age < 1320u ? 1320u - (age % 330u) : 0u;
            int32_t slap = (white - fast) * (90 + bright) / 256;
            raw = (int32_t)(((int64_t)slap * body * (120 + burst / 7u)) >> 31);
            raw += (int32_t)(((int64_t)(white - lp) * trans * (25 + strike)) >> 32);
            /* Hands and chest/body reflections below the clap's sharp slap. */
            raw += (int32_t)(((int64_t)lp * body * (60 + body_mix)) >> 33);
        } else {
            /* Acoustic cymbal experiment: excite three damped, inharmonic
             * noise bandpass modes instead of oscillators/ring modulation.
             * Each band receives a slightly different stochastic excitation,
             * producing broad, shifting plate energy rather than a chord.
             * The broad hiss and stick strike have their own envelopes. */
            int32_t bands[3];
            int32_t input[3] = {white - lp, white - fast, fast - lp};
            uint32_t k;
            for (k = 0; k < 3u; k++) {
                int32_t band = plate_bands[k], low = plate_low[k];
                int32_t f = plate_coef[k];
                low += (int32_t)(((int64_t)f * band) >> 15);
                /* Damped state-variable bandpass, bounded to avoid a
                 * runaway resonance on aggressive cymbal settings. */
                int32_t high = input[k] - low -
                               (int32_t)(((int64_t)plate_damp * band) >> 15);
                band += (int32_t)(((int64_t)f * high) >> 15);
                low = clamp(low, -131072, 131072);
                band = clamp(band, -131072, 131072);
                plate_low[k] = low;
                plate_bands[k] = band;
                bands[k] = band;
            }
            int32_t upper = white - fast;
            int32_t broad = (white - lp) / 2 + upper / 2;
            int32_t plate = bands[0] * (60 + metal) / 256 +
                            bands[1] * (75 + bright) / 256 +
                            bands[2] * (35 + ring_amt) / 256;
            if (kind == AC_CRASH) {
                /* SPLASH is deliberately strong: 0 = rounded, mostly plate;
                 * 127 = explosive upper-frequency attack and spray.
                 * Unlike the previous model it changes spectral balance as
                 * well as attack gain, so the range is clearly audible. */
                int32_t splash = wire_amt;
                raw = (int32_t)(((int64_t)plate * body * (110 + body_mix)) >> 33);
                raw += (int32_t)(((int64_t)broad * body * (35 + splash * 2)) >> 34);
                raw += (int32_t)(((int64_t)upper * trans * (25 + splash * 5)) >> 34);
            } else if (kind == AC_RIDE_BELL) {
                /* A more focused bell, emphasising the mid plate resonance
                 * with a hard stick impulse. */
                raw = (int32_t)(((int64_t)(bands[0] + bands[1] * 2) *
                                 body * (80 + metal)) >> 32);
                raw += (int32_t)(((int64_t)upper * trans * (45 + strike)) >> 31);
            } else {
                /* Ride splash: lighter than the crash and longer than a
                 * single stick strike, with a separate broad wash. */
                raw = (int32_t)(((int64_t)plate * body * (95 + bright)) >> 33);
                raw += (int32_t)(((int64_t)broad * body * (60 + wire_amt * 2)) >> 34);
                raw += (int32_t)(((int64_t)upper * trans *
                                  (40 + wire_amt * 3)) >> 34);
            }
        }
        /* Modulation range extension. This layer deliberately has no
         * contribution at the patch's existing neutral/preset settings.
         * All eight fields now have a distinct audible role, including
         * previously inert cymbal TUNE/RING, tom STICK/SHELL, hat CHICK/
         * DECAY and clap SNAP/TEXTURE/TAIL. Use the same existing noise,
         * head and strike states: no additional synth engine or voices. */
        if (mod_render) {
            int32_t high = white - fast;
            int32_t mid = fast - lp;
            int32_t low = lp;
            int32_t extra = 0;
            int32_t tune_delta = mod_delta[0];
            int32_t body_delta = mod_delta[2];
            int32_t hit_delta = mod_delta[3];
            int32_t aux_delta = mod_delta[4];
            int32_t bright_delta = mod_delta[5];
            int32_t texture_delta = mod_delta[6];
            int32_t ring_delta = mod_delta[7];
            /* TUNE: body colour for untuned kick/snare/clap, plate spectrum
             * for cymbals; toms already have a separate true-pitch path. */
            if (mod_delta[0] && !ac_is_tom(kind))
                extra += (int32_t)(((int64_t)mid * body * tune_delta) >> 34);
            /* BODY: change thump/wash strength rather than merely the decay.
             * Preserve the existing low-frequency kick and snare generators. */
            if (body_delta)
                extra += (int32_t)(((int64_t)low * body * body_delta) >> 33);
            /* HIT: independently emphasise or soften the initial transient. */
            if (hit_delta)
                extra += (int32_t)(((int64_t)high * trans * hit_delta) >> 33);
            /* WIRES / BEATER / SPLASH / STICK / CHICK / SNAP:
             * For pads where field 5 was previously unused, make it an
             * unmistakable initial strike and short rattle control. */
            if (aux_delta && (kind == AC_HAT_C || kind == AC_HAT_O ||
                ac_is_tom(kind) || kind == AC_CLAP || kind == AC_RIDE_BELL))
                extra += (int32_t)(((int64_t)high * trans * aux_delta) >> 32);
            /* SPLASH is more than a volume knob: also vary crash and ride
             * attack-to-wash balance around the exact original sound. */
            if (aux_delta && (kind == AC_CRASH || kind == AC_RIDE_SPLASH)) {
                extra += (int32_t)(((int64_t)high * trans * aux_delta) >> 33);
                extra += (int32_t)(((int64_t)mid * body * aux_delta) >> 34);
            }
            /* BRIGHT: strong upper-band emphasis on all sounds. The snare
             * already has a bright transient; this adds sustained spectral
             * contrast without overwriting its approved default. */
            if (bright_delta)
                extra += (int32_t)(((int64_t)high * body * bright_delta) >> 34);
            /* METAL/TEXTURE/SHELL: previously ineffective for tom and clap;
             * on cymbals modify the noisy metal sheet, not just its resonator. */
            if (texture_delta && (ac_is_tom(kind) || kind == AC_CLAP ||
                kind == AC_HAT_C || kind == AC_HAT_O || ac_is_metal(kind)))
                extra += (int32_t)(((int64_t)mid * body * texture_delta) >> 33);
            /* RING/TAIL: separate body-band tail coloration on pads whose
             * earlier RING scarcely affected the sound, especially hats
             * and clap. Decay is also modulated above. */
            if (ring_delta && (kind == AC_HAT_C || kind == AC_HAT_O || kind == AC_CLAP ||
                ac_is_metal(kind)))
                extra += (int32_t)(((int64_t)(mid + low) * body * ring_delta) >> 34);
            raw = (int32_t)clamp(raw + extra, -32768, 32767);
        }
        /* Independent kit-wide timbre stage, deliberately separate from
         * the per-pad controls above. Each global function has the same
         * musical meaning on all twelve sounds. All deltas are zero at 100;
         * even the existing 0..127 pad patches are left untouched.
         *
         * TONE     shift the low/mid spectral balance
         * BODY     more/less low-frequency thump or body of cymbal wash
         * ATTACK   more/less initial stick, beater or hand transient
         * BRIGHT   high-frequency energy
         * TEXTURE  noisy/granular excitation independently of brightness
         * RING     low-mid lingering resonance (also its decay above)
         * DAMP     duration control above
         * LEVEL    final gain below
         */
        if (kit_timbre) {
            int32_t high = white - fast;
            int32_t mid = fast - lp;
            int32_t bass = lp;
            int64_t extra = 0;
            if (kit[0])
                extra += ((int64_t)(mid - bass) * body * kit[0]) >> 31;
            if (kit[2])
                extra += ((int64_t)bass * body * kit[2]) >> 30;
            if (kit[3])
                extra += ((int64_t)high * trans * kit[3]) >> 30;
            if (kit[4])
                extra += ((int64_t)high * body * kit[4]) >> 31;
            if (kit[5])
                extra += ((int64_t)(white - wire) * body * kit[5]) >> 31;
            if (kit[6])
                extra += ((int64_t)(mid + bass) * body * kit[6]) >> 32;
            raw = (int32_t)clamp((int64_t)raw + extra, -32768, 32767);
        }
        if (raw > 32767) raw = 32767;
        if (raw < -32768) raw = -32768;
        out[i] += voice_amp((int32_t)(((int64_t)raw * level_gain) >> 15), m, i);
    }
    if (ac_is_metal(kind)) {
        ph0 = (uint32_t)plate_low[0];
        ph1 = (uint32_t)plate_low[1];
        ph2 = (uint32_t)plate_low[2];
    }
    a->phase[0] = ph0; a->phase[1] = ph1; a->phase[2] = ph2;
    a->seed = rng; a->age = age;
    a->noise_lp = lp; a->noise_fast = fast; a->wires_lp = wire;
    a->body = body > 0 ? body : 0; a->transient = trans > 0 ? trans : 0;
    if (a->body < 100) a->active = 0;
}
static const preset_t AC_PRESETS[] = {
    {"KRIPKE-1", {64, 64, 85, 80, 92, 87, 65, 64}, {0, 127, 127, 127}, 0, TAG(DRUM)},
    {"KRIPKE-2", {64, 105, 65, 95, 68, 93, 54, 64}, {0, 127, 127, 127}, 0, TAG(DRUM)},
    {"KRIPKE-3", {64, 28, 119, 88, 108, 102, 98, 64}, {0, 127, 127, 127}, 0, TAG(DRUM)},
};
static const engine_t ENG_DRUMAC = {
    .name = "DRUM-AC",
    .page_title = {"SHELL", "METAL"},
    .edit = {
        {"TUNE", F_PCT, 0, 127, 64, 0, 0},
        {"DAMP", F_PCT, 0, 127, 64, 0, 0},
        {"BODY", F_PCT, 0, 127, 85, 0, 0},
        {"HIT", F_PCT, 0, 127, 80, 0, 0},
        {"WIRES", F_PCT, 0, 127, 92, 0, 0},
        {"BRIGHT", F_PCT, 0, 127, 87, 0, 0},
        {"METAL", F_PCT, 0, 127, 65, 0, 0},
        {"RING", F_PCT, 0, 127, 64, 0, 0},
    },
    .presets = AC_PRESETS,
    .npresets = NELEM(AC_PRESETS),
    .note_on = ac_note_on,
    .render = ac_render,
    .amp = ac_amp,
    .knob = {P_E1, P_E2, P_E3, P_E5},
    .poly = NVOICE,
    .oneshot = 1,
};
