#!/usr/bin/env python3
"""Verify all 15 imported VOICE/LOFI factory sounds against the uploaded bank."""
import json
import re
from pathlib import Path

bank = json.loads(Path("tests/fixtures/voice_lofi_factory_bank.json").read_text())
header = Path("firmware/src/voice_lofi_factory_import.h").read_text()
ui = Path("firmware/src/ui.c").read_text()
assert (bank["pCount"], bank["pE0"], len(bank["patches"])) == (127, 115, 15)
assert "#pragma once" in header

for engine, ident, stem, source, table, count in (
    ("VOICE", 5, "FORMANT", "firmware/src/eng_formant.c", "FORMANT_PRESETS", 5),
    ("LOFI", 3, "LOFI", "firmware/src/eng_lofi.c", "LOFI_PRESETS", 10),
):
    patches = [p for p in bank["patches"] if p["engine"] == ident]
    assert len(patches) == count and all(p["engineName"] == engine for p in patches)
    assert all(len(p["params"]) == 127 for p in patches)
    assert f"#define {stem}_FACTORY_IMPORTED {count}u" in header

    common = header.split(
        f"static const int8_t {stem}_IMPORT_COMMON[{stem}_FACTORY_IMPORTED][P_E0] = {{", 1
    )[1].split("\n};", 1)[0]
    rows = re.findall(r"/\* (\d+)\. ([A-Z0-9-]+) \*/\s*\{([^{}]+)\}", common)
    assert len(rows) == count, (engine, "common row count", len(rows))
    for i, (index, name, values) in enumerate(rows):
        actual = [int(v) for v in re.findall(r"-?\d+", values)]
        assert (int(index), name) == (i + 1, patches[i]["name"])
        assert actual == patches[i]["params"][:115], (engine, name, "common mismatch")

    body = Path(source).read_text().split(f"static const preset_t {table}[] = {{", 1)[1].split("\n};", 1)[0]
    entries = re.findall(
        r'^\s*\{"([^"]+)", \{([^{}]+)\}, \{([^{}]+)\}, (-?\d+), '
        r'TAG\(NONE\), FX\(([^()]*)\), PAT\(0\)\},',
        body, re.M,
    )
    assert len(entries) == count, (engine, "preset count", len(entries))
    for patch, (name, edit, env, fenv, sends) in zip(patches, entries):
        params = patch["params"]
        assert name == patch["name"]
        assert [int(x.strip()) for x in edit.split(",")] == params[115:123], name
        assert [int(x.strip()) for x in env.split(",")] == params[1:5], name
        assert int(fenv) == params[5], name
        assert [int(x.strip()) for x in sends.split(",")] == params[33:37], name
    symbol = "ENGI_VOICE" if engine == "VOICE" else "ENGI_LOFI"
    assert f"trk_eng(t) == {symbol} && pi < {stem}_FACTORY_IMPORTED" in ui
    assert f"const int8_t *v = {stem}_IMPORT_COMMON[pi];" in ui
    print(f"PASS: {count} {engine} factory presets match all 127 exported parameters")

print("PASS: previous VOICE/LOFI factory sounds completely replaced")
