/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Kerem Kilic (Ellic Studio) */
/* Melodee transport adapter for Optimist's stack (see docs/BLE_MIDI.md).
 * BLE IRQ -> private RX ring -> TIMER5 -> the existing audio input ring.
 * Audio/UI -> TX ring -> BLE IRQ. Foreground mutations mask the BLE IRQs only.
 * One radio link, either role: Mode CONNECT OUT scans on the Bluetooth page and keeps the last device linked
 * (relink), Mode VISIBLE advertises for a computer or phone. No USB echo/bridge and no wireless update SysEx. */
/* Link 0 is the established controller; link 1 is a low-duty discovery experiment. */
static uint8_t mashina_hw_link;
static void mashina_proto_select(uint32_t);
#include "ble/ble_stack.c"
static void mashina_proto_select(uint32_t i)
{
    mashina_hw_link=(uint8_t)i;
    bll_ptr=&bll_bank[i];llc_ptr=&llc_bank[i];bhs_ptr=&bhs_bank[i];
    bhs_key_ptr=&bhs_key_bank[i];batt_ptr=&batt_bank[i];bsmp_ptr=&bsmp_bank[i];
    bcen_ptr=&bcen_bank[i];bgc_ptr=&bgc_bank[i];
}
#include "ble/ble_vm.c"

#define MBQ 32u
#define MBRT 16u                                /* clock, transport, Song Position (usb.c midi_rt_q) */
static struct {
    uint32_t rx[MBQ], time[MBQ], tx[MBQ], ms[MBQ];
    volatile uint32_t rw, rr, tw, tr, rt_w, rt_r;
    uint32_t rt[MBRT], rt_ms[MBRT];
    uint8_t peeked_rt;
    volatile uint8_t ready, lost, overflow;
    uint8_t on, started, cal, scanning, attempt, was_ready, connected, was_pairing, closing, forgetting;
    uint8_t boot, relink, relink_scan;          /* radio start owed from power-on; looking for the last device */
    uint8_t idx[BLE_SCAN_N], count;
    uint32_t epoch_ms, deadline, aged, generation, relink_t, probe_t;
    char shown[24];                             /* "VISIBLE 1A2B": Melodee 1A2B on the computer */
    struct ble_rf_trims trims;
    struct ble_scan_tab found;
    struct ble_dev_store store;
    struct ble_found target;
    struct ble_keys keys;
    volatile uint8_t keys_new;
    char failure[24];
} mb __attribute__((section(".lowbuf"), aligned(4)));


static void mb_dirty(void) { mb_saved.magic = MEL_BLE_MAGIC; mb_revision++; }
/* TIMER5 may enter while foreground holds this lock. Keep its unlock from
 * re-enabling BLE underneath a foreground mutation. The depth itself is atomic. */
static uint32_t mb_lock_depth;
static void mb_lock(void)
{
    uint32_t ie = fm1_icfg(); fm1_irq_off();
    if (!mb_lock_depth++ && mb.started) fm1_ble_irqs_hold(1);
    fm1_icfg_set(ie);
}
static void mb_unlock(void)
{
    uint32_t ie = fm1_icfg(); fm1_irq_off();
    if (--mb_lock_depth == 0u && mb.started) fm1_ble_irqs_hold(0);
    fm1_icfg_set(ie);
}

