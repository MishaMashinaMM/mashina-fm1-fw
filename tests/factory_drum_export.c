/* SPDX-License-Identifier: GPL-3.0-only
 * Derive 304-byte drum records from the actual firmware engine defaults.
 * Host simulator only: never writes FM1 flash or modifies factory sources.
 *
 * Exact path: factory preset -> per-track engine sound -> USER record capture
 * -> 112-byte drum sidecar. Capturing the real firmware DSP structures avoids
 * guessing the 12 independent pad patches and their kit-level controls.
 */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static const uint32_t KIT_ENGINE[]={
    ENGI_DRUM,ENGI_DRUMLAB,ENGI_DRUMAC,ENGI_DRUMIND
};
int main(void)
{
    unsigned n=0;
    ui_power_on();
    song.sel=0;
    /* Factory capture is a single-track host operation. DRUM-X has one
     * global kit shared by every live drum track. Other startup tracks may
     * already use DRUM-X; keep them out of drum_elsewhere() so every BUTTLER
     * preset actually loads its own DX_KITS[i] before capture. */
    trk_live=1;
    puts("{\"format\":\"mashina-derived-factory-drum-records\",\"version\":1,\"source\":\"live-firmware-host-capture\",\"records\":[");
    for(uint32_t eidx=0;eidx<4;eidx++){
        uint32_t engine=KIT_ENGINE[eidx];
        uint32_t count=ENGINES[engine]->npresets;
        for(uint32_t i=0;i<count;i++){
            uint8_t record[304],side[UPF_PK];
            const char *name=ENGINES[engine]->presets[i].name;
            int result;
            set_engine_of(TSEL,engine);
            TSEL->engine=TSEL->eng_req;
            /* This public preset path also reinitializes each kit's full
             * pad data (DX_KITS, LAB, AC, IND); apply_preset_to alone
             * updates EDIT values but can retain a previous kit. */
            apply_preset(i);
            result=up_store(0,name);
            if((result!=0&&result!=3)||!up_used(0)||
               upd_companion_get(0,side)||side[111]!=UPD_FMT){
                fprintf(stderr,"Cannot capture complete factory kit %u/%u %s (save=%d, side marker=%u)\n",
                        engine,i,name,result,side[111]);
                return 1;
            }
            memcpy(record,up_rec(0),sizeof(up_rec_t));
            memcpy(record+sizeof(up_rec_t),side,sizeof side);
            if(n++)puts(",");
            printf("{\"engine\":%u,\"enginePresetIndex\":%u,\"name\":\"%s\",\"recordBytes\":[",
                   engine,i,name);
            for(uint32_t b=0;b<sizeof record;b++)
                printf("%s%u",b?",":"",(unsigned)record[b]);
            uint32_t checksum=2166136261u;
            for(uint32_t byte=0;byte<sizeof record;byte++)
                checksum=(checksum^record[byte])*16777619u;
            printf("],\"fnv32\":%u}",checksum);
        }
    }
    puts("]}");
    if(n!=18){fprintf(stderr,"Expected 18 factory kits, got %u\n",n);return 1;}
    return 0;
}
