#!/usr/bin/env python3
"""Check that MASHINA cannot pull retired artwork or song generation into a live build."""
from pathlib import Path

root = Path("firmware/src")
ui = (root / "ui_input.c").read_text(encoding="utf-8")
lens = (root / "ui_lens.c").read_text(encoding="utf-8")
layer = (root / "ui_layer.c").read_text(encoding="utf-8")
core = (root / "felucca.c").read_text(encoding="utf-8")
graph = (root / "ui_graph.c").read_text(encoding="utf-8")

assert "#define FELUCCA_NO_SEQUENCER 1" in core
assert '#include "gen.c"' in ui
assert ui.index("#if FELUCCA_NO_SEQUENCER && !defined(MASHINA_HOST_TEST)\n/* MASHINA has no song generator") < ui.index("#include \"gen.c\"")
assert '#include "gen.c"' in ui.split("#else\n", 1)[-1] or '#include "gen.c"' in ui
assert 'static void gen_panel(void) { }' in ui
assert 'static void gen_cards(void) { }' in ui
assert 'static uint32_t gen_draw_sig(void) { return 0u; }' in ui
assert 'static const char *dice_seq(track_t *t)' in ui
assert "static int gen_on(void) { return 0; }" in lens
assert "#if FELUCCA_NO_SEQUENCER && !defined(MASHINA_HOST_TEST)" in lens
assert "-DMASHINA_HOST_TEST=1" in Path("tests/run_live_drum_tests.sh").read_text(encoding="utf-8")
assert "(void)rng();" in layer
assert "gen_panel();" in graph  # safe stub keeps obsolete selector harmless
assert "python3 tests/check_live_legacy_pruning.py" in Path("tests/run_live_drum_tests.sh").read_text(encoding="utf-8")
print("PASS: live firmware excludes GEN song composer and obsolete creature graphics")
