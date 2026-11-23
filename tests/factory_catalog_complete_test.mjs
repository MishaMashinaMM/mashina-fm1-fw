/* SPDX-License-Identifier: GPL-3.0-only
 * Tests real generated kit sidecars against all 107 original sounds and all
 * 32 uploadable factory fallback positions. Never touches device flash.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {decodeRecord} from "../web/presets-library-codec.mjs";
import {importFactoryCatalog,importFactoryDrumRecords,defaultFactorySelection,
  validateFactorySelection,factorySelectionFile,importFactorySelection,buildFactoryUpload}
  from "../web/factory-catalog-selection.mjs";
const path=process.argv[2];
assert.ok(path,"expected generated factory-drum-records.json");
const original=readFileSync("web/factory-presets-from-code.json","utf8");
const catalog=importFactoryCatalog(JSON.parse(original));
const kits=importFactoryDrumRecords(JSON.parse(readFileSync(path,"utf8")),catalog);
assert.equal(kits.records.length,18);
const selection=defaultFactorySelection(catalog);
const view=validateFactorySelection(catalog,selection,kits);
assert.equal(view.ready,32);
assert.equal(view.blocked,0);
const records=buildFactoryUpload(catalog,selection,kits);
assert.equal(records.length,32);
assert.equal(records.filter(x=>[10,15,19,20].includes(x.engine)).length,9);
for(let i=0;i<32;i++) {
  const p=catalog.presets[selection[i]],r=records[i];
  assert.equal(r.slot,i);
  assert.equal(r.name,p.name);
  assert.equal(r.engine,p.engine);
  assert.equal(r.recordBytes.length,304);
  assert.deepEqual(decodeRecord(r.recordBytes).recordBytes,r.recordBytes);
}
const exported=factorySelectionFile(catalog,selection,kits);
const restored=importFactorySelection(JSON.parse(JSON.stringify(exported)));
assert.equal(restored.catalog.presets.length,107);
assert.equal(restored.drumRecords.records.length,18);
assert.deepEqual(buildFactoryUpload(restored.catalog,restored.selection,restored.drumRecords),records);
assert.equal(readFileSync("web/factory-presets-from-code.json","utf8"),original,
  "original source JSON must not be rewritten");
const broken=structuredClone(kits);
broken.records[0].recordBytes[303]=0;
assert.throws(()=>importFactoryDrumRecords(broken,catalog),/invalid|incomplete|companion/i);
const renamed=structuredClone(kits);
renamed.records[0].name="WRONG";
assert.throws(()=>importFactoryDrumRecords(renamed,catalog),/name does not match/i);
console.log("PASS: all 107 factory sounds selectable, 32 defaults fully uploadable (9 drum kits), exact JSON round-trip, no master mutations");
