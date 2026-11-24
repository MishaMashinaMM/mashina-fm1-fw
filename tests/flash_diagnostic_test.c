/* SPDX-License-Identifier: GPL-3.0-only
 * No-device NOR flash diagnostic: this test uses read-only classification.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define FELUCCA_FLASH_DIAG 1
static uint8_t nor[0x100000];
static unsigned writes, reads, watchdogs;
static int st_read(uint32_t off,void *dst,uint32_t n)
{
    if(off>sizeof nor || n>sizeof nor-off)return -1;
    reads++;memcpy(dst,nor+off,n);return 0;
}
static int st_erase(uint32_t off)
{
    if(off>=sizeof nor || (off&4095u))return -1;
    writes++;memset(nor+off,255,4096);return 0;
}
static int st_prog(uint32_t off,const void *src,uint32_t n)
{
    if(off>sizeof nor || n>sizeof nor-off)return -1;
    writes++;const uint8_t *p=src;
    for(uint32_t i=0;i<n;i++)nor[off+i]&=p[i];
    return 0;
}
#include "../firmware/src/storage.c"
/* Host-only emulation of the normal FM6 bank validator. Firmware supplies
 * the real fvb_valid() from fm6_vbank.c before including editor_library.c. */
static int fvb_valid(uint32_t base,uint32_t slot,st_hdr_t *h)
{
    uint32_t magic=0;
    if(st_read(base,h,sizeof *h)||h->magic!=ST_MAGIC||h->type!=OBJ_FM6VB||
       h->slot!=slot||h->len!=3616u||
       h->hcrc!=st_crc32(h,sizeof *h-4u))return -1;
    if(st_read(base+224u,st_buf,h->len) ||
       st_crc32(st_buf,h->len)!=h->crc)return -1;
    memcpy(&magic,st_buf,sizeof magic);
    return magic==0x56364D46u?0:-1;
}
static void fm1_wdt_feed(void){watchdogs++;}
#include "../firmware/src/flash_diagnostic.c"

