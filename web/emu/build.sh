#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Felucca in the browser: the firmware's sources as the host tests build them (web/emu/felucca_web.c), compiled
# to WebAssembly with Emscripten, plus the page. After X0X's web/emu/build.sh (charlesvestal/fm1-x0x, GPL-3.0).
#   web/emu/build.sh   ->  build/emu/{index.html, worklet.js, felucca.wasm, fonts/}
# Run after one ./build.sh (build/gen: the tables, the fonts, the CC0 samples).
# -ffp-contract=off: no fused multiply-adds, as the host build. EMU_OPT replaces -O2, EMU_CFLAGS adds flags
# (EMU_CFLAGS=-msimd128: let the compiler vectorise; every browser with AudioWorklet has wasm SIMD).
# The page shows the version (FELUCCA_VERSION) marked "(preview)" before its release; EMU_PREVIEW=0 drops the mark.
set -e
cd "$(dirname "$0")/../.."
[ -f build/gen/felucca_tables.h ] || { echo "emu: run ./build.sh once first (build/gen)"; exit 1; }
command -v emcc >/dev/null 2>&1 || { echo "emu: no emcc (Emscripten)"; exit 1; }
VERSION=$(sed -n 's/^#define FELUCCA_VERSION "\([^"]*\)".*/\1/p' firmware/src/felucca.c)
# Branch-only simulator for reference-only projects / 32 LIBRARY slots.
# This is an in-browser WASM NOR simulation, not a hardware installer.
case "${MASHINA_EMU_REFS:-0}" in 0|1) ;; *) echo "emu: MASHINA_EMU_REFS must be 0 or 1" >&2; exit 1;; esac
case "${MASHINA_EMU_LIB96:-0}" in 0|1) ;; *) echo "emu: MASHINA_EMU_LIB96 must be 0 or 1" >&2; exit 1;; esac
LIBRARY_SLOTS=32
if [ "${MASHINA_EMU_LIB96:-0}" = 1 ]; then
    [ "${MASHINA_EMU_REFS:-0}" = 1 ] || { echo "emu: LIB96 requires MPR1 simulator" >&2; exit 1; }
    LIBRARY_SLOTS=96
fi
if [ "${MASHINA_EMU_REFS:-0}" = 1 ]; then
    VERSION="${EMU_VERSION:-$VERSION-mpr1-sim}"
else
    [ -z "${EMU_VERSION:-}" ] || VERSION="$EMU_VERSION"
fi
mkdir -p build/emu/fonts
# shellcheck disable=SC2086
emcc ${EMU_OPT:--O2} -ffp-contract=off -std=gnu99 -w ${EMU_CFLAGS:-} \
    "-DFELUCCA_NO_SEQUENCER=${FELUCCA_NO_SEQUENCER:-1}" \
    "-DMASHINA_EMU_REFS=${MASHINA_EMU_REFS:-0}" \
    "-DMASHINA_EMU_LIB96=${MASHINA_EMU_LIB96:-0}" \
    "-DFELUCCA_VERSION=\"$VERSION\"" -Ibuild/gen -Ifirmware/src \
    --no-entry -sSTANDALONE_WASM -sSTACK_SIZE=1048576 -sINITIAL_MEMORY=33554432 -sFILESYSTEM=0 \
    -o build/emu/felucca.wasm web/emu/felucca_web.c
[ "${EMU_PREVIEW:-1}" = 0 ] && PREVIEW='' || PREVIEW=' (preview)'
# Ship the navigation and translation assets with the isolated artifact.
# Installed Pages sites may have these in the parent folder; downloaded
# previews must not require any files outside build/emu.
sed -e "s/@VERSION@/$VERSION/g" -e "s/@PREVIEW@/$PREVIEW/g" -e "s/@LIBRARY_SLOTS@/$LIBRARY_SLOTS/g" \
    -e 's#../mashina-nav.css#./mashina-nav.css#g' \
    -e 's#../mashina-nav.js#./mashina-nav.js#g' \
    -e 's#../mashina-emu-locale.js#./mashina-emu-locale.js#g' \
    -e "s/@FACTORY_STORE_SUFFIX@/${EMU_STORE_SUFFIX:-}/g" \
    web/emu/index.html > build/emu/index.html
cp web/mashina-nav.css web/mashina-nav.js web/mashina-emu-locale.js build/emu/
cp web/emu/worklet.js web/emu/emulator-flash-backup.mjs web/emu/stems.js web/emu/sample-bank.js web/fm1backup.js build/emu/
cp web/factory-voices.json web/factory-samples.json build/emu/
cp web/emu/fonts/DotGothic16-subset.woff web/emu/fonts/OFL.txt build/emu/fonts/
echo "emu: build/emu ($(wc -c < build/emu/felucca.wasm | tr -d ' ') B wasm, $VERSION)"
