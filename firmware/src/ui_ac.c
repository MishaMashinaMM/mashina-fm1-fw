/* DRUM-AC independent kick/snare/crash patch editor.
 * SOUND 1: TUNE/DAMP/BODY; SOUND 2: HIT/WIRES/BRIGHT;
 * SOUND 3: METAL/RING; SOUND 4 unused.
 * K1 selects instrument; K2..K4 change its own 0..127 controls.
 */
static const char *const AC_FIELDS[8] = {
    "TUNE", "DAMP", "BODY", "HIT", "WIRES", "BRIGHT", "METAL", "RING"
};
static const char *const AC_PAD_NAMES[AC_NPAD] = {
    "KICK 36", "SNARE 38", "CRASH 49", "C-HAT 42", "O-HAT 46",
    "FLOOR 41", "LOW TOM 43", "MID TOM 45", "HIGH TOM 47",
    "CLAP 39", "R-BELL 53", "R-SPL 51"
};
static const char *ac_field_name(uint32_t pad, uint32_t field)
{
    if (field == 4u)
        return pad == AC_KICK ? "BEATER" :
               pad == AC_CRASH ? "SPLASH" : AC_FIELDS[field];
    if (field == 6u && (pad == AC_KICK || pad == AC_SNARE))
        return "TEXTURE";
    return AC_FIELDS[field];
}
static uint8_t ac_ui_pad;
static uint32_t ac_page_index(const page_t *pg)
{
    uint32_t p = dx_pagen(pg);
    return p >= 1u && p <= 3u ? p - 1u : 0u;
}
static void ac_knob(const page_t *pg, uint32_t k, int32_t steps)
{
    uint32_t field = ac_page_index(pg) * 3u + k - 1u;
    if (k == 0u) {
        ac_ui_pad = (uint8_t)clamp((int32_t)ac_ui_pad + (steps > 0 ? 1 : -1), 0, AC_NPAD - 1u);
    } else if (field < 8u) {
        uint8_t *patch = ac_patch_of(TSEL, ac_ui_pad);
        if (patch)
            ac_patch_set(TSEL, ac_ui_pad, field,
                         (uint32_t)clamp((int32_t)ac_param_value(TSEL, ac_ui_pad, field) +
                                        accel(EN_K1 + k, steps, 127), 0, 127));
    }
    ui.force = 1;
}
static void ac_cards(const page_t *pg)
{
    uint32_t k, first = ac_page_index(pg) * 3u;
    uint8_t *patch = ac_patch_of(TSEL, ac_ui_pad);
    char val[12];
    draw_column(0, "SOUND", AC_PAD_NAMES[ac_ui_pad], "", VAL(0u),
                (int32_t)ac_ui_pad * 1000 / (AC_NPAD - 1u), ICON_X_PATTERN);
    for (k = 0; k < 3u; k++) {
        uint32_t field = first + k;
        if (patch && field < 8u) {
            uint32_t value = ac_param_value(TSEL, ac_ui_pad, field);
            fmt_int(val, (int32_t)value);
            draw_column(k + 1u, ac_field_name(ac_ui_pad, field), val, "", VAL(k + 1u),
                        (int32_t)value * 1000 / 127, ICON_LEVEL);
        } else
            draw_column(k + 1u, "", "", "", T_DIM, -1, ICON_NONE);
    }
}
