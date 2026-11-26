/* SPDX-License-Identifier: GPL-3.0-only */
/* This handler is only reached when the explicit experimental feature is ON.
 * Writes still fail closed unless FELUCCA_LIB32_WRITE=1 was set for the build. */
#if FELUCCA_FLASH_DIAG
#include "flash_diagnostic.c"
#endif

/* Only the explicitly NON-DISTRIBUTABLE clean-layout build can arm the
 * destructive first install. These variables cannot exist in normal or
 * diagnostic firmware. A PREPARE command never writes. A COMMIT requires
 * a fresh one-time 7-bit nonce and a literal action phrase, and runs the
 * complete 53-sector preflight AGAIN immediately before any erase. */
#if FELUCCA_CLEAN_INSTALL_LAYOUT
static uint32_t clean_arm_ms, clean_arm_usb;
static uint8_t clean_arm_nonce, clean_armed;
#endif

static int ed_library_handle(uint32_t cmd, const uint8_t *args, uint32_t n)
{
    uint8_t response[LIB_WIRE_MAX];
    uint32_t length;
    if (cmd == 95u) { /* MPR1_CAP: 32 project sections, separate write gate */
#if defined(FELUCCA_PROJECT_REFS) && FELUCCA_PROJECT_REFS
        if(n) {ed_b(127u);ed_b(1u);return 1;}
        ed_b(1u);ed_b(32u);
#if FELUCCA_PROJECT_REFS_WRITE
#if FELUCCA_INITIALIZED_UPDATE || FELUCCA_PUBLIC_LAYOUT
        ed_b(flash_ok && clean_layout_normal_writes ? 1u : 0u);
#else
        ed_b(flash_ok?1u:0u);
#endif
#else
        ed_b(0u);
#endif
#if FELUCCA_PROJECT_REFS_FORMAT
#if FELUCCA_PUBLIC_LAYOUT
        ed_b(flash_ok && !clean_layout_normal_writes ? 1u : 0u);
#else
        ed_b(flash_ok?1u:0u);
#endif
#else
        ed_b(0u);
#endif
        return 1;
#else
        ed_b(0u);ed_b(0u);ed_b(0u);return 1;
#endif
    }
    if (cmd == 97u) { /* MPR1_PROBE: per-bank read-only flash status */
        if(n){ed_b(127u);ed_b(1u);return 1;}
        if(transport_busy()) {
            ed_b(16u);
            for(uint32_t i=0;i<16u;i++)ed_b(4u);
            return 1; /* no long read scan while a performance is playing */
        }
        ed_b(16u);
        for(uint32_t bank=0;bank<16u;bank++){
            uint32_t seq=0u;
#if defined(FELUCCA_PROJECT_REFS) && FELUCCA_PROJECT_REFS
            if(!flash_ok)ed_b(4u);
            else if(mprs_latest(bank,&seq)>=0)ed_b(1u);
            else if(mprs_blank(bank))ed_b(0u);
            else {
                uint32_t valid=1u;
                for(uint32_t copy=0;copy<2u;copy++)
                    if(!mprs_erased_sector(bank,copy) &&
                       !mprs_legacy_sector(bank,copy))valid=0u;
                ed_b(valid?2u:3u);
            }
#elif FELUCCA_FLASH_DIAG
            (void)seq;
            ed_b(flash_ok?ed_diag_project_bank(bank):4u);
#else
            (void)seq;
            ed_b(4u);
#endif
            fm1_wdt_feed(); /* 16 banks may require many flash read windows */
        }
        return 1;
    }
    if (cmd == 98u) { /* LIB32_PROBE: 4 banks, read only */
        if(n){ed_b(127u);ed_b(1u);return 1;}
        if(transport_busy()) {
            ed_b(LIB_BANKS);
            for(uint32_t i=0;i<LIB_BANKS;i++)ed_b(4u);
            return 1;
        }
        ed_b(LIB_BANKS);
        for(uint32_t bank=0;bank<LIB_BANKS;bank++){
            uint32_t seq=0u;
            if(!flash_ok)ed_b(4u);
            else if(lib_latest(bank,&seq)>=0)ed_b(1u);
            else if(lib_uncommitted(bank))ed_b(0u);
#if FELUCCA_FLASH_DIAG
            else {
                /* These new LIB1 candidates are retired FUN song sectors
                 * 16..19. Recognize their complete CRC-validated records
                 * without treating damaged/foreign bytes as safe to erase. */
                uint32_t legacy=ed_diag_project_bank(16u+bank);
                ed_b(legacy==2u?2u:legacy==4u?4u:3u);
            }
#else
            else ed_b(3u);
#endif
            fm1_wdt_feed(); /* read-only probe must not starve watchdog */
        }
        return 1;
    }
    if (cmd == 99u) { /* READ-ONLY sector CRC fingerprints; diagnostic image only */
#if FELUCCA_FLASH_DIAG
        if(n!=2u || args[0]>=53u || args[1]==0u || args[1]>8u ||
           (uint32_t)args[0]+args[1]>53u){
            ed_b(127u);ed_b(1u);return 1;
        }
        ed_b(1u);ed_b(args[0]);ed_b(args[1]);
        for(uint32_t k=0;k<args[1];k++){
            uint32_t hash=0u;
            uint32_t status=(!flash_ok || transport_busy())?4u:
              ed_diag_sector_crc((uint32_t)args[0]+k,&hash);
            ed_b(status);
            for(uint32_t i=0;i<5u;i++)ed_b(hash>>(7u*i));
        }
        return 1;
#else
        ed_b(127u);ed_b(1u);return 1;
#endif
    }
    if (cmd == 100u) { /* read-only 53-sector format preflight, never approval */
#if FELUCCA_FLASH_DIAG
        if(n!=2u || args[0]>=53u || args[1]==0u || args[1]>8u ||
           (uint32_t)args[0]+args[1]>53u){
            ed_b(127u);ed_b(1u);return 1;
        }
        ed_b(1u);ed_b(args[0]);ed_b(args[1]);
        for(uint32_t k=0;k<args[1];k++){
            const uint32_t status=(!flash_ok||transport_busy())?4u:
              ed_diag_layout_sector((uint32_t)args[0]+k);
            ed_b(status);
        }
        return 1;
#else
        ed_b(127u);ed_b(1u);return 1;
#endif
    }
    if (cmd == 101u) { /* CLEAN32_PREPARE: read-only, no erases */
#if FELUCCA_CLEAN_INSTALL_LAYOUT
        if(n || !flash_ok || transport_busy()){
            clean_armed=0u;
            ed_b(2u);
        } else {
            uint32_t rc=(uint32_t)clean_layout_preflight();
            clean_armed=(uint8_t)(rc==0u);
            clean_arm_ms=fm1_ms;
            clean_arm_usb=usb.resets;
            clean_arm_nonce=(uint8_t)((fm1_ms ^ (fm1_ms>>7) ^ 0x5Au)&0x7Fu);
            ed_b(rc); /* 0: source layout valid; 1: foreign/new; 2: busy */
            ed_b(clean_armed?clean_arm_nonce:0u);
        }
#else
        ed_b(2u); /* no formatter in ordinary firmware */
#endif
        return 1;
    }
    if (cmd == 102u) { /* CLEAN32_COMMIT: destructive, build-only */
#if FELUCCA_CLEAN_INSTALL_LAYOUT
#if FELUCCA_LIB96
        static const uint8_t phrase[9]={'M','A','S','H','I','N','A','9','6'};
#else
        static const uint8_t phrase[9]={'M','A','S','H','I','N','A','3','2'};
#endif
        int authorized=clean_armed && n==10u &&
                       memcmp(args,phrase,sizeof phrase)==0 &&
                       args[9]==clean_arm_nonce && clean_arm_usb==usb.resets &&
                       (uint32_t)(fm1_ms-clean_arm_ms)<=15000u;
        clean_armed=0u; /* consume the ticket even on a rejected attempt */
        if(!authorized)ed_b(1u);
        else if(ed_flash_stop())ed_b(2u);
        else {
            /* Synchronous one-device format is the ONLY temporary unlock.
             * Even on success, normal writes remain locked until a reboot
             * proves that the MPS1 marker was durably committed. */
            clean_layout_normal_writes=1u;
            const uint32_t result=(uint32_t)clean_layout_initialize();
            clean_layout_normal_writes=0u;
            ed_b(result);
        }
        /* Success means reboot is REQUIRED. Never auto-reset or hide a
         * failed/partial format behind a second attempt. */
#else
        ed_b(2u);
#endif
        return 1;
    }
    if (cmd == 96u) { /* MPR1_INIT, NOT enabled in ordinary builds */
/* The combined-layout prototype has NO on-device erase command. Until
 * full USER/settings/auxiliary partitions and recovery are verified, a
 * project-only format would create an inconsistent half-clean device. */
#if FELUCCA_PROJECT_REFS && FELUCCA_PROJECT_REFS_FORMAT && !FELUCCA_CLEAN_INSTALL_LAYOUT
        static const uint8_t key[4]={0x4Du,0x50u,0x52u,0x31u}; /* "MPR1" */
        if(n!=4u || memcmp(args,key,4u)) {
            ed_b(1u);
        } else if(ed_flash_stop()) {
            ed_b(2u);
        } else {
            ed_b((uint32_t)mprs_init_clean_break());
        }
#else
        ed_b(2u); /* always reject destructive initialization */
#endif
        return 1;
    }
    if (cmd == 94u) { /* CAPTURE_USER, never changes sound or writes flash */
        uint32_t slot = n ? args[0] : UP_SLOTS;
        uint32_t rc = 0u;
        if (n != 1u || slot >= UP_SLOTS || !up_used(slot)) rc = 1u;
        else {
            const up_rec_t *src = up_rec(slot);
            memset(lib_wire_record, 0, LIB_ENTRY);
            memcpy(lib_wire_record, src, sizeof *src);
            if (src->engine == ENGI_FM6 || upd_engine(src->engine)) {
                if (!upf_valid(&upf) || !(upf.used & (1u << slot)) ||
                    upf.e[slot].tag != upf_tag(src)) rc = 2u;
                else memcpy(lib_wire_record + sizeof *src, upf.e[slot].pk, UPF_PK);
            }
            if (!rc && !lib_record_valid(lib_wire_record)) rc = 3u;
        }
        ed_b(slot);
        ed_b(rc);
        if (!rc) {
            uint32_t len = lib_pack7(lib_wire_record, LIB_ENTRY, response);
            for (uint32_t i = 0; i < len; i++) ed_b(response[i]);
        }
        return 1;
    }
    if (cmd < LIB_WIRE_CAP || cmd > LIB_WIRE_ERASE) return 0;
    if (cmd == LIB_WIRE_PUT || cmd == LIB_WIRE_ERASE) {
        if (ed_flash_stop()) {
            ed_b(n ? args[0] : 127u);
            ed_b(2u);
            return 1;
        }
    }
    length = lib_wire_reply(cmd,args,n,response);
    /* A successful USB replacement/erase changes the PRESETS page metadata
     * immediately, without waiting for the next cold-boot scan. Re-read only
     * the written slot; never accept metadata from an unverified PUT packet. */
    if ((cmd == LIB_WIRE_PUT || cmd == LIB_WIRE_ERASE) &&
        length == 2u && response[1] == 0u && n && args[0] < LIB_SLOTS) {
        library_metadata_refresh(args[0]);
        ui.force = 1;
    }
    if (cmd == LIB_WIRE_CAP && length == 3u) {
#if FELUCCA_LIB32_WRITE
#if FELUCCA_INITIALIZED_UPDATE || FELUCCA_PUBLIC_LAYOUT
        response[2] = flash_ok && clean_layout_normal_writes ? 1u : 0u;
#else
        response[2] = flash_ok ? 1u : 0u;
#endif
#else
        response[2] = 0u;
#endif
    }
    for(uint32_t i=0;i<length;i++) ed_b(response[i]);
    if (!length) { ed_b(127u); ed_b(1u); }
    return 1;
}
