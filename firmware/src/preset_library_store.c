/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental 32-slot library storage, writes locked until reclaimed FUN
 * project sectors and the clean-install/updater path are physically validated.
 *
 * Four banks of eight combined records. Each bank has two 4KiB copy-on-write
 * sectors; 32 KiB total at 0xC0000..0xC7FFF (retired FUN project sectors).
 * No heap and no full-bank RAM mirror.
 * Each record is the current 192-byte USER sound plus a 112-byte FM6/drum
 * companion. The caller owns validation/conversion and sound installation.
 *
 * Do NOT include in the device build or route physical flash writes here until
 * the reservation and upgrade safety gate is explicitly passed.
 *
 * Host supplies lib_flash_read / lib_flash_erase / lib_flash_prog.
 */
#include <stdint.h>

#include "preset_geometry.h"
#define LIB_STORE_SLOTS MASHINA_LIBRARY_SLOTS
#define LIB_STORE_END MASHINA_LIBRARY_END
/* Expanded geometry is host-only until all device write gates are reviewed. */
enum { LIB_SLOTS = LIB_STORE_SLOTS, LIB_PER_BANK = 8, LIB_BANKS = LIB_STORE_SLOTS / 8, LIB_ENTRY = 304,
       LIB_SECTOR = 4096, LIB_PAYLOAD_OFF = 256, LIB_BANK_BYTES = LIB_PER_BANK * LIB_ENTRY };
#define LIB_BASE 0xC0000u
#define LIB_MAGIC 0x3142494Cu /* "LIB1" */
#define LIB_VERSION 1u

_Static_assert(LIB_BANK_BYTES <= LIB_SECTOR - LIB_PAYLOAD_OFF, "library bank exceeds one sector");
_Static_assert(LIB_BASE + LIB_BANKS * 2u * LIB_SECTOR == LIB_STORE_END, "library must use retired FUN sectors only");

typedef struct {
    uint32_t magic, version, bank, sequence, length, crc;
} lib_head_t;

/* Main-loop flash scratch: same zeroed low RAM as st_buf, not audio or radio DMA. */
#if defined(MASHINA_BLE_EXPERIMENT) && MASHINA_BLE_EXPERIMENT
static uint8_t lib_work[LIB_BANK_BYTES] __attribute__((section(".lowbuf"), aligned(4)));
#else
static uint8_t lib_work[LIB_BANK_BYTES] __attribute__((section(".pool")));
#endif /* 2,432 B scratch, not 32 complete presets */

static int lib_flash_read(uint32_t off, void *p, uint32_t n);
static int lib_flash_erase(uint32_t off);
static int lib_flash_prog(uint32_t off, const void *p, uint32_t n);

static uint32_t lib_base(uint32_t bank, uint32_t copy)
{
    return LIB_BASE + (bank * 2u + copy) * LIB_SECTOR;
}
static uint32_t lib_crc(const void *data, uint32_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= p[i];
        for (uint32_t b = 0; b < 8; b++) crc = (crc >> 1) ^ ((crc & 1u) ? 0xEDB88320u : 0u);
    }
    return ~crc;
}
static int lib_valid(uint32_t bank, uint32_t copy, lib_head_t *h)
{
    uint32_t a = lib_base(bank,copy);
    if (lib_flash_read(a,h,sizeof *h)) return 0;
    if (h->magic != LIB_MAGIC || h->version != LIB_VERSION || h->bank != bank ||
        h->length != LIB_BANK_BYTES) return 0;
    if (lib_flash_read(a + LIB_PAYLOAD_OFF,lib_work,LIB_BANK_BYTES)) return 0;
    return h->crc == lib_crc(lib_work,LIB_BANK_BYTES);
}
/* Returns 0 or 1 for newest valid bank, or -1 for an unused bank.
 * Sequence arithmetic uses unsigned wraparound. */
