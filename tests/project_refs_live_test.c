/* SPDX-License-Identifier: GPL-3.0-only
 * Full reference-based project SAVE/LOAD integration over simulated NOR.
 * Compile-time gates here are test-only: no physical FM1 is ever touched.
 */
#define FELUCCA_FLASH 1
#define FELUCCA_LIB32 1
#define FELUCCA_LIB32_WRITE 1
#define FELUCCA_PROJECT_REFS 1
#define FELUCCA_PROJECT_REFS_WRITE 1
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
static void fm1_wdt_feed(void){}
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
    /* Persist across a simulated reboot/empty .noinit project cache. */
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
    bad+=check("clear the new project without touching presets",!project_clear_slot(1));
    bad+=check("cleared project remains empty after reboot",({
        memset(proj_slot,0,sizeof proj_slot);proj_fetch(1);!project_used(1);
    }));
    bad+=check("shared USER 001 remains usable after project clear",up_used(0));
    bad+=check("all project writes confined to simulated 1MiB flash",!outside);
    printf("MPR LIVE: %s (NOR erases=%u writes=%u)\n",bad?"FAIL":"ALL TESTS PASSED",erases,writes);
    return !!bad;
}
