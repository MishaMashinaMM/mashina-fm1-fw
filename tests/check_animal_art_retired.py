#!/usr/bin/env python3
"""The obsolete creature and lens UI must not enter MASHINA firmware."""
from pathlib import Path

retired = (
    "ui_organic.c", "ui_being.c", "ui_engvis.c",
    "ui_lzhome.c", "ui_lzedit.c", "ui_lzpage.c",
)
graph = Path("firmware/src/ui_graph.c").read_text(encoding="utf-8")
optimizer = Path("tools/size_fns.py").read_text(encoding="utf-8")
for filename in retired:
    assert not Path("firmware/src", filename).exists(), filename
    assert f'#include "{filename}"' not in graph, filename
    assert f'"{filename}"' not in optimizer, filename

# The live UI still uses its own drawing and non-decorative FX/SAVE panels.
for active in ("ui_fm6op.c", "ui_tech.c", "ui_lzsave.c", "ui_lzfx.c"):
    assert f'#include "{active}"' in graph, active
assert "static void lz_arc(" in graph
assert "static int32_t bg_energy(void)" in graph
print("PASS: obsolete creature and lens artwork removed, active UI helpers intact")
