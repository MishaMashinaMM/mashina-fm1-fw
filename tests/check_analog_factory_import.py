#!/usr/bin/env python3
"""Verify the 18 MASHINA ANALOG factory presets against the accepted JSON export.

The fingerprint covers each preset's original user-bank slot, name, all common
sound parameters, eight ANALOG EDIT parameters, envelope, filter envelope and FX.
This catches accidental omissions in a source edit or regeneration.
"""
import json
import re
from pathlib import Path

engine = Path("firmware/src/eng_analog.c").read_text(encoding="utf-8")
overlay = Path("firmware/src/analog_factory_import.h").read_text(encoding="utf-8")
expected_names = (
    "THALES", "ANAXIMENES", "ANAXIMANDER", "ZENO", "PARMENIDES",
    "XENOPHANES", "HERACLITUS", "PYTHAGORAS", "EMPEDOCLES",
    "PROTAGORAS", "GORGIAS", "ANAXAGORAS", "LEUCIPPUS",
    "SOCRATES", "DEMOCRITUS", "ANTISTENES", "PRODICUS", "ARCHYTAS",
)
rows = re.findall(r"/\* (\d+)\. ([A-Z]+) \*/\s*\{([^{}]+)\}", overlay)
assert len(rows) == 18, f"Expected 18 imported common arrays, got {len(rows)}"
common = []
for i, (num, name, values) in enumerate(rows):
    assert int(num) == i + 1 and name == expected_names[i]
    p = [int(s) for s in re.findall(r"-?\d+", values)]
    assert len(p) == 115, (name, len(p))
    assert all(-128 <= v <= 127 for v in p)
    common.append(p)

factory = engine.split("static const preset_t ANALOG_PRESETS[] = {", 1)[1].split("};", 1)[0]
items = re.findall(
    r'^\s*\{"([^"]+)", \{([^{}]+)\}, \{([^{}]+)\}, '
    r'(-?\d+), TAG\(NONE\), FX\(([^()]*)\), PAT\(0\)\},',
    factory, flags=re.M,
)
assert len(items) == 18, f"Expected 18 replacement factory entries, got {len(items)}"
combined = []
for i, (name, e, env, fenv, fx) in enumerate(items):
    assert name == expected_names[i]
    eng = [int(v.strip()) for v in e.split(",")]
    envelope = [int(v.strip()) for v in env.split(",")]
    effects = [int(v.strip()) for v in fx.split(",")]
    assert len(eng) == 8 and len(envelope) == 4 and len(effects) == 4
    assert envelope == common[i][1:5], name
    assert int(fenv) == common[i][5], name
    assert effects == common[i][33:37], name
    combined.append([i, name, common[i], eng, envelope, int(fenv), effects])

payload = json.dumps(combined, separators=(",", ":"))
h = 2166136261
for ch in payload:
    h = ((h ^ ord(ch)) * 16777619) & 0xFFFFFFFF
assert f"{h:08x}" == "9be859a5", f"Export fidelity fingerprint changed: {h:08x}"
assert "if (trk_eng(t) == ENGI_ANALOG && pi < ANALOG_FACTORY_IMPORTED)" in Path("firmware/src/ui.c").read_text()
order = Path("firmware/src/engines.c").read_text(encoding="utf-8").split("ENGINE_ORDER[NENG_SHOWN] = {", 1)[1].split("};", 1)[0]
assert re.findall(r"ENGI_([A-Z0-9]+)\\s*,", order)[0] == "ANALOG", "ANALOG must be first in global PRESETS order"
print("PASS: 18 imported ANALOG sounds occupy global presets 1-18 and match the complete export")
