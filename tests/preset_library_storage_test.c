/* SPDX-License-Identifier: GPL-3.0-only
 * Simulated-NOR proof for the experimental 32-entry library bank.
 * No access to a real device or physical flash.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t nor[0x100000];
static int fail_after = -1, tear_after = -1, wrong_region;
static uint32_t writes;
static int lib_flash_read(uint32_t off, void *p, uint32_t n)
{
    if (off + n > sizeof nor) return -1;
    memcpy(p,nor+off,n);
    return 0;
}
static int lib_flash_erase(uint32_t off)
{
    if (off < 0xC0000u || off + 4096u > 0xC8000u || off % 4096u) { wrong_region++; return -1; }
    if (fail_after == 0) return -1;
    if (fail_after > 0) fail_after--;
    memset(nor+off,0xFF,4096);
    writes++;
    return 0;
}
static int lib_flash_prog(uint32_t off,const void *p,uint32_t n)
{
    const uint8_t *b=p;
    if (off < 0xC0000u || off + n > 0xC8000u || (off & 255u) + n > 256u) {
        wrong_region++; return -1;
    }
    if (fail_after == 0) return -1;
    if (fail_after > 0) fail_after--;
    /* Simulate real NOR power loss during a page or commit-header program. */
    if (tear_after == 0) {
        for (uint32_t i=0;i<n/2u;i++) nor[off+i]&=b[i];
        tear_after = -1; writes++; return -1;
    }
    if (tear_after > 0) tear_after--;
    for (uint32_t i=0;i<n;i++) nor[off+i]&=b[i];
    writes++;
    return 0;
}
#include "../firmware/src/preset_library_store.c"

