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

/* Selective USER destination copy. Unlike LIBRARY, USER records live in
 * two ordinary ST/UPB1 A/B banks (16 × 192 bytes), and their matching
 * FM6/drum sidecars live in the distinct UPF6 A/B object. Update both only
 * inside an exported full emulator JSON; never write device or browser.
 */
function readStCandidate(sectors,off,copy,type,expectedLength){
  const d=sector(sectors,off);
  if(u32(d,0)!==0x554c4546||
     (d[4]|d[5]<<8)!==type||
     (d[6]|d[7]<<8)!==copy||
     u32(d,12)!==expectedLength||
     u32(d,28)!==crc32(d.slice(0,28)))return null;
  const data=d.slice(256,256+expectedLength);
  if(u32(d,16)!==crc32(data))return null;
  return {copy,sequence:u32(d,8),data};
}
function newerRecord(a,b){
  return !a?b:!b?a:newer(a.sequence,b.sequence)?b:a;
}
function stState(sectors,addresses,type,expectedLength,label){
  const a=readStCandidate(sectors,addresses[0],0,type,expectedLength);
  const b=readStCandidate(sectors,addresses[1],1,type,expectedLength);
  const current=newerRecord(a,b);
  check(current||addresses.every(off=>isErased(sector(sectors,off))),
        label+" contains unknown or damaged stored data");
  return current;
}
function stCommitted(type,copy,seq,data){
  check(data.length<=3840,"ST payload too large");
  const out=new Uint8Array(SECTOR).fill(255);
  put32(out,0,0x554c4546);
  out[4]=type&255;out[5]=type>>8;
  out[6]=copy;out[7]=0;
  put32(out,8,seq);put32(out,12,data.length);
  put32(out,16,crc32(data));put32(out,20,0xffffffff);
  put32(out,24,0xffffffff);put32(out,28,crc32(out.slice(0,28)));
  out.set(data,256);return out;
}
function userTag(raw){
  let h=2166136261;
  for(let i=0;i<192;i++)if(i<4||i>=16)
    h=Math.imul(h^raw[i],16777619)>>>0;
  return h;
}
function getUpfState(sectors){
  const addresses=[0x9f000,0xfe000];
  const a=readStCandidate(sectors,addresses[0],0,8,3728);
  const b=readStCandidate(sectors,addresses[1],1,8,3728);
  const current=newerRecord(a,b);
  if(current)check(u32(current.data,0)===0x36465055&&
    (current.data[4]|current.data[5]<<8)===1&&
    (current.data[6]|current.data[7]<<8)===32,
    "UPF6 companion bank has an invalid header");
  if(!current){
    const retired=[0,1].map(i=>
      readStCandidate(sectors,addresses[i],i,7,3472));
    const available=addresses.map((off,i)=>isErased(sector(sectors,off))?i:-1).filter(x=>x>=0);
    check(available.length>0&&
      addresses.every((off,i)=>isErased(sector(sectors,off))||retired[i]),
      "UPF6 companion sectors are foreign or damaged");
    const data=new Uint8Array(3728);
    put32(data,0,0x36465055);
    data[4]=1;data[6]=32;
    return {data,current:null,copy:available[0],addresses};
  }
  return {data:current.data.slice(),current,
    copy:current.copy===0?1:0,addresses};
}
export function editEmulatorUserSnapshot(snapshot,edits){
  check(Array.isArray(edits)&&edits.length>0&&edits.length<=32,
    "Provide 1..32 USER changes");
  const before=inspectEmulatorFlash(snapshot);
  const sectors=decodeEmulatorFlash(snapshot),out={...sectors};
  const prepared=[],seen=new Set(),banks=new Map();
  for(const edit of edits){
    const slot=edit?.slot;
    check(Number.isInteger(slot)&&slot>=1&&slot<=32&&!seen.has(slot),
      "Invalid or repeated USER slot "+slot);
    seen.add(slot);
    const rec=edit.recordBytes===null?new Uint8Array(304):
      Uint8Array.from(decodeRecord(edit.recordBytes).recordBytes);
    const index=slot-1,bank=Math.floor(index/16);
    if(!banks.has(bank))banks.set(bank,[]);
    banks.get(bank).push({slot,index:index%16,rec});
    prepared.push({slot,previous:before.users[index].name||"Empty",
      next:edit.recordBytes===null?"Empty":decodeRecord(rec).name});
  }
  const changedOffsets=new Set();
  for(const [bank,items] of banks){
    const addresses=[0xdc000+bank*8192,0xdd000+bank*8192];
    const current=stState(sectors,addresses,5+bank,3080,"USER bank "+bank);
    const payload=current?current.data.slice():new Uint8Array(3080);
    if(!current){
      put32(payload,0,0x31425055);
      payload[4]=192;payload[6]=16;
    }
    check(u32(payload,0)===0x31425055&&
      (payload[4]|payload[5]<<8)===192&&
      (payload[6]|payload[7]<<8)===16,"Invalid USER bank payload");
    for(const {index,rec} of items)payload.set(rec.slice(0,192),8+index*192);
    const copy=current?.copy===0?1:0;
    out[addresses[copy]]=stCommitted(5+bank,copy,current?((current.sequence+1)>>>0):1,payload);
    changedOffsets.add(addresses[copy]);
  }
  // Rebuild only companion entries for changed USER slots. All 32 other
  // tags and independent sound companions remain byte-for-byte unchanged.
  const upf=getUpfState(sectors);
  let used=u32(upf.data,8);
  for(const {slot,rec} of [...banks.values()].flat()){
    const index=slot-1,offset=16+index*116;
    upf.data.fill(0,offset,offset+116);
    if(rec[0]===0xa5&&[10,12,15,19,20].includes(rec[2])){
      check(rec[2]===12||rec[303]===0xd3,"Missing complete USER drum-kit companion");
      put32(upf.data,offset,userTag(rec.slice(0,192)));
      upf.data.set(rec.slice(192,304),offset+4);
      used=(used|(1<<index))>>>0;
    }else used=(used&(~(1<<index)))>>>0;
  }
  put32(upf.data,8,used);
  out[upf.addresses[upf.copy]]=stCommitted(8,upf.copy,
    upf.current?((upf.current.sequence+1)>>>0):1,upf.data);
  changedOffsets.add(upf.addresses[upf.copy]);
  const exported=encodeEmulatorFlash(out);
  const after=inspectEmulatorFlash(exported);
  const sources=userSourceRecords(exported);
  for(const {slot,recordBytes} of edits){
    const read=after.users[slot-1];
    if(recordBytes===null)check(read.status==="empty","USER clear did not persist");
    else{
      const from=sources.find(x=>x.id==="U"+slot);
      check(read.status==="saved"&&from?.available&&
            JSON.stringify(from.recordBytes)===JSON.stringify(recordBytes),
        "USER "+slot+" read-back differs from requested full sound");
    }
  }
  check(JSON.stringify(after.projects)===JSON.stringify(before.projects),
    "Editing USER sounds unexpectedly changed project references");
  check(JSON.stringify(after.library)===JSON.stringify(before.library),
    "Editing USER sounds unexpectedly changed LIBRARY presets");
  for(const [off,bytes] of Object.entries(sectors))
    if(!changedOffsets.has(Number(off)))
      check(JSON.stringify(Array.from(out[off]))===JSON.stringify(Array.from(bytes)),
        "Editing USER unexpectedly changed NOR sector "+off);
  return {snapshot:exported,changes:prepared,inspection:after};
}
