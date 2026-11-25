#!/usr/bin/env python3
"""Fail publication if README's primary online links do not resolve in Pages output."""
from pathlib import Path
from urllib.parse import urlparse
import re

root = Path("build/pages")
readme = Path("README.md").read_text(encoding="utf-8")
section = readme.split("### MASHINA online", 1)[1].split("\n\n", 2)[1].split("\n\n", 1)[0]
links = re.findall(r"\[([^]]+)\]\((https://[^)]+)\)", section)
expected = {
    "Project homepage": "index.html",
    "Play in emulator": "emu/index.html",
    "FM1 web installer": "webapp/installer/index.html",
    "Web editor": "webapp/editor/index.html",
    "Web Library": "web-library/index.html",
    "User manual": "manual/index.html",
}
base = "https://mishamashinamm.github.io"
prefix = "/mashina-fm1-fw/"
assert len(links) == len(expected), f"Expected {len(expected)} primary links, got {links!r}"
for label, url in links:
    assert label in expected, f"Unexpected README link: {label}"
    dest = expected[label]
    parsed = urlparse(url)
    assert parsed.scheme == "https" and parsed.netloc == "mishamashinamm.github.io", url
    assert parsed.path.startswith(prefix), url
    rel = parsed.path[len(prefix):].rstrip("/")
    actual = root / (rel + "/index.html" if rel else "index.html")
    assert actual.as_posix() == (root / dest).as_posix(), f"{label}: unexpected URL route {url}"
    assert actual.is_file() and actual.stat().st_size > 500, f"Missing published page: {actual}"
    print(f"OK {label}: {url} -> {actual}")
print("PASS: all README destinations exist in the Pages artifact")
