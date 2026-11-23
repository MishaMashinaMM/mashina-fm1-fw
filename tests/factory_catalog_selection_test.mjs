/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {execFileSync} from "node:child_process";
import {encodeRecord} from "../web/presets-library-codec.mjs";
import {importFactoryCatalog,defaultFactorySelection,validateFactorySelection,
        factorySelectionFile,importFactorySelection,buildFactoryUpload}
  from "../web/factory-catalog-selection.mjs";

const raw=readFileSync("web/factory-presets-from-code.json","utf8");
const original=JSON.parse(raw);
// Immutable baseline for the carefully curated 107-preset export. Updating
// this SHA requires an explicit reviewed regeneration, never a selection UI
// operation or automatic write-back.
const expectedMasterBlob="2b1bd810365dc42c6491cab2cca673055e6e04e2";
assert.equal(execFileSync("git",["hash-object","web/factory-presets-from-code.json"],{encoding:"utf8"}).trim(),
             expectedMasterBlob,"the protected 107-preset master JSON changed");
const catalog=importFactoryCatalog(original);
assert.equal(JSON.stringify(original),JSON.stringify(catalog));
assert.equal(original.presets.length,107);
assert.equal(original.presets.filter(p=>p.fm6).length,10);
const defaults=defaultFactorySelection(catalog);
assert.equal(defaults.length,32);
assert.equal(defaults.filter(i=>i!==null).length,32);
const view=validateFactorySelection(catalog,defaults);
assert.equal(view.ready+view.blocked,32);
assert.equal(view.blocked,9); // 2 each for four drums, plus third DRUM-X
assert.equal(view.ready,23);
assert.throws(()=>buildFactoryUpload(catalog,defaults),/drum kit|kit is not yet serialized/i);
const backup=factorySelectionFile(catalog,defaults);
const again=importFactorySelection(JSON.parse(JSON.stringify(backup)));
assert.deepEqual(again.selection,defaults);
assert.equal(JSON.stringify(again.catalog),JSON.stringify(catalog));
assert.equal(JSON.stringify(original),JSON.stringify(JSON.parse(raw)),
  "source master JSON must remain untouched");
const melodic=[...catalog.presets.flatMap((p,i)=>p.engine===12?[i]:[]),
  ...catalog.presets.flatMap((p,i)=>p.params&&p.engine!==12?[i]:[])].slice(0,32);
assert.equal(melodic.length,32);
const plan=buildFactoryUpload(catalog,melodic);
assert.equal(plan.length,32);
for(let i=0;i<32;i++){
  assert.equal(plan[i].slot,i);
  assert.deepEqual(plan[i].recordBytes,Array.from(encodeRecord(catalog.presets[melodic[i]])));
}
assert.equal(plan.filter(p=>p.engine===12).length,10);
// The complete FM6 voices must survive catalog -> WebMIDI record conversion.
for(const row of plan.filter(p=>p.engine===12)){
  const patch=catalog.presets[melodic[row.slot]];
  assert.deepEqual(row.fm6,patch.fm6);
}
const altered=structuredClone(catalog);
altered.presets[0].params[0]=256;
assert.throws(()=>importFactoryCatalog(altered),/exact protected 107-preset source revision/i);
const plausibleEdit=structuredClone(catalog);
plausibleEdit.presets[0].params[0]=99;
assert.throws(()=>importFactoryCatalog(plausibleEdit),/exact protected 107-preset source revision/i);
const missing=structuredClone(catalog);
missing.presets.pop();
assert.throws(()=>importFactoryCatalog(missing),/107 presets/i);
const malformed=structuredClone(catalog);
malformed.presets[37].initialLibrarySlot=33; // duplicate default
assert.throws(()=>importFactoryCatalog(malformed),/exact protected 107-preset source revision/);
assert.throws(()=>importFactorySelection({...backup,version:2}),/selection JSON/);
assert.equal(validateFactorySelection(catalog,Array(32).fill(null)).empty,32);
assert.ok(raw.includes('"totalFactoryPresets": 107'));
console.log("PASS: original 107-entry JSON imports unchanged; 32 defaults, strict drum blocking, 32 complete melodic records and 10 full FM6 voices survive export/import");
