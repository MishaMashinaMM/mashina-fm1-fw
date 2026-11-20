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
    /* The sound-level base patch never changes when a global knob moves.
     * Global 100% is transparent, just as on DRUM-LAB. */
    uint32_t value = patch[field];
    uint32_t ti = (uint32_t)(t - trk);
    if (drumlab_global_ready[ti])
        value = value * drumlab_global[ti][field] / 100u;
    return (uint8_t)value;
}
static void ac_patch_set(track_t *t, uint32_t instrument, uint32_t field, uint32_t value)
{
    uint8_t *p = ac_patch_of(t, instrument);
    if (!p || field >= 8u) return;
    /* Editor displays the effective value. Convert edits back into the
     * independent pad value; don't rewrite the global kit multiplier. */
    uint32_t ti = (uint32_t)(t - trk);
    uint32_t g = drumlab_global_ready[ti] ? drumlab_global[ti][field] : 100u;
    p[field] = (uint8_t)clamp(g ? (value * 100u + g / 2u) / g : value, 0, 127);
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
static void ac_note_on(track_t *t, voice_t *v)
{
    uint32_t slot = (uint32_t)(v - t->v);
    ac_voice_t *a;
    if (slot >= NVOICE) return;
    a = &ac_state[t - trk][slot];
    memset(a, 0, sizeof *a);
    if (ac_kind(v->note) == AC_NPAD) return;
    a->kind = (uint8_t)ac_kind(v->note);
    a->seed = 0xB5297A4Du ^ ((uint32_t)v->note << 18) ^ (uint32_t)v->age;
    if (!a->seed) a->seed = 1;
    /* Only CRASH needs oscillator phases. KICK/SNARE use phase[0..1]
     * as two signed, noise-driven filter memories; neither has a pitch. */
    a->inc = ac_is_metal(a->kind) ?
             pitch_inc((a->kind == AC_RIDE_BELL ? 88u : 75u) * 16u) : 0u;
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
    uint32_t slot = (uint32_t)(v - t->v), i, ph0, ph1, ph2, rng, age, inc;
    int32_t body, trans, step_b, step_t, lp, fast, wire;
    uint32_t tau_b, tau_t, mul_b, mul_t, kind, lid;
    int32_t level_gain;
    int32_t tune, damp, strike, wire_amt, bright, metal, body_mix, ring_amt;
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
    lid = kind < 8u ? P_LN0 + kind : P_LN8 + kind - 8u;
    {   /* Quadratic pad LEVEL, identical to DRUM-LAB; calculated per block. */
        int32_t level = clamp(t->p[lid], 0, 127);
        level_gain = level == 127 ? 32768 : level * level * 32768 / 16129;
    }
    inc = ac_is_metal(kind) ?
          a->inc + (uint32_t)(((int64_t)a->inc * (tune - 64)) / 192) : 0u;
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
    if (tau_b < 25000u) tau_b = 25000u;
    tau_t = ac_is_metal(kind) ? 85000u :
            kind == AC_SNARE ? 22000u : kind == AC_CLAP ? 30000u : 9500u;
    mul_b = dlab_decay_long(n, tau_b);
    mul_t = dlab_decay_long(n, tau_t);
    body = a->body; trans = a->transient;
    step_b = ((int32_t)(((int64_t)body * mul_b) >> 16) - body) / (int32_t)n;
    step_t = ((int32_t)(((int64_t)trans * mul_t) >> 16) - trans) / (int32_t)n;
    ph0 = a->phase[0]; ph1 = a->phase[1]; ph2 = a->phase[2];
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
            int32_t k1 = 600 + (int32_t)height * 540 + tune * 13;
            int32_t k2 = 150 + (int32_t)height * 130 + tune * 3;
            low += (int32_t)(((int64_t)(white - low) * k1) >> 15);
            deep += (int32_t)(((int64_t)(low - deep) * k2) >> 15);
            int32_t thud = deep * (100 + body_mix) / 128 +
                           (low - deep) * (22 + ring_amt) / 128;
            raw = (int32_t)(((int64_t)thud * body) >> 23);
            raw += (int32_t)(((int64_t)(white - fast) * trans * (25 + strike)) >> 32);
            /* A touch of short, damped head pitch: 0.18 s maximum and
             * deliberately much quieter than the aperiodic impact.
             * TUNE adjusts this subtle component without turning the tom
             * into the boing-like melodic drum we rejected for kick/snare. */
            if (age < 8000u) {
                int32_t semitone = clamp(35 + (int32_t)height * 4 + (tune - 64) / 16, 26, 65);
                uint32_t tone_inc = pitch_inc((uint32_t)semitone * 16u);
                int32_t head = sine_i(ph2);
                raw += (int32_t)(((int64_t)head * body * (8000u - age) *
                                  (64 + ring_amt)) >> 51);
                ph2 += tone_inc;
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
            int32_t s0 = sine_i(ph0), s1 = sine_i(ph1), s2 = sine_i(ph2);
            /* Noise-driven cymbal plate: a multi-band metallic wash whose
             * brightness fluctuates irregularly with several inharmonic
             * modes. The modes modulate the noise rather than dominate the
             * sustain as audible pure notes. This better approximates a
             * struck vibrating plate without prerecorded samples. */
            int32_t wash = (white - fast) / 2 + (fast - lp) / 2;
            int32_t upper = (white - fast);
            int32_t lower = fast - lp;
            int32_t flutter = 140 + (s0 + s1 / 2 + s2 / 3) / 700;
            int32_t plate = (upper * (75 + bright) + lower * (50 + metal)) / 256;
            int32_t changing = (int32_t)(((int64_t)plate * flutter) >> 7);
            int32_t ring = (s0 + s1 * (1 + ring_amt / 48) + s2) / (3 + ring_amt / 48);
            if (kind == AC_CRASH) {
                /* Broad initial edge, then a long, restless wash. SPLASH
                 * adds cymbal strike noise separately from sustained metal. */
                raw = (int32_t)(((int64_t)changing * body * (110 + body_mix)) >> 31);
                raw += (int32_t)(((int64_t)ring * body * (10 + metal / 6)) >> 35);
                raw += (int32_t)(((int64_t)upper * trans * (45 + strike + wire_amt / 2)) >> 32);
            } else if (kind == AC_RIDE_BELL) {
                /* Bell retains a distinct hard metallic ping, but surrounded
                 * by a textured, inharmonic noise tail rather than three
                 * exposed stationary sine oscillators. */
                raw = (int32_t)(((int64_t)changing * body * (45 + bright)) >> 32);
                raw += (int32_t)(((int64_t)ring * body * (80 + metal)) >> 33);
                raw += (int32_t)(((int64_t)upper * trans * (35 + strike)) >> 32);
            } else {
                /* Ride splash: washy bow strike with fast splash transient. */
                raw = (int32_t)(((int64_t)changing * body * (130 + bright)) >> 31);
                raw += (int32_t)(((int64_t)ring * body * (10 + metal / 6)) >> 35);
                raw += (int32_t)(((int64_t)upper * trans * (75 + strike + wire_amt / 2)) >> 32);
            }
            ph0 += inc;
            ph1 += (uint32_t)(((uint64_t)inc * 6021u) >> 12);
            ph2 += (uint32_t)(((uint64_t)inc * (7900u + (uint32_t)metal * 11u)) >> 12);
        }
        if (raw > 32767) raw = 32767;
        if (raw < -32768) raw = -32768;
        out[i] += voice_amp((int32_t)(((int64_t)raw * level_gain) >> 15), m, i);
    }
    a->phase[0] = ph0; a->phase[1] = ph1; a->phase[2] = ph2;
    a->seed = rng; a->age = age;
    a->noise_lp = lp; a->noise_fast = fast; a->wires_lp = wire;
    a->body = body > 0 ? body : 0; a->transient = trans > 0 ? trans : 0;
    if (a->body < 100) a->active = 0;
}
static const preset_t AC_PRESETS[] = {
    {"ACOUSTIC", {64, 64, 85, 80, 92, 87, 65, 64}, {0, 127, 127, 127}, 0, TAG(DRUM)},
    {"DRY KIT", {64, 105, 65, 95, 68, 93, 54, 64}, {0, 127, 127, 127}, 0, TAG(DRUM)},
    {"BIG ROOM", {64, 28, 119, 88, 108, 102, 98, 64}, {0, 127, 127, 127}, 0, TAG(DRUM)},
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
