/* SPDX-License-Identifier: GPL-3.0-only
 * Exact portable browser-only NOR JSON export/import (no hardware writes).
 * Covers USER, MPR1 project banks, LIB32, settings, autosave, corruption.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {storeSectors,restoreSectors} from "../web/emu/worklet.js";
import {FORMAT,VERSION,encodeEmulatorFlash,decodeEmulatorFlash,restorePersistentEmulatorFlash,readPersistentEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";

const SIZE=0x100000,SECTOR=4096;
const nor=new Uint8Array(SIZE).fill(255);
const owned=[0x97000,0xA0000,0xBF000,0xE5000,0xEA000,0xF1000,0xFC000,0xFF000];
for(let i=0;i<owned.length;i++){
  nor[owned[i]+7]=i+1;
  nor[owned[i]+31]=i+120;
}
nor[0xE8000+7]=54; // reserved sector must NOT export.
const snapshot=storeSectors(nor);
const encoded=encodeEmulatorFlash(snapshot);
assert.equal(encoded.format,FORMAT);
assert.equal(encoded.version,VERSION);
assert.deepEqual(encoded.sectors.map(x=>x.offset),owned);
const json=JSON.stringify(encoded);
const restored=decodeEmulatorFlash(json);
const fresh=new Uint8Array(SIZE).fill(255);
restoreSectors(fresh,restored);
assert.deepEqual(storeSectors(fresh),snapshot,"byte-exact complete NOR roundtrip");
assert.equal(fresh[0xE8000+7],255,"reserved sector restored");
assert.deepEqual(decodeEmulatorFlash(encodeEmulatorFlash({})),{},"empty banks work");
assert.throws(()=>decodeEmulatorFlash('{"format":"other"}'),/compatible/);
const tampered=structuredClone(encoded);
tampered.sectors[0].bytes="A"+tampered.sectors[0].bytes.slice(1);
assert.throws(()=>decodeEmulatorFlash(tampered),/checksum|encoding/);
const extra=structuredClone(encoded);
extra.sectors.push({...extra.sectors[0]});
assert.throws(()=>decodeEmulatorFlash(extra),/duplicate/);
const reserved=structuredClone(encoded);
reserved.sectors[0].offset=0xE8000;
assert.throws(()=>decodeEmulatorFlash(reserved),/Invalid/);
const unaligned=structuredClone(encoded);
unaligned.sectors[0].offset++;
assert.throws(()=>decodeEmulatorFlash(unaligned),/Invalid/);
const truncated=structuredClone(encoded);
truncated.sectors[0].bytes=truncated.sectors[0].bytes.slice(0,-4);
assert.throws(()=>decodeEmulatorFlash(truncated),/encoding|size/);
const invalidVersion=structuredClone(encoded);
invalidVersion.version++;
assert.throws(()=>decodeEmulatorFlash(invalidVersion),/compatible/);
console.log("PASS: emulator JSON backup preserves exact USER/MPR1/LIB32/settings/autosave bytes; rejects corrupt, unowned and incompatible files");

// Permanent restore is an explicit, separate operation: a validated JSON
// replaces only the emulator's existing localStorage key.
const KEY="felucca-emu-flash";
class Storage {
  constructor(){this.data=new Map();}
  getItem(k){return this.data.has(k)?this.data.get(k):null;}
  setItem(k,v){this.data.set(k,String(v));}
  removeItem(k){this.data.delete(k);}
}
const storage=new Storage();
const backupA=storeSectors(nor);
const backupB=storeSectors(fresh);
const backupBEdited={...backupB,[0xA0000]:backupB[0xA0000].slice()};
backupBEdited[0xA0000][31]=62; // second session differs in stored project data
storage.setItem(KEY,JSON.stringify({original:"untouched"}));
const initial=storage.getItem(KEY);
assert.throws(()=>restorePersistentEmulatorFlash(storage,KEY,
  {[0xE8000]:new Uint8Array(SECTOR)}),/Invalid/);
assert.equal(storage.getItem(KEY),initial,"invalid permanent restore mutated existing saved data");
const n=restorePersistentEmulatorFlash(storage,KEY,decodeEmulatorFlash(JSON.stringify(encodeEmulatorFlash(backupA))));
assert.equal(n,owned.length);
assert.deepEqual(readPersistentEmulatorFlash(storage.getItem(KEY)),backupA,"saved backup A differs");
restorePersistentEmulatorFlash(storage,KEY,backupBEdited);
assert.deepEqual(readPersistentEmulatorFlash(storage.getItem(KEY)),backupBEdited,"saved backup B differs");
restorePersistentEmulatorFlash(storage,KEY,backupA);
assert.deepEqual(readPersistentEmulatorFlash(storage.getItem(KEY)),backupA,"backup A cannot replace backup B");
const beforeFailure=storage.getItem(KEY);
const failing={
  getItem:k=>storage.getItem(k),
  setItem:()=>{throw Error("quota exceeded");},
  removeItem:k=>storage.removeItem(k)
};
assert.throws(()=>restorePersistentEmulatorFlash(failing,KEY,backupBEdited),/Permanent restore failed/);
assert.equal(storage.getItem(KEY),beforeFailure,"failed write corrupted prior restore");
console.log("PASS: permanent restore A -> B -> A replaces only saved emulator NOR, validates first and preserves state on quota failure");



// Full firmware integration: normal emulator boot -> project SAVE -> JSON
// snapshot -> fresh WASM boot, same stored section and preset identities.
const wasm=readFileSync("build/emu/felucca.wasm");
async function boot(sectors){
  const {instance}=await WebAssembly.instantiate(wasm,{});
  const ex=instance.exports;
  ex._initialize();
  ex.web_nor_erase();
  if(sectors)restoreSectors(new Uint8Array(ex.memory.buffer,ex.web_nor(),ex.web_nor_size()),sectors);
  ex.web_boot();
  return ex;
}
const first=await boot();
assert.equal(first.web_mpr_test_save(),0,"cannot save boot section");
assert.equal(first.web_mpr_test_used(),1);
const liveStore=storeSectors(new Uint8Array(first.memory.buffer,first.web_nor(),first.web_nor_size()));
const portable=JSON.stringify(encodeEmulatorFlash(liveStore));
const second=await boot(decodeEmulatorFlash(portable));
assert.equal(second.web_mpr_test_used(),1,"project section lost on fresh emulator");
for(let k=0;k<4;k++){
  assert.equal(second.web_mpr_ref(k),33+k,"lost startup factory ref T"+(k+1));
}
console.log("PASS: actual WASM project save -> portable JSON -> independent emulator reboot");
const permanent=new Storage();
restorePersistentEmulatorFlash(permanent,KEY,decodeEmulatorFlash(portable));
const third=await boot(readPersistentEmulatorFlash(permanent.getItem(KEY)));
assert.equal(third.web_mpr_test_used(),1,"permanent browser restore lost saved project section");
for(let k=0;k<4;k++)assert.equal(third.web_mpr_ref(k),33+k,"persistent restored source mismatch on track "+(k+1));
console.log("PASS: actual WASM project save -> portable JSON -> persistent browser storage -> fresh boot");
