/* SPDX-License-Identifier: GPL-3.0-only
 * MASHINA project references: clean-break storage format prototype.
 *
 * This codec is intentionally independent of existing FUN* project snapshots.
 * NO device save/recall path uses it yet. Project settings payload is opaque
 * until each existing per-project field has an explicit new-format mapping.
 */
#include <stdint.h>
#include <string.h>

enum {
    MPR_TRACKS = 6, MPR_PRESETS = 64, MPR_EMPTY = 255,
    MPR_NAME = 12, MPR_HEADER = 32, MPR_MAX_SETTINGS = 2048,
    MPR_MAX_RECORD = MPR_HEADER + MPR_MAX_SETTINGS
};
#define MPR_MAGIC 0x3152504Du /* little endian "MPR1" */
#define MPR_VERSION 1u

/* Header:
 *  0..3  magic MPR1
 *  4     version
 *  5     section (0=A, 1=B, 2=C, 3=D)
 *  6     settings length LSB
 *  7     settings length MSB
 *  8..13 six slot references: 0..31 USER, 32..63 LIB, 255 unassigned
 * 14..25 section name, ASCII padded with zero
 * 26..27 reserved (zero)
 * 28..31 CRC32 of header bytes 0..27 followed by payload bytes
 * 32..   project-specific data (NOT sound data)
 */
static uint32_t mpr_crc_add(uint32_t crc, const uint8_t *b, uint32_t n)
{
    for (uint32_t i=0;i<n;i++) {
        crc ^= b[i];
        for (uint32_t j=0;j<8;j++)
            crc=(crc>>1)^((crc&1u)?0xEDB88320u:0u);
    }
    return crc;
}
static uint32_t mpr_crc(const uint8_t *b, uint32_t payload)
{
    uint32_t c=mpr_crc_add(0xFFFFFFFFu,b,28u);
    return ~mpr_crc_add(c,b+MPR_HEADER,payload);
}
static void mpr_u32(uint8_t *b, uint32_t x)
{
    for(uint32_t i=0;i<4;i++)b[i]=(uint8_t)(x>>(8u*i));
}
static uint32_t mpr_get32(const uint8_t *b)
{
    return (uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
}
static int mpr_name_valid(const uint8_t *s)
{
    int zero=0;
    for(uint32_t i=0;i<MPR_NAME;i++) {
        if(!s[i])zero=1;
        else if(zero || s[i]<32u || s[i]>126u)return 0;
    }
    return 1;
}
static int mpr_encode(uint8_t out[MPR_MAX_RECORD], uint32_t section,
                      const uint8_t ref[MPR_TRACKS], const uint8_t name[MPR_NAME],
                      const uint8_t *settings, uint32_t size)
{
    if (!out || !ref || !name || section>3u || size>MPR_MAX_SETTINGS ||
        (size && !settings) || !mpr_name_valid(name))return 0;
    for(uint32_t i=0;i<MPR_TRACKS;i++)
        if(ref[i]>=MPR_PRESETS && ref[i]!=MPR_EMPTY)return 0;
    memset(out,0,MPR_HEADER);
    mpr_u32(out,MPR_MAGIC);
    out[4]=MPR_VERSION;out[5]=(uint8_t)section;
    out[6]=(uint8_t)size;out[7]=(uint8_t)(size>>8);
    memcpy(out+8,ref,MPR_TRACKS);
    memcpy(out+14,name,MPR_NAME);
    if(size)memcpy(out+MPR_HEADER,settings,size);
    mpr_u32(out+28,mpr_crc(out,size));
    return (int)(MPR_HEADER+size);
}
static int mpr_decode(const uint8_t *in, uint32_t length,
                      uint32_t *section, uint8_t refs[MPR_TRACKS],
                      uint8_t name[MPR_NAME], uint8_t *settings, uint32_t max,
                      uint32_t *settings_len)
{
    if(!in || !section || !refs || !name || !settings_len || length<MPR_HEADER ||
       mpr_get32(in)!=MPR_MAGIC || in[4]!=MPR_VERSION || in[5]>3u ||
       in[26] || in[27] || !mpr_name_valid(in+14))return 0;
    uint32_t n=(uint32_t)in[6]|((uint32_t)in[7]<<8);
    if(n>MPR_MAX_SETTINGS || n>max || length!=MPR_HEADER+n ||
       (n && !settings) || mpr_get32(in+28)!=mpr_crc(in,n))return 0;
    for(uint32_t i=0;i<MPR_TRACKS;i++)
        if(in[8+i]>=MPR_PRESETS && in[8+i]!=MPR_EMPTY)return 0;
    /* No partial output on invalid records. */
    *section=in[5];
    memcpy(refs,in+8,MPR_TRACKS);
    memcpy(name,in+14,MPR_NAME);
    if(n)memcpy(settings,in+MPR_HEADER,n);
    *settings_len=n;
    return 1;
}
