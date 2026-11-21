#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Stage 25D: live-only drum regression tests. The original run_tests.sh still
# covers the upstream sequencer and OCT effects, which are retired in this fork.
set -euo pipefail
cd "$(dirname "$0")/.."

python3 tests/check_analog_factory_import.py

out=build/host/live_drums
mkdir -p "$out"
for test in drum_edit_redraw_test drum_midi_map_test keyboard_zones_ui_test tuning_pitch_test drumlab_ui_test industrial_ui_test brass_ui_test atmos_ui_test cc_macro_ui_test drums_clock_test midi_rx_test \
            drumlab_engine_test drumind_engine_test industrial_engine_test brass_engine_test atmos_engine_test brass_stress_test drumlab_stage24b_test project_test drumx_test \
            drum_stage27_test drum_stage27b_stress_test; do
    echo "== $test (FELUCCA_NO_SEQUENCER=1)"
    cc -O1 -w -DFELUCCA_NO_SEQUENCER=1 -Ibuild/gen -Ifirmware/src \
       -o "$out/$test" "tests/$test.c" -lm
    if "$out/$test" > "$out/$test.log" 2>&1; then
        echo "PASS: $test"
    else
        echo "FAIL: $test"
        tail -n 30 "$out/$test.log"
        exit 1
    fi
done
echo 'ALL LIVE DRUM TESTS PASSED'
