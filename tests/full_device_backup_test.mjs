/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {bkCrc,bkPack,bkUnpack,bkU32,bkR32} from "../web/fm1backup.js";
import {encodeProject,decodeProject,MAX_RECORD} from "../web/projects-library-codec.mjs";
import {encodeRecord,pack7,unpack7} from "../web/presets-library-codec.mjs";
import {captureUser} from "../web/presets-library-sync.mjs";
import {validateFullDevice,projectCapability,restoreProjectsOnly,restoreFullDevice,probeFlash,captureFullDevice,captureProjectsOnly,planLibraryChanges,applyVerifiedLibraryChanges,planUserRestore,applyVerifiedUserRestore,planUserBatch,applyVerifiedUserBatch,proposeUserImport,planProjectRestore,applyVerifiedProjectRestore,planFullRestore,applyVerifiedFullRestore} from "../web/full-device-backup.mjs";
const preset=Array.from(encodeRecord({engine:0,name:"SOUND 1",
  params:Array.from({length:127},(_,i)=>i%70)}));
const current=encodeProject({section:0,refs:[32,33,255,255,255,255],
  name:"CURRENT",settingsBytes:[1,2,3,240]});
const section=encodeProject({section:1,refs:[32,33,255,255,255,255],
  name:"SECTION B",settingsBytes:[44,128,254]});
const pad=x=>{const a=Uint8Array.from(x);const out=new Uint8Array(MAX_RECORD);out.set(a);return out;};
const raw=bytes=>({size:bytes.length,crc:bkCrc(bytes),data:Buffer.from(bytes).toString("base64")});
const ids=[0,1,2,3,4,5,6,7,8,9,10,...Array.from({length:32},(_,i)=>40+i)];
const objects=ids.map(id=>{
  const d=id===0?pad(current):id===1?Uint8Array.from([1,2,3,4]):
          id===10?Uint8Array.from([83,79,78,71,2,0,0,0]):
          (id===3||id===41)?pad(section):new Uint8Array(0);
  return {id,...raw(d)};
});
const base={format:"mashina-full-device",version:1,
  archive:{format:"felucca-backup",version:1,objects},
  userPresets:Array(32).fill(null),
  libraryPresets:Array(32).fill(null),
  projects:Array(32).fill(null),
  currentPerformance:current
};
base.libraryPresets[0]=preset;
base.projects[1]=section;
const valid=validateFullDevice(base);
assert.equal(valid.format,"mashina-full-device");
assert.deepEqual(valid.projects[1],section);
assert.deepEqual(valid.currentPerformance,current);
assert.equal(valid.archive.objects.length,43);
assert.equal(valid.selectedProject,1);
const withUser=structuredClone(base);
const storedUserBank=new Uint8Array(3080);
storedUserBank.set([0x55,0x50,0x42,0x31,192,0,16,0]); // "UPB1" / 192 / 16
storedUserBank.set(preset.slice(0,192),8);
withUser.userPresets[0]=preset;
withUser.archive.objects[6]={id:6,...raw(storedUserBank)};
assert.deepEqual(validateFullDevice(withUser).userPresets[0],preset);
const changedBank=structuredClone(withUser);
const mismatched=storedUserBank.slice();
mismatched[8+17]^=1;
changedBank.archive.objects[6]={id:6,...raw(mismatched)};
assert.throws(()=>validateFullDevice(changedBank),/USER 1 differs/);

assert.throws(()=>validateFullDevice({...base,projects:Array(32).fill(null)}),/disagree/);
const inconsistent=structuredClone(base);
inconsistent.archive.objects[3]={id:3,...raw(new Uint8Array(0))};
assert.throws(()=>validateFullDevice(inconsistent),/differs between archive aliases/);
assert.throws(()=>validateFullDevice({...base,currentPerformance:section}),/disagree/);
const corrupt=structuredClone(base);corrupt.archive.objects[11].crc=25;
assert.throws(()=>validateFullDevice(corrupt),/checksum/);
const tampered=structuredClone(base);tampered.archive.objects[0].size=33;
assert.throws(()=>validateFullDevice(tampered),/backup object|checksum|MPR1/);
let changed=0,notes=[];
async function request(cmd,args){
  if(cmd===95)return [1,32,1];
  if(cmd===94)return [args[0],1];
  if(cmd===67){changed++;notes.push(args.slice(0,2));return [args[0],args[1],0];}
  throw new Error("unexpected mock command "+cmd);
}
assert.deepEqual(await projectCapability(request),
  {version:1,sections:32,writable:true,canInitialize:false});
