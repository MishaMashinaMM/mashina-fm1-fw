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
#define MBRT 16u
/* Separate transport queues, keys, and liveness for each peer. The scan
 * catalogue and RF calibration are shared; neither is part of link state. */
typedef struct {
    uint32_t rx[MBQ], time[MBQ], tx[MBQ], ms[MBQ], rt[MBRT], rt_ms[MBRT];
    volatile uint32_t rw,rr,tw,tr,rt_w,rt_r;
    volatile uint8_t ready,lost,overflow;
    uint8_t peeked_rt,busy,connected,closing,was_pairing,was_ready,relink,keys_new;
    uint32_t deadline,probe_t,relink_t;
    struct ble_dev_store store;
    struct ble_found target;
    struct ble_keys keys;
} mb_peer_t;
static mb_peer_t mb_peer[2] __attribute__((section(".lowbuf"),aligned(4)));
#define MP (mb_peer[mashina_hw_link])
static struct {
    struct ble_rf_trims trims;
    struct ble_scan_tab found;
    uint32_t aged;
    uint8_t idx[BLE_SCAN_N],count,on,started,cal,scanning,scan_slot,start_req,pick;
} mb __attribute__((section(".lowbuf"),aligned(4)));

static void mb_dirty(void){mb_saved.magic=MEL_BLE_MAGIC;mb_revision++;}
static uint32_t mb_lock_depth;
static void mb_lock(void)
{
    uint32_t ie=fm1_icfg();fm1_irq_off();
    if(!mb_lock_depth++ && mb.started)fm1_ble_irqs_hold(1);
    fm1_icfg_set(ie);
}
static void mb_unlock(void)
{
    uint32_t ie=fm1_icfg();fm1_irq_off();
    if(--mb_lock_depth==0u && mb.started)fm1_ble_irqs_hold(0);
    fm1_icfg_set(ie);
}

