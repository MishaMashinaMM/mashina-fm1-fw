/* SPDX-License-Identifier: GPL-3.0-only
 * Branch regression: dedicated MIDI CC85/86/87 sound randomization.
 * Uses the actual MIDI dispatch, project storage and UI-thread sound recall.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void random_cc_test(uint32_t ch, uint32_t cc, uint32_t value)
{
    midi_control(ch - 1u, cc, value);
}
/* Exercise the shared MIDI queue used by USB and TRS, not only the command handler. */
static void queued_random_cc(uint32_t ch, uint32_t cc, uint32_t value, uint32_t source)
{
    uint32_t status = 0xB0u + ch - 1u;
    midi_enqueue((status >> 4) | status << 8 | cc << 16 | value << 24, source);
    events_block(CTL);
}
static void random_test_reset_state(void)
{
    uint32_t k;
    memset(random_baseline_valid, 0, sizeof random_baseline_valid);
    memset(random_pad_held, 0, sizeof random_pad_held);
    memset(random_last_ms, 0, sizeof random_last_ms);
    random_reset_pending = random_ui_dirty = 0u;
    for (k = 0; k < 3u; k++) random_saved_slot[k] = 0xffu;
}
static int pool_has(const uint8_t *p, uint32_t n, uint32_t id)
{
    uint32_t i;
    for (i = 0; i < n; i++) if (p[i] == id) return 1;
    return 0;
}
static int random_whitelist_test(void)
{
    static const uint8_t engines[] = {
        ENGI_ANALOG, ENGI_PHASE, ENGI_LOFI, ENGI_VOICE,
        ENGI_WHEEL, ENGI_NOISE, 12u, ENGI_ANIMAL,
        ENGI_DRUM, ENGI_DRUMLAB
    };
    uint8_t ids[12];
    uint32_t e, i, j, n;
    int valid = 1, special = 1;
    track_t *t = &trk[0];
    for (e = 0; e < sizeof engines; e++) {
        t->eng_req = engines[e];
        t->p[P_E0] = t->p[P_E1] = t->p[P_VLANG] = 0;
        n = random_sound_pool(t, ids, sizeof ids);
        if (n > sizeof ids) valid = 0;
        if ((engines[e] == ENGI_DRUM || engines[e] == ENGI_DRUMLAB) && n)
            valid = 0;
        for (i = 0; i < n; i++) {
            const param_desc_t *d = track_desc(t, ids[i]);
            if (!d || d->min != 0 || d->max != 127) valid = 0;
            for (j = 0; j < i; j++) if (ids[i] == ids[j]) valid = 0;
        }
        switch (engines[e]) {
        case ENGI_ANALOG:
            special &= !pool_has(ids, n, P_E0) && !pool_has(ids, n, P_E1) &&
                       !pool_has(ids, n, P_E7);
            break;
        case ENGI_PHASE:
            special &= !pool_has(ids, n, P_E3) && !pool_has(ids, n, P_E4);
            break;
        case ENGI_VOICE:
            special &= !pool_has(ids, n, P_E2) && !pool_has(ids, n, P_E3) &&
                       !pool_has(ids, n, P_E7);
            t->p[P_VLANG] = 1;
            n = random_sound_pool(t, ids, sizeof ids);
            special &= !pool_has(ids, n, P_E0) && !pool_has(ids, n, P_E1);
            break;
        case ENGI_WHEEL:
            special &= !pool_has(ids, n, P_E0) && !pool_has(ids, n, P_E7);
            break;
        case ENGI_NOISE:
            special &= !pool_has(ids, n, P_E0) && !pool_has(ids, n, P_E2) &&
                       !pool_has(ids, n, P_E6) && !pool_has(ids, n, P_E7);
            break;
        case ENGI_ANIMAL:
            special &= !pool_has(ids, n, P_E0) && !pool_has(ids, n, P_E3) &&
                       !pool_has(ids, n, P_E5) && !pool_has(ids, n, P_E6);
            break;
        case ENGI_LOFI:
            t->p[P_E1] = 5; /* BYTE: gritty texture, not time-loop */
            n = random_sound_pool(t, ids, sizeof ids);
            special &= pool_has(ids, n, P_E5) && !pool_has(ids, n, P_E7);
            t->p[P_E1] = 6; /* FLOAT: E5 is envelope decay */
            n = random_sound_pool(t, ids, sizeof ids);
            special &= !pool_has(ids, n, P_E5) && !pool_has(ids, n, P_E7);
            t->p[P_E1] = 0; t->p[P_E0] = 4; /* STEP: CRUSH now decay */
            n = random_sound_pool(t, ids, sizeof ids);
            special &= !pool_has(ids, n, P_E3);
            break;
        default: break;
        }
    }
    return check("All 8 melodic and 2 drum engines: unique in-range eligibility, drums excluded", valid) +
           check("Engine-specific pitch, selector, envelope and time exclusions", special);
}

