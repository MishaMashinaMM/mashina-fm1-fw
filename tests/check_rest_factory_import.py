#!/usr/bin/env python3
"""Check all 27 new factory sounds against the exact uploaded JSON.
Also ensure the original ANALOG first 18 have not been displaced.
"""
import json
import re
from pathlib import Path

bank = json.loads(Path("tests/fixtures/rest_factory_bank.json").read_text())
assert (bank["pCount"], bank["pE0"], len(bank["patches"])) == (127, 115, 27)
ui = Path("firmware/src/ui.c").read_text()
header = Path("firmware/src/rest_factory_import.h").read_text()
specs = (
    (0, "ANALOG", 2, "ANALOG_PRESETS", "firmware/src/eng_analog.c", "ANALOG"),
    (16, "INDUSTRI", 12, "INDUSTRIAL_PRESETS", "firmware/src/eng_industrial.c", "INDUSTRIAL"),
    (17, "BRASS", 5, "BRASS_PRESETS", "firmware/src/eng_brass.c", "BRASS"),
    (18, "ATMOS", 8, "ATMOS_PRESETS", "firmware/src/eng_atmos.c", "ATMOS"),
)
for engine_id, label, count, table, path, ident in specs:
    patches = [p for p in bank["patches"] if p["engine"] == engine_id]
    assert len(patches) == count and all(p["engineName"] == label for p in patches)
    assert all(len(p["params"]) == 127 for p in patches)
    src = Path(path).read_text()
    body = src.split(f"static const preset_t {table}[] = {{", 1)[1].split("\n};", 1)[0]
    entries = re.findall(
        r'^\s*\{"([^"]+)", \{([^{}]+)\}, \{([^{}]+)\}, (-?\d+), '
        r'TAG\(NONE\), FX\(([^()]*)\), PAT\(0\)\},', body, re.M)
    assert len(entries) == count + (18 if engine_id == 0 else 0), (label, len(entries))
    if engine_id == 0:
        entries = entries[-count:]
    for p, (name, e, env, fenv, fx) in zip(patches, entries):
        ps = p["params"]
        assert name == ({"ANALOG 01": "CICERO", "ANALOG 02": "PLUTARCH"}.get(p["name"], p["name"]))
        assert [int(v.strip()) for v in e.split(",")] == ps[115:123], name
        assert [int(v.strip()) for v in env.split(",")] == ps[1:5], name
        assert int(fenv) == ps[5], name
        assert [int(v.strip()) for v in fx.split(",")] == ps[33:37], name
    if engine_id == 0:
        common_src = Path("firmware/src/analog_factory_import.h").read_text()
        assert "#define ANALOG_FACTORY_IMPORTED 20u" in common_src
        offset = 18
        assert [e[0] for e in entries] == ["CICERO", "PLUTARCH"]
    else:
        common_src = header
        offset = 0
        assert f"#define {ident}_FACTORY_IMPORTED {count}u" in common_src
        assert f"trk_eng(t) == ENGI_{ident} && pi < {ident}_FACTORY_IMPORTED" in ui
        extras = re.search(
            rf"static const uint8_t {ident}_EXTRA_PRESETS\[\]\[4\] = \{{(.*?)\n\}};",
            src, re.S)
        assert extras, f"{ident} extra parameters missing"
        extra_rows = re.findall(r"\{([^{}]+)\}", extras.group(1))
        assert len(extra_rows) == count, (ident, len(extra_rows))
        for p, row in zip(patches, extra_rows):
            assert [int(v.strip()) for v in row.split(",")] == p["params"][123:127], p["name"]
    arr = common_src.split(
        f"static const int8_t {ident}_IMPORT_COMMON[{ident}_FACTORY_IMPORTED][P_E0] = {{", 1
    )[1].split("\n};", 1)[0]
    rows = re.findall(r"/\* (\d+)\. ([A-Z0-9 -]+) \*/\s*\{([^{}]+)\}", arr)
    assert len(rows) == count + offset, (ident, len(rows))
    for idx, p in enumerate(patches):
        pos, name, values = rows[idx + offset]
        assert int(pos) == idx + offset + 1 and name == ({"ANALOG 01": "CICERO", "ANALOG 02": "PLUTARCH"}.get(p["name"], p["name"]))
        assert [int(x) for x in re.findall(r"-?\d+", values)] == p["params"][:115], name
    print(f"PASS: {count} new {label} presets match exported 127-parameter values")

engine_order = Path("firmware/src/engines.c").read_text()
assert "global factory positions 1..20" in engine_order
print("PASS: two ANALOG presets appended; remaining engines shift in global ordering")
