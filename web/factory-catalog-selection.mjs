/* SPDX-License-Identifier: GPL-3.0-only
 * Factory source catalog -> selectable 32-slot Web Library pack.
 *
 * This is NON-DESTRUCTIVE. The 107-source catalog is never modified and
 * incomplete drum kits are NEVER converted into guessed binary records.
 * An offline selection remains self-contained even if the source file moves.
 */
import {encodeRecord,decodeRecord,LIBRARY_SLOTS,validLibrarySize} from "./presets-library-codec.mjs";

const fail=(condition,message)=>{if(!condition)throw Error(message);};
const SIDECAR_ENGINES=new Set([10,15,19,20]);
const CLEAN_NAME=/^[\x20-\x7e]{1,12}$/;
const VALID_ENGINE=new Set([0,2,3,5,7,10,11,12,15,16,17,18,19,20]);
const uint=(x,max)=>Number.isInteger(x)&&x>=0&&x<=max;
export const SOURCE_FORMAT="mashina-factory-presets-from-code";
export const SELECTION_FORMAT="mashina-factory-selection-32";
/* Fixed fingerprint of the exact 107 original curated presets' parsed JSON.
 * Whitespace is irrelevant; changing any patch value, FM6 voice or metadata
 * requires deliberate regeneration/review of this protected source revision. */
export const MASTER_FNV32=0x4b96d34f;
function masterFingerprint(source){
  const str=JSON.stringify(source);
  let hash=2166136261;
  for(let i=0;i<str.length;i++)hash=Math.imul(hash^str.charCodeAt(i),16777619)>>>0;
  return hash;
}
export const UNRESOLVED_DRUM_MESSAGE="Factory drum kit needs the matching complete 304-byte companion records. Selection can be saved, but uploading this slot is blocked.";

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
  fail(masterFingerprint(input)===MASTER_FNV32,
       "This is not the exact protected 107-preset source revision. The original sounds/metadata differ; refusing to silently substitute them.");
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
  fail(Array.isArray(indexes)&&validLibrarySize(indexes.length),
       "A selection must describe exactly 32 or 96 LIBRARY positions");
  return indexes.map((v,i)=>{
    if(v===null)return null;
    fail(uint(v,106),"Invalid source factory index in LIBRARY "+(i+33));
    return v;
  });
}
/* A separate, firmware-derived sidecar adds complete factory kits WITHOUT
 * changing a single byte of the original 107-preset source catalog.
 * Exactly one full record for each of 18 factory drum sounds is mandatory;
 * reordering, truncation, swapped names or mismatched engine is rejected.
 */
export function importFactoryDrumRecords(input,catalog){
  const c=importFactoryCatalog(catalog);
  fail(input?.format==="mashina-derived-factory-drum-records"&&input.version===1 &&
       Array.isArray(input.records)&&input.records.length===18,
       "Expected 18 compiled factory drum-kit records");
  const map=new Map();
  for(const record of input.records){
    const p=c.presets.find(x=>x.engine===record?.engine &&
                            x.enginePresetIndex===record?.enginePresetIndex);
    fail(!!p&&p.requiresEngineFactoryKit,"Unknown factory drum engine/preset index");
    fail(record.name===p.name,"Drum-kit name does not match source: "+p.name);
    fail(!map.has(p.factoryIndex-1),"Duplicate factory drum record "+p.name);
    const decoded=decodeRecord(record.recordBytes);
    fail(decoded.engine===p.engine&&decoded.name===p.name&&
         decoded.recordBytes[303]===0xD3,
         "Invalid or incomplete drum-kit sidecar: "+p.name);
    let checksum=2166136261;
    for(const value of decoded.recordBytes)
      checksum=Math.imul(checksum^value,16777619)>>>0;
    fail(record.fnv32===checksum,
         "Factory drum-kit integrity checksum mismatch: "+p.name);
    map.set(p.factoryIndex-1,decoded.recordBytes);
  }
  fail(map.size===18,"Factory drum sidecars are incomplete");
  return structuredCloneCompatible(input);
}
function validatedKits(catalog,kits){
  if(kits===null||kits===undefined)return null;
  const list=importFactoryDrumRecords(kits,catalog);
  return new Map(list.records.map(p=>{
    const idx=catalog.presets.findIndex(x=>x.engine===p.engine &&
                                      x.enginePresetIndex===p.enginePresetIndex);
    return [idx,p.recordBytes];
  }));
}

