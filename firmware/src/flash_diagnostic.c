/* Diagnostic firmware uses the established FUN project engine while reading
 * candidate MPR1 flash only. Never erase/program/initialize anything here.
 * Codes: erased=0, complete MPR1=1, complete legacy FUN sector=2,
 * unknown/corrupt=3, unreadable=4. */
#if FELUCCA_FLASH_DIAG
static uint32_t ed_diag_project_sector(uint32_t bank,uint32_t copy)
{
    uint32_t base=0xA0000u+(bank*2u+copy)*4096u;
    uint8_t page[256];
    uint32_t all_blank=1u;
    for(uint32_t off=0;off<4096u;off+=sizeof page){
        if(st_read(base+off,page,sizeof page))return 4u;
        for(uint32_t i=0;i<sizeof page;i++)
            if(page[i]!=0xFFu)all_blank=0u;
        fm1_wdt_feed();
    }
    if(all_blank)return 0u;
    /* Legacy project section: both header and full payload CRC must match. */
    {
        st_hdr_t old;
        uint32_t obj=OBJ_SONG1+bank;
        if(st_sector(obj,copy)==base && st_copy_valid(obj,copy,&old))return 2u;
    }
    /* Also recognize a full committed compact project sector, even when
     * the diagnostic firmware itself has MPR1 disabled. */
    {
        uint32_t head[5],crc=0xFFFFFFFFu;
        if(st_read(base,head,sizeof head))return 4u;
        if(head[0]!=0x3153504Du || head[1]!=bank || head[3]!=3648u)
            return 3u;
        for(uint32_t off=0;off<3648u;off+=sizeof page){
            uint32_t n=3648u-off;
            if(n>sizeof page)n=sizeof page;
            if(st_read(base+256u+off,page,n))return 4u;
            crc=st_crc32_add(crc,page,n);
            fm1_wdt_feed();
        }
        return (~crc==head[4])?1u:3u;
    }
}
/* Read-only, bounded CRC32 fingerprint of one 4KiB flash sector.
 * Indices 0..31: all 16 MPR1 A/B pairs at 0xA0000..0xBFFFF.
 * Indices 32..39: all four proposed LIB32 A/B pairs at 0xC0000..0xC7FFF.
 * Indices 40..43: USER 001..032 banks at 0xDC000..0xDFFFF.
 * Indices 44..45: FM6/drum companion copies at 0x9F000 / 0xFE000.
 * Indices 46..52: settings A/B, song index A/B, autosave A/B and
 *                  FM6 voice bank at FC000 FD000 D8000 D9000 E5000 E6000 E7000.
 * Address map intentionally excludes unknown USR and bootloader areas.
 * A fingerprint can compare two device states but NEVER establishes flash
 * ownership or recovery-tool preservation. No raw sound data transmitted.
 * Return 0 on success and 4 on unreadable or disallowed sector.
 */
static uint32_t ed_diag_sector_address(uint32_t index)
{
    if(index>=53u)return 0u;
    return index<32u?0xA0000u+index*4096u:
           index<40u?0xC0000u+(index-32u)*4096u:
           index<44u?0xDC000u+(index-40u)*4096u:
           index==44u?0x9F000u:index==45u?0xFE000u:
           index==46u?0xFC000u:index==47u?0xFD000u:
           index==48u?0xD8000u:index==49u?0xD9000u:
           index==50u?0xE5000u:index==51u?0xE6000u:0xE7000u;
}
static uint32_t ed_diag_sector_crc(uint32_t index,uint32_t *checksum)
{
    uint8_t page[256];
    uint32_t base=ed_diag_sector_address(index),crc=0xFFFFFFFFu;
    if(!checksum || index>=53u)return 4u;
    for(uint32_t off=0;off<4096u;off+=sizeof page){
        if(st_read(base+off,page,sizeof page))return 4u;
        crc=st_crc32_add(crc,page,sizeof page);
        fm1_wdt_feed();
    }
    *checksum=~crc;
    return 0u;
}
/* Exact physical first-install classification for each of 53 whitelisted
 * sectors. The CRC-only fingerprint cannot distinguish CRC-committed empty
 * FELU objects (same whole-sector CRC across different object IDs), damaged
 * copies, or unowned data. This command returns only the validated status;
 * no user bytes, presets or project data are transmitted.
 *
 * 0 erased; 1 valid NEW MPR1; 2 verified OLD source-owned record;
 * 3 unrecognized/malformed; 4 unreadable/disallowed. All 53 sectors must
 * be 0 or 2 for the never-published clean reset preflight to accept them.
 * No status, individually or collectively, grants physical write permission.
 */
static uint32_t ed_diag_layout_sector(uint32_t index)
{
    if(index>=53u)return 4u;
    if(index<40u)return ed_diag_project_sector(index/2u,index%2u);
    const uint32_t base=ed_diag_sector_address(index);
    uint8_t page[256];
    uint32_t blank=1u;
    for(uint32_t off=0;off<4096u;off+=sizeof page){
        if(st_read(base+off,page,sizeof page))return 4u;
        for(uint32_t k=0;k<sizeof page;k++)
            if(page[k]!=0xFFu)blank=0u;
        fm1_wdt_feed();
    }
    if(blank)return 0u;
    uint32_t obj=0u,copy=0u;
    st_hdr_t head;
    if(index<44u){obj=OBJ_UPRESET0+(index-40u)/2u;copy=(index-40u)%2u;}
    else if(index<46u){obj=OBJ_UPFM6;copy=index-44u;}
    else if(index<48u){obj=OBJ_SETTINGS;copy=index-46u;}
    else if(index<50u){obj=OBJ_SONGIDX;copy=index-48u;}
    else if(index<52u){obj=OBJ_AUTOSAVE;copy=index-50u;}
    if(index==52u)
        return fvb_valid(base,0u,&head)==0?2u:3u;
    if(st_sector(obj,copy)!=base)return 3u;
    if(st_copy_valid(obj,copy,&head))return 2u;
    if(index>=44u && index<46u &&
       st_copy_valid(OBJ_FM6BANK,copy,&head))return 2u;
    if(index>=50u && index<52u &&
       fvb_valid(base,1u,&head)==0)return 2u;
    return 3u;
}
static uint32_t ed_diag_project_bank(uint32_t bank)
{
    uint32_t a=ed_diag_project_sector(bank,0);
    uint32_t b=ed_diag_project_sector(bank,1);
    if(a==4u || b==4u)return 4u;
    if(a==3u || b==3u)return 3u;
    if(a==b)return a;
    if(a==0u)return b;
    if(b==0u)return a;
    return 3u; /* mixed legacy and new records need manual inspection */
}
#endif