BLE_API int ble_app_midi_peek(uint32_t *pkt, uint32_t *ms)
{
    mb.peeked_rt = mb.rt_r != mb.rt_w;
    if (mb.peeked_rt) {
        RING_PUBLISH();
        *pkt = mb.rt[mb.rt_r % MBRT]; *ms = mb.rt_ms[mb.rt_r % MBRT];
        return 1;
    }
    if (mb.tr == mb.tw) return 0;
    RING_PUBLISH();
    *pkt = mb.tx[mb.tr % MBQ]; *ms = mb.ms[mb.tr % MBQ];
    return 1;
}
BLE_API void ble_app_midi_pop(void)                 /* (the one peeked: a pulse may have arrived since) */
{
    RING_PUBLISH();
    if (mb.peeked_rt) mb.rt_r++;
    else mb.tr++;
}
BLE_API void ble_app_midi_in(uint32_t pkt, uint16_t ts, uint16_t last)
{
    uint32_t status = (pkt >> 8) & 255u;
    /* Route performance messages and clock only. OTA/editor framing stays USB. */
    if (!mb.on || mb.closing || !((status >= 0x80u && status < 0xF0u) ||
        status == 0xF2u || status == 0xF8u || status == 0xFAu || status == 0xFBu || status == 0xFCu)) return;
    if (mb.rw - mb.rr >= MBQ) { mb.overflow = 1; return; }
    mb.epoch_ms = fm1_ms;
    uint32_t at = mb.rw % MBQ, back = (last - ts) & 0x1FFFu;
    mb.rx[at] = pkt;
    /* Preserve relative timing within the packet without trusting the peer's epoch. */
    mb.time[at] = fm1_ms - (back < 1000u ? back : 0u);
    RING_PUBLISH(); mb.rw++;
}
BLE_API void ble_app_state(void)
{
    uint8_t ready = (uint8_t)(ble_midi_ready() && !mb.closing);
    uint8_t connected = (uint8_t)ble_connected();
    if ((mb.ready && !ready) || (mb.connected && !connected)) mb.lost = 1;
    if (!connected) {
        mb.closing = 0;
        if (mb.forgetting) { memset(&bhs_key, 0, sizeof bhs_key); mb.forgetting = 0; }
    }
    if(ready && !mb.ready)mb.probe_t=fm1_ms;
    mb.connected = connected;
    mb.ready = ready;
    if (!ready) { mb.tr = mb.tw; mb.rt_r = mb.rt_w; }
}
BLE_API void ble_app_bond(const uint8_t rand[8], uint16_t ediv, const uint8_t ltk[16])
{
    if (!mb.on || mb.closing) return;
    mb_saved.bond[0] = 0xB5u; mb_saved.bond[1] = 1;
    ble_cpy(mb_saved.bond + 2, rand, 8);
    ble_wr16(mb_saved.bond + 10, ediv);
    ble_cpy(mb_saved.bond + 12, ltk, 16); mb_dirty();
}
BLE_API void ble_app_peer_id(const uint8_t irk[16], const uint8_t addr[6], uint8_t rnd)
{ (void)irk; (void)addr; (void)rnd; }
BLE_API void ble_app_central_keys(const struct ble_keys *k)
{
    if (!mb.on || mb.closing || !mb.attempt) return;
    mb.keys = *k; RING_PUBLISH(); mb.keys_new = 1;
}

static int mel_ble_ready(void) { return mb.ready && mb.on; }
static int mel_ble_room(void) { return mb.tw - mb.tr < MBQ; }
static void mel_ble_sync(uint32_t pkt, uint32_t pulse)
{
    if (!mb.ready || !mb.on || mb.rt_w - mb.rt_r >= (pulse ? MBRT - 4u : MBRT)) return;
    mb.rt[mb.rt_w % MBRT] = pkt; mb.rt_ms[mb.rt_w % MBRT] = fm1_ms;
    RING_PUBLISH(); mb.rt_w++;
}
static int mel_ble_emit(uint32_t pkt)
{
    if (!mb.ready || !mb.on) return 0;
    /* All callers are serialized against BLE consumption. Refuse admission
     * before other transports accept the packet, so retries cannot duplicate it. */
    if (mb.tw - mb.tr >= MBQ) return 0;
    mb.tx[mb.tw % MBQ] = pkt; mb.ms[mb.tw % MBQ] = fm1_ms;
    RING_PUBLISH(); mb.tw++;
    return 1;
}

static void mel_ble_drain(void) /* TIMER5, the shared input queue's sole producer context */
{
    if (mb.lost || mb.overflow) {
        mb_lock(); mb.rr = mb.rw; mb.lost = mb.overflow = 0; mb_unlock();
        /* Preserve wired owners. A full shared queue still invokes its
         * existing global overflow recovery rather than losing the marker. */
        (void)midi_enqueue(0,3u);
    }
    for (uint32_t n = 0; mb.rr != mb.rw && n < MBQ; n++) {
        RING_PUBLISH(); uint32_t at = mb.rr % MBQ;
        if (!midi_enqueue(mb.rx[at], 3u)) { mb.rr = mb.rw; midi_in_overflow = 1; break; }
        midi_in_ms[(mi_w - 1u) % MQ] = mb.time[at];
        mb.rr++;
    }
}