export function defaultFactorySelection(catalog,slots=LIBRARY_SLOTS){
  const c=importFactoryCatalog(catalog);
  fail(validLibrarySize(slots),"Unsupported factory LIBRARY geometry");
  const byEngine=new Map();
  for(const p of c.presets){
    if(!byEngine.has(p.engine))byEngine.set(p.engine,[]);
    byEngine.get(p.engine).push(p);
  }
  const list=[];
  for(let round=0;list.length<slots&&round<c.presets.length;round++)
    for(const engine of byEngine.values()){
      const p=engine[round];
      if(p&&list.length<slots)list.push(p.factoryIndex-1);
    }
  fail(list.length===slots,"Compiled catalog too small");
  for(let i=0;i<32;i++)
    fail(c.presets[list[i]].initialLibrarySlot===i+33,"Existing factory mapping changed");
  return list;
}
export function validateFactorySelection(catalog,indexes,kits=null){
  const c=importFactoryCatalog(catalog);
  const refs=validSelectionIndexes(indexes);
  const fullKits=validatedKits(c,kits);
  const entries=refs.map((factoryIndex,slot)=>{
    if(factoryIndex===null)return null;
    const sound=c.presets[factoryIndex];
    return {slot,librarySlot:slot+33,sourceFactoryIndex:factoryIndex+1,
            name:sound.name,engine:sound.engine,engineName:sound.engineName,
            ready:!SIDECAR_ENGINES.has(sound.engine)||fullKits?.has(factoryIndex)===true,
            issue:SIDECAR_ENGINES.has(sound.engine)&&!fullKits?.has(factoryIndex)?
                  UNRESOLVED_DRUM_MESSAGE:null};
  });
  return {entries,ready:entries.filter(x=>x?.ready).length,
          blocked:entries.filter(x=>x&&!x.ready).length,
          empty:entries.filter(x=>x===null).length};
}
export function factorySelectionFile(catalog,indexes,kits=null){
  const c=importFactoryCatalog(catalog);
  const refs=validSelectionIndexes(indexes);
  const full= kits===null ? null : importFactoryDrumRecords(kits,c);
  validateFactorySelection(c,refs,full);
  return {format:refs.length===96?"mashina-factory-selection-96":SELECTION_FORMAT,version:1,
          catalog:c,selection:refs,
          ...(full?{drumRecords:full}:{})};
}
export function importFactorySelection(file){
  fail([SELECTION_FORMAT,"mashina-factory-selection-96"].includes(file?.format)&&file.version===1,
       "Not a MASHINA factory selection JSON");
  const catalog=importFactoryCatalog(file.catalog);
  const selection=validSelectionIndexes(file.selection);
  fail(selection.length===(file.format===SELECTION_FORMAT?32:96),"Factory selection geometry mismatch");
  const drumRecords=file.drumRecords===undefined?null:
                    importFactoryDrumRecords(file.drumRecords,catalog);
  validateFactorySelection(catalog,selection,drumRecords);
  return {catalog,selection,drumRecords};
}
/* No writes and no guesswork. If even one selected drum cannot produce its
 * full kit sidecar, reject the WHOLE source-selection upload. Otherwise
 * existing prepared complete binary records are returned by slot. */
export function buildFactoryUpload(catalog,indexes,kits=null){
  const c=importFactoryCatalog(catalog),refs=validSelectionIndexes(indexes);
  const fullKits=validatedKits(c,kits);
  const records=[];
  for(let slot=0;slot<refs.length;slot++){
    const index=refs[slot];
    if(index===null)continue;
    const p=c.presets[index];
    if(SIDECAR_ENGINES.has(p.engine)){
      fail(fullKits?.has(index),
           "LIBRARY "+(slot+33)+" ("+p.name+"): "+UNRESOLVED_DRUM_MESSAGE);
      records.push({slot,...decodeRecord(fullKits.get(index))});
    }else{
      const rec=encodeRecord(p);
      records.push({slot,...decodeRecord(rec)});
    }
  }
  return records;
}