assert.deepEqual(await projectCapability(async()=>[1,32,1,1]),
  {version:1,sections:32,writable:true,canInitialize:true});
const probe=await probeFlash(async(cmd)=>cmd===97?[16,...Array(16).fill(2)]:
                                                  [4,...Array(4).fill(0)]);
assert.equal(probe.canInitializeProjects,true);
assert.equal(probe.libraryAllBlank,true);
assert.equal(probe.unknown,false);
const alien=await probeFlash(async(cmd)=>cmd===97?[16,3,...Array(15).fill(0)]:
                                                  [4,...Array(4).fill(0)]);
assert.equal(alien.canInitializeProjects,false);
assert.equal(alien.unknown,true);
await assert.rejects(probeFlash(async()=>[0]),/diagnostics/);
let readOnlyWrites=0;
const readOnly=async(cmd,args)=>{
  if(cmd===67)readOnlyWrites++;
  return cmd===95?[1,32,0]:[args?.[0]??0,1];
};
await assert.rejects(restoreProjectsOnly(readOnly,
  {format:"mashina-projects-mpr1",version:1,projects:8,sectionsPerProject:4,
   sections:base.projects}),/read-only/);
assert.equal(readOnlyWrites,0);
await assert.rejects(restoreFullDevice(async(cmd)=>cmd===89?[1,32,1]:cmd===95?[1,32,0]:[],base),
  /does not allow/);
/* End-to-end mock WebMIDI service: exercise the exact chunked BACKUP and
 * full LIBRARY protocols in the same order as a real FM1 restore, including
 * read-after-write validation and empty-position clearing. */
const mockObjects=new Map(objects.map(x=>[x.id,Uint8Array.from(Buffer.from(x.data,"base64"))]));
const mockLib=new Map([[0,Uint8Array.from(preset)]]);
let staged=null,sets=0,erasures=0,archiveGets=0;
async function mock(cmd,args=[],opts={}){
  if(cmd===89)return [1,32,1];
  if(cmd===95)return [1,32,1,0];
  if(cmd===94){
    const slot=args[0], bank=mockObjects.get(6+Math.floor(slot/16));
    if(!bank?.length)return [slot,1];
    assert.equal(bank.length,3080);
    const off=8+(slot%16)*192,head=bank.slice(off,off+192);
    if(head[0]!==0xa5)return [slot,1];
    const rec=new Uint8Array(304);rec.set(head);
    if([10,12,15,19,20].includes(rec[2])){
      const extra=mockObjects.get(9);
      if(!extra?.length||extra.length!==3728)return [slot,1];
      const mask=(extra[8]|extra[9]<<8|extra[10]<<16|extra[11]<<24)>>>0;
      if(!(mask&(1<<slot)))return [slot,1];
      const start=16+slot*116;
      let tag=2166136261;
      for(let i=0;i<192;i++)
        if(i<4||i>=16)tag=Math.imul(tag^head[i],16777619)>>>0;
      const expected=(extra[start]|extra[start+1]<<8|
        extra[start+2]<<16|extra[start+3]<<24)>>>0;
      if(tag!==expected)return [slot,1];
      rec.set(extra.slice(start+4,start+116),192);
    }
    return [slot,0,...pack7(rec)];
  }
  if(cmd===91) {
    const v=mockLib.get(args[0]);
    return v?[args[0],0,...pack7(v)]:[args[0],3];
  }
  if(cmd===92){
    mockLib.set(args[0],Uint8Array.from(unpack7(args.slice(1))));sets++;
    return [args[0],0];
  }
  if(cmd===93){mockLib.delete(args[0]);erasures++;return [args[0],0];}
  if(cmd===65){
    return [1,0,43,...ids.flatMap(id=>{
      const d=mockObjects.get(id);
      return [id,...bkU32(d.length),...bkU32(bkCrc(d))];
    })];
  }
  if(cmd===66){
    const id=args[0],off=bkR32(args,1),size=args[6]|args[7]<<7;
    const data=mockObjects.get(id);
    archiveGets++;
    assert.ok(data&&off+size<=data.length);
    return [id,0,...bkU32(off),size&127,size>>7,...bkPack(data.slice(off,off+size))];
  }
  if(cmd===67){
    const phase=args[0],id=args[1];
    if(phase===0){
      const size=bkR32(args,2),crc=bkR32(args,7);
      staged={id,size,crc,bytes:new Uint8Array(size)};return [phase,id,0];
    }
    if(phase===1){
      assert.equal(staged?.id,id);
      const off=bkR32(args,2),size=Math.min(256,staged.size-off);
      staged.bytes.set(bkUnpack(args.slice(7),size),off);
      return [phase,id,0];
    }
    if(phase===2){
      assert.equal(staged?.id,id);
      assert.equal(bkCrc(staged.bytes),staged.crc);
      mockObjects.set(id,staged.bytes);
      staged=null;return [phase,id,0];
    }
    if(phase===3){staged=null;return [phase,id,0];}
  }
  throw new Error("mock received unknown command "+cmd);
}
// Projects-only export must not silently return a self-inconsistent archive.
const projectExport=await captureProjectsOnly(mock,"MPR1 MOCK");
assert.equal(projectExport.sections.length,32);
assert.deepEqual(projectExport.sections[1],section);
const beforeBadAlias=mockObjects.get(3).slice();
mockObjects.set(3,pad(encodeProject({section:1,refs:[33,255,255,255,255,255],
  name:"BAD ALIAS",settingsBytes:[7]})));
