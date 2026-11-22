/* SPDX-License-Identifier: GPL-3.0-only
 * Independent USER kits for DRUM-LAB, DRUM-AC and DRUM-IND.
 *
 * Reuse the existing 32-slot, atomic A/B UPFM6 companion bank.  An individual
 * USER slot holds either an FM6 voice or a drum kit, never both.  The sidecar's
 * normal record tag includes the packed globals below, and backup id 9 already
 * archives this bank. The original 192-byte USER records remain unchanged.
 *
 * 112-byte auxiliary payload:
 * LAB: 120 ten-parameter 7-bit values (105 bytes), 12 four-bit kinds
 *      (6 bytes), format byte (1).
 * AC/IND: 96 eight-parameter 7-bit values (84 bytes), 12 override bitmasks
 *      (12 bytes), edited-pad mask (2 bytes), format byte (1).
 * Eight 0..200 global values are stored in unused USER record packed[] bytes.
 */
#define UPD_GLOBAL_AT P_COUNT
#define UPD_MARK_AT (P_COUNT + 8u)
#define UPD_MARK 0xA9u
#define UPD_FMT 0xD3u
#define UPD_DX_MARK 0xDAu
#define UPD_DX_SIZE 149u
/* DX's 12 modes and 120 7-bit oscillator values plus send, layer and kit
 * controls need 149 bytes. 111 bytes fit the shared companion slot, with
 * 15 bytes in the main record's unused packed[] tail and 23 bytes in its
 * obsolete sequencer-pattern fields. The pattern was never part of a sound
 * and is no longer exposed by MASHINA's sequencer-free interface. */
_Static_assert(sizeof(dx_lane_t) == 11u, "DRUM-X lane format changed");

_Static_assert(UPD_MARK_AT < UP_FM_AT, "USER record must have spare packed bytes for kit-wide controls");

static int upd_engine(uint32_t e)
{
    return e == ENGI_DRUM || e == ENGI_DRUMLAB || e == ENGI_DRUMAC || e == ENGI_DRUMIND;
}

/* Bit-packed 7-bit values are read/written without any dependency on CPU
 * endianness. The last unused bits are zero. */
static void upd_pack7(uint8_t *dst, const uint8_t *src, uint32_t count)
{
    uint32_t acc = 0, bits = 0, out = 0;
    for (uint32_t i = 0; i < count; i++) {
        acc |= (uint32_t)(src[i] & 127u) << bits;
        bits += 7u;
        while (bits >= 8u) {
            dst[out++] = (uint8_t)acc;
            acc >>= 8u;
            bits -= 8u;
        }
    }
    if (bits) dst[out] = (uint8_t)acc;
}
static void upd_unpack7(uint8_t *dst, const uint8_t *src, uint32_t count)
{
    uint32_t acc = 0, bits = 0, pos = 0;
    for (uint32_t i = 0; i < count; i++) {
        while (bits < 7u) {
            acc |= (uint32_t)src[pos++] << bits;
            bits += 8u;
        }
        dst[i] = (uint8_t)(acc & 127u);
        acc >>= 7u;
        bits -= 7u;
    }
}
/* DRUM-X's kit is shared by the drum engine (as in project.c): save and
 * restore a full copy, not a pointer to the currently playing kit. */
