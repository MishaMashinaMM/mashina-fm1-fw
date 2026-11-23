/* SPDX-License-Identifier: GPL-3.0-only
 * DRUM-X BELL note / live EDIT audition mapping regression.
 * User MIDI maps remain separate from internal GM sound note numbers.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int bell_check(const char *label, int yes)
{
    printf("drumx bell: %-64s %s\n", label, yes ? "PASS" : "FAIL");
    return !yes;
}
static uint32_t active_lane_count(track_t *t, uint32_t lane, uint32_t since)
{
    uint32_t n = 0;
    for (uint32_t k = 0; k < NVOICE; k++)
        n += t->v[k].active && t->v[k].age > since &&
             t->v[k].s[0] == lane;
    return n;
}
int main(void)
{
    int bad = 0;
    uint32_t v0;
    track_t *t = &trk[0];
    ui_power_on();
    set_engine_of(t, ENGI_DRUM);
    t->engine = t->eng_req; /* complete the simulated engine switch before Note On */
    drum_midi_custom[0] = 0u;
    bad += bell_check("factory BELL is MIDI 47 (B2), the end of C2..B2",
                      DRUM_NOTE12[DV_BELL] == 47u);
    bad += bell_check("legacy internal BELL synthesis remains MIDI 56",
                      DRUM_LANE_NOTE[DV_BELL] == 56u &&
                      drum_lane(56u) == DV_BELL && drum_lane(47u) == DV_TOM);
    bad += bell_check("default MIDI 47 selects BELL for EDIT",
                      dx_mapped_lane(t, 47u) == DV_BELL);
    bad += bell_check("default kit repeats BELL at every B (35, 47, 59, 71)",
                      dx_mapped_lane(t, 35u) == DV_BELL &&
                      dx_mapped_lane(t, 59u) == DV_BELL &&
                      dx_mapped_lane(t, 71u) == DV_BELL);
    bad += bell_check("other pads repeat across octaves (C kick, G# TOM2)",
                      dx_mapped_lane(t, 48u) == DV_KICK &&
                      dx_mapped_lane(t, 56u) == DV_T2);
    memset(t->v, 0, sizeof t->v);
    v0 = vage;
    trk_note_on(t, 47u, 110u);
    bad += bell_check("MIDI 47 actually triggers the BELL sound",
                      active_lane_count(t, DV_BELL, v0) == 1u);
    trk_note_off(t, 47u);
    memset(t->v, 0, sizeof t->v);
    v0 = vage;
    trk_note_on(t, 59u, 110u);
    bad += bell_check("BELL sounds on MIDI 59 in next octave too",
                      active_lane_count(t, DV_BELL, v0) == 1u);
    trk_note_off(t, 59u);
    trk_note_off(t, 47u);
    bad += bell_check("a saved custom BELL mapping can override factory 47",
                      drum_note_set(t, DV_BELL, 60u) &&
                      dx_mapped_lane(t, 60u) == DV_BELL &&
                      dx_mapped_lane(t, 47u) == DV_NLANE12);
    bad += bell_check("custom MIDI map disables all-octave repetition",
                      dx_mapped_lane(t, 48u) == DV_NLANE12 &&
                      dx_mapped_lane(t, 59u) == DV_NLANE12);
    /* An EDIT note selects the pad corresponding to its live assignment. */
    dx_ui_lane = DV_KICK;
    dx_ui_seen = 36u;
    last_note = 60u;
    dx_follow();
    bad += bell_check("tapping custom BELL note selects it on SOUND pages",
                      dx_ui_lane == DV_BELL);
    dx_ui_lane = DV_KICK;
    dx_ui_seen = 60u;
    last_note = 49u;
    dx_follow();
    bad += bell_check("unmapped note leaves manual SOUND selection intact",
                      dx_ui_lane == DV_KICK);
    /* Duplicate mapping layers two sounds; lowest-index lane wins EDIT. */
    bad += bell_check("duplicate note assignment is accepted",
                      drum_note_set(t, DV_KICK, 60u));
    memset(t->v, 0, sizeof t->v);
    v0 = vage;
    trk_note_on(t, 60u, 110u);
    bad += bell_check("duplicate MIDI 60 triggers both KICK and BELL",
                      active_lane_count(t, DV_KICK, v0) &&
                      active_lane_count(t, DV_BELL, v0));
    bad += bell_check("EDIT selects first matching lane in pad order",
                      dx_mapped_lane(t, 60u) == DV_KICK);
    dx_ui_seen = 47u;
    last_note = 60u;
    dx_follow();
    bad += bell_check("EDIT follows that deterministic shared-note winner",
                      dx_ui_lane == DV_KICK);
    dx_ui_lane = DV_BELL; /* user manually chose another pad */
    dx_follow();
    bad += bell_check("manual LANE selection persists without another tap",
                      dx_ui_lane == DV_BELL);
    last_note_serial++; /* simulate a fresh tap of the SAME MIDI note 60 */
    dx_follow();
    bad += bell_check("repeated same-note tap reselects the layered first pad",
                      dx_ui_lane == DV_KICK);
    bad += bell_check("each track has independent mappings",
                      dx_mapped_lane(&trk[1], 47u) == DV_BELL);
    printf("DRUM-X BELL MAP %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
