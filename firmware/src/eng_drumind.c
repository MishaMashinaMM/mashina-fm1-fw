/* GPL-3.0-only — DRUM-IND: twelve non-tonal industrial percussion textures. */
enum { IND_CLANG, IND_ANVIL, IND_METAL, IND_SCRAPE, IND_WHISTLE, IND_SCREECH,
       IND_BUZZ, IND_LIGHTNING, IND_ENGINE, IND_WHOOSH, IND_MOTOR, IND_EXPLOSION,
       IND_NPAD };
static const char *const IND_NAMES[12] = {
    "CLANG C", "ANVIL C#", "METAL D", "SCRAPE D#", "WHISTLE E", "SCREECH F",
    "BUZZ F#", "LIGHTNING G", "ENGINE G#", "WHOOSH A", "MOTOR A#", "EXPLOSION B"
};
static const char *const IND_FIELDS[8] = {
    "GRAIN", "DECAY", "WEIGHT", "STRIKE", "MOTION", "AIR", "CRUNCH", "TAIL"
};
typedef struct {
    uint32_t rng, age;
    int32_t lp, mid, slow, rough, band, band_low, amp, hit;
    uint8_t kind, active;
} ind_voice_t;
static ind_voice_t ind_state[NPART][NVOICE] __attribute__((section(".pool")));
static uint8_t ind_patch[NPART][12][8] __attribute__((section(".pool")));
static uint8_t ind_override[NPART][12] __attribute__((section(".pool")));
static uint8_t ind_ready[NPART] __attribute__((section(".pool")));
static uint16_t ind_dirty[NPART] __attribute__((section(".pool")));
static void ind_patch_reset(track_t *t)
{
    uint32_t ti = (uint32_t)(t - trk);
    if (ti >= NPART) return;
    ind_ready[ti] = 0; ind_dirty[ti] = 0;
    memset(ind_override[ti], 0, sizeof ind_override[ti]);
}
static void ind_patch_ensure(track_t *t)
{
    uint32_t ti = (uint32_t)(t - trk);
    if (ti >= NPART || ind_ready[ti]) return;
    for (uint32_t k = 0; k < IND_NPAD; k++)
        for (uint32_t f = 0; f < 8u; f++)
            ind_patch[ti][k][f] = (uint8_t)clamp(t->p[P_E0 + f], 0, 127);
    ind_ready[ti] = 1u;
}
static uint8_t *ind_patch_of(track_t *t, uint32_t pad)
{
    if ((uint32_t)(t - trk) >= NPART || pad >= IND_NPAD) return 0;
    ind_patch_ensure(t);
    return ind_patch[t - trk][pad];
}
static uint8_t ind_param_value(track_t *t, uint32_t pad, uint32_t f)
{
    uint8_t *p = ind_patch_of(t, pad);
    return p && f < 8u ? p[f] : 0;
}
static void ind_patch_set(track_t *t, uint32_t pad, uint32_t f, uint32_t value)
{
    uint8_t *p = ind_patch_of(t, pad);
    if (!p || f >= 8u) return;
    p[f] = (uint8_t)clamp(value, 0, 127);
    ind_override[t - trk][pad] |= (uint8_t)(1u << f);
    ind_dirty[t - trk] |= (uint16_t)(1u << pad);
}
static uint32_t ind_random(uint32_t *s)
{
    uint32_t x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return *s = x;
}
static void ind_note_on(track_t *t, voice_t *v)
{
    uint32_t slot = (uint32_t)(v - t->v);
    if (slot >= NVOICE) return;
    ind_voice_t *a = &ind_state[t - trk][slot];
    memset(a, 0, sizeof *a);
    a->rng = 0xA67E71D5u ^ ((uint32_t)v->note * 0x9E3779B9u) ^ (uint32_t)v->age;
    if (!a->rng) a->rng = 1;
    a->kind = (uint8_t)(v->note % 12u);
    a->amp = a->hit = 1 << 24;
    a->active = 1;
}
static int32_t ind_amp(track_t *t, voice_t *v, int32_t adsr)
{
    uint32_t slot = (uint32_t)(v - t->v);
    if (slot >= NVOICE || !ind_state[t - trk][slot].active) {
        v->active = v->gate = 0; v->stage = 0; v->env = 0; return 0;
    }
    if (v->stage == 4u) return adsr;
    v->env = 1 << 24;
    return 32767;
}
/* Noise spectral profiles, not note frequencies. The last two columns
 * describe one-shot envelope length in milliseconds and initial attack.
 * Narrow noisy whistle/screech sweeps intentionally never settle on a
 * fixed musical pitch; ENGINE and MOTOR pulse irregular noise, not tones. */
