/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {encodeRecord} from "../web/presets-library-codec.mjs";
import {encodeEmulatorFlash,decodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {storeSectors} from "../web/emu/worklet.js";
import {inspectEmulatorFlash,resolveFactoryFallbacks} from "../web/emulator-library-inspector.mjs";
import {editEmulatorLibrarySnapshot,editEmulatorProjectReferences} from "../web/emulator-snapshot-editor.mjs";
import {defaultFactorySelection} from "../web/factory-catalog-selection.mjs";
const wasm=readFileSync("build/emu/felucca.wasm");
const {instance}=await WebAssembly.instantiate(wasm,{});
const e=instance.exports;
e.web_nor_erase();e.web_boot();
assert.equal(e.web_lib_slots(),96);
const master=JSON.parse(readFileSync("web/factory-presets-from-code.json","utf8"));
const defaults=defaultFactorySelection(master,96);
assert.equal(new Set(defaults).size,96);
assert.deepEqual(defaults.slice(0,89),master.presets.flatMap((p,i)=>p.requiresEngineFactoryKit?[]:[i]));
assert.deepEqual(defaults.slice(89).map(i=>master.presets[i].name),
  ["BUTTLER-1","BUTTLER-2","ZIZEK-1","ZIZEK-2","KRIPKE-1","KRIPKE-3","BADIOU-1"]);
for(let i=0;i<96;i++){
  const p=master.presets[defaults[i]];
  assert.equal(e.web_lib_factory_engine(i),p.engine,"engine mapping slot "+(i+33));
  assert.equal(e.web_lib_factory_index(i),p.enginePresetIndex,"preset mapping slot "+(i+33));
}
assert.equal(e.web_mpr_test_save(),0);
const nor=new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size());
const initial=encodeEmulatorFlash(storeSectors(nor),96);
const resolved=resolveFactoryFallbacks(inspectEmulatorFlash(initial),master);
assert.equal(resolved.library.length,96);
assert.deepEqual(resolved.library.map(p=>p.name),defaults.map(i=>master.presets[i].name));
const recordBytes=Array.from(encodeRecord({engine:0,name:"LIB128 TEST",params:Array(127).fill(0)}));
const edited=editEmulatorLibrarySnapshot(initial,[{slot:128,recordBytes}]);
assert.deepEqual(edited.inspection.library[95].recordBytes,recordBytes);
assert.deepEqual(edited.inspection.users,resolved.users);
assert.deepEqual(edited.inspection.projects,resolved.projects);
const project=editEmulatorProjectReferences(edited.snapshot,[{project:1,section:"A",track:1,reference:127}]);
const {instance:boot}=await WebAssembly.instantiate(wasm,{});
const b=boot.exports;b.web_nor_erase();
const mem=new Uint8Array(b.memory.buffer,b.web_nor(),b.web_nor_size());
for(const [off,data] of Object.entries(decodeEmulatorFlash(project.snapshot,96)))mem.set(data,Number(off));
b.web_boot();
assert.equal(b.web_mpr_test_load(),0);
assert.equal(b.web_mpr_ref(0),128);
assert.equal(b.web_mpr_assign(0,127),0);
assert.equal(b.web_mpr_assign(0,128),1);
assert.throws(()=>decodeEmulatorFlash(project.snapshot,32),/requires LIB96/);
console.log("PASS: 96 compiled factory mappings, final-slot NOR edit and project recall after independent WASM boot");

// Exercise the actual browser MIDI enqueue path (not the direct assign helper).
for(let i=0;i<180;i++)b.web_render(1024);
const pcHtml=readFileSync("build/emu/index.html","utf8");
assert.match(pcHtml,/id="midi-test-pc-channel"[^>]*min="1" max="16"/);
assert.match(pcHtml,/id="midi-test-pc"[^>]*min="0" max="127"/);
assert.match(pcHtml,/midiTestSend\(0xC0 \| channel, program, 0\)/);
for(const program of [32,63,64,95,127]){
  b.web_midi(0xC0,program,0);
  for(let i=0;i<12;i++)b.web_render(1024);
  assert.equal(b.web_mpr_ref(0),program+1,"CH1 PC "+program+" through MIDI queue");
}
b.web_midi(0xCF,127,0);
for(let i=0;i<12;i++)b.web_render(1024);
assert.equal(b.web_mpr_ref(0),128,"CH16 must not change a track preset");
console.log("PASS: browser Program Change input and queued MIDI PC through 127");
