/* SPDX-License-Identifier: GPL-3.0-only
 * Additive drum trigger delay: track + pad, zero-latency default,
 * short MIDI hits and project persistence. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int timing_check(const char *label, int ok)
{
    printf("DRUM TIMING: %-57s %s\n", label, ok ? "PASS" : "FAIL");
    return !ok;
}
static int active(const track_t *t)
{
    for (uint32_t i = 0; i < NVOICE; i++)
        if (t->v[i].active) return 1;
    return 0;
}
static void ready(track_t *t, uint32_t e)
{
    set_engine_of(t, e);
    t->engine = t->eng_req;
    t->xf_on = 0u;
    t->p[P_MUTE] = 0;
    trk_all_off(t);
    memset(t->v, 0, sizeof t->v);
}
int main(void)
{
    static const uint8_t engines[] = {
        ENGI_DRUM, ENGI_DRUMLAB, ENGI_DRUMAC, ENGI_DRUMIND
    };
    int bad = 0;
    ui_power_on();
    for (uint32_t k = 0; k < 4u; k++) {
        track_t *a = &trk[0], *b = &trk[1];
        ready(a, engines[k]);
        ready(b, engines[k]);
        drum_track_delay[0] = 0u;
        drum_track_delay[1] = 12u;
        drum_pad_delay[0][0] = 0u;
        drum_pad_delay[1][0] = 7u;
        trk_note_on(a, 36u, 120u);
        trk_note_on(b, 36u, 120u);
        bad += timing_check("Track A immediate, Track B pending", active(a) && !active(b));
        trk_note_off(b, 36u); /* short physical strike must not discard pending hit */
        drum_delay_process(FS * 18u / 1000u);
        bad += timing_check("Track B still silent before 19ms", !active(b));
        drum_delay_process(FS * 2u / 1000u);
        bad += timing_check("Track B audible after additive 19ms", active(b));
        trk_all_off(a);
        trk_all_off(b);
    }
    ready(&trk[0], ENGI_DRUMAC);
    ready(&trk[1], ENGI_DRUMAC);
    drum_track_delay[0] = 0u;
    drum_pad_delay[0][0] = 0u;
    drum_track_delay[1] = 12u;
    drum_pad_delay[1][0] = 7u;
    bad += timing_check("Project saves timing offsets", project_save(0) == 0);
    drum_track_delay[1] = drum_pad_delay[1][0] = 0u;
    project_load(0);
    bad += timing_check("Project restores both offsets",
                        drum_track_delay[1] == 12u && drum_pad_delay[1][0] == 7u);
    trk_all_off(&trk[1]);
    memset(trk[1].v, 0, sizeof trk[1].v);
    trk_note_on(&trk[1], 36u, 120u);
    bad += timing_check("Note queued before panic/track-off", drum_delay_pending > 0u);
    trk_all_off(&trk[1]);
    drum_delay_process(FS / 10u);
    bad += timing_check("Pending hits discarded on track-off", !active(&trk[1]));
    printf("ADDITIVE DRUM TIMING %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
