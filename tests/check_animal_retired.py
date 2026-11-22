#!/usr/bin/env python3
"""ANIMAL retirement: no sound code or presets; ID 14 remains reserved."""
import re
from pathlib import Path

core = Path("firmware/src/core.h").read_text(encoding="utf-8")
eng = Path("firmware/src/engines.c").read_text(encoding="utf-8")
proj = Path("firmware/src/project.c").read_text(encoding="utf-8")
assert not Path("firmware/src/eng_animal.c").exists()
assert '#include "eng_animal.c"' not in eng
assert "ENG_ANIMAL" not in eng and "ANIMAL_PRESETS" not in eng
assert "#define ENGI_ANIMAL 14u" in core, "Stored engine IDs must not change"
assert "e == ENGI_ANIMAL" in core.split("static inline int eng_gone",1)[1].split("}",1)[0], "ANIMAL must be retired"
assert "&ENG_GONE,                   /* 14:" in eng, "Slot 14 must use dummy engine"
order = eng.split("static const uint8_t ENGINE_ORDER[NENG_SHOWN] = {",1)[1].split("};",1)[0]
assert "ENGI_ANIMAL" not in order, "Retired engine must not be selectable"
assert "#define NENG_SHOWN (NENGINES - !FELUCCA_FM4 - 6)" in core
for fm4 in (False, True):
    # Count actual order entries, including optional FM4 entry.
    shown = re.findall(r"\bENGI_[A-Z0-9]+\b", order)
    actual = len(shown) if fm4 else len(shown) - 1
    expected = 21 - (0 if fm4 else 1) - 6
    assert actual == expected, (actual, expected, fm4)
assert "{ENGI_NOISE, 0, 0}" in eng, "Track 3 must have a playable startup engine"
assert "proj_phys_gone" in proj and "eng_gone(d->engine)" in proj, "Projects must migrate retired sounds"
for lang in ("", ".es", ".ja", ".ru", ".sr", ".hi"):
    doc = Path(f"docs/MASHINA_MANUAL{lang}.md").read_text(encoding="utf-8")
    assert "**ANIMAL**" not in doc, lang
print("PASS: ANIMAL removed from code, presets, UI and manuals; ID 14 stays reserved")
