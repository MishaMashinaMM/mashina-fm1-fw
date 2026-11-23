/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental MPR1 project-owned settings payload, revision 1.
 * Included after project_t in project.c, or in host project tests.
 *
 * NO sound engine, sound parameter, FM6 patch, independent drum-kit sample,
 * sequencer step or legacy snapshot is stored here. The six MPR1 preset IDs
 * identify those sounds. Project-owned settings, mappings, lyrics and 32 CC
 * macros survive independent of the shared preset's future sound edits.
 *
 * This is a byte-exact, bounded codec. Never silently truncate a section:
 * if even the compact representation is full, SAVE must refuse it.
 */
#include <stdint.h>
/* Embedded AC79 build uses firmware/src/libc.c; host tests provide libc. */
#define MPR_SETTINGS_VERSION 1u

static int mpr_is_project_param(uint32_t i)
{
    /* Deliberately matches ui.c's sound-load param_kept rules for MASHINA
     * without the retired DIGITAL/FM4 mode. When the parameter model changes,
     * the MPR_SETTINGS_VERSION must be bumped and this list audited. */
    return i == P_LEVEL || i == P_PAN || i == P_MUTE ||
           (i >= P_AMODE && i <= P_SGATE) ||
           (i >= P_SLCR && i <= P_SLDEPTH) ||
           i == P_SOFS || i == P_POFS ||
           (i == P_HVEL || i == P_HTIM || i == P_HGAT || i == P_HRQ) ||
           (i == P_CHRD || i == P_CVOI || i == P_CSTR ||
            i == P_TCHAN || i == P_MSND || i == P_MPAT ||
            i == P_MAMT || i == P_MMODE);
}
static uint32_t mpr_project_param_count(void)
{
    uint32_t n=0;
    for(uint32_t i=0;i<P_COUNT;i++)n+=(uint32_t)mpr_is_project_param(i);
    return n;
}
typedef struct {
    uint8_t *data;
    uint32_t size,pos;
    int failed;
} mpr_output_t;
typedef struct {
    const uint8_t *data;
    uint32_t size,pos;
    int failed;
} mpr_input_t;
static void mpr_write(mpr_output_t *o,const void *data,uint32_t size)
{
    if(o->failed || size>o->size-o->pos){o->failed=1;return;}
    if(size)memcpy(o->data+o->pos,data,size);
    o->pos+=size;
}
static void mpr_byte(mpr_output_t *o,uint32_t v)
{
    uint8_t b=(uint8_t)v; mpr_write(o,&b,1);
}
static void mpr_short(mpr_output_t *o,int32_t v)
{
    mpr_byte(o,(uint32_t)v); mpr_byte(o,(uint32_t)v>>8);
}
static void mpr_read(mpr_input_t *o,void *dst,uint32_t size)
{
    if(o->failed || size>o->size-o->pos){o->failed=1;return;}
    if(size)memcpy(dst,o->data+o->pos,size);
    o->pos+=size;
}
static uint32_t mpr_take(mpr_input_t *o)
{
    uint8_t b=0; mpr_read(o,&b,1);return b;
}
static int16_t mpr_take_short(mpr_input_t *o)
{
    uint32_t lo=mpr_take(o),hi=mpr_take(o);
    return (int16_t)((hi<<8)|lo);
}
/* Both encoder and decoder serialize fields in the same fixed order; field
 * groups are intentionally explicit so accidental layout drift is reviewable.
 * Project-only pad controls and MIDI remappings are retained; kit *sounds*
 * (lab_patch/dx/lab_global) are intentionally sourced from the preset bank.
 */
static uint32_t mpr_project_settings_encode(const project_t *p,uint8_t *data,uint32_t cap)
{
    if(!p || !data || cap>MPR_MAX_SETTINGS || p->cm_count>CM_N ||
       mpr_project_param_count()>255u) return 0;
    mpr_output_t o={data,cap,0,0};
    mpr_byte(&o,MPR_SETTINGS_VERSION);
    mpr_byte(&o,G_COUNT);
    mpr_byte(&o,mpr_project_param_count());
    mpr_byte(&o,p->sel);
    for(uint32_t i=0;i<G_COUNT;i++)mpr_short(&o,p->g[i]);
    for(uint32_t k=0;k<NTRK;k++)
        for(uint32_t i=0;i<P_COUNT;i++)
            if(mpr_is_project_param(i)) {
                int32_t v=p->t[k].p[i];
                if(v < -64 || v > 127) return 0;
                mpr_byte(&o,(uint32_t)(v+64));
            }
    mpr_write(&o,p->rx,NTRK);
    mpr_write(&o,p->transpose,NTRK);
    mpr_write(&o,p->fine,NTRK);
    mpr_write(&o,p->drum_timing,NTRK);
    mpr_write(&o,p->zone,sizeof p->zone);
    mpr_byte(&o,p->cm_count);
    mpr_write(&o,p->cm_rows,p->cm_count*CM_ROW_SIZE);
    mpr_write(&o,p->eq_gain,3);
    mpr_byte(&o,p->eq_on);
    mpr_write(&o,p->macro,sizeof p->macro);
    mpr_write(&o,p->pfx_lane,sizeof p->pfx_lane);
    mpr_byte(&o,p->pfx_ltgt);
    mpr_short(&o,p->dx_mute);
    mpr_write(&o,p->lab_note_on,sizeof p->lab_note_on);
    mpr_write(&o,p->lab_note,sizeof p->lab_note);
    mpr_write(&o,p->dx_note_on,sizeof p->dx_note_on);
    mpr_write(&o,p->dx_note,sizeof p->dx_note);
    mpr_write(&o,p->pad_pan,sizeof p->pad_pan);
    mpr_write(&o,p->pad_pitch,sizeof p->pad_pitch);
    mpr_write(&o,p->pad_timing,sizeof p->pad_timing);
    mpr_write(&o,p->perf_curve,sizeof p->perf_curve);
    mpr_write(&o,p->perf_sound,sizeof p->perf_sound);
    mpr_write(&o,p->perf_choke,sizeof p->perf_choke);
    mpr_write(&o,p->perf_retrigger,sizeof p->perf_retrigger);
    for(uint32_t k=0;k<NTRK;k++){
        uint32_t n=0;
        while(n+1u<LYR_LEN && p->lyr[k][n])n++;
        mpr_byte(&o,n);
        mpr_write(&o,p->lyr[k],n);
    }
    return o.failed?0:o.pos;
}
/* Caller must supply a zeroed project_t for all omitted sound fields.
 * On malformed data, partial output must not be used. Decode exact length
 * only: trailing bytes, unsupported versions and bad counts are rejected. */
