/* DRUM-IND: 12 independently editable aperiodic industrial pads.
 * SOUND 1..3: GRAIN/DECAY/WEIGHT, STRIKE/MOTION/AIR, CRUNCH/TAIL.
 * K1 selects sound, K2..4 control that sound's individual stored values.
 */
static uint8_t ind_ui_pad;
static const char *ind_level_name(uint32_t lane)
{
    return lane < IND_NPAD ? IND_NAMES[lane] : "PAD";
}
static uint32_t ind_page_index(const page_t *pg)
{
    uint32_t p = dx_pagen(pg);
    return p >= 1u && p <= 3u ? p - 1u : 0u;
}
static void ind_knob(const page_t *pg, uint32_t k, int32_t steps)
{
    uint32_t field = ind_page_index(pg) * 3u + k - 1u;
    if (k == 0u)
        ind_ui_pad = (uint8_t)clamp((int32_t)ind_ui_pad + (steps > 0 ? 1 : -1),
                                   0, IND_NPAD - 1u);
    else if (field < 8u)
        ind_patch_set(TSEL, ind_ui_pad, field,
                      (uint32_t)clamp((int32_t)ind_param_value(TSEL, ind_ui_pad, field) +
                                     accel(EN_K1 + k, steps, 127), 0, 127));
    ui.force = 1;
}
static void ind_cards(const page_t *pg)
{
    uint32_t first = ind_page_index(pg) * 3u;
    char val[12];
    draw_column(0, "SOUND", IND_NAMES[ind_ui_pad], "", VAL(0u),
                (int32_t)ind_ui_pad * 1000 / (IND_NPAD - 1u), ICON_X_PATTERN);
    for (uint32_t k = 0u; k < 3u; k++) {
        uint32_t f = first + k;
        if (f < 8u) {
            uint32_t value = ind_param_value(TSEL, ind_ui_pad, f);
            fmt_int(val, (int32_t)value);
            draw_column(k + 1u, IND_FIELDS[f], val, "", VAL(k + 1u),
                        (int32_t)value * 1000 / 127, ICON_LEVEL);
        } else
            draw_column(k + 1u, "", "", "", T_DIM, -1, ICON_NONE);
    }
}
