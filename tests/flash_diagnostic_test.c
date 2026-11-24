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
    /* 40 candidate sectors (32 MPR1 + 8 LIB32) are fingerprinted
     * in bounded 256-byte reads, without any write or raw MIDI data. */
    {
        uint32_t crc=0u,locations[4]={0xA0000u,0xBF000u,0xC0000u,0xC7000u};
        uint32_t indexes[4]={0u,31u,32u,39u};
        int exact=1;
        for(uint32_t i=0;i<4u;i++){
            uint32_t expected=st_crc32(nor+locations[i],4096u);
            if(ed_diag_sector_crc(indexes[i],&crc)!=0u || crc!=expected)
                exact=0;
        }
        bad+=ck("exact CRC32 for first/last MPR1 and LIBRARY sectors",exact);
        bad+=ck("out-of-range fingerprints always refused",
                ed_diag_sector_crc(40u,&crc)==4u &&
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
    bad+=ck("all flash probes remain entirely read-only",writes==0);
    printf("FLASH DIAG: %s\n",bad?"FAIL":"ALL TESTS PASSED");
    return !!bad;
}
