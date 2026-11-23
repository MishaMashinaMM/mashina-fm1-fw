/* SPDX-License-Identifier: GPL-3.0-only
 * Full reference-based project SAVE/LOAD integration over simulated NOR.
 * Compile-time gates here are test-only: no physical FM1 is ever touched.
 */
#define FELUCCA_FLASH 1
#define FELUCCA_LIB32 1
#define FELUCCA_LIB32_WRITE 1
#define FELUCCA_PROJECT_REFS 1
#define FELUCCA_PROJECT_REFS_WRITE 1
#define FELUCCA_PROJECT_REFS_FORMAT 1 /* host NOR only, never a firmware release */
#define MASHINA_LIB_HOST_FLASH 1
#define MASHINA_MPRS_HOST_FLASH 1
#define main hostsim_main
#include "hostsim.c"
#undef main

static int32_t fm1_enc_take(uint32_t e){(void)e;return 0;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
static void lcd_sync(void){}
static void lcd_power(uint32_t s){(void)s;}
static void lcd_wake_now(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p)
{(void)x;(void)y;(void)w;(void)h;(void)p;}
#include "../firmware/src/gfx.c"
#include "../firmware/src/panel.c"
#include "../firmware/src/ui.c"
static void panel_setup(void){}

static uint8_t nor[0x100000],flash_ok=1;
static uint32_t erases,writes,outside;
static int st_read(uint32_t off,void *dst,uint32_t n)
{
    if(off>0x100000u || n>0x100000u-off)return -1;
    memcpy(dst,nor+off,n);return 0;
}
static int st_erase(uint32_t off)
{
    if(off>=0x100000u || (off&4095u)){outside++;return -1;}
    memset(nor+off,0xFF,4096);erases++;return 0;
}
static int st_prog(uint32_t off,const void *src,uint32_t n)
{
    if(off>=0x100000u || n>0x100000u-off){outside++;return -1;}
    const uint8_t *s=src;
    for(uint32_t i=0;i<n;i++)nor[off+i]&=s[i];
    writes++;return 0;
}
static uint32_t irq_save(void){return 0;}
static void irq_restore(uint32_t f){(void)f;}
static uint32_t fl_jedec_ram(void){return 0x856014u;}
static void fl_plain_window_init(void){}
#define FL_FAR(fn) (fn)
#include "../firmware/src/storage.c"
#include "../firmware/src/upreset.c"
static int lib_host_read(uint32_t off,void *dst,uint32_t n)
{
    if(off<0xEA000u || off>0xF2000u || n>0xF2000u-off){outside++;return -1;}
    return st_read(off,dst,n);
}
static int lib_host_erase(uint32_t off)
{
    if(off<0xEA000u || off>=0xF2000u || (off&4095u)){outside++;return -1;}
    return st_erase(off);
}
static int lib_host_prog(uint32_t off,const void *p,uint32_t n)
{
    if(off<0xEA000u || off>0xF2000u || n>0xF2000u-off ||
       (off&255u)+n>256u){outside++;return -1;}
    return st_prog(off,p,n);
}
#include "../firmware/src/preset_library_runtime.c"
/* The gated clean-break formatter references the watchdog before project.c
 * finishes including its helper modules. Declare the host stub first. */
static void fm1_wdt_feed(void){}
#include "../firmware/src/project.c"

/* Existing WebMIDI backup commands: chunked read/write of all 8xA-D MPR1
 * sections and independent USER/settings objects. */
enum { ED_BACKUP_LIST=65,ED_BACKUP_GET,ED_BACKUP_PUT };
static uint8_t ed_rep[4096];
static uint32_t ed_rep_n;
static void ed_b(uint32_t v)
{
    if(ed_rep_n<sizeof ed_rep)ed_rep[ed_rep_n++]=(uint8_t)(v&127u);
}
static uint32_t ed_unpack7(const uint8_t *a,uint32_t na,uint8_t *out,uint32_t max)
{
    uint32_t n=0;
    while(na && n<max){
        uint32_t mask=*a++;
        na--;
        uint32_t k=na>7u?7u:na;
        if(!k || n+k>max || (mask>>k))return 0;
        for(uint32_t j=0;j<k;j++,na--)
            out[n++]=(uint8_t)(*a++|(((mask>>j)&1u)<<7));
    }
    return na?0:n;
}
static uint8_t ed_smp_buf[512];
static int ed_flash_stop(void){return transport_busy();}
#include "../firmware/src/editor_backup.c"

static int check(const char *what,int yes)
{
    printf("MPR LIVE %-61s %s\n",what,yes?"PASS":"FAIL");
    return !yes;
}
int main(void)
{
    int bad=0;
    uint8_t saved[MPR_MAX_RECORD];
    /* The project/sound split MUST stay identical to the parameters kept
     * by a normal preset load. If a new engine adds project-only fields,
     * this regression forces an explicit MPR1 settings format review. */
    {
        int matched=1;
        for(uint32_t i=0;i<P_COUNT;i++)
            if(!!param_kept(i)!=!!mpr_is_project_param(i))
                matched=0;
        bad+=check("project-only parameter map equals actual sound-load retention",matched);
    }
    memset(nor,0xFF,sizeof nor);
    memset(&song,0,sizeof song);
    memset(trk,0,sizeof trk);
    memset(proj_slot,0,sizeof proj_slot);
    memset(up_bank,0,sizeof up_bank);
    settings_init();
    host_tracks_init();
    midi_zone_reset();
    for(uint32_t k=0;k<NTRK;k++)midi_rx[k]=(uint8_t)(k+1u);
    song.sel=0;
    set_engine_of(TSEL,0);
    TSEL->p[P_ATK]=49;
    {
        int r=up_store(0,"BASS ONE");
        bad+=check("create USER 001",(r==0||r==3)&&up_used(0));
    }
    bad+=check("assign USER 001 to track 1",!project_reference_load(0,0));
    bad+=check("assign built-in LIBRARY 033 to track 2",!project_reference_load(1,32));
    bad+=check("assign USER 001 to track 3",!project_reference_load(2,0));
    trk[3].p[P_MUTE]=0;
    bad+=check("active unassigned track cannot be silently lost on save",
               project_save_as(1,"SET B")==2 && !project_used(1));
    bad+=check("project SAVE error identifies active unreferenced track",
               ui.msg[0]=='T' && ui.msg[1]=='4');
    /* The NAME screen must remain open after a failed reference-only save.
     * Previously it closed while the chosen project section stayed EMPTY. */
    name_open(NK_PROJ_SAVE,1u);
    name_ok();
    bad+=check("failed project SAVE keeps the NAME editor open for correction",
               name_on() && !project_used(1));
    name_close();
    for(uint32_t k=3;k<NTRK;k++)trk[k].p[P_MUTE]=1;
    /* MPR1 must not silently discard an edited sound simply because its
     * track still carries the source USER slot. */
    trk[0].p[P_ATK]=51;
    bad+=check("project save refuses edited but unsaved USER sound",
               project_save_as(1,"SET B")==2 && !project_used(1));
    bad+=check("reloading USER 001 restores source-sound identity",
               !project_reference_load(0,0));
    trk[0].p[P_LEVEL]=90;
    trk[1].p[P_PAN]=-12;
    midi_transpose[0]=12;
    midi_fine[1]=-15;
    midi_zone[2][Z_NOTE_MIN]=45;
    midi_zone[2][Z_NOTE_MAX]=79;
    cm_count=1u;cm_rows[0][0]=72u;
    int res=project_save_as(1,"SET B");
    bad+=check("save section B with only six preset references",res==0&&project_used(1));
    bad+=check("project record has compact MPR1 magic",
               mpr_get32(proj_slot[1].raw)==MPR_MAGIC &&
               proj_slot[1].raw[8]==0 &&
               proj_slot[1].raw[9]==32 &&
               proj_slot[1].raw[10]==0 &&
               proj_slot[1].raw[11]==MPR_EMPTY);
    bad+=check("project record is at most 1824 bytes",mpr_get32(proj_slot[1].raw)==MPR_MAGIC &&
               (MPR_HEADER+(unsigned)(proj_slot[1].raw[6]|proj_slot[1].raw[7]<<8))<=MPR_MAX_RECORD);
    memcpy(saved,proj_slot[1].raw,sizeof saved);
    {
        uint8_t oldSlot=nor[0x99000u]; /* retired first-project B sector */
        live.dirty|=1u<<1u;
        bad+=check("dirty project flush uses only new compact A/B bank",
                   song_flush()==0 && nor[0x99000u]==oldSlot &&
                   !live.dirty && mpr_slot_used(1));
    }
    /* Persist across a simulated reboot/empty .noinit project cache. */
    {
        uint8_t retired=nor[0x99000u];
        live.dirty|=1u<<1u;
        song_poll();
        bad+=check("background song_poll never writes old FUN project sectors",
                   !live.dirty && nor[0x99000u]==retired &&
                   mpr_slot_used(1));
    }
    memset(proj_slot,0,sizeof proj_slot);
    proj_fetch(1);
    bad+=check("saved project survives reboot in atomic NOR bank",
               !memcmp(saved,proj_slot[1].raw,sizeof saved));
    song.sel=0;
    set_engine_of(TSEL,0);
    TSEL->p[P_ATK]=70;
    {
        int r=up_store(0,"BASS ONE");
        bad+=check("update shared USER 001 preset",r==0||r==3);
    }
    trk[0].p[P_LEVEL]=12;
    trk[1].p[P_PAN]=36;
    midi_transpose[0]=0;
    midi_fine[1]=0;
    cm_count=0;
    project_load(1);
    bad+=check("changed preset updates all referenced project tracks",
               trk[0].p[P_ATK]==70 &&
               trk[2].p[P_ATK]==70 &&
               trk[0].user==1 && trk[2].user==1);
    bad+=check("independent project mix and tuning remain unchanged",
               trk[0].p[P_LEVEL]==90 &&
               trk[1].p[P_PAN]==-12 &&
               midi_transpose[0]==12 &&
               midi_fine[1]==-15);
    bad+=check("project-specific CC macros, MIDI zones remain unchanged",
               cm_count==1 && cm_rows[0][0]==72 &&
               midi_zone[2][Z_NOTE_MIN]==45 &&
               midi_zone[2][Z_NOTE_MAX]==79);
    bad+=check("unassigned tracks are always muted",
               trk[3].p[P_MUTE]==1 &&
               trk[4].p[P_MUTE]==1 &&
               trk[5].p[P_MUTE]==1);
    char name[PROJ_NAME_LEN+1u];
    bad+=check("project name comes from new record",
               project_name(1,name) && !strcmp(name,"SET B"));
    bad+=check("rename changes name but retains preset references",
               !project_rename(1,"SET NEW") && project_name(1,name) &&
               !strcmp(name,"SET NEW") &&
               proj_slot[1].raw[8]==0 && proj_slot[1].raw[9]==32);
    memset(proj_slot,0,sizeof proj_slot);
    proj_fetch(1);
    bad+=check("renamed project survives reboot",
               project_name(1,name)&&!strcmp(name,"SET NEW"));
    /* WebMIDI full-backup has a fixed 1824-byte project object in both the
     * current section aliases and 40..71 global project addresses. */
    {
        uint32_t length=0;
        const uint8_t *body;
        bad+=check("capture current live MPR1 state via existing WebMIDI backup",
                   ed_bk_capture()==0 &&
                   ed_bk_object(0,&length)==ED_BK_RAW &&
                   length==MPR_MAX_RECORD);
        body=ed_bk_object(41u,&length);
        bad+=check("backup ID 41 exports project 1 section B MPR1 record",
                   body && length==MPR_MAX_RECORD &&
                   !memcmp(body,proj_slot[1].raw,MPR_MAX_RECORD));
        bad+=check("old FUN snapshots are forbidden on clean-break imports",
                   !proj_store_len(PROJ_STORE_SIZE) &&
                   proj_store_len(MPR_MAX_RECORD));
        memcpy(ED_BK_RAW,proj_slot[1].raw,MPR_MAX_RECORD);
        ed_bk_id=41u;ed_bk_len=MPR_MAX_RECORD;
        ed_bk_pos=ed_bk_len;ed_bk_crc=st_crc32(ED_BK_RAW,ed_bk_len);
        bad+=check("backup PUT commits a validated reference-only section",
                   ed_bk_commit()==0);
        ED_BK_RAW[37]^=1u;
        ed_bk_crc=st_crc32(ED_BK_RAW,ed_bk_len);
        bad+=check("backup PUT refuses damaged MPR1 data even with outer CRC",
                   ed_bk_commit()==2u);
        ED_BK_RAW[37]^=1u;
        ed_bk_crc=st_crc32(ED_BK_RAW,ed_bk_len);
        ed_bk_id=0u;
        bad+=check("restore live unsaved MPR1 state without changing a saved slot",
                   ed_bk_commit()==0u && project_used(1));
    }
    /* RESTORE LAST must use the same six preset references as a manual
     * project save, never reintroduce the legacy embedded-sound snapshot.
     * The host NOR sim uses the real autosave storage sector pair. */
    {
        uint8_t baseline[MPR_MAX_RECORD],after[MPR_MAX_RECORD];
        int n;
        ui_prefs |= PREF_RESTORE_ON;
        for(uint32_t k=0;k<NTRK;k++)
            for(uint32_t v=0;v<NVOICE;v++)trk[k].v[v].active=0;
        fm1_in.notes=0;
        fm1_in.buttons=0;
        song.playing=0;
        bad+=check("prepare reference sources for compact autosave",
                   !project_reference_load(0,0u) &&
                   !project_reference_load(1,UP_SLOTS) &&
                   !project_reference_load(2,0u));
        for(uint32_t k=3;k<NTRK;k++)trk[k].user=0;
        trk[0].p[P_LEVEL]=73;
        trk[1].p[P_PAN]=-17;
        midi_transpose[0]=19;
        fm1_ms=100000u;
        autosave_poll();                  /* first observation, no write */
        fm1_ms=111000u;                   /* 11 s of silence and stable state */
        autosave_poll();
        n=st_load(OBJ_AUTOSAVE,baseline,sizeof baseline);
        bad+=check("reference-only idle autosave persists fixed MPR1 record",
                   n==MPR_MAX_RECORD &&
                   mpr_get32(baseline)==MPR_MAGIC &&
                   baseline[8]==0 && baseline[9]==UP_SLOTS &&
                   as.writes>=1u);
        /* An unsaved tone edit must NEVER replace the last good autosave:
         * this would otherwise silently serialize an inaccurate reference. */
        trk[0].p[P_ATK]=(int16_t)(trk[0].p[P_ATK]+1);
        fm1_ms=122000u;
        autosave_poll();
        fm1_ms=133000u;
        autosave_poll();
        n=st_load(OBJ_AUTOSAVE,after,sizeof after);
        bad+=check("unsaved sound edit cannot overwrite reference-only autosave",
                   n==MPR_MAX_RECORD && !memcmp(after,baseline,sizeof after));
        trk[0].p[P_LEVEL]=8;
        trk[1].p[P_PAN]=33;
        midi_transpose[0]=0;
        bad+=check("RESTORE LAST restores references and independent settings",
                   autosave_boot(1)==1 && trk[0].user==1u &&
                   trk[1].user==UP_SLOTS+1u &&
                   trk[0].p[P_LEVEL]==73 &&
                   trk[1].p[P_PAN]==-17 &&
                   midi_transpose[0]==19);
        trk[0].p[P_LEVEL]=35;
        bad+=check("crash/unsafe boot must never restore last performance",
                   autosave_boot(0)==0 && trk[0].p[P_LEVEL]==35);
    }
    bad+=check("clear the new project without touching presets",!project_clear_slot(1));
    bad+=check("cleared project remains empty after reboot",({
        memset(proj_slot,0,sizeof proj_slot);proj_fetch(1);!project_used(1);
    }));
    bad+=check("shared USER 001 remains usable after project clear",up_used(0));
    bad+=check("all project writes confined to simulated 1MiB flash",!outside);
    /* The new reference storage reuses former SONG 2..5 flash sectors. An
     * explicit fresh-start operation may erase only CRC-verified old records
     * or entirely blank sectors, never alien data or valid MPR1 projects. */
    {
        uint32_t prior=writes;
        bad+=check("first-install initializer refuses already-initialized MPR1 sectors",
                   mprs_init_clean_break()==1 && writes==prior);
        memset(nor+MPRS_BASE,0xFF,MPRS_BANKS*2u*MPRS_SECTOR);
        uint8_t legacy[16]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
        bad+=check("prepare CRC-validated legacy project section",
                   !st_save(OBJ_SONG1+3,legacy,sizeof legacy));
        uint8_t userBefore[sizeof up_bank[0]];
        memcpy(userBefore,&up_bank[0],sizeof userBefore);
        uint8_t originalOther=nor[0x97000u];
        bad+=check("explicit initializer clears verified OLD song sector",
                   mprs_init_clean_break()==0 && mprs_blank(3));
        bad+=check("initialization does not modify USER sound bank",
                   !memcmp(userBefore,&up_bank[0],sizeof userBefore));
        bad+=check("initialization does not touch old first-project sector",
                   nor[0x97000u]==originalOther);
        nor[MPRS_BASE+4095u]=0x33;
        prior=writes;
        bad+=check("unknown nonblank flash is never silently initialized",
                   mprs_init_clean_break()==1 && writes==prior &&
                   nor[MPRS_BASE+4095u]==0x33);
        nor[MPRS_BASE+4095u]=0xFF;
        uint8_t key[4]={0x4d,0x50,0x52,0x31};
        bad+=check("clean-break format is idempotent on erased project banks",
                   !mprs_init_clean_break());
    }
    printf("MPR LIVE: %s (NOR erases=%u writes=%u)\n",bad?"FAIL":"ALL TESTS PASSED",erases,writes);
    return !!bad;
}
