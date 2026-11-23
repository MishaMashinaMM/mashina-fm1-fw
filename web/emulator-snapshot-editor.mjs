/* SPDX-License-Identifier: GPL-3.0-only
 * OFFLINE ONLY: copy selected, complete 304-byte sounds into LIBRARY bank
 * positions inside an *exported experimental emulator flash JSON*. Writes
 * neither browser localStorage nor WebMIDI/physical FM1. Does not invent USER
 * drum/FM6 companions or overwrite the protected factory master archive.
 */
import {encodeEmulatorFlash,decodeEmulatorFlash} from "./emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash} from "./emulator-library-inspector.mjs";
import {decodeRecord,encodeRecord} from "./presets-library-codec.mjs";
import {importFactoryCatalog,importFactoryDrumRecords} from "./factory-catalog-selection.mjs";
const SECTOR=4096, LIB_BASE=0xea000, PAYLOAD=256, BANK_BYTES=2432;
const check=(ok,msg)=>{if(!ok)throw Error(msg);};
const u32=(x,p)=>(x[p]|x[p+1]<<8|x[p+2]<<16|x[p+3]<<24)>>>0;
const put32=(x,p,n)=>{for(let i=0;i<4;i++)x[p+i]=(n>>>(8*i))&255;};
const isErased=x=>x.every(v=>v===255);
const newer=(a,b)=>a!==b&&((b-a)>>>0)<0x80000000;
export function crc32(bytes){
  let c=0xffffffff;
  for(const b of bytes){c^=b;for(let k=0;k<8;k++)c=(c>>>1)^((c&1)?0xedb88320:0);}
  return (~c)>>>0;
}
function sector(sectors,off){return sectors[off]??new Uint8Array(SECTOR).fill(255);}
function bankState(sectors,bank){
  const base=LIB_BASE+bank*2*SECTOR;
  const candidates=[0,1].map(copy=>{
    const bytes=sector(sectors,base+copy*SECTOR);
    const valid=u32(bytes,0)===0x3142494c&&u32(bytes,4)===1&&
      u32(bytes,8)===bank&&u32(bytes,16)===BANK_BYTES&&
      u32(bytes,20)===crc32(bytes.slice(PAYLOAD,PAYLOAD+BANK_BYTES));
    return {copy,bytes,valid,seq:u32(bytes,12)};
  });
  let current=null;
  for(const choice of candidates)if(choice.valid&&
     (!current||newer(current.seq,choice.seq)))current=choice;
  check(current||candidates.every(x=>isErased(x.bytes)),
    "LIBRARY bank "+bank+" contains nonempty flash without valid commit");
  return {base,current,bank};
}
export function factorySourceRecords(catalog,kits=null){
  const master=importFactoryCatalog(catalog);
  const fullKits=kits===null?null:importFactoryDrumRecords(kits,master);
  const kitMap=new Map((fullKits?.records??[]).map(p=>[
    master.presets.findIndex(x=>x.engine===p.engine&&x.enginePresetIndex===p.enginePresetIndex),p.recordBytes]));
  return master.presets.map((p,i)=>{
    if(p.requiresEngineFactoryKit&&!kitMap.has(i))return {
      id:"F"+p.factoryIndex,name:p.name,engine:p.engine,
      available:false,issue:"Complete drum-kit sidecar unavailable"};
    const recordBytes=p.requiresEngineFactoryKit?kitMap.get(i):Array.from(encodeRecord(p));
    return {id:"F"+p.factoryIndex,name:p.name,engine:p.engine,
      available:true,recordBytes};
  });
}
/* Sources can be any valid saved 304-byte LIBRARY override, including a
 * snapshot from a different session, or exact factory records. No unsupported
 * USER-to-LIBRARY conversion (FM6 and drum kits have separate companions). */
export function snapshotSourceRecords(snapshot){
  return inspectEmulatorFlash(snapshot).library.filter(x=>x.status==="overridden")
    .map(x=>({id:"L"+x.slot,name:x.name,engine:x.engine,available:true,
      recordBytes:x.recordBytes}));
}
/* Changes are staged as a pure transformation of one validated target JSON.
 * For each affected bank, write the *other* A/B sector and its valid commit
 * header. Preserve the previous committed copy for recovery. No merge from
 * unrelated snapshots, and all other USER/project/settings bytes are exact.
 *
 * edits: [{slot: 33..64, recordBytes: 304-byte array|null}]
 * null clears only the override -> restores the firmware factory fallback.
 */
