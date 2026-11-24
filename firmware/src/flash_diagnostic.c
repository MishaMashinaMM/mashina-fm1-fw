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
        if(st_sector(obj,copy)==base && !st_head(obj,copy,&old) &&
           !st_body(obj,copy,&old))return 2u;
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
 * A fingerprint can compare two device states but NEVER establishes flash
 * ownership or recovery-tool preservation. No raw sound data transmitted.
 * Return 0 on success and 4 on unreadable or disallowed sector.
 */
static uint32_t ed_diag_sector_crc(uint32_t index,uint32_t *checksum)
{
    uint8_t page[256];
    uint32_t base,crc=0xFFFFFFFFu;
    if(!checksum || index>=40u)return 4u;
    base=index<32u?0xA0000u+index*4096u:
                    0xC0000u+(index-32u)*4096u;
    for(uint32_t off=0;off<4096u;off+=sizeof page){
        if(st_read(base+off,page,sizeof page))return 4u;
        crc=st_crc32_add(crc,page,sizeof page);
        fm1_wdt_feed();
    }
    *checksum=~crc;
    return 0u;
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

