/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {analyzeFlashFingerprints,ERASED_4K_CRC32} from "../web/flash-fingerprint-analysis.mjs";
assert.equal(ERASED_4K_CRC32,"F154670A");
const sectors=Array.from({length:40},(_,i)=>({
  index:i,address:i<32?0xA0000+i*4096:0xEA000+(i-32)*4096,
  readable:true,crc32:i<32?"ABCD0000":("0000000"+((i-32)%4+1)).slice(-8)
}));
const diag={format:"mashina-flash-diagnostic",version:2,
  fingerprints:{format:"mashina-flash-sector-fingerprints",version:1,
    sectorBytes:4096,complete:true,sectors}};
const r=analyzeFlashFingerprints(diag);
assert.equal(r.libraryRepeated16KiBPattern,true);
assert.equal(r.libraryBaseAddress,"0xEA000");
assert.equal(r.decision,"PHYSICAL_WRITES_BLOCKED");
assert.equal(r.libraryDuplicateChecksums.length,4);
assert.equal(r.libraryErasedSectorAddresses.length,0);
sectors[39].crc32=ERASED_4K_CRC32;
const changed=analyzeFlashFingerprints(diag);
assert.equal(changed.libraryRepeated16KiBPattern,false);
assert.deepEqual(changed.libraryErasedSectorAddresses,["0xF1000"]);
assert.equal(changed.decision,"PHYSICAL_WRITES_BLOCKED");
assert.throws(()=>analyzeFlashFingerprints({...diag,version:1}),/version-2/);
sectors[2].readable=false;
assert.throws(()=>analyzeFlashFingerprints(diag),/unreadable/);
const newLayout=structuredClone(diag);
newLayout.fingerprints.sectors[2].readable=true;
newLayout.fingerprints.sectors.slice(32).forEach((sec,i)=>{sec.address=0xc0000+i*4096;sec.readable=true;});
const moved=analyzeFlashFingerprints(newLayout);
assert.equal(moved.libraryBaseAddress,"0xC0000");
assert.deepEqual(moved.libraryErasedSectorAddresses,["0xC7000"]);
assert.equal(moved.decision,"PHYSICAL_WRITES_BLOCKED");
console.log("PASS: physical flash repeated CRC analysis is read-only, exact-address validated, never grants write permission");
