/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {capability,readLibrary,captureUser,uploadPrepared,libraryBackupFile} from "../web/presets-library-sync.mjs";
import {encodeRecord,pack7,unpack7,LIB_CMD} from "../web/presets-library-codec.mjs";
const params=Array.from({length:127},(_,i)=>(i*5)%128);
const full=(engine,name)=>{
  if(engine!==19)return Array.from(encodeRecord({name,engine,params}));
  const raw=Array.from({length:304},()=>0);
  raw[0]=0xA5;raw[1]=4;raw[2]=19;raw[3]=127;
  [...name].forEach((c,i)=>raw[4+i]=c.charCodeAt(0));
  for(let i=0;i<127;i++)raw[16+i]=(i*5)%192;
  raw[159]=0x47;raw[303]=0xD3;
  return raw;
};
const user=[full(0,"ANALOG"),full(19,"DRUM AC")];
const slots=new Map();
let putCount=0,badPut=false;
async function request(cmd,a){
  if(cmd===LIB_CMD.cap)return [1,32,1];
  const s=a[0];
  if(cmd===LIB_CMD.captureUser){
    if(s>=user.length)return [s,1];
    return [s,0,...pack7(user[s])];
  }
  if(cmd===LIB_CMD.get)return slots.has(s)?[s,0,...pack7(slots.get(s))]:[s,3];
  if(cmd===LIB_CMD.put){
    putCount++;
    if(badPut)return [s,2];
    slots.set(s,Array.from(unpack7(a.slice(1))));return [s,0];
  }
  throw new Error("unexpected cmd "+cmd);
}
const cap=await capability(request);
assert.equal(cap.writable,true);
const prepared=await captureUser(request);
assert.equal(prepared.length,2);
assert.equal(prepared[1].engine,19);
assert.equal(prepared[1].recordBytes[303],0xD3);
assert.deepEqual(libraryBackupFile(prepared).patches.length,2);
assert.equal(await uploadPrepared(request,cap,prepared),2);
assert.equal(putCount,2);
const library=await readLibrary(request);
assert.deepEqual(library.map(x=>x.recordBytes),prepared.map(x=>x.recordBytes));
assert.equal(library.length,2);
await assert.rejects(uploadPrepared(request,{writable:false},prepared),/read-only/);
// A bad entry late in a batch must cause ZERO hardware writes.
putCount=0;
await assert.rejects(uploadPrepared(request,cap,[prepared[0],prepared[0]]),/Duplicate\/invalid destination slot/);
assert.equal(putCount,0);
const truncatedKit={...prepared[1],recordBytes:prepared[1].recordBytes.slice(0,303),slot:3};
await assert.rejects(uploadPrepared(request,cap,[prepared[0],truncatedKit]),/requires 304 bytes/);
assert.equal(putCount,0);
badPut=true;
await assert.rejects(uploadPrepared(request,cap,prepared),/rejected/);
console.log("PASS: capture all complete USER sound+drum-kit records, JSON backup, verified library restore and failure gates");
