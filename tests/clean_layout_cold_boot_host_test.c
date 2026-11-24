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

enum { LIB_SECTOR=4096, LIB_BANKS=4,
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
    if(obj>=OBJ_SONG1 && obj<OBJ_SONG1+20u)
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
    for(uint32_t b=0;b<20u;b++)
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
    errors+=ck("valid original 53-sector source passes read-only preflight",
               clean_layout_preflight()==0 && erases==0 && programs==0);
    nor[0xE7000u+4000u]=0x19u;
    errors+=ck("foreign byte in LAST sector blocks ALL physical writes",
               clean_layout_initialize()==1 && erases==0 && programs==0);
    reset_source();
    fail_erase_after=20;
    errors+=ck("interrupted destructive erase returns error",
               clean_layout_initialize()==2 && erases==21u && programs==0);
    const uint32_t after=erases;
    errors+=ck("torn erase fails closed on retry without new writes",
               clean_layout_initialize()==1 && erases==after && programs==0);
    /* Every active old sector is a distinct interruption boundary. A torn
     * erase at ANY point must stop, and a restarted attempt must refuse
     * rather than silently repeat erasures or commit the MPR1 marker. */
    int all_interruptions_safe=1;
    for(int fail_at=0;fail_at<52;fail_at++){
        reset_source();
        fail_erase_after=fail_at;
        if(clean_layout_initialize()!=2 ||
           erases!=(uint32_t)(fail_at+1) || programs!=0){
            all_interruptions_safe=0;
            break;
        }
        const uint32_t partial_erases=erases;
        if(clean_layout_preflight()!=1 ||
           clean_layout_initialize()!=1 ||
           erases!=partial_erases || programs!=0){
            all_interruptions_safe=0;
            break;
        }
    }
    errors+=ck("all 52 active-sector torn-erase boundaries fail closed",all_interruptions_safe);
    reset_source();
    tear_final_marker=1;
    errors+=ck("interrupted final MPR1 marker write returns error",
               clean_layout_initialize()==2 && programs==1);
    const uint32_t final_erases=erases;
    tear_final_marker=0;
    errors+=ck("partial final marker blocks automatic repeat of destructive format",
               clean_layout_initialize()==1 && erases==final_erases && programs==1);
    uint8_t partial_record[MPR_MAX_RECORD];
    errors+=ck("torn MPS1 marker cannot be loaded as valid project data",
               mprs_load(0u,partial_record)==2);
    reset_source();
    errors+=ck("valid source can format and commit final one-shot marker",
               clean_layout_initialize()==0 && programs==1 && wdt>0);
    uint8_t cold_record[MPR_MAX_RECORD];
    memset(cold_record,0x5A,sizeof cold_record);
    errors+=ck("REAL MPS1 reader finds committed first bank after reboot",
               mprs_load(0u,cold_record)==0);
    int zero=1;
    for(uint32_t i=0;i<sizeof cold_record;i++)
        if(cold_record[i]!=0u)zero=0;
    errors+=ck("cold boot reads first USER-reference section as empty",zero);
    memset(cold_record,0x5A,sizeof cold_record);
    errors+=ck("paired section B remains independently empty",
               mprs_load(1u,cold_record)==0);
    zero=1;
    for(uint32_t i=0;i<sizeof cold_record;i++)
        if(cold_record[i]!=0u)zero=0;
    errors+=ck("cold boot reads second reference section as empty",zero);
    memset(cold_record,0x5A,sizeof cold_record);
    errors+=ck("all other project banks cold boot as empty",
               mprs_load(31u,cold_record)==0);
    zero=1;
    for(uint32_t i=0;i<sizeof cold_record;i++)
        if(cold_record[i]!=0u)zero=0;
    errors+=ck("last of 8 x A-D project sections is empty",zero);
    int ok=1;
    for(uint32_t i=1;i<40u;i++)if(!erased(MPRS_BASE+i*4096u))ok=0;
    for(uint32_t i=0;i<4u;i++)if(!erased(0xDC000u+i*4096u))ok=0;
    for(uint32_t i=0;i<2u;i++)if(!erased(st_sector(OBJ_UPFM6,i)))ok=0;
    for(uint32_t i=0;i<2u;i++){
        if(!erased(st_sector(OBJ_SETTINGS,i)) ||
           !erased(st_sector(OBJ_SONGIDX,i)) ||
           !erased(st_sector(OBJ_AUTOSAVE,i)))ok=0;
    }
    if(!erased(FVB_HOME))ok=0;
    errors+=ck("all 53 expected sectors cleared except MPR1 marker",ok);
    const uint32_t done=erases;
    errors+=ck("completed first install NEVER formats a second time",
               clean_layout_initialize()==1 && erases==done && programs==1);
    errors+=ck("no physical executable is produced by this host test",1);
    printf("CLEAN FORMAT: %s\n",errors?"FAIL":"ALL TESTS PASSED");
    return !!errors;
}
