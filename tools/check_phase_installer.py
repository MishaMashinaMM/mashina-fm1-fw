#!/usr/bin/env python3
"""Validate and clearly label the isolated PHASE test WebMIDI installer."""
import hashlib
import json
import sys
from pathlib import Path

site = Path(sys.argv[1])
fw = site / "firmware" / "felucca-0.10-preview.fwsc"
installer = site / "webapp" / "installer" / "index.html"
versions = site / "firmware" / "versions.json"
assert fw.is_file() and fw.stat().st_size > 100_000, "Firmware image missing or too small"
assert installer.is_file(), "WebMIDI installer missing"
j = json.loads(versions.read_text(encoding="utf-8"))
assert len(j) == 1 and j[0]["version"] == "0.10-preview", j
assert j[0]["product"] == "FM-1_910", j
raw = fw.read_bytes()
assert b"FELUCCA-LOADER-1" in raw, "Not a complete update package"
digest = hashlib.sha256(raw).hexdigest()
assert (site / "firmware" / "RELEASE_SHA256").read_text().strip() == digest, "Package SHA mismatch"
html = installer.read_text(encoding="utf-8")
assert "felucca-0.10-preview.fwsc" in html, "Installer does not point to PHASE preview"
assert "FM-1_910" in html, "Installer product identity mismatch"
assert "0.10-preview" in html, "Installer has wrong version"
warning = """<div role="alert" style="position:sticky;top:0;z-index:9999;background:#261400;color:#ffb84d;padding:14px;text-align:center;border-bottom:2px solid #ff9900;font:700 14px/1.5 Arial,sans-serif">
MASHINA PHASE FACTORY TEST · v0.10-preview · 10 PHASE presets · NOT THE OFFICIAL RELEASE
</div>"""
assert "</body>" in html, "Cannot insert preview banner"
html = html.replace("</body>", warning + "\n</body>", 1)
installer.write_text(html, encoding="utf-8")
# The generated second installer entry must not be a mislabeled production page.
(site / "webapp" / "installer-orange-v010" / "index.html").write_text(html, encoding="utf-8")
(site / "README-PHASE-TEST.txt").write_text(
    "MASHINA v0.10-preview | PHASE 10-preset FM1 test firmware\n"
    "Source: factory-presets-phase-10. This is NOT the official v0.10 installer.\n\n"
    "1. Back up existing FM1 projects and user sounds before flashing.\n"
    "2. Extract the complete ZIP to a directory.\n"
    "3. In that directory run: python -m http.server 8790\n"
    "4. In desktop Chrome or Edge visit: http://localhost:8790/webapp/installer/\n"
    "5. Check the orange PHASE FACTORY TEST banner before starting.\n"
    "6. Keep the FM1 connected until installation completes.\n\n"
    "Do not launch index.html using file:// or serve only the installer subfolder.\n"
    "The firmware file is under firmware/, and the installer uses relative paths.\n"
    f"Package SHA256: {digest}\n",
    encoding="utf-8",
)
print(f"PASS: isolated test installer, FM-1_910 identity, package SHA256 {digest}")