static void mel_ble_boot(void) /* CPU1 held, IRQs off; does not start the radio */
{
    struct ble_vm_info info;
    struct ble_rf_trims vm;
    int save = 0;
    if (mb_saved.magic != MEL_BLE_MAGIC) memset(&mb_saved, 0, sizeof mb_saved);
    int complete = ble_vm_scan(mel_ble_flash_read, 0, &info, &vm, 0, 0);
    mb.cal = (uint8_t)(ble_rf_choose(complete, &vm, info.area, mb_saved.rf, &mb.trims, &save) != BLE_RF_NONE);
    if (save) mb_dirty();
    if (!mb.cal) {
        /* No stored calibration: the radio measures its own at start (hal/fm1_rf_cal.h, JieLi's SDK calibration
         * in C). 106 / 107 / 108 are the SDK board's constants; 187 is measured, about 160 ms. */
        static const struct ble_rf_trims sdk = {{0x0B, 0x0B}, {1, 7, 4, 7, 11, 1, 7}, {0}, {0}};
        mb.trims = sdk;
        fm1_rf_cal_on = 1;
        mb.cal = 1;                           /* (a saved calibration imported later still replaces it) */
    }
    ble_store_load(&mb.store, mb_saved.dev);
    ble_scan_clear(&mb.found);
    mb.boot = (uint8_t)!!(mb_saved.flags & MB_RADIO);   /* (ble_boot_poll starts it once quiet) */
    ble_hw_wl82_attach(); fm1_ble_crumb_boot();
}

/* Relink: a round every MB_RELINK_MS looks for the last device up to MB_RELINK_SCAN_MS and connects when it is seen
 * (a private address resolves only from a scan). Never a new pairing loop: a refusal ends it, as does Disconnect. */
#define MB_RELINK_MS 30000u
#define MB_RELINK_SCAN_MS 10000u
static void mb_relink(int on)
{
    mb.relink = (uint8_t)(on && !(mb_saved.flags & MB_VISIBLE) && ble_store_has_last(&mb.store));
    mb.relink_scan = 0; mb.relink_t = fm1_ms - MB_RELINK_MS;
}

/* Called with BLE IRQs held. Keep link 0's host/LL state untouched.
 * A low-duty active scan on a separate hardware instance tests radio coexistence. */
static void mb_aux_scan(int on)
{
    mashina_proto_select(1);
    if(on && drv.state!=HW_SCAN){
        ble_scan_clear(&mb.found);mb.count=0;bscan.r=bscan.w;
        struct ble_hw_scan s;
        s.interval=160u; s.window=8u; s.active=1;
        s.own=mb_saved.addr; s.own_rand=1;
        bll.state=LL_SCAN;
        ble_hw_scan_start(&s);
    }else if(!on && drv.state==HW_SCAN){
        ble_hw_scan_stop();bll.state=LL_OFF;
        bb.inst[1]=0;
    }
    mashina_proto_select(0);
}
static int mel_ble_set(int on)
{
    mb.boot = 0;
    if (on && (!mb.cal)) return 0;     /* (RES_OK: the radio program, res.c) */
    if (on && !mb.started) {
        if (!mb_saved.valid) {
            for (uint32_t i = 0; i < 6u; i++) mb_saved.addr[i] = (uint8_t)fm1_ble_rng32();
            mb_saved.addr[5] |= 0xC0u; mb_saved.valid = 1; mb_dirty();
        }
        /* Complete pending display DMA before radio setup; feed the watchdog
         * ahead of calibration and park any second-core audio job. */
        mel_ble_radio_start(&mb.trims);
        mashina_proto_select(1);ble_init(mb_saved.addr,1);mashina_proto_select(0);
        ble_init(mb_saved.addr, 1);
        ble_ll_advertise(0);
        if (mb_saved.bond[0] == 0xB5u && mb_saved.bond[1] == 1)
            ble_host_set_key(mb_saved.bond + 2, ble_rd16(mb_saved.bond + 10), mb_saved.bond + 12);
        mb.started = 1;
    }
    uint8_t f = (uint8_t)(on ? mb_saved.flags | MB_RADIO : mb_saved.flags & ~MB_RADIO);
    if (f != mb_saved.flags) { mb_saved.flags = f; mb_dirty(); }
    mb_lock();
    mb.on = (uint8_t)!!on;
    if (!on) { if(mb.started)mb_aux_scan(0); mb.scanning = mb.attempt = mb.relink_scan = 0; mb.lost = 1; mb.ready=0; }
    if (mb.started) ble_enable(on);
    mb_unlock(); mb.generation++;
    if(!on){ble_scan_clear(&mb.found);mb.count=0;}
    mb_relink(on);
    return 1;
}