await assert.rejects(captureProjectsOnly(mock,"MPR1 MOCK"),
  /differs between archive aliases/);
mockObjects.set(3,beforeBadAlias);
// Reject a corrupt occupied LIBRARY slot before changing any project.
// The imported section references LIBRARY 033 and 034 (refs 32 and 33).
let preflightWrites=0;
const brokenLibrary=async(cmd,args=[],opts={})=>{
  if(cmd===91&&args[0]===0)return [0,2]; // damaged occupied LIBRARY record
  if(cmd===67)preflightWrites++;
  return mock(cmd,args,opts);
};
const projectPreflightFile={format:"mashina-projects-mpr1",version:1,
  projects:8,sectionsPerProject:4,sections:base.projects};
await assert.rejects(restoreProjectsOnly(brokenLibrary,projectPreflightFile),
  /flash\/data error/);
assert.equal(preflightWrites,0);
// Seed a stale section that the imported JSON marks empty. The restore must
// actually clear it, not merely skip its write or report it as cleared.
mockObjects.set(42,pad(encodeProject({section:2,refs:[32,255,255,255,255,255],
  name:"STALE",settingsBytes:[91,92]})));
// Project-only restore now verifies all 32 sections by reading flash back.
const projectFile={format:"mashina-projects-mpr1",version:1,projects:8,
  sectionsPerProject:4,sections:base.projects};
let projectProgress=0;
const projectResult=await restoreProjectsOnly(mock,projectFile,
  ()=>projectProgress++);
assert.equal(projectResult.restored,32);
assert.ok(projectProgress>32); // includes read-back progress
assert.deepEqual(Array.from(mockObjects.get(41)),Array.from(pad(section)));
assert.equal(mockObjects.get(40).length,0);
assert.equal(mockObjects.get(42).length,0); // stale section was actively cleared
// A stale selected-project alias must also fail the post-restore check.
const originalAlias=mockObjects.get(3).slice();
mockObjects.set(3,pad(encodeProject({section:1,refs:[33,255,255,255,255,255],
  name:"STALE ALIAS",settingsBytes:[7]})));
await assert.rejects(restoreProjectsOnly(mock,projectFile),
  /differs between archive aliases/);
mockObjects.set(3,originalAlias);
// Simulate an acknowledged section commit that silently loses its record.
let corruptProjectOnCommit=true;
const projectCorruptingMock=async(cmd,args=[],opts={})=>{
  const reply=await mock(cmd,args,opts);
  if(corruptProjectOnCommit&&cmd===67&&args[0]===2&&args[1]===41){
    mockObjects.set(41,new Uint8Array(0));
    corruptProjectOnCommit=false;
  }
  return reply;
};
await assert.rejects(restoreProjectsOnly(projectCorruptingMock,projectFile),
  /Project-only restore verification failed/);
