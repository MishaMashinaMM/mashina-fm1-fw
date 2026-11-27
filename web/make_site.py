#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
"""Make the site (GitHub Pages):

  index.html                  redirect to the installer (the old URL keeps working)
  firmware/felucca-VER.fwsc   the package (+ LICENSE, LICENSING.md, LICENSES/: the package holds
                              JieLi SDK files under Apache-2.0, see LICENSING.md)
  webapp/installer/index.html index_pkg.html, self-contained (fm1pkg.js, fm1ota.js, fm1backup.js, fm1diag.js and the
                              metadata inlined)
  404.html                    web/404.html: an old or mistyped link (/installer/, /webapp/, ...:1:1) to the page it meant
  webapp/editor/index.html    editor.html (+ fukiai.ttf, FUKIAI-LICENSE.txt)
  src/                        not touched

  web/make_site.py build/felucca-X.Y.fwsc X.Y OUT_DIR [PREV_DIR]
  web/make_site.py manual OUT_DIR   generates the six-language MASHINA operating guide from docs/MASHINA_MANUAL*.md

PREV_DIR (JIANT 0.6.46): earlier versions offered next to the current one (firmware/versions.json, the installer's
version list): its versions.json ([{"version", "file"}]) and the packages it names, up to KEEP_OLD, one build per
version. The MASHINA main workflow publishes only the current release. The optional PREV_DIR parameter remains available
for explicit local packaging, where each package keeps its own identity and is validated before inclusion.

The package must be one made by tools/fm1pkg_make.py (Felucca's own loader, no vendor files).
Its identity (FM-1_9xx) is read from the package; the device must report it after
the install.
"""
import json
import hashlib
import os
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F
KEEP_OLD = 8  # limit for optional, explicitly supplied previous release packages
SAFE = re.compile(r"felucca-[A-Za-z0-9.-]+\.fwsc")


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def base_of(version):
    """v0.6.46-abc1234 -> v0.6.46: the builds of one version (0.9.9-beta4: only a commit's 7 hex digits come off, so
    v0.9.9-beta3 and v0.9.9 stay two versions)"""
    return re.sub(r"-[0-9a-f]{7}$", "", version)


