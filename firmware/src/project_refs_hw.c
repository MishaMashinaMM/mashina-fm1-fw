/* SPDX-License-Identifier: GPL-3.0-only
 * FM1 hardware adapter for the experimental reference project store.
 * Compile only under FELUCCA_PROJECT_REFS. Reads are bounded to the proposed
 * 0xA0000..0xBFFFF reservation. Writes are LOCKED independently by default.
 * Hardware ownership must still be validated before enabling any write.
 */
static int mprs_allowed(uint32_t address,uint32_t len)
{
    return len && address>=MPRS_BASE &&
           len<=MPRS_BANKS*2u*MPRS_SECTOR &&
           address<=MPRS_BASE+MPRS_BANKS*2u*MPRS_SECTOR-len;
}
static int mprs_read(uint32_t off,void *dst,uint32_t len)
{
#if FELUCCA_FLASH
    if(!flash_ok || !dst || !mprs_allowed(off,len))return -1;
    return st_read(off,dst,len);
#else
    (void)off;(void)dst;(void)len;return -1;
#endif
}
static int mprs_erase(uint32_t off)
{
#if defined(MASHINA_MPRS_HOST_FLASH)
    if(!mprs_allowed(off,MPRS_SECTOR) || (off&(MPRS_SECTOR-1u)))return -1;
    return st_erase(off);
#elif FELUCCA_FLASH && FELUCCA_PROJECT_REFS_WRITE
    uint32_t took,f;int rc;
    if(!flash_ok || !mprs_allowed(off,MPRS_SECTOR) ||
       (off&(MPRS_SECTOR-1u)) || transport_busy())return -1;
    f=irq_save();
    audio_silence();
    rc=FL_FAR(fl_erase4k_ram)(off,&took);
    irq_restore(f);
    return rc;
#else
    (void)off;return -1;
#endif
}
static int mprs_prog(uint32_t off,const void *src,uint32_t len)
{
#if defined(MASHINA_MPRS_HOST_FLASH)
    if(!src || !mprs_allowed(off,len) || len>256u || (off&255u)+len>256u)return -1;
    return st_prog(off,src,len);
#elif FELUCCA_FLASH && FELUCCA_PROJECT_REFS_WRITE
    uint32_t took,f;int rc;
    if(!flash_ok || !src || !mprs_allowed(off,len) ||
       len>256u || (off&255u)+len>256u || transport_busy())return -1;
    f=irq_save();
    audio_silence();
    rc=FL_FAR(fl_prog_ram)(off,src,len,&took);
    irq_restore(f);
    return rc;
#else
    (void)off;(void)src;(void)len;return -1;
#endif
}