assert.equal(corruptProjectOnCommit,false);
// Recover the test device before exercising the full-device archive flow.
await restoreProjectsOnly(mock,projectFile);
const collected=await captureFullDevice(mock,"MPR1 MOCK");
assert.equal(collected.format,"mashina-full-device");
assert.equal(collected.libraryPresets[0]?.length,304);
assert.equal(collected.projects[1]?.length,section.length);
assert.ok(archiveGets>=12); // sparse archive: only nonempty objects have GET chunks
const back=await restoreFullDevice(mock,collected);
assert.equal(back.restoredProjects,32);
assert.equal(back.userSlots,32);
assert.equal(back.librarySlots,32);
assert.equal(erasures,31);
assert.equal(sets,1);
assert.deepEqual(Array.from(mockLib.get(0)),preset);
assert.equal(staged,null);
// Simulate a device that acknowledges the global-settings write but persists
// different bytes. The follow-up backup must catch this silent corruption.
let corruptSettingsOnCommit=true;
const corruptingMock=async(cmd,args=[],opts={})=>{
  const reply=await mock(cmd,args,opts);
  if(corruptSettingsOnCommit&&cmd===67&&args[0]===2&&args[1]===1){
    const damaged=mockObjects.get(1).slice();
    damaged[0]^=1;
    mockObjects.set(1,damaged);
    corruptSettingsOnCommit=false;
  }
  return reply;
};
await assert.rejects(restoreFullDevice(corruptingMock,collected),
  /Global settings verification failed/);
assert.equal(corruptSettingsOnCommit,false);
/* Full 32+32 mock transaction with nine distinct, COMPLETE original-style
 * drum records, including UPF6 sidecars, to exercise the actual 43-object
 * archive plus separate USER sound validation and read-after-write recovery.
 * The owner's personal source JSON is NOT committed to this repository. */
{
  const slots=[0,1,2,4,5,6,8,9,10];
  const engines=[19,19,19,15,15,15,10,10,10];
  const bank=new Uint8Array(3080);
  bank.set([0x55,0x50,0x42,0x31,192,0,16,0]);
  const upf=new Uint8Array(3728);
  upf.set([0x55,0x50,0x46,0x36,1,0,32,0]);
  const saved=[];
  const write32=(dst,off,n)=>{
    for(let i=0;i<4;i++)dst[off+i]=(n>>>(8*i))&255;
  };
  let mask=0;
  for(let i=0;i<9;i++){
    const slot=slots[i],rec=Uint8Array.from(preset);
    rec[2]=engines[i];rec[3]=127;rec.fill(0,4,16);
    rec.set(Buffer.from("KIT-"+String(slot+1).padStart(2,"0")),4);
    rec[20]=68+i;rec.fill(0,192);
    for(let k=192;k<303;k++)rec[k]=(k*5+i*37)&255;
    rec[303]=0xd3;
    bank.set(rec.slice(0,192),8+slot*192);
    let tag=2166136261;
    for(let k=0;k<192;k++)
      if(k<4||k>=16)tag=Math.imul(tag^rec[k],16777619)>>>0;
    const at=16+slot*116;
    write32(upf,at,tag);upf.set(rec.slice(192),at+4);
    saved.push(Array.from(rec));mask|=1<<slot;
  }
  write32(upf,8,mask);
  mockObjects.set(6,bank);
  mockObjects.set(9,upf);
  const nine=await captureFullDevice(mock,"9 KIT MIGRATION SIMULATOR");
  assert.deepEqual(nine.userPresets.filter(Boolean),saved);
  assert.equal(nine.archive.objects.length,43);
  assert.equal(nine.libraryPresets.filter(Boolean).length,1);
  assert.equal(nine.projects.length,32);
  assert.equal(nine.selectedProject,1);
  const stamped=validateFullDevice(nine);
  assert.deepEqual(stamped.userPresets.filter(Boolean),saved);
  /* All 43 objects' target bytes, including USER and companion banks, are
   * erased before the restore to simulate a first-install clean slate.
   * Initial MPR1/current references are reloaded from the verified archive. */
  for(const id of [6,7,9])mockObjects.set(id,new Uint8Array(0));
  mockLib.clear();
  assert.equal((await captureUser(mock)).length,0);
  const restored=await restoreFullDevice(mock,nine);
  assert.equal(restored.restoredProjects,32);
  const readBack=await captureFullDevice(mock,"9 KIT MIGRATION SIMULATOR");
  assert.deepEqual(readBack.userPresets.filter(Boolean),saved,
    "nine original-style drum records lost or aliased on full archive restore");
  assert.deepEqual(readBack.libraryPresets,nine.libraryPresets);
  assert.deepEqual(readBack.projects,nine.projects);
  assert.deepEqual(readBack.currentPerformance,nine.currentPerformance);
  for(const id of [1,6,7,9,10]){
    const expected=nine.archive.objects.find(x=>x.id===id);
    const got=readBack.archive.objects.find(x=>x.id===id);
    assert.deepEqual(got.bytes,expected.bytes,
      "important complete source archive object "+id+" failed restore");
  }
  /* A silent companion change must prevent a false-success restore.
   * Strict archive and user sound verification are separate gates. */
  let corrupted=false;
  const broken=async(cmd,args=[],opts={})=>{
    const result=await mock(cmd,args,opts);
    if(cmd===67&&args[0]===2&&args[1]===9&&!corrupted){
      mockObjects.get(9)[300]^=1;corrupted=true;
    }
    return result;
  };
  await assert.rejects(restoreFullDevice(broken,nine),
    /USER or FM6\/drum-kit sound verification failed|checksum|capture|mismatch|Invalid/);
  assert.equal(corrupted,true);
  await restoreFullDevice(mock,nine);
  console.log("PASS: nine complete custom-style drum USERs -> 43-object MPR1 full-device backup -> blank destination -> restore -> archive+sound+project readback and corruption refusal");
}
console.log("PASS: full-device MPR1 validation, strict archive equality, 32-section chunked project restore and write gates");


