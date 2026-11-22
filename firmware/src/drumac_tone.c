/* SPDX-License-Identifier: GPL-3.0-only
 * Permanent DRUM-AC tone contour: two first-order low-pass states.
 *
 * Based on the three-band GLO EQ at LOW +10, MID -7, HIGH +3,
 * with 20% less mid-band contribution than the first lightweight preview.
 * No UI, no preset/project parameters.
 *
 * Two one-pole filters partition the mono signal:
 *   low = LP(171 Hz), mid = LP(1663 Hz) - low, high = x - LP(1663 Hz)
 *   y = 1.840*low + 0.217*mid + 1.293*high
 * Rewritten as three Q12 gain terms to avoid computing three buffers.
 * LP histories are kept in Q12 to avoid truncated quiet bass tails.
 *
 * Runs ONCE per DRUM-AC track after voice summing, before track distortion
 * and FX sends. Not a replacement for the user's GLO EQ.
 */
typedef struct {
    int64_t low_q12, upper_q12;
    uint8_t ready;
    uint8_t tail_blocks;
} ac_tone_t;

static ac_tone_t ac_tone[NPART] __attribute__((section(".pool")));

#define AC_TONE_LP_A    1576  /* 171 Hz, Q16, FS=44117.647 */
#define AC_TONE_LP_B   13818  /* 1663 Hz, Q16 */
#define AC_TONE_HIGH_Q12  5297
#define AC_TONE_MID_MINUS_HIGH_Q12 (-4409)
#define AC_TONE_LOW_MINUS_MID_Q12  6647
#define AC_TONE_TAIL_BLOCKS 64u  /* ~46 ms, enough to settle 171-Hz state */

static void ac_tone_forget(uint32_t ti)
{
    if (ti >= NPART) return;
    ac_tone[ti].low_q12 = 0;
    ac_tone[ti].upper_q12 = 0;
    ac_tone[ti].ready = 0u;
    ac_tone[ti].tail_blocks = 0u;
}

static void ac_tone_process(track_t *t, int32_t *mono, uint32_t n)
{
    uint32_t ti = (uint32_t)(t - trk), i;
    ac_tone_t *s;
    if (ti >= NPART) return;
    if (t->engine != ENGI_DRUMAC) {
        ac_tone_forget(ti);
        return;
    }
    s = &ac_tone[ti];
    s->ready = 1u;
    for (i = 0; i < n; i++) {
        int64_t xq = (int64_t)mono[i] * 4096;
        int64_t dlow = ((xq - s->low_q12) * AC_TONE_LP_A) >> 16;
        int64_t dupper = ((xq - s->upper_q12) * AC_TONE_LP_B) >> 16;
        int64_t y;
        s->low_q12 += dlow;
        s->upper_q12 += dupper;
        y = (int64_t)AC_TONE_HIGH_Q12 * mono[i] +
            ((int64_t)AC_TONE_MID_MINUS_HIGH_Q12 * s->upper_q12 >> 12) +
            ((int64_t)AC_TONE_LOW_MINUS_MID_Q12 * s->low_q12 >> 12);
        y >>= 12;
        mono[i] = y > INT32_MAX ? INT32_MAX : y < INT32_MIN ? INT32_MIN : (int32_t)y;
    }
}
