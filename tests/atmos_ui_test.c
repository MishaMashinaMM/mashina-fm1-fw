/* SPDX-License-Identifier: GPL-3.0-only
 * ATMOS: EDIT 3 navigation, descriptors, factory recall and lane isolation.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
int main(void)
{
    int bad = 0;
    uint32_t edit3 = NPAGES, levels3 = NPAGES;
    ui_power_on();
    set_engine_of(TSEL, ENGI_ATMOS);
    TSEL->engine = TSEL->eng_req;
    for (uint32_t p = 0; p < NPAGES; p++) {
        if (str_eq(PAGES[p].title, "EDIT 3")) edit3 = p;
        if (str_eq(PAGES[p].title, "LEVELS 3")) levels3 = p;
    }
    bad += check("ATMOS EDIT 3 page visible", edit3 < NPAGES && page_visible(edit3));
    bad += check("DRUM-LAB LEVELS 3 hidden on ATMOS", levels3 < NPAGES && !page_visible(levels3));
    bad += check("factory KANT extra controls recalled",
                 TSEL->p[P_LN8] == ATMOS_EXTRA_PRESETS[0][0] &&
                 TSEL->p[P_LN9] == ATMOS_EXTRA_PRESETS[0][1] &&
                 TSEL->p[P_LN10] == ATMOS_EXTRA_PRESETS[0][2] &&
                 TSEL->p[P_LN11] == ATMOS_EXTRA_PRESETS[0][3]);
    bad += check("third page displays ATMOS descriptors",
                 str_eq(track_desc(TSEL, P_LN8)->label, "BREATH") &&
                 str_eq(track_desc(TSEL, P_LN9)->label, "MOTION") &&
                 str_eq(track_desc(TSEL, P_LN10)->label, "FLOW") &&
                 str_eq(track_desc(TSEL, P_LN11)->label, "SPREAD"));
    go_title("EDIT 3"); frame();
    uint32_t before = (uint32_t)TSEL->p[P_LN9];
    turn(EN_K2, 1); frame();
    bad += check("EDIT 3 K2 edits MOTION", TSEL->p[P_LN9] > before);
    apply_preset(1u);
    bad += check("BURKE preset recalls all four extras",
                 TSEL->p[P_LN8] == ATMOS_EXTRA_PRESETS[1][0] &&
                 TSEL->p[P_LN9] == ATMOS_EXTRA_PRESETS[1][1] &&
                 TSEL->p[P_LN10] == ATMOS_EXTRA_PRESETS[1][2] &&
                 TSEL->p[P_LN11] == ATMOS_EXTRA_PRESETS[1][3]);
    set_engine_of(TSEL, ENGI_DRUMLAB);
    TSEL->engine = TSEL->eng_req;
    bad += check("switch to DRUM-LAB restores lane defaults",
                 TSEL->p[P_LN8] == TP[P_LN8].def &&
                 TSEL->p[P_LN9] == TP[P_LN9].def &&
                 TSEL->p[P_LN10] == TP[P_LN10].def &&
                 TSEL->p[P_LN11] == TP[P_LN11].def);
    bad += check("DRUM-LAB LEVELS 3 visible", levels3 < NPAGES && page_visible(levels3));
    printf("ATMOS UI TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
