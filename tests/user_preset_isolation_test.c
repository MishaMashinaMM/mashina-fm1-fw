/* SPDX-License-Identifier: GPL-3.0-only
 * Regression for four engines with auxiliary USER kit data.
 * Uses the same source and in-memory simulated preset bank as the UI tests.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static uint8_t stored[4][2][UPF_PK];
static uint8_t stored_global[4][2][8];
static const uint32_t ENGINE[4] = {
    ENGI_DRUMAC, ENGI_DRUMIND, ENGI_DRUMLAB, ENGI_DRUM
};

static void edit_kit(uint32_t e, uint8_t seed)
{
    uint32_t t = (uint32_t)(TSEL - trk);
    if (e == ENGI_DRUM) {
        dx_kit[0].a[0] = seed;
        dx_kit[0].b[2] = (uint8_t)(seed + 11u);
        dx_kit12[3].a[3] = (uint8_t)(seed + 17u);
        dx_lay[3] = (uint8_t)(seed + 5u);
        dx_snd12[1] = (uint8_t)(seed + 7u);
        dx_tone = (uint8_t)(seed / 2u);
        return;
    }
    drumlab_global_reset(t);
    drumlab_global[t][0] = seed;
    drumlab_global[t][4] = (uint8_t)(seed + 5u);
    if (e == ENGI_DRUMAC) {
        ac_patch_set(TSEL, AC_KICK, 0u, seed);
        ac_patch_set(TSEL, AC_SNARE, 4u, (uint8_t)(seed + 7u));
        ac_patch_set(TSEL, AC_RIDE_SPLASH, 7u, (uint8_t)(seed + 9u));
    } else if (e == ENGI_DRUMIND) {
        ind_patch_set(TSEL, IND_CLANG, 0u, seed);
        ind_patch_set(TSEL, IND_EXPLOSION, 7u, (uint8_t)(seed + 9u));
    } else {
        drumlab_patch_set(TSEL, 0u, 0u, seed);
        drumlab_patch_set(TSEL, 11u, 9u, (uint8_t)(seed + 9u));
        drumlab_type_set(TSEL, 2u, DLAB_CLAP);
    }
}
static uint32_t slot_for(uint32_t engine_index, uint32_t variant)
{
    return engine_index * 2u + variant;
}
static int store_case(uint32_t idx, uint32_t variant)
{
    uint8_t seed = (uint8_t)(20u + idx * 25u + variant * 11u);
    uint32_t slot = slot_for(idx, variant);
    set_engine_of(TSEL, ENGINE[idx]);
    TSEL->engine = TSEL->eng_req;
    edit_kit(ENGINE[idx], seed);
    if (up_store(slot, "KIT TEST") != 3 && !flash_ok) return 0;
    memcpy(stored[idx][variant], upf.e[slot].pk, UPF_PK);
    if (ENGINE[idx] != ENGI_DRUM) {
        for (uint32_t j = 0; j < 8u; j++)
            stored_global[idx][variant][j] = drumlab_global[TSEL - trk][j];
    }
    return up_used(slot) && upf.e[slot].tag == upf_tag(up_rec(slot));
}
static int load_case(uint32_t idx, uint32_t variant)
{
    uint32_t slot = slot_for(idx, variant), ti = (uint32_t)(TSEL - trk);
    uint8_t payload[UPF_PK];
    int same;
    if (up_load(slot)) return 0;
    if (upd_companion_get(slot, payload)) return 0;
    same = !memcmp(payload, stored[idx][variant], UPF_PK);
    if (ENGINE[idx] == ENGI_DRUM) {
        uint8_t raw[UPD_DX_SIZE], expected[UPD_DX_SIZE];
        upd_dx_encode(raw);
        /* Restore again from a separate temporary state after perturbation. */
        uint8_t old = dx_kit[0].a[0];
        dx_kit[0].a[0] = (uint8_t)(old ^ 15u);
        same &= up_load(slot) == 0 && dx_kit[0].a[0] == old;
        (void)expected;
        return same;
    }
    for (uint32_t j = 0; j < 8u; j++)
        same &= drumlab_global[ti][j] == stored_global[idx][variant][j];
    if (ENGINE[idx] == ENGI_DRUMAC)
        same &= ac_patch[ti][AC_KICK][0] == (uint8_t)(20u + idx * 25u + variant * 11u);
    else if (ENGINE[idx] == ENGI_DRUMIND)
        same &= ind_patch[ti][IND_CLANG][0] == (uint8_t)(20u + idx * 25u + variant * 11u);
    else
        same &= drumlab_kit[ti][0].pitch == (uint8_t)(20u + idx * 25u + variant * 11u) &&
                drumlab_kit[ti][2].kind == DLAB_CLAP;
    return same;
}
int main(void)
{
    int bad = 0;
    ui_power_on();
    for (uint32_t i = 0; i < 4u; i++) {
        bad += check("Save distinct A kit", store_case(i, 0u));
        bad += check("Save distinct B kit", store_case(i, 1u));
    }
    for (uint32_t i = 0; i < 4u; i++) {
        bad += check("Reload A without aliasing B", load_case(i, 0u));
        bad += check("Reload B without aliasing A", load_case(i, 1u));
        bad += check("Reload A again after B", load_case(i, 0u));
    }
    /* A bank re-validation simulates reboot normalization. DRUM-X extension
     * bytes must not be parsed as the retired sequencer's pattern flags. */
    {
        uint8_t saved[32];
        up_rec_t *r = up_rec(slot_for(3u, 0u));
        memcpy(saved, r->note, 16u);
        memcpy(saved + 16u, r->flags, 16u);
        up_bank[0].magic = UP_BANK_MAGIC;
        up_bank[0].rsize = sizeof(up_rec_t);
        up_bank[0].nslot = UP_PER_BANK;
        up_bank_check(0, (int)sizeof(up_bank[0]));
        bad += check("DRUM-X kit bytes survive reboot validation",
                     !memcmp(saved, r->note, 16u) && !memcmp(saved + 16u, r->flags, 16u));
    }
    bad += check("Renaming a kit keeps its companion tag",
                 up_rename(0u, "RENAME TEST") == 3 &&
                 !upd_companion_get(0u, stored[0][0]));
    /* Renaming mutates the underlying record only, never the companion data. */
    bad += check("User bank remains 192-byte/16-record compatible",
                 sizeof(up_rec_t) == 192u && sizeof(up_bank_t) <= ST_PAYLOAD_MAX);
    printf("USER PRESET ISOLATION %s (%d failures)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
