/* SPDX-License-Identifier: GPL-3.0-only
 * Offline-only restoration of complete USER records from a legacy 43-object
 * device archive. No MIDI, flash or external services are touched.
 */
import assert from "node:assert/strict";
import {BACKUP_IDS,bkCrc} from "../web/fm1backup.js";
import {encodeRecord,parseImportedLibrary} from "../web/presets-library-codec.mjs";
import {extractLegacyUserPresets,freshMpr1EmulatorFromLegacy} from "../web/legacy-user-extract.mjs";
import {decodeEmulatorFlash,encodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash} from "../web/emulator-library-inspector.mjs";
import {userSourceRecords} from "../web/emulator-snapshot-editor.mjs";

const bytes=(v)=>Buffer.from(v);
const record=(name,engine)=>Uint8Array.from(encodeRecord({
  name,engine,params:Array.from({length:127},(_,i)=>(i*3)%100),
  ...(engine===12?{fm6:Array.from({length:128},(_,i)=>(i*11)&127)}:{})
}));
const sound1=record("SAVED ANAL",0);
const sound18=record("SAVED FM6",12);
/* Genuine industrial-kit sidecars are opaque 112-byte payloads terminated
 * with the firmware's D3 companion marker. They must survive unmodified. */
const drum=Uint8Array.from({length:304},(_,i)=>i&255);
drum[0]=0xA5;drum[1]=4;drum[2]=19;drum[3]=127;
drum.fill(0,4,16);drum.set(Buffer.from("SAVED KIT"),4);
for(let i=0;i<127;i++)drum[16+i]=(i*7)%192;
drum[159]=0x47;drum[303]=0xD3;
const bank0=new Uint8Array(3080),bank1=new Uint8Array(3080);
for(const b of [bank0,bank1])b.set([0x55,0x50,0x42,0x31,192,0,16,0]);
bank0.set(sound1.slice(0,192),8);
bank0.set(drum.slice(0,192),8+192); // USER 002
bank1.set(sound18.slice(0,192),8+192); // slot index 17
const companion=new Uint8Array(3728);
companion.set([0x55,0x50,0x46,0x36,1,0,32,0]);
companion.set([2,0,2,0],8); // USER 002 and USER 018 sidecars present
const tag=rec=>{
  let h=2166136261;
  for(let i=0;i<192;i++)if(i<4||i>=16)
    h=Math.imul(h^rec[i],16777619)>>>0;
  return h;
};
const put32=(out,at,n)=>{
  for(let k=0;k<4;k++)out[at+k]=(n>>>(8*k))&255;
};
put32(companion,16+1*116,tag(drum));
companion.set(drum.slice(192,304),16+1*116+4);
put32(companion,16+17*116,tag(sound18));
companion.set(sound18.slice(192,304),16+17*116+4);
const raw=new Map([[0,Uint8Array.of(1,2,3,4)],[1,Uint8Array.of(4,3,2,1)],
                   [6,bank0],[7,bank1],[9,companion]]);
const obj=(id,arr)=>({id,size:arr.length,crc:arr.length?bkCrc(arr):0,data:bytes(arr).toString("base64")});
const original={format:"felucca-backup",version:1,
  firmware:"MASHINA legacy test",created:"2026-11-24T00:00:00.000Z",
  objects:BACKUP_IDS.map(id=>obj(id,raw.get(id)??new Uint8Array(0)))};
const result=extractLegacyUserPresets(original);
assert.equal(result.format,"mashina-preset-library-32");
assert.equal(result.kind,"legacy-user-capture");
assert.equal(result.patches.length,3);
assert.deepEqual(result.patches.map(p=>p.slot),[0,1,17]);
assert.deepEqual(result.patches[0].recordBytes,Array.from(sound1));
assert.deepEqual(result.patches[1].recordBytes,Array.from(drum));
assert.deepEqual(result.patches[2].recordBytes,Array.from(sound18));
assert.equal(result.originalFirmware,original.firmware);
assert.equal(parseImportedLibrary(result).length,3);
const fresh=freshMpr1EmulatorFromLegacy(original);
const layout=inspectEmulatorFlash(fresh);
assert.equal(layout.users.filter(x=>x.status==="saved").length,3);
assert.equal(layout.library.filter(x=>x.status==="factory").length,32);
assert.equal(layout.projects.sections.filter(Boolean).length,0);
assert.deepEqual(userSourceRecords(fresh).map(x=>x.recordBytes),
  [Array.from(sound1),Array.from(drum),Array.from(sound18)]);
assert.deepEqual(Object.keys(decodeEmulatorFlash(fresh)).map(Number).sort(),
  [0x9f000,0xdc000],"new NOR snapshot must include ONLY USER bank and companion sectors");
assert.deepEqual(encodeEmulatorFlash(decodeEmulatorFlash(fresh)),fresh);
assert.deepEqual(freshMpr1EmulatorFromLegacy(original),fresh,
  "clean emulator creation should be deterministic and entirely read-only");
const changed=structuredClone(original);
changed.objects.find(o=>o.id===6).crc=(changed.objects.find(o=>o.id===6).crc^0x01)>>>0;
assert.throws(()=>extractLegacyUserPresets(changed),/checksum/);
const mismatched=structuredClone(original);
const comp=Uint8Array.from(companion);
comp[16+17*116]^=1;
mismatched.objects[mismatched.objects.findIndex(o=>o.id===9)]=obj(9,comp);
assert.throws(()=>extractLegacyUserPresets(mismatched),/sidecar tag/);
const brokenKit=structuredClone(original);
const badKit=Uint8Array.from(companion);badKit[16+1*116+115]^=1; // D3 marker damaged
brokenKit.objects[9]=obj(9,badKit);
assert.throws(()=>extractLegacyUserPresets(brokenKit),/USER 2|sidecar/);
const missingSidecar=structuredClone(original);
missingSidecar.objects[9]=obj(9,new Uint8Array(0));
assert.throws(()=>extractLegacyUserPresets(missingSidecar),/missing FM6\/drum sidecar/);
const wrongEngine=structuredClone(original);
const malformed=Uint8Array.from(bank0);malformed[8+2]=255;
wrongEngine.objects[6]=obj(6,malformed);
assert.throws(()=>extractLegacyUserPresets(wrongEngine),/USER 1/);
const allEmpty=structuredClone(original);
allEmpty.objects[6]=obj(6,new Uint8Array(0));
allEmpty.objects[7]=obj(7,new Uint8Array(0));
allEmpty.objects[9]=obj(9,new Uint8Array(0));
assert.equal(extractLegacyUserPresets(allEmpty).patches.length,0);
assert.equal(freshMpr1EmulatorFromLegacy(allEmpty).sectors.length,0);
console.log("PASS: legacy 43-object archive exports original USER 304-byte records, FM6 sidecar, rejects corrupt data; no device writes");
