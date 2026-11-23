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
const bad=[...projects];bad[5]=[...projects[5]];bad[5][33]^=1;
assert.throws(()=>makePerformanceBundle({userPresets:user,libraryPresets:library,projects:bad}),/checksum/);
assert.throws(()=>parsePerformanceBundle({...bundle,version:2}),/unsupported/);
console.log("PASS: self-contained USER/LIBRARY/project bundle, fallback, JSON round-trip, missing-reference safety and restore ordering");
