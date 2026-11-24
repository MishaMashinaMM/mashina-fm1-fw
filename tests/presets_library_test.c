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

    /* On SAVE > PRESETS, K4 is a bank selector, not Jiant's sound-tag list.
     * Slot 047 must remain selectable even if its stored sound cannot load.
     * K1 / PRESETS traverse all 32 physical LIBRARY positions. */
    ui_power_on();
    my_steps(TSEL);
    go_page(GR_BROWSE);
    browse_bank = BROWSE_ALL;
    browse_cursor_valid = 0u;
    turn(EN_K4,1);
    fixed = browse_bank == BROWSE_USER && browse_length() == 32u;
    turn(EN_K4,1);
    fixed &= browse_bank == BROWSE_LIBRARY && browse_length() == 32u;
    browse_cursor = 45u; /* displayed slot 046, zero-based */
    browse_cursor_valid = 1u;
    turn(EN_K1,1);
    {
        uint32_t pos = preset_pos(&total);
        fixed &= browse_cursor == 46u && pos == 14u &&
                 preset_number(pos) == 47u && total == 32u;
    }
    bad += pass("K4 ALL / USER / LIB, and 046 advances to 047 (not 033)", fixed);
    fixed = 1;
    for (i=32u;i<64u;i++) {
        browse_cursor = (uint8_t)i;
        browse_cursor_valid = 1u;
        turn(EN_K1,1);
        fixed &= browse_cursor == (i+1u<64u?i+1u:32u);
    }
    bad += pass("All 32 LIBRARY positions 033..064 browse and wrap only at 064", fixed);
    turn(EN_K4,-1);
    turn(EN_K4,-1);
    bad += pass("K4 restores fixed 64-slot ALL directory",
                browse_bank == BROWSE_ALL && browse_length() == 64u);
    return !!bad;
}
