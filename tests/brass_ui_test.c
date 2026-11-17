/* SPDX-License-Identifier: GPL-3.0-only
 * BRASS EDIT 3 navigation, four-knob edit, factory defaults and DRUM isolation.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

int main(void)
{
    int bad = 0;
    uint32_t edit3 = NPAGES, levels3 = NPAGES;
    ui_power_on();
    set_engine_of(TSEL, ENGI_BRASS);
    TSEL->engine = TSEL->eng_req;
    for (uint32_t p = 0; p < NPAGES; p++) {
        if (str_eq(PAGES[p].title, "EDIT 3")) edit3 = p;
        if (str_eq(PAGES[p].title, "LEVELS 3")) levels3 = p;
    }
    bad += check("BRASS third EDIT page is visible",
                 edit3 < NPAGES && page_visible(edit3));
    bad += check("DRUM levels page stays hidden on BRASS",
                 levels3 < NPAGES && !page_visible(levels3));
    bad += check("BRASS factory extras start fully neutral",
                 !TSEL->p[P_LN8] && !TSEL->p[P_LN9] &&
                 !TSEL->p[P_LN10] && !TSEL->p[P_LN11]);
    {
        uint8_t list[32];
        uint32_t n = nav_list(SEC_EDIT, list), found = 0;
        for (uint32_t k = 0; k < n; k++)
            found += nv_page(SEC_EDIT, list[k]) == edit3;
        bad += check("BRASS EDIT 3 reachable in real navigation", found == 1u);
        nav_go(SEC_EDIT, 2u, 0); frame();
        bad += check("third tab opens correct EDIT 3 page",
                     !ui.home && ui.page == edit3);
    }
    {
        int32_t before = TSEL->p[P_LN9];
        turn(EN_K2, 1); frame();
        bad += check("EDIT 3 GRIND responds to second knob",
                     TSEL->p[P_LN9] > before);
    }
    apply_preset(4u);
    bad += check("BRASS LEAD loads TRUMPET end of TYPE morph",
                 TSEL->p[P_E0] == 127);
    bad += check("factory recall resets all four industrial additions",
                 !TSEL->p[P_LN8] && !TSEL->p[P_LN9] &&
                 !TSEL->p[P_LN10] && !TSEL->p[P_LN11]);
    set_engine_of(TSEL, ENGI_DRUMLAB);
    TSEL->engine = TSEL->eng_req;
    bad += check("DRUM-LAB existing LEVELS 3 remains visible",
                 levels3 < NPAGES && page_visible(levels3));
    bad += check("DRUM-LAB EDIT 3 remains hidden",
                 edit3 < NPAGES && !page_visible(edit3));
    bad += check("drum levels restored on engine switch",
                 TSEL->p[P_LN8] == TP[P_LN8].def &&
                 TSEL->p[P_LN9] == TP[P_LN9].def &&
                 TSEL->p[P_LN10] == TP[P_LN10].def &&
                 TSEL->p[P_LN11] == TP[P_LN11].def);
    printf("BRASS UI TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
