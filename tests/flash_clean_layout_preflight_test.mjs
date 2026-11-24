/* SPDX-License-Identifier: GPL-3.0-only
 * Contract for diagnostic WebMIDI command 100. This test never connects to
 * MIDI hardware and never initiates a physical flash operation.
 */
import assert from "node:assert/strict";
import {captureCleanLayoutPreflight} from "../web/full-device-backup.mjs";
const status=Array.from({length:53},(_,i)=>i===45?3:i===52?0:2);
let calls=0;
const progress=[];
const request=async(cmd,[start,count])=>{
  assert.equal(cmd,100);
  assert.equal(start,calls*8);
  assert.equal(count,start===48?5:8);
  calls++;
  return [1,start,count,...status.slice(start,start+count)];
};
const report=await captureCleanLayoutPreflight(request,(n,total)=>progress.push([n,total]));
assert.equal(calls,7);
assert.deepEqual(progress,[[8,53],[16,53],[24,53],[32,53],[40,53],[48,53],[53,53]]);
assert.equal(report.sectorCount,53);
assert.equal(report.sectors.length,53);
assert.deepEqual(report.sectors.slice(40,53).map(s=>s.address),[
  0xdc000,0xdd000,0xde000,0xdf000,0x9f000,0xfe000,
  0xfc000,0xfd000,0xd8000,0xd9000,0xe5000,0xe6000,0xe7000
]);
assert.equal(report.physicalWritesAllowed,false);
assert.equal(report.allowedContent,false);
assert.deepEqual(report.counts.map(x=>x.count),[1,0,51,1,0]);
assert.equal(report.sectors[45].status,"foreign/corrupt");
const allValid=await captureCleanLayoutPreflight(async(_cmd,[start,count])=>
  [1,start,count,...Array(count).fill(2)]);
assert.equal(allValid.allowedContent,true);
assert.equal(allValid.physicalWritesAllowed,false);
assert.match(allValid.note,/BLOCKED/);
for(const invalid of [
  [1,0,8],[1,1,8,...Array(8).fill(2)],
  [1,0,8,...Array(8).fill(9)],
  [2,0,8,...Array(8).fill(2)]
]){
  await assert.rejects(()=>captureCleanLayoutPreflight(async()=>invalid));
}
console.log("PASS: 53 exact validated sector statuses, denied writes, malformed replies rejected");
