/* Host regression tests for the live CC85/86/87 implementation.
 * Compile with cc -std=c11 -Wall -Wextra -Werror -O2 tests/test_midi_randomization.c -o /tmp/test-mashina-random
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FELUCCA_FM4 0
enum {
    ENGI_ANALOG = 0, ENGI_DIGITAL = 1, ENGI_PHASE = 2, ENGI_LOFI = 3,
    ENGI_SAMPLE = 4, ENGI_VOICE = 5, ENGI_TRIO = 6, ENGI_WHEEL = 7,
    ENGI_GRAIN = 8, ENGI_PHYS = 9, ENGI_DRUM = 10, ENGI_NOISE = 11,
    ENGI_FM6 = 12, ENGI_SLICE = 13, ENGI_ANIMAL = 14, ENGI_DRUMLAB = 15,
    ENGI_INDUSTRIAL = 16, ENGI_BRASS = 17, ENGI_ATMOS = 18,
    ENGI_DRUMAC = 19, ENGI_DRUMIND = 20
};
enum {
    P_LEVEL = 0, P_PAN = 1, P_VLANG = 2, P_FCUT = 6, P_FRES = 7,
    P_DIST = 8, P_MORPH = 9,
    P_E0 = 16, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7,
    P_LN8 = 40, P_LN9, P_LN10, P_LN11,
    P_COUNT = 64
};
typedef struct { int32_t min, max; } param_desc_t;
typedef struct { uint8_t eng_req; int16_t p[P_COUNT]; } track_t;
static track_t trk[6];
static uint32_t fm1_ms;
static uint32_t eng_idx(uint32_t e) { return e; }
static int eng_gone(uint32_t e)
{
    return e == ENGI_SAMPLE || e == ENGI_TRIO || e == ENGI_GRAIN ||
           e == ENGI_PHYS || e == ENGI_SLICE;
}
static const param_desc_t *track_desc(const track_t *t, uint32_t id)
{
    static param_desc_t d;
    d.min = 0;
    d.max = 127;
    if (t->eng_req == ENGI_FM6) {
        if (id == P_E1) { d.min = -7; d.max = 7; }
        if (id == P_E2 || id == P_E4) { d.min = -64; d.max = 63; }
        if (id == P_E0) { d.min = 0; d.max = 32; }
    }
    if (id == P_E0 && (t->eng_req == ENGI_ATMOS ||
                        t->eng_req == ENGI_INDUSTRIAL))
        d.max = 5;
    if (id == P_LN8 && t->eng_req == ENGI_INDUSTRIAL)
        d.max = 5;  /* WAVE2: protected selector */
    if (id == P_LN11 && t->eng_req == ENGI_INDUSTRIAL)
        d.max = 2;  /* MODE: protected selector */
    return &d;
}
static int32_t clamp(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}
static int motion_capture(track_t *t, uint32_t id, int16_t value)
{
    (void)t; (void)id; (void)value;
    return 0;
}
#define RING_PUBLISH() ((void)0)
#include "../firmware/src/midi_randomization.c"