static const uint16_t IND_PROFILE[12][5] = {
    {130,2100,11400,440,105}, /* CLANG: existing dense metal impact */
    {350,1150, 8200,260,127}, /* ANVIL: existing heavy hammer impact */
    {110,3600,18800,580, 95}, /* METAL: bright sheet of rattling fragments */
    {120,4300,15900,520, 77}, /* SCRAPE: existing rough drawn metal */
    { 90,1200,10600,600, 36}, /* WHISTLE: swept narrow noisy jet */
    { 90,4600,20000,510, 97}, /* SCREECH: fractured high friction squeal */
    {140,5400,21500,450, 85}, /* BUZZ: abrasive erratic electrical buzz */
    {320,2700,24800,710,127}, /* LIGHTNING: snap / thunder roll */
    {360,1800, 8800,950, 77}, /* ENGINE: slow uneven mechanical chug */
    { 85,3200,17000,850, 50}, /* WHOOSH: existing air sweep */
    {210,3600,13400,880, 92}, /* MOTOR: fast rattling rotary air */
    {520,1700,16300,1020,127} /* EXPLOSION: broad blast / turbulent tail */
};
/* A hash-like, jittered gating function. The periodic *envelope* is fast
 * for BUZZ and MOTOR but no oscillator/fixed pitched carrier is generated. */
