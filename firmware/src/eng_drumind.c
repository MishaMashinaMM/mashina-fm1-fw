/* GPL-3.0-only — DRUM-IND: twelve non-tonal industrial percussion textures. */
enum { IND_CLANG, IND_ANVIL, IND_PIPE, IND_SCRAPE, IND_RATTLE, IND_CHAIN,
       IND_CRUSH, IND_SQUISH, IND_STEAM, IND_WHOOSH, IND_GRIND, IND_STATIC, IND_NPAD };
static const char *const IND_NAMES[12] = {
    "CLANG C", "ANVIL C#", "PIPE D", "SCRAPE D#", "RATTLE E", "CHAIN F",
    "CRUSH F#", "SQUISH G", "STEAM G#", "WHOOSH A", "GRIND A#", "STATIC B"
};
static const char *const IND_FIELDS[8] = {
    "GRAIN", "DECAY", "WEIGHT", "STRIKE", "MOTION", "AIR", "CRUNCH", "TAIL"
};
typedef struct {
    uint32_t rng, age;
    int32_t lp, mid, slow, rough, amp, hit;
    uint8_t kind, active;
} ind_voice_t;
static ind_voice_t ind_state[NPART][NVOICE] __attribute__((section(".pool")));
static uint8_t ind_patch[NPART][12][8] __attribute__((section(".pool")));
static uint8_t ind_override[NPART][12] __attribute__((section(".pool")));
static uint8_t ind_ready[NPART];
static uint16_t ind_dirty[NPART];
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
/* Non-pitched noise profiles: low / mid / high filter coefficients, decay
 * in milliseconds, attack density. No sine oscillators or comb loops. */
