/* SPDX-License-Identifier: GPL-3.0-only
 * MASHINA DRUM-AC: first acoustic percussion milestone.
 * Independent kick, snare and crash models, all generated in fixed point.
 * No sample data, no dependence on DRUM-X/DRUM-LAB patches.
 * MIDI: 36 kick, 38 snare, 49 crash; 42 is a crash preview shortcut.
 */
enum { AC_KICK, AC_SNARE, AC_CRASH };
typedef struct {
    uint32_t phase[3], inc, seed, age;
    int32_t body, transient, noise_lp, noise_fast, wires_lp;
    uint8_t kind, active;
} ac_voice_t;
static ac_voice_t ac_state[NPART][NVOICE];

static uint32_t ac_random(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return x;
}
static uint32_t ac_kind(uint32_t note)
{
    if (note == 38u) return AC_SNARE;
    if (note == 49u || note == 42u) return AC_CRASH;
    return AC_KICK;
}
static void ac_note_on(track_t *t, voice_t *v)
{
    uint32_t slot = (uint32_t)(v - t->v);
    ac_voice_t *a;
    if (slot >= NVOICE) return;
    a = &ac_state[t - trk][slot];
    memset(a, 0, sizeof *a);
    if (v->note != 36u && v->note != 38u && v->note != 42u && v->note != 49u)
        return;
    a->kind = (uint8_t)ac_kind(v->note);
    a->seed = 0xB5297A4Du ^ ((uint32_t)v->note << 18) ^ (uint32_t)v->age;
    if (!a->seed) a->seed = 1;
    a->inc = pitch_inc((uint32_t)(a->kind == AC_KICK ? 35u :
                          a->kind == AC_SNARE ? 51u : 75u) * 16u);
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
/* Per-voice damped membrane and excitation, with cymbal inharmonic
 * partials and two noise bands. Decays are computed only per audio block. */
static void ac_render(track_t *t, voice_t *v, int32_t *out,
                      uint32_t n, const vmod_t *m)
{
    uint32_t slot = (uint32_t)(v - t->v), i, ph0, ph1, ph2, rng, age, inc;
    int32_t body, trans, step_b, step_t, lp, fast, wire;
    uint32_t tau_b, tau_t, mul_b, mul_t, kind;
    int32_t tune, damp, strike, wire_amt, bright, metal, body_mix, ring_amt;
    ac_voice_t *a;
    if (slot >= NVOICE || !n) return;
    a = &ac_state[t - trk][slot];
    if (!a->active) return;
    kind = a->kind;
    tune = clamp(t->p[P_E0], 0, 127);
    damp = clamp(t->p[P_E1], 0, 127);
    body_mix = clamp(t->p[P_E2], 0, 127);
    strike = clamp(t->p[P_E3], 0, 127);
    wire_amt = clamp(t->p[P_E4], 0, 127);
    bright = clamp(t->p[P_E5], 0, 127);
    metal = clamp(t->p[P_E6], 0, 127);
    ring_amt = clamp(t->p[P_E7], 0, 127);
    inc = a->inc + (uint32_t)(((int64_t)a->inc * (tune - 64)) / 192);
    tau_b = kind == AC_KICK ? 160000u + (uint32_t)body_mix * 2800u :
            kind == AC_SNARE ? 80000u + (uint32_t)body_mix * 2100u :
                               380000u + (uint32_t)body_mix * 16000u;
    tau_b = tau_b * (uint32_t)(180 - damp) / 116u;
    if (tau_b < 25000u) tau_b = 25000u;
    tau_t = kind == AC_CRASH ? 85000u : kind == AC_SNARE ? 22000u : 9500u;
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
        int32_t s0 = sine_i(ph0), s1 = sine_i(ph1), s2 = sine_i(ph2);
        int32_t raw;
        /* Noise filters are bounded and share no state between drum voices. */
        lp += (int32_t)(((int64_t)(white - lp) * (800 + bright * 45)) >> 15);
        fast += (int32_t)(((int64_t)(white - fast) * (7800 + bright * 115)) >> 15);
        wire += (int32_t)(((int64_t)(white - wire) * (1800 + bright * 65)) >> 15);
        if (kind == AC_KICK) {
            /* A quickly relaxing head with an inharmonic shell mode and
             * short beater strike. */
            uint32_t sweep = age < 2400u ? 2400u - age : 0u;
            raw = (int32_t)(((int64_t)(s0 * 3 + s1 * (1 + ring_amt / 32)) * body * (70 + body_mix)) >> 34);
            raw += (int32_t)(((int64_t)(white - fast) * trans * (35 + strike)) >> 32);
            ph0 += inc + (uint32_t)(((uint64_t)inc * sweep) >> 12);
            ph1 += (inc * 7u) / 5u;
        } else if (kind == AC_SNARE) {
            /* Two membrane modes plus a decaying snare-wire rattle. */
            int32_t membrane = (s0 * 3 + s1 * (1 + ring_amt / 24)) / (4 + ring_amt / 24);
            int32_t rattle = (white - wire) * wire_amt / 127;
            raw = (int32_t)(((int64_t)membrane * body * (60 + body_mix)) >> 31);
            raw += (int32_t)(((int64_t)rattle * body * (45 + wire_amt)) >> 32);
            raw += (int32_t)(((int64_t)(white - fast) * trans * (25 + strike)) >> 32);
            ph0 += inc;
            ph1 += (inc * 8u) / 5u;
        } else {
            /* A long metallic wash with several non-harmonic resonances.
             * Broadband splash dominates the attack; partials evolve into
             * the sustain. Crash is deliberately not a short electronic hat. */
            int32_t wash = (white - fast) / 2 + (fast - lp) / 2;
            int32_t ring = (s0 + s1 * (1 + ring_amt / 48) + s2) / (3 + ring_amt / 48);
            raw = (int32_t)(((int64_t)wash * body * (120 + bright)) >> 32);
            raw += (int32_t)(((int64_t)ring * body * (35 + metal)) >> 33);
            raw += (int32_t)(((int64_t)(white - lp) * trans * (45 + strike)) >> 32);
            ph0 += inc;
            ph1 += (uint32_t)(((uint64_t)inc * 6021u) >> 12);
            ph2 += (uint32_t)(((uint64_t)inc * (7900u + (uint32_t)metal * 11u)) >> 12);
        }
        if (raw > 32767) raw = 32767;
        if (raw < -32768) raw = -32768;
        out[i] += voice_amp(raw, m, i);
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
