/* SPDX-License-Identifier: GPL-3.0-only
 * Offline-only performance bundle: 32 USER presets, 32 LIBRARY slots, 32
 * project sections (8 projects x A-D). No device writes. Not yet a complete
 * FM1 backup: global device settings and real project WebMIDI are pending.
 */
import {decodeRecord,encodeRecord,validLibrarySize} from "./presets-library-codec.mjs";
import {decodeProject,restoreProjectsBackup,SECTION_COUNT,PROJECT_COUNT} from "./projects-library-codec.mjs";
const TOTAL=PROJECT_COUNT*SECTION_COUNT;
const check=(ok,msg)=>{if(!ok)throw new Error(msg);};
function presetPositions(input,bank) {
  check(Array.isArray(input)&&(bank==="LIBRARY"?validLibrarySize(input.length):input.length===32),bank+" has invalid fixed positions");
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
  for(const raw of sections)if(raw!==null)
    check(decodeProject(raw).refs.every(ref=>ref===255||ref<32+lib.length), "Project reference exceeds LIBRARY geometry");
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


/* Offline collections have no device-slot limit. Entries are complete
 * records, independent of the 32-position device JSON formats. Projects
 * are grouped into four A-D sections, with fixed slot references intact.
 * Exact duplicate entries are merged; same-name variants are retained. */
export function mergeCollection(kind,...lists){
  check(["user","library","projects"].includes(kind),"Unknown collection type");
  const result=[],seen=new Set();
  for(const list of lists){
    check(Array.isArray(list),"Invalid collection entries");
    for(const item of list){
      let entry,key;
      if(kind==="projects"){
        check(Array.isArray(item?.sections)&&item.sections.length===4,"Project collection entry needs four A–D sections");
        const sections=item.sections.map((r,i)=>{
          if(r===null)return null;
          const p=decodeProject(r);check(p.section===i,"Project collection section mismatch");return p.recordBytes;
        });
        if(!sections.some(Boolean))continue;
        const name=String(item.name||sections.map(r=>r&&decodeProject(r).name).find(Boolean)||"Unnamed project").slice(0,120);
        entry={name,sections};key=JSON.stringify(sections);
      }else{
        const p=decodeRecord(item.recordBytes??item);
        entry={recordBytes:p.recordBytes};key=JSON.stringify(p.recordBytes);
      }
      if(!seen.has(key)){seen.add(key);result.push(entry);}
    }
  }
  return result;
}
/* Removal changes only the editable local collection and its imported
 * fixed-slot source mapping, never device snapshots or pending writes. */
export function removeSoundsFromCollection(kind,entries,packs,chosen){
  check(kind==="user"||kind==="library","Expected a sound collection");
  const keys=new Set(mergeCollection(kind,chosen).map(p=>JSON.stringify(p.recordBytes)));
  const before=mergeCollection(kind,entries);
  const keep=p=>!keys.has(JSON.stringify(p.recordBytes));
  const remaining=before.filter(keep);
  return {entries:remaining,packs:packs.filter(keep),removed:before.length-remaining.length};
}
/* Portable local download name; preserve the user's spelling and spaces. */
export function collectionFilename(value){
  let base=String(value??"").trim().replace(/(?:\.json)+$/i,"").trim();
  check(base.length>0,"Enter a file name.");
  check(base.length<=175,"File name is too long.");
  check(!/[<>:"/\\|?*\x00-\x1f]/.test(base),"File name contains an invalid character.");
  check(!/[. ]$/.test(base)&&!/^(?:CON|PRN|AUX|NUL|COM[1-9¹²³]|LPT[1-9¹²³])(?:\.|$)/i.test(base),"Choose a valid file name.");
  return base+".json";
}
export function collectionFile(kind,entries){
  return {format:"mashina-offline-collection",version:1,kind,entries:mergeCollection(kind,entries)};
}
export function projectCollectionFromSections(sections){
  check(Array.isArray(sections)&&sections.length===32,"Expected 8 × A–D sections");
  return mergeCollection("projects",Array.from({length:8},(_,i)=>({sections:sections.slice(4*i,4*i+4)})));
}
export function readCollection(kind,file){
  if(file?.format==="mashina-offline-collection"){
    check(file.version===1&&file.kind===kind,"Collection type/version does not match this tab");
    return mergeCollection(kind,file.entries);
  }
  if(kind==="projects"){
    const sections=file?.format==="mashina-full-device"?file.projects:restoreProjectsBackup(file);
    return projectCollectionFromSections(sections);
  }
  if(file?.format==="mashina-full-device"){
    const entries=file[kind==="user"?"userPresets":"libraryPresets"];
    check(Array.isArray(entries)&&(kind==="library"?validLibrarySize(entries.length):entries.length===32),"Invalid full backup preset bank");
    return mergeCollection(kind,entries.filter(Boolean));
  }
  check(Array.isArray(file?.patches),"Expected a sound collection or sound pack");
  return mergeCollection(kind,file.patches.map(p=>({recordBytes:Array.from(encodeRecord(p))})));
}


/* Group placement is an atomic, non-wrapping permutation of fixed slots.
 * Source order is ascending slot number. For overlapping drops, displaced
 * destination-only records fill the vacated source-only slots in order.
 * This preserves every record, even for a noncontiguous selection. */
export function groupPlacement(values,sourceSlots,start){
  check(Array.isArray(values)&&Array.isArray(sourceSlots)&&sourceSlots.length>0,"Select at least one item");
  const src=[...sourceSlots].sort((a,b)=>a-b),n=values.length;
  check(new Set(src).size===src.length&&src.every(i=>Number.isInteger(i)&&i>=0&&i<n),"Invalid source selection");
  check(Number.isInteger(start)&&start>=0&&start+src.length<=n,"Selection does not fit from this destination");
  const dst=src.map((_,i)=>start+i),sourceSet=new Set(src),destSet=new Set(dst);
  const displaced=dst.filter(i=>!sourceSet.has(i)),vacated=src.filter(i=>!destSet.has(i));
  return [...dst.map((slot,i)=>({slot,value:values[src[i]]})),...vacated.map((slot,i)=>({slot,value:values[displaced[i]]}))];
}
