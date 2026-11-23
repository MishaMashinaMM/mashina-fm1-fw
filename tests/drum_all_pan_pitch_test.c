/* SPDX-License-Identifier: GPL-3.0-only
 * Verify all four drum engines expose identical per-pad PAN/PITCH controls,
 * preserve them through project storage, and route new drum sides to stereo.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static int check_pad(const char *label, int yes)
{
    printf("PAD PAN/PITCH: %-62s %s\n",label,yes?"PASS":"FAIL");
    return !yes;
}
int main(void)
{
    static const uint8_t engines[4]={ENGI_DRUM,ENGI_DRUMLAB,ENGI_DRUMAC,ENGI_DRUMIND};
    int bad=0;
    int32_t mono[CTL];
    for(uint32_t j=0;j<4;j++){
        uint32_t e=engines[j];
        track_t *t=&trk[0];
        ui_power_on();
        set_engine_of(t,e);
        t->engine=t->eng_req; t->xf_on=0u;
        song.sel=0u;
        drum_pad_pan[0][0]=drum_pad_pitch[0][0]=0;
        press(B_SEQ);
        bad+=check_pad("SEQ map opens",ui.drum_map_open);
        turn(EN_K2,1);
        turn(EN_K3,1);
        bad+=check_pad("K2 PAN and K3 PITCH act on selected pad",
                       drum_pad_pan[0][0]>0&&drum_pad_pitch[0][0]>0);
        bad+=check_pad("per-pad pitch ratio responds to +/- semitones",
                       drum_pad_freq_ratio(0,0)>4096u);
        bad+=check_pad("project writes per-pad stereo and pitch",
                       project_save(0)==0);
        drum_pad_pan[0][0]=drum_pad_pitch[0][0]=0;
        project_load(0);
        bad+=check_pad("project reload restores both controls",
                       drum_pad_pan[0][0]>0&&drum_pad_pitch[0][0]>0);
        /* The positive PAN must create right/left side separation
         * rather than remain monophonic after a UI-only change. */
        t->engine=t->eng_req; t->xf_on=0u;
        memset(t->v,0,sizeof t->v);
        trk_note_on(t,36u,110u);
        for(uint32_t b=0;b<16;b++)track_render(t,mono,CTL);
        {
            int64_t side=0;
            for(uint32_t k=0;k<CTL;k++)side+=dx_sl[k]>0?dx_sl[k]:-dx_sl[k];
            bad+=check_pad("per-pad PAN drives stereo side output",side>0);
        }
    }
    printf("ALL PAD PAN/PITCH %s (%d failed)\n",bad?"FAILED":"PASSED",bad);
    return !!bad;
}