static int mpr_project_settings_decode(project_t *p,const uint8_t *data,uint32_t len)
{
    if(!p || !data || len>MPR_MAX_SETTINGS || len<4u)return 0;
    mpr_input_t o={data,len,0,0};
    if(mpr_take(&o)!=MPR_SETTINGS_VERSION ||
       mpr_take(&o)!=G_COUNT ||
       mpr_take(&o)!=mpr_project_param_count())return 0;
    p->sel=(uint8_t)mpr_take(&o);
    if(p->sel>=NTRK)return 0;
    for(uint32_t i=0;i<G_COUNT;i++)p->g[i]=mpr_take_short(&o);
    for(uint32_t k=0;k<NTRK;k++)
        for(uint32_t i=0;i<P_COUNT;i++)
            if(mpr_is_project_param(i))p->t[k].p[i]=(int16_t)mpr_take(&o)-64;
    mpr_read(&o,p->rx,NTRK);p->rx_on=1u;
    mpr_read(&o,p->transpose,NTRK);
    mpr_read(&o,p->fine,NTRK);
    mpr_read(&o,p->drum_timing,NTRK);
    mpr_read(&o,p->zone,sizeof p->zone);p->zone_on=1u;
    p->cm_count=(uint8_t)mpr_take(&o);
    if(o.failed || p->cm_count>CM_N)return 0;
    mpr_read(&o,p->cm_rows,p->cm_count*CM_ROW_SIZE);
    mpr_read(&o,p->eq_gain,3);p->eq_on=(uint8_t)mpr_take(&o);p->eq_present=1u;
    mpr_read(&o,p->macro,sizeof p->macro);
    mpr_read(&o,p->pfx_lane,sizeof p->pfx_lane);
    p->pfx_ltgt=(uint8_t)mpr_take(&o);
    p->dx_mute=(uint16_t)mpr_take_short(&o);
    mpr_read(&o,p->lab_note_on,sizeof p->lab_note_on);
    mpr_read(&o,p->lab_note,sizeof p->lab_note);
    mpr_read(&o,p->dx_note_on,sizeof p->dx_note_on);
    mpr_read(&o,p->dx_note,sizeof p->dx_note);
    mpr_read(&o,p->pad_pan,sizeof p->pad_pan);
    mpr_read(&o,p->pad_pitch,sizeof p->pad_pitch);
    mpr_read(&o,p->pad_timing,sizeof p->pad_timing);
    mpr_read(&o,p->perf_curve,sizeof p->perf_curve);
    mpr_read(&o,p->perf_sound,sizeof p->perf_sound);
    mpr_read(&o,p->perf_choke,sizeof p->perf_choke);
    mpr_read(&o,p->perf_retrigger,sizeof p->perf_retrigger);
    for(uint32_t k=0;k<NTRK;k++){
        uint32_t n=mpr_take(&o);
        if(n>=LYR_LEN || n>o.size-o.pos){o.failed=1;break;}
        mpr_read(&o,p->lyr[k],n);
        p->lyr[k][n]=0;
    }
    if(o.failed || o.pos!=o.size)return 0;
    for(uint32_t k=0;k<NTRK;k++){
        if(!p->rx[k] || p->rx[k]>16u || p->transpose[k]<-36 ||
           p->transpose[k]>36 || p->fine[k]<-50 || p->fine[k]>50 ||
           p->drum_timing[k]>50u)return 0;
        if(p->zone[k][Z_NOTE_MIN]>p->zone[k][Z_NOTE_MAX] ||
           p->zone[k][Z_VEL_MIN]>p->zone[k][Z_VEL_MAX] ||
           p->zone[k][Z_NOTE_MAX]>127u || p->zone[k][Z_VEL_MAX]>127u ||
           p->lab_note_on[k]>1u || p->dx_note_on[k]>1u)return 0;
        for(uint32_t pad=0;pad<12u;pad++) {
            if(p->pad_pan[k][pad]<-64 || p->pad_pan[k][pad]>64 ||
               p->pad_pitch[k][pad]<-24 || p->pad_pitch[k][pad]>24 ||
               p->pad_timing[k][pad]>50u ||
               p->lab_note[k][pad]>127u ||
               p->dx_note[k][pad]>127u)return 0;
        }
    }
    for(uint32_t i=0;i<3u;i++)
        if(p->eq_gain[i]<-15 || p->eq_gain[i]>15)return 0;
    if(p->eq_on>1u || p->pfx_ltgt>2u)return 0;
    return 1;
}
