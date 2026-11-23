/* SPDX-License-Identifier: GPL-3.0-only
 * All drum engines: octave repeating factory layout, exact custom MIDI map,
 * unchanged pad notes, layered note duplicates and project roundtrip.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int map_check(const char *label, int yes)
{
    printf("all drum MIDI: %-66s %s\n", label, yes ? "PASS" : "FAIL");
    return !yes;
}
static uint32_t hit_note(track_t *t, uint32_t note, uint32_t kind)
{
    uint32_t before = vage, hits = 0u;
    trk_note_on(t, note, 110u);
    for (uint32_t i = 0; i < NVOICE; i++)
        if (t->v[i].active && t->v[i].age > before &&
            (kind == 255u || (t->engine == ENGI_DRUMAC ?
             ac_state[t-trk][i].kind : t->engine == ENGI_DRUMIND ?
             ind_state[t-trk][i].kind : t->engine == ENGI_DRUM ?
             (uint32_t)t->v[i].s[0] : drumlab_lane(t->v[i].note)) == kind))
            hits++;
    trk_note_off(t, note);
    return hits;
}
static void ready_engine(track_t *t, uint32_t engine)
{
    set_engine_of(t, engine);
    t->engine = t->eng_req;
    t->xf_on = 0u;
    t->p[P_MUTE] = 0;
    memset(t->v, 0, sizeof t->v);
}
int main(void)
{
    int bad = 0;
    track_t *t = &trk[0];
    ui_power_on();
    ready_engine(t, ENGI_DRUM);
    drum_midi_custom[0] = 0;
    bad += map_check("DRUM-X KICK repeats on C at 24/36/48/60",
                     hit_note(t, 24u, DV_KICK) && hit_note(t, 36u, DV_KICK) &&
                     hit_note(t, 48u, DV_KICK) && hit_note(t, 60u, DV_KICK));
    bad += map_check("DRUM-X BELL repeats on B at 35/47/59/71",
                     hit_note(t, 35u, DV_BELL) && hit_note(t, 47u, DV_BELL) &&
                     hit_note(t, 59u, DV_BELL) && hit_note(t, 71u, DV_BELL));
    bad += map_check("custom X mapping disables octave repeat",
                     drum_note_set(t, DV_BELL, 80u) &&
                     hit_note(t, 80u, DV_BELL) && !hit_note(t, 59u, DV_BELL));

    ready_engine(t, ENGI_DRUMLAB);
    drumlab_midi_custom[0] = 0;
    bad += map_check("DRUM-LAB factory repeats across octaves",
                     hit_note(t, 36u, 0u) && hit_note(t, 48u, 0u));
    bad += map_check("DRUM-LAB custom KICK 80 / SNARE 81 is exact",
                     drumlab_note_set(t, 0u, 80u) &&
                     drumlab_note_set(t, 1u, 81u) &&
                     hit_note(t, 80u, 0u) && hit_note(t, 81u, 1u) &&
                     !hit_note(t, 36u, 255u) && !hit_note(t, 48u, 255u));
    bad += map_check("unchanged DRUM-LAB pad 2 stays only on MIDI 38",
                     hit_note(t, 38u, 2u) && !hit_note(t, 50u, 255u));
    bad += map_check("DRUM-LAB layering allows two pads on MIDI 80",
                     drumlab_note_set(t, 1u, 80u) && hit_note(t, 80u, 0u) &&
                     hit_note(t, 80u, 1u));

    ready_engine(t, ENGI_DRUMAC);
    drumlab_midi_custom[0] = 0u;
    bad += map_check("DRUM-AC default kick and snare repeat",
                     hit_note(t, 48u, AC_KICK) && hit_note(t, 50u, AC_SNARE));
    bad += map_check("DRUM-AC custom KICK 78 / SNARE 79 is exact",
                     drumlab_note_set(t, AC_KICK, 78u) &&
                     drumlab_note_set(t, AC_SNARE, 79u) &&
                     hit_note(t, 78u, AC_KICK) && hit_note(t, 79u, AC_SNARE) &&
                     !hit_note(t, 36u, 255u) && !hit_note(t, 48u, 255u));
    bad += map_check("unchanged DRUM-AC CRASH remains MIDI 37 only",
                     drumlab_midi_note[0][AC_CRASH] == AC_NOTES[AC_CRASH] &&
                     hit_note(t, 37u, AC_CRASH) && !hit_note(t, 49u, 255u));

    bad += map_check("DRUM-AC mapping survives project save", project_save(0) == 0);
    drumlab_midi_custom[0] = 0u;
    project_load(0);
    bad += map_check("DRUM-AC mapping survives project load",
                     drumlab_midi_custom[0] &&
                     drumlab_midi_note[0][AC_KICK] == 78u &&
                     drumlab_midi_note[0][AC_SNARE] == 79u);

    ready_engine(t, ENGI_DRUMIND);
    drumlab_midi_custom[0] = 0u;
    bad += map_check("DRUM-IND default CLANG / ANVIL repeat",
                     hit_note(t, 48u, IND_CLANG) && hit_note(t, 49u, IND_ANVIL));
    bad += map_check("DRUM-IND custom CLANG 78 / ANVIL 79 is exact",
                     drumlab_note_set(t, IND_CLANG, 78u) &&
                     drumlab_note_set(t, IND_ANVIL, 79u) &&
                     hit_note(t, 78u, IND_CLANG) && hit_note(t, 79u, IND_ANVIL) &&
                     !hit_note(t, 36u, 255u) && !hit_note(t, 48u, 255u));
    bad += map_check("unchanged DRUM-IND METAL stays only on MIDI 38",
                     hit_note(t, 38u, IND_METAL) && !hit_note(t, 50u, 255u));
    bad += map_check("DRUM-IND mapping survives project save", project_save(0) == 0);
    drumlab_midi_custom[0] = 0u;
    project_load(0);
    bad += map_check("DRUM-IND mapping survives project load",
                     drumlab_midi_custom[0] &&
                     drumlab_midi_note[0][IND_CLANG] == 78u &&
                     drumlab_midi_note[0][IND_ANVIL] == 79u);
    printf("ALL DRUM MIDI MAPPING %s (%d failed)\n", bad ? "FAILED" : "PASSED", bad);
    return !!bad;
}