static void mel_ble_scan(int on)
{
    mb_lock();
    if(mb.started && mb.ready){
        mb.scanning=(uint8_t)(on && mb.on);
        mb_aux_scan(mb.scanning);
    }else{
        if(mb.started)mb_aux_scan(0);
        mb.scanning = (uint8_t)(on && mb.on && !(mb_saved.flags & MB_VISIBLE) && !mb.attempt && !ble_connected());
        if (mb.started) ble_ll_scan(mb.scanning);
    }
    mb_unlock();
}
/* Mode: VISIBLE advertises (no scan, no relink), CONNECT OUT stays quiet unless scanning or connecting out.
 * A link up stays up; the mode applies when it ends. */
static void mel_ble_visible(int on)
{
    uint8_t f = (uint8_t)(on ? mb_saved.flags | MB_VISIBLE : mb_saved.flags & ~MB_VISIBLE);
    if (f == mb_saved.flags) return;
    mb_saved.flags = f; mb_dirty();
    mb_lock();
    mb.failure[0] = 0;
    if (on) mb.scanning = 0;
    if (mb.started) { ble_ll_advertise(on); if (on) ble_ll_scan(0); }
    mb_unlock(); mb.generation++;
    mb_relink(mb.on);
}
static void mb_drop(void)
{
    mb_lock();
    if(mb.started)mb_aux_scan(0);
    mb.scanning=0;
    mb.closing = (uint8_t)ble_connected(); mb.ready = 0; mb.tr = mb.tw; mb.rt_r = mb.rt_w;
    if (mb.started) { ble_enable(0); ble_enable(mb.on); }
    mb.attempt = 0; mb.lost = 1;
    mb.failure[0] = 0; mb.generation++;
    mb_unlock();
}
static void mel_ble_disconnect(void) { mb.relink = 0; mb_drop(); }
static void mel_ble_forget(void)
{
    mel_ble_disconnect();
    mb_lock();
    mb.forgetting = mb.closing;
    memset(mb_saved.bond, 0, sizeof mb_saved.bond);
    ble_store_forget(&mb.store); ble_store_save(&mb.store, mb_saved.dev);
    if (mb.started) { memset(&bhs_key, 0, sizeof bhs_key); ble_smp_reset(); }
    mb_dirty(); mb_unlock();
}

static int mb_same_peer(const uint8_t addr[6], uint8_t rnd)
{
    const struct ble_dev *d = &mb.store.dev;
    return ble_store_has_last(&mb.store) &&
        ((((d->info & BLE_DEV_RANDOM) != 0) == rnd && !memcmp(addr, d->addr, 6)) ||
         (rnd && (d->info & BLE_DEV_IRK) && ble_rpa_resolve(d->irk, addr)));
}
static void mb_remember(void)
{
    if (!mb_same_peer(mb.target.addr, mb.target.addr_rand))
        ble_store_set_last(&mb.store, mb.target.addr, mb.target.addr_rand, mb.target.name, mb.target.kind);
}
static int mb_connect_target(void)
{
    struct ble_peer peer;
    memset(&peer, 0, sizeof peer);
    memcpy(peer.addr, mb.target.addr, 6); peer.addr_rand = mb.target.addr_rand;
    struct ble_dev *d = &mb.store.dev;
    if (mb_same_peer(peer.addr, peer.addr_rand)) {
        peer.bonded = !!(d->info & BLE_DEV_BONDED);
        memcpy(peer.ltk, d->ltk, 16); memcpy(peer.rand, d->rand, 8); peer.ediv = ble_rd16(d->ediv);
        peer.sec = d->sec;
    }
    mb.failure[0] = 0; mb.keys_new = 0; memset(&mb.keys, 0, sizeof mb.keys);
    mb_lock(); int ok = ble_central_connect(&peer); mb_unlock();
    if (ok) { mb.attempt = 1; mb.was_ready = mb.was_pairing = 0; mb.deadline = fm1_ms; mb.generation++; }
    return ok;
}
static int mel_ble_connect(uint32_t row)
{
    if (!mb.on || row >= mb.count || ble_connected() || mb.attempt) return 0;
    mb.target = mb.found.e[mb.idx[row]];
    return mb_connect_target();
}
static int mel_ble_last(void)
{
    if (!mb.on || !ble_store_has_last(&mb.store) || ble_connected() || mb.attempt) return 0;
    memset(&mb.target, 0, sizeof mb.target);
    memcpy(mb.target.addr, mb.store.dev.addr, 6);
    mb.target.addr_rand = !!(mb.store.dev.info & BLE_DEV_RANDOM);
    memcpy(mb.target.name, mb.store.dev.name, BLE_NAME_MAX);
    mb.target.kind = BLE_KIND_MIDI;
    /* Resolve a remembered private address from the live scan table. No automatic pairing loops. */
    if (mb.store.dev.info & BLE_DEV_IRK)
        for (uint32_t i = 0; i < mb.count; i++) {
            struct ble_found *f = &mb.found.e[mb.idx[i]];
            if (f->addr_rand && ble_rpa_resolve(mb.store.dev.irk, f->addr)) { mb.target = *f; break; }
        }
    return mb_connect_target();
}