static const uint16_t IND_PROFILE[12][5] = {
    {130,2100,11400,440,105}, {350,1150,8200,260,127},
    {220,1350,9300,360,83}, {120,4300,15900,520,77},
    {170,3700,13800,330,96}, {200,2400,11800,410,100},
    {430,1800,8600,240,127}, {520,1500,6500,470,65},
    {100,5700,20500,780,43}, {85,3200,17000,850,50},
    {250,5000,17500,610,88}, {120,8000,25000,380,114}
};
static void ind_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    uint32_t slot = (uint32_t)(v - t->v);
    if (slot >= NVOICE || !n) return;
    ind_voice_t *a = &ind_state[t - trk][slot];
    if (!a->active) return;
    uint32_t kind = a->kind;
    uint8_t *p = ind_patch_of(t, kind);
    if (!p) return;
    drumlab_global_ensure((uint32_t)(t - trk));
    int32_t kit[8];
    for (uint32_t k = 0; k < 8u; k++)
        kit[k] = (int32_t)drumlab_global[t - trk][k] - 100;
    int32_t grain=p[0], damp=p[1], weight=p[2], hit=p[3];
    int32_t motion=p[4], air=p[5], crunch=p[6], tail=p[7];
    int32_t low_k=clamp((int32_t)IND_PROFILE[kind][0]+weight*7-kit[0]*3,70,6000);
    int32_t mid_k=clamp((int32_t)IND_PROFILE[kind][1]+grain*17+kit[0]*40,250,16000);
    int32_t high_k=clamp((int32_t)IND_PROFILE[kind][2]+air*42+kit[4]*50,1200,30000);
    uint32_t duration=(uint32_t)clamp((int32_t)IND_PROFILE[kind][3]*1000+
                        (tail-64)*4100-(damp-64)*3500-kit[1]*2500+kit[6]*1200,
                        25000,1400000);
    uint32_t mul_b=dlab_decay_long(n,duration);
    uint32_t mul_t=dlab_decay_long(n,(uint32_t)clamp(26000+(127-hit)*170,19000,49000));
    int32_t amp=a->amp, trans=a->hit, lp=a->lp, mid=a->mid, slow=a->slow, rough=a->rough;
    int32_t step_b=((int32_t)(((int64_t)amp*mul_b)>>16)-amp)/(int32_t)n;
    int32_t step_t=((int32_t)(((int64_t)trans*mul_t)>>16)-trans)/(int32_t)n;
    uint32_t seed=a->rng, age=a->age;
    uint32_t lid=kind<8u?P_LN0+kind:P_LN8+kind-8u;
    int32_t lane=clamp(t->p[lid],0,127);
    int32_t gain=(int32_t)(((int64_t)lane*lane*32768*(100+kit[7]))/(16129*100));
    int32_t attack=clamp((int32_t)IND_PROFILE[kind][4]+hit+kit[3],0,320);
    int32_t texture=clamp(80+crunch+kit[5],0,320);
    int32_t bodymix=clamp(110+weight+kit[2],0,380);
    int32_t brightness=clamp(120+air+kit[4],0,380);
    for (uint32_t i=0; i<n; i++,age++,amp+=step_b,trans+=step_t) {
        int32_t white=(int32_t)(ind_random(&seed)>>16)-32768;
        int32_t impulse=(int32_t)(ind_random(&seed)>>16)-32768;
        slow+=(int32_t)(((int64_t)(white-slow)*low_k)>>15);
        lp+=(int32_t)(((int64_t)(white-lp)*mid_k)>>15);
        mid+=(int32_t)(((int64_t)(white-mid)*high_k)>>15);
        rough+=(int32_t)(((int64_t)(impulse-rough)*(1400+crunch*95))>>15);
        int32_t bass=slow, body=lp-slow, hiss=white-mid, grit=impulse-rough;
        int32_t pulse=256;
        if(kind==IND_SCRAPE||kind==IND_GRIND)
            pulse=135+(int32_t)(ind_random(&seed)&127u);
        else if(kind==IND_RATTLE||kind==IND_CHAIN||kind==IND_STATIC)
            pulse=(ind_random(&seed)&(kind==IND_STATIC?3u:7u))==0u?256:26+motion/2;
        else if(kind==IND_CRUSH) pulse=age<2000u?256:40;
        else if(kind==IND_STEAM||kind==IND_WHOOSH) {
            uint32_t up=kind==IND_STEAM?3800u:7100u;
            pulse=(int32_t)(age<up?age*256u/up:256u);
        } else if(kind==IND_SQUISH) pulse=180+(int32_t)(ind_random(&seed)&63u);
        int32_t mix;
        switch(kind) {
        case IND_CLANG:mix=body*2+hiss+grit;break;
        case IND_ANVIL:mix=bass*3+body*2+grit;break;
        case IND_PIPE:mix=bass+body*3+hiss/2;break;
        case IND_SCRAPE:mix=hiss*2+grit*2+body;break;
        case IND_RATTLE:mix=grit*3+hiss*2;break;
        case IND_CHAIN:mix=body+grit*3+hiss;break;
        case IND_CRUSH:mix=bass*2+grit*3;break;
        case IND_SQUISH:mix=bass*3+body*2-hiss;break;
        case IND_STEAM:mix=hiss*3+body;break;
        case IND_WHOOSH:mix=hiss*2+body*2;break;
        case IND_GRIND:mix=grit*3+body+hiss;break;
        default:mix=grit*2+hiss*3;break;
        }
        int64_t raw=((int64_t)mix*amp*pulse*texture)>>42;
        raw+=((int64_t)bass*amp*bodymix)>>33;
        raw+=((int64_t)hiss*amp*brightness)>>35;
        raw+=((int64_t)(hiss+grit)*trans*attack)>>33;
        raw=clamp(raw,-32768,32767);
        out[i]+=voice_amp((int32_t)((raw*gain)>>15),m,i);
    }
    a->lp=lp;a->mid=mid;a->slow=slow;a->rough=rough;
    a->rng=seed;a->age=age;
    a->amp=amp>0?amp:0;a->hit=trans>0?trans:0;
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
