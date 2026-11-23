/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {bkCrc,bkPack,bkUnpack,bkU32,bkR32} from "../web/fm1backup.js";
import {encodeProject,decodeProject,MAX_RECORD} from "../web/projects-library-codec.mjs";
import {encodeRecord,pack7,unpack7} from "../web/presets-library-codec.mjs";
import {validateFullDevice,projectCapability,restoreProjectsOnly,restoreFullDevice,probeFlash,captureFullDevice,captureProjectsOnly} from "../web/full-device-backup.mjs";
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
  if(cmd===94)return [args[0],1]; // 32 USER positions are empty in this fixture
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
console.log("PASS: full-device MPR1 validation, strict archive equality, 32-section chunked project restore and write gates");
