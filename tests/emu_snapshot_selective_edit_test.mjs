/* SPDX-License-Identifier: GPL-3.0-only
 * Firmware-real browser NOR selective LIBRARY changes:
 * pure exported snapshot, A/B commit, exact preservation and corruption lock.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {storeSectors} from "../web/emu/worklet.js";
import {encodeEmulatorFlash,decodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash} from "../web/emulator-library-inspector.mjs";
import {editEmulatorLibrarySnapshot,factorySourceRecords,snapshotSourceRecords} from "../web/emulator-snapshot-editor.mjs";
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
