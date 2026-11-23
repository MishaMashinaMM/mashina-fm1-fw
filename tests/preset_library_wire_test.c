/* SPDX-License-Identifier: GPL-3.0-only
 * Full host-only WebMIDI binary transfer cycle; never accesses device flash.
 */
#define main storage_original_main
#include "preset_library_storage_test.c"
#undef main
#include "../firmware/src/preset_library_wire.c"

static void mk_valid(uint8_t r[LIB_ENTRY],uint32_t slot)
{
    memset(r,0,sizeof(uint8_t)*LIB_ENTRY);
    r[0]=0xA5; r[1]=4; r[2]=(uint8_t)(slot%21u); r[3]=127u;
    memcpy(r+4,"LIBTEST",7);
    for(uint32_t i=0;i<127u;i++)r[16+i]=(uint8_t)(i%192u);
    for(uint32_t i=192;i<LIB_ENTRY;i++)r[i]=(uint8_t)(i+slot*17u);
}
int main(void)
{
    static uint8_t r[LIB_ENTRY],got[LIB_ENTRY],args[1+LIB_WIRE_PAYLOAD],reply[LIB_WIRE_MAX],out[LIB_WIRE_MAX];
    uint32_t n,ret; int bad=0;
    memset(nor,0xFF,sizeof nor);
    mk_valid(r,2);
    { uint8_t cap[LIB_WIRE_MAX];
      uint32_t len=lib_wire_reply(LIB_WIRE_CAP,0,0,cap);
      bad+=check("CAP advertises 32 slots and host read-only status",
                 len==3u&&cap[0]==LIB_VERSION&&cap[1]==32u&&cap[2]==0u);
    }
    args[0]=2;
    n=lib_pack7(r,LIB_ENTRY,args+1);
    bad+=check("packed SysEx length is exactly 348 bytes",n==LIB_WIRE_PAYLOAD);
    bad+=check("7-bit SysEx payload contains no top bits",({
      int ok=1;for(uint32_t i=1;i<n+1;i++)if(args[i]&128u)ok=0;ok;
    }));
    bad+=check("PUT record with real binary sidecar",lib_wire_reply(LIB_WIRE_PUT,args,1+n,reply)==2&&reply[1]==0);
    ret=lib_wire_reply(LIB_WIRE_GET,args,1,out);
    bad+=check("GET returns packed 304-byte record",ret==2+LIB_WIRE_PAYLOAD&&out[1]==0);
    bad+=check("GET round-trip reproduces 304 bytes",
       lib_unpack7(out+2,ret-2,got)==LIB_ENTRY&&!memcmp(got,r,LIB_ENTRY));
    {uint8_t a[2]={0,4};
      ret=lib_wire_reply(LIB_WIRE_LIST,a,2,reply);
      bad+=check("LIST reports occupied slot 3, other three empty",ret==6u&&reply[2]==0&&reply[3]==0&&reply[4]==1&&reply[5]==0);
    }
    args[0]=33u;
    ret=lib_wire_reply(LIB_WIRE_PUT,args,1+n,reply);
    bad+=check("invalid destination refused",ret==2&&reply[1]==1);
    args[0]=2;
    /* Malformed length and forbidden high bits must fail with no flash writes. */
    writes=0;
    ret=lib_wire_reply(LIB_WIRE_PUT,args,n,reply);
    bad+=check("truncated PUT rejected before flash",ret==2&&reply[1]==1&&!writes);
    args[2]|=0x80u;
    ret=lib_wire_reply(LIB_WIRE_PUT,args,1+n,reply);
    bad+=check("non-7-bit frame rejected before flash",ret==2&&reply[1]==1&&!writes);
    args[2]&=0x7Fu;
    r[4]=0;
    lib_pack7(r,LIB_ENTRY,args+1);
    ret=lib_wire_reply(LIB_WIRE_PUT,args,1+n,reply);
    bad+=check("record missing name rejected before flash",ret==2&&reply[1]==3&&!writes);
    ret=lib_wire_reply(LIB_WIRE_ERASE,args,1,reply);
    bad+=check("ERASE clears exactly one library slot",ret==2&&reply[1]==0);
    ret=lib_wire_reply(LIB_WIRE_GET,args,1,out);
    bad+=check("GET of empty slot reports no content",ret==2&&out[1]==3);
    /* A CRC-protected bank may still contain a malformed sound (e.g. after
     * a different firmware version). It is NOT the same as a truly empty
     * LIBRARY slot: never omit it from an offline device backup. */
    {
        uint8_t damaged[LIB_ENTRY]={0};
        damaged[2]=9u; /* invalid nonempty record; not a factory fallback */
        bad+=check("simulate CRC-valid bank with malformed occupied sound",
                   lib_slot_write(2,damaged)==0);
        ret=lib_wire_reply(LIB_WIRE_GET,args,1,out);
        bad+=check("GET rejects malformed occupied slot rather than calling it empty",
                   ret==2u && out[1]==2u);
        uint8_t range[2]={2u,1u};
        ret=lib_wire_reply(LIB_WIRE_LIST,range,2,reply);
        bad+=check("LIST fails closed for malformed occupied slot",ret==0u);
        memset(damaged,0,sizeof damaged);
        bad+=check("restore genuinely empty slot",lib_slot_write(2,damaged)==0);
        ret=lib_wire_reply(LIB_WIRE_GET,args,1,out);
        bad+=check("restored empty slot reports factory fallback",ret==2u&&out[1]==3u);
    }
    bad+=check("all wire operations stayed inside library partition",!wrong_region);
    printf("LIB WIRE: %s\n",bad?"FAIL":"ALL TESTS PASSED");
    return !!bad;
}
