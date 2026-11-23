/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental MPR1 clean-break project SAVE/RECALL integration.
 * Included after project_capture/random_saved_live in project.c, only when
 * FELUCCA_PROJECT_REFS=1. Device flash writes remain separately locked.
 * Snapshot sound engines / FM6 voices / drum kit patches are NEVER written
 * into project records. The six byte identities are the sound authority.
 */
/* A project stores a PRESET REFERENCE, not uncommitted edits to the
 * currently loaded sound. Remember the exact live sound at preset load/save
 * and refuse PROJECT SAVE if it was edited without first saving the preset.
 * Project-only mix, zones, pad mappings etc are not part of this fingerprint.
 */
static uint32_t mpr_origin_hash[NTRK];
static uint8_t mpr_origin_valid[NTRK];
static uint32_t mpr_hash_bytes(uint32_t h,const void *src,uint32_t n)
{
    const uint8_t *b=src;
    while(n--){h^=*b++;h*=16777619u;}
    return h;
}
static uint32_t mpr_current_sound_hash(const track_t *t)
{
    uint32_t k=(uint32_t)(t-trk),h=2166136261u;
    if(k>=NTRK)return 0;
    h=mpr_hash_bytes(h,&t->eng_req,sizeof t->eng_req);
    for(uint32_t i=0;i<P_COUNT;i++)
        if(!mpr_is_project_param(i)) {
            int16_t v=motion_base_value(t,i);
            h=mpr_hash_bytes(h,&v,sizeof v);
        }
    if(t->eng_req==ENGI_FM6)
        h=mpr_hash_bytes(h,fm6_patch[k],sizeof fm6_patch[k]);
    if(t->eng_req==ENGI_DRUM){
        h=mpr_hash_bytes(h,dx_kit,sizeof dx_kit);
        h=mpr_hash_bytes(h,dx_kit12,sizeof dx_kit12);
    }
    if(t->eng_req==ENGI_DRUMLAB) {
        drumlab_kit_ensure((track_t *)t);
        h=mpr_hash_bytes(h,drumlab_kit[k],sizeof drumlab_kit[k]);
        h=mpr_hash_bytes(h,&drumlab_kit_dirty[k],sizeof drumlab_kit_dirty[k]);
    }
    if(t->eng_req==ENGI_DRUMAC) {
        ac_patch_ensure((track_t *)t);
        h=mpr_hash_bytes(h,ac_patch[k],sizeof ac_patch[k]);
        h=mpr_hash_bytes(h,ac_override[k],sizeof ac_override[k]);
        h=mpr_hash_bytes(h,&ac_dirty[k],sizeof ac_dirty[k]);
    }
    if(t->eng_req==ENGI_DRUMIND) {
        ind_patch_ensure((track_t *)t);
        h=mpr_hash_bytes(h,ind_patch[k],sizeof ind_patch[k]);
        h=mpr_hash_bytes(h,ind_override[k],sizeof ind_override[k]);
        h=mpr_hash_bytes(h,&ind_dirty[k],sizeof ind_dirty[k]);
    }
    return h;
}
static void mpr_capture_source(const track_t *t)
{
    uint32_t k=(uint32_t)(t-trk);
    if(k>=NTRK)return;
    mpr_origin_valid[k]=t->user>=1u && t->user<=64u;
    mpr_origin_hash[k]=mpr_origin_valid[k]?mpr_current_sound_hash(t):0u;
}
static int mpr_source_modified(const track_t *t)
{
    uint32_t k=(uint32_t)(t-trk);
    return k>=NTRK || !mpr_origin_valid[k] ||
           mpr_current_sound_hash(t)!=mpr_origin_hash[k];
}
static uint32_t mpr_global_section(uint32_t section)
{
    return (uint32_t)song_cur*4u+(section&3u);
}
static int mpr_open_blob(const uint8_t *r,uint32_t section,
                         uint8_t refs[NTRK],uint8_t name[PROJ_NAME_LEN],
                         project_t *p)
{
    uint32_t version_section=0,n=0,len;
    if(!r || section>=4u ||
       mpr_get32(r)!=MPR_MAGIC || r[4]!=MPR_VERSION)return 0;
    len=MPR_HEADER+((uint32_t)r[6]|((uint32_t)r[7]<<8));
    if(len>MPR_MAX_RECORD)return 0;
    /* Backup records are fixed 1824 bytes and zero-padded. Ensure a stale
     * or malicious hidden tail can never survive an import unnoticed. */
    for(uint32_t i=len;i<MPR_MAX_RECORD;i++)
        if(r[i])return 0;
    if(!mpr_decode(r,len,&version_section,refs,name,st_buf,
                   MPR_MAX_SETTINGS,&n) || version_section!=section)return 0;
    project_t *q=p?p:&proj_scratch;
    memset(q,0,sizeof *q);
    if(!mpr_project_settings_decode(q,st_buf,n))return 0;
    q->magic=PROJ_MAGIC;q->size=sizeof *q;
    memcpy(q->name,name,PROJ_NAME_LEN);
    q->sum=proj_sum(q);
    return 1;
}
static int mpr_cached(uint32_t section,uint8_t refs[NTRK],
                      uint8_t name[PROJ_NAME_LEN],project_t *p)
{
    if(section>=4u)return 0;
    return mpr_open_blob(proj_slot[section].raw,section,refs,name,p);
}
static int mpr_slot_used(uint32_t section)
{
    uint8_t refs[NTRK],name[PROJ_NAME_LEN];
    return mpr_cached(section,refs,name,0);
}
static int mpr_pack_live(const project_t *p,uint32_t section,
                         const uint8_t refs[NTRK],const uint8_t name[PROJ_NAME_LEN])
{
    uint8_t *r=proj_wire_u.raw;
    uint32_t n=mpr_project_settings_encode(p,r+MPR_HEADER,MPR_MAX_SETTINGS);
    int len;
    if(!n)return 0;
    /* Settings are already at their destination: mpr_encode is in-place. */
    len=mpr_encode(r,section,refs,name,r+MPR_HEADER,n);
    if(len<=0 || (uint32_t)len>MPR_MAX_RECORD)return 0;
    memset(r+len,0,MPR_MAX_RECORD-(uint32_t)len);
    return 1;
}
static int mpr_commit(uint32_t section)
{
    if(section>=4u)return 2;
#if FELUCCA_FLASH && FELUCCA_PROJECT_REFS_WRITE
    if(flash_ok && mprs_save(mpr_global_section(section),proj_wire_u.raw))
        return 2;
#endif
    /* On write-locked builds this is intentionally RAM-only (no persistence). */
    memcpy(proj_slot[section].raw,proj_wire_u.raw,MPR_MAX_RECORD);
    proj_wire_gen++;
    return 0;
}
/* A non-destructive, byte-exact view of the current *unsaved* performance:
 * the existing backup command's ID 0. It never commits to a project slot. */
