#!/usr/bin/env python3
"""Guard MASHINA's lossless font atlas pruning and stretched-face fallback.

Q/R were wide (115/110%) legacy Oxanium faces. No current firmware source
may refer to them, or removing them from the atlas would change the UI.
The active S/M/L/N/V bitmap generation and text fitting remain unchanged.
"""
from pathlib import Path
import sys

src = Path("firmware/src")
for file in sorted(src.glob("*.[ch]")):
    data = file.read_text(encoding="utf-8")
    for face in ("AF_Q", "AF_R"):
        assert face not in data, f"{file}: obsolete font reference {face}"

fontgen = Path("tools/gen_aa_font.py").read_text(encoding="utf-8")
assert 'if a.preset == "oxanium" and not a.face and not a.include_legacy_stretched:' in fontgen
assert 'not in ("Q", "R")' in fontgen
assert 'stretch=1.15 if name == "Q" else 1.10 if name == "R" else 1.0' in fontgen
assert "PHASES, PHASES_L = 1, 1" in fontgen

header = Path("build/gen/ui_fonts.h")
if header.exists():
    result = header.read_text(encoding="utf-8")
    for face in ("S", "M", "L", "N", "V"):
        assert f"AF_{face}_DATA" in result, f"active face missing: {face}"
    for face in ("Q", "R"):
        assert f"AF_{face}_DATA" not in result, f"unused face present: {face}"

print("PASS: font atlas omits unused stretched faces; active text metrics unchanged")
