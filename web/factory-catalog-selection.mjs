/* SPDX-License-Identifier: GPL-3.0-only
 * Factory source catalog -> selectable 32-slot Web Library pack.
 *
 * This is NON-DESTRUCTIVE. The 107-source catalog is never modified and
 * incomplete drum kits are NEVER converted into guessed binary records.
 * An offline selection remains self-contained even if the source file moves.
 */
import {encodeRecord,decodeRecord,LIBRARY_SLOTS} from "./presets-library-codec.mjs";

const fail=(condition,message)=>{if(!condition)throw Error(message);};
const SIDECAR_ENGINES=new Set([10,15,19,20]);
const CLEAN_NAME=/^[\x20-\x7e]{1,12}$/;
const VALID_ENGINE=new Set([0,2,3,5,7,10,11,12,15,16,17,18,19,20]);
const uint=(x,max)=>Number.isInteger(x)&&x>=0&&x<=max;
export const SOURCE_FORMAT="mashina-factory-presets-from-code";
export const SELECTION_FORMAT="mashina-factory-selection-32";
export const UNRESOLVED_DRUM_MESSAGE="Factory drum kit is not yet serialized as a complete 304-byte record. Selection can be saved, but uploading this slot is blocked.";

function validatePatch(p,at){
  fail(p&&typeof p==="object"&&uint(p.factoryIndex,107)&&p.factoryIndex===at+1,
       "Factory index mismatch at entry "+(at+1));
  fail(VALID_ENGINE.has(p.engine)&&uint(p.enginePresetIndex,106),
       "Invalid engine or factory engine index");
  fail(typeof p.name==="string"&&CLEAN_NAME.test(p.name),
       "Invalid factory name at entry "+(at+1));
  fail(p.definition&&Array.isArray(p.definition.envelope)&&p.definition.envelope.length===4,
       "Missing factory envelope");
  if(SIDECAR_ENGINES.has(p.engine)){
    fail(p.requiresEngineFactoryKit===true&&
         p.factoryKitIndex===p.enginePresetIndex,
         "Drum factory patch is missing its kit reference");
    fail(!p.recordBytes, "Factory kit reference is not a complete binary record");
  }else{
    fail(Array.isArray(p.params)&&p.params.length===127&&
         p.params.every(v=>Number.isInteger(v)&&v>=-64&&v<=127),
         "Factory sound "+p.name+" needs 127 valid parameter values");
    if(p.engine===12)fail(Array.isArray(p.fm6)&&p.fm6.length===128&&
                          p.fm6.every(v=>uint(v,127)),
                          "FM6 "+p.name+" missing 128-byte operator voice");
    else fail(p.fm6===undefined,"Unexpected FM6 patch on "+p.name);
    // Conversion is checked here, before users select or export anything.
    decodeRecord(encodeRecord(p));
  }
  return p;
}
export function importFactoryCatalog(input){
  fail(input?.format===SOURCE_FORMAT&&input.version===1,
       "Not the original MASHINA factory source catalog JSON");
  fail(Array.isArray(input.presets)&&input.presets.length===107,
       "Factory catalog must contain all 107 presets");
  fail(input.totalFactoryPresets===107&&input.fullParameterPresets===89&&
       input.drumKitFactoryReferences===18,
       "Catalog inventory counts do not match MASHINA firmware");
  const copy=structuredCloneCompatible(input);
  const perEngine=new Map(),mapped=new Set();
  let full=0,kits=0;
  for(const [index,p] of copy.presets.entries()){
    validatePatch(p,index);
    const n=perEngine.get(p.engine)??0;
    fail(p.enginePresetIndex===n,
         "Factory engine preset order differs at "+p.name);
    perEngine.set(p.engine,n+1);
    if(p.initialLibrarySlot!==undefined){
      fail(uint(p.initialLibrarySlot,64)&&p.initialLibrarySlot>=33&&
           !mapped.has(p.initialLibrarySlot),"Invalid default LIBRARY slot");
      mapped.add(p.initialLibrarySlot);
    }
    if(SIDECAR_ENGINES.has(p.engine))kits++;else full++;
  }
  fail(full===89&&kits===18&&mapped.size===32,
       "Factory catalog incomplete: no presets were imported");
  return copy;
}
// JSON round-trip gives a plain deep copy, supported on older Chromium too.
function structuredCloneCompatible(v){return JSON.parse(JSON.stringify(v));}
function validSelectionIndexes(indexes){
  fail(Array.isArray(indexes)&&indexes.length===LIBRARY_SLOTS,
       "A selection must describe exactly 32 LIBRARY positions");
  return indexes.map((v,i)=>{
    if(v===null)return null;
    fail(uint(v,106),"Invalid source factory index in LIBRARY "+(i+33));
    return v;
  });
}
export function defaultFactorySelection(catalog){
  const c=importFactoryCatalog(catalog);
  const list=Array(LIBRARY_SLOTS).fill(null);
  for(const p of c.presets)
    if(p.initialLibrarySlot!==undefined)
      list[p.initialLibrarySlot-33]=p.factoryIndex-1;
  return list;
}
export function validateFactorySelection(catalog,indexes){
  const c=importFactoryCatalog(catalog);
  const refs=validSelectionIndexes(indexes);
  const entries=refs.map((factoryIndex,slot)=>{
    if(factoryIndex===null)return null;
    const sound=c.presets[factoryIndex];
    return {slot,librarySlot:slot+33,sourceFactoryIndex:factoryIndex+1,
            name:sound.name,engine:sound.engine,engineName:sound.engineName,
            ready:!SIDECAR_ENGINES.has(sound.engine),
            issue:SIDECAR_ENGINES.has(sound.engine)?UNRESOLVED_DRUM_MESSAGE:null};
  });
  return {entries,ready:entries.filter(x=>x?.ready).length,
          blocked:entries.filter(x=>x&&!x.ready).length,
          empty:entries.filter(x=>x===null).length};
}
export function factorySelectionFile(catalog,indexes){
  const c=importFactoryCatalog(catalog);
  const refs=validSelectionIndexes(indexes);
  validateFactorySelection(c,refs);
  return {format:SELECTION_FORMAT,version:1,
          catalog:c,selection:refs};
}
export function importFactorySelection(file){
  fail(file?.format===SELECTION_FORMAT&&file.version===1,
       "Not a MASHINA 32-slot factory selection JSON");
  const catalog=importFactoryCatalog(file.catalog);
  const selection=validSelectionIndexes(file.selection);
  validateFactorySelection(catalog,selection);
  return {catalog,selection};
}
/* No writes and no guesswork. If even one selected drum cannot produce its
 * full kit sidecar, reject the WHOLE source-selection upload. Otherwise
 * existing prepared complete binary records are returned by slot. */
export function buildFactoryUpload(catalog,indexes){
  const c=importFactoryCatalog(catalog),refs=validSelectionIndexes(indexes);
  const records=[];
  for(let slot=0;slot<LIBRARY_SLOTS;slot++){
    const index=refs[slot];
    if(index===null)continue;
    const p=c.presets[index];
    fail(!SIDECAR_ENGINES.has(p.engine),
         "LIBRARY "+(slot+33)+" ("+p.name+"): "+UNRESOLVED_DRUM_MESSAGE);
    const rec=encodeRecord(p);
    records.push({slot,...decodeRecord(rec)});
  }
  return records;
}
