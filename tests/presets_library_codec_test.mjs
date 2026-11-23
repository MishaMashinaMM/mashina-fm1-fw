/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import fs from "node:fs";
import {encodeRecord,decodeRecord,packFm6,unpackFm6,pack7,unpack7,putRequest,parseImportedLibrary} from "../web/presets-library-codec.mjs";

const data=JSON.parse(fs.readFileSync("tests/fixtures/rest_factory_bank.json","utf8"));
const data2=JSON.parse(fs.readFileSync("tests/fixtures/voice_lofi_factory_bank.json","utf8"));
let n=0;
for(const p of [...data.patches,...data2.patches]){
  if([10,12,15,19,20].includes(p.engine))continue;
  const r=encodeRecord({name:p.name.slice(0,12),engine:p.engine,params:p.params});
  const round=decodeRecord(r);
  assert.equal(round.name,p.name.slice(0,12));
  assert.equal(round.engine,p.engine);
  assert.deepEqual(round.params,p.params);
  const wire=pack7(r);
  assert.equal(wire.length,348);
  assert.deepEqual(Array.from(unpack7(wire)),Array.from(r));
  const msg=putRequest(n%32,round);
  assert.equal(msg[0],240);assert.equal(msg[4],70);assert.equal(msg.at(-1),247);
  assert.deepEqual(Array.from(unpack7(msg.slice(6,-1))),Array.from(r));
  n++;
}
assert.ok(n>=30,"enough real melodic fixtures required");
const fm6=Array.from({length:128},(_,i)=>(i*37+11)&127);
assert.deepEqual(unpackFm6(packFm6(fm6)),fm6);
const base=Array.from({length:127},(_,i)=>i%128);
const fmRec=encodeRecord({name:"TEST FM6",engine:12,params:base,fm6});
assert.deepEqual(decodeRecord(fmRec).fm6,fm6);
const kit=Uint8Array.from({length:304},(_,i)=>i&255);
kit[0]=0xA5;kit[1]=4;kit[2]=19;kit[3]=127;kit.set(Buffer.from("KIT TEST"),4);
for(let i=0;i<127;i++)kit[16+i]=(i*7)%192;
kit[159]=0x47;
const dKit=decodeRecord(kit);
assert.deepEqual(Array.from(encodeRecord(dKit)),Array.from(kit),"full kit binary opaque round-trip");
assert.throws(()=>encodeRecord({name:"NO KIT",engine:19,params:base}),/Full drum-kit binary sidecar/);
assert.throws(()=>encodeRecord({name:"NO FM6",engine:12,params:base}),/FM6 patch is missing/);
assert.throws(()=>encodeRecord({name:"BAD",engine:0,params:Array(127).fill(300)}),/outside/);
assert.deepEqual(parseImportedLibrary({patches:[{name:"SYN",engine:0,params:base,slot:7}]}).map(x=>x.slot),[7]);
assert.throws(()=>unpack7([0],304),/truncated/);
console.log(`PASS: ${n} real factory JSON patches + FM6 + raw drum kit round-trip; WebMIDI 7-bit protocol strict`);
