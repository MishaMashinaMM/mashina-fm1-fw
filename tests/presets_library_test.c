/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental 32 USER-first + 32 factory-library PC map regression.
 * No physical flash writes or new library storage.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int pass(const char *what, int ok)
{
    printf("LIBRARY MAP: %-65s %s\n", what, ok ? "PASS" : "FAIL");
    return !ok;
}

int main(void)
{
    int bad = 0, fixed = 1, factory = 1, kept = 1, moves = 1;
    uint32_t total, i, k, e;
    ui_power_on();
    song.sel = 0;
    preset_all_pos(&total);
    bad += pass("total fixed at 64 slots, regardless of occupied USER count", total == 64u);
    for (i = 0; i < UP_SLOTS; i++) {
        e = preset_all_at(i, &k);
        if (e != NENGINES || k != i) fixed = 0;
    }
    bad += pass("PC 0..31 map to physical USER slot 0..31", fixed);
    for (i = UP_SLOTS; i < UP_SLOTS + LIB_FACTORY_SLOTS; i++) {
        e = preset_all_at(i, &k);
        if (e >= NENGINES || !eng_ok(e) || k >= ENGINES[e]->npresets)
            factory = 0;
    }
    bad += pass("PC 32..63 map to a valid built-in sound", factory);
    /* PC 31 is an unoccupied user slot: must do nothing, not load factory #1.
     * If a user preset was stored, it must keep its PC identity. */
    {
        const uint32_t previous_engine = TSEL->eng_req;
        int16_t previous_e = TSEL->p[P_E0];
        midi_pc_req[0] = 32u; /* PC 31 is represented as n+1 */
        midi_pc_poll();
        kept = TSEL->eng_req == previous_engine && TSEL->p[P_E0] == previous_e;
    }
    bad += pass("empty USER PC doesn't substitute an unrelated sound", kept);
    /* A detached, unlisted factory sound cannot masquerade as slot 33. */
    for (i = 0; i < NENG_SHOWN; i++) {
        uint32_t engine = eng_vis(i), j;
        for (j = 0; j < ENGINES[engine]->npresets; j++) {
            if (library_factory_rank(engine,j) < LIB_FACTORY_SLOTS) continue;
            set_engine_of(TSEL,engine);
            TSEL->preset = (uint8_t)j;
            TSEL->user = 0;
            moves = preset_all_pos(&total) == 64u && total == 64u;
            goto done;
        }
    }
done:
    bad += pass("off-list factory sound has no invented slot number", moves);
    return !!bad;
}
