/* SPDX-License-Identifier: GPL-3.0-only
 * The Web AudioWorklet must retain experimental LIB32 sectors across a
 * browser reload without accidentally preserving OTA/bootloader sectors.
 * Pure Node.js test: no Emscripten, audio, USB or device writes.
 */
import assert from "node:assert/strict";
import {STORE_AREAS,storeSectors,restoreSectors} from "../web/emu/worklet.js";

const ERASED=0xFF, SIZE=0x100000, SECTOR=4096;
const nor=new Uint8Array(SIZE).fill(ERASED);
const touched=[0x97000,0xA0000,0xBF000,0xC0000,0xC1000,0xC7000,0xDFFFF&~4095,
  0xE5000,0xEA000,0xEB000,0xF1000,0xFC000,0xFF000];
for(let i=0;i<touched.length;i++)nor[touched[i]+47]=i+1;
for(const off of [0xE0000,0xE4000,0xE7000,0xE8000,0xE9000,0xF2000,0xFB000])
  nor[off+47]=0x11; // OTA, reserved or unowned flash must not persist.
const sectors=storeSectors(nor);
assert.deepEqual(Object.keys(sectors).map(Number).sort((a,b)=>a-b),
  touched.slice().sort((a,b)=>a-b));
assert.ok(STORE_AREAS.some(([lo,hi])=>lo<=0xC0000&&hi>=0xC8000),
  "whole reclaimed 32-KiB LIBRARY reservation must persist");
assert.ok(STORE_AREAS.some(([lo,hi])=>lo<=0xEA000&&hi>=0xF2000),
  "legacy emulator JSON source sectors must remain importable offline");
const next=new Uint8Array(SIZE).fill(ERASED);
restoreSectors(next,sectors);
for(let i=0;i<touched.length;i++)
  assert.equal(next[touched[i]+47],i+1,"lost bank "+touched[i].toString(16));
for(const off of [0xE0000,0xE4000,0xE7000,0xE8000,0xE9000,0xF2000,0xFB000])
  assert.equal(next[off+47],ERASED,"restored unowned flash "+off.toString(16));
const outside=new Uint8Array(SECTOR).fill(0);
const malicious={...sectors,
  [0xE8000]:outside,
  [0xF2000]:outside,
  [0xEA001]:outside,
  [0xEA000]:new Uint8Array(SECTOR-1).fill(0)};
const restricted=new Uint8Array(SIZE).fill(ERASED);
restoreSectors(restricted,malicious);
assert.equal(restricted[0xE8000],ERASED);
assert.equal(restricted[0xF2000],ERASED);
assert.equal(restricted[0xEA001],ERASED);
assert.equal(restricted[0xEA000],ERASED);
assert.equal(storeSectors(new Uint8Array(SIZE).fill(ERASED))[0xEA000],undefined,
  "erased LIBRARY sectors must not be serialized");
console.log("PASS: emulator preserves all 8 LIB32 sectors and MPR1 banks; excludes reserved flash and rejects malformed restores");