static void mb_relink_poll(void)
{
    if (!mb.relink || !mb.on || mb.attempt || ble_connected() || mb.scanning) {
        if (mb.relink_scan && !mb.scanning && mb.started) { mb_lock(); ble_ll_scan(0); mb_unlock(); }   /* (the page's own) */
        mb.relink_scan = 0;
        return;
    }
    uint32_t t = fm1_ms - mb.relink_t;
    if (t >= MB_RELINK_MS) {
        mb.relink_t = fm1_ms; t = 0; mb.relink_scan = 1;
        mb_lock(); ble_ll_scan(1); mb_unlock();
    }
    if (!mb.relink_scan) return;
    for (uint32_t i = 0; i < mb.count; i++) {
        const struct ble_found *e = &mb.found.e[mb.idx[i]];
        if (mb_same_peer(e->addr, e->addr_rand)) {
            mb.relink_scan = 0;
            mb_lock(); ble_ll_scan(0); mb_unlock();
            mb.target = *e; (void)mb_connect_target();
            return;
        }
    }
    if (t >= MB_RELINK_SCAN_MS) { mb.relink_scan = 0; mb_lock(); ble_ll_scan(0); mb_unlock(); }
}

static uint8_t mb_start_req;
static void mel_ble_poll(void)
{
    if(mb_start_req){mb_start_req=0;mel_ble_set(1);ui.force=1;}
    if (!mb.started) return;
    if(mb.connected){
        mb_lock();
        if(bll.state==LL_CONN){
            if(bgc.probe==2u){bgc.probe=0;mb.probe_t=fm1_ms;}
            /* The baseband event counter may advance without a live peer.
             * An ATT response proves that the remote device is still alive,
             * even when it is idle and sends no MIDI. A read does not alter
             * the peer or decode its value as a performance message. */
            uint32_t elapsed=fm1_ms-mb.probe_t;
            if(ble_hw_time_us()-bll.t_rx>(uint32_t)bll.timeout*10000u ||
                (mb.ready && elapsed>=6000u))
                ll_close_by(BLE_ERR_CONN_TIMEOUT,BDC_SUPERVISION);
            else if(mb.ready && elapsed>=2000u && !bgc.probe)
                gc_probe();
        }
        mb_unlock();
    }
    if(!mb.ready && mashina_drv[1].state==HW_SCAN){mb_lock();mb_aux_scan(0);mb.scanning=0;mb_unlock();}
    uint8_t pdu[39]; uint16_t rssi; uint32_t n;
    for (uint32_t i = 0; i < BLE_SCAN_RING && (n = ble_ll_scan_take(pdu, &rssi)); i++)
        ble_scan_add(&mb.found, pdu, n, rssi, fm1_ms);
    if (fm1_ms - mb.aged >= 1000u) { mb.aged = fm1_ms; ble_scan_age(&mb.found, fm1_ms); }
    mb.count = (uint8_t)ble_scan_list(&mb.found, mb.idx);
    if (mb.attempt) {
        uint8_t state = ble_central_state(), pairing = (uint8_t)ble_central_pairing();
        if (pairing != mb.was_pairing) { mb.was_pairing = pairing; mb.deadline = fm1_ms; }
        if (mb.keys_new) {
            mb_lock(); struct ble_keys k = mb.keys; mb.keys_new = 0; mb_unlock();
            mb_remember();
            if (k.has & BLE_KEYS_LTK) ble_store_set_bond(&mb.store, k.ltk, k.rand, k.ediv, k.has & BLE_KEYS_AUTH);
            if (k.has & BLE_KEYS_ID) ble_store_set_id(&mb.store, k.irk, k.id, k.id_rand);
            ble_store_save(&mb.store, mb_saved.dev); mb_dirty();
        }
        if (state == BLE_CS_READY && !mb.was_ready) {
            mb.was_ready = 1;
            mb.relink = (uint8_t)!(mb_saved.flags & MB_VISIBLE); mb.relink_t = fm1_ms;   /* (kept linked from now) */
            mb_remember();
            ble_store_save(&mb.store, mb_saved.dev); mb_dirty(); mb.generation++;
        } else if (state == BLE_CS_IDLE) {
            uint32_t why = ble_central_fail();
            if (why == BLE_CF_KEY) { ble_store_drop_bond(&mb.store); ble_store_save(&mb.store, mb_saved.dev); mb_dirty(); }
            if (why != BLE_CF_LOST && why != BLE_CF_NONE) mb.relink = 0;   /* (gone: looked for again; refused: not) */
            if (!mb.relink) str_cpy(mb.failure, why == BLE_CF_KEY ? "FORGET ON PEER" : why == BLE_CF_AUTH || why == BLE_CF_PAIRING ?
                "PAIRING FAILED" : why == BLE_CF_NO_MIDI ? "NO MIDI SERVICE" : "DISCONNECTED", sizeof mb.failure);
            mb.attempt = 0; mb.generation++;
        } else if (!mb.was_ready && fm1_ms - mb.deadline > (ble_central_pairing() ? 45000u : 10000u)) {
            if (ble_central_pairing()) mb.relink = 0;   /* (a code nobody entered: no second prompt) */
            mb_drop();
            if (!mb.relink) str_cpy(mb.failure, "CONNECTION TIMEOUT", sizeof mb.failure);
        }
    }
    mb_relink_poll();
}
static uint32_t mel_ble_code(void)
{ return mb.on && mb.attempt ? ble_central_passkey() : BLE_NO_PASSKEY; }
static const char *mel_ble_status(void)
{
    if (!mb.cal) return "NO CAL";
    if (!mb.on) return "BLE OFF";
    if (mel_ble_code() != BLE_NO_PASSKEY) return "PAIRING CODE";
    if (mb.ready) return "BLE CONNECTED";
    if (mb.failure[0]) return mb.failure;
    if (mb.attempt) return ble_central_pairing() ? "PAIRING" : mb.relink ? "RECONNECTING" : "FINDING MIDI";
    if (ble_connected()) return "WAITING FOR MIDI";
    if (mb.scanning) return "SCANNING";
    if (mb.relink) return "RECONNECTING";
    if (!(mb_saved.flags & MB_VISIBLE)) return "BLE AVAILABLE";
    const uint8_t *p; uint32_t n = ble_gap_name(&p), i = n;
    while (i && p[i - 1u] != ' ') i--;          /* (the header has room for the name's address digits only) */
    if (n - i > sizeof mb.shown - 9u) n = i + sizeof mb.shown - 9u;
    str_cpy(mb.shown, "VISIBLE ", sizeof mb.shown);
    memcpy(mb.shown + 8, p + i, n - i); mb.shown[8u + n - i] = 0;
    return mb.shown;
}
static uint32_t mel_ble_signature(void)
{ return mb.generation + mb.found.gen * 13u + (uint32_t)mb.ready * 997u + mel_ble_code() * 17u + (uint32_t)ble_central_state() * 65537u; }

