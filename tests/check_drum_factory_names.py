#!/usr/bin/env python3
"""Assert factory drum kits have sequential names; do not change kit data."""
from pathlib import Path
import re

for path, symbol, prefix, count in (
    ("firmware/src/eng_drum.c", "DRUM_PRESETS", "BUTTLER", 9),
    ("firmware/src/eng_drumlab.c", "DRUMLAB_PRESETS", "ZIZEK", 3),
    ("firmware/src/eng_drumac.c", "AC_PRESETS", "KRIPKE", 3),
    ("firmware/src/eng_drumind.c", "IND_PRESETS", "BADIOU", 3),
):
    src = Path(path).read_text(encoding="utf-8")
    body = src.split(f"static const preset_t {symbol}[] = {{", 1)[1].split("\n};", 1)[0]
    names = re.findall(r'^\s*\{"([^"]+)"\s*,', body, re.M)
    expected = [f"{prefix}-{i}" for i in range(1, count + 1)]
    assert names == expected, (path, names, expected)
    assert all(len(x) <= 12 for x in names), path
    print(f"PASS: {symbol}: {', '.join(names)}")
