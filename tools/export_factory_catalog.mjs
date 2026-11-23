#!/usr/bin/env node
/* SPDX-License-Identifier: GPL-3.0-only
 * Regenerate the entire MASHINA factory preset catalog from source.
 *
 *   node tools/export_factory_catalog.mjs --check
 *   node tools/export_factory_catalog.mjs --write
 *
 * There are more factory sounds than the device's 32+32 writable slots.
 * This is an offline, source-traceable catalog, not a pretend WebMIDI backup.
 * Fully synthesized drum-kit companions still require a separate kit encoder.
 */
import fs from "node:fs";
import assert from "node:assert/strict";

const ENGINE_SPEC=[
  ["ANALOG",0,"eng_analog.c","ANALOG_PRESETS","analog_factory_import.h","ANALOG_IMPORT_COMMON"],
  ["DRUM-X",10,"eng_drum.c","DRUM_PRESETS"],
  ["VOICE",5,"eng_formant.c","FORMANT_PRESETS","voice_lofi_factory_import.h","FORMANT_IMPORT_COMMON"],
  ["LOFI",3,"eng_lofi.c","LOFI_PRESETS","voice_lofi_factory_import.h","LOFI_IMPORT_COMMON"],
  ["FM6",12,"eng_fm6.c","FM6_PRESETS","fm6_factory_import.h","FM6_IMPORT_COMMON"],
  ["PHASE",2,"eng_phase.c","PHASE_PRESETS","phase_factory_import.h","PHASE_IMPORT_COMMON"],
  ["WHEEL",7,"eng_wheel.c","WHEEL_PRESETS","wheel_factory_import.h","WHEEL_IMPORT_COMMON"],
  ["NOISE",11,"eng_noise.c","NOISE_PRESETS","noise_factory_import.h","NOISE_IMPORT_COMMON"],
  ["DRUM-LAB",15,"eng_drumlab.c","DRUMLAB_PRESETS"],
  ["INDUSTRI",16,"eng_industrial.c","INDUSTRIAL_PRESETS","rest_factory_import.h","INDUSTRIAL_IMPORT_COMMON","INDUSTRIAL_EXTRA_PRESETS"],
  ["BRASS",17,"eng_brass.c","BRASS_PRESETS","rest_factory_import.h","BRASS_IMPORT_COMMON","BRASS_EXTRA_PRESETS"],
  ["ATMOS",18,"eng_atmos.c","ATMOS_PRESETS","rest_factory_import.h","ATMOS_IMPORT_COMMON","ATMOS_EXTRA_PRESETS"],
  ["DRUM-AC",19,"eng_drumac.c","AC_PRESETS"],
  ["DRUM-IND",20,"eng_drumind.c","IND_PRESETS"]
];
const contents=new Map();
function source(file){
  if(!contents.has(file))contents.set(file,fs.readFileSync("firmware/src/"+file,"utf8"));
  return contents.get(file);
}
function initializer(text,mark) {
  const i=text.indexOf(mark);
  assert.ok(i>=0,"missing source array: "+mark);
  const p=text.indexOf("{",i);
  let depth=0,quoted=false,escaped=false;
  for(let j=p;j<text.length;j++){
    const c=text[j];
    if(quoted){
      if(escaped)escaped=false;
      else if(c==="\\")escaped=true;
      else if(c==='"')quoted=false;
      continue;
    }
    if(c==='"'){quoted=true;continue;}
    if(c==="{")depth++;
    else if(c==="}" && --depth===0)return text.slice(p+1,j);
  }
  throw Error("unclosed initializer: "+mark);
}
const list=s=>s.split(",").map(x=>Number(x.trim()));
function rows(text,mark){
  return [...initializer(text,mark).matchAll(/\{([^{}]+)\}/g)].map(m=>list(m[1]));
}
const expression=/^\s*\{"([^"]+)",\s*(\{[^{}]*\}|[A-Z_][A-Z0-9_]*)\s*,\s*\{([^{}]+)\}\s*,\s*(-?\d+)\s*,\s*TAG\(([^)]+)\)(?:,\s*FX\(([^)]*)\))?(?:,\s*PAT\(([^)]*)\))?\s*\}\s*,?/gm;
const result=[];
for(const spec of ENGINE_SPEC){
  const [engineName,engine,file,table,importFile,commonName,extraName]=spec;
  const src=source(file);
  const declared=[...initializer(src,"static const preset_t "+table+"[]").matchAll(expression)];
  assert.ok(declared.length>0,"no "+table+" presets");
  const imported=importFile?rows(source(importFile),"static const int8_t "+commonName+"["):[];
  const extras=extraName?rows(src,"static const uint8_t "+extraName+"["):[];
  if(importFile)assert.equal(imported.length,declared.length,"wrong common patch count "+engineName);
  if(extraName)assert.equal(extras.length,declared.length,"wrong extra patch count "+engineName);
  const operatorVoices=engine===12?rows(source(importFile),"static const uint8_t FM6_IMPORT_PATCH["):[];
  if(engine===12)assert.equal(operatorVoices.length,declared.length);
  for(let k=0;k<declared.length;k++){
    const m=declared[k];
    const edit=m[2].startsWith("{")?list(m[2].slice(1,-1)):null;
    const definition={
      edit,
      ...(edit?{}:{editSymbol:m[2]}),
      envelope:list(m[3]),filterEnvelope:Number(m[4]),
      tag:m[5],effects:m[6]?list(m[6]):null,pattern:m[7]?Number(m[7]):null
    };
    assert.equal(definition.envelope.length,4);
    const patch={
      engine,engineName,enginePresetIndex:k,name:m[1],
      source:"firmware/src/"+file,definition
    };
    if(importFile){
      assert.equal(imported[k].length,115);
      assert.ok(edit && edit.length===8);
      const trailing=extraName?extras[k]:[100,100,100,100];
      assert.equal(trailing.length,4);
      patch.params=[...imported[k],...edit,...trailing];
      assert.equal(patch.params.length,127);
      if(engine===12){
        assert.equal(operatorVoices[k].length,128);
        assert.ok(operatorVoices[k].every(x=>x>=0&&x<128));
        patch.fm6=operatorVoices[k];
      }
    } else {
      patch.requiresEngineFactoryKit=true;
      patch.factoryKitIndex=k;
    }
    result.push(patch);
  }
}
const byEngine=new Map();
for(const p of result){
  if(!byEngine.has(p.engine))byEngine.set(p.engine,[]);
  byEngine.get(p.engine).push(p);
}
let next=33;
for(let round=0;next<=64;round++)
  for(const spec of ENGINE_SPEC){
    const item=byEngine.get(spec[1])?.[round];
    if(item && next<=64)item.initialLibrarySlot=next++;
  }