export function editEmulatorLibrarySnapshot(snapshot,edits){
  check(Array.isArray(edits)&&edits.length>0&&edits.length<=32,
    "Provide 1..32 LIBRARY changes");
  const before=inspectEmulatorFlash(snapshot);
  const sectors=decodeEmulatorFlash(snapshot);
  const seen=new Set(),grouped=new Map(),changed=[];
  for(const edit of edits){
    const slot=edit?.slot;
    check(Number.isInteger(slot)&&slot>=33&&slot<=64&&!seen.has(slot),
      "Invalid or repeated LIBRARY slot "+slot);
    seen.add(slot);
    const record=edit.recordBytes===null?new Uint8Array(304):
      Uint8Array.from(decodeRecord(edit.recordBytes).recordBytes);
    const index=slot-33,bank=Math.floor(index/8);
    if(!grouped.has(bank))grouped.set(bank,[]);
    grouped.get(bank).push({index:index%8,slot,record});
    const prior=before.library[index];
    changed.push({slot,previous:prior.status==="factory"?"Factory default":prior.name,
      next:edit.recordBytes===null?"Factory default":decodeRecord(record).name,
      referencedBy:before.projects.sections.filter(Boolean).length});
  }
  const out={...sectors};
  for(const [bank,items] of grouped){
    const {base,current}=bankState(sectors,bank);
    const payload=current?current.bytes.slice(PAYLOAD,PAYLOAD+BANK_BYTES):new Uint8Array(BANK_BYTES);
    for(const item of items)payload.set(item.record,item.index*304);
    const nextCopy=current?.copy===0?1:0;
    const seq=current?((current.seq+1)>>>0):1;
    const page=new Uint8Array(SECTOR).fill(255);
    put32(page,0,0x3142494c);put32(page,4,1);put32(page,8,bank);
    put32(page,12,seq);put32(page,16,BANK_BYTES);put32(page,20,crc32(payload));
    page.set(payload,PAYLOAD);
    out[base+nextCopy*SECTOR]=page;
  }
  const result=encodeEmulatorFlash(out);
  const after=inspectEmulatorFlash(result);
  for(const edit of edits){
    const index=edit.slot-33;
    check(edit.recordBytes===null?after.library[index].status==="factory":
      after.library[index].status==="overridden"&&
      JSON.stringify(after.library[index].recordBytes)===JSON.stringify(edit.recordBytes),
      "Verification failed for LIBRARY slot "+edit.slot);
  }
  check(JSON.stringify(after.projects)===JSON.stringify(before.projects),
    "Project references or settings changed unexpectedly");
  check(JSON.stringify(after.users)===JSON.stringify(before.users),
    "USER preset inspection changed unexpectedly");
  // Exact byte identity for all sectors outside the affected LIBRARY banks.
  const changedOffsets=new Set([...grouped.keys()].flatMap(bank=>[
    LIB_BASE+bank*8192,LIB_BASE+bank*8192+4096]));
  for(const [off,bytes] of Object.entries(sectors))
    if(!changedOffsets.has(Number(off)))
      check(JSON.stringify(Array.from(out[off]))===JSON.stringify(Array.from(bytes)),
        "Unrelated NOR sector changed at "+off);
  return {snapshot:result,changes:changed,inspection:after};
}

/* Source USER sounds from exported flash. Exact 192-byte record is joined
 * to its independently committed UPF6 112-byte companion only when the
 * UPF6 per-record tag still matches the record's content (except its name).
 * This supports edited DRUM kits and FM6 patches without inventing bytes.
 * Records missing required companions remain visible but nontransferable.
 */
export function userSourceRecords(snapshot){
  const view=inspectEmulatorFlash(snapshot),sectors=decodeEmulatorFlash(snapshot);
  const UPF_A=0x9f000,UPF_B=0xfe000;
  function readUpfSector(off,copy){
    const data=sector(sectors,off);
    if(u32(data,0)!==0x554c4546||
       (data[4]|data[5]<<8)!==8||
       (data[6]|data[7]<<8)!==copy||
       u32(data,12)!==3728||
       u32(data,28)!==crc32(data.slice(0,28))||
       u32(data,16)!==crc32(data.slice(256,256+3728)))return null;
    const body=data.slice(256,256+3728);
    if(u32(body,0)!==0x36465055||
       (body[4]|body[5]<<8)!==1||
       (body[6]|body[7]<<8)!==32)return null;
    return {sequence:u32(data,8),data:body};
  }
  const a=readUpfSector(UPF_A,0),b=readUpfSector(UPF_B,1);
  const upf=b&&(!a||newer(a.sequence,b.sequence))?b:a;
  const companionFor=(slot,raw)=>{
    const idx=slot-1,body=upf?.data;
    if(!body||!(u32(body,8)&(1<<idx)))return null;
    let tag=2166136261;
    for(let i=0;i<raw.length;i++){
      if(i>=4&&i<16)continue; // changing the name cannot detach a patch
      tag=Math.imul(tag^raw[i],16777619)>>>0;
    }
    const off=16+idx*116;
    if(u32(body,off)!==tag)return null;
    return body.slice(off+4,off+116);
  };
  return view.users.filter(x=>x.status==="saved").map(user=>{
    const raw=Uint8Array.from(user.recordBytes);
    const needsCompanion=[10,12,15,19,20].includes(user.engine);
    const extra=companionFor(user.slot,raw);
    const complete=!needsCompanion||(
      !!extra&&(user.engine===12||extra[111]===0xd3));
    const result={id:"U"+user.slot,name:user.name,engine:user.engine,
      available:complete,
      ...(!complete?{issue:"USER sound needs its matching FM6/drum companion"}:{})};
    if(complete){
      const bytes=new Uint8Array(304);
      bytes.set(raw);
      if(extra)bytes.set(extra,192);
      result.recordBytes=Array.from(decodeRecord(bytes).recordBytes);
    }
    return result;
  });
}
