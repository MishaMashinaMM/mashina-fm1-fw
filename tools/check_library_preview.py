#!/usr/bin/env python3
"""Verify the standalone MASHINA Web Library preview is fully packaged.

No browser, USB connection or device writes. This catches missing ES modules,
invalid relative import paths, and malformed inline module syntax before a
downloadable preview is published.
"""
from __future__ import annotations
import html.parser
from pathlib import Path
import re
import subprocess
import sys
import tempfile

IMPORT = re.compile(r'\b(?:import|export)\s+(?:[^"\'\n]*?\s+from\s+)?["\'](\.[^"\']+)["\']')
class ModuleScripts(html.parser.HTMLParser):
    def __init__(self):
        super().__init__()
        self.in_module = False
        self.blocks = []
    def handle_starttag(self, tag, attrs):
        if tag == "script":
            attrs = dict(attrs)
            self.in_module = attrs.get("type") == "module"
            if self.in_module:
                self.blocks.append("")
    def handle_endtag(self, tag):
        if tag == "script":
            self.in_module = False
    def handle_data(self, data):
        if self.in_module:
            self.blocks[-1] += data

def check(root: Path) -> None:
    page = root / "index.html"
    assert page.is_file(), "Preview index.html is missing"
    parser = ModuleScripts()
    parser.feed(page.read_text(encoding="utf-8"))
    assert parser.blocks, "Web Library has no inline ES module"
    with tempfile.TemporaryDirectory() as temp:
        for index, script in enumerate(parser.blocks):
            filename = Path(temp) / f"preview-{index}.mjs"
            filename.write_text(script, encoding="utf-8")
            subprocess.run(["node", "--check", str(filename)], check=True)
    pending = [(page, code) for code in parser.blocks]
    visited = set()
    while pending:
        origin, code = pending.pop()
        for name in IMPORT.findall(code):
            path = (origin.parent / name).resolve()
            assert path.is_relative_to(root.resolve()), f"Import escapes preview: {name}"
            assert path.is_file(), f"Missing required preview module: {name} from {origin.name}"
            if path not in visited:
                visited.add(path)
                source = path.read_text(encoding="utf-8")
                subprocess.run(["node", "--check", str(path)], check=True)
                pending.append((path, source))
    assert len(visited) >= 6, "Too few bundled Web Library modules were checked"
    print(f"PASS: standalone Web Library syntax and {len(visited)} ES modules, all relative imports resolvable")

if __name__ == "__main__":
    assert len(sys.argv) == 2, "Usage: check_library_preview.py <built-preview-directory>"
    check(Path(sys.argv[1]))