result.forEach((p,i)=>{p.factoryIndex=i+1;p.sourcePresetIndex=p.enginePresetIndex+1});
assert.equal(result.length,107,"new preset count: update catalog and audit");
assert.equal(result.filter(x=>x.params).length,89);
assert.equal(result.filter(x=>x.requiresEngineFactoryKit).length,18);
assert.equal(result.filter(x=>x.fm6).length,10);
const data={
  format:"mashina-factory-presets-from-code",
  version:1,firmwareBranch:"presets-library",
  generatedFrom:"Factory preset C tables and exact factory-import C arrays",
  totalFactoryPresets:result.length,fullParameterPresets:89,
  drumKitFactoryReferences:18,defaultLibrarySlots:32,presetParameterCount:127,
  notes:[
    "Source catalog of every compiled factory preset; NOT a ready-to-flash 64-slot device backup.",
    "Melodic presets include 127 parameter values; 10 FM6 presets also include the complete 128-byte voice.",
    "DRUM-X, DRUM-LAB, DRUM-AC and DRUM-IND presets include their exact factory table values and kit indices, but not a 304-byte editable drum-kit sidecar. Reconstruct drum kits from engine code or device capture before producing loadable LIBRARY records.",
    "The firmware supports 32 USER + 32 LIBRARY preset slots at once; this 107-entry catalog is an offline source library from which a selection can be made."
  ],presets:result
};
const destination="web/factory-presets-from-code.json";
if(process.argv.includes("--write")){
  fs.writeFileSync(destination,JSON.stringify(data,null,2)+"\n");
  process.stdout.write("Wrote "+destination+": "+result.length+" factory presets\n");
}else if(process.argv.includes("--check")){
  assert.deepEqual(JSON.parse(fs.readFileSync(destination,"utf8")),data);
  process.stdout.write("PASS: compiled factory JSON matches all 107 source definitions and 10 FM6 voices\n");
}else{
  throw Error("Use --check or --write");
}
