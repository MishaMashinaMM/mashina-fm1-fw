/* SPDX-License-Identifier: GPL-3.0-only
 * 32-slot LIBRARY read/transfer adapter. EXPERIMENTAL and compile-time gated.
 *
 * The library sector ownership is not verified on physical devices. Therefore
 * FELUCCA_LIB32 defaults OFF and its hardware WRITE path defaults OFF
 * independently. This module never borrows an existing USER slot.
 *
 * Included AFTER upreset.c so it can use the same validated sound decoding,
 * FM6 patch handling and independent DRUM kit loader.
 */
#if FELUCCA_FLASH_DIAG
/* Implemented later by the read-only diagnostic, with complete FUN CRC
 * validation. Previewing a legacy bank never implies permission to erase. */
static uint32_t ed_diag_project_bank(uint32_t bank);
static uint8_t library_diag_preview_legacy;
#endif
#include "preset_library_store.c"
#include "preset_library_wire.c"

_Static_assert(sizeof(up_rec_t) + UPF_PK == LIB_ENTRY,
               "library entry must contain one full USER sound and companion");
static uint8_t library_record[LIB_ENTRY] __attribute__((section(".pool")));
/* Selected-track display names; no flash reads during audio/display redraw. */
static char library_track_names[NTRK][13] __attribute__((section(".pool")));
static void library_track_name(const track_t *t, char *out)
{
    uint32_t i = (uint32_t)(t - trk);
    str_cpy(out, i < NTRK ? library_track_names[i] : "", 13);
}
/* Both uploaded records and compiled-factory LIBRARY fallbacks must update
 * this display cache; flash reads are forbidden during mixer redraw. */
static void library_track_name_set(const track_t *t, const char *name)
{
    uint32_t i = (uint32_t)(t - trk);
    if (i < NTRK) str_cpy(library_track_names[i], name ? name : "", 13);
}

static int lib_flash_read(uint32_t addr, void *p, uint32_t n)
{
#if defined(MASHINA_LIB_HOST_FLASH)
    return lib_host_read(addr,p,n);
#elif FELUCCA_FLASH
    if (!flash_ok || addr < LIB_BASE || addr+n > LIB_BASE+LIB_BANKS*2u*LIB_SECTOR)
        return -1;
    return st_read(addr, p, n);
#else
    (void)addr; (void)p; (void)n; return -1;
#endif
}
static int lib_range_write(uint32_t addr, uint32_t n)
{
    return addr >= LIB_BASE && n && addr+n >= addr &&
           addr+n <= LIB_BASE+LIB_BANKS*2u*LIB_SECTOR;
}
static int lib_flash_erase(uint32_t addr)
{
#if defined(MASHINA_LIB_HOST_FLASH)
    return lib_host_erase(addr);
#elif FELUCCA_FLASH && FELUCCA_LIB32_WRITE
    uint32_t took, f; int rc;
    if (!flash_ok || !lib_range_write(addr, LIB_SECTOR) ||
        (addr & (LIB_SECTOR - 1u)) || transport_busy()) return -1;
    f = irq_save();
    audio_silence();
    rc = FL_FAR(fl_erase4k_ram)(addr, &took);
    irq_restore(f);
    return rc;
#else
    (void)addr; return -1;
#endif
}
static int lib_flash_prog(uint32_t addr, const void *src, uint32_t n)
{
#if defined(MASHINA_LIB_HOST_FLASH)
    return lib_host_prog(addr,src,n);
#elif FELUCCA_FLASH && FELUCCA_LIB32_WRITE
    uint32_t took, f; int rc;
    if (!flash_ok || !lib_range_write(addr, n) ||
        n > 256u || (addr & 255u)+n > 256u || transport_busy()) return -1;
    f = irq_save();
    audio_silence();
    rc = FL_FAR(fl_prog_ram)(addr, src, n, &took);
    irq_restore(f);
    return rc;
#else
    (void)addr; (void)src; (void)n; return -1;
#endif
}

