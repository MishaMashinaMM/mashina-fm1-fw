/* SPDX-License-Identifier: GPL-3.0-only
 * Exercise the ACTUAL compile-disabled first-install C routine against mock
 * NOR sectors. Strictly host-only: no USB, device or flash toolchain.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/project_refs.c"
#define FELUCCA_CLEAN_INSTALL_LAYOUT 1
#define FELUCCA_PROJECT_REFS_FORMAT 1
#define FELUCCA_PROJECT_REFS_WRITE 1
#define FELUCCA_FLASH 1

enum { LIB_SECTOR=4096, LIB_BANKS=12,
       ST_SECTOR=4096, PROJ_NO_SLOT=255 };
#define MPRS_BASE 0xA0000u
#define LIB_BASE 0xC0000u
#define FVB_HOME 0xE7000u
#define OBJ_SONG1 100u
#define OBJ_UPRESET0 200u
#define OBJ_UPFM6 202u
#define OBJ_FM6BANK 203u
#define OBJ_SETTINGS 204u
#define OBJ_SONGIDX 205u
#define OBJ_AUTOSAVE 206u

typedef struct {uint32_t marker,object,copy;} st_hdr_t;
static uint8_t nor[0x100000];
static uint32_t erases, programs, wdt;
static int fail_erase_after=-1, tear_final_marker=0;
static int flash_ok=1;
static uint8_t proj_cur;
static union {uint8_t raw[MPR_MAX_RECORD];} proj_wire_u;
static union {uint8_t raw[MPR_MAX_RECORD];} proj_slot[4];

static uint32_t st_sector(uint32_t obj,uint32_t copy) {
    if(copy>1)return 0u;
    if(obj>=OBJ_SONG1 && obj<OBJ_SONG1+28u)
        return MPRS_BASE+(obj-OBJ_SONG1)*8192u+copy*4096u;
    if(obj==OBJ_UPRESET0 || obj==OBJ_UPRESET0+1u)
        return 0xDC000u+(obj-OBJ_UPRESET0)*8192u+copy*4096u;
    if(obj==OBJ_UPFM6 || obj==OBJ_FM6BANK)
        return copy?0xFE000u:0x9F000u;
    if(obj==OBJ_SETTINGS)return 0xFC000u+copy*4096u;
    if(obj==OBJ_SONGIDX)return 0xD8000u+copy*4096u;
    if(obj==OBJ_AUTOSAVE)return 0xE5000u+copy*4096u;
    return 0u;
}
static int st_read(uint32_t off,void *dst,uint32_t n) {
    if(off>=sizeof nor || n>sizeof nor-off)return -1;
    memcpy(dst,nor+off,n);return 0;
}
static int st_erase(uint32_t off) {
    if(off>=sizeof nor || (off&4095u))return -1;
    if(fail_erase_after==0){
        memset(nor+off,0xFF,2048u);erases++;fail_erase_after=-1;return -1;
    }
    if(fail_erase_after>0)fail_erase_after--;
    memset(nor+off,0xFF,4096u);erases++;return 0;
}
static int mock_old_valid(uint32_t off,uint32_t obj,uint32_t copy) {
    st_hdr_t h;
    if(!off || off+4096u>sizeof nor || st_read(off,&h,sizeof h))return 0;
    if(h.marker!=0x4F4C4431u || h.object!=obj || h.copy!=copy)return 0;
    for(uint32_t i=sizeof h;i<4095u;i++)if(nor[off+i]!=0xAAu)return 0;
    return nor[off+4095u]==0x5Au;
}
static int st_copy_valid(uint32_t obj,uint32_t copy,st_hdr_t *head) {
    const uint32_t off=st_sector(obj,copy);
    if(!mock_old_valid(off,obj,copy))return 0;
    if(head)memcpy(head,nor+off,sizeof *head);
    return 1;
}
static int fvb_valid(uint32_t address,uint32_t mode,st_hdr_t *head) {
    uint32_t obj= mode?OBJ_FM6BANK:OBJ_FM6BANK;
    uint32_t copy=address==0xFE000u;
    if(!mock_old_valid(address,obj,copy))return -1;
    if(head)memcpy(head,nor+address,sizeof *head);
    return 0;
}
static int transport_busy(void){return 0;}
static void fm1_wdt_feed(void){wdt++;}
static uint32_t lib_base(uint32_t bank,uint32_t copy){
    return LIB_BASE+(bank*2u+copy)*LIB_SECTOR;
}
static int mprs_read(uint32_t off,void *p,uint32_t n){return st_read(off,p,n);}
static int lib_flash_read(uint32_t off,void *p,uint32_t n){return st_read(off,p,n);}
static int mprs_erase(uint32_t off){return st_erase(off);}
static int lib_flash_erase(uint32_t off){return st_erase(off);}
#include "../firmware/src/project_refs_store.c"
/* Program NOR with real one-way 1->0 behavior and a separately
 * interruptible FINAL MPS1 commit-header write. */