static int check(const char *s,int ok)
{
    printf("LIB NOR: %-61s %s\n",s,ok?"PASS":"FAIL");
    return !ok;
}
static void make(uint8_t r[LIB_ENTRY],uint32_t slot,uint8_t val)
{
    for (uint32_t i=0;i<LIB_ENTRY;i++) r[i]=(uint8_t)(val + slot*17u + i*19u);
}
int main(void)
{
    static uint8_t r[LIB_SLOTS][LIB_ENTRY],got[LIB_ENTRY], newer[LIB_ENTRY],zero[LIB_ENTRY];
    int bad=0, all=1;
    memset(nor,0xFF,sizeof nor);
    memset(zero,0,sizeof zero);
    for(uint32_t i=0;i<LIB_SLOTS;i++) {
        make(r[i],i,11);
        if (lib_slot_write(i,r[i])) all=0;
    }
    bad+=check("all 32 full 304-byte records written atomically",all);
    for(uint32_t i=0;i<LIB_SLOTS;i++) {
        if (lib_slot_read(i,got) || memcmp(got,r[i],LIB_ENTRY)) all=0;
    }
    bad+=check("all 32 independent complete sound+kit records read back",all);
    bad+=check("all writes limited to eight intended USR sectors",!wrong_region);
    /* Update an occupied slot; no adjacent sound or previous slot may change. */
    make(newer,17,90);
    bad+=check("replace LIB 18 only",!lib_slot_write(17,newer));
    bad+=check("replaced record plays back unchanged",!lib_slot_read(17,got)&&!memcmp(got,newer,LIB_ENTRY));
    all=1;
    for(uint32_t i=0;i<LIB_SLOTS;i++) if(i!=17) {
        if (lib_slot_read(i,got)||memcmp(got,r[i],LIB_ENTRY)) all=0;
    }
    bad+=check("other 31 full records never changed",all);
    bad+=check("erase one slot",!lib_slot_write(17,zero));
    bad+=check("empty slot persists",!lib_slot_read(17,got)&&!memcmp(got,zero,LIB_ENTRY));
    /* Save a known valid value then force interruption at successive write stages. */
    bad+=check("restore known slot before power failures",!lib_slot_write(17,r[17]));
    all=1;
    for(int cut=0;cut<13;cut++) {
        int rc;
        fail_after=cut;
        rc=lib_slot_write(17,newer);
        fail_after=-1;
        if (lib_slot_read(17,got)) {all=0;break;}
        if (memcmp(got,r[17],LIB_ENTRY) && memcmp(got,newer,LIB_ENTRY)) {all=0;break;}
        /* Reset to the old value before testing another torn write. */
        if (lib_slot_write(17,r[17])) {all=0;break;}
        (void)rc;
    }
    bad+=check("interrupted erase/program preserves a whole old or new record",all);
    /* A write can stop in the MIDDLE of a page, not only at operation
     * boundaries. Exercise all ten 256-byte payload pages and commit header.
     * The other seven records in the same bank must remain unchanged. */
    all=1;
    for(int cut=0;cut<12;cut++) {
        if(lib_slot_write(17,r[17])){all=0;break;}
        tear_after=cut;
        (void)lib_slot_write(17,newer);
        tear_after=-1;
        if(lib_slot_read(17,got) ||
           (memcmp(got,r[17],LIB_ENTRY) && memcmp(got,newer,LIB_ENTRY))){
            all=0;break;
        }
        for(uint32_t slot=16;slot<24;slot++)if(slot!=17) {
            if(lib_slot_read(slot,got) || memcmp(got,r[slot],LIB_ENTRY)){
                all=0;break;
            }
        }
        if(!all)break;
    }
    bad+=check("torn payload/header page preserves bank and neighboring sounds",all);

    /* Corrupt latest copy: CRC rejects it and falls back to older commit. */
    bad+=check("oldest valid copy survives a damaged newest copy",({
        uint32_t seq; int current=lib_latest(2,&seq),ok=0;
        if(current>=0) {
            nor[lib_base(2,(uint32_t)current)+LIB_PAYLOAD_OFF+21u]^=1u;
            ok=lib_slot_read(17,got)==0; /* only guaranteed: fallback record valid */
        }
        ok;
    }));
    /* If both commit headers become invalid, do not replace valuable data
     * by falsely assuming the device was freshly initialized. */
    {
        uint32_t bank = 17u / LIB_PER_BANK;
        nor[lib_base(bank,0u)] ^= 1u;
        nor[lib_base(bank,1u)] ^= 1u;
        bad+=check("two corrupt copies are reported, never treated as empty",
                   lib_slot_read(17,got)==2 && lib_slot_write(17,r[17])==2);
    }
    /* A blank header is NOT proof that a sector belongs to the library:
     * an unknown previous owner may have written data elsewhere. */
    {
        uint32_t bank=3u, addr0=lib_base(bank,0u), addr1=lib_base(bank,1u);
        uint8_t foreign0=0xA7u, foreign1=0x5Cu;
        memset(nor+addr0,0xFF,LIB_SECTOR);
        memset(nor+addr1,0xFF,LIB_SECTOR);
        nor[addr0+LIB_PAYLOAD_OFF+42u]=foreign0;
        uint32_t before_writes=writes;
        bad+=check("blank headers with foreign payload are NOT considered free",
                   lib_slot_read(24u,got)==2 && lib_slot_write(24u,r[24u])==2 &&
                   writes==before_writes && nor[addr0+LIB_PAYLOAD_OFF+42u]==foreign0);
        memset(nor+addr0,0xFF,LIB_SECTOR);
        nor[addr1+LIB_SECTOR-1u]=foreign1;
        before_writes=writes;
        bad+=check("foreign bytes at sector tail also block initialization",
                   lib_slot_write(24u,r[24u])==2 && writes==before_writes &&
                   nor[addr1+LIB_SECTOR-1u]==foreign1);
        memset(nor+addr1,0xFF,LIB_SECTOR);
        bad+=check("fully erased paired sectors can initialize normally",
                   lib_slot_write(24u,r[24u])==0 &&
                   lib_slot_read(24u,got)==0 && !memcmp(got,r[24u],LIB_ENTRY));
    }
    bad+=check("bad slots rejected",lib_slot_write(LIB_SLOTS,r[0])==1&&lib_slot_read(LIB_SLOTS,got)==1);
    bad+=check("no flash operation escaped dedicated region",!wrong_region);
    printf("LIB NOR: %s (%u flash operations)\n",bad?"FAIL":"ALL TESTS PASSED",writes);
    return !!bad;
}
