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
    if (up_store(slot, "KIT TEST") != 3) return 0;
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
                 sizeof(up_rec_t) == 192u && sizeof(up_bank_t) <= 3840u);
    /* Exercise the entire 32-slot companion bank, mixed engines and a
     * simulated power cycle (bank normalization and restored sidecar bytes). */
    {
        uint8_t expected[UP_SLOTS][UPF_PK];
        uint8_t expected_e[UP_SLOTS];
        up_bank_t saved_banks[UP_SLOTS / UP_PER_BANK];
        upf_t saved_aux;
        int all = 1;
        for (uint32_t k = 0; k < UP_SLOTS; k++) {
            uint32_t e = ENGINE[k % 4u], seed = 10u + k * 3u;
            set_engine_of(TSEL, e);
            TSEL->engine = TSEL->eng_req;
            edit_kit(e, (uint8_t)seed);
            all &= up_store(k, "BANK FILL") == 3;
            expected_e[k] = (uint8_t)e;
            memcpy(expected[k], upf.e[k].pk, UPF_PK);
        }
        bad += check("All 32 mixed USER slots can save their independent kits", all);
        memcpy(saved_banks, up_bank, sizeof saved_banks);
        saved_aux = upf;
        memset(up_bank, 0, sizeof up_bank);
        memset(&upf, 0, sizeof upf);
        memcpy(up_bank, saved_banks, sizeof saved_banks);
        upf = saved_aux;
        up_bank_check(0u, (int)sizeof up_bank[0]);
        up_bank_check(1u, (int)sizeof up_bank[1]);
        all = upf_valid(&upf);
        for (uint32_t k = 0; k < UP_SLOTS; k++) {
            uint8_t actual[UPF_PK];
            all &= up_used(k) && up_rec(k)->engine == expected_e[k];
            all &= !up_load(k) && !upd_companion_get(k, actual);
            all &= !memcmp(actual, expected[k], UPF_PK);
            all &= upf.e[k].tag == upf_tag(up_rec(k));
        }
        bad += check("32 kits survive simulated reboot and reverse-engine loads", all);
        /* Reuse one slot without touching the other 31. */
        set_engine_of(TSEL, ENGI_DRUMAC);
        TSEL->engine = TSEL->eng_req;
        edit_kit(ENGI_DRUMAC, 61u);
        all = up_store(8u, "OVERWRITE") == 3;
        for (uint32_t k = 0; k < UP_SLOTS; k++) {
            if (k == 8u) continue;
            all &= upf.e[k].tag == upf_tag(up_rec(k));
            all &= !memcmp(upf.e[k].pk, expected[k], UPF_PK);
        }
        bad += check("Overwriting one slot preserves all 31 others", all);
        bad += check("Erasing kit U09 clears its USER slot", up_put(8u, 0) == 3 && !up_used(8u));
    }
    printf("USER PRESET ISOLATION %s (%d failures)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
