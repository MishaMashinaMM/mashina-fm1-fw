/* SPDX-License-Identifier: GPL-3.0-only
 * Universal track timing: delayed note-on AND note-off, duration preservation,
 * short notes and independent layered tracks sharing MIDI input.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int check(const char *label, int yes)
{
    printf("UNIVERSAL TIMING: %-57s %s\n", label, yes ? "PASS" : "FAIL");
    return !yes;
}
static int voices_active(track_t *t)
{
    for (uint32_t k = 0; k < NVOICE; ++k)
        if (t->v[k].active) return 1;
    return 0;
}
static int voices_held(track_t *t)
{
    for (uint32_t k = 0; k < NVOICE; ++k)
        if (t->v[k].active && t->v[k].gate) return 1;
    return 0;
}
static void choose(track_t *t, uint32_t e)
{
    trk_all_off(t);
    set_engine_of(t,e);
    t->engine = t->eng_req; t->xf_on = 0;
    t->p[P_MUTE] = 0;
    memset(t->v,0,sizeof t->v);
}
int main(void)
{
    int bad = 0;
    track_t *a = &trk[0], *b = &trk[1];
    ui_power_on();
    choose(a,ENGI_ANALOG);
    choose(b,ENGI_BRASS);
    drum_track_delay[0]=0;
    drum_track_delay[1]=30;
    trk_note_on(a,60,100);
    trk_note_on(b,60,100);
    bad+=check("Different engine tracks: immediate A / pending B",
               voices_held(a) && !voices_active(b));
    trk_note_off(b,60); /* instantaneous input note */
    drum_delay_process(FS*29u/1000u);
    bad+=check("Short note not audible before 30 ms", !voices_active(b));
    drum_delay_process(FS*2u/1000u);
    bad+=check("Short note does start after offset", voices_active(b));
    bad+=check("Short note scheduled release is independent", !voices_held(b));
    trk_all_off(a); trk_all_off(b);
    choose(b,ENGI_ATMOS);
    drum_track_delay[1]=25;
    trk_note_on(b,64,100);
    drum_delay_process(FS*26u/1000u);
    bad+=check("Delayed melodic note is held after trigger", voices_held(b));
    /* Note Off arrives 26 ms after input; it must be delayed 25 ms too. */
    trk_note_off(b,64);
    drum_delay_process(FS*23u/1000u);
    bad+=check("Original duration preserved until postponed off",voices_held(b));
    drum_delay_process(FS*3u/1000u);
    bad+=check("Melodic note released after equal offset",!voices_held(b));
    trk_all_off(b);
    choose(b,ENGI_ANALOG);
    drum_track_delay[1]=25;
    trk_note_on(b,65,100);
    trk_all_off(b);
    drum_delay_process(FS/10u);
    bad+=check("Panic/track-off cancels pending melodic note",!voices_active(b));
    printf("UNIVERSAL TRACK TIMING %s (%d failed)\n",bad?"FAILED":"PASSED",bad);
    return !!bad;
}