/* Exercise the real selected-slot path against the same complete mock.
 * Saved and live references are both hard barriers; stale backups and
 * unexpected collateral changes cannot produce a false success. */
{
  const baseline=await captureFullDevice(mock,"INITIALIZED UPDATE");
  const counts=[sets,erasures];
  assert.throws(()=>planLibraryChanges(baseline,[{slot:0,recordBytes:null}]),/referenced/);
  assert.throws(()=>planLibraryChanges(baseline,[{slot:1,recordBytes:preset}]),/referenced/);
  const liveOnly=structuredClone(baseline);
  const live=encodeProject({section:0,refs:[37,255,255,255,255,255],name:"LIVE",settingsBytes:[1]});
  liveOnly.currentPerformance=live;liveOnly.archive.objects[0]={id:0,...raw(pad(live))};
  assert.throws(()=>planLibraryChanges(liveOnly,[{slot:5,recordBytes:preset}]),/Current performance/);
  assert.deepEqual([sets,erasures],counts);
  const stale=structuredClone(baseline);
  stale.libraryPresets[6]=preset;
  await assert.rejects(applyVerifiedLibraryChanges(mock,stale,[{slot:5,recordBytes:preset}]),/Device changed/);
  assert.deepEqual([sets,erasures],counts);
  await assert.rejects(applyVerifiedLibraryChanges(async(cmd,args,opts)=>cmd===95?[1,32,1,1]:mock(cmd,args,opts),
    baseline,[{slot:5,recordBytes:preset}]),/formatting disabled/);
  assert.deepEqual([sets,erasures],counts);
  const result=await applyVerifiedLibraryChanges(mock,baseline,[{slot:5,recordBytes:preset}]);
  assert.equal(result.changed,1);
  assert.deepEqual(result.after.libraryPresets[5],preset);
  assert.deepEqual([sets,erasures],[counts[0]+1,counts[1]]);
  const noop=await applyVerifiedLibraryChanges(mock,result.after,[{slot:5,recordBytes:preset}]);
  assert.equal(noop.changed,0);
  const reset=await applyVerifiedLibraryChanges(mock,result.after,[{slot:5,recordBytes:null}]);
  assert.equal(reset.after.libraryPresets[5],null);
  assert.deepEqual(reset.after.libraryPresets,baseline.libraryPresets);
  assert.deepEqual([sets,erasures],[counts[0]+1,counts[1]+1]);
  const collateral=async(cmd,args,opts)=>{
    const reply=await mock(cmd,args,opts);
    if(cmd===92)mockObjects.get(1)[0]^=1;
    return reply;
  };
  await assert.rejects(applyVerifiedLibraryChanges(collateral,reset.after,[{slot:5,recordBytes:preset}]),
    /post-write full-state verification failed/);
  console.log("PASS: selected-slot physical protocol mock — reference, stale backup, format capability, no-op, put, reset, collateral damage checks");
}


