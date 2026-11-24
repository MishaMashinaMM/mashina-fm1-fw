/* SPDX-License-Identifier: GPL-3.0-only
 * Host-only read-only factory audition over CRC-confirmed retired FUN sectors.
 * The hardware CRC classifier is tested separately in flash_diagnostic_test.
 */
#define FELUCCA_FLASH_DIAG 1
#define main library_sound_integration_main
#include "presets_library_sound_test.c"
#undef main

/* The hardware version checks complete FUN sector header and payload CRCs.
 * Return 2 only for the one simulated confirmed legacy bank. */
static uint32_t diag_confirmed_bank = UINT32_MAX;
static uint32_t ed_diag_project_bank(uint32_t bank)
{
    return bank == diag_confirmed_bank ? 2u : 3u;
}
static int preview_check(const char *label, int ok)
{
    printf("READONLY PREVIEW %-58s %s\n",label,ok?"PASS":"FAIL");
    return !ok;
}
int main(void)
{
    int bad=0;
    memset(lib_sim_nor,0xFF,sizeof lib_sim_nor);
    lib_sim_outside=0u;
    ui_power_on();
    song.sel=0u;
    /* Non-erased, non-LIB1 bytes occupy all four simulated bank pairs. */
    for(uint32_t bank=0;bank<LIB_BANKS;bank++) {
        const uint32_t base=LIB_BASE+bank*2u*LIB_SECTOR;
        lib_sim_nor[base+512u]=0x53u;
        lib_sim_nor[base+LIB_SECTOR+512u]=0x4Du;
    }
    for(uint32_t bank=0;bank<LIB_BANKS;bank++) {
        diag_confirmed_bank=16u+bank;
        for(uint32_t i=0;i<LIB_PER_BANK;i++) {
            uint32_t slot=bank*LIB_PER_BANK+i;
            set_engine_of(TSEL,ENGI_ANALOG);
            TSEL->user=0u;
            preset_go(UP_SLOTS+slot);
            bad+=preview_check("all 32 valid retired FUN slots audition their own factory position",
                               TSEL->user==UP_SLOTS+slot+1u &&
                               library_diag_preview_legacy==1u &&
                               msg_is("FACTORY PREVIEW"));
        }
    }
    diag_confirmed_bank=UINT32_MAX;
    track_t before=*TSEL;
    preset_go(UP_SLOTS);
    bad+=preview_check("unknown flash refuses fallback and retains audible sound",
                       !memcmp(TSEL,&before,sizeof before) &&
                       msg_is("LIBRARY UNAVAILABLE"));
    bad+=preview_check("all sound previews leave simulated NOR unchanged",
                       lib_sim_outside==0u);
    memset(lib_sim_nor+LIB_BASE,0xFF,LIB_BANKS*2u*LIB_SECTOR);
    preset_go(UP_SLOTS);
    bad+=preview_check("truly blank LIBRARY is normal factory fallback, not preview",
                       TSEL->user==UP_SLOTS+1u &&
                       !library_diag_preview_legacy);
    return !!bad;
}
