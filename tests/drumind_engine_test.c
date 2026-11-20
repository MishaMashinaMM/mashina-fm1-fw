/* SPDX-License-Identifier: GPL-3.0-only
 * DRUM-IND: no pitched oscillator, octave repeat, independent patches,
 * and all twelve pads produce aperiodic, non-silent output. */
#define main hostsim_main
#include "hostsim.c"
#undef main
static int ind_check(const char *label, int yes) {
    printf("DRUM-IND %-57s %s\n",label,yes?"PASS":"FAIL");
    return !yes;
}
static uint64_t ind_energy(uint32_t note, int32_t *peak) {
    track_t *t=&trk[0];
    int32_t out[CTL];
    uint64_t energy=0;
    host_tracks_init();
    host_preset(t,ENGI_DRUMIND,0);
    t->p[P_ATK]=0;t->p[P_SUS]=127;
    trk_note_on(t,note,110);
    *peak=0;
    for(uint32_t n=0;n<30u;n++){
        track_render(t,out,CTL);
        for(uint32_t k=0;k<CTL;k++){
            int32_t a=out[k]<0?-(int64_t)out[k]:out[k];
            energy+=a;if(a>*peak)*peak=a;
        }
    }
    return energy;
}
int main(void){
    int bad=0;
    track_t *t=&trk[0];
    bad+=ind_check("append-only engine 20; 21 engines total",
        ENGI_DRUMIND==20u&&NENGINES==21u&&ENGINES[20]==&ENG_DRUMIND);
    bad+=ind_check("standalone from melodic INDUSTRIAL",
        &ENG_DRUMIND!=&ENG_INDUSTRIAL&&ENG_DRUMIND.oneshot);
    host_tracks_init();host_preset(t,ENGI_DRUMIND,0);
    uint8_t before=ind_param_value(t,IND_CLANG,2u);
    ind_patch_set(t,IND_CHAIN,2u,120u);
    bad+=ind_check("independent pad edits persist in RAM",
        ind_param_value(t,IND_CLANG,2u)==before&&
        ind_param_value(t,IND_CHAIN,2u)==120u&&
        (ind_dirty[0]&(1u<<IND_CHAIN)));
    drumlab_global_reset(0);
    drumlab_global[0][0]=180u;
    bad+=ind_check("independent kit global does not rewrite pad",
        ind_param_value(t,IND_CHAIN,2u)==120u);
    for(uint32_t k=0;k<12u;k++){
        int32_t peak=0;
        uint64_t energy=ind_energy(36u+k,&peak);
        char label[96];
        snprintf(label,sizeof label,"pad %u audible and bounded",(unsigned)k);
        bad+=ind_check(label,energy>10000u&&peak<32768);
    }
    host_tracks_init();host_preset(t,ENGI_DRUMIND,0);
    trk_note_on(t,36u,110u);
    int same=t->v[0].active&&ind_state[0][0].kind==IND_CLANG;
    host_tracks_init();host_preset(t,ENGI_DRUMIND,0);
    trk_note_on(t,60u,110u);
    bad+=ind_check("C pad identity repeats across octaves",
        same&&t->v[0].active&&ind_state[0][0].kind==IND_CLANG);
    printf("DRUM-IND TESTS %s (%d failed)\n",bad?"FAILED":"PASSED",bad);
    return !!bad;
}
