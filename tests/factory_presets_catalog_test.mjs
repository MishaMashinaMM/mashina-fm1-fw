/* SPDX-License-Identifier: GPL-3.0-only
 * Source-catalog audit: factory names and engine ordering match the firmware,
 * full melodic params and FM6 voice data remain lossless, kit-dependent entries
 * are explicitly marked rather than silently pretending to be WebMIDI PUTs.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const source="web/factory-presets-from-code.json";
const data=JSON.parse(readFileSync(source,"utf8"));
assert.equal(data.format,"mashina-factory-presets-from-code");
assert.equal(data.version,1);
assert.equal(data.totalFactoryPresets,107);
assert.equal(data.presets.length,107);
assert.equal(data.fullParameterPresets,89);
assert.equal(data.drumKitFactoryReferences,18);
const engineSources=new Map([
  [0,["eng_analog.c","ANALOG_PRESETS"]],
  [10,["eng_drum.c","DRUM_PRESETS"]],
  [5,["eng_formant.c","FORMANT_PRESETS"]],
  [3,["eng_lofi.c","LOFI_PRESETS"]],
  [12,["eng_fm6.c","FM6_PRESETS"]],
  [2,["eng_phase.c","PHASE_PRESETS"]],
  [7,["eng_wheel.c","WHEEL_PRESETS"]],
  [11,["eng_noise.c","NOISE_PRESETS"]],
  [15,["eng_drumlab.c","DRUMLAB_PRESETS"]],
  [16,["eng_industrial.c","INDUSTRIAL_PRESETS"]],
  [17,["eng_brass.c","BRASS_PRESETS"]],
  [18,["eng_atmos.c","ATMOS_PRESETS"]],
  [19,["eng_drumac.c","AC_PRESETS"]],
  [20,["eng_drumind.c","IND_PRESETS"]]
]);
let total=0,full=0,kit=0,fm6=0;
const byEngine=new Map();
for(const [i,p] of data.presets.entries()){
  assert.equal(p.factoryIndex,i+1);
  assert.match(p.name,/^[\x20-\x7e]{1,12}$/);
  assert.ok(engineSources.has(p.engine));
  assert.equal(p.source,"firmware/src/"+engineSources.get(p.engine)[0]);
  const seen=byEngine.get(p.engine)||[];
  assert.equal(p.enginePresetIndex,seen.length);
  seen.push(p.name);byEngine.set(p.engine,seen);
  assert.equal(p.definition.edit?.length??0,p.definition.editSymbol?0:8);
  assert.equal(p.definition.envelope.length,4);
  if(p.params){
    full++;
    assert.equal(p.params.length,127);
    assert.ok(p.params.every(x=>Number.isInteger(x)&&x>=-64&&x<=127));
    assert.deepEqual(p.params.slice(115,123),p.definition.edit);
  }else{
    kit++;
    assert.equal(p.requiresEngineFactoryKit,true);
    assert.ok([10,15,19,20].includes(p.engine));
    assert.equal(p.factoryKitIndex,p.enginePresetIndex);
  }
  if(p.fm6){
    fm6++;assert.equal(p.engine,12);
    assert.equal(p.fm6.length,128);
    assert.ok(p.fm6.every(x=>Number.isInteger(x)&&x>=0&&x<128));
  }
  total++;
}
assert.equal(total,107);assert.equal(full,89);assert.equal(kit,18);assert.equal(fm6,10);
for(const [id,names] of byEngine){
  const [path,table]=engineSources.get(id);
  const text=readFileSync("firmware/src/"+path,"utf8");
  const head="static const preset_t "+table+"[] = {";
  assert.ok(text.includes(head),"missing "+head);
  const segment=text.slice(text.indexOf(head)+head.length);
  const body=segment.slice(0,segment.indexOf("\n};"));
  const namesInCode=[...body.matchAll(/^\s*\{"([^"]+)",/gm)].map(m=>m[1]);
  assert.deepEqual(names,namesInCode,"factory order changed for "+path);
}
const defaults=data.presets.filter(p=>p.initialLibrarySlot!==undefined);
assert.equal(defaults.length,32);
assert.deepEqual(defaults.map(p=>p.initialLibrarySlot),Array.from({length:32},(_,i)=>i+33));
console.log("PASS: 107 factory sounds, 89 full melodic patches, 18 explicitly kit-backed factory definitions, 10 FM6 voices, 32 stable library defaults");
