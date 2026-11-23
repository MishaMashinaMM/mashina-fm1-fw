/* SPDX-License-Identifier: GPL-3.0-only */
/* This handler is only reached when the explicit experimental feature is ON.
 * Writes still fail closed unless FELUCCA_LIB32_WRITE=1 was set for the build. */
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

static int ed_library_handle(uint32_t cmd, const uint8_t *args, uint32_t n)
{
    uint8_t response[LIB_WIRE_MAX];
    uint32_t length;
    if (cmd == 95u) { /* MPR1_CAP: 32 project sections, separate write gate */
#if defined(FELUCCA_PROJECT_REFS) && FELUCCA_PROJECT_REFS
        if(n) {ed_b(127u);ed_b(1u);return 1;}
        ed_b(1u);ed_b(32u);
#if FELUCCA_PROJECT_REFS_WRITE
        ed_b(flash_ok?1u:0u);
#else
        ed_b(0u);
#endif
#if FELUCCA_PROJECT_REFS_FORMAT
        ed_b(flash_ok?1u:0u);
#else
        ed_b(0u);
#endif
        return 1;
#else
        ed_b(0u);ed_b(0u);ed_b(0u);return 1;
#endif
    }
    if (cmd == 97u) { /* MPR1_PROBE: per-bank read-only flash status */
        if(n){ed_b(127u);ed_b(1u);return 1;}
        ed_b(16u);
        for(uint32_t bank=0;bank<16u;bank++){
            uint32_t seq=0u;
#if defined(FELUCCA_PROJECT_REFS) && FELUCCA_PROJECT_REFS
            if(!flash_ok)ed_b(4u);
            else if(mprs_latest(bank,&seq)>=0)ed_b(1u);
            else if(mprs_blank(bank))ed_b(0u);
            else {
                uint32_t valid=1u;
                for(uint32_t copy=0;copy<2u;copy++)
                    if(!mprs_erased_sector(bank,copy) &&
                       !mprs_legacy_sector(bank,copy))valid=0u;
                ed_b(valid?2u:3u);
            }
#elif FELUCCA_FLASH_DIAG
            (void)seq;
            ed_b(flash_ok?ed_diag_project_bank(bank):4u);
#else
            (void)seq;
            ed_b(4u);
#endif
            fm1_wdt_feed(); /* 16 banks may require many flash read windows */
        }
        return 1;
    }
    if (cmd == 98u) { /* LIB32_PROBE: 4 banks, read only */
        if(n){ed_b(127u);ed_b(1u);return 1;}
        ed_b(LIB_BANKS);
        for(uint32_t bank=0;bank<LIB_BANKS;bank++){
            uint32_t seq=0u;
            if(!flash_ok)ed_b(4u);
            else if(lib_latest(bank,&seq)>=0)ed_b(1u);
            else ed_b(lib_uncommitted(bank)?0u:3u);
            fm1_wdt_feed(); /* read-only probe must not starve watchdog */
        }
        return 1;
    }
    if (cmd == 96u) { /* MPR1_INIT, NOT enabled in ordinary builds */
#if FELUCCA_PROJECT_REFS && FELUCCA_PROJECT_REFS_FORMAT
        static const uint8_t key[4]={0x4Du,0x50u,0x52u,0x31u}; /* "MPR1" */
        if(n!=4u || memcmp(args,key,4u)) {
            ed_b(1u);
        } else if(ed_flash_stop()) {
            ed_b(2u);
        } else {
            ed_b((uint32_t)mprs_init_clean_break());
        }
#else
        ed_b(2u); /* always reject destructive initialization */
#endif
        return 1;
    }
    if (cmd == 94u) { /* CAPTURE_USER, never changes sound or writes flash */
        uint32_t slot = n ? args[0] : UP_SLOTS;
        uint32_t rc = 0u;
        if (n != 1u || slot >= UP_SLOTS || !up_used(slot)) rc = 1u;
        else {
            const up_rec_t *src = up_rec(slot);
            memset(lib_wire_record, 0, LIB_ENTRY);
            memcpy(lib_wire_record, src, sizeof *src);
            if (src->engine == ENGI_FM6 || upd_engine(src->engine)) {
                if (!upf_valid(&upf) || !(upf.used & (1u << slot)) ||
                    upf.e[slot].tag != upf_tag(src)) rc = 2u;
                else memcpy(lib_wire_record + sizeof *src, upf.e[slot].pk, UPF_PK);
            }
            if (!rc && !lib_record_valid(lib_wire_record)) rc = 3u;
        }
        ed_b(slot);
        ed_b(rc);
        if (!rc) {
            uint32_t len = lib_pack7(lib_wire_record, LIB_ENTRY, response);
            for (uint32_t i = 0; i < len; i++) ed_b(response[i]);
        }
        return 1;
    }
    if (cmd < LIB_WIRE_CAP || cmd > LIB_WIRE_ERASE) return 0;
    if (cmd == LIB_WIRE_PUT || cmd == LIB_WIRE_ERASE) {
        if (ed_flash_stop()) {
            ed_b(n ? args[0] : 127u);
            ed_b(2u);
            return 1;
        }
    }
    length = lib_wire_reply(cmd,args,n,response);
    if (cmd == LIB_WIRE_CAP && length == 3u) {
#if FELUCCA_LIB32_WRITE
        response[2] = flash_ok ? 1u : 0u;
#else
        response[2] = 0u;
#endif
    }
    for(uint32_t i=0;i<length;i++) ed_b(response[i]);
    if (!length) { ed_b(127u); ed_b(1u); }
    return 1;
}
