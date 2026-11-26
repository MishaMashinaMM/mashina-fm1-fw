#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Pinned GPL-3.0-only Melodee / Optimist BLE source; no vendor SDK BLE library.
set -eu
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git -C "$tmp" init -q
git -C "$tmp" fetch -q --depth=1 https://github.com/keremimo/melodee.git 165d177a425944f3d30b0b669255a6ed0050c3a5
git -C "$tmp" checkout -q --detach FETCH_HEAD
test "$(git -C "$tmp" rev-parse HEAD)" = 165d177a425944f3d30b0b669255a6ed0050c3a5
mkdir -p firmware/src/ble
cp "$tmp"/firmware/src/ble/* firmware/src/ble/
for f in fm1_ble.h fm1_ble_rf.h fm1_rf_cal.h ble_rf_tables.h; do
  cp "$tmp/firmware/hal/$f" "firmware/hal/$f"
done

# Local MASHINA fixes to the pinned stack. Fail loudly if the pinned source changes.
python3 - <<'PY'
from pathlib import Path
def replace(path, old, new):
    p=Path(path)
    s=p.read_text()
    assert s.count(old)==1, (path,old)
    p.write_text(s.replace(old,new))
replace("firmware/hal/fm1_ble_rf.h",
    "    fm1_ble_rf_stat.section = (uint8_t)g;",
    "    fm1_wdt_feed(); /* feed only on real RF replay progress */\n    fm1_ble_rf_stat.section = (uint8_t)g;")
replace("firmware/src/ble/ble_hw_wl82.c",
    "        drv.rx_seen = drv.rx_any = 1;",
    "        if ((st & 0xFu) == 1u) drv.rx_seen = drv.rx_any = 1;")
# A connected controller must periodically answer a standard read of the
# already discovered CCCD. Accept a matching ATT error as proof of life too.
# Never feed the returned bytes to the MIDI decoder.
replace("firmware/src/ble/ble_gattc.c",
    "    uint8_t st, wait_sec, due;",
    "    uint8_t probe; /* 0 idle, 1 outstanding, 2 answered */\n    uint8_t st, wait_sec, due;")
replace("firmware/src/ble/ble_gattc.c",
    "BLE_API uint16_t ble_gattc_midi(void) { return bgc.st == G_READY ? bgc.val : 0u; }",
    """BLE_API uint16_t ble_gattc_midi(void) { return bgc.st == G_READY ? bgc.val : 0u; }
static void gc_probe(void)
{
    if(bgc.st!=G_READY || bgc.probe || !bgc.cccd)return;
    uint8_t p[3]={0x0Au,0,0};
    ble_wr16(p+1,bgc.cccd);
    if(ble_ll_send(L2CAP_CID_ATT,p,sizeof p))bgc.probe=1;
}""")
replace("firmware/src/ble/ble_gattc.c",
    "    if (bgc.st == G_IDLE || bgc.st == G_READY || bgc.st == G_FAIL || bgc.wait_sec)\n        return 1;",
    """    if(bgc.st==G_READY && bgc.probe==1u &&
        (op==0x0Bu || (op==0x01u && n==5u && p[1]==0x0Au && ble_rd16(p+2)==bgc.cccd)))
        bgc.probe=2u;
    if (bgc.st == G_IDLE || bgc.st == G_READY || bgc.st == G_FAIL || bgc.wait_sec)
        return 1;""")
# Stage 1 of multi-controller support: two independent baseband instances.
# Link 0 still owns the sole protocol connection. Link 1 can only scan.
p = Path("firmware/src/ble/ble_hw_wl82.c")
s = p.read_text()
def once(old, new):
    global s
    assert s.count(old) == 1, old
    s = s.replace(old, new)
once('#define HW_LINK 0u', '#define HW_LINK ((uint32_t)mashina_hw_link)')
once('''struct ble_bb {
    volatile uint16_t inst[8];         /* 0x000 instance table: 0x8000 | link n's control block offset */
    uint8_t sw[0x30];                  /* 0x010 software area (unused) */
    struct ble_cb cb;''', '''struct mashina_ble_link_mem {
    struct ble_cb cb;''')
