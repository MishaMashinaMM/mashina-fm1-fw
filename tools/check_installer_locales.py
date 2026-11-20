#!/usr/bin/env python3
"""Check supported MASHINA installer language coverage and visible content."""
from pathlib import Path
import json
import re

html = Path("web/index_pkg.html").read_text(encoding="utf-8")
english = html.split("  en: {", 1)[1].split("TEXT.es = {", 1)[0]
english_keys = set(re.findall(r"(?m)^\s*([A-Za-z_][\w]*):", english)) - {"en"}
ru_blob = html.split("TEXT.ru = ", 1)[1].split(";\n", 1)[0]
russian = json.loads(ru_blob)
missing = sorted(english_keys - set(russian))
assert not missing, f"Untranslated installer strings: {missing}"
assert all(isinstance(v, str) and v.strip() for v in russian.values())
assert 'data-lang="ru" lang="ru">Русский</button>' in html
assert 'const LANGS = ["es", "en", "ja", "ru"]' in html
assert 'navLang.startsWith("ru")' in html
assert "November 18, 2026" not in html
assert "Previous v0.9 firmware features were tested" not in html
assert 'FM1 PERFORMANCE FW</span></a>' in html
print(f"PASS: Russian installer localization covers {len(english_keys)} English keys; all four languages offered")

manual = Path("docs/MASHINA_MANUAL.md").read_text(encoding="utf-8")
for forbidden in ("JIANT", "JLD", "Juan", "firmware build", "development"):
    assert forbidden.lower() not in manual.lower(), f"Manual should only describe MASHINA usage: {forbidden}"
assert Path("web/manual/index.html").read_text(encoding="utf-8").count("<!--MASHINA_MANUAL_CONTENT-->") == 1
print("PASS: canonical MASHINA manual and responsive page template")