// Bulk: a blocked final destination must prevent even the first PUT;
// unchanged entries are skipped; interrupted batches are explicitly partial.
{
  const before=await captureFullDevice(mock,"BATCH");
  const freshSlot=7,another=8,counts=[sets,erasures];
  const record2=Array.from(encodeRecord({engine:0,name:"BATCH TWO",params:Array(127).fill(45)}));
  await assert.rejects(applyVerifiedLibraryChanges(mock,before,[
    {slot:freshSlot,recordBytes:preset},{slot:1,recordBytes:record2}]),/referenced/);
  assert.deepEqual([sets,erasures],counts);
  await assert.rejects(applyVerifiedLibraryChanges(mock,before,[
    {slot:freshSlot,recordBytes:preset},{slot:freshSlot,recordBytes:record2}]),/Duplicate/);
  assert.deepEqual([sets,erasures],counts);
  const batch=await applyVerifiedLibraryChanges(mock,before,[
    {slot:0,recordBytes:before.libraryPresets[0]},
    {slot:freshSlot,recordBytes:preset},{slot:another,recordBytes:record2}]);
  assert.equal(batch.changed,2);
  assert.deepEqual(batch.after.libraryPresets[freshSlot],preset);
  assert.deepEqual(batch.after.libraryPresets[another],record2);
  assert.equal(sets,counts[0]+2);
  const failSecond=async(cmd,args,opts)=>{
    if(cmd===92&&args[0]===10)return [10,4];
    return mock(cmd,args,opts);
  };
  const c=sets;
  await assert.rejects(applyVerifiedLibraryChanges(failSecond,batch.after,[
    {slot:9,recordBytes:preset},{slot:10,recordBytes:record2},{slot:11,recordBytes:preset}]),
    /LIBRARY 43 failed after 1 of 3 verified changes/);
  assert.equal(sets,c+1);
  assert.equal(mockLib.has(11),false);
  const badLive=structuredClone(before);
  const live=encodeProject({section:0,refs:[31,255,255,255,255,255],name:"MISSING",settingsBytes:[1]});
  badLive.currentPerformance=live;badLive.archive.objects[0]={id:0,...raw(pad(live))};
  assert.throws(()=>validateFullDevice(badLive),/missing USER preset/);
  const legacy=structuredClone(before);
  legacy.archive.objects[8]={id:8,...raw(Uint8Array.of(1))};
  assert.throws(()=>validateFullDevice(legacy),/legacy FM6 migration/);
  console.log("PASS: bulk preflight, unchanged slots, two writes, partial-failure stop, missing live USER and legacy restore refusal");
}


// Selective USER trial: preserve occupied records, use the exact same
// 304-byte USER source, exercise both base banks and tagged companion data.
{
  const baseline=await captureFullDevice(mock,"USER RESTORE TRIAL");
  const occupied=baseline.userPresets.findIndex(Boolean);
  const analog=Array.from(encodeRecord({engine:0,name:"USER TEST",params:Array(127).fill(17)}));
  assert.throws(()=>planUserRestore(baseline,occupied,analog),/occupied/);
  const writes=[];
  const counted=async(cmd,args,opts)=>{
    if(cmd===67&&args[0]===2)writes.push(args[1]);
    return mock(cmd,args,opts);
  };
  const stale=structuredClone(baseline);
  stale.libraryPresets[15]=analog;
  await assert.rejects(applyVerifiedUserRestore(counted,stale,15,analog),/Device changed/);
  assert.deepEqual(writes,[]);
  await assert.rejects(applyVerifiedUserRestore(async(cmd,args,opts)=>cmd===95?[1,32,1,1]:counted(cmd,args,opts),baseline,15,analog),/formatting disabled/);
  assert.deepEqual(writes,[]);
  const a=await applyVerifiedUserRestore(counted,baseline,15,analog);
  assert.deepEqual(a.after.userPresets[15],analog);
  assert.deepEqual(writes,[6]);
  const drum=baseline.userPresets.find(p=>p&&p[2]===19);
  assert.ok(drum);
  const b=await applyVerifiedUserRestore(counted,a.after,31,drum);
  assert.deepEqual(b.after.userPresets[31],drum);
  assert.deepEqual(writes,[6,9,7]);
  assert.deepEqual(b.after.projects,baseline.projects);
  assert.deepEqual(b.after.libraryPresets,baseline.libraryPresets);
  assert.deepEqual(b.after.currentPerformance,baseline.currentPerformance);
  for(let i=0;i<32;i++)if(i!==15&&i!==31)
    assert.deepEqual(b.after.userPresets[i],baseline.userPresets[i]);
  const failBase=async(cmd,args,opts)=>{
    if(cmd===67&&args[0]===0&&args[1]===7)return [0,7,4];
    return counted(cmd,args,opts);
  };
  await assert.rejects(applyVerifiedUserRestore(failBase,b.after,30,drum),/stopped on object 7 after 1 acknowledged/);
  const interrupted=await captureFullDevice(mock,"INTERRUPTED");
  assert.equal(interrupted.userPresets[30],null);
  assert.deepEqual(interrupted.userPresets,b.after.userPresets);
  assert.deepEqual(interrupted.libraryPresets,b.after.libraryPresets);
  console.log("PASS: selective empty USER restores, exact 304-byte records, both banks, companion-first interruption and unselected data preservation");
}


