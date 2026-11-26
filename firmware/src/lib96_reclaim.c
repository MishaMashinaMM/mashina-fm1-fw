/* SPDX-License-Identifier: GPL-3.0-only
 * Explicit one-device expansion. The old 32 slots, MPR1, USER and settings
 * are never touched. Caller provides classified sectors, erase and watchdog.
 * No unknown/corrupt sector is erased, including an interrupted first erase.
 */
enum { LIB96_FIRST_BANK=4, LIB96_SECTORS=16 };
enum { L96_BLANK=0, L96_VALID=1, L96_LEGACY=2, L96_UNKNOWN=3, L96_IO=4 };
static uint32_t lib96_sector_state(uint32_t index);
static int lib96_erase_sector(uint32_t index);
static void lib96_watchdog(void);
static int lib96_reclaim_scan(uint8_t states[LIB96_SECTORS])
{
    int rc=0;
    for(uint32_t i=0;i<LIB96_SECTORS;i++){
        states[i]=(uint8_t)lib96_sector_state(i);
        if(states[i]>=L96_UNKNOWN)rc=1;
        lib96_watchdog();
    }
    for(uint32_t i=0;i<LIB96_SECTORS;i+=2){
        /* Do not erase either side of any committed LIB1 bank. */
        if((states[i]==L96_VALID && states[i+1]==L96_LEGACY) ||
           (states[i+1]==L96_VALID && states[i]==L96_LEGACY))rc=1;
    }
    return rc;
}
static int lib96_reclaim_apply(void)
{
    uint8_t states[LIB96_SECTORS];
    if(lib96_reclaim_scan(states))return 1;
    for(uint32_t i=0;i<LIB96_SECTORS;i++){
        if(states[i]==L96_LEGACY){
            if(lib96_sector_state(i)!=L96_LEGACY ||
               lib96_erase_sector(i) || lib96_sector_state(i)!=L96_BLANK)
                return 2;
        }
        lib96_watchdog();
    }
    return 0;
}