/* Check a reference without playing/changing a track: success for a
 * complete uploaded sound or an all-zero factory fallback. Unlike playback,
 * the result does not change song.sel or any project/preset data. */
static int library_slot_ready(uint32_t slot)
{
    up_rec_t *record = (up_rec_t *)library_record;
    if (slot >= LIB_SLOTS || lib_slot_read(slot, library_record)) return 1;
    if (lib_record_valid(library_record) && up_valid(record)) return 0;
    for (uint32_t i = 0; i < LIB_ENTRY; i++)
        if (library_record[i]) return 1;
    return 0;
}

/* Return 0 if loaded, 1 only for an EMPTY slot (factory fallback),
 * or 2 for flash failure / corrupted record. Never substitute a different
 * factory sound for a damaged nonempty preset: a project could then play
 * the wrong sound without warning. Only the chosen track changes. */
static int library_sound_load(uint32_t slot)
{
    up_rec_t *r = (up_rec_t *)library_record;
    track_t *t = TSEL;
    int16_t v[P_COUNT];
    uint32_t i;
    int rc;
    if (slot >= LIB_SLOTS) return 2;
    rc = lib_slot_read(slot, library_record);
    if (rc) {
#if FELUCCA_FLASH_DIAG
        /* The old firmware's FUN project data remains in the proposed LIB1
         * sectors. In this read-only diagnostic ONLY, a fully CRC-validated
         * retired FUN bank may audition the compiled factory fallback.
         * Unknown, corrupt and unreadable flash still fail closed.
         * No physical flash write, erasure or initialization occurs. */
        library_diag_preview_legacy = 0u;
        if (ed_diag_project_bank(16u + slot / LIB_PER_BANK) == 2u) {
            library_diag_preview_legacy = 1u;
            return 1;
        }
#endif
        return 2;
    }
#if FELUCCA_FLASH_DIAG
    library_diag_preview_legacy = 0u;
#endif
    if (!lib_record_valid(library_record) || !up_valid(r)) {
        /* A factory fallback is valid ONLY if the complete record is zero. */
        for (i = 0; i < LIB_ENTRY; i++)
            if (library_record[i]) return 2;
        return 1;
    }
    up_values(r, v);
    load_begin(t, LOAD_SOUND);
    panic_req |= (uint8_t)(1u << song.sel);
#if !FELUCCA_FM4
    if (r->engine == ENGI_DIGITAL) {
        int16_t p[P_COUNT];
        for (i = 0; i < P_COUNT; i++)
            p[i] = param_kept(i) ? t->p[i] : v[i];
        fm4_apply(t, p);
    } else
#endif
    {
        fm1_irq_off();
        t->eng_req = r->engine;
        for (i = 0; i < P_COUNT; i++)
            if (!param_kept(i)) t->p[i] = v[i];
        t->preset = 0;
        fm1_irq_on();
        if (r->engine == ENGI_FM6) {
            uint8_t pk[FM6_PACKED], values[FP_SIZE + 1u];
            upf_pk_get(library_record + sizeof(up_rec_t), pk);
            fm6_unpack(pk, values);
            fm6_set_patch((uint32_t)(t - trk), values);
            fm6_adopt((uint32_t)(t - trk));
        }
        if (upd_engine(r->engine))
            upd_track_load_data(t, r, library_record + sizeof(up_rec_t),
                                library_record[LIB_ENTRY - 1u] == UPD_FMT);
        if (eng_gone(r->engine)) set_engine_of(t, ENGI_PHYS_TO);
    }
    /* A library source marker is NOT a USER slot: user_of() returns none.
     * A project save stores the full sound snapshot, not this reference. */
    {
        char name[13];
        memcpy(name, r->name, 12u);
        name[12] = 0;
        library_track_name_set(t, name);
    }
    t->user = (uint8_t)(UP_SLOTS + slot + 1u);
    load_end(t);
#if defined(FELUCCA_PROJECT_REFS) && FELUCCA_PROJECT_REFS
    mpr_capture_source(t);
#endif
    sync_reload = 1;
    ui.force = 1;
    return 0;
}

