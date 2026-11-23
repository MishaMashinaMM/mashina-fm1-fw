/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental library WebMIDI transfer protocol, host-only.
 *
 * Commands 90 LIST, 91 GET, 92 PUT, 93 ERASE in the existing
 * F0 7D 46 4C CMD <args> F7 envelope. The device firmware MUST NOT
 * include this file until flash partition ownership is established.
 *
 * PUT carries one 304-byte opaque record encoded with standard 7-bit
 * grouping: MSB mask + up to seven low-7-bit bytes. This format can
 * represent every byte including full FM6/drum sidecar data without loss.
 *
 * Arguments/replies exclude CMD and frame wrappers:
 * LIST request: [start,count], response [start,count,used...], max 16.
 * GET request: [slot], response [slot,rc,encoded bytes if rc==0].
 * PUT request: [slot,encoded 304 B], response [slot,rc].
 * ERASE request: [slot], response [slot,rc].
 * GET rc: 0 valid record, 1 malformed, 2 unreadable/corrupt,
 * * 3 genuinely empty (all-zero factory fallback). PUT rc=3 invalid data.
 * No transport/save policy included here.
 */
#include <stdint.h>
#define LIB_WIRE_CAP 89u
#define LIB_WIRE_LIST 90u
#define LIB_WIRE_GET 91u
#define LIB_WIRE_PUT 92u
#define LIB_WIRE_ERASE 93u
#define LIB_WIRE_PAYLOAD ((LIB_ENTRY + 6u) / 7u + LIB_ENTRY)
#define LIB_WIRE_MAX 400u
_Static_assert(LIB_ENTRY == 304, "wire compatibility with 192+112 record");
_Static_assert(LIB_WIRE_PAYLOAD == 348u, "wire 7bit length");

static int lib_record_valid(const uint8_t *r)
{
    /* Only the all-zero empty record and a valid up_rec_t record should be
     * accepted in production; keep host-only staging deliberately strict.
     * USER used/version/engine/np checks mirror up_valid. The sidecar is
     * opaque and must be validated by the engine-specific integration. */
    uint8_t used = r[0], ver = r[1], engine = r[2], np = r[3];
    uint32_t i;
    if (used != 0xA5u || ver < 1u || ver > 5u || engine >= 21u ||
        np < 8u || np > (ver >= 4u ? 144u : 72u) || !r[4]) return 0;
    if (ver >= 4u) for (i=0;i<np;i++) if (r[16u+i] > 191u) return 0;
    for (i=0;i<12u;i++) if (r[4u+i] && (r[4u+i] < 32u || r[4u+i] > 126u)) return 0;
    /* A drum sound without its independent full 12-pad companion is NOT
     * a complete LIBRARY sound. Refuse incomplete records rather than
     * silently replacing an existing kit with factory/default pads. */
    if ((engine == 10u || engine == 15u || engine == 19u || engine == 20u) &&
        r[LIB_ENTRY - 1u] != 0xD3u) return 0;
    return 1;
}
static uint32_t lib_pack7(const uint8_t *src, uint32_t n, uint8_t *dst)
{
    uint32_t i = 0, k = 0;
    while (i < n) {
        uint32_t h = k++, m = n - i > 7u ? 7u : n - i;
        dst[h] = 0;
        for (uint32_t j=0;j<m;j++,i++) {
            dst[h] |= (uint8_t)((src[i] >> 7) << j);
            dst[k++] = src[i] & 127u;
        }
    }
    return k;
}
static uint32_t lib_unpack7(const uint8_t *src, uint32_t n, uint8_t *dst)
{
    uint32_t i=0,k=0;
    while (i < n && k < LIB_ENTRY) {
        uint32_t mask = src[i++], count = LIB_ENTRY-k > 7u ? 7u : LIB_ENTRY-k;
        if (i+count > n || (mask >> count)) return 0;
        for(uint32_t j=0;j<count;j++) {
            if(src[i] & 128u) return 0;
            dst[k++] = src[i++] | (uint8_t)(((mask >> j)&1u)<<7);
        }
    }
    return i==n && k==LIB_ENTRY ? k : 0;
}
static uint8_t lib_wire_record[LIB_ENTRY] __attribute__((section(".pool")));
/* The host supplies a validated copy of the persistent storage file.
 * Writes are explicitly confined to host tests; no USB handler calls here. */
static uint32_t lib_wire_reply(uint32_t cmd, const uint8_t *args, uint32_t n,
                               uint8_t out[LIB_WIRE_MAX])
{
    uint32_t slot, k;
    if (!out) return 0;
    if (cmd == LIB_WIRE_CAP) {
        if (n) return 0;
        /* The actual firmware adapter overrides this with its write gate;
         * the shared host codec only describes format and slot count. */
        out[0] = LIB_VERSION; out[1] = LIB_SLOTS; out[2] = 0u;
        return 3u;
    }
    if (!args) return 0;
    if (cmd == LIB_WIRE_LIST) {
        if (n != 2u || args[0] >= LIB_SLOTS || !args[1] || args[1] > 16u ||
            args[0] + args[1] > LIB_SLOTS) return 0;
        out[0]=args[0]; out[1]=args[1];
        for (k=0;k<args[1];k++) {
            if (lib_slot_read(args[0]+k,lib_wire_record)) return 0;
            if (lib_record_valid(lib_wire_record)) {
                out[2u+k]=1u;
            } else {
                /* LIST must also fail closed rather than report damage as
                 * an unoccupied factory fallback. */
                for (uint32_t i=0;i<LIB_ENTRY;i++)
                    if (lib_wire_record[i]) return 0;
                out[2u+k]=0u;
            }
        }
        return 2u+args[1];
    }
    if (cmd < LIB_WIRE_GET || cmd > LIB_WIRE_ERASE || !n) return 0;
    slot=args[0];
    out[0]=(uint8_t)slot;
    if (slot >= LIB_SLOTS) { out[1]=1; return 2; }
    if (cmd == LIB_WIRE_GET) {
        if(n!=1u) return 0;
        if(lib_slot_read(slot,lib_wire_record)) {out[1]=2;return 2;}
        if(!lib_record_valid(lib_wire_record)) {
            /* Only an actually blank record is an empty/factory slot.
             * A CRC-valid but malformed occupied record is corruption, NOT
             * an empty slot: backups must never silently omit it. */
            for (k=0; k<LIB_ENTRY; k++)
                if (lib_wire_record[k]) {out[1]=2;return 2;}
            out[1]=3;return 2;
        }
        out[1]=0;
        return 2u+lib_pack7(lib_wire_record,LIB_ENTRY,out+2);
    }
    if (cmd == LIB_WIRE_ERASE) {
        if(n!=1u) return 0;
        uint8_t blank[LIB_ENTRY]={0};
        out[1]=(uint8_t)(lib_slot_write(slot,blank)?2u:0u);
        return 2;
    }
    if (n!=1u+LIB_WIRE_PAYLOAD ||
        lib_unpack7(args+1,n-1,lib_wire_record)!=LIB_ENTRY) { out[1]=1;return 2; }
    if (!lib_record_valid(lib_wire_record)) {out[1]=3;return 2;}
    out[1]=(uint8_t)(lib_slot_write(slot,lib_wire_record)?2u:0u);
    return 2;
}
