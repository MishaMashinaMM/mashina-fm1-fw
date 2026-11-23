/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental reference-based project storage, host-only.
 * Sixteen banks, two project sections each, two alternating 4 KiB sectors.
 * Planned reservation 0xA0000..0xBFFFF (128 KiB), replacing the old
 * 256 KiB project allocation. No firmware save/recall calls this yet.
 *
 * A record occupies 1824 bytes maximum; 2*1824 = 3648 <= 3840.
 * Full bank is copied before update, new payload written before commit
 * header, and old sector is never erased until its replacement verifies.
 */
#include <stdint.h>
#include <string.h>

enum { MPRS_SECTOR=4096, MPRS_HEADER_OFF=256, MPRS_SLOTS=32,
       MPRS_BANKS=16, MPRS_PAIR=2, MPRS_BANK_BYTES=MPR_MAX_RECORD*2 };
#define MPRS_BASE 0xA0000u
#define MPRS_MAGIC 0x3153504Du /* MPS1 */
_Static_assert(MPRS_BANK_BYTES<=MPRS_SECTOR-MPRS_HEADER_OFF,"two sections must fit flash");
_Static_assert(MPRS_BASE+MPRS_BANKS*2u*MPRS_SECTOR==0xC0000u,"unexpected project flash reservation");

typedef struct {uint32_t magic,bank,seq,length,crc;} mprs_head_t;
static uint8_t mprs_buf[MPRS_BANK_BYTES];
static int mprs_read(uint32_t off,void *dst,uint32_t len);
static int mprs_erase(uint32_t off);
static int mprs_prog(uint32_t off,const void *src,uint32_t len);

static uint32_t mprs_base(uint32_t bank,uint32_t copy)
{
    return MPRS_BASE+(bank*2u+copy)*MPRS_SECTOR;
}
static uint32_t mprs_checksum(const uint8_t *p,uint32_t n)
{
    return ~mpr_crc_add(0xFFFFFFFFu,p,n);
}
static int mprs_valid(uint32_t bank,uint32_t copy,mprs_head_t *h)
{
    uint32_t base=mprs_base(bank,copy);
    if(mprs_read(base,h,sizeof *h) || h->magic!=MPRS_MAGIC ||
       h->bank!=bank || h->length!=MPRS_BANK_BYTES)return 0;
    if(mprs_read(base+MPRS_HEADER_OFF,mprs_buf,MPRS_BANK_BYTES))return 0;
    return mprs_checksum(mprs_buf,MPRS_BANK_BYTES)==h->crc;
}
static int mprs_latest(uint32_t bank,uint32_t *seq)
{
    mprs_head_t a,b;
    int va=mprs_valid(bank,0,&a),vb=mprs_valid(bank,1,&b);
    if(!va&&!vb){*seq=0;return -1;}
    if(vb && (!va || (uint32_t)(b.seq-a.seq)<0x80000000u)){
        *seq=b.seq;return 1;
    }
    *seq=a.seq;return 0;
}
static int mprs_blank(uint32_t bank)
{
    uint8_t page[256];
    for(uint32_t copy=0;copy<2;copy++)
        for(uint32_t off=0;off<MPRS_SECTOR;off+=sizeof page){
            if(mprs_read(mprs_base(bank,copy)+off,page,sizeof page))return 0;
            for(uint32_t k=0;k<sizeof page;k++)if(page[k]!=0xFFu)return 0;
        }
    return 1;
}
/* Return 0 success, 1 invalid slot or record, 2 storage error.
 * A blank slot is represented by zero bytes, NOT a valid MPR1 record. */
static int mprs_load(uint32_t slot,uint8_t record[MPR_MAX_RECORD])
{
    if(slot>=MPRS_SLOTS)return 1;
    uint32_t bank=slot/MPRS_PAIR,seq;
    int copy=mprs_latest(bank,&seq);
    if(copy<0){
        if(!mprs_blank(bank))return 2;
        memset(record,0,MPR_MAX_RECORD);return 0;
    }
    mprs_head_t head;
    if(!mprs_valid(bank,(uint32_t)copy,&head))return 2;
    memcpy(record,mprs_buf+(slot%MPRS_PAIR)*MPR_MAX_RECORD,MPR_MAX_RECORD);
    return 0;
}
static int mprs_save(uint32_t slot,const uint8_t record[MPR_MAX_RECORD])
{
    if(slot>=MPRS_SLOTS || !record)return 1;
    uint32_t bank=slot/MPRS_PAIR,seq;
    int current=mprs_latest(bank,&seq);
    if(current<0){
        if(!mprs_blank(bank))return 2;
        memset(mprs_buf,0,sizeof mprs_buf);
    } else {
        mprs_head_t old;
        if(!mprs_valid(bank,(uint32_t)current,&old))return 2;
    }
    memcpy(mprs_buf+(slot%MPRS_PAIR)*MPR_MAX_RECORD,record,MPR_MAX_RECORD);
    uint32_t next=current==0?1u:0u,base=mprs_base(bank,next);
    mprs_head_t head={MPRS_MAGIC,bank,seq+1u,MPRS_BANK_BYTES,mprs_checksum(mprs_buf,MPRS_BANK_BYTES)};
    if(mprs_erase(base))return 2;
    for(uint32_t off=0;off<MPRS_BANK_BYTES;off+=256u){
        uint32_t n=MPRS_BANK_BYTES-off;
        if(n>256u)n=256u;
        if(mprs_prog(base+MPRS_HEADER_OFF+off,mprs_buf+off,n))return 2;
    }
    if(mprs_prog(base,&head,sizeof head))return 2;
    mprs_head_t verify;
    if(!mprs_valid(bank,next,&verify)||verify.seq!=head.seq)return 2;
    return 0;
}
