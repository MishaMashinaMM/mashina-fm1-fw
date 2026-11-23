/* SPDX-License-Identifier: GPL-3.0-only
 * Test project-only field mapping against real project_t layout (no device).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define main hostsim_main
#include "hostsim.c"
#undef main
#define PROJ_HOST 1
#include "../firmware/src/project.c"
#include "../firmware/src/project_refs.c"
#include "../firmware/src/project_ref_settings.c"

static int ck(const char *name,int ok){
    printf("MPR SETTINGS %-51s %s\n",name,ok?"PASS":"FAIL");
    return !ok;
}
static project_t src,dst;
static uint8_t buf[MPR_MAX_SETTINGS];
int main(void)
{
    int bad=0;
    memset(&src,0,sizeof src);
    for(uint32_t k=0;k<G_COUNT;k++)src.g[k]=(int16_t)(k+18u);
    src.sel=4;
    for(uint32_t k=0;k<NTRK;k++){
        for(uint32_t i=0;i<P_COUNT;i++)
            src.t[k].p[i]=(int16_t)(mpr_is_project_param(i)?(int32_t)(i+k)-20:71);
        src.rx[k]=(uint8_t)(k+1);
        src.transpose[k]=(int8_t)(k*4-10);
        src.fine[k]=(int8_t)(k-3);
        src.drum_timing[k]=(uint8_t)(k*3);
        src.zone[k][Z_NOTE_MIN]=(uint8_t)k;
        src.zone[k][Z_NOTE_MAX]=(uint8_t)(120+k);
        src.zone[k][Z_VEL_MIN]=1;
        src.zone[k][Z_VEL_MAX]=127;
        for(uint32_t i=0;i<12;i++){
            src.pad_pan[k][i]=(int8_t)(i-6);
            src.pad_pitch[k][i]=(int8_t)(i-4);
            src.pad_timing[k][i]=(uint8_t)i;
            src.lab_note[k][i]=(uint8_t)(40+i);
            src.dx_note[k][i]=(uint8_t)(60+i);
            src.perf_curve[k][i]=(uint8_t)i;
            src.perf_sound[k][i]=(uint8_t)(i+1);
            src.perf_choke[k][i]=(uint8_t)(i+2);
            src.perf_retrigger[k][i]=(uint8_t)(i+3);
        }
        src.lab_note_on[k]=src.dx_note_on[k]=1;
        memcpy(src.lyr[k],"WORDS-AND-MELODIES",18);
        src.t[k].engine=9;src.t[k].preset=7;
        memset(src.fm6[k],0x42,sizeof src.fm6[k]); /* must not serialize */
    }
    src.cm_count=32;
    for(uint32_t k=0;k<32;k++)
        for(uint32_t i=0;i<CM_ROW_SIZE;i++)src.cm_rows[k][i]=(uint8_t)(k+i);
    src.eq_gain[0]=-8;src.eq_gain[1]=5;src.eq_gain[2]=12;
    src.eq_on=1;
    src.macro[0]=12;src.macro[3]=106;
    src.pfx_ltgt=2;src.dx_mute=0x513;
    for(uint32_t k=0;k<sizeof src.pfx_lane;k++)src.pfx_lane[k]=(uint8_t)(k+1);
    uint32_t n=mpr_project_settings_encode(&src,buf,sizeof buf);
    bad+=ck("real project-only fields fit proposed 2-in-1 sector",n>0 && n<=MPR_MAX_SETTINGS);
    if(n){
        bad+=ck("complete mapped project settings decode",
                mpr_project_settings_decode(&dst,buf,n));
        bad+=ck("macro table and MIDI receive channels survive",
                dst.cm_count==32 && !memcmp(src.cm_rows,dst.cm_rows,sizeof src.cm_rows) &&
                !memcmp(src.rx,dst.rx,NTRK));
        bad+=ck("mixer/tuning/keyboard zones survive",
                !memcmp(src.transpose,dst.transpose,NTRK) &&
                !memcmp(src.fine,dst.fine,NTRK) &&
                !memcmp(src.zone,dst.zone,sizeof src.zone) &&
                dst.t[4].p[P_LEVEL]==src.t[4].p[P_LEVEL]);
        bad+=ck("drum MIDI mapping/pan/pitch/performance survive",
                !memcmp(src.lab_note,dst.lab_note,sizeof src.lab_note) &&
                !memcmp(src.dx_note,dst.dx_note,sizeof src.dx_note) &&
                !memcmp(src.pad_pan,dst.pad_pan,sizeof src.pad_pan) &&
                !memcmp(src.pad_pitch,dst.pad_pitch,sizeof src.pad_pitch) &&
                !memcmp(src.perf_curve,dst.perf_curve,sizeof src.perf_curve));
        bad+=ck("voice lyrics, effects, macro values and global settings survive",
                !memcmp(src.lyr,dst.lyr,sizeof src.lyr) &&
                !memcmp(src.eq_gain,dst.eq_gain,3) &&
                !memcmp(src.g,dst.g,sizeof src.g) &&
                !memcmp(src.pfx_lane,dst.pfx_lane,sizeof src.pfx_lane) &&
                dst.dx_mute==src.dx_mute);
        bad+=ck("engine, FM6 and sound-only settings are never embedded",
                dst.t[0].engine==0 && dst.t[0].preset==0 &&
                dst.t[0].p[P_ATK]==0 && dst.fm6[0][0]==0);
        bad+=ck("reject truncated settings",!mpr_project_settings_decode(&dst,buf,n-1));
        uint8_t saved=buf[0];buf[0]=2;
        bad+=ck("reject different layout version",!mpr_project_settings_decode(&dst,buf,n));
        buf[0]=saved;
        if(n<MPR_MAX_SETTINGS) {
            bad+=ck("reject settings buffer with trailing garbage",({
                buf[n]=0x61;
                !mpr_project_settings_decode(&dst,buf,n+1);
            }));
        }
    }
    /* Recomputed outer CRC does not make invalid project-specific control
     * values safe to restore. The settings decoder must validate boundaries. */
    {
        int8_t savedPitch=src.pad_pitch[0][0];
        src.pad_pitch[0][0]=49;
        n=mpr_project_settings_encode(&src,buf,sizeof buf);
        bad+=ck("refuse corrupted project-only per-pad pitch",
                n && !mpr_project_settings_decode(&dst,buf,n));
        src.pad_pitch[0][0]=savedPitch;
        uint8_t max=src.zone[0][Z_NOTE_MAX];
        src.zone[0][Z_NOTE_MAX]=250;
        n=mpr_project_settings_encode(&src,buf,sizeof buf);
        bad+=ck("refuse keyboard-zone MIDI note outside 0..127",
                n && !mpr_project_settings_decode(&dst,buf,n));
        src.zone[0][Z_NOTE_MAX]=max;
        int8_t eq=src.eq_gain[1];
        src.eq_gain[1]=40;
        n=mpr_project_settings_encode(&src,buf,sizeof buf);
        bad+=ck("refuse unsupported project EQ gain",
                n && !mpr_project_settings_decode(&dst,buf,n));
        src.eq_gain[1]=eq;
    }
    /* Stress the upper bound: 32 active CC macros plus six full 63-byte
     * lyrics and six tracks of independent per-pad settings. */
    for(uint32_t k=0;k<NTRK;k++){
        memset(src.lyr[k],'A'+(int)k,LYR_LEN-1u);
        src.lyr[k][LYR_LEN-1u]=0;
    }
    n=mpr_project_settings_encode(&src,buf,sizeof buf);
    bad+=ck("full six-track lyric and CC-macro payload fits 2-in-1 bank",
            n>0u && n<=MPR_MAX_SETTINGS &&
            mpr_project_settings_decode(&dst,buf,n) &&
            !memcmp(dst.lyr,src.lyr,sizeof src.lyr));
    printf("MPR SETTINGS: %s (encoded %u/%u bytes)\n",bad?"FAIL":"ALL TESTS PASSED",n,MPR_MAX_SETTINGS);
    return !!bad;
}