/* USB maintenance only: restore a checked trim copy, never a raw flash sector.
 * Existing valid calibration cannot be replaced by a different unit's copy. */
static uint32_t mel_ble_import_cal(const uint8_t copy[BLE_RF_COPY_SIZE])
{
    if (!ble_rf_copy_ok(copy)) return 1;
    if (mb.on || mb.closing) return 4;
    if (mb.cal && !fm1_rf_cal_on && memcmp(mb_saved.rf, copy, BLE_RF_COPY_SIZE)) return 2;
    if (mb.cal && !fm1_rf_cal_on) return 0;
    memcpy(mb_saved.rf, copy, BLE_RF_COPY_SIZE);
    ble_rf_copy_get(copy, &mb.trims);
    fm1_rf_cal_on = 0;                        /* (the saved one instead of the measurement) */
    mb.cal = 1; mb_dirty(); mb.generation++;
    return 0;
}

/* MASHINA: read-only stock RF trim discovery; never write calibration sectors. */
static int mel_ble_flash_read(void *ctx, uint32_t off, uint8_t *dst, uint32_t n)
{
    (void)ctx;
    if (!flash_ok || off > 0x100000u || n > 0x100000u-off) return -1;
    return st_read(off,dst,n);
}
static void mel_ble_radio_start(const struct ble_rf_trims *t)
{
    felucca_dbg.stage=0xB0u;
    lcd_sync();
    fm1_wdt_feed();
    uint32_t ie=fm1_icfg(); fm1_irq_off();
#if FELUCCA_DUAL_CORE
    fm1_mc_park();
#endif
    felucca_dbg.stage=0xB1u;
    audio_silence();
    ble_hw_wl82_start(t);
    fm1_wdt_feed();
    fm1_icfg_set(ie);
    felucca_dbg.stage=0xB2u;
}

