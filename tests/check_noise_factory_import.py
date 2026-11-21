#!/usr/bin/env python3
"""Validate four user-imported NOISE factory presets and their sound parameters."""
import json
import re
from pathlib import Path

source = Path("firmware/src/eng_noise.c").read_text(encoding="utf-8")
header = Path("firmware/src/noise_factory_import.h").read_text(encoding="utf-8")
expected = ["MACHIAVELLI","MORE","MONTAIGNE","BACON"]
rows = re.findall(r"/\* (\d+)\. ([A-Z0-9-]+(?: [A-Z]+)?) \*/\s*\{([^{}]+)\}", header)
assert len(rows) == 4, f"Expected 4 NOISE rows, got {len(rows)}"
common = []
for i, (pos, name, values) in enumerate(rows):
    assert int(pos) == i + 1 and name == expected[i]
    params = [int(s) for s in re.findall(r"-?\d+", values)]
    assert len(params) == 115, (name, len(params))
    assert all(-128 <= v <= 127 for v in params)
    common.append(params)
body = source.split("static const preset_t NOISE_PRESETS[] = {", 1)[1].split("};", 1)[0]
items = re.findall(r'^\s*\{"([^"]+)", \{([^{}]+)\}, \{([^{}]+)\}, (-?\d+), TAG\(NONE\), FX\(([^()]*)\), PAT\(0\)\},', body, flags=re.M)
assert len(items) == 4, f"Legacy NOISE presets should be absent, got {len(items)}"
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
assert f"{h:08x}" == "25e74644", f"Bank fingerprint changed: {h:08x}"
assert "if (trk_eng(t) == ENGI_NOISE && pi < NOISE_FACTORY_IMPORTED)" in Path("firmware/src/ui.c").read_text(encoding="utf-8")
print("PASS: four imported NOISE factory presets match the uploaded bank")
