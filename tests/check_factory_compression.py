#!/usr/bin/env python3
"""Verify every compressed byte against the canonical factory snapshot."""
import ctypes
import json
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
records = json.loads((root / "factory/final-performance.json").read_text())["libraryPresets"]
with tempfile.TemporaryDirectory() as directory:
    tmp = pathlib.Path(directory)
    (tmp / "check.c").write_text('#include <stdint.h>\n#include <string.h>\n#include "mashina_factory_records.h"\nint decode(unsigned slot, uint8_t *out){return mashina_factory_decode(slot,out);}\n')
    subprocess.run(["cc","-shared","-fPIC","-O2","-I",str(root / "firmware/src"),str(tmp / "check.c"),"-o",str(tmp / "check.so")],check=True)
    lib = ctypes.CDLL(str(tmp / "check.so"))
    lib.decode.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_uint8)]
    lib.decode.restype = ctypes.c_int
    for i, record in enumerate(records):
        out = (ctypes.c_uint8 * 304)()
        assert lib.decode(i,out) == 0 and list(out) == record, f"factory record {i+1}"
    assert lib.decode(96,out) != 0
print("PASS: all 96 factory records match the canonical snapshot byte-for-byte")
