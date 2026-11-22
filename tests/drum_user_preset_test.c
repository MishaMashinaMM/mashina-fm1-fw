/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: all twelve DRUM-AC/DRUM-IND pad parameters and global controls
 * must be independent in every user preset slot (including slots 16 and 17).
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void set_kit(uint32_t eng, uint32_t marker)
{
    track_t *t = TSEL;
    if (eng == ENGI_DRUMAC) {
        ac_patch_reset(t);
        for (uint32_t pad = 0; pad < AC_NPAD; ++pad)
            for (uint32_t f = 0; f < 8u; ++f)
                ac_patch_set(t, pad, f, (marker + pad * 7u + f * 3u) & 127u);
    } else {
        ind_patch_reset(t);
        for (uint32_t pad = 0; pad < IND_NPAD; ++pad)
            for (uint32_t f = 0; f < 8u; ++f)
                ind_patch_set(t, pad, f, (marker + pad * 7u + f * 3u) & 127u);
    }
    drumlab_global_reset((uint32_t)(t - trk));
    for (uint32_t f = 0; f < 8u; ++f)
        drumlab_global[t - trk][f] = (uint8_t)(marker + f * 6u);
}
static int verify_kit(uint32_t eng, uint32_t marker)
{
    track_t *t = TSEL;
    for (uint32_t pad = 0; pad < 12u; ++pad)
        for (uint32_t f = 0; f < 8u; ++f) {
            uint32_t got = eng == ENGI_DRUMAC ? ac_param_value(t, pad, f) :
                                                      ind_param_value(t, pad, f);
            if (got != ((marker + pad * 7u + f * 3u) & 127u)) return 0;
        }
    for (uint32_t f = 0; f < 8u; ++f)
        if (drumlab_global[t - trk][f] != marker + f * 6u) return 0;
    return 1;
}
int main(void)
{
    int bad = 0;
    ui_power_on();
    upkit_empty();
    for (uint32_t engn = 0; engn < 2; ++engn) {
        uint32_t eng = engn ? ENGI_DRUMIND : ENGI_DRUMAC;
        uint32_t slot0 = engn ? 16u : 0u, slot1 = slot0 + 1u, slot2 = slot0 + 2u;
        set_engine_of(TSEL, eng);
        TSEL->engine = TSEL->eng_req;
        for (uint32_t i = 0; i < 3u; ++i) {
            set_kit(eng, 11u + i * 21u);
            bad += check("save each full 12-pad kit to its separate user slot",
                         up_store(slot0 + i, "KIT") == 3);
        }
        bad += check("three independent snapshots in flash bank RAM",
                     ((upkit.used >> slot0) & 7u) == 7u);
        for (uint32_t i = 0; i < 3u; ++i) {
            set_kit(eng, 88u); /* deliberately overwrite the current live kit */
            bad += check("user kit loads from its own slot",
                         !up_load(slot0 + i) && verify_kit(eng, 11u + i * 21u));
        }
        bad += check("rename preserves kit data", up_rename(slot0, "RENAMED") == 3);
        bad += check("renamed kit still restores exact pads and global values",
                     !up_load(slot0) && verify_kit(eng, 11u));
        /* Re-save the third kit: the first two must not change. */
        set_kit(eng, 76u);
        bad += check("overwrite slot 3 with new kit", up_store(slot2, "NEW KIT") == 3);
        bad += check("first kit unchanged", !up_load(slot0) && verify_kit(eng, 11u));
        bad += check("second kit unchanged", !up_load(slot1) && verify_kit(eng, 32u));
        bad += check("third kit changed independently", !up_load(slot2) && verify_kit(eng, 76u));
        /* Fail-safe: an invalid saved snapshot must NOT inherit the last edited kit. */
        upkit.e[slot1].tag++;
        set_kit(eng, 99u);
        bad += check("stale snapshot rejected, live pad state cleared",
                     !up_load(slot1) && !verify_kit(eng, 99u) &&
                     (eng == ENGI_DRUMAC ? !ac_dirty[TSEL - trk] : !ind_dirty[TSEL - trk]));
    }
    printf("DRUM USER PRESET TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