// Exercise the new manager's delta transactions on a mock that mirrors
// firmware-selected project aliases, including an actual mixed restore.
{
  const commitIds=[];
  async function managerMock(cmd,args=[],opts={}){
    const reply=await mock(cmd,args,opts);
    if(cmd===67&&args[0]===2&&reply[2]===0){
      const id=args[1];commitIds.push(id);
      const selected=mockObjects.get(10)[5];
      if(id===10||id>=40&&id<72){
        for(let i=0;i<4;i++)mockObjects.set(2+i,mockObjects.get(40+selected*4+i).slice());
      }
    }
    return reply;
  }
  const baseline=await captureFullDevice(managerMock,"MANAGER");
  const free=baseline.userPresets.flatMap((r,i)=>r===null?[i]:[]).filter(i=>i!==30);
  const a=free.find(i=>i<16),b=free.find(i=>i>=16);
  assert.notEqual(a,undefined);assert.notEqual(b,undefined);
  const drum=baseline.userPresets.find(r=>r&&r[2]===19);
  const analog=Array.from(encodeRecord({engine:0,name:"MANAGER ONE",params:Array(127).fill(13)}));
  const edits=[{slot:a,recordBytes:analog},{slot:b,recordBytes:drum}];
  assert.throws(()=>planUserBatch(baseline,[edits[0],edits[0]]),/Duplicate/);
  const ub=await applyVerifiedUserBatch(managerMock,baseline,edits);
  assert.deepEqual(commitIds,[9,6,7]);
  assert.deepEqual(ub.after.userPresets[a],analog);
  assert.deepEqual(ub.after.userPresets[b],drum);
  assert.deepEqual(ub.after.projects,baseline.projects);
  // Select one changed section, leave another imported change untouched.
  const target=structuredClone(ub.after.projects);
  target[3]=encodeProject({section:3,refs:[a,b,32,255,255,255],name:"NEW D",settingsBytes:[1,2,3]});
  target[4]=encodeProject({section:0,refs:[b,255,255,255,255,255],name:"NEW A",settingsBytes:[4]});
  const file={format:"mashina-projects-mpr1",version:1,projects:8,sectionsPerProject:4,sections:target};
  const plan=planProjectRestore(ub.after,file,[3]);
  assert.deepEqual(plan.objectIds,[43]);
  commitIds.length=0;
  const pr=await applyVerifiedProjectRestore(managerMock,ub.after,file,[3]);
  assert.deepEqual(commitIds,[43]);
  assert.deepEqual(pr.after.projects[3],target[3]);
  assert.deepEqual(pr.after.projects[4],ub.after.projects[4]);
  assert.deepEqual(pr.after.currentPerformance,ub.after.currentPerformance);
  // Restore target back to previous full snapshot (USER changes + project
  // deletion + a LIBRARY override removal). No unchanged banks written.
  const spare=20;
  const beforeFull=await applyVerifiedLibraryChanges(managerMock,pr.after,[{slot:spare,recordBytes:analog}]);
  const fullPlan=planFullRestore(beforeFull.after,baseline);
  assert.deepEqual(fullPlan.library.map(x=>x.slot),[spare]);
  assert.deepEqual(fullPlan.objectIds,[9,6,7,43]);
  commitIds.length=0;const eraseBefore=erasures;
  const result=await applyVerifiedFullRestore(managerMock,beforeFull.after,baseline);
  assert.deepEqual(commitIds,[9,6,7,43]);assert.equal(erasures,eraseBefore+1);
  assert.deepEqual(result.after.userPresets,baseline.userPresets);
  assert.deepEqual(result.after.libraryPresets,baseline.libraryPresets);
  assert.deepEqual(result.after.projects,baseline.projects);
  assert.deepEqual(result.after.archive.objects,baseline.archive.objects);
  commitIds.length=0;
  const noop=await applyVerifiedFullRestore(managerMock,result.after,baseline);
  assert.equal(noop.changed,0);assert.deepEqual(commitIds,[]);
  const stale=structuredClone(baseline);stale.libraryPresets[spare]=analog;
  await assert.rejects(applyVerifiedFullRestore(managerMock,stale,baseline),/Device changed/);
  assert.deepEqual(commitIds,[]);
  const missing=structuredClone(file);
  missing.sections[3]=encodeProject({section:3,refs:[a,255,255,255,255,255],name:"MISSING",settingsBytes:[1]});
  assert.throws(()=>planProjectRestore(baseline,missing,[3]),/empty USER slot/);
  console.log("PASS: manager multi-USER batch, selective project writes, aliases, delta full restore, no-op and stale backup refusal");
}


