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
            return id == P_E3 || id == P_E5;
        if (t->p[P_E1] == 4u)
            return id == P_E3 || id == P_E5 || id == P_E6 || id == P_E7;
        return id == P_E2 || (id == P_E3 && mode != 4u) || id == P_E6 || id == P_E7;
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
