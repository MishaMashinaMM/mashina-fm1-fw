/* SPDX-License-Identifier: GPL-3.0-only */
/* This handler is only reached when the explicit experimental feature is ON.
 * Writes still fail closed unless FELUCCA_LIB32_WRITE=1 was set for the build. */
static int ed_library_handle(uint32_t cmd, const uint8_t *args, uint32_t n)
{
    uint8_t response[LIB_WIRE_MAX];
    uint32_t length;
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
    if (cmd == LIB_WIRE_CAP && length == 3u) {
#if FELUCCA_LIB32_WRITE
        response[2] = flash_ok ? 1u : 0u;
#else
        response[2] = 0u;
#endif
    }
    for(uint32_t i=0;i<length;i++) ed_b(response[i]);
    if (!length) { ed_b(127u); ed_b(1u); }
    return 1;
}
