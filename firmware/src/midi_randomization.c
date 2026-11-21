/* MIDI live randomization eligibility: deliberately conservative.
 * IDs are engine EDIT slots P_E0..P_E7 or shared sound-shaping parameters.
 * This list does not imply that an arbitrary stored 0..127 value is continuous.
 * All descriptors must still be checked for min=0, max=127 at runtime.
 */
/* All playable melodic engines may vary; the four drum engines and retired
 * engines are excluded. Avoid waveform/mode/patch selectors and pitch controls. */
static int random_melodic_engine(uint32_t e)
{
    return e != ENGI_DRUM && e != ENGI_DRUMLAB &&
           e != ENGI_DRUMAC && e != ENGI_DRUMIND && !eng_gone(e) &&
           (FELUCCA_FM4 || e != ENGI_DIGITAL);
}
static int random_sound_param(const track_t *t, uint32_t id)
{
    uint32_t e = eng_idx(t->eng_req), mode = (uint32_t)t->p[P_E0];
    if (!random_melodic_engine(e))
        return 0;
    if (id == P_DIST)
        return 1;
    if (id == P_FCUT || id == P_FRES)
        return e != ENGI_FM6;
    if (id == P_MORPH)
        return e == ENGI_ANALOG || e == ENGI_PHASE || e == ENGI_LOFI;
    if (id >= P_LN8 && id <= P_LN11) {
        if (e == ENGI_ATMOS || e == ENGI_BRASS) return 1;
        if (e == ENGI_INDUSTRIAL) return id == P_LN9 || id == P_LN10;
        return 0;
    }
    if (id < P_E0 || id > P_E7)
        return 0;
    switch (e) {
    case ENGI_ANALOG:
        return id == P_E2 || id == P_E3 || id == P_E4 || id == P_E5 || id == P_E6;
    case ENGI_PHASE:
        return id == P_E2 || id == P_E6 || id == P_E7;
    case ENGI_LOFI:
        /* BYTE/FLOAT/STEP reinterpret these controls; protect waveforms,
         * pitch modulation and STEP's envelope. */
        if (t->p[P_E1] == 5u) return id == P_E3 || id == P_E5;
        if (t->p[P_E1] == 6u) return id == P_E3;
        return (id == P_E2 && t->p[P_E1] != 4u) ||
               (id == P_E3 && mode != 4u) || id == P_E7;
    case ENGI_VOICE:
        return id == P_E4 || id == P_E5 || id == P_E6 ||
               (!t->p[P_VLANG] && (id == P_E0 || id == P_E1));
    case ENGI_WHEEL:
        return id == P_E5 || id == P_E6;
    case ENGI_NOISE:
        return id == P_E1 || id == P_E3;
    case ENGI_ANIMAL:
        return id == P_E1 || id == P_E2;
    case ENGI_FM6:
        /* Operator feedback/modulation, not algorithm, patch slot or pitch. */
        return id == P_E1 || id == P_E2 || id == P_E4;
    case ENGI_ATMOS:
        return id >= P_E1 && id <= P_E7;
    case ENGI_BRASS:
        return id >= P_E1 && id <= P_E7;
    case ENGI_INDUSTRIAL:
        return id >= P_E1 && id <= P_E7;
    default:
        return 0;
    }
}
static uint32_t random_sound_pool(const track_t *t, uint8_t *ids, uint32_t cap)
{
    uint32_t i, n = 0;
    static const uint8_t candidates[] = {
        P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
        P_FCUT, P_FRES, P_DIST, P_MORPH,
        P_LN8, P_LN9, P_LN10, P_LN11
    };
    for (i = 0; i < sizeof candidates; i++) {
        uint32_t id = candidates[i];
        const param_desc_t *d;
        if (!random_sound_param(t, id)) continue;
        d = track_desc(t, id);
        if (!d || d->max <= d->min) continue;
        /* FM6's signed operator controls are safe; other engines use the
         * original conservative 0..127 eligibility rule. */
        if (eng_idx(t->eng_req) != ENGI_FM6 &&
            (d->min != 0 || d->max != 127)) continue;
        if (n < cap) ids[n++] = (uint8_t)id;
    }
    return n;
}

/* A snapshot belongs to the CURRENT sound, whether factory, user or project.
 * The first randomization after an edit/load starts from the live parameters.
 * Only T1..T3 use the reserved command channels. */
static int16_t random_baseline[3][P_COUNT];
static uint8_t random_baseline_engine[3];
static volatile uint8_t random_variation_active[3];
static volatile uint8_t random_baseline_valid[3];
/* High-only pads may repeat the same value on each new press. Suppress
 * close-together duplicate packets, but allow another high after 120 ms.
 * This cannot distinguish a held controller's slow repeats from new presses. */