static void upd_dx_encode(uint8_t *raw)
{
    uint8_t values[120];
    memset(raw, 0, UPD_DX_SIZE);
    for (uint32_t lane = 0; lane < 12u; lane++) {
        const dx_lane_t *p = lane < 8u ? &dx_kit[lane] : &dx_kit12[lane - 8u];
        raw[105u + lane] = p->mode;
        for (uint32_t f = 0; f < 5u; f++) {
            values[lane * 10u + f] = p->a[f];
            values[lane * 10u + f + 5u] = p->b[f];
        }
    }
    upd_pack7(raw, values, 120u);
    memcpy(raw + 117u, dx_lay, 8u);
    memcpy(raw + 125u, dx_snd, 8u);
    memcpy(raw + 133u, dx_lay12, 4u);
    memcpy(raw + 137u, dx_snd12, 4u);
    memcpy(raw + 141u, dx_mot, 4u);
    raw[145] = dx_tone;
    raw[146] = dx_hands;
    raw[147] = (uint8_t)dx_mute;
    raw[148] = (uint8_t)(dx_mute >> 8u);
}
static void upd_dx_extras(up_rec_t *r, const uint8_t *raw)
{
    /* raw[0..110] go to the sidecar; [111..148] are in this record. */
    for (uint32_t i = 0; i < 8u; i++)
        r->packed[UPD_GLOBAL_AT + i] = raw[111u + i];
    r->packed[UPD_MARK_AT] = UPD_DX_MARK;
    for (uint32_t i = 0; i < 7u; i++)
        r->packed[UPD_MARK_AT + 1u + i] = raw[119u + i];
    for (uint32_t i = 0; i < 16u; i++)
        r->note[i] = raw[126u + i];
    for (uint32_t i = 0; i < 7u; i++)
        r->flags[i] = raw[142u + i];
    for (uint32_t i = 7u; i < 16u; i++)
        r->flags[i] = 0u;
}
static int upd_dx_decode(const up_rec_t *r, const uint8_t *side)
{
    uint8_t raw[UPD_DX_SIZE], values[120];
    if (r->np != P_COUNT || r->ver < 4u ||
        r->packed[UPD_MARK_AT] != UPD_DX_MARK) return 1;
    memcpy(raw, side, 111u);
    for (uint32_t i = 0; i < 8u; i++)
        raw[111u + i] = r->packed[UPD_GLOBAL_AT + i];
    for (uint32_t i = 0; i < 7u; i++)
        raw[119u + i] = r->packed[UPD_MARK_AT + 1u + i];
    for (uint32_t i = 0; i < 16u; i++)
        raw[126u + i] = r->note[i];
    for (uint32_t i = 0; i < 7u; i++)
        raw[142u + i] = r->flags[i];
    upd_unpack7(values, raw, 120u);
    for (uint32_t lane = 0; lane < 12u; lane++) {
        dx_lane_t *p = lane < 8u ? &dx_kit[lane] : &dx_kit12[lane - 8u];
        p->mode = raw[105u + lane];
        for (uint32_t f = 0; f < 5u; f++) {
            p->a[f] = values[lane * 10u + f];
            p->b[f] = values[lane * 10u + f + 5u];
        }
    }
    memcpy(dx_lay, raw + 117u, 8u);
    memcpy(dx_snd, raw + 125u, 8u);
    memcpy(dx_lay12, raw + 133u, 4u);
    memcpy(dx_snd12, raw + 137u, 4u);
    memcpy(dx_mot, raw + 141u, 4u);
    dx_tone = raw[145];
    dx_hands = raw[146];
    dx_mute = (uint16_t)(raw[147] | (uint16_t)raw[148] << 8u);
    return 0;
}
static void upd_dx_capture(up_rec_t *r)
{
    uint8_t raw[UPD_DX_SIZE];
    upd_dx_encode(raw);
    upd_dx_extras(r, raw);
}

static void upd_globals_store(up_rec_t *r, uint32_t tr)
{
    drumlab_global_ensure(tr);
    for (uint32_t i = 0; i < 8u; i++)
        r->packed[UPD_GLOBAL_AT + i] = drumlab_global[tr][i];
    r->packed[UPD_MARK_AT] = UPD_MARK;
}
static void upd_globals_load(track_t *t, const up_rec_t *r)
{
    uint32_t tr = (uint32_t)(t - trk);
    drumlab_global_reset(tr);
    if (r->np == P_COUNT && r->ver >= 4u && r->packed[UPD_MARK_AT] == UPD_MARK) {
        for (uint32_t i = 0; i < 8u; i++)
            drumlab_global[tr][i] = r->packed[UPD_GLOBAL_AT + i] <= 200u ?
                                    r->packed[UPD_GLOBAL_AT + i] : 100u;
    }
}
static int upd_companion_get(uint32_t k, uint8_t *dst)
{
    if (k >= UP_SLOTS || !up_used(k) || !upd_engine(up_rec(k)->engine) ||
        !upf_valid(&upf) || !(upf.used & (1u << k)) ||
        upf.e[k].tag != upf_tag(up_rec(k)))
        return 1;
    memcpy(dst, upf.e[k].pk, UPF_PK);
    return dst[UPF_PK - 1u] == UPD_FMT ? 0 : 1;
}
static int upd_companion_store(uint32_t k, const uint8_t *data)
{
    if (k >= UP_SLOTS || !up_used(k)) return 2;
    if (!upf_valid(&upf)) upf_empty();
    memcpy(upf.e[k].pk, data, UPF_PK);
    upf.e[k].tag = upf_tag(up_rec(k));
    upf.used |= 1u << k;
    return upf_save();
}

/* Captures before the USER record is committed; note that all twelve pads,
 * including pads never opened in the editor, are materialized from this track.
 */