once('''_Static_assert(__builtin_offsetof(struct ble_bb, cb) == 0x40u, "HW §4: link 0's block at +0x40, as stock");''', '''struct ble_bb {
    volatile uint16_t inst[8];
    uint8_t sw[0x30];
    struct mashina_ble_link_mem link[2];
};
_Static_assert(__builtin_offsetof(struct ble_bb, link[0].cb) == 0x40u, "link 0 offset");
#define BB (bb.link[HW_LINK])''')
for name in ("cb", "rx", "tx"):
    s = s.replace("bb." + name, "BB." + name)
once("} drv;", "} mashina_drv[2];\n#define drv (mashina_drv[HW_LINK])")
# The unused shared header is initialized at radio boot only.
once("    uint8_t *p = (uint8_t *)&bb.sw;\n    fm1_ble_step(FM1_BLE_STEP_LINK_STOP);", "    fm1_ble_step(FM1_BLE_STEP_LINK_STOP);")
once("    for (c = 0; c < sizeof bb.sw; c++)\n        p[c] = 0;\n", "")
once("void ble_wl82_rx_irq(void)", "static void mashina_ble_rx_one(void)")
once("void ble_wl82_event_irq(void)", "static void mashina_ble_event_one(void)")
anchor="/* once at boot, before the interrupts are on"
assert s.count(anchor)==1
s=s.replace(anchor, '''/* Dispatch only pending sources. Preserve the foreground link selector. */
void ble_wl82_rx_irq(void)
{
    uint8_t old=mashina_hw_link;
    for(mashina_hw_link=0;mashina_hw_link<2u;mashina_hw_link++)
        if(fm1_ble_rx_pending(HW_LINK))mashina_ble_rx_one();
    mashina_hw_link=old;
}
void ble_wl82_event_irq(void)
{
    uint8_t old=mashina_hw_link;
    for(mashina_hw_link=0;mashina_hw_link<2u;mashina_hw_link++){
        if(fm1_ble_event_pending(HW_LINK))mashina_ble_event_one();
        else if(FM1_BLE_G2EN & FM1_BLE_G2PND & (1u<<HW_LINK))fm1_ble_event_tail(HW_LINK);
    }
    mashina_hw_link=old;
}

''' + anchor)
p.write_text(s)
p = Path("firmware/src/ble/ble_hw_wl82_central.c")
s=p.read_text()
for name in ("cb", "rx", "tx"):
    s=s.replace("bb."+name,"BB."+name)
p.write_text(s)
replace("firmware/src/ble/ble_ll_central.c",
    "if (bll.state != LL_SCAN || len < 8u || len > LL_REP_MAX)",
    "if ((!mashina_hw_link && bll.state != LL_SCAN) || len < 8u || len > LL_REP_MAX)")
# Preserve the other instance's group-2 enable bits, never write its ACK.
replace("firmware/hal/fm1_ble.h",
    "    FM1_BLE_G2EN = 0x100u << link;",
    "    FM1_BLE_G2EN = (FM1_BLE_G2EN & 0xFFu & ~(1u<<link)) | (0x100u<<link);")
replace("firmware/hal/fm1_ble.h",
    "    FM1_BLE_G2EN = 0x101u << link;",
    "    FM1_BLE_G2EN = (FM1_BLE_G2EN & 0xFFu) | (0x101u<<link);")
# Avoid echoing write-to-ack bits of other hardware instances.
replace("firmware/hal/fm1_ble.h",
    "    FM1_BLE_G2EN &= ~(0x101u << link);",
    "    FM1_BLE_G2EN = (FM1_BLE_G2EN & 0xFFu & ~(1u<<link)) | (0x100u<<link);")
replace("firmware/hal/fm1_ble.h",
    "        FM1_BLE_G2EN |= 0x100u << link;",
    "        FM1_BLE_G2EN = (FM1_BLE_G2EN & 0xFFu) | (0x100u<<link);")
# Keep the larger DMA block in the proven pool region, move only the
# small fixed PLL table to regular SRAM to retain the pool reserve.
replace("firmware/hal/fm1_ble.h",
    'static uint32_t fm1_ble_pll_tbl[81 * 3] __attribute__((section(".pool"), aligned(4)));',
    'static uint32_t fm1_ble_pll_tbl[81 * 3] __attribute__((aligned(4)));')

PY
