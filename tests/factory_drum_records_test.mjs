/* SPDX-License-Identifier: GPL-3.0-only
 * Verify synthesized 18 factory kit records are complete and use the
 * current firmware's physical 304-byte WEB LIBRARY sound format.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {decodeRecord} from "../web/presets-library-codec.mjs";
const file=process.argv[2];
assert.ok(file,"pass generated drum record JSON");
const raw=JSON.parse(readFileSync(file,"utf8"));
assert.equal(raw.format,"mashina-derived-factory-drum-records");
assert.equal(raw.version,1);
assert.equal(raw.records.length,18);
const source=JSON.parse(readFileSync("web/factory-presets-from-code.json","utf8"));
const drums=source.presets.filter(p=>p.requiresEngineFactoryKit);
assert.equal(drums.length,18);
const visited=new Set();
for(const row of raw.records){
  const key=row.engine+":"+row.enginePresetIndex;
  assert.ok(!visited.has(key),"duplicate compiled factory drum kit "+key);
  visited.add(key);
  const matching=drums.find(p=>p.engine===row.engine &&
    p.enginePresetIndex===row.enginePresetIndex);
  assert.ok(matching,"unexpected factory kit "+key);
  assert.equal(row.name,matching.name);
  assert.equal(row.recordBytes.length,304);
  const record=decodeRecord(row.recordBytes);
  assert.equal(record.name,matching.name);
  assert.equal(record.engine,matching.engine);
  assert.equal(record.recordBytes[303],0xd3);
  assert.ok(record.recordBytes.slice(192,303).some(v=>v!==0),
            "missing genuine synthesized pad data "+row.name);
}
// DRUM-X has nine distinct embedded factory kits, not merely nine sets
// of EDIT values above an accidentally re-used first kit. This guards
// against calling apply_preset_to instead of the complete apply_preset path.
const drumx=raw.records.filter(p=>p.engine===10);
assert.equal(drumx.length,9);
const dxKitSignatures=drumx.map(p=>JSON.stringify(p.recordBytes.slice(192,303)));
assert.equal(new Set(dxKitSignatures).size,9,
             "nine distinct BUTTLER kits were not captured independently");
console.log("PASS: 18 complete factory DRUM-X/LAB/AC/IND presets derived using firmware kit capture; 9 distinct BUTTLER kit sidecars");
