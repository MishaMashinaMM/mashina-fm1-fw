/* SPDX-License-Identifier: GPL-3.0-only
 * Regression: EDIT knob changes must never clear and repaint the whole
 * 240x198 body. This is visibly perceived as blinking on the FM1 LCD.
 * All three 12-pad synth kits share the same draw_engine_grid_cell path.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int knob_redraw_check(uint32_t engine, const char *name)
{
    int bad=0;
    uint32_t before_full;
    uint64_t before_px,delta;
    ui_power_on();
    set_engine_of(TSEL,engine);
    TSEL->engine=TSEL->eng_req;
    go_title("SOUND");
    frame();
    frame(); /* warm the four-cell cache and header/footer signatures */
    before_full=host_big_fills;
    before_px=ui_sent_px;
    turn(EN_K2,1);
    delta=ui_sent_px-before_px;
    char label[110];
    snprintf(label,sizeof label,"%s SOUND knob: no full LCD clear",name);
    bad+=check(label,host_big_fills==before_full);
    snprintf(label,sizeof label,"%s SOUND knob: one edited grid cell, not four",name);
    bad+=check(label,delta>0u && delta<24000u);
    before_full=host_big_fills;
    before_px=ui_sent_px;
    frame(); /* unchanged data: no 4-cell repaint */
    delta=ui_sent_px-before_px;
    snprintf(label,sizeof label,"%s idle EDIT frame: no body blanking",name);
    bad+=check(label,host_big_fills==before_full && delta<24000u);

    go_title("EDIT 1");
    frame();frame();
    before_full=host_big_fills;
    before_px=ui_sent_px;
    turn(EN_K1,-1);
    delta=ui_sent_px-before_px;
    snprintf(label,sizeof label,"%s global EDIT knob: no full LCD clear",name);
    bad+=check(label,host_big_fills==before_full);
    snprintf(label,sizeof label,"%s global EDIT knob: only changed cell",name);
    bad+=check(label,delta>0u && delta<24000u);
    /* Explicit navigation/wake force still repaints all four cells. */
    before_full=host_big_fills;
    ui.force=1u;frame();
    snprintf(label,sizeof label,"%s page force: full redraw remains available",name);
    bad+=check(label,host_big_fills>before_full);
    return bad;
}
int main(void)
{
    int bad=0;
    bad+=knob_redraw_check(ENGI_DRUMLAB,"DRUM-LAB");
    bad+=knob_redraw_check(ENGI_DRUMAC,"DRUM-AC");
    bad+=knob_redraw_check(ENGI_DRUMIND,"DRUM-IND");
    printf("DRUM EDIT LCD REDRAW %s (%d failed)\n",bad?"FAILED":"PASSED",bad);
    return !!bad;
}
