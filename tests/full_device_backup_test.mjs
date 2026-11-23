/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {bkCrc} from "../web/fm1backup.js";
import {encodeProject,decodeProject,MAX_RECORD} from "../web/projects-library-codec.mjs";
import {encodeRecord,pack7} from "../web/presets-library-codec.mjs";
import {validateFullDevice,projectCapability,restoreProjectsOnly,restoreFullDevice} from "../web/full-device-backup.mjs";
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
          id===41?pad(section):new Uint8Array(0);
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
assert.throws(()=>validateFullDevice({...base,projects:Array(32).fill(null)}),/disagree/);
assert.throws(()=>validateFullDevice({...base,currentPerformance:section}),/disagree/);
const corrupt=structuredClone(base);corrupt.archive.objects[11].crc=25;
assert.throws(()=>validateFullDevice(corrupt),/checksum/);
const tampered=structuredClone(base);tampered.archive.objects[0].size=33;
assert.throws(()=>validateFullDevice(tampered),/archive object|checksum|MPR1/);
let changed=0,notes=[];
async function request(cmd,args){
  if(cmd===95)return [1,32,1];
  if(cmd===94)return [args[0],1];
  if(cmd===67){changed++;notes.push(args.slice(0,2));return [args[0],args[1],0];}
  throw new Error("unexpected mock command "+cmd);
}
assert.deepEqual(await projectCapability(request),{version:1,sections:32,writable:true});
let progress=0;
const result=await restoreProjectsOnly(request,
  {format:"mashina-projects-mpr1",version:1,projects:8,sectionsPerProject:4,
   sections:base.projects},()=>progress++);
assert.equal(result.restored,32);
assert.equal(progress,32);
assert.ok(changed>=32*2);
assert.ok(notes.some(x=>x[0]===0&&x[1]===41));
assert.ok(notes.some(x=>x[0]===2&&x[1]===41));
assert.ok(notes.some(x=>x[0]===0&&x[1]===40));
const readOnly=async(cmd,args)=>cmd===95?[1,32,0]:[args?.[0]??0,1];
const before=changed;
await assert.rejects(restoreProjectsOnly(readOnly,
  {format:"mashina-projects-mpr1",version:1,projects:8,sectionsPerProject:4,
   sections:base.projects}),/read-only/);
assert.equal(changed,before);
await assert.rejects(restoreFullDevice(async(cmd)=>cmd===89?[1,32,1]:cmd===95?[1,32,0]:[],base),
  /does not allow/);
console.log("PASS: full-device MPR1 validation, strict archive equality, 32-section chunked project restore and write gates");