static int mpr_capture_unsaved(uint32_t section)
{
    uint8_t refs[NTRK],name[PROJ_NAME_LEN]={0};
    if(section>=4u || transport_busy())return 0;
    for(uint32_t k=0;k<NTRK;k++){
        uint32_t u=trk[k].user;
        refs[k]=(uint8_t)(u>=1u && u<=64u?u-1u:MPR_EMPTY);
        /* Only muted/inactive tracks may be unassigned. An audible track
         * without a preset reference has a live sound that MPR1 cannot
         * represent; refuse a misleading backup instead of losing it. */
        if(refs[k]==MPR_EMPTY && k<trk_live && !trk[k].p[P_MUTE])return 0;
        if(refs[k]!=MPR_EMPTY && mpr_source_modified(&trk[k]))return 0;
        if(refs[k]<UP_SLOTS && !up_used(refs[k]))return 0;
        if(refs[k]>=UP_SLOTS && refs[k]!=MPR_EMPTY &&
           library_slot_ready(refs[k]-UP_SLOTS))return 0;
    }
    for(uint32_t k=0;k<PROJ_NAME_LEN && proj_name[k];k++)
        name[k]=(uint8_t)proj_name[k];
    project_capture(&proj_scratch);
    return mpr_pack_live(&proj_scratch,section,refs,name);
}
static int mpr_live_save(uint32_t section,const char *rename)
{
    uint8_t refs[NTRK],name[PROJ_NAME_LEN]={0};
    project_t *p=&proj_scratch;
    const char *name_source=rename?rename:proj_name;
    if(section>=4u || transport_busy())return 2;
    for(uint32_t k=0;k<NTRK;k++){
        uint32_t u=trk[k].user;
        refs[k]=(uint8_t)(u>=1u && u<=64u?u-1u:MPR_EMPTY);
        if(refs[k]==MPR_EMPTY && k<trk_live && !trk[k].p[P_MUTE]){
            char msg[] = "T1: ASSIGN PRESET";
            msg[1] = (char)('1' + k);
            ui_message(msg);return 2;
        }
        if(refs[k]!=MPR_EMPTY && mpr_source_modified(&trk[k])){
            char msg[] = "T1: SAVE SOUND FIRST";
            msg[1] = (char)('1' + k);
            ui_message(msg);return 2;
        }
        if(refs[k]<UP_SLOTS && !up_used(refs[k])){
            ui_message("MISSING USER PRESET");return 2;
        }
        if(refs[k]>=UP_SLOTS && refs[k]!=MPR_EMPTY &&
           library_slot_ready(refs[k]-UP_SLOTS)){
            ui_message("LIBRARY PRESET ERROR");return 2;
        }
    }
    for(uint32_t k=0;k<PROJ_NAME_LEN && name_source[k];k++)
        name[k]=(uint8_t)name_source[k];
    project_capture(p);
    if(!mpr_pack_live(p,section,refs,name)){
        ui_message("SECTION FULL");return 2;
    }
    if(mpr_commit(section)){
        ui_message("SAVE ERROR");return 2;
    }
    memcpy(proj_name,name,PROJ_NAME_LEN);
    proj_name[PROJ_NAME_LEN]=0;
    proj_cur=(uint8_t)section;
    live.cur=(int8_t)section;
    live.dirty&=(uint8_t)~(1u<<section);
    random_saved_live(section);
#if FELUCCA_FLASH && FELUCCA_PROJECT_REFS_WRITE
    ui_message(flash_ok?"SAVED":"SAVED (RAM ONLY)");
#else
    ui_message("SAVED (RAM ONLY)");
#endif
    return 0;
}
static int mpr_live_clear(uint32_t section)
{
    if(section>=4u || transport_busy())return 1;
    memset(proj_wire_u.raw,0,MPR_MAX_RECORD);
    if(mpr_commit(section)){ui_message("CLEAR ERROR");return 1;}
    live.dirty&=(uint8_t)~(1u<<section);
    if(proj_cur==section)proj_cur=PROJ_NO_SLOT;
    ui_message("PROJECT SLOT CLEARED");
    ui.force=1;
    return 0;
}
static int mpr_live_name(uint32_t section,char *out)
{
    uint8_t refs[NTRK],name[PROJ_NAME_LEN];
    out[0]=0;
    if(!mpr_cached(section,refs,name,0))return 0;
    memcpy(out,name,PROJ_NAME_LEN);
    out[PROJ_NAME_LEN]=0;
    return 1;
}
static int mpr_live_rename(uint32_t section,const char *new_name)
{
    project_t *p=&proj_scratch;
    uint8_t refs[NTRK],name[PROJ_NAME_LEN]={0};
    if(transport_busy()){ui_message("STOP TO RENAME");return 1;}
    if(!mpr_cached(section,refs,name,p)){ui_message("EMPTY SLOT");return 1;}
    memset(name,0,sizeof name);
    for(uint32_t k=0;k<PROJ_NAME_LEN && new_name[k];k++)name[k]=(uint8_t)new_name[k];
    if(!mpr_pack_live(p,section,refs,name)){ui_message("SECTION FULL");return 2;}
    if(mpr_commit(section)){ui_message("SAVE ERROR");return 2;}
    if(proj_cur==section){
        memcpy(proj_name,name,PROJ_NAME_LEN);
        proj_name[PROJ_NAME_LEN]=0;
    }
    ui_message("RENAMED");
    return 0;
}
/* Apply only project-level settings AFTER the shared preset sounds load.
 * No sound engine selection, tone parameters, FM6 or full drum-kit patches
 * are overwritten by this function. */