static int lib_latest(uint32_t bank, uint32_t *sequence)
{
    lib_head_t a,b;
    int va = lib_valid(bank,0,&a), vb = lib_valid(bank,1,&b);
    if (!va && !vb) { *sequence = 0u; return -1; }
    if (vb && (!va || (uint32_t)(b.sequence - a.sequence) < 0x80000000u)) {
        *sequence = b.sequence;
        return 1;
    }
    *sequence = a.sequence;
    return 0;
}
/* No valid commit can mean genuinely new storage OR foreign/corrupt data.
 * On first use, require BOTH whole sectors to be erased, not just their
 * headers: otherwise an old partition owner with blank first bytes could
 * be destroyed. A torn first write fails closed until explicit recovery.
 * Never auto-erase an unknown/partly programmed bank. */
static int lib_uncommitted(uint32_t bank)
{
    uint8_t page[256];
    for (uint32_t copy = 0; copy < 2u; copy++) {
        for (uint32_t off = 0; off < LIB_SECTOR; off += sizeof page) {
            if (lib_flash_read(lib_base(bank,copy) + off,page,sizeof page))
                return 0;
            for (uint32_t i = 0; i < sizeof page; i++)
                if (page[i] != 0xFFu) return 0;
        }
    }
    return 1;
}
/* Return 0 for valid/empty slot, 1 for bad index, 2 for a flash error.
 * Empty banks are entirely zero bytes; records have their own 'used' flag. */
static int lib_slot_read(uint32_t slot, uint8_t out[LIB_ENTRY])
{
    uint32_t seq, bank;
    int copy;
    if (slot >= LIB_SLOTS) return 1;
    bank = slot / LIB_PER_BANK;
    copy = lib_latest(bank,&seq);
    if (copy < 0) {
        if (!lib_uncommitted(bank)) return 2;
        memset(out,0,LIB_ENTRY);
        return 0;
    }
    {
        lib_head_t h;
        if (!lib_valid(bank,(uint32_t)copy,&h)) return 2;
    }
    memcpy(out,lib_work + (slot % LIB_PER_BANK) * LIB_ENTRY,LIB_ENTRY);
    return 0;
}
/* Write a complete 304-byte record or an all-zero record to erase one slot.
 * Payload first, header/commit LAST; old sector untouched until new verified. */
static int lib_slot_write(uint32_t slot, const uint8_t in[LIB_ENTRY])
{
    uint32_t bank, sequence, addr, copy, offset;
    int old;
    lib_head_t h, verify;
    if (slot >= LIB_SLOTS) return 1;
    bank = slot / LIB_PER_BANK;
    old = lib_latest(bank,&sequence);
    if (old < 0) {
        if (!lib_uncommitted(bank)) return 2;
        memset(lib_work,0,sizeof lib_work);
    }
    else if (!lib_valid(bank,(uint32_t)old,&h)) return 2;
    memcpy(lib_work + (slot % LIB_PER_BANK) * LIB_ENTRY,in,LIB_ENTRY);
    copy = old == 0 ? 1u : 0u;
    addr = lib_base(bank,copy);
    h.magic = LIB_MAGIC;
    h.version = LIB_VERSION;
    h.bank = bank;
    h.sequence = sequence + 1u;
    h.length = LIB_BANK_BYTES;
    h.crc = lib_crc(lib_work,sizeof lib_work);
    if (lib_flash_erase(addr)) return 2;
    /* Always program <=256-byte chunks aligned to flash pages. */
    for (offset = 0; offset < sizeof lib_work; offset += 256u) {
        uint32_t n = sizeof lib_work - offset;
        if (n > 256u) n = 256u;
        if (lib_flash_prog(addr + LIB_PAYLOAD_OFF + offset,lib_work + offset,n)) return 2;
    }
    if (lib_flash_prog(addr,&h,sizeof h)) return 2;
    if (!lib_valid(bank,copy,&verify) || verify.sequence != h.sequence) return 2;
    return 0;
}
