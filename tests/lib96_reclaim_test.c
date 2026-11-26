/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/src/lib96_reclaim.c"
static uint8_t sectors[16];static unsigned erases;static int fail_at=-1;
static uint32_t lib96_sector_state(uint32_t i){assert(i<16);return sectors[i];}
static void lib96_watchdog(void){}
static int lib96_erase_sector(uint32_t i){assert(i<16&&sectors[i]==L96_LEGACY);erases++;if((int)i==fail_at){sectors[i]=L96_UNKNOWN;return 1;}sectors[i]=L96_BLANK;return 0;}
int main(void){
 uint8_t states[16];
 memset(sectors,0,16);assert(!lib96_reclaim_scan(states));assert(!lib96_reclaim_apply()&&!erases);
 sectors[0]=L96_LEGACY;sectors[15]=L96_UNKNOWN;assert(lib96_reclaim_apply()==1&&!erases&&sectors[0]==L96_LEGACY);
 sectors[15]=L96_IO;assert(lib96_reclaim_apply()==1&&!erases);
 sectors[15]=L96_BLANK;sectors[1]=L96_VALID;assert(lib96_reclaim_apply()==1&&!erases);
 sectors[1]=L96_LEGACY;sectors[2]=L96_VALID;sectors[3]=L96_VALID;
 assert(!lib96_reclaim_apply()&&erases==2&&sectors[0]==0&&sectors[1]==0&&sectors[2]==1&&sectors[3]==1);
 assert(!lib96_reclaim_apply()&&erases==2);
 sectors[4]=L96_LEGACY;sectors[5]=L96_LEGACY;fail_at=5;
 assert(lib96_reclaim_apply()==2&&erases==4&&sectors[4]==0&&sectors[5]==3);
 assert(lib96_reclaim_apply()==1&&erases==4);
 puts("PASS: LIB96 expansion preflight refuses unknown/mixed/IO, preserves valid LIB1, verifies erase, interruption fails closed");
}
