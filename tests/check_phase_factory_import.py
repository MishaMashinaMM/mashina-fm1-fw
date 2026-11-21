#!/usr/bin/env python3
"""Verify the PHASE factory replacement preserves the accepted 10-sound JSON bank."""
import json
import re
from pathlib import Path

source = Path("firmware/src/eng_phase.c").read_text(encoding="utf-8")
header = Path("firmware/src/phase_factory_import.h").read_text(encoding="utf-8")
expected = ["PSEUDO-DIONY","MAXIMUS","AL-KINDI","JOHN SCOTUS","AL-FARABI","AVICENNA","ANSELM","AVERROES","COLUCCIO","GEMISTUS"]
rows = re.findall(r"/\\* (\\d+)\\. ([A-Z0-9-]+(?: [A-Z]+)?) \\*/\\s*\\{([^{}]+)\\}", header)
assert len(rows) == 10, len(rows)
common = []
for i, (pos, name, values) in enumerate(rows):
    assert int(pos) == i + 1 and name == expected[i]
    params = [int(x) for x in re.findall(r"-?\\d+", values)]
    assert len(params) == 115, (name, len(params))
    assert all(-128 <= x <= 127 for x in params)
    common.append(params)
body = source.split("static const preset_t PHASE_PRESETS[] = {", 1)[1].split("};", 1)[0]
items = re.findall(r'^\\s*\\{"([^"]+)", \\{([^{}]+)\\}, \\{([^{}]+)\\}, (-?\\d+), TAG\\(NONE\\), FX\\(([^()]*)\\), PAT\\(0\\)\\},', body, flags=re.M)
assert len(items) == 10, len(items)
combined = []
for i, (name, values, env, fenv, fx) in enumerate(items):
    assert name == expected[i]
    e = [int(x.strip()) for x in values.split(",")]
    envelope = [int(x.strip()) for x in env.split(",")]
    effects = [int(x.strip()) for x in fx.split(",")]
    assert len(e) == 8 and envelope == common[i][1:5]
    assert int(fenv) == common[i][5] and effects == common[i][33:37]
    combined.append([i, name, common[i], e, envelope, int(fenv), effects])
h = 2166136261
for ch in json.dumps(combined, separators=(",", ":")):
    h = ((h ^ ord(ch)) * 16777619) & 0xFFFFFFFF
assert f"{h:08x}" == "f53dc49b", f"Bank content changed: {h:08x}"
assert "if (trk_eng(t) == ENGI_PHASE && pi < PHASE_FACTORY_IMPORTED)" in Path("firmware/src/ui.c").read_text(encoding="utf-8")
print("PASS: 10 PHASE factory presets match uploaded JSON (names and all sound parameters)")
