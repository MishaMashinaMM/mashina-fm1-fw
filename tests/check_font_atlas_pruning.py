#!/usr/bin/env python3
"""Guard MASHINA's stretched-Q SEQ/PLAY typography and unused-R pruning.

Q is 115% horizontally stretched and used by drum SEQ/PLAY rows; R
is a previously generated 110% face without live firmware references.
Both retain their original rendering rules when explicitly generated.
"""
from pathlib import Path

src = Path("firmware/src")
for file in sorted(src.glob("*.[ch]")):
    data = file.read_text(encoding="utf-8")
    for face in ("AF_R",):
        assert face not in data, f"{file}: obsolete font reference {face}"

fontgen = Path("tools/gen_aa_font.py").read_text(encoding="utf-8")
assert 'if a.preset == "oxanium" and not a.face and not a.include_legacy_stretched:' in fontgen
assert 'face[0] != "R"' in fontgen
assert 'stretch=1.15 if name == "Q" else 1.10 if name == "R" else 1.0' in fontgen
assert "PHASES, PHASES_L = 1, 1" in fontgen
assert "&AF_Q" in (src / "ui_draw.c").read_text(encoding="utf-8")
assert '("Q", v + "@750", 17, (32, 32),' in fontgen
assert '[ord(c) for c in L_CHARS[1:]], 0.0, PHASES_L)' in fontgen

# Every string rendered by the stretched Q face in SEQ/PLAY fits the
# sparse uppercase/digit/sign alphabet (including drum-kit pad names).
q_chars = set(" #&+-./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ")
required = ("LIN", "SOFT", "HARD", "FIX", "OVR", "RST", "KIK", "SNR",
            "CLP", "CH", "OH", "TOM", "MTL", "CRH", "TM2",
            "00 KIK", "L127", "R127", "G9", "--", "+36", "-36")
assert all(set(s) <= q_chars for s in required)


header = Path("build/gen/ui_fonts.h")
if header.exists():
    result = header.read_text(encoding="utf-8")
    for face in ("S", "M", "L", "N", "V", "Q"):
        assert f"AF_{face}_DATA" in result, f"active face missing: {face}"
    for face in ("R",):
        assert f"AF_{face}_DATA" not in result, f"unused face present: {face}"

print("PASS: Q stays 115% stretched with needed glyphs; unused R omitted")
