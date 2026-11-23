/* SPDX-License-Identifier: GPL-3.0-only
 * Per-track drum MIDI reset: long SEL, confirm/cancel and all four engines.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int reset_check(const char *msg, int yes)
{
    printf("DRUM MAP RESET: %-64s %s\n", msg, yes ? "PASS" : "FAIL");
    return !yes;
}
int main(void)
{
    int bad = 0;
    static const uint8_t ENGINES_TO_TEST[4] =
        {ENGI_DRUM, ENGI_DRUMLAB, ENGI_DRUMAC, ENGI_DRUMIND};
    for (uint32_t i = 0; i < 4u; i++) {
        track_t *t, *other;
        uint32_t e = ENGINES_TO_TEST[i];
        uint8_t saved_note;
        ui_power_on();
        t = &trk[0]; other = &trk[1];
        set_engine_of(t, e);
        t->engine = t->eng_req;
        set_engine_of(other, ENGI_DRUM);
        other->engine = other->eng_req;
        song.sel = 0;
        /* A different track's custom assignment must survive every reset. */
        drum_midi_custom[1] = 0u;
        drum_note_set(other, DV_BELL, 80u);
        if (e == ENGI_DRUM) {
            drum_midi_custom[0] = 0u;
            drum_note_set(t, DV_BELL, 79u);
            saved_note = drum_midi_note[0][DV_BELL];
        } else {
            drumlab_midi_custom[0] = 0u;
            drumlab_note_set(t, 0u, 79u);
            saved_note = drumlab_midi_note[0][0];
        }
        /* Keep a non-MIDI parameter: reset must not initialize the sound. */
        t->p[P_E0] = 61;
        press(B_SEQ);
        bad += reset_check("SEQ MIDI MAP opens for the selected track", ui.drum_map_open);
        press(B_SCL);
        bad += reset_check("short SEL does not reset or navigate",
                           ui.drum_map_open && !ui.drum_map_reset_confirm);
        bad += reset_check("custom state remains unchanged after short SEL",
                           e == ENGI_DRUM ?
                           drum_midi_custom[0] && drum_midi_note[0][DV_BELL] == saved_note :
                           drumlab_midi_custom[0] && drumlab_midi_note[0][0] == saved_note);
        hold(B_SCL);
        bad += reset_check("long SEL opens confirmation without changing map",
                           ui.drum_map_open && ui.drum_map_reset_confirm &&
                           ui.drum_map_reset_trk == 0 &&
                           (e == ENGI_DRUM ? drum_midi_custom[0] : drumlab_midi_custom[0]));
        press(B_OCTDN);
        bad += reset_check("OCT- cancels without clearing custom assignments",
                           !ui.drum_map_reset_confirm &&
                           (e == ENGI_DRUM ? drum_midi_custom[0] : drumlab_midi_custom[0]));
        hold(B_SCL);
        press(B_OCTUP);
        bad += reset_check("OCT+ returns this track to DEFAULT mode",
                           !ui.drum_map_reset_confirm &&
                           !(e == ENGI_DRUM ? drum_midi_custom[0] : drumlab_midi_custom[0]));
        bad += reset_check("other track is untouched",
                           drum_midi_custom[1] && drum_midi_note[1][DV_BELL] == 80u);
        bad += reset_check("sound parameter unaffected by MIDI map reset", t->p[P_E0] == 61);
        bad += reset_check("MIDI MAP stays open after reset", ui.drum_map_open);
    }
    printf("DRUM MAP RESET %s (%d failures)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
