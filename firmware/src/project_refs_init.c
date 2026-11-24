/* SPDX-License-Identifier: GPL-3.0-only
 * One-time INITIALIZATION ONLY for clean-break MPR1 storage.
 * Never compiled into a normal firmware image.
 *
 * The candidate MPR1 128 KiB region exactly reuses legacy song-section
 * flash sectors. The first MPR1 write deliberately refuses to erase foreign
 * or old data. This explicit formatter accepts ONLY entirely erased sectors
 * or valid CRC-checked legacy project records at each corresponding address.
 * A partial/unknown sector, or an existing MPR1 bank, is NEVER erased.
 *
 * Format is not atomic across 32 sectors: preflight all, erase each known
 * retired sector, verify blank. An interrupted operation can be restarted
 * only if all remaining sectors are still valid legacy records or blank.
 */
static void fm1_wdt_feed(void); /* firmware HAL; host test supplies a stub */
static int mprs_erased_sector(uint32_t bank,uint32_t copy)
{
    uint8_t page[256];
    uint32_t base=mprs_base(bank,copy);
    for(uint32_t off=0;off<MPRS_SECTOR;off+=sizeof page) {
        if(mprs_read(base+off,page,sizeof page))return 0;
        for(uint32_t i=0;i<sizeof page;i++)
            if(page[i]!=0xFFu)return 0;
    }
    return 1;
}
static int mprs_legacy_sector(uint32_t bank,uint32_t copy)
{
    st_hdr_t h;
    uint32_t obj=OBJ_SONG1+bank;
    if(st_sector(obj,copy)!=mprs_base(bank,copy))return 0;
    return st_copy_valid(obj,copy,&h);
}
static int mprs_init_clean_break(void)
{
#if FELUCCA_PROJECT_REFS_FORMAT && FELUCCA_PROJECT_REFS_WRITE && FELUCCA_FLASH
    uint32_t bank,copy;
    if(!flash_ok || transport_busy())return 2;
    /* Every sector is inspected before the very first destructive write. */
    for(bank=0;bank<MPRS_BANKS;bank++)
        for(copy=0;copy<2u;copy++)
            if(!mprs_erased_sector(bank,copy) &&
               !mprs_legacy_sector(bank,copy))
                return 1; /* unknown, corrupt or new MPR1 data */
    for(bank=0;bank<MPRS_BANKS;bank++)
        for(copy=0;copy<2u;copy++)
            if(!mprs_erased_sector(bank,copy)) {
                if(mprs_erase(mprs_base(bank,copy)) ||
                   !mprs_erased_sector(bank,copy))
                    return 2;
                fm1_wdt_feed();
            }
    for(uint32_t k=0;k<4u;k++)memset(proj_slot[k].raw,0,MPR_MAX_RECORD);
    proj_cur=PROJ_NO_SLOT;
    return 0;
#else
    return 2;
#endif
}

/* COMBINED CLEAN-BREAK LAYOUT — experimental and never callable in standard
 * firmware. Project storage A0000..BFFFF and the reclaimed LIB1 sectors
 * C0000..C7FFF must be checked BEFORE touching even one old sector.
 *
 * Accepted bytes: fully erased sector, or a complete CRC-verified OLD FUN
 * project sector for the exact legacy object/copy that owned that address.
 * Any MPR1/LIB1/new-format, corrupt, foreign or unreadable sector => refuse.
 * A power cut during erasure leaves only old verified/erased sector pairs, so
 * the same preflight can restart without automatically overwriting new data.
 *
 * IMPORTANT: This covers ONLY the 40 retired FUN sectors. USER, FM6/drum
 * companions, settings, song index, autosave and old project 1 are out of
 * scope; the full clean-install procedure must handle those separately.
 * Do not publish a physical installer until those operations, the updater,
 * backup/restore and interruption recovery have been independently verified.
 */
static int clean_lib_erased_sector(uint32_t bank,uint32_t copy)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    uint8_t page[256];
    uint32_t base=lib_base(bank,copy);
    if(bank>=LIB_BANKS || copy>1u)return 0;
    for(uint32_t off=0;off<LIB_SECTOR;off+=sizeof page){
        if(lib_flash_read(base+off,page,sizeof page))return 0;
        for(uint32_t k=0;k<sizeof page;k++)if(page[k]!=0xFFu)return 0;
        fm1_wdt_feed();
    }
    return 1;
#else
    (void)bank;(void)copy;return 0;
#endif
}
static int clean_lib_legacy_sector(uint32_t bank,uint32_t copy)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    st_hdr_t h;
    const uint32_t obj=OBJ_SONG1+MPRS_BANKS+bank;
    if(bank>=LIB_BANKS || copy>1u ||
       st_sector(obj,copy)!=lib_base(bank,copy))return 0;
    return st_copy_valid(obj,copy,&h);
#else
    (void)bank;(void)copy;return 0;
#endif
}
static int clean_layout_preflight(void)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    if(!flash_ok || transport_busy())return 2;
    /* Exact 128+32 KiB application storage, no USR or other partitions. */
    if(MPRS_BASE!=0xA0000u || MPRS_BASE+MPRS_BANKS*2u*MPRS_SECTOR!=LIB_BASE ||
       LIB_BASE+LIB_BANKS*2u*LIB_SECTOR!=0xC8000u)return 2;
    for(uint32_t bank=0;bank<MPRS_BANKS;bank++)
        for(uint32_t copy=0;copy<2u;copy++){
            if(!mprs_erased_sector(bank,copy) &&
               !mprs_legacy_sector(bank,copy))return 1;
            fm1_wdt_feed();
        }
    for(uint32_t bank=0;bank<LIB_BANKS;bank++)
        for(uint32_t copy=0;copy<2u;copy++){
            if(!clean_lib_erased_sector(bank,copy) &&
               !clean_lib_legacy_sector(bank,copy))return 1;
            fm1_wdt_feed();
        }
    return 0;
#else
    return 2;
#endif
}
static int clean_layout_initialize(void)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    uint32_t status=clean_layout_preflight();
    if(status)return (int)status; /* zero erases on any preflight failure */
    /* Project formatter has its own repeated preflight and bounded erases. */
    if(mprs_init_clean_break())return 2;
    for(uint32_t bank=0;bank<LIB_BANKS;bank++)
        for(uint32_t copy=0;copy<2u;copy++){
            if(!clean_lib_erased_sector(bank,copy)){
                if(lib_flash_erase(lib_base(bank,copy)) ||
                   !clean_lib_erased_sector(bank,copy))return 2;
                fm1_wdt_feed();
            }
        }
    return 0;
#else
    return 2;
#endif
}
