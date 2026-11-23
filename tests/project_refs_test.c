/* SPDX-License-Identifier: GPL-3.0-only
 * Host-only clean-break MPR1 project reference record checks.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/project_refs.c"
static int ck(const char *name,int ok)
{
    printf("MPR1 %-58s %s\n",name,ok?"PASS":"FAIL");
    return !ok;
}
int main(void)
{
    uint8_t record[MPR_MAX_RECORD], settings[MPR_MAX_SETTINGS], restored[MPR_MAX_SETTINGS];
    uint8_t refs[MPR_TRACKS]={13,44,2,51,13,60}, refout[MPR_TRACKS], name[MPR_NAME]={0}, nameout[MPR_NAME];
    uint32_t section=99, len=99;
    int bad=0;
    memcpy(name,"SECTION B",9);
    for(uint32_t i=0;i<sizeof settings;i++)settings[i]=(uint8_t)((i*73u+19u)&255u);
    int n=mpr_encode(record,1,refs,name,settings,sizeof settings);
    bad+=ck("max settings payload fits compact variable-length record",
            n==MPR_MAX_RECORD && MPR_MAX_RECORD==2080);
    bad+=ck("six references and full project settings round-trip",
            mpr_decode(record,n,&section,refout,nameout,restored,sizeof restored,&len)
            && section==1 && len==sizeof settings &&
            !memcmp(refs,refout,sizeof refs) &&
            !memcmp(name,nameout,sizeof name) &&
            !memcmp(settings,restored,sizeof settings));
    bad+=ck("reject damaged settings via checksum",({
        uint8_t saved=record[100];record[100]^=0x44;
        int rejected=!mpr_decode(record,n,&section,refout,nameout,restored,sizeof restored,&len);
        record[100]=saved;rejected;
    }));
    bad+=ck("reject invalid preset references",({
        uint8_t saved=refs[1];refs[1]=64;
        int rejected=!mpr_encode(record,1,refs,name,settings,20);
        refs[1]=saved;rejected;
    }));
    bad+=ck("explicitly unassigned tracks are valid",({
        refs[5]=MPR_EMPTY;
        int m=mpr_encode(record,3,refs,name,settings,20);
        m==52 && mpr_decode(record,m,&section,refout,nameout,restored,sizeof restored,&len)
            && section==3 && refout[5]==MPR_EMPTY;
    }));
    bad+=ck("reject truncated records",!mpr_decode(record,51,&section,refout,nameout,restored,sizeof restored,&len));
    bad+=ck("reject trailing bytes",!mpr_decode(record,53,&section,refout,nameout,restored,sizeof restored,&len));
    bad+=ck("reject out-of-range project section",!mpr_encode(record,4,refs,name,settings,10));
    bad+=ck("reject settings exceeding flash record limit",!mpr_encode(record,0,refs,name,settings,2049));
    printf("MPR1 %s\n",bad?"FAIL":"ALL TESTS PASSED");
    return !!bad;
}
