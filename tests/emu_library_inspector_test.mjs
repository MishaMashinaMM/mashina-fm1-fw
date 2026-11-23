/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only Web Library introspection of actual experimental emulator NOR.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {storeSectors} from "../web/emu/worklet.js";
import {encodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash,resolveFactoryFallbacks} from "../web/emulator-library-inspector.mjs";
import {restoreProjectsBackup} from "../web/projects-library-codec.mjs";
const wasm=readFileSync("build/emu/felucca.wasm");
const {instance}=await WebAssembly.instantiate(wasm,{});
const e=instance.exports;
e._initialize();e.web_nor_erase();e.web_boot();
assert.equal(e.web_mpr_test_save(),0,"save initial project");
const nor=new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size());
const snapshot=encodeEmulatorFlash(storeSectors(nor));
const original=JSON.stringify(snapshot);
const result=inspectEmulatorFlash(original);
assert.equal(result.users.length,32);
assert.equal(result.library.length,32);
assert.equal(result.projects.sections.length,32);
assert.equal(result.library.filter(x=>x.status==="factory").length,32);
const master=JSON.parse(readFileSync("web/factory-presets-from-code.json","utf8"));
const named=resolveFactoryFallbacks(result,master);
const originals=master.presets.filter(x=>x.initialLibrarySlot!==undefined);
for(let slot=33;slot<=64;slot++){
  const expected=originals.find(x=>x.initialLibrarySlot===slot);
  const actual=named.library[slot-33];
  assert.ok(expected,"factory slot "+slot+" missing in original catalog");
  assert.equal(actual.name,expected.name,"wrong factory name at "+slot);
  assert.equal(actual.engine,expected.engine,"wrong factory engine at "+slot);
  assert.equal(actual.status,"factory","factory preset incorrectly marked as an override");
}
assert.equal(result.library[0].name,"Factory fallback","inspection's raw flash result was modified");
console.log("PASS: 32 fallback slots resolved to exact compiled factory names/engines without mutating flash snapshot");

assert.equal(result.users.filter(x=>x.status==="empty").length,32);
const slots=restoreProjectsBackup(result.projects);
assert.notEqual(slots[0],null,"initial saved project section A omitted");
assert.equal(slots.slice(1).filter(Boolean).length,0,"unwritten sections should be empty");
assert.deepEqual(inspectEmulatorFlash(original),result,"non-deterministic inspection");
assert.equal(original,JSON.stringify(snapshot),"snapshot unexpectedly modified");
console.log("PASS: actual WASM MPR1 project saved to NOR is inspected as 8xA–D plus 32 USER and 32 LIBRARY");

// Read-only synthetic committed LIB1 and USER banks with exact firmware headers.
const sectors=storeSectors(nor);
const crc=d=>{
  let c=0xffffffff;
  for(const b of d){c^=b;for(let j=0;j<8;j++)c=(c>>>1)^((c&1)?0xedb88320:0);}
  return (~c)>>>0;
};
const w16=(d,i,n)=>{d[i]=n&255;d[i+1]=(n>>>8)&255};
const w32=(d,i,n)=>{for(let k=0;k<4;k++)d[i+k]=(n>>>(8*k))&255};
const place=(off,d)=>{sectors[off]=d;};
const sound=new Uint8Array(304);sound[0]=0xa5;sound[1]=4;sound[2]=0;sound[3]=127;
sound.set([...Buffer.from("SOUND-A")],4);sound.fill(64,16,143);sound[159]=0x47;
const lib=new Uint8Array(4096).fill(255);
const body=new Uint8Array(2432);body.set(sound,0);
w32(lib,0,0x3142494c);w32(lib,4,1);w32(lib,8,0);w32(lib,12,8);
w32(lib,16,2432);w32(lib,20,crc(body));lib.set(body,256);
place(0xea000,lib);
const user=new Uint8Array(4096).fill(255);
const bank=new Uint8Array(3080);w32(bank,0,0x31425055);w16(bank,4,192);w16(bank,6,16);
bank.set(sound.slice(0,192),8);
w32(user,0,0x554c4546);w16(user,4,5);w16(user,6,0);
w32(user,8,7);w32(user,12,3080);w32(user,16,crc(bank));
w32(user,20,0xffffffff);w32(user,24,0xffffffff);
w32(user,28,crc(user.slice(0,28)));
user.set(bank,256);place(0xdc000,user);
let parsed=inspectEmulatorFlash(JSON.stringify(encodeEmulatorFlash(sectors)));
assert.equal(parsed.users[0].status,"saved");
assert.equal(parsed.users[0].name,"SOUND-A");
assert.equal(parsed.library[0].status,"overridden");
assert.equal(parsed.library[0].name,"SOUND-A");
const namedOverride=resolveFactoryFallbacks(parsed,master);
assert.equal(namedOverride.library[0].name,"SOUND-A","custom override must win over factory THALES");
assert.equal(namedOverride.library[0].engine,0);
assert.equal(namedOverride.library[0].status,"overridden");
assert.equal(namedOverride.library[1].name,originals.find(x=>x.initialLibrarySlot===34).name);
const altered=structuredClone(master);
altered.presets[0].name="CHANGED";
assert.throws(()=>resolveFactoryFallbacks(parsed,altered),/protected 107-preset source revision/);
console.log("PASS: custom overrides win and altered 107-source catalogs are rejected");

assert.equal(parsed.library[1].status,"factory");
console.log("PASS: committed USER and LIBRARY NOR records decoded with proper names and fallback slots");
const corruptLib=lib.slice();corruptLib[260]^=1;
const broken={...sectors,[0xea000]:corruptLib};
assert.throws(()=>inspectEmulatorFlash(encodeEmulatorFlash(broken)),/LIBRARY bank 0/);
const brokenUser={...sectors,[0xdc000]:new Uint8Array(4096)};
assert.throws(()=>inspectEmulatorFlash(encodeEmulatorFlash(brokenUser)),/USER bank 0/);
console.log("PASS: damaged committed banks refused, not treated as empty sound slots");
