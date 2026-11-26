/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental reference-based project storage, compiled into the FM1
 * only under FELUCCA_PROJECT_REFS. All hardware writes remain separately
 * locked unless explicitly enabled. Sixteen banks, two project sections
 * each, two alternating 4 KiB sectors. Candidate reservation
 * 0xA0000..0xBFFFF (128 KiB) replaces the old 256 KiB allocation.
 *
 * A record occupies 1824 bytes maximum; 2*1824 = 3648 <= 3840.
 * Full bank is copied before update, new payload written before commit
 * header, and old sector is never erased until its replacement verifies.
 */
#include <stdint.h>
/* Embedded AC79 build uses firmware/src/libc.c; host tests provide libc. */

enum { MPRS_SECTOR=4096, MPRS_HEADER_OFF=256, MPRS_SLOTS=32,
       MPRS_BANKS=16, MPRS_PAIR=2, MPRS_BANK_BYTES=MPR_MAX_RECORD*2 };
#define MPRS_BASE 0xA0000u
#if defined(FELUCCA_LIB96) && FELUCCA_LIB96
#define MPRS_MAGIC 0x3253504Du /* MPS2: expanded physical layout */
#else
#define MPRS_MAGIC 0x3153504Du /* MPS1 */
#endif
_Static_assert(MPRS_BANK_BYTES<=MPRS_SECTOR-MPRS_HEADER_OFF,"two sections must fit flash");
_Static_assert(MPRS_BASE+MPRS_BANKS*2u*MPRS_SECTOR==0xC0000u,"unexpected project flash reservation");

typedef struct {uint32_t magic,bank,seq,length,crc;} mprs_head_t;
/* Stream old and new section bytes in 256-byte pages. The former full-bank
 * 3648-byte static pool buffer reduced AC79 safety headroom below 8 KiB.
 * No shared scratch is needed: copy A remains untouched while copy B commits. */
static int mprs_read(uint32_t off,void *dst,uint32_t len);
static int mprs_erase(uint32_t off);
static int mprs_prog(uint32_t off,const void *src,uint32_t len);

static uint32_t mprs_base(uint32_t bank,uint32_t copy)
{
    return MPRS_BASE+(bank*2u+copy)*MPRS_SECTOR;
}
/* Validate the committed sector without allocating a whole-bank image.
 * A physical read failure or a partially written payload is never accepted. */
static int mprs_valid(uint32_t bank,uint32_t copy,mprs_head_t *h)
{
    uint32_t base=mprs_base(bank,copy),crc=0xFFFFFFFFu;
    uint8_t page[256];
    if(mprs_read(base,h,sizeof *h) || h->magic!=MPRS_MAGIC ||
       h->bank!=bank || h->length!=MPRS_BANK_BYTES)return 0;
    for(uint32_t off=0;off<MPRS_BANK_BYTES;off+=sizeof page) {
        uint32_t n=MPRS_BANK_BYTES-off;
        if(n>sizeof page)n=sizeof page;
        if(mprs_read(base+MPRS_HEADER_OFF+off,page,n))return 0;
        crc=mpr_crc_add(crc,page,n);
    }
    return ~crc==h->crc;
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
    if(slot>=MPRS_SLOTS || !record)return 1;
    uint32_t bank=slot/MPRS_PAIR,seq;
    int copy=mprs_latest(bank,&seq);
    if(copy<0){
        if(!mprs_blank(bank))return 2;
        memset(record,0,MPR_MAX_RECORD);return 0;
    }
    mprs_head_t head;
    if(!mprs_valid(bank,(uint32_t)copy,&head))return 2;
    return mprs_read(mprs_base(bank,(uint32_t)copy)+MPRS_HEADER_OFF+
                     (slot%MPRS_PAIR)*MPR_MAX_RECORD,
                     record,MPR_MAX_RECORD)?2:0;
}
/* Assemble one bounded program page without buffering the whole bank.
 * The new section comes from RAM; its paired section comes from the old,
 * CRC-verified copy or from zeros when first initializing an erased bank.
 * An old copy is never erased during this operation. */
static int mprs_page(uint32_t slot,const uint8_t *record,
                     uint32_t oldbase,uint32_t off,uint8_t *page,uint32_t n)
{
    uint32_t done=0,replace=slot%MPRS_PAIR;
    while(done<n) {
        uint32_t index=(off+done)/MPR_MAX_RECORD;
        uint32_t within=(off+done)%MPR_MAX_RECORD;
        uint32_t part=MPR_MAX_RECORD-within;
        if(part>n-done)part=n-done;
        if(index==replace)memcpy(page+done,record+within,part);
        else if(oldbase!=0xFFFFFFFFu) {
            if(mprs_read(oldbase+MPRS_HEADER_OFF+
                         index*MPR_MAX_RECORD+within,page+done,part))return 2;
        } else memset(page+done,0,part);
        done+=part;
    }
    return 0;
}
static int mprs_save(uint32_t slot,const uint8_t record[MPR_MAX_RECORD])
{
    if(slot>=MPRS_SLOTS || !record)return 1;
    uint32_t bank=slot/MPRS_PAIR,seq;
    int current=mprs_latest(bank,&seq);
    uint32_t oldbase=0xFFFFFFFFu;
    if(current<0){
        if(!mprs_blank(bank))return 2;
    } else {
        mprs_head_t old;
        if(!mprs_valid(bank,(uint32_t)current,&old))return 2;
        oldbase=mprs_base(bank,(uint32_t)current);
    }
    uint32_t next=current==0?1u:0u,base=mprs_base(bank,next);
    uint32_t crc=0xFFFFFFFFu;
    uint8_t page[256];
    /* Calculate the intended complete CRC before erasing anything. */
    for(uint32_t off=0;off<MPRS_BANK_BYTES;off+=sizeof page) {
        uint32_t n=MPRS_BANK_BYTES-off;
        if(n>sizeof page)n=sizeof page;
        if(mprs_page(slot,record,oldbase,off,page,n))return 2;
        crc=mpr_crc_add(crc,page,n);
    }
    mprs_head_t head={MPRS_MAGIC,bank,seq+1u,MPRS_BANK_BYTES,~crc};
    if(mprs_erase(base))return 2;
    for(uint32_t off=0;off<MPRS_BANK_BYTES;off+=sizeof page){
        uint32_t n=MPRS_BANK_BYTES-off;
        if(n>sizeof page)n=sizeof page;
        if(mprs_page(slot,record,oldbase,off,page,n) ||
           mprs_prog(base+MPRS_HEADER_OFF+off,page,n))return 2;
    }
    if(mprs_prog(base,&head,sizeof head))return 2;
    mprs_head_t verify;
    if(!mprs_valid(bank,next,&verify)||verify.seq!=head.seq)return 2;
    return 0;
}