static int contains(const uint8_t *ids, uint32_t n, uint8_t id)
{
    uint32_t i;
    for (i = 0; i < n; i++) if (ids[i] == id) return 1;
    return 0;
}
static void init(uint32_t eng)
{
    uint32_t i;
    memset(trk, 0, sizeof trk);
    for (i = 0; i < P_COUNT; i++) trk[0].p[i] = 60;
    trk[0].eng_req = (uint8_t)eng;
    /* Across tests, use the public sound-change hook, not test-only resets. */
    random_sound_changed(0);
    memset(random_pad_held, 0, sizeof random_pad_held);
    fm1_ms += 250;
}
static void engines(void)
{
    static const uint8_t melodic[] = {
        ENGI_ANALOG, ENGI_PHASE, ENGI_LOFI, ENGI_VOICE, ENGI_WHEEL,
        ENGI_NOISE, ENGI_FM6, ENGI_ANIMAL, ENGI_INDUSTRIAL, ENGI_BRASS, ENGI_ATMOS
    };
    static const uint8_t drums[] = {ENGI_DRUM, ENGI_DRUMLAB, ENGI_DRUMAC, ENGI_DRUMIND};
    uint8_t ids[16];
    uint32_t i, n;
    for (i = 0; i < sizeof melodic; i++) {
        init(melodic[i]);
        assert(random_melodic_engine(trk[0].eng_req));
        n = random_sound_pool(&trk[0], ids, sizeof ids);
        assert(n >= 2); /* Every melodic engine has more than a generic effect. */
        assert(!contains(ids, n, P_E0)); /* No waveform/algorithm switches. */
        assert(!contains(ids, n, P_E7) || trk[0].eng_req != ENGI_FM6);
        if (trk[0].eng_req == ENGI_FM6) {
            assert(contains(ids, n, P_E1) && contains(ids, n, P_E2));
        }
        if (trk[0].eng_req == ENGI_ATMOS || trk[0].eng_req == ENGI_BRASS)
            assert(contains(ids, n, P_LN8) && contains(ids, n, P_LN11));
        if (trk[0].eng_req == ENGI_INDUSTRIAL) {
            assert(contains(ids, n, P_LN9) && contains(ids, n, P_LN10));
            assert(!contains(ids, n, P_LN8) && !contains(ids, n, P_LN11));
        }
    }
    for (i = 0; i < sizeof drums; i++) {
        init(drums[i]);
        assert(!random_melodic_engine(trk[0].eng_req));
        assert(random_sound_pool(&trk[0], ids, sizeof ids) == 0);
        random_command(12, 85, 127);
        assert(!random_variation_active[0]);
        random_command(12, 87, 127);
        assert(!random_reset_pending);
    }
}
static void current_sound_and_reset(void)
{
    int16_t before[P_COUNT];
    int changed = 0;
    uint32_t i;
    init(ENGI_ANALOG);
    /* User edits before the FIRST command are the baseline, even without save. */
    trk[0].p[P_E4] = 91;
    memcpy(before, trk[0].p, sizeof before);
    random_command(12, 85, 127);
    assert(random_variation_active[0]);
    for (i = 0; i < P_COUNT; i++) if (trk[0].p[i] != before[i]) changed++;
    assert(changed > 0);
    random_command(12, 87, 127);
    assert(random_reset_pending & 1u);
    random_reset_poll();
    assert(!random_variation_active[0]);
    assert(memcmp(before, trk[0].p, sizeof before) == 0);

    /* Selecting a different preset of the SAME engine makes a new base. */
    random_sound_changed(0);
    trk[0].p[P_E4] = 22;
    memcpy(before, trk[0].p, sizeof before);
    random_command(12, 85, 0); /* rearm */
    random_command(12, 85, 127);
    random_command(12, 87, 0);
    random_command(12, 87, 127);
    random_reset_poll();
    assert(memcmp(before, trk[0].p, sizeof before) == 0);

    /* Different MIDI channels select distinct target tracks. */
    random_command(0, 85, 127);
    assert(!random_variation_active[1]);
}
static void signed_fm6_and_no_drums(void)
{
    int16_t before[P_COUNT];
    uint32_t i;
    init(ENGI_FM6);
    trk[0].p[P_E1] = 0;
    trk[0].p[P_E2] = 0;
    trk[0].p[P_E4] = 0;
    memcpy(before, trk[0].p, sizeof before);
    random_command(12, 86, 127);
    assert(random_variation_active[0]);
    assert(trk[0].p[P_E1] >= -7 && trk[0].p[P_E1] <= 7);
    assert(trk[0].p[P_E2] >= -64 && trk[0].p[P_E2] <= 63);
    assert(trk[0].p[P_E4] >= -64 && trk[0].p[P_E4] <= 63);
    random_command(12, 87, 127);
    random_reset_poll();
    for (i = 0; i < P_COUNT; i++) assert(trk[0].p[i] == before[i]);
    /* Unsaved switch to a drum never applies CC85/86/87. */
    trk[0].eng_req = ENGI_DRUMAC;
    random_sound_changed(0);
    random_command(12, 85, 0);
    random_command(12, 85, 127);
    assert(!random_variation_active[0]);
}
int main(void)
{
    engines();
    current_sound_and_reset();
    signed_fm6_and_no_drums();
    puts("PASS CC85/86/87 current sound, melodic engines, FM6 and four drum exclusions");
    return 0;
}
