/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {encodeRecord} from "../web/presets-library-codec.mjs";
import {encodeProject} from "../web/projects-library-codec.mjs";
import {makePerformanceBundle,parsePerformanceBundle,planPerformanceRestore,analyzeProjectReferences} from "../web/performance-backup.mjs";
const user=Array(32).fill(null),library=Array(32).fill(null),projects=Array(32).fill(null);
const params=Array.from({length:127},(_,i)=>(i%32));
user[13]=Array.from(encodeRecord({engine:0,name:"BASS 14",params}));
library[12]=Array.from(encodeRecord({engine:0,name:"LEAD 45",params}));
projects[5]=encodeProject({section:1,refs:[13,44,255,13,44,255],
                          name:"PROJECT 2B",settingsBytes:[0,255,1,128,47,11]});
const bundle=makePerformanceBundle({userPresets:user,libraryPresets:library,projects});
assert.equal(bundle.userPresets.length,32);
assert.equal(bundle.libraryPresets.length,32);
assert.equal(bundle.projects.length,32);
const restored=parsePerformanceBundle(JSON.parse(JSON.stringify(bundle)));
assert.deepEqual(restored,bundle);
const plan=planPerformanceRestore(restored);
assert.deepEqual(plan.order,["USER","LIBRARY","PROJECTS"]);
assert.equal(plan.userWrites,1);
assert.equal(plan.libraryWrites,1);
assert.equal(plan.projectWrites,1);
assert.equal(plan.userErases,31);
assert.equal(plan.libraryErases,31);
assert.equal(plan.projectErases,31);
const usage=analyzeProjectReferences(restored.projects,restored.userPresets);
assert.equal(usage.missingUserPresets.length,0);
assert.equal(usage.usedPresetCount,2);
// Incomplete standalone imports must fail instead of loading the wrong sound.
const missing=[...user];missing[13]=null;
const issue=analyzeProjectReferences(projects,missing);
assert.equal(issue.missingUserPresets.length,2);
assert.equal(issue.missingUserPresets[0].preset,14);
assert.equal(issue.missingUserPresets[0].project,2);
assert.equal(issue.missingUserPresets[0].section,"B");
assert.throws(()=>makePerformanceBundle({userPresets:missing,libraryPresets:library,projects}),/missing USER/);
// A LIBRARY reference to a compiled factory remains valid with null slot.
const factory=[...library];factory[12]=null;
const fallback=makePerformanceBundle({userPresets:user,libraryPresets:factory,projects});
assert.equal(planPerformanceRestore(fallback).libraryWrites,0);
assert.equal(planPerformanceRestore(fallback).libraryErases,32);
const bad=[...projects];bad[5]=[...projects[5]];bad[5][33]^=1;
assert.throws(()=>makePerformanceBundle({userPresets:user,libraryPresets:library,projects:bad}),/checksum/);
assert.throws(()=>parsePerformanceBundle({...bundle,version:2}),/unsupported/);
console.log("PASS: self-contained USER/LIBRARY/project bundle, fallback, JSON round-trip, missing-reference safety and restore ordering");

import {mergeCollection,collectionFile,readCollection,removeSoundsFromCollection} from "../web/performance-backup.mjs";
const A=user[13],B=library[12],C=Array.from(encodeRecord({engine:0,name:"BASS 14",params:params.map(x=>(x+1)%32)}));
for(const kind of ["user","library"]){
  const initial=mergeCollection(kind,[A,B,C,A]);
  const mapped=[{slot:5,recordBytes:A},{slot:7,recordBytes:B}];
  const edited=removeSoundsFromCollection(kind,initial,mapped,[{recordBytes:A},{recordBytes:B}]);
  assert.equal(edited.removed,2);assert.deepEqual(edited.entries,[{recordBytes:C}]);assert.deepEqual(edited.packs,[]);
  assert.equal(initial.length,3);assert.equal(mapped.length,2);
  assert.deepEqual(readCollection(kind,JSON.parse(JSON.stringify(collectionFile(kind,edited.entries)))),edited.entries);
  assert.deepEqual(mergeCollection(kind,edited.entries,[A,B]).map(p=>p.recordBytes),[C,A,B]);
  const empty=removeSoundsFromCollection(kind,edited.entries,[],edited.entries);
  assert.deepEqual(readCollection(kind,collectionFile(kind,empty.entries)),[]);
}
console.log("PASS: editable collections remove any source, preserve same-name variants, remove fixed-slot sources, save and reopen exact remaining records");

import {collectionFilename} from "../web/performance-backup.mjs";
assert.equal(collectionFilename("My live presets"),"My live presets.json");
assert.equal(collectionFilename("  Set 1.JSON  "),"Set 1.json");
assert.equal(collectionFilename("Set.json.json"),"Set.json");
assert.equal(collectionFilename("Моји звуци"),"Моји звуци.json");
for(const bad of ["","   ",".json","../escape","C:\\test","NUL","COM1","LPT2.txt","bad?","ending.","a".repeat(176)])assert.throws(()=>collectionFilename(bad));
console.log("PASS: collection download filenames accept custom names and normalize JSON extension");
