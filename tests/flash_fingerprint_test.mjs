/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only physical diagnostic protocol 99: 46 exact flash sector CRCs.
 */
import assert from "node:assert/strict";
import {captureFlashFingerprints,compareFlashFingerprints} from "../web/full-device-backup.mjs";
const values=Array.from({length:46},(_,i)=>(Math.imul(i+1,0x9e3779b9)^0xA51CB937)>>>0);
let calls=0,progress=[];
const request=async(cmd,args)=>{
  assert.equal(cmd,99);
  const [start,count]=args;
  assert.equal(start,calls*8);calls++;
  assert.equal(count,start===40?6:8);
  const out=[1,start,count];
  for(let i=start;i<start+count;i++){
    const crc=values[i];
    out.push(0,...Array.from({length:5},(_,k)=>(crc>>>(7*k))&127));
  }
  return out;
};
const a=await captureFlashFingerprints(request,(done,total)=>progress.push([done,total]));
assert.equal(calls,6);
assert.deepEqual(progress,[[8,46],[16,46],[24,46],[32,46],[40,46],[46,46]]);
assert.equal(a.sectors.length,46);
assert.equal(a.version,2);
assert.equal(a.complete,true);
assert.equal(a.mode,"READ_ONLY_DIAGNOSTIC");
assert.equal(a.sectors[0].address,0xA0000);
assert.equal(a.sectors[31].address,0xBF000);
assert.equal(a.sectors[32].address,0xC0000);
assert.equal(a.sectors[39].address,0xC7000);
assert.deepEqual(a.sectors.slice(40).map(s=>s.address),[0xDC000,0xDD000,0xDE000,0xDF000,0x9F000,0xFE000]);
assert.deepEqual(a.sectors.map(x=>parseInt(x.crc32,16)),values);
const same=compareFlashFingerprints(a,structuredClone(a));
assert.equal(same.same,true);
const modified=structuredClone(a);
modified.sectors[32].crc32="ABCDEF12";
const diff=compareFlashFingerprints(a,modified);
assert.equal(diff.same,false);
assert.deepEqual(diff.changed.map(x=>x.address),[0xC0000]);
assert.throws(()=>compareFlashFingerprints(a,{...a,complete:false}),/Complete/);
const mismatched=structuredClone(a);
mismatched.sectors[45].address=0xEA000;
assert.throws(()=>compareFlashFingerprints(a,mismatched),/compatible/);
assert.throws(()=>compareFlashFingerprints(a,{...a,version:1}),/compatible/);
const badStatus=async(_cmd,[start,count])=>[
  1,start,count,...Array.from({length:count},()=>[4,0,0,0,0,0]).flat()
];
const unreadable=await captureFlashFingerprints(badStatus);
assert.equal(unreadable.complete,false);
assert.equal(unreadable.sectors[0].crc32,null);
assert.equal(unreadable.sectors[45].readable,false);
const corrupt=async(_cmd,[start,count])=>[
  1,start,count,...Array.from({length:count},()=>[0,0,0,0,0,16]).flat()
];
await assert.rejects(()=>captureFlashFingerprints(corrupt),/7-bit CRC/);
await assert.rejects(()=>captureFlashFingerprints(async()=>[1,0,8]),/compatible/);
await assert.rejects(()=>captureFlashFingerprints(async()=>[1,1,8]),/compatible/);
await assert.rejects(()=>captureFlashFingerprints(async()=>[
  1,0,8,...Array.from({length:8},()=>[4,1,0,0,0,0]).flat()]),/Unreadable/);
console.log("PASS: 46 exact CRC32 values, disjoint USER/project/LIBRARY ranges, batching, comparison and malformed/unreadable rejection; no writes");