static int mprs_prog(uint32_t off,const void *src,uint32_t n){
    if(off>=sizeof nor || n>sizeof nor-off)return -1;
    const uint8_t *b=(const uint8_t*)src;
    for(uint32_t i=0;i<n;i++)
        if((nor[off+i] & b[i])!=b[i])return -1;
    if(off==MPRS_BASE && n==sizeof(mprs_head_t)){
        programs++;
        if(tear_final_marker){
            for(uint32_t i=0;i<2;i++)nor[off+i]&=b[i];
            return -1;
        }
    }
    for(uint32_t i=0;i<n;i++)nor[off+i]&=b[i];
    return 0;
}
/* Use the REAL MPRS save and readback logic, not a simulated header stub. */

#include "../firmware/src/project_refs_init.c"

static void old_sector(uint32_t obj,uint32_t copy){
    uint32_t off=st_sector(obj,copy);
    st_hdr_t h={0x4F4C4431u,obj,copy};
    memset(nor+off,0xAA,4096u);
    memcpy(nor+off,&h,sizeof h);
    nor[off+4095u]=0x5Au;
}
static void reset_source(void) {
    memset(nor,0xFF,sizeof nor);
    erases=programs=wdt=0;fail_erase_after=-1;tear_final_marker=0;flash_ok=1;
    for(uint32_t b=0;b<28u;b++)
        for(uint32_t c=0;c<2u;c++)old_sector(OBJ_SONG1+b,c);
    for(uint32_t b=0;b<2u;b++)
        for(uint32_t c=0;c<2u;c++)old_sector(OBJ_UPRESET0+b,c);
    for(uint32_t c=0;c<2u;c++){
        old_sector(OBJ_UPFM6,c);
        old_sector(OBJ_SETTINGS,c);
        old_sector(OBJ_SONGIDX,c);
        old_sector(OBJ_AUTOSAVE,c);
    }
    /* The owner previously observed this one FM6 voice sector erased. */
    memset(nor+FVB_HOME,0xFF,4096u);
}
static int erased(uint32_t off){
    for(uint32_t i=0;i<4096u;i++)if(nor[off+i]!=0xFFu)return 0;
    return 1;
}
static int ck(const char *label,int success){
    printf("CLEAN FORMAT %-61s %s\n",label,success?"PASS":"FAIL");
    return !success;
}
int main(void){
    int errors=0;
    reset_source();
    memset(nor+MPRS_BASE,0x6A,0x38000u);
    uint32_t old=0x3153504Du;memcpy(nor+MPRS_BASE,&old,4);
    nor[0x4000]=0x77; nor[0xE0000]=0x66; nor[0xEA000]=0x88;
    nor[0x97000]=0x55; nor[0xDA000]=0x44; nor[0xFF000]=0x33;
    mprs_head_t marker;
    errors+=ck("MPS1 marker cannot unlock LIB96",!mprs_valid(0,0,&marker));
    errors+=ck("arbitrary old contents pass read-only preflight",clean_layout_preflight()==0&&erases==0&&programs==0);
    errors+=ck("format commits MPS2 marker last",clean_layout_initialize()==0&&programs==1&&mprs_valid(0,0,&marker)&&marker.magic==0x3253504Du);
    int ok=1;for(uint32_t i=1;i<56u;i++)if(!erased(MPRS_BASE+i*4096u))ok=0;
    for(uint32_t i=0;i<6;i++)if(!erased(st_sector(clean_user_object(i),clean_user_copy(i))))ok=0;
    for(uint32_t i=0;i<7;i++)if(!erased(clean_aux_address(i)))ok=0;
    errors+=ck("all 69 owned sectors clear except marker",ok);
    errors+=ck("protected regions unchanged",nor[0x4000]==0x77&&nor[0xE0000]==0x66&&nor[0xEA000]==0x88&&nor[0x97000]==0x55&&nor[0xDA000]==0x44&&nor[0xFF000]==0x33);
    uint8_t record[MPR_MAX_RECORD],again[MPR_MAX_RECORD];memset(record,0x5A,sizeof record);
    errors+=ck("cold boot first and last project sections empty",mprs_load(0,record)==0&&record[0]==0&&mprs_load(31,record)==0&&record[0]==0);
    memset(record,0x31,sizeof record);
    errors+=ck("marker survives alternating project save",mprs_save(0,record)==0&&mprs_valid(0,1,&marker)&&mprs_load(0,again)==0&&!memcmp(record,again,sizeof record));
    unsigned done=erases;errors+=ck("completed format cannot repeat",clean_layout_initialize()==1&&erases==done);
    int failures=1;
    for(int n=0;n<69;n++){
        reset_source();memset(nor+MPRS_BASE,0x6A,0x38000u);memset(nor+FVB_HOME,0x33,4096);
        if(n==68)tear_final_marker=1;
        fail_erase_after=n;
        if(clean_layout_initialize()!=2||(n<68&&programs)||mprs_valid(0,0,&marker)||mprs_valid(0,1,&marker)){failures=0;break;}
    }
    errors+=ck("erase interruption leaves no valid new marker",failures);
    reset_source();tear_final_marker=1;
    errors+=ck("torn final marker leaves writes locked",clean_layout_initialize()==2&&!mprs_valid(0,0,&marker)&&!mprs_valid(0,1,&marker));
    return !!errors;
}
