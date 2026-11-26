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
PY
