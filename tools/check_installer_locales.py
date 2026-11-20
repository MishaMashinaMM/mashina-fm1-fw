#!/usr/bin/env python3
"""Check supported MASHINA installer language coverage and visible content."""
from pathlib import Path
import json
import re

html = Path("web/index_pkg.html").read_text(encoding="utf-8")
english = html.split("  en: {", 1)[1].split("TEXT.es = {", 1)[0]
english_keys = set(re.findall(r"(?m)^\s*([A-Za-z_][\w]*):", english)) - {"en"}
for lang, name in [("ru", "Русский"), ("sr", "Српски"), ("hi", "हिन्दी")]:
    strings = json.JSONDecoder().raw_decode(html.split(f"TEXT.{lang} = ", 1)[1])[0]
    missing = sorted(english_keys - set(strings))
    assert not missing, f"Untranslated installer strings ({lang}): {missing}"
    assert all(isinstance(v, str) and v.strip() for v in strings.values())
    assert f'data-lang="{lang}"' in html, f"Missing selector for {name}"
assert 'const LANGS = ["en", "es", "ja", "ru", "sr", "hi"]' in html
for code in ("en", "es", "ja", "ru", "sr", "hi"):
    assert f'data-lang="{code}"' in html
assert "November 18, 2026" not in html
assert "Previous v0.9 firmware features were tested" not in html
assert 'FM1 PERFORMANCE FW</span></a>' in html
print(f"PASS: Russian, Serbian and Hindi installer locales have {len(english_keys)} keys; all six languages offered")

manual = Path("docs/MASHINA_MANUAL.md").read_text(encoding="utf-8")
for forbidden in ("JLD", "firmware build", "development process"):
    assert forbidden.lower() not in manual.lower(), f"Manual should only describe MASHINA usage: {forbidden}"
assert Path("web/manual/index.html").read_text(encoding="utf-8").count("<!--MASHINA_MANUAL_CONTENT-->") == 1
print("PASS: canonical MASHINA manual and responsive page template")

# The English manual is canonical; each translation must provide detailed
# operating instructions, not just a seven-section outline.
base = Path("docs/MASHINA_MANUAL.md")
english_text = base.read_text(encoding="utf-8")
for lang in ("es", "ja", "ru", "sr", "hi"):
    file = Path(f"docs/MASHINA_MANUAL.{lang}.md")
    content = file.read_text(encoding="utf-8")
    headings = re.findall(r"(?m)^#{2,3} .+$", content)
    rows = re.findall(r"(?m)^\\|[^\\n]+\\|$", content)
    assert len(headings) >= 20, f"{lang}: manual is missing detailed subsections ({len(headings)})"
    assert len(rows) >= 50, f"{lang}: manual needs complete settings/control tables ({len(rows)})"
    assert len(content) >= len(english_text) * 0.66, f"{lang}: manual is unexpectedly abridged"
    for token in ("MIXER", "ROUTE", "ZONES", "TUNING", "ATMOS", "BRASS",
                  "INDUSTRIAL", "DRUM-X", "DRUM-LAB", "STRIKE", "SEQ",
                  "PLAY", "REC", "MIDI Learn", "CC85", "CC86", "CC87",
                  "CC88", "CC89", "CC90", "PANIC", "EQ", "INIT SOUND",
                  "CLEAR SLOT", "NEW PROJECT", "CPU TEST"):
        assert token.lower() in content.lower(), f"{lang}: missing manual topic {token}"
    for band in ("50", "700", "10"):
        assert band in content, f"{lang}: missing global EQ frequency {band}"
    for val in ("36", "50", "127", "80", "81", "15", "32"):
        assert val in content, f"{lang}: missing reference value {val}"
    for pad in ("BD", "SD", "CP", "CH", "OH", "TM", "RM", "BL", "B2", "S2", "O2", "T2"):
        assert re.search(r"\b" + re.escape(pad) + r"\b", content), f"{lang}: missing DRUM-X pad {pad}"
    for forbidden in ("JLD", "development process"):
        assert forbidden.lower() not in content.lower(), f"{lang}: unrelated manual attribution {forbidden}"
    print(f"PASS: {lang} manual — {len(headings)} sections/subsections, {len(rows)} table rows")

# Opening a bare page URL must show English regardless of browser language,
# previous settings or an earlier in-site language selection.
nav = Path("web/mashina-nav.js").read_text(encoding="utf-8")
editor = Path("web/editor.html").read_text(encoding="utf-8")
assert 'const code = langs.some(x => x[0] === requested) ? requested : "en";' in nav
assert 'let lang = "en";' in html
assert 'let lang = EDITOR_LANGS.includes(editorQueryLang) ? editorQueryLang : "en";' in editor
for source in (nav, html, editor):
    assert 'localStorage.getItem("mashina-lang")' not in source
print("PASS: English is the default for all bare URLs; explicit ?lang= selects translations")

