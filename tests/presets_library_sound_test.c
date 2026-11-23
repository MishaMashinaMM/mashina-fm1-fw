/* SPDX-License-Identifier: GPL-3.0-only
 * 32/32 real sound-loader integration on a simulated NOR.
 * Includes the actual firmware UI and sound engine; no physical flash.
 */
#define FELUCCA_LIB32 1
#define MASHINA_LIB_HOST_FLASH 1
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static uint8_t lib_sim_nor[0x100000];
static uint32_t lib_sim_outside;
static int lib_host_read(uint32_t off, void *out, uint32_t n)
{
    if (off < 0xEA000u || off + n > 0xF2000u) return 1;
    memcpy(out,lib_sim_nor+off,n);
    return 0;
}
static int lib_host_erase(uint32_t off)
{
    if (off < 0xEA000u || off + 4096u > 0xF2000u || (off & 4095u)) {
        lib_sim_outside++;return 1;
    }
    memset(lib_sim_nor+off,0xFF,4096);
    return 0;
}
static int lib_host_prog(uint32_t off,const void *p,uint32_t n)
{
    const uint8_t *src=p;
    if (off < 0xEA000u || off + n > 0xF2000u || (off&255u)+n>256u) {
        lib_sim_outside++;return 1;
    }
    for(uint32_t i=0;i<n;i++)lib_sim_nor[off+i]&=src[i];
    return 0;
}
#include "../firmware/src/preset_library_runtime.c"

