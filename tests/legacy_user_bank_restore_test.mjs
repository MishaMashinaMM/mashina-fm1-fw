/* SPDX-License-Identifier: GPL-3.0-only
 * Simulated WebMIDI, NO DEVICE, proving original USER-only restore never
 * sends old project/settings objects and verifies complete 304-byte patches.
 */
import assert from "node:assert/strict";
import {BACKUP_IDS,bkCrc,bkR32,bkUnpack} from "../web/fm1backup.js";
import {encodeRecord,pack7} from "../web/presets-library-codec.mjs";
import {restoreOriginalUserBanks} from "../web/legacy-user-bank-restore.mjs";

const originalBytes=engine=>encodeRecord({
  name:engine===12?"FM6 ORIGINAL":"ANAL ORIG",engine,
  params:Array.from({length:127},(_,i)=>(i*3)%100),
  ...(engine===12?{fm6:Array.from({length:128},(_,i)=>(i*11)&127)}:{})
});
const analog=originalBytes(0),fm6=originalBytes(12);
const b0=new Uint8Array(3080),b1=new Uint8Array(3080),comp=new Uint8Array(3728);
for(const b of [b0,b1])b.set([0x55,0x50,0x42,0x31,192,0,16,0]);
b0.set(analog.slice(0,192),8);
b1.set(fm6.slice(0,192),8+192); // USER 018
comp.set([0x55,0x50,0x46,0x36,1,0,32,0]);comp[10]=2; // used bit 17
let h=2166136261;
for(let i=0;i<192;i++)if(i<4||i>=16)h=Math.imul(h^fm6[i],16777619)>>>0;
for(let k=0;k<4;k++)comp[16+17*116+k]=(h>>>(k*8))&255;
comp.set(fm6.slice(192,304),16+17*116+4);
const raw=new Map([[0,Uint8Array.of(1,2,3,4)],[1,Uint8Array.of(4,3,2,1)],
                   [6,b0],[7,b1],[9,comp]]);
const obj=(id,b)=>({id,size:b.length,crc:bkCrc(b),data:Buffer.from(b).toString("base64")});
const source={format:"felucca-backup",version:1,objects:BACKUP_IDS.map(id=>
  obj(id,raw.get(id)??new Uint8Array(0)))};
function makeMock(){
  const sent=[],done=new Map(),pending=new Map(),recs=new Map([[0,analog],[17,fm6]]);
  const request=async(cmd,args)=>{
    if(cmd===89)return [1,32,1];
    if(cmd===95)return [1,32,1,1];
    if(cmd===97)return [16,1,...Array(15).fill(0)];
    if(cmd===98)return [4,0,0,0,0];
    if(cmd===94){
      const slot=args[0],rec=done.size===3?recs.get(slot):null;
      return rec?[slot,0,...pack7(rec)]:[slot,1];
    }
    assert.equal(cmd,67,"Only USER backup PUT is allowed");
    const stage=args[0],id=args[1];sent.push([stage,id]);
    assert([6,7,9].includes(id),"old settings or FUN project were sent");
    if(stage===0){
      const len=bkR32(args,2),crc=bkR32(args,7);
      pending.set(id,{bytes:new Uint8Array(len),crc,pos:0});
    } else if(stage===1){
      const p=pending.get(id);assert(p);
      const off=bkR32(args,2);
      assert.equal(off,p.pos);
      const n=Math.min(256,p.bytes.length-off);
      p.bytes.set(bkUnpack(args.slice(7),n),off);p.pos+=n;
    } else if(stage===2){
      const p=pending.get(id);assert(p);
      assert.equal(p.pos,p.bytes.length);
      assert.equal(bkCrc(p.bytes),p.crc);
      done.set(id,p.bytes);pending.delete(id);
    } else if(stage===3)pending.delete(id);
    return [stage,id,0];
  };
  return {sent,done,request};
}
{
  const mock=makeMock();
  await assert.rejects(()=>restoreOriginalUserBanks(mock.request,source),/explicit owner/);
  assert.equal(mock.sent.length,0);
}
{
  const corrupt=structuredClone(source);
  corrupt.objects.find(o=>o.id===9).crc^=1;
  const mock=makeMock();
  await assert.rejects(()=>restoreOriginalUserBanks(mock.request,corrupt,{authorized:true}),/checksum/);
  assert.equal(mock.sent.length,0,"failed source validation sent a write");
}
{
  const mock=makeMock();
  const result=await restoreOriginalUserBanks(mock.request,source,{authorized:true});
  assert.deepEqual(result,{verifiedSounds:2,restoredObjectIds:[6,7,9],
    oldProjectsRestored:false,oldSettingsRestored:false});
  assert.deepEqual([...mock.done.keys()],[6,7,9]);
  assert(mock.sent.every(([,id])=>[6,7,9].includes(id)));
  for(const id of [6,7,9])
    assert.deepEqual([...mock.done.get(id)],[...raw.get(id)]);
}
{
  const writes=[];
  const mock=makeMock();
  const request=async(cmd,args)=>{
    if(cmd===67){
      writes.push(args);
      return [args[0],args[1],4]; // device FLASH WRITE ERROR
    }
    return mock.request(cmd,args);
  };
  await assert.rejects(()=>restoreOriginalUserBanks(request,source,{authorized:true}),
    /Backup PUT refused stage 0 for USER object 6/);
  assert(writes.every(a=>a[1]===6),"failure must not touch another object");
}
/* Hardware may advertise new write capabilities while still containing
 * retired FUN data or a partially formatted sector: NEVER PUT to it. */
for(const [probe,reply] of [
  [97,[16,2,...Array(15).fill(0)]],  // legacy project sectors
  [97,[16,1,0,0,3,...Array(12).fill(0)]], // corrupt next bank
  [98,[4,0,0,1,0]],                  // occupied LIBRARY slot
  [98,[4,0,0,3,0]]                   // unknown/partly erased LIBRARY
]){
  const mock=makeMock();
  const request=(cmd,args)=>cmd===probe?reply:mock.request(cmd,args);
  await assert.rejects(()=>restoreOriginalUserBanks(request,source,{authorized:true}),
    /freshly initialized|nonempty or unknown LIBRARY/);
  assert.equal(mock.sent.length,0,"unsafe layout must send ZERO write commands");
}
/* A write-stage failure in the second bank must never start the third. */
{
  const mock=makeMock();
  const request=async(cmd,args)=>{
    if(cmd===67 && args[0]===1 && args[1]===7)
      return [1,7,4];
    return mock.request(cmd,args);
  };
  await assert.rejects(()=>restoreOriginalUserBanks(request,source,{authorized:true}),
    /Original USER restore stopped on object 7/);
  assert(!mock.sent.some(([,id])=>id===9),
    "failure in USER bank 7 must not touch companion bank 9");
}
/* A successful PUT is not enough: complete record readback must match
 * the untouched original, including FM6/drum companion bytes. */
{
  const mock=makeMock();
  const request=async(cmd,args)=>{
    if(cmd===94 && mock.done.size===3 && args[0]===17){
      const changed=Uint8Array.from(fm6);changed[245]^=1;
      return [17,0,...pack7(changed)];
    }
    return mock.request(cmd,args);
  };
  await assert.rejects(()=>restoreOriginalUserBanks(request,source,{authorized:true}),
    /differs from original complete 304-byte sound/);
  assert.deepEqual([...mock.done.keys()],[6,7,9],
    "failed verification must not trigger an automatic rewrite");
}
console.log("PASS: authorized 43-object archive restores ONLY USER banks 6/7/9 and verifies FM6/drum-compatible capture; errors stop without retries");