def earlier(prev, version, product):
    """[(entry, file)]: PREV_DIR's versions, newest first, one per version, not this one, at most KEEP_OLD, each
    package there and a Felucca package for this model (its own identity goes in the entry: an older line has another)"""
    out, seen = [], {base_of(version)}
    try:
        listed = json.loads((prev / "versions.json").read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return out
    for e in listed if isinstance(listed, list) else []:
        v, f = str(e.get("version", "")), str(e.get("file", ""))
        src = prev / f
        if not SAFE.fullmatch(f) or base_of(v) in seen or not src.is_file():
            continue
        raw = src.read_bytes()
        pf = product_of(raw)
        if pf.split("_")[0] != product.split("_")[0] or b"FELUCCA-LOADER-1" not in raw:
            continue
        seen.add(base_of(v))
        out.append(({"version": v, "file": f, "product": pf}, src))
        if len(out) >= KEEP_OLD:
            break
    return out


def manual(out):
    """Publish the current MASHINA usage manual as a standalone responsive page.

    docs/MASHINA_MANUAL.md is the single source for the web and Markdown manuals.
    Historical interactive-manual assets were retired; the operating manuals
    remain the source of truth for all six site languages.
    """
    import markdown
    src = HERE / "manual" / "index.html"
    dst = Path(out) / "manual"
    dst.mkdir(parents=True, exist_ok=True)
    template = src.read_text(encoding="utf-8")
    marker = "<!--MASHINA_MANUAL_CONTENT-->"
    if template.count(marker) != 1:
        raise SystemExit("MASHINA manual template must contain exactly one content marker")

    all_articles = []
    for lang in ("en", "es", "ja", "ru", "sr", "hi"):
        name = "MASHINA_MANUAL.md" if lang == "en" else f"MASHINA_MANUAL.{lang}.md"
        md = HERE.parent / "docs" / name
        source = md.read_text(encoding="utf-8")
        content = markdown.markdown(source, extensions=["tables", "toc", "fenced_code"],
                                    extension_configs={"toc": {"toc_depth": "2-3"}})
        # Scope heading anchors to the language to avoid duplicate IDs.
        import re as _re
        content = _re.sub(r' id="([^"]+)"', lambda m: f' id="{lang}-{m.group(1)}"', content)
        content = _re.sub(r'href="#([^"]+)"', lambda m: f'href="#{lang}-{m.group(1)}"', content)
        toc = []
        for lvl, anchor, label in _re.findall(
            r'<h([23]) id="([^"]+)">([^<]+)</h[23]>', content
        ):
            indent = " style='margin-left:16px'" if lvl == "3" else ""
            toc.append(f'<li{indent}><a href="#{anchor}">{label}</a></li>')
        titles = {"en": "Contents", "es": "Contenido", "ja": "目次",
                  "ru": "Содержание", "sr": "Садржај", "hi": "विषय-सूची"}
        toc_html = '<nav id="toc" aria-label="Contents"><strong>' + titles[lang] + '</strong><ul>' + "".join(toc) + '</ul></nav>'
        content = content.replace("</h1>", "</h1>" + toc_html, 1)
        for forbidden in ("JLD", "development process"):
            if forbidden.lower() in content.lower():
                raise SystemExit(f"{name} includes non-user-facing reference: {forbidden}")
        visible = " data-active" if lang == "en" else ""
        locale = "sr-Cyrl" if lang == "sr" else lang
        all_articles.append(f'<article lang="{locale}" data-manual-lang="{lang}"{visible}>\n{content}\n</article>')
    html = template.replace(marker, "\n".join(all_articles))
    (dst / "index.html").write_text(html, encoding="utf-8")
    print(f"site: {dst}: MASHINA user manual ({len(html)} B)")


def main(pkg, version, out, prev=None):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_9\d\d", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not a Felucca package (FM-1_9xx)")
    if b"FELUCCA-LOADER-1" not in raw:              # Felucca loader marker: never publish a package with vendor files
        raise SystemExit(f"{pkg}: no Felucca loader in it; the site ships only fm1pkg_make.py packages "
                         "(a package patched from an official one carries vendor files)")
    html = (HERE / "index_pkg.html").read_text(encoding="utf-8")
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1backup.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1diag.js").read_text(encoding="utf-8"))
    name = f"felucca-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    old = earlier(Path(prev), version, product) if prev else []
    versions = [{"version": version, "file": name, "product": product}] + [e for e, _ in old]
    editions = json.loads((HERE / "EDITIONS.json").read_text(encoding="utf-8"))
    for entry in versions:
        entry.update(editions.get(entry["version"], {}))
    # This fork publishes only its own MASHINA build, never an upstream edition
    # or an older firmware selected from an unrelated site.
    selected = versions[0]
    release_hash = hashlib.sha256(raw).hexdigest()[:12]
    source_commit = os.getenv("GITHUB_SHA", "")[:12] or "local"
    release_tag = f"{source_commit}-{release_hash}"
    meta = json.dumps({"version": selected["version"], "product": selected["product"],
                       "pkg": "../../firmware/" + selected["file"] + "?release=" + release_hash,
                       "source_commit": source_commit, "release_hash": release_hash,
                       "channel": "preview" if version.endswith("-preview") else "public",
                       "versions": [{"version": e["version"], "pkg": "../../firmware/" + e["file"] + "?release=" + (release_hash if e == selected else "historic"), "product": e["product"],
                                     "edition": e.get("edition", ""), "status": e.get("status", "")}
                                    for e in versions]})
    for mark in ("/*LIB*/", "/*META*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"index_pkg.html must contain {mark} once; update make_site.py")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta)
    inst, ed, fw = out / "webapp" / "installer", out / "webapp" / "editor", out / "firmware"
    for d in (inst, ed, fw):
        d.mkdir(parents=True, exist_ok=True)
    for f in fw.glob("felucca-*.fwsc"):            # the current package and the earlier ones kept, nothing else
        f.unlink()
    brand = (HERE / "jiant-ui.css").read_text(encoding="utf-8")
    installer_html = html.replace("/*JIANT_UI*/", brand).replace("/*BUILD_TAG*/", release_tag)
    if version != "0.16":
        installer_html = installer_html.replace("v0.16 · MASHINA · BUILD", f"v{version.lstrip(chr(118))} · MASHINA · BUILD")
    if version in ("0.15", "0.16", "0.14-lib96test", "0.15-bletest"):
        needle = "await runInstall({ up, image, product, ui, state });"
        assert installer_html.count(needle) == 1
        installer_html = installer_html.replace(needle, 'await runInstall({ up, image, product, ui, state, cleanLib96Trial:true, confirmTrialBackup:()=>confirm(t("firstText")) });')
    (inst / "index.html").write_text(installer_html, encoding="utf-8")
    # New permanent URL gives browsers a fresh document even when the older installer URL was cached.
    fresh = out / "webapp" / ("installer-orange-v" + version.replace(".", ""))
    fresh.mkdir(parents=True, exist_ok=True)
    (fresh / "index.html").write_text(installer_html, encoding="utf-8")
    shutil.copy(pkg, fw / name)
    for e, src in old:
        shutil.copy(src, fw / e["file"])
    (fw / "versions.json").write_text(json.dumps(versions, indent=1) + "\n", encoding="utf-8")
    (fw / "RELEASE_SHA256").write_text(hashlib.sha256(raw).hexdigest() + "\n", encoding="utf-8")
    lic = HERE.parent / "LICENSES"                  # the package holds JieLi SDK files (Apache-2.0): their
    (fw / "LICENSES").mkdir(exist_ok=True)          # licence travels next to it, with Felucca's own
    names = sorted(f.name for f in lic.glob("*.txt"))
    for n in names:
        shutil.copy(lic / n, fw / "LICENSES" / n)
    (fw / "LICENSES" / "index.html").write_text(    # the installer links this folder: Pages lists no folders
        '<!doctype html><meta charset="utf-8"><title>Felucca licences</title><h1>Licence texts</h1><ul>'
        + "".join(f'<li><a href="{n}">{n}</a></li>' for n in names)
        + '</ul><p><a href="../LICENSING.md">LICENSING.md</a> · <a href="../LICENSE">LICENSE (GPL-3.0)</a></p>\n',
        encoding="utf-8")
    for doc in ("LICENSE", "LICENSING.md"):
        shutil.copy(HERE.parent / doc, fw / doc)
    editor = (HERE / "editor.html").read_text(encoding="utf-8").replace("/*JIANT_UI*/", brand)
    (ed / "index.html").write_text(editor, encoding="utf-8")
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt", "fm1backup.js", "factory-samples.json", "factory-voices.json", "FACTORY-VOICES.md", "FACTORY-SAMPLES.md"):
        if (HERE / f).exists():
            shutil.copy(HERE / f, ed / f)
    if version in ("0.14", "0.15", "0.16", "0.14-lib96test", "0.15-bletest"):
        library = out / "web-library"
        library.mkdir(parents=True, exist_ok=True)
        for src, dest in (("presets-library-manager.html", "index.html"), ("presets-library-guide.html", "guide.html"), ("initialize-library.html", "initialize.html")):
            shutil.copy(HERE / src, library / dest)
        for f in ("presets-library-codec.mjs", "presets-library-midi.mjs", "presets-library-sync.mjs", "factory-presets-from-code.json", "factory-catalog-selection.mjs", "full-device-backup.mjs", "projects-library-codec.mjs", "projects-library-view.mjs", "performance-backup.mjs", "fm1backup.js", "legacy-user-extract.mjs", "emulator-snapshot-editor.mjs", "emulator-library-inspector.mjs"):
            shutil.copy(HERE / f, library / f)
        (library / "emu").mkdir(exist_ok=True)
        for f in ("worklet.js", "emulator-flash-backup.mjs"):
            shutil.copy(HERE / "emu" / f, library / "emu" / f)
        shutil.copy(HERE.parent / "build/host/factory-drum-records.json", library / "factory-drum-records.json")
        if (HERE / "factory-final-library.json").exists():
            shutil.copy(HERE / "factory-final-library.json", library / "factory-final-library.json")
        # The engineering lab is not part of the public manager. Keep the
        # DOM element for its harmless event binding, but do not expose a link.
        public_manager = library / "index.html"
        public_manager.write_text(public_manager.read_text(encoding="utf-8").replace('<div class="panel advanced">', '<div class="panel advanced" style="display:none">').replace('href="./advanced.html"', 'href="./guide.html"'), encoding="utf-8")
        if version in ("0.15", "0.16", "0.14-lib96test", "0.15-bletest"):
            # This layout initializes itself before any old state is read.
            (library / "initialize.html").unlink()
            manager = public_manager.read_text(encoding="utf-8")
            manager = manager.replace('href="initialize.html" hidden>Initialize storage', 'href="./guide.html" hidden>Storage information')
            public_manager.write_text(manager, encoding="utf-8")
    shutil.copy(HERE / "404.html", out / "404.html")
    (out / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>MASHINA</title>'
        '<meta http-equiv="refresh" content="0; url=webapp/installer/">'
        '<a href="webapp/installer/">MASHINA installer</a>\n', encoding="utf-8")
    print(f"site: {out}: webapp/installer ({len(html)} B), webapp/editor, firmware/{name} ({len(raw)} B, {product})"
          + (f", earlier: {' '.join(e['version'] for e, _ in old)}" if old else ""))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "manual":
        manual(sys.argv[2])
        sys.exit(0)
    if len(sys.argv) not in (4, 5):
        sys.exit(__doc__)
    main(*sys.argv[1:5])

