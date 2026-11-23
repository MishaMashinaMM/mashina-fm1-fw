/* SPDX-License-Identifier: GPL-3.0-only
 * Firmware-real browser NOR selective LIBRARY changes:
 * pure exported snapshot, A/B commit, exact preservation and corruption lock.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {storeSectors} from "../web/emu/worklet.js";
import {encodeEmulatorFlash,decodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash} from "../web/emulator-library-inspector.mjs";
import {editEmulatorLibrarySnapshot,factorySourceRecords,snapshotSourceRecords,userSourceRecords,crc32} from "../web/emulator-snapshot-editor.mjs";
const master=JSON.parse(readFileSync("web/factory-presets-from-code.json","utf8"));
const factory=factorySourceRecords(master);
assert.equal(factory.length,107);
assert.equal(factory.filter(x=>x.available).length,89);
const thales=factory.find(x=>x.name==="THALES");
const plato=factory.find(x=>x.name==="PLATO");
assert(thales?.recordBytes.length===304);
assert(plato?.recordBytes.length===304);
assert(factory.find(x=>x.name==="BUTTLER-1").available===false,"kit requires exact sidecar");

const wasm=readFileSync("build/emu/felucca.wasm");
const {instance}=await WebAssembly.instantiate(wasm,{});
const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();
assert.equal(e.web_mpr_test_save(),0);
const nor=new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size());
const initial=encodeEmulatorFlash(storeSectors(nor));
const originalJSON=JSON.stringify(initial);
const first=editEmulatorLibrarySnapshot(initial,[
  {slot:33,recordBytes:thales.recordBytes},
  {slot:34,recordBytes:plato.recordBytes},
  {slot:41,recordBytes:thales.recordBytes},
]);
assert.equal(first.inspection.library[0].name,"THALES");
assert.equal(first.inspection.library[1].name,"PLATO");
assert.equal(first.inspection.library[8].name,"THALES");
assert.equal(first.inspection.library.filter(x=>x.status==="overridden").length,3);
assert.deepEqual(inspectEmulatorFlash(initial).library.filter(x=>x.status==="overridden"),[]);
assert.equal(JSON.stringify(initial),originalJSON,"source snapshot edited in place");
const base=decodeEmulatorFlash(initial),changed=decodeEmulatorFlash(first.snapshot);
for(const [off,bytes] of Object.entries(base)){
  if(Number(off)>=0xea000&&Number(off)<0xf2000)continue;
  assert.deepEqual(changed[off],bytes,"unrelated NOR sector mutated at "+off);
}
const references=inspectEmulatorFlash(initial).projects;
assert.deepEqual(first.inspection.projects,references,"project references or opaque settings changed");
const second=editEmulatorLibrarySnapshot(first.snapshot,[{slot:33,recordBytes:plato.recordBytes}]);
assert.equal(second.inspection.library[0].name,"PLATO");
assert.equal(second.inspection.library[1].name,"PLATO");
assert.equal(second.inspection.library[8].name,"THALES");
const erased=editEmulatorLibrarySnapshot(second.snapshot,[{slot:33,recordBytes:null}]);
assert.equal(erased.inspection.library[0].status,"factory");
assert.equal(erased.inspection.library[1].name,"PLATO");
const copies=decodeEmulatorFlash(erased.snapshot);
assert(copies[0xea000]&&copies[0xeb000],"A/B copies not present after multiple saves");
console.log("PASS: selective exported NOR JSON edits 3 sounds, preserves MPR1/USER/settings, chooses A/B and clears one fallback");
const list=snapshotSourceRecords(first.snapshot);
assert.equal(list.length,3);
assert.equal(list[0].recordBytes.length,304);
const cross=editEmulatorLibrarySnapshot(initial,[{slot:60,recordBytes:list[0].recordBytes}]);
assert.equal(cross.inspection.library[27].name,"THALES");
console.log("PASS: exact 304-byte saved LIBRARY record copied from one snapshot into another");
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[]),/1..32/);
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[{slot:65,recordBytes:thales.recordBytes}]),/Invalid/);
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[{slot:33,recordBytes:thales.recordBytes},{slot:33,recordBytes:null}]),/repeated/);
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[{slot:33,recordBytes:thales.recordBytes.slice(0,192)}]),/304/);
let forged=structuredClone(initial);
forged.sectors[0].bytes=forged.sectors[0].bytes.replace(/A/,"B");
assert.throws(()=>editEmulatorLibrarySnapshot(forged,[{slot:33,recordBytes:thales.recordBytes}]),/checksum|Invalid|compatible/);
const broken=decodeEmulatorFlash(first.snapshot);
broken[0xea000]=new Uint8Array(4096);broken[0xeb000]=new Uint8Array(4096);
assert.throws(()=>editEmulatorLibrarySnapshot(encodeEmulatorFlash(broken),[{slot:33,recordBytes:thales.recordBytes}]),/LIBRARY bank 0/);
console.log("PASS: partial, duplicate, invalid/corrupt sources are rejected without producing modified backups");


const stSector=(type,payload,copy=0,sequence=1)=>{
  const out=new Uint8Array(4096).fill(255);
  const w16=(p,v)=>{out[p]=v&255;out[p+1]=(v>>8)&255;};
  const w32=(p,v)=>{for(let k=0;k<4;k++)out[p+k]=(v>>>(k*8))&255;};
  w32(0,0x554c4546);w16(4,type);w16(6,copy);w32(8,sequence);
  w32(12,payload.length);w32(16,crc32(payload));
  w32(20,0xffffffff);w32(24,0xffffffff);
  w32(28,crc32(out.slice(0,28)));
  out.set(payload,256);return out;
};
const baseSectors=decodeEmulatorFlash(initial);
const simple=thales.recordBytes;
const fm=plato.recordBytes;
const drum=Uint8Array.from(simple);
drum[2]=10;drum.set([...Buffer.from("DRUM-ONE")],4);
drum.fill(0,192);drum[303]=0xd3;
const bank=new Uint8Array(3080);
const writeU16=(dest,p,v)=>{dest[p]=v&255;dest[p+1]=(v>>8)&255;};
const writeU32=(dest,p,v)=>{for(let k=0;k<4;k++)dest[p+k]=(v>>>(8*k))&255;};
writeU32(bank,0,0x31425055);writeU16(bank,4,192);writeU16(bank,6,16);
bank.set(simple.slice(0,192),8);
bank.set(fm.slice(0,192),8+192);
bank.set(drum.slice(0,192),8+384);
const upf=new Uint8Array(3728);
writeU32(upf,0,0x36465055);writeU16(upf,4,1);writeU16(upf,6,32);
writeU32(upf,8,(1<<1)|(1<<2));
const tag=raw=>{
  let v=2166136261;
  for(let i=0;i<raw.length;i++){
    if(i>=4&&i<16)continue;
    v=Math.imul(v^raw[i],16777619)>>>0;
  }
  return v;
};
writeU32(upf,16+1*116,tag(fm.slice(0,192)));
upf.set(fm.slice(192),16+1*116+4);
writeU32(upf,16+2*116,tag(drum.slice(0,192)));
upf.set(drum.slice(192),16+2*116+4);
const synthetic={...baseSectors,[0xdc000]:stSector(5,bank),[0x9f000]:stSector(8,upf)};
let copied=userSourceRecords(encodeEmulatorFlash(synthetic));
assert.equal(copied.length,3);
assert(copied.every(x=>x.available),"all 3 USER sounds have complete companions");
assert.deepEqual(copied[0].recordBytes,simple);
assert.deepEqual(copied[1].recordBytes,fm);
assert.deepEqual(copied[2].recordBytes,Array.from(drum));
const fullCopy=editEmulatorLibrarySnapshot(initial,
  copied.map((x,i)=>({slot:33+i,recordBytes:x.recordBytes})));
assert.equal(fullCopy.inspection.library[1].engine,12);
assert.equal(fullCopy.inspection.library[2].engine,10);
const oldUpf=structuredClone(upf);oldUpf[16+2*116+4]^=1;
const mismatch=userSourceRecords(encodeEmulatorFlash(
  {...synthetic,[0x9f000]:stSector(8,Uint8Array.from(oldUpf))}));
assert.equal(mismatch[2].available,true,
  "matching tag should not be changed by companion content corruption without CRC metadata");
const badTag=upf.slice();badTag[16+2*116]^=1;
const stale=userSourceRecords(encodeEmulatorFlash(
  {...synthetic,[0x9f000]:stSector(8,badTag)}));
assert.equal(stale[2].available,false,"stale drum companion must not be presented as transferable");
const missing=userSourceRecords(encodeEmulatorFlash({...synthetic,[0x9f000]:new Uint8Array(4096).fill(255)}));
assert.equal(missing[0].available,true);
assert.equal(missing[1].available,false);
assert.equal(missing[2].available,false);
console.log("PASS: USER source export preserves exact melodic/FM6/drum companions and blocks absent or stale tags");
