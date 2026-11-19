#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Fetch pinned, OFL-licensed Oxanium font for the ATMOS UI experiment.
Uses GitHub's immutable blob SHA to verify the downloaded font and license.
Cached files are verified on every build; no font files are committed.
"""
import hashlib
from pathlib import Path
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1] / "assets" / "fonts"
BASE = "https://raw.githubusercontent.com/google/fonts/main/ofl/oxanium/"
FILES = {
    "Oxanium[wght].ttf": "ead485c7bf517197db91ef657614ac37d8007775",
    "OFL-Oxanium.txt": "38913c09407f12006ade3c8391bc3ad48fadc4ee",
}

def git_sha(data):
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\\0" + data).hexdigest()

def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    for filename, sha in FILES.items():
        path = ROOT / filename
        if path.is_file() and git_sha(path.read_bytes()) == sha:
            continue
        remote = "OFL.txt" if filename.startswith("OFL-") else filename
        with urlopen(BASE + remote.replace("[", "%5B").replace("]", "%5D"), timeout=40) as resp:
            data = resp.read()
        if git_sha(data) != sha:
            raise RuntimeError("Oxanium source hash mismatch: " + filename)
        path.write_bytes(data)
        print("Fetched verified Oxanium asset:", filename)

if __name__ == "__main__":
    main()
