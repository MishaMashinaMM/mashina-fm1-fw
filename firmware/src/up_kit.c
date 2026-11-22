/* SPDX-License-Identifier: GPL-3.0-only
 * MASHINA: independent user-preset drum kit snapshots.
 *
 * A user preset record stores P_COUNT ordinary sound parameters, not the 12
 * separate pad voices of DRUM-AC/DRUM-IND. Retain a complete 12x8 kit plus the
 * eight kit-wide controls for each user slot in a dedicated A/B flash object.
 * This does not change existing 192-byte user preset records or the synthesis.
 *
 * The record's unused packed bytes after P_COUNT hold a checksum of the
 * corresponding kit. The snapshot also records upf_tag(record) so a failed
 * second-stage write, overwritten slot or old firmware record cannot restore
 * another kit by accident. Preset renames do not change the tag.
 */
#define UPKIT_MAGIC 0x544B5055u        /* "UPKT" */
#define UPKIT_VERSION 1u
#define UPKIT_PADS 12u
#define UPKIT_FIELDS 8u
#define UPKIT_STAMP_AT P_COUNT
_Static_assert(UPKIT_STAMP_AT + 4u <= UP_FM_AT, "no free user-record stamp bytes");

typedef struct {
    uint32_t tag;
    uint8_t global[8];
    uint8_t pad[UPKIT_PADS][UPKIT_FIELDS];
} upkit_ent_t;
typedef struct {
    uint32_t magic;
    uint16_t version, nslot;
    uint32_t used, reserved;
    upkit_ent_t e[UP_SLOTS];
} upkit_t;
_Static_assert(sizeof(upkit_ent_t) == 108u, "complete drum kit entry");
_Static_assert(sizeof(upkit_t) == 3472u && sizeof(upkit_t) <= 3840u,
               "drum preset snapshots fit the two free flash sectors");

static upkit_t upkit __attribute__((section(".pool")));

static int upkit_valid(const upkit_t *b)
{
    if (b->magic != UPKIT_MAGIC || b->version != UPKIT_VERSION || b->nslot != UP_SLOTS)
        return 0;
    for (uint32_t i = 0; i < UP_SLOTS; ++i) {
        if (!((b->used >> i) & 1u)) continue;
        for (uint32_t j = 0; j < 8u; ++j)
            if (b->e[i].global[j] > 200u) return 0;
        for (uint32_t j = 0; j < UPKIT_PADS; ++j)
            for (uint32_t k = 0; k < UPKIT_FIELDS; ++k)
                if (b->e[i].pad[j][k] > 127u) return 0;
    }
    return 1;
}

static void upkit_empty(void)
{
    memset(&upkit, 0, sizeof upkit);
    upkit.magic = UPKIT_MAGIC;
    upkit.version = UPKIT_VERSION;
    upkit.nslot = UP_SLOTS;
}
static void upkit_boot(void)
{
#if FELUCCA_FLASH
    if (flash_ok && st_load(OBJ_UPKIT, &upkit, sizeof upkit) == (int)sizeof upkit &&
        upkit_valid(&upkit)) return;
#endif
    upkit_empty();     /* older v0.11 installations have no kit snapshots */
}

static int upkit_engine(uint32_t engine)
{
    return engine == ENGI_DRUMAC || engine == ENGI_DRUMIND;
}

static void upkit_capture(track_t *t, upkit_ent_t *e)
{
    uint32_t ti = (uint32_t)(t - trk);
    memset(e, 0, sizeof *e);
    drumlab_global_ensure(ti);
    memcpy(e->global, drumlab_global[ti], sizeof e->global);
    if (t->eng_req == ENGI_DRUMAC) {
        ac_patch_ensure(t);
        memcpy(e->pad, ac_patch[ti], sizeof e->pad);
    } else if (t->eng_req == ENGI_DRUMIND) {
        ind_patch_ensure(t);
        memcpy(e->pad, ind_patch[ti], sizeof e->pad);
    }
}

static uint32_t upkit_crc(const upkit_ent_t *e)
{
    const uint8_t *bytes = e->global;
    uint32_t hash = 2166136261u;
    for (uint32_t i = 0; i < sizeof e->global + sizeof e->pad; ++i)
        hash = (hash ^ bytes[i]) * 16777619u;
    return hash;
}
static void upkit_stamp(up_rec_t *r, const upkit_ent_t *e)
{
    uint32_t checksum = upkit_crc(e);
    for (uint32_t i = 0; i < 4u; ++i)
        r->packed[UPKIT_STAMP_AT + i] = (uint8_t)(checksum >> (8u * i));
}
static uint32_t upkit_saved_stamp(const up_rec_t *r)
{
    uint32_t result = 0;
    for (uint32_t i = 0; i < 4u; ++i)
        result |= (uint32_t)r->packed[UPKIT_STAMP_AT + i] << (8u * i);
    return result;
}

static int upkit_save(void)
{
#if FELUCCA_FLASH
    if (flash_ok) return st_save(OBJ_UPKIT, &upkit, sizeof upkit) ? 2 : 0;
#endif
    return 3;
}

/* The base record has already been committed. Never report the whole
 * operation as successful if its kit snapshot cannot also be persisted. */
static int upkit_store(uint32_t k, const upkit_ent_t *snapshot)
{
    upkit_ent_t previous = upkit.e[k];
    uint32_t was_used = upkit.used;
    if (!upkit_valid(&upkit)) upkit_empty();
    upkit.e[k] = *snapshot;
    upkit.e[k].tag = upf_tag(up_rec(k));
    upkit.used |= 1u << k;
    int rc = upkit_save();
    if (rc == 2) {
        upkit.e[k] = previous;
        upkit.used = was_used;
    }
    return rc;
}

static int upkit_get(uint32_t k, const upkit_ent_t **e)
{
    if (k >= UP_SLOTS || !up_used(k) || !upkit_engine(up_rec(k)->engine) ||
        !upkit_valid(&upkit) || !((upkit.used >> k) & 1u)) return 0;
    const upkit_ent_t *p = &upkit.e[k];
    if (p->tag != upf_tag(up_rec(k)) || upkit_saved_stamp(up_rec(k)) != upkit_crc(p))
        return 0;
    *e = p;
    return 1;
}

static int upkit_track_load(track_t *t, uint32_t k)
{
    const upkit_ent_t *e;
    uint32_t ti = (uint32_t)(t - trk);
    int ok = upkit_get(k, &e);
    if (t->eng_req == ENGI_DRUMAC) ac_patch_reset(t);
    else if (t->eng_req == ENGI_DRUMIND) ind_patch_reset(t);
    drumlab_global_reset(ti);
    if (!ok) return 0; /* legacy record: use fresh defaults, never previous kit */
    memcpy(drumlab_global[ti], e->global, sizeof e->global);
    drumlab_global_ready[ti] = 1u;
    if (t->eng_req == ENGI_DRUMAC) {
        memcpy(ac_patch[ti], e->pad, sizeof e->pad);
        ac_ready[ti] = 1u;
        ac_dirty[ti] = 0x0FFFu;
        memset(ac_override[ti], 0xFF, sizeof ac_override[ti]);
    } else {
        memcpy(ind_patch[ti], e->pad, sizeof e->pad);
        ind_ready[ti] = 1u;
        ind_dirty[ti] = 0x0FFFu;
        memset(ind_override[ti], 0xFF, sizeof ind_override[ti]);
    }
    return 1;
}
