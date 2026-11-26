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
