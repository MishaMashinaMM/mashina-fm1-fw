#!/usr/bin/env python3
"""Guard the retired ANIMAL synth's orphaned graphical assets.

The old creature renderers are disabled by FELUCCA_NO_SEQUENCER; this
check also prevents the deleted engine's vector art from lingering in
the source tree or resurfacing via the optional legacy visualizer.
"""
from pathlib import Path

engvis = Path("firmware/src/ui_engvis.c").read_text(encoding="utf-8")
being = Path("firmware/src/ui_being.c").read_text(encoding="utf-8")
graph = Path("firmware/src/ui_graph.c").read_text(encoding="utf-8")
size = Path("tools/size_fns.py").read_text(encoding="utf-8")
engine = Path("firmware/src/engines.c").read_text(encoding="utf-8")

assert "vx_animal(" not in engvis
assert "VX_AN[" not in engvis
assert "VA_POS[" not in engvis
assert "ENG_ANIMAL" not in engvis
assert "AN_CMOS" not in engvis
assert "GENOME[8]" in being
assert "larva" not in being
assert '#include "ui_engvis.c"' in graph
assert '#include "ui_being.c"' in graph
assert '#include "ui_organic.c"' in graph
for name in ("ui_engvis.c", "ui_being.c", "ui_organic.c"):
    # All three of these old visualizers are in the sequencer-only path;
    # no changes to their contents should affect MASHINA application flash.
    marker = f'#include "{name}"'
    where = graph.index(marker)
    assert graph.rfind("#if !FELUCCA_NO_SEQUENCER", 0, where) > graph.rfind("#endif", 0, where), name
assert "eng_animal.c" not in size
assert "eng_animal.c" not in engine
print("PASS: ANIMAL-only creature art removed; legacy visualizers remain compiled out")