int main(void)
{
    int bad = 0, valid = 1, changed = 0;
    uint32_t n, i, seed;
    uint8_t pool[12];
    int16_t t2_before[P_COUNT], t3_before[P_COUNT];
    track_t *t;
    static project_store_t saved_snapshot;

    ui_power_on();
    bad += random_whitelist_test();
    ui_power_on();
    random_test_reset_state();
    t = &trk[0];

    /* No saved section means there is no implied baseline to overwrite. */
    seed = random_seed;
    random_cc_test(13, 85, 127);
    bad += check("Unsaved sounds cannot invent an implicit saved baseline",
                 !random_baseline_valid[0] && random_seed == seed);
    random_cc_test(13, 85, 0);

    n = random_sound_pool(t, pool, sizeof pool);
    bad += check("ANALOG has at least two eligible continuous parameters", n >= 2u);
    for (i = 0; i < n; i++) t->p[pool[i]] = 64;
    trk[1].p[P_DIST] = trk[2].p[P_DIST] = 64;
    t->p[P_LEVEL] = 70;
    t->p[P_PAN] = 40;
    t->p[P_ATK] = 20;
    bad += check("Explicit project save succeeds", project_save_as(0u, 0) == 0);
    bad += check("Saved sound baseline tracks the persisted project slot",
                 random_saved_slot[0] == 0u && random_saved_slot[1] == 0u &&
                 random_saved_slot[2] == 0u && random_baseline[0][P_ATK] == 20);

    memcpy(&saved_snapshot, &proj_slot[0], sizeof saved_snapshot);
    memcpy(t2_before, trk[1].p, sizeof t2_before);
    memcpy(t3_before, trk[2].p, sizeof t3_before);
    random_cc_test(13u, 85u, 127u);
    for (i = 0; i < n; i++) {
        int32_t d = (int32_t)t->p[pool[i]] - 64;
        if (!d) continue;
        changed++;
        if ((d < 0 ? -d : d) < 15 || (d < 0 ? -d : d) > 30) valid = 0;
    }
    bad += check("CC85 changes exactly two distinct eligible sound parameters by 15..30",
                 changed == 2 && valid);
    bad += check("MIDI channel 13 always targets only physical track 1",
                 !memcmp(trk[1].p, t2_before, sizeof t2_before) &&
                 !memcmp(trk[2].p, t3_before, sizeof t3_before));

    seed = random_seed;
    random_cc_test(13u, 85u, 127u);
    bad += check("Duplicate high CC within 120 ms is suppressed", random_seed == seed);
    fm1_ms += 121u;
    random_cc_test(13u, 85u, 127u);
    bad += check("High-only pad message after debounce retriggers", random_seed != seed);
    seed = random_seed;
    random_cc_test(13u, 85u, 0u);
    random_cc_test(13u, 85u, 127u);
    bad += check("Release value re-arms immediately", random_seed != seed);

    /* Reset is deliberately asynchronous: the MIDI handler must not load
     * flash or manipulate the UI; the UI poll loads the saved track sound. */
    t->p[P_FCUT] = 17;
    t->p[P_ATK] = 4;
    t->p[P_LEVEL] = 97;
    t->p[P_PAN] = 95;
    global_eq_gain[0] = 7;
    random_cc_test(13u, 87u, 127u);
    bad += check("CC87 queues rather than synchronously loading a project",
                 (random_reset_pending & 1u) && t->p[P_ATK] == 4);
    frame();
    bad += check("CC87 discards manual edits and restores the actual saved sound",
                 t->p[P_FCUT] == random_baseline[0][P_FCUT] &&
                 t->p[P_ATK] == 20 && !(random_reset_pending & 1u));
    bad += check("CC87 preserves mix and global EQ",
                 t->p[P_LEVEL] == 97 && t->p[P_PAN] == 95 && global_eq_gain[0] == 7);
    bad += check("MIDI randomization never writes the stored project",
                 !memcmp(&saved_snapshot, &proj_slot[0], sizeof saved_snapshot));

    /* Hard randomization draws three distinct slots with 30..60 offsets. */
    changed = 0;
    valid = 1;
    random_cc_test(13u, 86u, 127u);
    for (i = 0; i < n; i++) {
        int32_t d = (int32_t)t->p[pool[i]] - 64;
        if (!d) continue;
        changed++;
        if ((d < 0 ? -d : d) < 30 || (d < 0 ? -d : d) > 60) valid = 0;
    }
    bad += check("CC86 changes exactly three distinct eligible parameters by 30..60",
                 changed == 3 && valid);
    random_cc_test(13u, 87u, 0u);
    random_cc_test(13u, 87u, 127u);
    frame();

    /* Even an unsaved engine change cannot replace the saved original. */
    t->eng_req = ENGI_LOFI;
    t->p[P_FCUT] = 8;
    random_cc_test(13u, 87u, 0u);
    random_cc_test(13u, 87u, 127u);
    frame();
    bad += check("CC87 restores the saved engine after an unsaved engine switch",
                 t->eng_req == ENGI_ANALOG && t->p[P_FCUT] == random_baseline[0][P_FCUT]);

    /* The latest successful save, rather than the initial one, wins. */
    t->p[P_FCUT] = 90;
    t->p[P_ATK] = 12;
    bad += check("Saving an edited sound to another slot succeeds", project_save_as(1u, 0) == 0);
    t->p[P_FCUT] = 32;
    t->p[P_ATK] = 2;
    random_cc_test(13u, 87u, 0u);
    random_cc_test(13u, 87u, 127u);
    frame();
    bad += check("CC87 uses the most recently saved version", random_saved_slot[0] == 1u &&
                 t->p[P_FCUT] == 90 && t->p[P_ATK] == 12);

    /* A stored obsolete macro must not intercept the reserved messages. */
    cm_count = 1u;
    memset(cm_rows, 0, sizeof cm_rows);
    cm_rows[0][0] = 85u;
    cm_rows[0][1] = 0xc0u;
    cm_rows[0][2] = t->eng_req;
    cm_rows[0][3] = P_LEVEL;
    cm_rows[0][4] = CM_ON;
    cm_rows[0][5] = 0u;
    cm_rows[0][6] = 127u;
    t->p[P_LEVEL] = 65;
    random_cc_test(13u, 85u, 0u);
    random_cc_test(13u, 85u, 127u);
    bad += check("Reserved CC85 cannot execute previously stored REC macros",
                 t->p[P_LEVEL] == 65 && !ml_free_cc(85u) && !ml_free_cc(86u) && !ml_free_cc(87u));

    /* Ch14 is T2, ch15 is T3 regardless of their normal note RX mapping. */
    memcpy(t3_before, trk[2].p, sizeof t3_before);
    seed = random_seed;
    random_cc_test(14u, 85u, 127u);
    bad += check("MIDI channel 14 addresses physical track 2", random_seed != seed &&
                 !memcmp(trk[2].p, t3_before, sizeof t3_before));
    seed = random_seed;
    random_cc_test(15u, 86u, 127u);
    bad += check("MIDI channel 15 addresses physical track 3", random_seed != seed);

    /* Removing the saved section must never reset to stale unsaved data. */
    bad += check("Clearing the saved slot succeeds", project_clear_slot(1u) == 0);
    bad += check("Clearing the source invalidates the reset baseline",
                 random_saved_slot[0] == 0xffu && !random_baseline_valid[0]);
    t->p[P_ATK] = 3;
    random_cc_test(13u, 87u, 0u);
    random_cc_test(13u, 87u, 127u);
    frame();
    bad += check("CC87 cannot resurrect a cleared saved section", t->p[P_ATK] == 3);

    /* A Live Recall changes the source back to the actual recalled section. */
    bad += check("Live Recall of the first saved section succeeds",
                 project_live_sounds(0u) == 0);
    bad += check("Live Recall updates the baseline and saved slot for all three tracks",
                 random_saved_slot[0] == 0u && random_saved_slot[1] == 0u &&
                 random_saved_slot[2] == 0u && t->p[P_ATK] == 20);
    t->p[P_ATK] = 6;
    random_cc_test(13u, 87u, 0u);
    random_cc_test(13u, 87u, 127u);
    frame();
    bad += check("CC87 after Live Recall restores its section's saved sound",
                 t->p[P_ATK] == 20);

    /* The saved melodic sound wins even if the live engine becomes a drum. */
    t->eng_req = ENGI_DRUM;
    t->p[P_ATK] = 2;
    random_cc_test(13u, 87u, 0u);
    random_cc_test(13u, 87u, 127u);
    frame();
    bad += check("CC87 restores a saved melodic sound from an unsaved drum engine",
                 t->eng_req == ENGI_ANALOG && t->p[P_ATK] == 20);

    /* Conversely, a saved drum sound must not respond to either command. */
    trk[1].eng_req = ENGI_DRUM;
    bad += check("Saving a section containing DRUM-X succeeds", project_save_as(2u, 0) == 0);
    seed = random_seed;
    random_cc_test(14u, 85u, 0u);
    random_cc_test(14u, 85u, 127u);
    random_cc_test(14u, 87u, 127u);
    frame();
    bad += check("A saved DRUM-X track is excluded from randomization and reset",
                 random_seed == seed && trk[1].eng_req == ENGI_DRUM &&
                 !(random_reset_pending & 2u));

    /* USB and TRS feed the same reserved-channel handler independently of
     * note RX assignments, even if three melodic tracks layer on channel 1. */
    trk[0].eng_req = ENGI_ANALOG; /* saved slot 2's T1 remains melodic */
    midi_rx[0] = midi_rx[1] = midi_rx[2] = 1u;
    queued_random_cc(13u, 85u, 0u, 1u);
    seed = random_seed;
    queued_random_cc(13u, 85u, 127u, 1u);
    bad += check("USB MIDI queue dispatches reserved CC85 on channel 13",
                 random_seed != seed);
    queued_random_cc(13u, 85u, 0u, 2u);
    seed = random_seed;
    queued_random_cc(13u, 85u, 127u, 2u);
    bad += check("TRS MIDI queue dispatches reserved CC85 on channel 13",
                 random_seed != seed);

    printf(bad ? "randomization MIDI: %d FAILED\n" : "randomization MIDI: all passed\n", bad);
    return bad ? 1 : 0;
}