static void upd_capture_globals(up_rec_t *r, track_t *t)
{
    if (r->engine == ENGI_DRUM) upd_dx_capture(r);
    else if (upd_engine(r->engine)) upd_globals_store(r, (uint32_t)(t - trk));
}
static int upd_store(uint32_t k, track_t *t)
{
    uint32_t ti = (uint32_t)(t - trk), lane, field;
    uint8_t data[UPF_PK] = {0};
    uint8_t values[120] = {0};
    if (ti >= NPART || !upd_engine(t->eng_req)) return 0;
    if (t->eng_req == ENGI_DRUM) {
        uint8_t raw[UPD_DX_SIZE];
        upd_dx_encode(raw);
        memcpy(data, raw, 111u);
        data[UPF_PK - 1u] = UPD_FMT;
        return upd_companion_store(k, data);
    }
    if (t->eng_req == ENGI_DRUMLAB) {
        drumlab_kit_ensure(t);
        for (lane = 0; lane < 12u; lane++) {
            const uint8_t *p = &drumlab_kit[ti][lane].pitch;
            for (field = 0; field < 10u; field++)
                values[lane * 10u + field] = p[field];
        }
        upd_pack7(data, values, 120u);                 /* 105 bytes */
        for (lane = 0; lane < 12u; lane++) {
            uint8_t kind = drumlab_kit[ti][lane].kind & 15u;
            if (lane & 1u) data[105u + lane / 2u] |= (uint8_t)(kind << 4u);
            else data[105u + lane / 2u] = kind;
        }
    } else {
        uint16_t mask;
        if (t->eng_req == ENGI_DRUMAC) {
            ac_patch_ensure(t);
            mask = ac_dirty[ti];
            for (lane = 0; lane < 12u; lane++) {
                for (field = 0; field < 8u; field++)
                    values[lane * 8u + field] = ac_patch[ti][lane][field];
                data[84u + lane] = ac_override[ti][lane];
            }
        } else {
            ind_patch_ensure(t);
            mask = ind_dirty[ti];
            for (lane = 0; lane < 12u; lane++) {
                for (field = 0; field < 8u; field++)
                    values[lane * 8u + field] = ind_patch[ti][lane][field];
                data[84u + lane] = ind_override[ti][lane];
            }
        }
        upd_pack7(data, values, 96u);                  /* 84 bytes */
        data[96] = (uint8_t)mask;
        data[97] = (uint8_t)(mask >> 8u);
    }
    data[UPF_PK - 1u] = UPD_FMT;
    return upd_companion_store(k, data);
}
/* Called after the standard USER values are applied, before the load ends.
 * Reset previous kits even when a legacy USER slot has no companion entry. */
static void upd_track_load(track_t *t, uint32_t k)
{
    uint32_t ti = (uint32_t)(t - trk), lane, field;
    uint8_t data[UPF_PK], values[120];
    uint16_t mask;
    if (ti >= NPART || !upd_engine(t->eng_req)) return;
    if (t->eng_req == ENGI_DRUM) {
        if (!upd_companion_get(k, data)) upd_dx_decode(up_rec(k), data);
        return;
    }
    upd_globals_load(t, up_rec(k));
    if (t->eng_req == ENGI_DRUMLAB) {
        drumlab_kit_ready[ti] = 0u;
        drumlab_kit_dirty[ti] = 0u;
        drumlab_kit_ensure(t);
    } else if (t->eng_req == ENGI_DRUMAC) {
        ac_patch_reset(t);
        ac_patch_ensure(t);
    } else {
        ind_patch_reset(t);
        ind_patch_ensure(t);
    }
    if (upd_companion_get(k, data)) return;
    if (t->eng_req == ENGI_DRUMLAB) {
        upd_unpack7(values, data, 120u);
        for (lane = 0; lane < 12u; lane++) {
            uint8_t kind = (lane & 1u) ? data[105u + lane / 2u] >> 4u :
                                           data[105u + lane / 2u] & 15u;
            if (kind >= DLAB_KIND_COUNT) return;
        }
        for (lane = 0; lane < 12u; lane++) {
            uint8_t *p = &drumlab_kit[ti][lane].pitch;
            for (field = 0; field < 10u; field++)
                p[field] = values[lane * 10u + field];
            drumlab_kit[ti][lane].kind = (lane & 1u) ? data[105u + lane / 2u] >> 4u :
                                                          data[105u + lane / 2u] & 15u;
        }
        drumlab_kit_dirty[ti] = 0x0FFFu;
    } else {
        upd_unpack7(values, data, 96u);
        mask = (uint16_t)(data[96] | (uint16_t)data[97] << 8u) & 0x0FFFu;
        for (lane = 0; lane < 12u; lane++) {
            for (field = 0; field < 8u; field++)
                if (t->eng_req == ENGI_DRUMAC)
                    ac_patch[ti][lane][field] = values[lane * 8u + field];
                else
                    ind_patch[ti][lane][field] = values[lane * 8u + field];
            if (t->eng_req == ENGI_DRUMAC)
                ac_override[ti][lane] = data[84u + lane];
            else
                ind_override[ti][lane] = data[84u + lane];
        }
        if (t->eng_req == ENGI_DRUMAC) ac_dirty[ti] = mask;
        else ind_dirty[ti] = mask;
    }
}