BLE_API int ble_app_midi_peek(uint32_t *pkt,uint32_t *ms)
{
    mb_peer_t *p=&MP;
    p->peeked_rt=p->rt_r!=p->rt_w;
    if(p->peeked_rt){RING_PUBLISH();*pkt=p->rt[p->rt_r%MBRT];*ms=p->rt_ms[p->rt_r%MBRT];return 1;}
    if(p->tr==p->tw)return 0;
    RING_PUBLISH();*pkt=p->tx[p->tr%MBQ];*ms=p->ms[p->tr%MBQ];return 1;
}
BLE_API void ble_app_midi_pop(void)
{
    RING_PUBLISH();if(MP.peeked_rt)MP.rt_r++;else MP.tr++;
}
BLE_API void ble_app_midi_in(uint32_t pkt,uint16_t ts,uint16_t last)
{
    mb_peer_t *p=&MP;
    uint32_t st=(pkt>>8)&255u;
    if(!mb.on||p->closing||!((st>=0x80u&&st<0xF0u)||st==0xF2u||st==0xF8u||st==0xFAu||st==0xFBu||st==0xFCu))return;
    if(p->rw-p->rr>=MBQ){p->overflow=1;return;}
    uint32_t at=p->rw%MBQ,back=(last-ts)&0x1FFFu;
    p->rx[at]=pkt;p->time[at]=fm1_ms-(back<1000u?back:0u);
    RING_PUBLISH();p->rw++;
}
BLE_API void ble_app_state(void)
{
    mb_peer_t *p=&MP;
    uint8_t ready=ble_midi_ready()&&!p->closing,connected=ble_connected();
    if((p->ready&&!ready)||(p->connected&&!connected))p->lost=1;
    if(!connected)p->closing=0;
    if(ready&&!p->ready)p->probe_t=fm1_ms;
    p->connected=connected;p->ready=ready;
    if(!ready){p->tr=p->tw;p->rt_r=p->rt_w;}
}
BLE_API void ble_app_bond(const uint8_t rand[8],uint16_t ediv,const uint8_t ltk[16])
{ (void)rand;(void)ediv;(void)ltk; } /* peripheral role is not offered by MASHINA */
BLE_API void ble_app_peer_id(const uint8_t irk[16],const uint8_t addr[6],uint8_t rnd)
{(void)irk;(void)addr;(void)rnd;}
BLE_API void ble_app_central_keys(const struct ble_keys *k)
{
    if(!mb.on||MP.closing||!MP.busy)return;
    MP.keys=*k;RING_PUBLISH();MP.keys_new=1;
}
static int mel_ble_ready(void){return mb.on&&(mb_peer[0].ready||mb_peer[1].ready);}
static int mel_ble_room(void)
{
    for(uint32_t i=0;i<2u;i++)if(mb_peer[i].ready&&mb_peer[i].tw-mb_peer[i].tr>=MBQ)return 0;
    return 1;
}
static void mel_ble_sync(uint32_t pkt,uint32_t pulse)
{
    for(uint32_t i=0;i<2u;i++){
        mb_peer_t *p=&mb_peer[i];
        if(!mb.on||!p->ready||p->rt_w-p->rt_r>=(pulse?MBRT-4u:MBRT))continue;
        p->rt[p->rt_w%MBRT]=pkt;p->rt_ms[p->rt_w%MBRT]=fm1_ms;RING_PUBLISH();p->rt_w++;
    }
}
static int mel_ble_emit(uint32_t pkt)
{
    if(!mel_ble_ready()||!mel_ble_room())return 0;
    for(uint32_t i=0;i<2u;i++){
        mb_peer_t *p=&mb_peer[i];if(!p->ready)continue;
        p->tx[p->tw%MBQ]=pkt;p->ms[p->tw%MBQ]=fm1_ms;RING_PUBLISH();p->tw++;
    }
    return 1;
}
static void mel_ble_drain(void)
{
    /* TIMER5 must not use MP: it can preempt either BLE context. */
    for(uint32_t i=0;i<2u;i++){
        mb_peer_t *p=&mb_peer[i];
        if(p->lost||p->overflow){
            mb_lock();p->rr=p->rw;p->lost=p->overflow=0;mb_unlock();
            (void)midi_enqueue(0,3u+i);
        }
        for(uint32_t n=0;p->rr!=p->rw&&n<MBQ;n++){
            RING_PUBLISH();uint32_t at=p->rr%MBQ;
            if(!midi_enqueue(p->rx[at],3u+i)){p->rr=p->rw;midi_in_overflow=1;break;}
            midi_in_ms[(mi_w-1u)%MQ]=p->time[at];p->rr++;
        }
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
    ble_store_load(&mb_peer[0].store, mb_saved.dev);
    ble_scan_clear(&mb.found);

    ble_hw_wl82_attach(); fm1_ble_crumb_boot();
}


/* All protocol operations below execute with BLE interrupts held; select
 * link 0 before releasing that lock. The per-link IRQ dispatcher restores
 * the previously selected context. A scan uses one otherwise free link. */
static int mb_same(const struct ble_dev_store *s,const uint8_t *addr,uint32_t rnd)
{
    const struct ble_dev *d=&s->dev;
    return ble_store_has_last(s)&&
        ((((d->info&BLE_DEV_RANDOM)!=0)==rnd&&!memcmp(addr,d->addr,6))||
          (rnd&&(d->info&BLE_DEV_IRK)&&ble_rpa_resolve(d->irk,addr)));
}
static int mb_free(void)
{
    for(int i=0;i<2;i++)if(!mb_peer[i].busy&&!mb_peer[i].closing)return i;
    return -1;
}
static int mb_used(const struct ble_found *f)
{
    for(int i=0;i<2;i++){
        mb_peer_t *p=&mb_peer[i];
        if(p->busy&&((f->addr_rand==p->target.addr_rand&&!memcmp(f->addr,p->target.addr,6))||
                      mb_same(&p->store,f->addr,f->addr_rand)))return 1;
    }
    return 0;
}
static void mb_scan_stop(void)
{
    if(!mb.scanning)return;
    mashina_proto_select(mb.scan_slot);
    ble_ll_scan(0);mb.scanning=0;
    mashina_proto_select(0);
}
static void mb_scan_start(uint32_t slot)
{
    mb_scan_stop();ble_scan_clear(&mb.found);mb.count=0;bscan.r=bscan.w;
    mashina_proto_select(slot);
    bll.scan_want=1;bll.state=LL_SCAN;
    struct ble_hw_scan s;
    /* Preserve the tested 5 ms / 100 ms auxiliary scan while another
     * controller is connected. Otherwise use the original scan geometry. */
    s.interval=mel_ble_ready()?160u:BLE_SCAN_INTERVAL;
    s.window=mel_ble_ready()?8u:BLE_SCAN_WINDOW;
    s.active=1;s.own=bll.addr;s.own_rand=1;
    ble_hw_scan_start(&s);
    mb.scan_slot=(uint8_t)slot;mb.scanning=1;mashina_proto_select(0);
}
static void mb_drop_peer(uint32_t slot,int forget)
{
    mashina_proto_select(slot);
    mb_peer_t *p=&MP;
    p->relink=0;p->closing=ble_connected();
    p->ready=0;p->lost=1;p->busy=0;
    if(bll.state==LL_CONN)ll_close_by(BLE_ERR_REMOTE_USER,BDC_LOCAL);
    else if(bll.state==LL_INIT)ble_central_cancel();
    else if(bll.state==LL_SCAN)ble_ll_scan(0);
    p->connected=p->closing=0;
    bcen.st=BLE_CS_IDLE;
    if(forget){ble_store_forget(&p->store);if(!slot){ble_store_save(&p->store,mb_saved.dev);mb_dirty();}}
    mashina_proto_select(0);
}
static int mel_ble_set(int on)
{
    if(on&&!mb.cal)return 0;
    if(on&&!mb.started){
        if(!mb_saved.valid){
            for(uint32_t i=0;i<6u;i++)mb_saved.addr[i]=(uint8_t)fm1_ble_rng32();
            mb_saved.addr[5]|=0xC0u;mb_saved.valid=1;mb_dirty();
        }
        mel_ble_radio_start(&mb.trims);
        for(uint32_t i=0;i<2u;i++){mashina_proto_select(i);ble_init(mb_saved.addr,1);}
        mashina_proto_select(0);mb.started=1;
    }
    if(!mb.started)return 1;
    mb_lock();
    if(!on){mb_scan_stop();mb_drop_peer(0,0);mb_drop_peer(1,0);}
    mb.on=!!on;
    for(uint32_t i=0;i<2u;i++){
        mashina_proto_select(i);ble_enable(on);
        MP.relink=on&&ble_store_has_last(&MP.store);
        MP.relink_t=fm1_ms-30000u;
    }
    mashina_proto_select(0);
    mb_unlock();
    if(!on){ble_scan_clear(&mb.found);mb.count=0;}
    return 1;
}
static void mel_ble_scan(int on)
{
    if(!mb.started)return;
    mb_lock();
    int slot=mb_free();
    if(on&&mb.on&&slot>=0)mb_scan_start((uint32_t)slot);
    else if(!on)mb_scan_stop();
    mb_unlock();
}
static void mel_ble_visible(int on){(void)on;}
static void mel_ble_disconnect(void)
{
    mb_lock();mb_scan_stop();
    if(mb.started){mb_drop_peer(0,0);mb_drop_peer(1,0);}
    mb_unlock();
}
static void mel_ble_forget(void)
{
    mb_lock();mb_scan_stop();
    if(mb.started){mb_drop_peer(0,1);mb_drop_peer(1,1);}
    mb_unlock();
}
static int mb_connect(uint32_t slot,const struct ble_found *f)
{
    if(!mb.on||mb_peer[slot].busy||mb_used(f))return 0;
    mb_scan_stop();mashina_proto_select(slot);
    mb_peer_t *p=&MP;
    p->target=*f;
    struct ble_peer peer={0};
    memcpy(peer.addr,f->addr,6);peer.addr_rand=f->addr_rand;
    const struct ble_dev *d=&p->store.dev;
    if(mb_same(&p->store,f->addr,f->addr_rand)){
        peer.bonded=!!(d->info&BLE_DEV_BONDED);memcpy(peer.ltk,d->ltk,16);
        memcpy(peer.rand,d->rand,8);peer.ediv=ble_rd16(d->ediv);peer.sec=d->sec;
    }
    p->keys_new=0;p->busy=1;p->was_pairing=p->was_ready=0;p->deadline=fm1_ms;
    int ok=ble_central_connect(&peer);
    if(!ok)p->busy=0;
    mashina_proto_select(0);return ok;
}
static int mel_ble_connect(uint32_t row)
{
    if(row>=mb.count)return 0;
    mb_lock();int slot=mb_free(),ok=0;
    if(slot>=0)ok=mb_connect((uint32_t)slot,&mb.found.e[mb.idx[row]]);
    mb_unlock();return ok;
}
static int mel_ble_last(void)
{
    if(!mb.on)return 0;
    mb_peer[0].relink=1;mb_peer[0].relink_t=fm1_ms-30000u;
    return 1;
}
static void mb_peer_poll(uint32_t slot)
{
    mashina_proto_select(slot);
    mb_peer_t *p=&MP;
    if(!p->busy)return;
    if(bll.state==LL_CONN){
        if(bgc.probe==2u){bgc.probe=0;p->probe_t=fm1_ms;}
        uint32_t elapsed=fm1_ms-p->probe_t;
        if(ble_hw_time_us()-bll.t_rx>(uint32_t)bll.timeout*10000u||(p->ready&&elapsed>=6000u))
            ll_close_by(BLE_ERR_CONN_TIMEOUT,BDC_SUPERVISION);
        else if(p->ready&&elapsed>=2000u&&!bgc.probe)gc_probe();
    }
    uint8_t state=ble_central_state(),pairing=ble_central_pairing();
    if(pairing!=p->was_pairing){p->was_pairing=pairing;p->deadline=fm1_ms;}
    if(p->keys_new||(state==BLE_CS_READY&&!p->was_ready)){
        if(!mb_same(&p->store,p->target.addr,p->target.addr_rand))
            ble_store_set_last(&p->store,p->target.addr,p->target.addr_rand,p->target.name,p->target.kind);
        if(p->keys_new){
            struct ble_keys *k=&p->keys;p->keys_new=0;
            if(k->has&BLE_KEYS_LTK)ble_store_set_bond(&p->store,k->ltk,k->rand,k->ediv,k->has&BLE_KEYS_AUTH);
            if(k->has&BLE_KEYS_ID)ble_store_set_id(&p->store,k->irk,k->id,k->id_rand);
        }
        if(!slot){ble_store_save(&p->store,mb_saved.dev);mb_dirty();}
        if(state==BLE_CS_READY){p->was_ready=1;p->relink=1;}
    }
    if(state==BLE_CS_IDLE){
        uint8_t why=ble_central_fail();
        p->busy=0;p->relink_t=fm1_ms-30000u;
        if(why==BLE_CF_KEY){ble_store_drop_bond(&p->store);if(!slot){ble_store_save(&p->store,mb_saved.dev);mb_dirty();}}
        if(why!=BLE_CF_LOST&&why!=BLE_CF_NONE)p->relink=0;
    }else if(!p->was_ready&&fm1_ms-p->deadline>(pairing?45000u:10000u)){
        mb_drop_peer(slot,0);
    }
}
static void mel_ble_poll(void)
{
    if(mb.start_req){mb.start_req=0;mel_ble_set(1);ui.force=1;}
    if(!mb.started||!mb.on)return;
    mb_lock();mb_peer_poll(0);mb_peer_poll(1);mashina_proto_select(0);mb_unlock();
    uint8_t pdu[39];uint16_t rssi;uint32_t n;
    for(uint32_t i=0;i<BLE_SCAN_RING&&(n=ble_ll_scan_take(pdu,&rssi));i++)
        ble_scan_add(&mb.found,pdu,n,rssi,fm1_ms);
    if(fm1_ms-mb.aged>=1000u){mb.aged=fm1_ms;ble_scan_age(&mb.found,fm1_ms);}
    mb.count=(uint8_t)ble_scan_list(&mb.found,mb.idx);
    /* Reconnect either remembered controller on a free instance. Do not
     * restart an active manual scan or steal a connected peer's slot. */
    mb_lock();
    for(uint32_t i=0;i<2u;i++){
        mb_peer_t *p=&mb_peer[i];if(p->busy||!p->relink)continue;
        if(mb.scanning&&mb.scan_slot==i){
            for(uint32_t j=0;j<mb.count;j++){
                struct ble_found *f=&mb.found.e[mb.idx[j]];
                if(mb_same(&p->store,f->addr,f->addr_rand)&&!mb_used(f)){mb_connect(i,f);break;}
            }
            if(mb.scanning&&fm1_ms-p->relink_t>=10000u&&fm1_ms-p->relink_t<30000u)mb_scan_stop();
        }else if(!mb.scanning&&fm1_ms-p->relink_t>=30000u){
            p->relink_t=fm1_ms;mb_scan_start(i);
        }
    }
    mb_unlock();
}
static uint32_t mel_ble_code(void)
{
    uint32_t code=BLE_NO_PASSKEY;
    mb_lock();
    for(uint32_t i=0;i<2u;i++){
        mashina_proto_select(i);
        if(MP.busy&&(code=ble_central_passkey())!=BLE_NO_PASSKEY)break;
    }
    mashina_proto_select(0);mb_unlock();return code;
}
static const char *mel_ble_status(void)
{
    if(!mb.cal)return "NO CAL";
    if(!mb.on)return "BLE OFF";
    if(mb_peer[0].ready&&mb_peer[1].ready)return "2 BLE CONNECTED";
    if(mel_ble_code()!=BLE_NO_PASSKEY)return "PAIRING CODE";
    if(mel_ble_ready())return "BLE CONNECTED";
    if(mb_peer[0].busy||mb_peer[1].busy)return "FINDING MIDI";
    if(mb.scanning)return "SCANNING";
    return "BLE AVAILABLE";
}
static uint32_t mel_ble_signature(void)
{
    return mb.found.gen*13u+mb.on+mb.scanning*3u+mb_peer[0].ready*997u+mb_peer[1].ready*1999u+mel_ble_code()*17u;
}
/* USB maintenance only: restore a checked trim copy, never a raw flash sector.
 * Existing valid calibration cannot be replaced by a different unit's copy. */
static uint32_t mel_ble_import_cal(const uint8_t copy[BLE_RF_COPY_SIZE])
{
    if (!ble_rf_copy_ok(copy)) return 1;
    if (mb.on || (mb_peer[0].busy||mb_peer[1].busy)) return 4;
    if (mb.cal && !fm1_rf_cal_on && memcmp(mb_saved.rf, copy, BLE_RF_COPY_SIZE)) return 2;
    if (mb.cal && !fm1_rf_cal_on) return 0;
    memcpy(mb_saved.rf, copy, BLE_RF_COPY_SIZE);
    ble_rf_copy_get(copy, &mb.trims);
    fm1_rf_cal_on = 0;                        /* (the saved one instead of the measurement) */
    mb.cal = 1; mb_dirty();
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


/* Linked or connecting peers first, then available discovered devices.
 * Encoded row 0/1 = peer; row 2+n = discovered record index. */
static uint8_t mb_rows[BLE_SCAN_N+2u],mb_row_n;
static void mb_rows_update(void)
{
    mb_row_n=0;
    for(uint32_t i=0;i<2u;i++)if(mb_peer[i].busy)mb_rows[mb_row_n++]=(uint8_t)i;
    for(uint32_t i=0;i<mb.count;i++)if(!mb_used(&mb.found.e[mb.idx[i]]))mb_rows[mb_row_n++]=(uint8_t)(i+2u);
    if(mb.pick>=mb_row_n)mb.pick=mb_row_n?mb_row_n-1u:0u;
}
static void ble_page_knob(uint32_t k,int32_t steps)
{
    int right=steps>0;mb_rows_update();
    if(k==0u){if(!right){mb.start_req=0;mel_ble_set(0);}else if(!transport_busy()){if(!mb.started)mb.start_req=1;else mel_ble_set(1);}}
    else if(k==1u)mel_ble_scan(right);
    else if(k==2u&&mb_row_n)mb.pick=(uint8_t)clamp((int32_t)mb.pick+(right?1:-1),0,mb_row_n-1u);
    else if(k==3u&&mb_row_n){
        uint32_t row=mb_rows[mb.pick];
        if(right&&row>=2u)mel_ble_connect(row-2u);
        else if(!right&&row<2u){mb_lock();mb_drop_peer(row,0);mb_unlock();}
    }
    ui.force=1;
}
static void ble_page_draw(void)
{
    static uint32_t prev_head,prev_rows[4],prev_bottom;
    const uint32_t full=ui.force;
    uint32_t code=mel_ble_code();mb_rows_update();
    const char *status=mel_ble_status();
    uint32_t head=mb.on|(uint32_t)mb.scanning<<1;
    for(const char *p=status;*p;p++)head=head*31u+(uint8_t)*p;
    if(full||head!=prev_head){
        cv_begin(240,74,T_BG);
        cv_text(4,2,&AF_S,"K1 RADIO",T_TEXT);cv_text_r(115,2,&AF_M,mb.on?"ON":"OFF",T_VALUE,T_BG);
        cv_text(124,2,&AF_S,"K2 SCAN",T_TEXT);cv_text_r(236,2,&AF_M,mb.scanning?"ON":"OFF",T_VALUE,T_BG);
        cv_rect(0,24,240,1,RGB(58,58,58));
        cv_text(4,29,&AF_S,"STATUS",T_TEXT);cv_text_r(236,29,&AF_M,status,T_VALUE,T_BG);
        cv_text(4,48,&AF_M,"MAX 2 MIDI DEVICES",T_TEXT);
        cv_blit(0,0);prev_head=head;
    }
    if(full){cv_begin(240,18,RGB(58,58,58));cv_text(4,2,&AF_S,"NO.",T_TEXT);cv_text(34,2,&AF_S,"MIDI DEVICE",T_TEXT);cv_text_r(236,2,&AF_S,"STATUS",T_TEXT,RGB(58,58,58));cv_blit(0,74);}
    for(uint32_t i=0;i<4u;i++){
        uint32_t row=mb.pick/4u*4u+i,selected=row==mb.pick&&row<mb_row_n;
        uint32_t id=row<mb_row_n?mb_rows[row]:255u;
        const struct ble_found *f=id<2u?&mb_peer[id].target:id!=255u?&mb.found.e[mb.idx[id-2u]]:0;
        const char *name=f?(f->name[0]?f->name:"UNNAMED"):(!i&&!mb_row_n?(mb.on?"NO DEVICES FOUND":"RADIO OFF"):"");
        const char *tag=id<2u?(mb_peer[id].ready?"LINKED":"WAIT"):"";
        uint32_t bars=f&&id>=2u?ble_scan_bars(&mb.found,mb.idx[id-2u]):0;
        uint32_t sig=1u+row*8u+selected*4u+bars+id*64u;
        for(const char *p=name;*p;p++)sig=sig*31u+(uint8_t)*p;
        for(const char *p=tag;*p;p++)sig=sig*31u+(uint8_t)*p;
        if(!full&&sig==prev_rows[i])continue;prev_rows[i]=sig;
        uint16_t bg=selected?T_VALUE:(i&1u?RGB(23,23,23):RGB(36,36,36)),ink=selected?T_INK:T_TEXT;
        cv_begin(240,24,bg);
        if(f){char num[4];fmt_int(num,row+1u);cv_text(4,CAP_IN(M,24),&AF_M,num,ink);
            if(id<2u)cv_text_r(236,CAP_IN(S,24),&AF_S,tag,ink,bg);
            else for(uint32_t j=0;j<3u;j++)cv_rect(211+j*8u,18-(j+1u)*4u,5,(j+1u)*4u,j<bars?(selected?T_INK:T_VALUE):RGB(70,70,70));}
        cv_free_text(34,CAP_IN(M,24),&AF_M,name,ink,bg,id<2u?140:172);cv_blit(0,92+i*24u);
    }
    if(full||code!=prev_bottom){
        cv_begin(240,34,T_BG);prev_bottom=code;
        if(code!=BLE_NO_PASSKEY){char pass[7];pass[6]=0;for(int i=5;i>=0;i--){pass[i]=(char)('0'+code%10u);code/=10u;}cv_text(4,1,&AF_S,"PAIRING CODE",T_TEXT);cv_text_r(236,1,&AF_M,pass,T_VALUE,T_BG);}
        else{cv_text(4,1,&AF_S,"K3 SELECT DEVICE",T_TEXT);cv_text(4,17,&AF_S,"K4 CONNECT > / DROP <",T_TEXT);}
        cv_blit(0,188);
    }
    if(full){cv_begin(240,18,T_BG);cv_rect(3,1,43,16,T_VALUE);cv_text_in(5,1,39,&AF_S,"GLO",T_INK,T_VALUE);cv_rect(48,1,65,16,RGB(58,58,58));cv_text_in(51,1,59,&AF_S,"SYSTEM",T_TEXT,RGB(58,58,58));cv_text(117,1,&AF_S,"BLUETOOTH",T_VALUE);cv_text_r(237,1,&AF_S,"8/8",T_VALUE,T_BG);cv_blit(0,222);}
    ui.force=0;
}
