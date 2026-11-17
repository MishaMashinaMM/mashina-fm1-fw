/* SPDX-License-Identifier: GPL-3.0-only
 * ZONES must reuse the six-row mixer and navigation, without hijacking PRESETS.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

int main(void)
{
    int bad = 0;
    uint32_t zones_page = NPAGES;
    ui_power_on();
    for (uint32_t i = 0; i < NPAGES; i++)
        if (str_eq(PAGES[i].title, "ZONES")) zones_page = i;
    bad += check("six-row ZONES page is in GLO", zones_page < NPAGES &&
                 page_visible(zones_page) && PAGES[zones_page].scope == SC_TRK);
    {
        uint8_t list[32];
        uint32_t n = nav_list(SEC_GLO, list), found = 0;
        for (uint32_t i = 0; i < n; i++)
            found += nv_page(SEC_GLO, list[i]) == zones_page;
        bad += check("GLO includes ZONES exactly once", found == 1u);
        bad += check("MIXER is still first GLO page",
                     nv_page(SEC_GLO, 0u) < NPAGES &&
                     str_eq(PAGES[nv_page(SEC_GLO, 0u)].title, "MIXER"));
        nav_go(SEC_GLO, 1u, 0);
        frame();
        bad += check("ZONES is immediately after MIXER",
                     !ui.home && ui.page == zones_page);
    }
    for (uint32_t i = 0; i < NTRK; i++)
        bad += check("every track defaults to full note/velocity range",
                     midi_zone[i][0] == 0 && midi_zone[i][1] == 127 &&
                     midi_zone[i][2] == 1 && midi_zone[i][3] == 127);
    song.sel = 0;
    turn(EN_K1, 1); turn(EN_K2, -1);
    turn(EN_K3, 1); turn(EN_K4, -1); frame();
    bad += check("K1-K4 adjust note/velocity boundaries",
                 midi_zone[0][0] > 0 && midi_zone[0][1] < 127 &&
                 midi_zone[0][2] > 1 && midi_zone[0][3] < 127);
    bad += check("other track retains neutral ranges",
                 midi_zone[1][0] == 0 && midi_zone[1][1] == 127 &&
                 midi_zone[1][2] == 1 && midi_zone[1][3] == 127);
    midi_zone[0][0] = midi_zone[0][1] = 60;
    midi_zone[0][2] = midi_zone[0][3] = 100;
    turn(EN_K1, 1); turn(EN_K2, -1);
    turn(EN_K3, 1); turn(EN_K4, -1); frame();
    bad += check("MIN never exceeds MAX (including velocity)",
                 midi_zone[0][0] == 60 && midi_zone[0][1] == 60 &&
                 midi_zone[0][2] == 100 && midi_zone[0][3] == 100);
    midi_zone[4][0] = 20; midi_zone[5][3] = 90;
    project_capture(&proj_scratch);
    midi_zone_reset();
    project_restore_runtime(&proj_scratch);
    bad += check("T1, T5 and T6 zones restored with project",
                 midi_zone[0][0] == 60 && midi_zone[0][2] == 100 &&
                 midi_zone[4][0] == 20 && midi_zone[5][3] == 90);
    system_init();
    bad += check("NEW PROJECT resets all six zones", midi_zone[0][0] == 0 &&
                 midi_zone[0][1] == 127 && midi_zone[0][2] == 1 &&
                 midi_zone[0][3] == 127 && midi_zone[4][0] == 0 &&
                 midi_zone[5][3] == 127);
    printf("KEYBOARD ZONES UI TESTS %s (%d failed)\n",
           bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
