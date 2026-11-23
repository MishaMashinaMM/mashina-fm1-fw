#!/usr/bin/env python3
"""Host-only *lossless* compact-bank experiment for the 32 replaceable slots.

Never writes firmware, WebMIDI, or hardware flash. Reads real factory JSON fixtures
and measures representative and worst-case payloads against two 4 KiB A/B sectors.
Uses deterministic deflate (Python zlib) solely to evaluate compression; it is NOT
an on-device decoding implementation, and no compression-based capacity guarantee.
"""
import json
from pathlib import Path
import random
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
N = 32
PARAMS = 127
SIDECAR = 112
PAYLOAD = 4096-256
rng = random.Random(0x32F1)

def full_record(name, engine, values, aux=b""):
    if len(values) != PARAMS or any(not isinstance(v,int) or v < -32768 or v > 32767 for v in values):
        raise ValueError("requires exactly 127 full signed parameter values")
    if len(aux) not in (0,SIDECAR):
        raise ValueError("auxiliary data must be absent or exactly 112 bytes")
    nm = name.encode("ascii",errors="strict")[:12].ljust(12,b"\x00")
    # Full fidelity to available exported parameter *values*, not an FM1 wire record:
    return struct.pack("<BB12s",engine,len(aux),nm)+struct.pack("<127h",*values)+aux

def measure(label, records):
    raw = b"".join(records)
    packed = zlib.compress(raw,9)
    fits_single = len(packed)<=PAYLOAD
    print(f"{label:29s} raw={len(raw):6,d} B  deflate={len(packed):6,d} B  "
          f"A/B-4K={'FIT' if fits_single else 'NO'}")
    return len(packed)

f1=json.loads((ROOT/"tests/fixtures/rest_factory_bank.json").read_text())
f2=json.loads((ROOT/"tests/fixtures/voice_lofi_factory_bank.json").read_text())
patches=f1["patches"]+f2["patches"]
assert all(len(p["params"])==PARAMS for p in patches)
assert len(patches)>=N, "fixture lost representative sounds"
# Spread across engines rather than selecting 32 copies of a similar patch.
groups={}
for p in patches: groups.setdefault(p["engine"],[]).append(p)
picked=[]
while len(picked)<N and any(groups.values()):
    for g in groups.values():
        if g and len(picked)<N: picked.append(g.pop(0))
assert len(picked)==N

fixture=[full_record(p["name"],p["engine"],p["params"]) for p in picked]
fixture_extra=[full_record(p["name"],p["engine"],p["params"],bytes(SIDECAR)) for p in picked]
random_values=lambda : [rng.randint(-64,191) for _ in range(PARAMS)]
random_bank=[full_record(f"PATCH {i:02d}",rng.randint(0,20),random_values(),bytes(rng.getrandbits(8) for _ in range(SIDECAR))) for i in range(N)]
print(f"Two permitted free 4 KiB sectors, A/B writable payload {PAYLOAD} B per atomic copy.")
print("These numbers represent a standalone compression experiment, not production record format.")
a=measure("real melodic factory mix",fixture)
b=measure("same sounds + blank sidecars",fixture_extra)
c=measure("random full data + sidecars",random_bank)
assert c>PAYLOAD, "unexpected: adversarial set now fits; review scenario"
assert len(b"".join(random_bank))==N*(14+PARAMS*2+SIDECAR)
print("RESULT: representative packs may compress, but arbitrary full kits and patches cannot be guaranteed to fit.")
print("SAFE FLASH-WRITE GATE REMAINS CLOSED.")
