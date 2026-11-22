/* SPDX-License-Identifier: GPL-3.0-only
 * INDUSTRIAL 3-page editor integration and DRUM-LAB level isolation. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

int main(void)
{
    int bad = 0;
    uint32_t edit3 = NPAGES, levels3 = NPAGES;
    ui_power_on();
    set_engine_of(TSEL, ENGI_INDUSTRIAL);
    TSEL->engine = TSEL->eng_req;
    for (uint32_t p = 0; p < NPAGES; p++) {
        if (str_eq(PAGES[p].title, "EDIT 3")) edit3 = p;
        if (str_eq(PAGES[p].title, "LEVELS 3")) levels3 = p;
    }
    bad += check("INDUSTRIAL-only EDIT 3 page visible",
                 edit3 < NPAGES && page_visible(edit3));
    bad += check("DRUM-LAB LEVELS 3 hidden on INDUSTRIAL",
                 levels3 < NPAGES && !page_visible(levels3));
    bad += check("factory PASCAL extra values recalled",
                 TSEL->p[P_LN8] == INDUSTRIAL_EXTRA_PRESETS[0][0] &&
                 TSEL->p[P_LN9] == INDUSTRIAL_EXTRA_PRESETS[0][1] &&
                 TSEL->p[P_LN10] == INDUSTRIAL_EXTRA_PRESETS[0][2] &&
                 TSEL->p[P_LN11] == INDUSTRIAL_EXTRA_PRESETS[0][3]);
    {
        uint8_t entries[32];
        uint32_t n = nav_list(SEC_EDIT, entries), found = 0, k;
        for (k = 0; k < n; k++)
            found += nv_page(SEC_EDIT, entries[k]) == edit3;
        bad += check("actual EDIT navigation includes third page", found == 1u);
        nav_go(SEC_EDIT, 2u, 0); frame();
        bad += check("navigating to third EDIT entry opens its controls",
                     !ui.home && ui.page == edit3);
    }
    go_title("EDIT 3"); frame();
    uint32_t before = (uint32_t)TSEL->p[P_LN9];
    turn(EN_K2, 1); frame();
    bad += check("EDIT 3 second knob edits MIX",
                 TSEL->p[P_LN9] > before);
    apply_preset(1u);
    bad += check("CAVENDISH resets extra knobs to its factory values",
                 TSEL->p[P_LN8] == INDUSTRIAL_EXTRA_PRESETS[1][0] &&
                 TSEL->p[P_LN9] == INDUSTRIAL_EXTRA_PRESETS[1][1] &&
                 TSEL->p[P_LN10] == INDUSTRIAL_EXTRA_PRESETS[1][2] &&
                 TSEL->p[P_LN11] == INDUSTRIAL_EXTRA_PRESETS[1][3]);
    set_engine_of(TSEL, ENGI_DRUMLAB);
    TSEL->engine = TSEL->eng_req;
    bad += check("leaving INDUSTRIAL restores drum lane default levels",
                 TSEL->p[P_LN8] == TP[P_LN8].def &&
                 TSEL->p[P_LN9] == TP[P_LN9].def &&
                 TSEL->p[P_LN10] == TP[P_LN10].def &&
                 TSEL->p[P_LN11] == TP[P_LN11].def);
    bad += check("DRUM-LAB LEVELS 3 remains visible",
                 levels3 < NPAGES && page_visible(levels3));
    bad += check("INDUSTRIAL EDIT 3 hidden for DRUM-LAB",
                 edit3 < NPAGES && !page_visible(edit3));
    printf("INDUSTRIAL UI TESTS %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
