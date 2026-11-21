#!/usr/bin/env python3
"""Check all WHEEL factory entries against the uploaded user bank's fingerprint."""
import json
import re
from pathlib import Path

engine = Path("firmware/src/eng_wheel.c").read_text(encoding="utf-8")
header = Path("firmware/src/wheel_factory_import.h").read_text(encoding="utf-8")
expected = ["BRUNI","BATTISTA","MARSILIO","MIRANDOLA","ERASMUS"]
rows = re.findall(r"/\* (\d+)\. ([A-Z0-9-]+(?: [A-Z]+)?) \*/\s*\{([^{}]+)\}", header)
assert len(rows) == 5, f"Expected 5 WHEEL common parameter rows, got {len(rows)}"
common = []
for i, (pos, name, values) in enumerate(rows):
    assert int(pos) == i + 1 and name == expected[i]
    params = [int(s) for s in re.findall(r"-?\d+", values)]
    assert len(params) == 115, (name, len(params))
    assert all(-128 <= v <= 127 for v in params)
    common.append(params)
body = engine.split("static const preset_t WHEEL_PRESETS[] = {", 1)[1].split("};", 1)[0]
items = re.findall(r'^\s*\{"([^"]+)", \{([^{}]+)\}, \{([^{}]+)\}, (-?\d+), TAG\(NONE\), FX\(([^()]*)\), PAT\(0\)\},', body, flags=re.M)
assert len(items) == 5, f"Old WHEEL entries should be removed, got {len(items)}"
combined = []
for i, (name, e, env, fenv, fx) in enumerate(items):
    assert name == expected[i]
    engine_values = [int(x.strip()) for x in e.split(",")]
    envelope = [int(x.strip()) for x in env.split(",")]
    effects = [int(x.strip()) for x in fx.split(",")]
    assert len(engine_values) == 8 and envelope == common[i][1:5]
    assert int(fenv) == common[i][5] and effects == common[i][33:37]
    combined.append([i, name, common[i], engine_values, envelope, int(fenv), effects])
h = 2166136261
for ch in json.dumps(combined, separators=(",", ":")):
    h = ((h ^ ord(ch)) * 16777619) & 0xFFFFFFFF
assert f"{h:08x}" == "bb1ebdb7", f"WHEEL bank fingerprint changed: {h:08x}"
assert "if (trk_eng(t) == ENGI_WHEEL && pi < WHEEL_FACTORY_IMPORTED)" in Path("firmware/src/ui.c").read_text(encoding="utf-8")
print("PASS: all 5 WHEEL factory sounds match user bank and old entries are gone")
