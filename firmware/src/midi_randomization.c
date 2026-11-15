/* MIDI live randomization eligibility: deliberately conservative.
 * IDs are engine EDIT slots P_E0..P_E7 or shared sound-shaping parameters.
 * This list does not imply that an arbitrary stored 0..127 value is continuous.
 * All descriptors must still be checked for min=0, max=127 at runtime.
 */
static int random_sound_param(const track_t *t, uint32_t id)
{
    uint32_t e = eng_idx(t->eng_req);
    uint32_t mode = (uint32_t)t->p[P_E0];
    if (e == ENGI_DRUM || e == ENGI_DRUMLAB || eng_gone(e))
        return 0;
    if (id == P_DIST)
        return 1;
    if (id == P_FCUT || id == P_FRES)
        return e != ENGI_FM6; /* FM6 uses its own operator-based tone shaping. */
    if (id == P_MORPH)
        return e == ENGI_ANALOG || e == ENGI_PHASE || e == ENGI_LOFI;
    if (id < P_E0 || id > P_E7)
        return 0;
    switch (e) {
    case ENGI_ANALOG:
        return id == P_E2 || id == P_E3 || id == P_E4 || id == P_E5 || id == P_E6;
    case ENGI_PHASE:
        return id == P_E2 || id == P_E6 || id == P_E7;
    case ENGI_LOFI:
        /* BYTE/FLOAT remap EDIT fields to algorithms and time-based motion.
         * Restrict to continuous timbre/feedback parameters only. */
        if (t->p[P_E1] == 5u || t->p[P_E1] == 6u)
            return id == P_E3 || (t->p[P_E1] == 5u && id == P_E5);
        if (t->p[P_E1] == 4u)
            return id == P_E6 || id == P_E7;
        return (id == P_E2 && t->p[P_E1] != 4u) || (id == P_E3 && mode != 4u) || id == P_E6 || id == P_E7;
    case ENGI_VOICE:
        /* VOICE words mode remaps E0..E2 and E7 to selectors/articulation. */
        return id == P_E4 || id == P_E5 || id == P_E6 ||
               (!t->p[P_VLANG] && (id == P_E0 || id == P_E1));
    case ENGI_WHEEL:
        return id == P_E5 || id == P_E6;
    case ENGI_NOISE:
        return id == P_E1 || id == P_E2 || id == P_E3 || id == P_E6 ||
               (mode < 2u && id == P_E7);
    case ENGI_ANIMAL:
        /* Mode-dependent controls: only stable continuous tonal slots. */
        return id == P_E1 || id == P_E2 || (mode == 0u && id == P_E3);
    case ENGI_FM6:
        return 0; /* FM6's EDIT macros are signed, pitch or patch selectors. */
    default:
        return 0;
    }
}
static uint32_t random_sound_pool(const track_t *t, uint8_t *ids, uint32_t cap)
{
    uint32_t i, n = 0;
    static const uint8_t candidates[] = {
        P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
        P_FCUT, P_FRES, P_DIST, P_MORPH
    };
    for (i = 0; i < sizeof candidates; i++) {
        uint32_t id = candidates[i];
        const param_desc_t *d;
        if (!random_sound_param(t, id))
            continue;
        d = track_desc(t, id);
        if (!d || d->min != 0 || d->max != 127)
            continue;
        if (n < cap)
            ids[n++] = (uint8_t)id;
    }
    return n;
}

/* Baselines are captured when a project is loaded or explicitly saved.
 * Only T1..T3 are addressable by the reserved MIDI channels. */
static int16_t random_baseline[3][P_COUNT];
static uint8_t random_baseline_engine[3];
static uint8_t random_baseline_valid[3];
/* High-only pads may repeat the same value on each new press. Suppress
 * close-together duplicate packets, but allow another high after 120 ms.
 * This cannot distinguish a held controller's slow repeats from new presses. */
#define RANDOM_CC_DEBOUNCE_TICKS (120u * 1000u * FM1_TICKS_PER_US)
static uint32_t random_last_tick[3][3];
static uint8_t random_pad_held[3][3];
static uint32_t random_seed = 0xA3C59AC3u;

static void random_capture_track(uint32_t k)
{
    uint32_t j;
    if (k >= 3u) return;
    for (j = 0; j < P_COUNT; j++)
        random_baseline[k][j] = trk[k].p[j];
    random_baseline_engine[k] = trk[k].eng_req;
    random_baseline_valid[k] = 1u;
    /* Do not reset trigger state here: a first command can capture its baseline. */
}
static void random_capture_all(void)
{
    uint32_t k;
    for (k = 0; k < 3u; k++) random_capture_track(k);
}
static uint32_t random_next(void)
{
    uint32_t x = random_seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return random_seed = x;
}
/* Called from the MIDI dispatch before all other CC handlers. */
static void random_command(uint32_t ch, uint32_t cc, uint32_t value)
{
    uint32_t k, cmd, n, i, take, strength;
    uint8_t pool[12];
    track_t *t;
    if (ch < 12u || ch > 14u) return;
    k = ch - 12u;
    cmd = cc - 85u;
    if (value < 64u) {
        random_pad_held[k][cmd] = 0u;
        return;
    }
    {
        uint32_t now = fm1_ticks();
        if (random_pad_held[k][cmd] &&
            (uint32_t)(now - random_last_tick[k][cmd]) < RANDOM_CC_DEBOUNCE_TICKS)
            return;
        random_pad_held[k][cmd] = 1u;
        random_last_tick[k][cmd] = now;
    }
    t = &trk[k];
    if (t->eng_req == ENGI_DRUM || t->eng_req == ENGI_DRUMLAB || eng_gone(t->eng_req))
        return;
    if (!random_baseline_valid[k] || random_baseline_engine[k] != t->eng_req)
        random_capture_track(k);
    if (cmd == 2u) {
        for (i = 0; i < P_COUNT; i++) {
            if (!param_kept(i) && t->p[i] != random_baseline[k][i])
                t->p[i] = random_baseline[k][i];
        }
        ui.force = 1;
        return;
    }
    n = random_sound_pool(t, pool, sizeof pool);
    take = cmd == 0u ? 2u : 3u;
    if (take > n) take = n;
    strength = cmd == 0u ? 15u : 30u;
    for (i = 0; i < take; i++) {
        uint32_t j = i + random_next() % (n - i);
        uint8_t id = pool[i];
        int32_t offset, v;
        pool[i] = pool[j];
        pool[j] = id;
        offset = (int32_t)(strength + random_next() % (strength + 1u));
        if (random_next() & 1u) offset = -offset;
        v = clamp((int32_t)random_baseline[k][id] + offset, 0, 127);
        t->p[id] = (int16_t)v;
    }
    ui.force = 1;
}
