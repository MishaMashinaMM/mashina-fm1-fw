#!/usr/bin/env python3
"""Verify all 10 imported FM6 factory sounds match the exported bank exactly."""
import json
import re
from pathlib import Path

expected = ("PLATO", "DIOGENES", "ARISTOTLE", "XENOKRATES", "EPICURUS",
            "BION", "PLUTARCH", "EPICTETUS", "APOLLONIUS", "SOTION")
src = Path("firmware/src/eng_fm6.c").read_text(encoding="utf-8")
header = Path("firmware/src/fm6_factory_import.h").read_text(encoding="utf-8")
ui = Path("firmware/src/ui.c").read_text(encoding="utf-8")
body = src.split("static const preset_t FM6_PRESETS[] = {", 1)[1].split("};", 1)[0]

def parse_import_array(symbol, nvalues):
    content = header.split(symbol + "[FM6_IMPORT_N]", 1)[1].split("};", 1)[0]
    rows = re.findall(r"/\* (\d+)\. ([A-Z]+) \*/\s*\{([^{}]+)\}", content)
    assert len(rows) == 10, (symbol, len(rows))
    out = []
    for i, (pos, name, values) in enumerate(rows):
        assert int(pos) == i + 1 and name == expected[i], (symbol, i, pos, name)
        row = [int(v.strip()) for v in values.split(",") if v.strip()]
        assert len(row) == nvalues, (symbol, name, len(row), nvalues)
        out.append(row)
    return out

common = parse_import_array("FM6_IMPORT_COMMON", 115)
patches = parse_import_array("FM6_IMPORT_PATCH", 128)
for i, pk in enumerate(patches):
    assert all(0 <= v <= 127 for v in pk), expected[i]
rows = re.findall(
    r'^\s*\{"([^"]+)", \{([^{}]+)\}, \{([^{}]+)\}, '
    r'(-?\d+), TAG\(NONE\), FX\(([^()]*)\), PAT\(0\)\},',
    body, flags=re.M,
)
all_names = [line.strip().split('"')[1] for line in body.splitlines()
             if line.strip().startswith('{"')]
assert tuple(all_names) == expected, ("Factory FM6 bank includes missing or legacy patches", all_names)
assert len(rows) == 10
combined = []
for i, (name, eng, env, fenv, fx) in enumerate(rows):
    e = [int(v.strip()) for v in eng.split(",")]
    envelope = [int(v.strip()) for v in env.split(",")]
    effects = [int(v.strip()) for v in fx.split(",")]
    assert name == expected[i]
    assert len(e) == 8 and envelope == common[i][1:5]
    assert int(fenv) == common[i][5] and effects == common[i][33:37]
    combined.append([i, name, common[i], e, envelope, int(fenv), effects, patches[i]])

payload = json.dumps(combined, separators=(",", ":"))
h = 2166136261
for ch in payload:
    h = ((h ^ ord(ch)) * 16777619) & 0xFFFFFFFF
assert f"{h:08x}" == "497464f0", f"Source-bank fidelity mismatch: {h:08x}"
assert "fm6_unpack(FM6_IMPORT_PATCH[pi], unpacked)" in ui
assert "fm6_set_patch(trk_index(t), unpacked)" in ui
assert "fm6_adopt(trk_index(t))" in ui
assert "pi < FM6_IMPORT_N" in ui
print("PASS: 10 FM6 factory sounds match exported name, common params, macros, and full operator patches")