int main(void)
{
    uint8_t blob[LIB_ENTRY], savedAnalog[LIB_ENTRY], erased[LIB_ENTRY], before[sizeof(up_rec_t)], kit[UPF_PK], after[UPF_PK];
    uint32_t selected;
    int bad=0;
    memset(lib_sim_nor,0xFF,sizeof lib_sim_nor);
    ui_power_on();
    song.sel=0;
    set_engine_of(TSEL,0u);
    TSEL->p[P_ATK]=49;
    bad+=check("USER 1 sound can be stored independently before library capture",
               up_store(0,"LIB ONE")==3 && up_used(0));
    memcpy(before,up_rec(0),sizeof before);
    memset(blob,0,sizeof blob);
    memcpy(blob,up_rec(0),sizeof before);
    bad+=check("put USER 1 into LIBRARY 33 without mutating USER 1",
               lib_slot_write(0,blob)==0&&!memcmp(before,up_rec(0),sizeof before));
    TSEL->p[P_ATK]=9;
    selected=song.sel;
    preset_go(UP_SLOTS);
    bad+=check("PRESETS loads LIBRARY 33 from flash into active track",
               TSEL->p[P_ATK]==49 && TSEL->user==UP_SLOTS+1u && song.sel==selected);
    {
        char name[16];
        library_track_name(TSEL,name);
        bad+=check("uploaded library sound name appears without reading flash again",
                   !strcmp(name,"LIB ONE"));
    }
    bad+=check("original editable USER 1 remains byte-identical",
               !memcmp(before,up_rec(0),sizeof before));
    /* PC32 targets track 2 without changing track selection or track 1. */
    {
        int16_t old=TSEL->p[P_ATK];
        selected=song.sel;
        trk[1].p[P_ATK]=7;
        midi_pc_req[1]=UP_SLOTS+1u;
        midi_pc_poll();
        bad+=check("PC32 loads LIBRARY 33 into track 2 without moving selection",
                   trk[1].p[P_ATK]==49 && trk[1].user==UP_SLOTS+1u && song.sel==selected);
        bad+=check("track 1 remains independent",TSEL->p[P_ATK]==old);
    }
    /* Drum-AC: complete 12-pad sidecar is an independent part of record. */
    set_engine_of(TSEL,ENGI_DRUMAC);
    TSEL->engine=TSEL->eng_req;
    ac_patch_set(TSEL,AC_KICK,0u,37u);
    ac_patch_set(TSEL,AC_SNARE,4u,84u);
    bad+=check("USER 2 captures independent complete acoustic drum kit",up_store(1,"KIT TWO")==3);
    bad+=check("DRUM-AC companion is present and tagged",!upd_companion_get(1,kit));
    memset(blob,0,sizeof blob);
    memcpy(blob,up_rec(1),sizeof(up_rec_t));
    memcpy(blob+sizeof(up_rec_t),kit,UPF_PK);
    bad+=check("put full DRUM-AC kit into LIBRARY 34",lib_slot_write(1,blob)==0);
    ac_patch_set(TSEL,AC_KICK,0u,2u);
    ac_patch_set(TSEL,AC_SNARE,4u,3u);
    preset_go(UP_SLOTS+1u);
    bad+=check("LIBRARY 34 restores complete DRUM-AC pad sounds",
               TSEL->eng_req==ENGI_DRUMAC && ac_patch[TSEL-trk][AC_KICK][0]==37u &&
               ac_patch[TSEL-trk][AC_SNARE][4]==84u);
    bad+=check("USER 2 kit remains unchanged after LIBRARY playback",
               !upd_companion_get(1,after)&&!memcmp(kit,after,UPF_PK));
    bad+=check("library data writes never touch the 32 existing USER slots or projects",
               lib_sim_outside==0);
    /* Simulate an updater erasing the entire transient LIBRARY region. This
     * must not erase USER data; built-in factory fallback remains playable. */
    bad+=check("capture exact binary library sound before upgrade",
               lib_slot_read(0,savedAnalog)==0);
    memset(lib_sim_nor+LIB_BASE,0xFF,LIB_BANKS*2u*LIB_SECTOR);
    preset_go(UP_SLOTS);
    bad+=check("after update erase, LIBRARY 33 falls back to compiled factory sound",
               TSEL->user==UP_SLOTS+1u && lib_slot_read(0,erased)==0 && !memcmp(erased,(uint8_t[LIB_ENTRY]){0},LIB_ENTRY));
    bad+=check("restore melodic LIBRARY 33 from saved offline record",
               lib_slot_write(0,savedAnalog)==0);
    preset_go(UP_SLOTS);
    bad+=check("melodic sound returns after offline library restore",
               TSEL->p[P_ATK]==49 && TSEL->user==UP_SLOTS+1u);
    bad+=check("restore USER-derived LIBRARY 34 from offline binary JSON record",
               lib_slot_write(1,blob)==0);
    preset_go(UP_SLOTS+1u);
    bad+=check("DRUM-AC sound returns after update and offline restoration",
               TSEL->eng_req==ENGI_DRUMAC && ac_patch[TSEL-trk][AC_KICK][0]==37u &&
               ac_patch[TSEL-trk][AC_SNARE][4]==84u);
    bad+=check("update/restore preserves original USER 1 record",
               !memcmp(before,up_rec(0),sizeof before));
    /* Reference-based sections resolve six slot identities without writing
     * USER or LIBRARY flash, and keep the player's selected track unchanged. */
    {
        uint32_t chosen = song.sel;
        uint8_t sound_before[sizeof(up_rec_t)];
        uint32_t first_track_engine = trk[0].eng_req;
        memcpy(sound_before, up_rec(0), sizeof sound_before);
        bad+=check("project reference USER 001 loads into track 3",
                   !project_reference_load(2,0u) &&
                   trk[2].user==1u && trk[2].p[P_ATK]==49 && song.sel==chosen);
        bad+=check("project reference LIBRARY 034 loads into track 4",
                   !project_reference_load(3,UP_SLOTS+1u) &&
                   trk[3].user==UP_SLOTS+2u && trk[3].eng_req==ENGI_DRUMAC &&
                   song.sel==chosen);
        bad+=check("project reference does not alter another track",
                   trk[0].eng_req==first_track_engine);
        bad+=check("missing USER 032 is refused without changing selection",
                   project_reference_load(4,31u)==1 && song.sel==chosen);
        bad+=check("out-of-range reference and track are refused",
                   project_reference_load(NTRK,0u)==1 &&
                   project_reference_load(0,64u)==1);
        bad+=check("reference recall never mutates stored USER data",
                   !memcmp(sound_before,up_rec(0),sizeof sound_before));
        /* Whole-section preflight refuses a missing USER sound BEFORE any
         * of the earlier tracks are modified. */
        {
            uint8_t refs[NTRK] = {0u, UP_SLOTS+1u, 255u, 31u, 0u, 255u};
            uint8_t prev = trk[0].user;
            bad+=check("invalid section reference preflight prevents partial recall",
                       project_references_load_all(refs)==1 &&
                       trk[0].user==prev && song.sel==chosen);
            refs[3]=0u;
            bad+=check("valid section references restore multiple tracks without moving selection",
                       project_references_load_all(refs)==0 &&
                       trk[0].user==1u && trk[1].user==UP_SLOTS+2u &&
                       trk[3].user==1u && song.sel==chosen);
        }
        /* Erase the simulated LIBRARY bank: the same slot ID now resolves
         * through its built-in factory sound, not a dangling reference. */
        memset(lib_sim_nor+LIB_BASE,0xFF,LIB_BANKS*2u*LIB_SECTOR);
        bad+=check("unwritten LIBRARY 033 retains stable reference identity",
                   !project_reference_load(5,UP_SLOTS) &&
                   trk[5].user==UP_SLOTS+1u && song.sel==chosen);
        /* Corrupted occupied slots are not silently replaced by a different
         * factory sound: reject the project reference instead. */
        bad+=check("prepare occupied LIBRARY 033 for corruption test",
                   !lib_slot_write(0,savedAnalog));
        {
            uint32_t seq;
            int copy=lib_latest(0,&seq);
            bad+=check("find committed library copy",copy>=0);
            if(copy>=0) {
                lib_sim_nor[lib_base(0,(uint32_t)copy)+LIB_PAYLOAD_OFF+22u]^=1u;
                uint8_t oldUser=trk[5].user;
                bad+=check("corrupt occupied LIBRARY 033 rejects recall without changing track",
                           project_reference_load(5,UP_SLOTS)==1 &&
                           trk[5].user==oldUser && song.sel==chosen);
            }
        }
    }
    printf("LIB SOUND INTEGRATION: %s\n",bad?"FAIL":"ALL TESTS PASSED");
    return !!bad;
}
