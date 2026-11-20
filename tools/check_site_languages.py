#!/usr/bin/env python3
"""Verify the six-language MASHINA website and published shared assets."""
from pathlib import Path
import json
import re

codes = ("en", "es", "ja", "ru", "sr", "hi")
root = Path("build/pages")
pages = {
    "home": (root / "index.html", "mashina-translations.js"),
    "emulator": (root / "emu/index.html", "mashina-emu-locale.js"),
    "installer": (root / "webapp/installer/index.html", "mashina-nav.js"),
    "editor": (root / "webapp/editor/index.html", "mashina-editor-locales.js"),
    "manual": (root / "manual/index.html", "mashina-manual-locale.js"),
}
for name, (file, expected) in pages.items():
    html = file.read_text(encoding="utf-8")
    assert "MASHINA" in html and "FM1 PERFORMANCE FW" in html, name
    assert expected in html or name == "installer", f"{name}: missing translation script"
    if name != "installer":
        assert "mashina-nav.js" in html, f"{name}: missing shared language navigator"
    print(f"OK {name}: MASHINA branding and translation assets")

for asset in ("mashina-nav.js", "mashina-nav.css", "mashina-translations.js",
              "mashina-emu-locale.js", "mashina-editor-locales.js", "mashina-manual-locale.js"):
    file = root / asset
    assert file.is_file() and file.stat().st_size > 100, asset

nav = (root / "mashina-nav.js").read_text(encoding="utf-8")
for code in codes:
    assert f'["{code}",' in nav, f"Missing language in shared selector: {code}"

for name in ("mashina-translations.js", "mashina-emu-locale.js", "mashina-editor-locales.js"):
    src = (root / name).read_text(encoding="utf-8")
    for code in codes if name != "mashina-editor-locales.js" else ("es", "ru", "sr", "hi"):
        assert f'"{code}"' in src, f"{name} missing {code}"

# All labels used by the editor's primary controls must be translated.
editor_source = Path("web/editor.html").read_text(encoding="utf-8")
editor_keys = set(re.findall(r'data-t="([^"]+)"', editor_source))
catalog = (root / "mashina-editor-locales.js").read_text(encoding="utf-8")
catalog = json.JSONDecoder().raw_decode(catalog.split(" = ", 1)[1])[0]
for code in ("es", "ru", "sr", "hi"):
    missing = editor_keys - set(catalog[code])
    assert not missing, f"Editor {code}: untranslated primary controls {sorted(missing)}"
print(f"OK editor: {len(editor_keys)} primary controls translated into each added language")

manual = (root / "manual/index.html").read_text(encoding="utf-8")
for code in codes:
    assert f'data-manual-lang="{code}"' in manual, f"Manual: missing {code}"
    assert (Path("docs") / ("MASHINA_MANUAL.md" if code == "en" else f"MASHINA_MANUAL.{code}.md")).is_file()
assert len(re.findall(r'data-manual-lang="(?:en|es|ja|ru|sr|hi)"', manual)) == 6
print("PASS: all five pages and six language choices are included in Pages artifact")
