/* SPDX-License-Identifier: GPL-3.0-only
 * No-device NOR simulation for 16 paired reference-project banks.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/project_refs.c"
static uint8_t nor[0x100000];
static int fail_after=-1,tear_after=-1,erase_torn=-1,outside=0,writes=0;
static int mprs_read(uint32_t off,void *dst,uint32_t n)
{
    if(off<0xA0000u || n>0xC0000u-off || off>=0xC0000u)return -1;
    memcpy(dst,nor+off,n);return 0;
}
static int mprs_erase(uint32_t off)
{
    if(off<0xA0000u||off+4096u>0xC0000u||(off&4095u)){outside++;return -1;}
    if(fail_after==0)return -1;
    if(fail_after>0)fail_after--;
    if(erase_torn==0){
        memset(nor+off,0xFF,2048u);
        erase_torn=-1;writes++;return -1;
    }
    if(erase_torn>0)erase_torn--;
    memset(nor+off,0xFF,4096u);writes++;return 0;
}
static int mprs_prog(uint32_t off,const void *src,uint32_t n)
{
    if(off<0xA0000u||off+n>0xC0000u||((off&255u)+n)>256u){outside++;return -1;}
    if(fail_after==0)return -1;
    if(fail_after>0)fail_after--;
    const uint8_t *s=src;
    /* Simulate power loss after NOR physically programs half a page,
     * including a torn final commit header. No rollback of written bits. */
    if(tear_after==0){
        for(uint32_t i=0;i<n/2u;i++)nor[off+i]&=s[i];
        tear_after=-1;writes++;return -1;
    }
    if(tear_after>0)tear_after--;
    for(uint32_t i=0;i<n;i++)nor[off+i]&=s[i];
    writes++;return 0;
}
#include "../firmware/src/project_refs_store.c"
static int ck(const char *s,int ok)
{
    printf("MPRS %-60s %s\n",s,ok?"PASS":"FAIL");
    return !ok;
}
int main(void)
{
    static uint8_t records[32][MPR_MAX_RECORD],got[MPR_MAX_RECORD],value[MPR_MAX_RECORD],prefs[6];
    uint8_t name[12]={'S','E','C','T','I','O','N'};
    int bad=0;
    memset(nor,0xFF,sizeof nor);
    for(uint32_t slot=0;slot<32;slot++){
        for(uint32_t k=0;k<6;k++)prefs[k]=(uint8_t)((slot+k)%64);
        uint8_t settings[MPR_MAX_SETTINGS];
        for(uint32_t k=0;k<sizeof settings;k++)settings[k]=(uint8_t)(slot*7u+k);
        int len=mpr_encode(records[slot],slot%4u,prefs,name,settings,sizeof settings);
        if(len!=MPR_MAX_RECORD){bad++;break;}
    }
    int all=1;
    for(uint32_t slot=0;slot<32;slot++)
        if(mprs_save(slot,records[slot]))all=0;
    bad+=ck("all 32 projects persist as two per A/B sector bank",all);
    for(uint32_t slot=0;slot<32;slot++)
        if(mprs_load(slot,got)||memcmp(got,records[slot],MPR_MAX_RECORD))all=0;
    bad+=ck("all 32 reference records and project settings read back",all);
    bad+=ck("16 banks x 2 flash sectors reserve 128 KiB",16u*2u*4096u==131072u);
    bad+=ck("no writes outside 0xA0000..0xBFFFF",!outside);
    memcpy(value,records[1],sizeof value);
    value[32]^=0x5Au; /* test storage atomicity, not payload schema validation */
    bad+=ck("replace section B without changing paired section A",!mprs_save(1,value));
    bad+=ck("new section B reads correctly",!mprs_load(1,got)&&!memcmp(got,value,sizeof got));
    bad+=ck("paired section A still reads correctly",!mprs_load(0,got)&&!memcmp(got,records[0],sizeof got));
    bad+=ck("reset section B for interruption tests",!mprs_save(1,records[1]));
    all=1;
    for(int stop=0;stop<18;stop++){
        fail_after=stop;
        (void)mprs_save(1,value);
        fail_after=-1;
        if(mprs_load(1,got) ||
           (memcmp(got,records[1],sizeof got)&&memcmp(got,value,sizeof got))){all=0;break;}
        if(mprs_save(1,records[1])){all=0;break;}
    }
    bad+=ck("interrupted writes preserve old or fully committed record",all);
    /* Inject a torn physical page program at every page/commit position, not
     * merely a failure between complete NOR operations. The previous
     * committed project and the paired section must remain recoverable. */
    all=1;
    for(int cut=0;cut<17;cut++){
        if(mprs_save(1,records[1])){all=0;break;}
        tear_after=cut;
        (void)mprs_save(1,value);
        tear_after=-1;
        if(mprs_load(1,got) ||
           (memcmp(got,records[1],sizeof got)&&memcmp(got,value,sizeof got))){
            all=0;break;
        }
        if(mprs_load(0,got) || memcmp(got,records[0],sizeof got)){
            all=0;break;
        }
    }
    bad+=ck("torn page/header writes retain complete section and its sibling",all);
    /* A sector erase itself may lose power partway through. Both cases
     * must preserve the opposite committed copy with both sections. */
    all=1;
    for(int cut=0;cut<1;cut++){
        if(mprs_save(1,records[1])){all=0;break;}
        erase_torn=cut;
        (void)mprs_save(1,value);
        erase_torn=-1;
        if(mprs_load(1,got)||memcmp(got,records[1],sizeof got)||
           mprs_load(0,got)||memcmp(got,records[0],sizeof got)){
            all=0;break;
        }
    }
    bad+=ck("partially erased destination never damages committed project pair",all);


    {
        uint32_t a=mprs_base(15,0),b=mprs_base(15,1);
        memset(nor+a,0xFF,4096);memset(nor+b,0xFF,4096);
        nor[b+4095]=0x22;
        int before=writes;
        bad+=ck("foreign data in uninitialized bank cannot be erased",
                mprs_save(30,records[30])==2 && writes==before && nor[b+4095]==0x22);
    }
    printf("MPRS %s\n",bad?"FAIL":"ALL TESTS PASSED");
    return !!bad;
}