#define RANDOM_CC_DEBOUNCE_MS 120u
static uint32_t random_last_ms[3][3];
static uint8_t random_pad_held[3][3];
static uint32_t random_seed = 0xA3C59AC3u;
static uint8_t random_seed_ready;
static volatile uint8_t random_ui_dirty;
static volatile uint8_t random_reset_pending;
/* A sound change cancels an outstanding variation, but never changes the project. */
static void random_sound_changed(uint32_t k)
{
    if (k >= 3u) return;
    random_baseline_valid[k] = 0u;
    random_variation_active[k] = 0u;
    random_reset_pending &= (uint8_t)~(1u << k);
}

static void random_capture_track(uint32_t k)
{
    uint32_t j;
    if (k >= 3u) return;
    /* MIDI may arrive while the main thread copies a new saved baseline.
     * Hide any partially written snapshot until the final publish. */
    random_baseline_valid[k] = 0u;
    for (j = 0; j < P_COUNT; j++)
        random_baseline[k][j] = trk[k].p[j];
    random_baseline_engine[k] = trk[k].eng_req;
    RING_PUBLISH();
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
    uint8_t pool[16];
    track_t *t;
    if (ch < 12u || ch > 14u) return;
    k = ch - 12u;
    cmd = cc - 85u;
    if (value < 64u) {
        random_pad_held[k][cmd] = 0u;
        return;
    }
    {
        uint32_t now = fm1_ms;
        if (random_pad_held[k][cmd] &&
            (uint32_t)(now - random_last_ms[k][cmd]) < RANDOM_CC_DEBOUNCE_MS)
            return;
        random_pad_held[k][cmd] = 1u;
        random_last_ms[k][cmd] = now;
    }
    t = &trk[k];
    if (!random_melodic_engine(t->eng_req)) return;
    if (cmd == 2u) {
        /* CC87 only reverses randomization of this current sound. */
        if (random_variation_active[k] && random_baseline_valid[k] &&
            random_baseline_engine[k] == t->eng_req)
            random_reset_pending |= (uint8_t)(1u << k);
        return;
    }
    /* A fresh preset, user sound, project, or sound edited since the last
     * reset becomes the baseline on the first CC85/86 press. */
    if (!random_variation_active[k] || !random_baseline_valid[k] ||
        random_baseline_engine[k] != t->eng_req)
        random_capture_track(k);
    /* A human pad's first press timing diversifies the sequence across boots,
     * while keeping all subsequent choices deterministic for host regressions. */
    if (!random_seed_ready) {
        random_seed ^= (fm1_ms + 1u) * 0x9e3779b9u;
        random_seed_ready = 1u;
    }
    random_reset_pending &= (uint8_t)~(1u << k);
    n = random_sound_pool(t, pool, sizeof pool);
    if (!n) return;
    take = cmd == 0u ? 2u : 3u;
    if (take > n) take = n;
    strength = cmd == 0u ? 20u : 40u;
    for (i = 0; i < take; i++) {
        uint32_t j = i + random_next() % (n - i);
        uint8_t id = pool[i];
        const param_desc_t *d;
        int32_t offset, v, range;
        pool[i] = pool[j];
        pool[j] = id;
        d = track_desc(t, id);
        range = d->max - d->min;
        offset = (int32_t)(strength + random_next() % (cmd == 0u ? 41u : 51u));
        /* Scale for FM6's narrow or bipolar operators (e.g. FB -7..7). */
        offset = (offset * range + 63) / 127;
        if (!offset) offset = 1;
        if (random_next() & 1u) offset = -offset;
        v = clamp((int32_t)random_baseline[k][id] + offset, d->min, d->max);
        t->p[id] = (int16_t)v;
        /* Match ordinary MIDI CC edits: update motion's live base so a later
         * explicit SAVE captures this sound rather than an old automation value. */
        (void)motion_capture(t, id, (int16_t)v);
    }
    random_variation_active[k] = 1u;
    random_ui_dirty = 1u;
}

/* Apply CC87 on the UI thread without sound_put(), which kills active voices.
 * Restore the sound snapshot in place while preserving mixer controls and active notes. */
static void random_reset_poll(void)
{
    uint32_t k, id;
    uint8_t pending = random_reset_pending;
    random_reset_pending &= (uint8_t)~pending;
    for (k = 0; k < 3u; k++) {
        track_t *t = &trk[k];
        if (!(pending & (1u << k))) continue;
        if (!random_variation_active[k] || !random_baseline_valid[k] ||
            t->eng_req != random_baseline_engine[k] ||
            !random_melodic_engine(t->eng_req))
            continue;
        for (id = 0; id < P_COUNT; id++) {
            if (id == P_LEVEL || id == P_PAN) continue;
            if (t->p[id] != random_baseline[k][id]) {
                t->p[id] = random_baseline[k][id];
                (void)motion_capture(t, id, random_baseline[k][id]);
            }
        }
        random_variation_active[k] = 0u;
        random_ui_dirty = 1u;
    }
}
