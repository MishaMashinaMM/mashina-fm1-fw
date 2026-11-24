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
 * The operation covers all active project, LIBRARY, USER, companion and
 * auxiliary settings, index, autosave and voice-bank partitions. Old first
 * project and unreferenced legacy sectors are deliberately left untouched.
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
/* USER/companion are intentionally distinct partitions from retired FUN:
 * USER 001..032 are two 16-record st objects (4 x 4KiB sectors), and the
 * FM6/drum companion uses the separately owned 0x9F000 / 0xFE000 sectors.
 * Former FM6-bank data can occupy the same companion pair. Do not touch
 * unknown USR partitions. The current clean-install prototype separately
 * handles voice-bank and settings sectors after complete preflight.
 * This code is unreachable unless CLEAN_INSTALL_LAYOUT is compile-enabled.
 */
static uint32_t clean_user_object(uint32_t index)
{
    return index < 4u ? OBJ_UPRESET0 + index/2u : OBJ_UPFM6;
}
static uint32_t clean_user_copy(uint32_t index)
{
    return index < 4u ? index%2u : index-4u;
}
/* Exact whitelist, not just a broad USER address range. A future storage
 * map change must REFUSE initialization rather than silently format the new
 * location merely because it holds a CRC-valid unrelated storage record. */
static int clean_user_address_ok(uint32_t index)
{
    static const uint32_t expected[6]={
        0xDC000u,0xDD000u,0xDE000u,0xDF000u,0x9F000u,0xFE000u
    };
    if(index>=6u)return 0;
    const uint32_t copy=clean_user_copy(index);
    if(st_sector(clean_user_object(index),copy)!=expected[index])return 0;
    if(index>=4u && st_sector(OBJ_FM6BANK,copy)!=expected[index])return 0;
    return 1;
}
static int clean_user_sector_erased(uint32_t index)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    uint8_t page[256];
    uint32_t obj=clean_user_object(index),copy=clean_user_copy(index);
    uint32_t base;
    if(!clean_user_address_ok(index))return 0;
    base=st_sector(obj,copy);
    for(uint32_t off=0;off<ST_SECTOR;off+=sizeof page){
        if(st_read(base+off,page,sizeof page))return 0;
        for(uint32_t k=0;k<sizeof page;k++)if(page[k]!=0xFFu)return 0;
        fm1_wdt_feed();
    }
    return 1;
#else
    (void)index;return 0;
#endif
}
static int clean_user_legacy_sector(uint32_t index)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    st_hdr_t header;
    if(!clean_user_address_ok(index))return 0;
    uint32_t obj=clean_user_object(index),copy=clean_user_copy(index);
    if(st_copy_valid(obj,copy,&header))return 1;
    if(index>=4u && st_copy_valid(OBJ_FM6BANK,copy,&header))return 1;
    return 0;
#else
    (void)index;return 0;
#endif
}
/* Seven remaining active-state sectors require a clean slate too:
 * settings (FC000/FD000), old song selection (D8000/D9000),
 * RESTORE LAST / FM6 staging (E5000/E6000), FM6 voice bank (E7000).
 * The loader/OTA recovery map must be physically verified before enabling
 * these erasures. CRC-valid staged FM6 voices are accepted in autosave A/B;
 * a foreign, corrupted or partly written copy is never auto-formatted.
 * Unreferenced old P1/SONG sectors are intentionally left alone.
 */
static uint32_t clean_aux_address(uint32_t index)
{
    if(index<2u)return st_sector(OBJ_SETTINGS,index);
    if(index<4u)return st_sector(OBJ_SONGIDX,index-2u);
    if(index<6u)return st_sector(OBJ_AUTOSAVE,index-4u);
    return FVB_HOME;
}
static int clean_aux_address_ok(uint32_t index)
{
    static const uint32_t expected[7]={
        0xFC000u,0xFD000u,0xD8000u,0xD9000u,
        0xE5000u,0xE6000u,0xE7000u
    };
    return index<7u && clean_aux_address(index)==expected[index];
}
static int clean_aux_erased_sector(uint32_t index)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    uint8_t page[256];
    uint32_t base;
    if(!clean_aux_address_ok(index))return 0;
    base=clean_aux_address(index);
    for(uint32_t off=0;off<ST_SECTOR;off+=sizeof page){
        if(st_read(base+off,page,sizeof page))return 0;
        for(uint32_t i=0;i<sizeof page;i++)if(page[i]!=0xFFu)return 0;
        fm1_wdt_feed();
    }
    return 1;
#else
    (void)index;return 0;
#endif
}
static int clean_aux_legacy_sector(uint32_t index)
{
#if FELUCCA_CLEAN_INSTALL_LAYOUT
    st_hdr_t h;
    if(!clean_aux_address_ok(index))return 0;
    if(index<2u)return st_copy_valid(OBJ_SETTINGS,index,&h);
    if(index<4u)return st_copy_valid(OBJ_SONGIDX,index-2u,&h);
    if(index<6u){
        const uint32_t copy=index-4u;
        return st_copy_valid(OBJ_AUTOSAVE,copy,&h) ||
               fvb_valid(clean_aux_address(index),1u,&h)==0;
    }
    return fvb_valid(FVB_HOME,0u,&h)==0;
#else
    (void)index;return 0;
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
    /* USER and FM6/drum companion are preflighted in the SAME pass,
     * before a single project or library sector is erased. */
    for(uint32_t index=0;index<6u;index++){
        if(!clean_user_sector_erased(index) &&
           !clean_user_legacy_sector(index))return 1;
        fm1_wdt_feed();
    }
    for(uint32_t index=0;index<7u;index++){
        if(!clean_aux_erased_sector(index) &&
           !clean_aux_legacy_sector(index))return 1;
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
    for(uint32_t index=0;index<6u;index++){
        if(!clean_user_sector_erased(index)){
            if(st_erase(st_sector(clean_user_object(index),
                                  clean_user_copy(index))) ||
               !clean_user_sector_erased(index))return 2;
            fm1_wdt_feed();
        }
    }
    for(uint32_t index=0;index<7u;index++){
        if(!clean_aux_erased_sector(index)){
            if(st_erase(clean_aux_address(index)) ||
               !clean_aux_erased_sector(index))return 2;
            fm1_wdt_feed();
        }
    }
    /* Commit a self-identifying EMPTY MPR1 first project bank LAST.
     * Both records in that bank are all-zero / logically unassigned.
     * A later accidental invocation must reject this NEW-format sector
     * before any erasure, even if the user saved no projects yet.
     * Reuse existing project record scratch: no new 1824-byte buffer.
     * If the final commit is interrupted, reject unknown partial data and
     * require explicit recovery rather than silently erasing it. */
    memset(proj_wire_u.raw,0,MPR_MAX_RECORD);
    if(mprs_save(0u,proj_wire_u.raw))return 2;
    /* The physical caller must reboot to reload empty USER/project slots,
     * default global settings/song selection and no RESTORE LAST / voice
     * bank. Unreferenced retired project sectors remain untouched.
     * No destructive command is exposed in downloadable firmware. */
    return 0;
#else
    return 2;
#endif
}