{
  const before=await captureFullDevice(mock,"REFERENCE POLICY");
  // Put a saved-only reference in a non-selected project; current still
  // references LIBRARY 033/034. Permit a saved-only change explicitly.
  const saved=structuredClone(before);
  const p=encodeProject({section:0,refs:[52,255,255,255,255,255],name:"SAVED REF",settingsBytes:[1]});
  saved.projects[4]=p;saved.archive.objects.find(o=>o.id===44).size=MAX_RECORD;
  Object.assign(saved.archive.objects.find(o=>o.id===44),raw(pad(p)));
  assert.throws(()=>planLibraryChanges(saved,[{slot:20,recordBytes:preset}]),/referenced/);
  assert.equal(planLibraryChanges(saved,[{slot:20,recordBytes:preset}],{allowSavedReferences:true}).length,1);
  assert.throws(()=>planLibraryChanges(saved,[{slot:1,recordBytes:preset}],{allowSavedReferences:true}),/Current performance/);
  const existing=before.userPresets.find(Boolean);
  const analog=Array.from(encodeRecord({engine:0,name:"IMPORT QUEUE",params:Array(127).fill(12)}));
  const occupied=before.userPresets.findIndex(Boolean);
  const q=proposeUserImport(before,[{slot:occupied,recordBytes:existing},{slot:occupied,recordBytes:analog}]);
  assert.equal(q.skipped.length,1);assert.equal(q.changes.length,1);
  assert.equal(before.userPresets[q.changes[0].slot],null);
  console.log("PASS: intentional saved reference changes, live protection, automatic safe USER import queue");
}


{
  const before=await captureFullDevice(mock,"DETACH POLICY");
  const detached=structuredClone(before);
  const old=decodeProject(before.currentPerformance);
  detached.currentPerformance=encodeProject({...old,section:1});
  Object.assign(detached.archive.objects.find(o=>o.id===0),raw(pad(detached.currentPerformance)));
  const active=(detached.selectedProject-1)*4+1;
  const sections=structuredClone(detached.projects);sections[active]=null;
  const file={format:"mashina-projects-mpr1",version:1,projects:8,sectionsPerProject:4,sections};
  const plan=planProjectRestore(detached,file,[active]);
  assert.equal(plan.detached,true);
  assert.deepEqual(plan.expected.currentPerformance,before.currentPerformance);
  const livePlan=planFullRestore(before,detached);
  assert.equal(livePlan.detached,true);assert.ok(livePlan.objectIds.includes(0));
  assert.deepEqual(livePlan.expected.currentPerformance,before.currentPerformance);
  console.log("PASS: exact expected live metadata normalization on real firmware's detached restore");
}


// Reproduce an acknowledged ID0 commit with the previous live performance
// still present. A strict failure must carry the actual validated readback,
// not invite an automatic retry or incorrectly claim a complete restore.
{
  const before=await captureFullDevice(mock,"LIVE MISMATCH");
  const source=structuredClone(before);
  const live=decodeProject(before.currentPerformance);
  const refs=live.refs.slice();refs[0]=refs[0]===32?33:32;
  source.currentPerformance=encodeProject({...live,refs,section:0});
  Object.assign(source.archive.objects.find(x=>x.id===0),raw(pad(source.currentPerformance)));
  const orig=mockObjects.get(0).slice(),calls=[];
  const ignoredLive=async(cmd,args=[],opts={})=>{
    const reply=await mock(cmd,args,opts);
    if(cmd===67&&args[0]===2){calls.push(args[1]);if(args[1]===0)mockObjects.set(0,orig.slice());}
    return reply;
  };
  const expected=planFullRestore(before,source);
  assert.deepEqual(expected.objectIds,[0]);
  await assert.rejects(applyVerifiedFullRestore(ignoredLive,before,source),e=>{
    assert.match(e.message,/Saved presets, projects and settings match.*Live performance does not match/);
    assert.deepEqual(e.readback.currentPerformance,before.currentPerformance);
    assert.deepEqual(e.readback.userPresets,before.userPresets);
    return true;
  });
  assert.deepEqual(calls,[0]);
  console.log("PASS: acknowledged live restore mismatch preserves strict failure and exact diagnostic readback");
}
