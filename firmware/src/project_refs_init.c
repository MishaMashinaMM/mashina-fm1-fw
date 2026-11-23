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
    return st_head(obj,copy,&h)==0 && st_body(obj,copy,&h)==0;
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