static int32_t ind_gate(uint32_t age, int32_t rate, int32_t shape, uint32_t rng)
{
    uint32_t period = (uint32_t)clamp(rate, 120, 9000);
    uint32_t phase = (age + ((rng >> 22) & 31u)) % period;
    uint32_t duty = (uint32_t)clamp(shape, 15, 235);
    return phase * 256u / period < duty ? 256 : 18;
}
static void ind_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    uint32_t slot=(uint32_t)(v-t->v);
    if(slot>=NVOICE||!n) return;
    ind_voice_t *a=&ind_state[t-trk][slot];
    if(!a->active) return;
    const uint32_t kind=a->kind;
    uint8_t *p=ind_patch_of(t,kind);
    if(!p) return;
    drumlab_global_ensure((uint32_t)(t-trk));
    int32_t kit[8];
    for(uint32_t k=0;k<8u;k++) kit[k]=(int32_t)drumlab_global[t-trk][k]-100;
    const int32_t grain=p[0],damp=p[1],weight=p[2],hit=p[3];
    const int32_t motion=p[4],air=p[5],crunch=p[6],tail=p[7];
    /* Broad ranges, with different physical meanings by instrument:
     * GRAIN shifts spectra or pulse granularity, MOTION changes sweeps/rate,
     * WEIGHT changes low-end, AIR brightens, CRUNCH adds unstable fragments.
     * All eight values affect at least one distinct audible component. */
    const int32_t low_k=clamp((int32_t)IND_PROFILE[kind][0]+weight*28-kit[0]*10,70,9000);
    const int32_t mid_k=clamp((int32_t)IND_PROFILE[kind][1]+(grain-64)*85+kit[0]*60,250,19000);
    const int32_t high_k=clamp((int32_t)IND_PROFILE[kind][2]+(air-64)*150+kit[4]*75,1100,30500);
    const uint32_t duration=(uint32_t)clamp((int32_t)IND_PROFILE[kind][3]*1000+
       (tail-64)*12000-(damp-64)*11500-kit[1]*4500+kit[6]*2000,24000,1800000);
    const uint32_t mul_b=dlab_decay_long(n,duration);
    const uint32_t mul_t=dlab_decay_long(n,(uint32_t)clamp(36000-(hit-64)*260,13000,57000));
    int32_t amp=a->amp,trans=a->hit,lp=a->lp,mid=a->mid,slow=a->slow,rough=a->rough;
    int32_t band=a->band,band_low=a->band_low;
    const int32_t step_b=((int32_t)(((int64_t)amp*mul_b)>>16)-amp)/(int32_t)n;
    const int32_t step_t=((int32_t)(((int64_t)trans*mul_t)>>16)-trans)/(int32_t)n;
    uint32_t seed=a->rng,age=a->age;
    const uint32_t lid=kind<8u?P_LN0+kind:P_LN8+kind-8u;
    const int32_t lane=clamp(t->p[lid],0,127);
    const int32_t gain=(int32_t)(((int64_t)lane*lane*32768*(100+kit[7]))/(16129*100));
    const int32_t attack=clamp((int32_t)IND_PROFILE[kind][4]+(hit-64)*3+kit[3],0,440);
    const int32_t texture=clamp(165+(crunch-64)*2+kit[5],20,410);
    const int32_t bodymix=clamp(155+(weight-64)*3+kit[2],10,450);
    const int32_t brightness=clamp(155+(air-64)*3+kit[4],10,450);
    for(uint32_t i=0;i<n;i++,age++,amp+=step_b,trans+=step_t){
        int32_t white=(int32_t)(ind_random(&seed)>>16)-32768;
        int32_t impulse=(int32_t)(ind_random(&seed)>>16)-32768;
        slow+=(int32_t)(((int64_t)(white-slow)*low_k)>>15);
        lp+=(int32_t)(((int64_t)(white-lp)*mid_k)>>15);
        mid+=(int32_t)(((int64_t)(white-mid)*high_k)>>15);
        rough+=(int32_t)(((int64_t)(impulse-rough)*
                           clamp(2400+(crunch-64)*190,280,24000))>>15);
        int32_t bass=slow,body=lp-slow,hiss=white-mid,grit=impulse-rough;
        /* Moving, damped band-pass: whistle = narrow air jet, screech =
         * friction, METAL = bright rattling plate resonances. Aperiodic
         * noise excitation; no harmonic or fixed-pitch sine oscillators. */
        int32_t jet=0;
        if(kind==IND_WHISTLE||kind==IND_SCREECH||kind==IND_METAL){
            uint32_t sweep=(age/(uint32_t)clamp(6+(127-motion)/9,6,24))%128u;
            int32_t center=kind==IND_WHISTLE?4100:
                           kind==IND_METAL?8700:10500;
            int32_t f=clamp(center+(int32_t)sweep*
                 (kind==IND_WHISTLE?62:kind==IND_METAL?90:105)
                 +(grain-64)*75+kit[0]*20,900,26000);
            band_low+=(int32_t)(((int64_t)f*band)>>15);
            int32_t high=white-band_low-(int32_t)(((int64_t)
                (kind==IND_WHISTLE?6700:kind==IND_METAL?9400:11500)*band)>>15);
            band+=(int32_t)(((int64_t)f*high)>>15);
            band_low=clamp(band_low,-65536,65536);
            band=clamp(band,-65536,65536);
            jet=band;
        }
        int32_t pulse=256,mix=0;
        switch(kind){
        case IND_CLANG:mix=body*2+hiss+grit;break;
        case IND_ANVIL:mix=bass*3+body*2+grit;break;
        case IND_METAL:{
            /* Grainy glittering sheet metal; very bright and fragmented,
             * deliberately not the low-mid CLANG or ANVIL impulse. */
            pulse=85+(int32_t)(ind_random(&seed)&171u);
            mix=hiss*2+grit*2+jet*3+body/2;break;
        }
        case IND_SCRAPE:
            pulse=135+(int32_t)(ind_random(&seed)&127u);
            mix=hiss*2+grit*2+body;break;
        case IND_WHISTLE:
            pulse=age<(uint32_t)(1800+(127-motion)*60)?
                (int32_t)(age*256u/(uint32_t)(1800+(127-motion)*60)):256;
            mix=jet*5+hiss/6;break;
        case IND_SCREECH:
            pulse=(ind_random(&seed)&15u)<(uint32_t)(5+motion/14)?256:48;
            mix=jet*5+grit*2;break;
        case IND_BUZZ:
            pulse=ind_gate(age,clamp(1200-(motion-64)*8,220,2600),
                 65+grain,seed);
            mix=grit*3+hiss*2;break;
        case IND_LIGHTNING:{
            /* Hot first crack and irregular splintering/rolling thunder. */
            uint32_t r=ind_random(&seed);
            pulse=age<800u?256:(r&31u)<(uint32_t)(2+motion/9)?230:58;
            mix=age<1700u?grit*4+hiss*3:bass*4+grit*2+hiss;break;
        }
        case IND_ENGINE:
            pulse=ind_gate(age,clamp(6900-motion*31,1500,7000),
                115+grain/2,seed);
            mix=bass*4+body*3+grit;break;
        case IND_WHOOSH:{
            uint32_t up=7100u;
            pulse=(int32_t)(age<up?age*256u/up:256u);
            mix=hiss*2+body*2;break;
        }
        case IND_MOTOR:
            pulse=ind_gate(age,clamp(2300-motion*13,390,2500),
                 95+grain,seed);
            mix=body*2+hiss*2+grit*2;break;
        case IND_EXPLOSION:{
            uint32_t r=ind_random(&seed);
            pulse=age<1100u?256:(r&7u)==0u?240:160;
            mix=age<3000u?grit*4+hiss*4:bass*5+body*3+grit;break;
        }
        }
        /* Model-specific mixing, but the same eight edit controls remain
         * independently effective. No harmonic oscillator is introduced. */
        int64_t raw=((int64_t)mix*amp*pulse*texture)>>42;
        raw+=((int64_t)bass*amp*bodymix)>>33;
        raw+=((int64_t)hiss*amp*brightness)>>35;
        raw+=((int64_t)(hiss+grit)*trans*attack)>>33;
        /* Make formerly weak controls materially shape the actual sound.
         * New additives are ZERO at each preset's own seed values, keeping
         * approved CLANG/ANVIL/SCRAPE/WHOOSH baseline characters intact.
         * Grain scatters short high-frequency fragments; weight changes
         * force/body; crunch exposes air grit. Motion adjusts irregular
         * impulse density on all 12 instruments, not only the motors. */
        {
            int32_t grain_change=grain-t->p[P_E0];
            int32_t weight_change=weight-t->p[P_E2];
            int32_t motion_change=motion-t->p[P_E4];
            int32_t crunch_change=crunch-t->p[P_E6];
            if(grain_change && (kind==IND_SCRAPE || kind==IND_EXPLOSION))
                raw+=((int64_t)grit*amp*grain_change)>>32;
            if(weight_change && (kind==IND_SCREECH || kind==IND_EXPLOSION))
                raw+=((int64_t)(bass+body)*amp*weight_change)>>32;
            if(weight_change && kind==IND_EXPLOSION)
                raw=(raw*clamp(256+weight_change*2,60,450))>>8;
            if(crunch_change && kind==IND_WHOOSH)
                raw+=((int64_t)grit*amp*crunch_change)>>32;
            if(motion_change && (kind==IND_CLANG || kind==IND_ANVIL ||
                 kind==IND_METAL || kind==IND_SCRAPE || kind==IND_WHOOSH)){
                int32_t scatter=(int32_t)(ind_random(&seed)&255u)-128;
                raw+=((int64_t)(hiss+grit)*amp*motion_change)>>33;
                raw+=((int64_t)scatter*motion_change*hiss*amp)>>39;
            }
        }
        raw=clamp(raw,-32768,32767);
        out[i]+=voice_amp((int32_t)((raw*gain)>>15),m,i);
    }
    a->lp=lp;a->mid=mid;a->slow=slow;a->rough=rough;
    a->band=band;a->band_low=band_low;
    a->rng=seed;a->age=age;a->amp=amp>0?amp:0;a->hit=trans>0?trans:0;
    if(a->amp<100)a->active=0;
}
static const preset_t IND_PRESETS[] = {
    {"STEEL YARD",{64,64,78,86,70,85,80,64},{0,127,127,127},0,TAG(DRUM)},
    {"PRESSURE",{50,93,95,72,97,96,65,83},{0,127,127,127},0,TAG(DRUM)},
    {"SCRAPHEAP",{93,38,62,110,115,107,116,61},{0,127,127,127},0,TAG(DRUM)}
};
static const engine_t ENG_DRUMIND = {
    .name="DRUM-IND",.page_title={"IMPACT","SURFACE"},
    .edit={
        {"GRAIN",F_PCT,0,127,64,0,0},{"DECAY",F_PCT,0,127,64,0,0},
        {"WEIGHT",F_PCT,0,127,78,0,0},{"STRIKE",F_PCT,0,127,86,0,0},
        {"MOTION",F_PCT,0,127,70,0,0},{"AIR",F_PCT,0,127,85,0,0},
        {"CRUNCH",F_PCT,0,127,80,0,0},{"TAIL",F_PCT,0,127,64,0,0}
    },
    .presets=IND_PRESETS,.npresets=NELEM(IND_PRESETS),
    .note_on=ind_note_on,.render=ind_render,.amp=ind_amp,
    .knob={P_E0,P_E2,P_E3,P_E5},.poly=NVOICE,.oneshot=1
};
