/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {analyzeFlashFingerprints,ERASED_4K_CRC32,EMPTY_FELU_RECORD_CRC32} from "../web/flash-fingerprint-analysis.mjs";
import {crc32} from "../web/emulator-snapshot-editor.mjs";
assert.equal(ERASED_4K_CRC32,"F154670A");
assert.equal(EMPTY_FELU_RECORD_CRC32,"127B0F88");
/* A real empty FELU record has two nested commit CRCs: one for its
 * zero-byte body and one for the record header. Those CRCs cancel out
 * object-ID / slot / sequence differences in a CRC of the whole sector.
 * The 127B0F88 repetition therefore does NOT establish a hardware alias.
 * This constructs multiple DIFFERENT byte sequences and checks their CRC. */
const emptyLegacySector=(type,copy,seq)=>{
  const bytes=new Uint8Array(4096).fill(0xff);
  const v=new DataView(bytes.buffer);
  v.setUint32(0,0x554c4546,true); // FELU
  v.setUint16(4,type,true);
  v.setUint16(6,copy,true);
  v.setUint32(8,seq,true);
  v.setUint32(12,0,true); // empty body
  v.setUint32(16,0,true); // CRC32 of zero-byte body
  v.setUint32(20,0xffffffff,true);
  v.setUint32(24,0xffffffff,true);
  v.setUint32(28,crc32(bytes.subarray(0,28)),true);
  return bytes;
};
const syntheticEmpty=[emptyLegacySector(12,0,1),emptyLegacySector(31,1,998),
                      emptyLegacySector(35,0,27)];
assert.notDeepEqual(syntheticEmpty[0].slice(0,32),syntheticEmpty[1].slice(0,32));
for(const bytes of syntheticEmpty)
  assert.equal(crc32(bytes).toString(16).toUpperCase().padStart(8,"0"),EMPTY_FELU_RECORD_CRC32);

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
const expanded=structuredClone(newLayout);
expanded.fingerprints.version=2;
for(const address of [0xDC000,0xDD000,0xDE000,0xDF000,0x9F000,0xFE000])
  expanded.fingerprints.sectors.push({
    index:expanded.fingerprints.sectors.length,address,readable:true,
    crc32:address===0x9F000?ERASED_4K_CRC32:"1234ABCD"
  });
const newReview=analyzeFlashFingerprints(expanded);
assert.equal(newReview.librarySectorCount,8);
assert.equal(newReview.userCompanionSectorCount,6);
assert.deepEqual(newReview.userCompanionErasedSectorAddresses,["0x9F000"]);
assert.equal(newReview.decision,"PHYSICAL_WRITES_BLOCKED");
const wrongExtra=structuredClone(expanded);
wrongExtra.fingerprints.sectors[45].address=0xEA000;
assert.throws(()=>analyzeFlashFingerprints(wrongExtra),/Invalid or unreadable/);
const allState=structuredClone(expanded);
allState.fingerprints.version=3;
for(const address of [0xFC000,0xFD000,0xD8000,0xD9000,0xE5000,0xE6000,0xE7000])
  allState.fingerprints.sectors.push({
    index:allState.fingerprints.sectors.length,address,readable:true,
    crc32:address===0xE7000?ERASED_4K_CRC32:"1234ABCD"
  });
const report53=analyzeFlashFingerprints(allState);
assert.equal(report53.librarySectorCount,8);
assert.equal(report53.userCompanionSectorCount,6);
assert.equal(report53.auxiliarySectorCount,7);
assert.deepEqual(report53.auxiliaryErasedSectorAddresses,["0xE7000"]);
assert.equal(report53.decision,"PHYSICAL_WRITES_BLOCKED");
const wrongAux=structuredClone(allState);
wrongAux.fingerprints.sectors[52].address=0xEA000;
assert.throws(()=>analyzeFlashFingerprints(wrongAux),/Invalid or unreadable/);
allState.fingerprints.sectors[38].crc32=EMPTY_FELU_RECORD_CRC32;
const emptyReport=analyzeFlashFingerprints(allState);
assert(emptyReport.emptyFeluPatternSectorAddresses.includes("0xC6000"));
assert(emptyReport.messages.some(x=>x.includes("does NOT indicate flash aliasing")));
console.log("PASS: physical flash repeated CRC analysis is read-only, exact-address validated, never grants write permission");