static void mpr_apply_project_settings(const project_t *p,const uint8_t refs[NTRK])
{
    uint32_t k,i;
    transport_req=2;
    panic_req=(1u<<NTRK)-1u;
    seq_stop();
    transport_req=0;
    for(k=0;k<NTRK;k++)
        lyr_set(k,p->lyr[k]);
    fm1_irq_off();
    for(i=0;i<3u;i++)global_eq_gain[i]=p->eq_gain[i];
    global_eq_on=p->eq_on;
    memcpy(midi_rx,p->rx,sizeof p->rx);
    memcpy(midi_transpose,p->transpose,sizeof midi_transpose);
    memcpy(midi_fine,p->fine,sizeof midi_fine);
    memcpy(drum_track_delay,p->drum_timing,sizeof drum_track_delay);
    memcpy(midi_zone,p->zone,sizeof midi_zone);
    cm_count=p->cm_count;
    memcpy(cm_rows,p->cm_rows,sizeof cm_rows);
    memcpy(macro_v,p->macro,sizeof macro_v);
    memcpy(pfx_lane,p->pfx_lane,sizeof pfx_lane);
    pfx_ltgt=p->pfx_ltgt;
    dx_mute_set(p->dx_mute);
    for(i=0;i<G_COUNT;i++)
        if(i!=G_SLOT && i!=G_LOAD && i!=G_SAVE)
            song.g[i]=(int16_t)clamp(p->g[i],GP[i].min,GP[i].max);
    for(k=0;k<NTRK;k++){
        track_t *t=&trk[k];
        uint32_t e=t->eng_req;
        for(i=0;i<P_COUNT;i++)
            if(mpr_is_project_param(i))
                t->p[i]=(int16_t)param_fit(param_desc_of(e,i),p->t[k].p[i]);
        memcpy(drum_pad_pan[k],p->pad_pan[k],12u);
        memcpy(drum_pad_pitch[k],p->pad_pitch[k],12u);
        memcpy(drum_pad_delay[k],p->pad_timing[k],12u);
        memcpy(drum_perf_curve[k],p->perf_curve[k],12u);
        memcpy(drum_perf_sound[k],p->perf_sound[k],12u);
        memcpy(drum_perf_choke[k],p->perf_choke[k],12u);
        memcpy(drum_retrigger[k],p->perf_retrigger[k],12u);
        drum_midi_custom[k]=(uint8_t)(e==ENGI_DRUM && p->dx_note_on[k]);
        if(drum_midi_custom[k])memcpy(drum_midi_note[k],p->dx_note[k],12u);
        drumlab_midi_custom[k]=(uint8_t)((e==ENGI_DRUMLAB || e==ENGI_DRUMAC ||
                                          e==ENGI_DRUMIND) && p->lab_note_on[k]);
        if(drumlab_midi_custom[k])memcpy(drumlab_midi_note[k],p->lab_note[k],12u);
        if(refs[k]==MPR_EMPTY)t->p[P_MUTE]=1; /* no stale sound from prior section */
    }
    song.sel=p->sel;
    fm1_irq_on();
    lfo_fade_restart((1u<<NTRK)-1u);
    sync_reload=1;
    ui.force=1;
}
static int mpr_live_load(uint32_t section)
{
    project_t *p=&proj_scratch;
    uint8_t refs[NTRK],name[PROJ_NAME_LEN];
    if(!mpr_cached(section,refs,name,p)){
        ui_message("EMPTY OR BAD SECTION");return 1;
    }
    if(project_references_load_all(refs)){
        ui_message("MISSING OR BAD PRESET");return 1;
    }
    mpr_apply_project_settings(p,refs);
    memcpy(proj_name,name,PROJ_NAME_LEN);
    proj_name[PROJ_NAME_LEN]=0;
    proj_cur=(uint8_t)section;
    live.cur=(int8_t)section;
    ui_message("LOADED");
    return 0;
}
/* Restore backup ID 0 into the current live performance, without changing
 * any of the eight projects' 32 stored sections. Must be done AFTER presets. */
static int mpr_restore_unsaved(const uint8_t raw[MPR_MAX_RECORD])
{
    uint8_t refs[NTRK],name[PROJ_NAME_LEN];
    uint32_t section=raw?raw[5]:4u;
    if(section>=4u || !mpr_open_blob(raw,section,refs,name,&proj_scratch))
        return 1;
    if(project_references_load_all(refs))return 1;
    mpr_apply_project_settings(&proj_scratch,refs);
    memcpy(proj_name,name,PROJ_NAME_LEN);
    proj_name[PROJ_NAME_LEN]=0;
    proj_cur=PROJ_NO_SLOT; /* restored live sound, not a stored section */
    ui.force=1;
    return 0;
}
static int mpr_live_recall_sound(uint32_t section,uint32_t track)
{
    uint8_t refs[NTRK],name[PROJ_NAME_LEN];
    if(track>=NTRK || !mpr_cached(section,refs,name,0) ||
       refs[track]==MPR_EMPTY)return 1;
    return project_reference_load(track,refs[track]);
}
