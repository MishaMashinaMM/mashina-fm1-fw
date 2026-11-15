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
        return e != 12u; /* FM6 uses its own operator-based tone shaping. */
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
        /* WAVE is E1: BYTE has VAR/GRIT; FLOAT has VAR but E5 is
         * envelope decay. STEP repurposes CRUSH as an envelope.
         * BEND and VIB affect pitch modulation, so exclude both. */
        if (t->p[P_E1] == 5u)
            return id == P_E3 || id == P_E5;
        if (t->p[P_E1] == 6u)
            return id == P_E3;
        return (id == P_E2 && t->p[P_E1] != 4u) ||
               (id == P_E3 && mode != 4u) || id == P_E7;
    case ENGI_VOICE:
        /* VOICE words mode remaps E0..E2 and E7 to selectors/articulation. */
        return id == P_E4 || id == P_E5 || id == P_E6 ||
               (!t->p[P_VLANG] && (id == P_E0 || id == P_E1));
    case ENGI_WHEEL:
        return id == P_E5 || id == P_E6;
    case ENGI_NOISE:
        /* FREQ/DRIFT control oscillator pitch; CRUSH can retime its clock. */
        return id == P_E1 || id == P_E3;
    case ENGI_ANIMAL:
        /* SIZE/CAP are excluded, as are SWEEP, RATE and GROUP. */
        return id == P_E1 || id == P_E2;
    case 12u:
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
/* The saved section used for each track; 0xff means there is no saved sound. */
static volatile uint8_t random_saved_slot[3] = {0xffu, 0xffu, 0xffu};

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
        uint32_t now = fm1_ms;
        if (random_pad_held[k][cmd] &&
            (uint32_t)(now - random_last_ms[k][cmd]) < RANDOM_CC_DEBOUNCE_MS)
            return;
        random_pad_held[k][cmd] = 1u;
        random_last_ms[k][cmd] = now;
    }
    t = &trk[k];
    if (cmd == 2u) {
        /* Reset is a saved-project sound load. A melodic saved sound may
         * replace an unsaved DRUM engine, but never randomize a saved drum kit.
         * The load runs on the main/UI thread (storage and IRQ guards). */
        if (random_baseline_valid[k] &&
            (random_baseline_engine[k] == ENGI_DRUM ||
             random_baseline_engine[k] == ENGI_DRUMLAB ||
             eng_gone(random_baseline_engine[k])))
            return;
        random_reset_pending |= (uint8_t)(1u << k);
        return;
    }
    if (t->eng_req == ENGI_DRUM || t->eng_req == ENGI_DRUMLAB || eng_gone(t->eng_req))
        return;
    /* An unsaved engine switch is not a new saved baseline. Do not mutate it. */
    if (!random_baseline_valid[k] || random_saved_slot[k] >= 4u ||
        random_baseline_engine[k] != t->eng_req)
        return;
    /* A human pad's first press timing diversifies the sequence across boots,
     * while keeping all subsequent choices deterministic for host regressions. */
    if (!random_seed_ready) {
        random_seed ^= (fm1_ms + 1u) * 0x9e3779b9u;
        random_seed_ready = 1u;
    }
    n = random_sound_pool(t, pool, sizeof pool);
    take = cmd == 0u ? 2u : 3u;
    if (take > n) take = n;
    strength = cmd == 0u ? 20u : 40u;
    for (i = 0; i < take; i++) {
        uint32_t j = i + random_next() % (n - i);
        uint8_t id = pool[i];
        int32_t offset, v;
        pool[i] = pool[j];
        pool[j] = id;
        offset = (int32_t)(strength + random_next() % (cmd == 0u ? 41u : 51u));
        if (random_next() & 1u) offset = -offset;
        v = clamp((int32_t)random_baseline[k][id] + offset, 0, 127);
        t->p[id] = (int16_t)v;
        /* Match ordinary MIDI CC edits: update motion's live base so a later
         * explicit SAVE captures this sound rather than an old automation value. */
        (void)motion_capture(t, id, (int16_t)v);
    }
    random_ui_dirty = 1u;
}