/* GLO > SYSTEM > BLUETOOTH. Explicit radio enable, one MIDI peer.
 * All controls run in the foreground. The audio input path is unchanged. */
static uint8_t mb_pick __attribute__((section(".lowbuf")));
static void ble_page_knob(uint32_t k,int32_t steps)
{
    int right=steps>0;
    if(k==0u){
        if(!right){mb_start_req=0;mel_ble_set(0);}
        else if(!transport_busy()){
            if(!mb.started)mb_start_req=1;
            else mel_ble_set(1);
        }
    }
    else if(k==1u){mel_ble_scan(right);if(right)mb_pick=0;}
    else if(k==2u && mb.count && !mb.ready){mb_pick=(uint8_t)clamp((int32_t)mb_pick+(right?1:-1),0,mb.count-1u);}
    else if(k==3u){if(right && !mb.ready && mb_pick<mb.count){mel_ble_scan(0);mel_ble_connect(mb_pick);}else if(!right)mel_ble_disconnect();}
    ui.force=1;
}
/* Standard 240x240 MASHINA table layout. Split canvas to stay below
 * CV_MAX. Redraw only changed blocks; scanning must not flash the whole LCD. */
static void ble_page_draw(void)
{
    static uint32_t prev_head,prev_rows[4],prev_bottom;
    const uint32_t full=ui.force;
    uint32_t code=mel_ble_code();
    const uint32_t linked=mb.ready;
    if(linked)mb_pick=0;
    else if(mb_pick>=mb.count)mb_pick=mb.count?mb.count-1u:0u;
    const char *status=mel_ble_status();
    uint32_t head=mb.on | (uint32_t)mb.scanning<<1 | (uint32_t)mb.ready<<2;
    for(const char *p=status;*p;p++)head=head*31u+(uint8_t)*p;
    const char *peer=mb.ready?(mb.target.name[0]?mb.target.name:"MIDI DEVICE"):"";
    for(const char *p=peer;*p;p++)head=head*31u+(uint8_t)*p;
    if(full || head!=prev_head){
        cv_begin(240,74,T_BG);
        cv_text(4,2,&AF_S,"K1 RADIO",T_TEXT);
        cv_text_r(115,2,&AF_M,mb.on?"ON":"OFF",T_VALUE,T_BG);
        cv_text(124,2,&AF_S,"K2 SCAN",T_TEXT);
        cv_text_r(236,2,&AF_M,mb.scanning?"ON":"OFF",T_VALUE,T_BG);
        cv_rect(0,24,240,1,RGB(58,58,58));
        cv_text(4,29,&AF_S,"STATUS",T_TEXT);
        cv_text_r(236,29,&AF_M,status,T_VALUE,T_BG);
        cv_free_text(4,48,&AF_M,peer,T_TEXT,T_BG,232);
        cv_blit(0,0);
        prev_head=head;
    }
    if(full){
        cv_begin(240,18,RGB(58,58,58));
        cv_text(4,2,&AF_S,"NO.",T_TEXT);
        cv_text(34,2,&AF_S,"MIDI DEVICE",T_TEXT);
        cv_text_r(236,2,&AF_S,"SIGNAL",T_TEXT,RGB(58,58,58));
        cv_blit(0,74);
    }
    for(uint32_t i=0;i<4u;i++){
        uint32_t row=mb_pick/4u*4u+i,selected=row==mb_pick && row<mb.count;
        const struct ble_found *f=linked?(i==0?&mb.target:0):(row<mb.count?&mb.found.e[mb.idx[row]]:0);
        uint32_t auxidx=0;
        if(linked && i && mb.scanning){
            uint32_t n=0;
            for(uint32_t j=0;j<mb.count;j++){
                const struct ble_found *q=&mb.found.e[mb.idx[j]];
                if(q->addr_rand==mb.target.addr_rand && !memcmp(q->addr,mb.target.addr,6))continue;
                if(++n==i){f=q;auxidx=mb.idx[j];break;}
            }
        }
        const char *name=f?(f->name[0]?f->name:"UNNAMED"):(!i && !mb.count&&!linked?(mb.on?"NO DEVICES FOUND":"RADIO OFF"):"");
        uint32_t bars=f&&(!linked||i)?ble_scan_bars(&mb.found,linked?auxidx:mb.idx[row]):0u;
        if(linked)selected=i==0;
        uint32_t sig=1u+row*8u+selected*4u+bars+linked*512u;
        for(const char *p=name;*p;p++)sig=sig*31u+(uint8_t)*p;
        if(!full && sig==prev_rows[i])continue;
        prev_rows[i]=sig;
        uint16_t bg=selected?T_VALUE:(i&1u?RGB(23,23,23):RGB(36,36,36));
        uint16_t ink=selected?T_INK:T_TEXT;
        cv_begin(240,24,bg);
        if(f){
            char num[4];fmt_int(num,row+1u);
            cv_text(4,CAP_IN(M,24),&AF_M,num,ink);
            if(linked && !i)cv_text_r(236,CAP_IN(S,24),&AF_S,"LINKED",ink,bg);
            else for(uint32_t j=0;j<3u;j++)
                cv_rect(211+j*8u,18-(j+1u)*4u,5,(j+1u)*4u,j<bars?(selected?T_INK:T_VALUE):RGB(70,70,70));
        }
        cv_free_text(34,CAP_IN(M,24),&AF_M,name,ink,bg,linked&&!i?140:172);
        cv_blit(0,92+i*24u);
    }
    uint32_t bottom=code^(linked?1u<<30:0u);
    if(full || bottom!=prev_bottom){
        cv_begin(240,34,T_BG);
        if(code!=BLE_NO_PASSKEY){
            char pass[7];pass[6]=0;
            for(int i=5;i>=0;i--){pass[i]=(char)('0'+code%10u);code/=10u;}
            cv_text(4,1,&AF_S,"PAIRING CODE",T_TEXT);
            cv_text_r(236,1,&AF_M,pass,T_VALUE,T_BG);
        }else{
            cv_text(4,1,&AF_S,linked?"K2 SCAN (LINK TEST)":"K3 SELECT DEVICE",T_TEXT);
            cv_text(4,17,&AF_S,"K4 CONNECT > / DROP <",T_TEXT);
        }
        cv_blit(0,188);
        prev_bottom=bottom;
    }
    if(full){
        cv_begin(240,18,T_BG);
        cv_rect(3,1,43,16,T_VALUE);
        cv_text_in(5,1,39,&AF_S,"GLO",T_INK,T_VALUE);
        cv_rect(48,1,65,16,RGB(58,58,58));
        cv_text_in(51,1,59,&AF_S,"SYSTEM",T_TEXT,RGB(58,58,58));
        cv_text(117,1,&AF_S,"BLUETOOTH",T_VALUE);
        cv_text_r(237,1,&AF_S,"8/8",T_VALUE,T_BG);
        cv_blit(0,222);
    }
    ui.force=0;
}