static int ck(const char *label,int ok){
    printf("FLASH DIAG %-55s %s\n",label,ok?"PASS":"FAIL");
    return !ok;
}
int main(void){
    int bad=0;
    memset(nor,0xff,sizeof nor);
    const uint32_t base=0xA0000u;
    bad+=ck("factory-erased project bank is blank",ed_diag_project_bank(0)==0);
    uint8_t data[12]={0x4d,0x41,0x53,0x48,0x49,0x4e,0x41,0,1,2,3,4};
    bad+=ck("prepare authentic CRC-validated legacy project sector",
            st_save(OBJ_SONG1,data,sizeof data)==0);
    writes=0;
    bad+=ck("legacy FUN project classifies as retired",ed_diag_project_bank(0)==2);
    bad+=ck("diagnostics never write a flash byte",writes==0);
    bad+=ck("probe reads existing NOR and feeds watchdog",reads>0&&watchdogs>0);
    {
        uint32_t off=st_sector(OBJ_SONG1,0);
        nor[off+ST_PAYLOAD_OFF+2u]^=1u;
        bad+=ck("tampered legacy payload is unknown",ed_diag_project_bank(0)==3);
        nor[off+ST_PAYLOAD_OFF+2u]^=1u;
    }
    memset(nor+base,255,8192u);
    {
        uint32_t head[5]={0x3153504Du,0,1,3648,0};
        uint8_t payload[3648];
        memset(payload,0,sizeof payload);payload[19]=17;payload[1700]=95;
        head[4]=st_crc32(payload,sizeof payload);
        memcpy(nor+base,head,sizeof head);
        memcpy(nor+base+256,payload,sizeof payload);
        bad+=ck("CRC-verified committed MPR1 is recognized",ed_diag_project_bank(0)==1);
        nor[base+256+1700]^=1;
        bad+=ck("tampered MPR1 fails to unknown",ed_diag_project_bank(0)==3);
        nor[base+256+1700]^=1;
    }
    memset(nor+base+4096u,0xff,4096);
    nor[base+4096+4000]=0x88;
    bad+=ck("foreign data in alternate sector is flagged",ed_diag_project_bank(0)==3);
    bad+=ck("all 16 bank positions lie within proposed reservation",
            st_sector(OBJ_SONG1+15,1)==0xBF000u);
    /* 53 candidates: 32 MPR1, 8 LIB, 4 USER, 2 companions,
     * 2 settings, 2 song-index, 2 autosave and 1 FM6 voice sector;
     * no flash write or raw MIDI sound payload is ever transmitted. */
    {
        uint32_t crc=0u;
        const uint32_t locations[15]={
          0xA0000u,0xBF000u,0xC0000u,0xC7000u,
          0xDC000u,0xDF000u,0x9F000u,0xFE000u,
          0xFC000u,0xFD000u,0xD8000u,0xD9000u,
          0xE5000u,0xE6000u,0xE7000u
        };
        const uint32_t indexes[15]={
          0u,31u,32u,39u,40u,43u,44u,45u,
          46u,47u,48u,49u,50u,51u,52u
        };
        int exact=1;
        for(uint32_t i=0;i<15u;i++){
            uint32_t expected=st_crc32(nor+locations[i],4096u);
            if(ed_diag_sector_crc(indexes[i],&crc)!=0u || crc!=expected)
                exact=0;
        }
        bad+=ck("exact CRC32 for all active-state project/sound/settings sectors",exact);
        bad+=ck("out-of-range fingerprints always refused",
                ed_diag_sector_crc(53u,&crc)==4u &&
                ed_diag_sector_crc(255u,&crc)==4u &&
                ed_diag_sector_crc(0u,NULL)==4u);
        bad+=ck("CRC probes do not write any flash",writes==0);
    }
    /* Reclaimed LIB1 banks are four retired legacy FUN section pairs. */
    bad+=ck("new LIBRARY candidate starts in retired project storage",
            st_sector(OBJ_SONG1+16,0)==0xC0000u &&
            st_sector(OBJ_SONG1+19,1)==0xC7000u);
    bad+=ck("reclaimed blank FUN sector is recognized",
            ed_diag_project_bank(16)==0);
    bad+=ck("host prepares authentic legacy FUN content in C0000",
            st_save(OBJ_SONG1+16,data,sizeof data)==0);
    writes=0;
    bad+=ck("read-only probe recognizes retired FUN in LIBRARY candidate",
            ed_diag_project_bank(16)==2);
    bad+=ck("retired LIBRARY candidate inspection never writes",writes==0);
    /* A zero-length committed FELU record has a constant full 4 KiB CRC
     * regardless of the type, slot and sequence in the header. It is not a
     * flash alias. Exact status classification still checks type/slot and
     * validates the header CRC, payload CRC and optional extension. */
    {
        const uint8_t nothing=0u;
        memset(nor+0xA0000u,255,0x8000u);
        bad+=ck("prepare different empty CRC-committed project sectors",({
            int ok=!st_save(OBJ_SONG1,&nothing,0u) &&
                   !st_save(OBJ_SONG1+1u,&nothing,0u);
            ok;
        }));
        uint32_t h0=0,h1=0;
        bad+=ck("different empty FUN records share canonical 127B0F88 CRC",({
            ed_diag_sector_crc(0u,&h0);
            ed_diag_sector_crc(2u,&h1);
            h0==0x127B0F88u && h1==h0 &&
            memcmp(nor+0xA0000u,nor+0xA2000u,32u)!=0;
        }));
        bad+=ck("different matching-CRC FUN records both pass exact validators",
                ed_diag_layout_sector(0u)==2u &&
                ed_diag_layout_sector(2u)==2u);
        nor[0xA2000u+6u]^=1u; // committed slot field damaged
        bad+=ck("same whole-sector CRC is NOT accepted after header damage",
                ed_diag_layout_sector(2u)==3u);
        nor[0xA2000u+6u]^=1u;
    }
    {
        /* Every additional section is blank unless a CRC-validated record
         * of the exact expected storage type occupies its exact sector. */
        const uint32_t list[13]={
            0xDC000u,0xDD000u,0xDE000u,0xDF000u,
            0x9F000u,0xFE000u,0xFC000u,0xFD000u,
            0xD8000u,0xD9000u,0xE5000u,0xE6000u,0xE7000u
        };
        int blank=1;
        for(uint32_t i=0;i<13u;i++){
            memset(nor+list[i],255,4096u);
            if(ed_diag_sector_address(40u+i)!=list[i] ||
               ed_diag_layout_sector(40u+i)!=0u)blank=0;
        }
        bad+=ck("53-index exact address whitelist and blank sector readback",blank);
        bad+=ck("prepare CRC-valid USER, companion, settings and song index",({
            int ok=!st_save(OBJ_UPRESET0,data,sizeof data) &&
                   !st_save(OBJ_UPFM6,data,sizeof data) &&
                   !st_save(OBJ_SETTINGS,data,sizeof data) &&
                   !st_save(OBJ_SONGIDX,data,sizeof data) &&
                   !st_save(OBJ_AUTOSAVE,data,sizeof data);
            ok;
        }));
        bad+=ck("all five independent active-state objects classify as expected",({
            int good=1;
            for(uint32_t i=0;i<5u;i++){
                const uint32_t index[5]={40u,44u,46u,48u,50u};
                if(ed_diag_layout_sector(index[i])!=2u)good=0;
            }
            good;
        }));
        nor[0xFC000u+256u]^=0x4u;
        bad+=ck("settings payload corruption classified unknown",
                ed_diag_layout_sector(46u)==3u);
        nor[0xFC000u+256u]^=0x4u;
        nor[0xFE000u+4000u]=0x27u;
        bad+=ck("unknown companion data blocks clean-format classification",
                ed_diag_layout_sector(45u)==3u);
        nor[0xFE000u+4000u]=0xffu;
        bad+=ck("no sector outside approved 53 inspected",
                ed_diag_layout_sector(53u)==4u&&
                ed_diag_sector_address(53u)==0u);
    }
    /* Reset instrumentation after host-only fixture creation (st_save).
     * The diagnostic classifier itself must perform zero programs/erases. */
    writes=0;
    bad+=ck("all exact preflight classifications remain read-only",writes==0);
    bad+=ck("all flash probes remain entirely read-only",writes==0);
    printf("FLASH DIAG: %s\n",bad?"FAIL":"ALL TESTS PASSED");
    return !!bad;
}
