/* SPDX-License-Identifier: GPL-3.0-only
 * Offline-only performance bundle: 32 USER presets, 32 LIBRARY slots, 32
 * project sections (8 projects x A-D). No device writes. Not yet a complete
 * FM1 backup: global device settings and real project WebMIDI are pending.
 */
import {decodeRecord} from "./presets-library-codec.mjs";
import {decodeProject,SECTION_COUNT,PROJECT_COUNT} from "./projects-library-codec.mjs";
const TOTAL=PROJECT_COUNT*SECTION_COUNT;
const check=(ok,msg)=>{if(!ok)throw new Error(msg);};
function presetPositions(input,bank) {
  check(Array.isArray(input)&&input.length===32,bank+" requires 32 fixed positions");
  return input.map((item,index)=>{
    if(item===null)return null; // empty USER, or LIBRARY compiled factory
    const bytes=Array.isArray(item)?item:item?.recordBytes;
    const p=decodeRecord(bytes);
    return p.recordBytes;
  });
}
function projectPositions(input) {
  check(Array.isArray(input)&&input.length===TOTAL,"projects require exactly 32 positions");
  return input.map((item,index)=>{
    if(item===null)return null;
    const bytes=Array.isArray(item)?item:item?.recordBytes;
    const section=decodeProject(bytes);
    check(section.section===index%SECTION_COUNT,
          "project "+(Math.floor(index/SECTION_COUNT)+1)+" section "+("ABCD"[index%4])+" is misplaced");
    return section.recordBytes;
  });
}
export function analyzeProjectReferences(projects,users) {
  const missing=[],used=new Map();
  for(let index=0;index<TOTAL;index++) {
    if(projects[index]===null)continue;
    const p=decodeProject(projects[index]);
    for(let track=0;track<p.refs.length;track++) {
      const ref=p.refs[track];
      if(ref===255)continue;
      const places=used.get(ref)??[];
      places.push({project:Math.floor(index/4)+1,section:"ABCD"[index%4],track:track+1});
      used.set(ref,places);
      if(ref<32 && users[ref]===null)
        missing.push({preset:ref+1,project:Math.floor(index/4)+1,section:"ABCD"[index%4],track:track+1});
    }
  }
  return {missingUserPresets:missing,usedPresetCount:used.size};
}
export function makePerformanceBundle({userPresets,libraryPresets,projects}) {
  const user=presetPositions(userPresets,"USER");
  const lib=presetPositions(libraryPresets,"LIBRARY");
  const sections=projectPositions(projects);
  const issues=analyzeProjectReferences(sections,user);
  check(issues.missingUserPresets.length===0,
        "backup contains project references to missing USER presets; cannot form self-contained performance bundle");
  return {format:"mashina-performance-bundle",version:1,
    userPresets:user,libraryPresets:lib,projects:sections};
}
export function parsePerformanceBundle(input) {
  check(input?.format==="mashina-performance-bundle"&&input.version===1,
        "unsupported performance bundle");
  return makePerformanceBundle(input);
}
/* Validate before any restoration step. A LIBRARY slot with null content uses
 * the compiled factory fallback and is still a valid reference. */
export function planPerformanceRestore(bundle) {
  const parsed=parsePerformanceBundle(bundle);
  return {
    bundle:parsed,
    order:["USER", "LIBRARY", "PROJECTS"],
    userWrites:parsed.userPresets.filter(x=>x!==null).length,
    userErases:parsed.userPresets.filter(x=>x===null).length,
    libraryWrites:parsed.libraryPresets.filter(x=>x!==null).length,
    libraryErases:parsed.libraryPresets.filter(x=>x===null).length,
    projectWrites:parsed.projects.filter(x=>x!==null).length,
    projectErases:parsed.projects.filter(x=>x===null).length,
    // Null is an EXPLICIT erase on restore: otherwise a factory fallback
    // could be replaced by stale uploaded data on the target FM1.
    // No automatic erasure/USB activity; caller implements staged restore
    // and explicit confirmation after project transport is available.
  };
}
