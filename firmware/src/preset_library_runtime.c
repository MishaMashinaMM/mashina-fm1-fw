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
#include "preset_library_store.c"
#include "preset_library_wire.c"

_Static_assert(sizeof(up_rec_t) + UPF_PK == LIB_ENTRY,
               "library entry must contain one full USER sound and companion");
static uint8_t library_record[LIB_ENTRY] __attribute__((section(".pool")));

static int lib_flash_read(uint32_t addr, void *p, uint32_t n)
{
#if FELUCCA_FLASH
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
#if FELUCCA_FLASH && FELUCCA_LIB32_WRITE
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
#if FELUCCA_FLASH && FELUCCA_LIB32_WRITE
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

/* Return 0 if library slot actually loaded; return 1 for an empty/invalid
 * library slot, so the caller can load its built-in curated factory sound.
 * Only the chosen track changes; never writes into existing USER bank. */
static int library_sound_load(uint32_t slot)
{
    up_rec_t *r = (up_rec_t *)library_record;
    track_t *t = TSEL;
    int16_t v[P_COUNT];
    uint32_t i;
    if (slot >= LIB_SLOTS || lib_slot_read(slot, library_record) ||
        !up_valid(r)) return 1;
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
    t->user = (uint8_t)(UP_SLOTS + slot + 1u);
    load_end(t);
    sync_reload = 1;
    ui.force = 1;
    return 0;
}

/* This handler is only reached when the explicit experimental feature is ON.
 * Writes still fail closed unless FELUCCA_LIB32_WRITE=1 was set for the build. */
static int ed_library_handle(uint32_t cmd, const uint8_t *args, uint32_t n)
{
    uint8_t response[LIB_WIRE_MAX];
    uint32_t length;
    if (cmd < LIB_WIRE_LIST || cmd > LIB_WIRE_ERASE) return 0;
    if (cmd == LIB_WIRE_PUT || cmd == LIB_WIRE_ERASE) {
        if (ed_flash_stop()) {
            ed_b(n ? args[0] : 127u);
            ed_b(2u);
            return 1;
        }
    }
    length = lib_wire_reply(cmd,args,n,response);
    for(uint32_t i=0;i<length;i++) ed_b(response[i]);
    if (!length) { ed_b(127u); ed_b(1u); }
    return 1;
}
