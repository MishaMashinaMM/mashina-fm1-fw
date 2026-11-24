/* SPDX-License-Identifier: GPL-3.0-only
 * Firmware-real browser NOR selective LIBRARY changes:
 * pure exported snapshot, A/B commit, exact preservation and corruption lock.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {storeSectors,restoreSectors} from "../web/emu/worklet.js";
import {encodeEmulatorFlash,decodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash} from "../web/emulator-library-inspector.mjs";
import {decodeProject} from "../web/projects-library-codec.mjs";
import {BACKUP_IDS,bkCrc} from "../web/fm1backup.js";
import {extractLegacyUserPresets,freshMpr1EmulatorFromLegacy} from "../web/legacy-user-extract.mjs";
import {editEmulatorLibrarySnapshot,editEmulatorUserSnapshot,editEmulatorProjectReferences,factorySourceRecords,snapshotSourceRecords,userSourceRecords,crc32} from "../web/emulator-snapshot-editor.mjs";
const master=JSON.parse(readFileSync("web/factory-presets-from-code.json","utf8"));
const factory=factorySourceRecords(master);
assert.equal(factory.length,107);
assert.equal(factory.filter(x=>x.available).length,89);
const thales=factory.find(x=>x.name==="THALES");
const plato=factory.find(x=>x.name==="PLATO");
assert(thales?.recordBytes.length===304);
assert(plato?.recordBytes.length===304);
assert(factory.find(x=>x.name==="BUTTLER-1").available===false,"kit requires exact sidecar");

const wasm=readFileSync("build/emu/felucca.wasm");
const {instance}=await WebAssembly.instantiate(wasm,{});
const e=instance.exports;e._initialize();e.web_nor_erase();e.web_boot();
assert.equal(e.web_mpr_test_save(),0);
const nor=new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size());
const initial=encodeEmulatorFlash(storeSectors(nor));
const originalJSON=JSON.stringify(initial);
const first=editEmulatorLibrarySnapshot(initial,[
  {slot:33,recordBytes:thales.recordBytes},
  {slot:34,recordBytes:plato.recordBytes},
  {slot:41,recordBytes:thales.recordBytes},
]);
assert.equal(first.inspection.library[0].name,"THALES");
assert.equal(first.inspection.library[1].name,"PLATO");
assert.equal(first.inspection.library[8].name,"THALES");
assert.equal(first.inspection.library.filter(x=>x.status==="overridden").length,3);
assert.deepEqual(inspectEmulatorFlash(initial).library.filter(x=>x.status==="overridden"),[]);
assert.equal(JSON.stringify(initial),originalJSON,"source snapshot edited in place");
const base=decodeEmulatorFlash(initial),changed=decodeEmulatorFlash(first.snapshot);
for(const [off,bytes] of Object.entries(base)){
  if(Number(off)>=0xc0000&&Number(off)<0xc8000)continue;
  assert.deepEqual(changed[off],bytes,"unrelated NOR sector mutated at "+off);
}
const references=inspectEmulatorFlash(initial).projects;
assert.deepEqual(first.inspection.projects,references,"project references or opaque settings changed");
const second=editEmulatorLibrarySnapshot(first.snapshot,[{slot:33,recordBytes:plato.recordBytes}]);
assert.equal(second.inspection.library[0].name,"PLATO");
assert.equal(second.inspection.library[1].name,"PLATO");
assert.equal(second.inspection.library[8].name,"THALES");
const erased=editEmulatorLibrarySnapshot(second.snapshot,[{slot:33,recordBytes:null}]);
assert.equal(erased.inspection.library[0].status,"factory");
assert.equal(erased.inspection.library[1].name,"PLATO");
const copies=decodeEmulatorFlash(erased.snapshot);
assert(copies[0xc0000]&&copies[0xc1000],"A/B copies not present after multiple saves");
console.log("PASS: selective exported NOR JSON edits 3 sounds, preserves MPR1/USER/settings, chooses A/B and clears one fallback");
// Archived 2026 experimental JSON used LIB1 at EA000. It must remain
// inspectable and its full bank must migrate offline into C0000 when edited.
const archivedSectors=decodeEmulatorFlash(first.snapshot);
for(let bank=0;bank<4;bank++)for(let copy=0;copy<2;copy++){
  const src=0xc0000+(bank*2+copy)*4096,old=0xea000+(bank*2+copy)*4096;
  if(archivedSectors[src]){archivedSectors[old]=archivedSectors[src];delete archivedSectors[src];}
}
const archived=encodeEmulatorFlash(archivedSectors);
assert.equal(inspectEmulatorFlash(archived).library[0].name,"THALES");
assert.equal(inspectEmulatorFlash(archived).library[1].name,"PLATO");
const reclaimed=editEmulatorLibrarySnapshot(archived,[{slot:35,recordBytes:thales.recordBytes}]);
assert.equal(reclaimed.inspection.library[0].name,"THALES");
assert.equal(reclaimed.inspection.library[1].name,"PLATO");
assert.equal(reclaimed.inspection.library[2].name,"THALES");
assert(decodeEmulatorFlash(reclaimed.snapshot)[0xc0000]||decodeEmulatorFlash(reclaimed.snapshot)[0xc1000],
  "offline edit must create the reclaimed C0000 LIB1 bank");
assert.equal(inspectEmulatorFlash(archived).library[2].status,"factory",
  "historical source must never be changed by the conversion");
console.log("PASS: pre-relocation exported NOR JSON keeps all sounds and migrates edited banks offline");
const list=snapshotSourceRecords(first.snapshot);
assert.equal(list.length,3);
assert.equal(list[0].recordBytes.length,304);
const cross=editEmulatorLibrarySnapshot(initial,[{slot:60,recordBytes:list[0].recordBytes}]);
assert.equal(cross.inspection.library[27].name,"THALES");
console.log("PASS: exact 304-byte saved LIBRARY record copied from one snapshot into another");
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[]),/1..32/);
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[{slot:65,recordBytes:thales.recordBytes}]),/Invalid/);
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[{slot:33,recordBytes:thales.recordBytes},{slot:33,recordBytes:null}]),/repeated/);
assert.throws(()=>editEmulatorLibrarySnapshot(initial,[{slot:33,recordBytes:thales.recordBytes.slice(0,192)}]),/304/);
let forged=structuredClone(initial);
forged.sectors[0].bytes=forged.sectors[0].bytes.replace(/A/,"B");
assert.throws(()=>editEmulatorLibrarySnapshot(forged,[{slot:33,recordBytes:thales.recordBytes}]),/checksum|Invalid|compatible/);
const broken=decodeEmulatorFlash(first.snapshot);
broken[0xc0000]=new Uint8Array(4096);broken[0xc1000]=new Uint8Array(4096);
assert.throws(()=>editEmulatorLibrarySnapshot(encodeEmulatorFlash(broken),[{slot:33,recordBytes:thales.recordBytes}]),/LIBRARY bank 0/);
console.log("PASS: partial, duplicate, invalid/corrupt sources are rejected without producing modified backups");


const stSector=(type,payload,copy=0,sequence=1)=>{
  const out=new Uint8Array(4096).fill(255);
  const w16=(p,v)=>{out[p]=v&255;out[p+1]=(v>>8)&255;};
  const w32=(p,v)=>{for(let k=0;k<4;k++)out[p+k]=(v>>>(k*8))&255;};
  w32(0,0x554c4546);w16(4,type);w16(6,copy);w32(8,sequence);
  w32(12,payload.length);w32(16,crc32(payload));
  w32(20,0xffffffff);w32(24,0xffffffff);
  w32(28,crc32(out.slice(0,28)));
  out.set(payload,256);return out;
};
const baseSectors=decodeEmulatorFlash(initial);
const simple=thales.recordBytes;
const fm=plato.recordBytes;
const drum=Uint8Array.from(simple);
drum[2]=10;drum.set([...Buffer.from("DRUM-ONE")],4);
drum.fill(0,192);drum[303]=0xd3;
const bank=new Uint8Array(3080);
const writeU16=(dest,p,v)=>{dest[p]=v&255;dest[p+1]=(v>>8)&255;};
const writeU32=(dest,p,v)=>{for(let k=0;k<4;k++)dest[p+k]=(v>>>(8*k))&255;};
writeU32(bank,0,0x31425055);writeU16(bank,4,192);writeU16(bank,6,16);
bank.set(simple.slice(0,192),8);
bank.set(fm.slice(0,192),8+192);
bank.set(drum.slice(0,192),8+384);
const upf=new Uint8Array(3728);
writeU32(upf,0,0x36465055);writeU16(upf,4,1);writeU16(upf,6,32);
writeU32(upf,8,(1<<1)|(1<<2));
const tag=raw=>{
  let v=2166136261;
  for(let i=0;i<raw.length;i++){
    if(i>=4&&i<16)continue;
    v=Math.imul(v^raw[i],16777619)>>>0;
  }
  return v;
};
writeU32(upf,16+1*116,tag(fm.slice(0,192)));
upf.set(fm.slice(192),16+1*116+4);
writeU32(upf,16+2*116,tag(drum.slice(0,192)));
upf.set(drum.slice(192),16+2*116+4);
const synthetic={...baseSectors,[0xdc000]:stSector(5,bank),[0x9f000]:stSector(8,upf)};
let copied=userSourceRecords(encodeEmulatorFlash(synthetic));
assert.equal(copied.length,3);
assert(copied.every(x=>x.available),"all 3 USER sounds have complete companions");
assert.deepEqual(copied[0].recordBytes,simple);
assert.deepEqual(copied[1].recordBytes,fm);
assert.deepEqual(copied[2].recordBytes,Array.from(drum));
const fullCopy=editEmulatorLibrarySnapshot(initial,
  copied.map((x,i)=>({slot:33+i,recordBytes:x.recordBytes})));
assert.equal(fullCopy.inspection.library[1].engine,12);
assert.equal(fullCopy.inspection.library[2].engine,10);
const oldUpf=structuredClone(upf);oldUpf[16+2*116+4]^=1;
const mismatch=userSourceRecords(encodeEmulatorFlash(
  {...synthetic,[0x9f000]:stSector(8,Uint8Array.from(oldUpf))}));
assert.equal(mismatch[2].available,true,
  "matching tag should not be changed by companion content corruption without CRC metadata");
const badTag=upf.slice();badTag[16+2*116]^=1;
const stale=userSourceRecords(encodeEmulatorFlash(
  {...synthetic,[0x9f000]:stSector(8,badTag)}));
assert.equal(stale[2].available,false,"stale drum companion must not be presented as transferable");
const missing=userSourceRecords(encodeEmulatorFlash({...synthetic,[0x9f000]:new Uint8Array(4096).fill(255)}));
assert.equal(missing[0].available,true);
assert.equal(missing[1].available,false);
assert.equal(missing[2].available,false);
console.log("PASS: USER source export preserves exact melodic/FM6/drum companions and blocks absent or stale tags");


const changedUser=editEmulatorUserSnapshot(initial,[
  {slot:1,recordBytes:copied[0].recordBytes},
  {slot:2,recordBytes:copied[1].recordBytes},
  {slot:3,recordBytes:copied[2].recordBytes},
]);
assert.equal(changedUser.inspection.users.filter(x=>x.status==="saved").length,3);
assert.equal(changedUser.inspection.users[0].name,"THALES");
assert.equal(changedUser.inspection.users[1].name,"PLATO");
assert.equal(changedUser.inspection.users[2].name,"DRUM-ONE");
let restoredUsers=userSourceRecords(changedUser.snapshot);
assert(restoredUsers.every(x=>x.available),"FM6 and drum companions lost after USER destination import");
assert.deepEqual(restoredUsers.map(x=>x.recordBytes),
  copied.map(x=>x.recordBytes),"USER records/companion roundtrip not exact");
const originalBeforeUser=decodeEmulatorFlash(initial),afterUser=decodeEmulatorFlash(changedUser.snapshot);
for(const [off,bytes] of Object.entries(originalBeforeUser)){
  if([0xdc000,0xdd000,0x9f000,0xfe000].includes(Number(off)))continue;
  assert.deepEqual(afterUser[off],bytes,"USER edit changed unrelated sector "+off);
}
assert.deepEqual(changedUser.inspection.projects,inspectEmulatorFlash(initial).projects);
assert.deepEqual(changedUser.inspection.library,inspectEmulatorFlash(initial).library);
const updatedUser=editEmulatorUserSnapshot(changedUser.snapshot,[{slot:1,recordBytes:copied[2].recordBytes}]);
assert.equal(userSourceRecords(updatedUser.snapshot)[0].engine,10);
assert.equal(userSourceRecords(updatedUser.snapshot)[1].engine,12);
const clearedUser=editEmulatorUserSnapshot(updatedUser.snapshot,[{slot:1,recordBytes:null}]);
assert.equal(clearedUser.inspection.users[0].status,"empty");
assert.equal(userSourceRecords(clearedUser.snapshot).length,2);
assert.equal(userSourceRecords(clearedUser.snapshot)[0].name,"PLATO");
assert.throws(()=>editEmulatorUserSnapshot(initial,[{slot:1,recordBytes:copied[2].recordBytes.slice(0,192)}]),/304/);
assert.throws(()=>editEmulatorUserSnapshot(initial,[{slot:0,recordBytes:copied[0].recordBytes}]),/Invalid/);
assert.throws(()=>editEmulatorUserSnapshot(initial,[{slot:1,recordBytes:null},{slot:1,recordBytes:null}]),/repeated/);
const combined=editEmulatorUserSnapshot(first.snapshot,[{slot:1,recordBytes:copied[1].recordBytes}]);
assert.equal(combined.inspection.library[0].name,"THALES");
assert.equal(combined.inspection.users[0].name,"PLATO");
assert.deepEqual(combined.inspection.projects,inspectEmulatorFlash(initial).projects);
console.log("PASS: selective USER copy, FM6/drum companion tags, A/B cycles, empty restore, and mixed USER+LIBRARY snapshot");


const origSection=decodeProject(inspectEmulatorFlash(initial).projects.sections[0]);
const refEdits=editEmulatorProjectReferences(initial,[
  {project:1,section:"A",track:1,reference:32},
  {project:1,section:"A",track:2,reference:33},
  {project:1,section:"A",track:3,reference:255},
]);
const newSection=decodeProject(refEdits.inspection.projects.sections[0]);
assert.deepEqual(newSection.refs.slice(0,3),[32,33,255]);
assert.deepEqual(newSection.settingsBytes,origSection.settingsBytes);
assert.equal(newSection.name,origSection.name);
assert.deepEqual(refEdits.inspection.users,inspectEmulatorFlash(initial).users);
assert.deepEqual(refEdits.inspection.library,inspectEmulatorFlash(initial).library);
const changedAgain=editEmulatorProjectReferences(refEdits.snapshot,[
  {project:1,section:"A",track:1,reference:34},
]);
assert.equal(decodeProject(changedAgain.inspection.projects.sections[0]).refs[0],34);
assert.equal(decodeProject(changedAgain.inspection.projects.sections[0]).refs[1],33);
const noRefChange=editEmulatorProjectReferences(changedAgain.snapshot,[
  {project:1,section:"A",track:1,reference:origSection.refs[0]},
  {project:1,section:"A",track:2,reference:origSection.refs[1]},
  {project:1,section:"A",track:3,reference:origSection.refs[2]},
]);
assert.deepEqual(decodeProject(noRefChange.inspection.projects.sections[0]).refs,origSection.refs);
assert.throws(()=>editEmulatorProjectReferences(initial,[{project:1,section:"B",track:1,reference:32}]),/empty Project/);
assert.throws(()=>editEmulatorProjectReferences(initial,[{project:1,section:"A",track:1,reference:0}]),/empty/);
assert.throws(()=>editEmulatorProjectReferences(initial,[{project:1,section:"A",track:1,reference:32},{project:1,section:"A",track:1,reference:33}]),/Duplicate/);
const mixRefs=editEmulatorProjectReferences(combined.snapshot,[{project:1,section:"A",track:1,reference:0}]);
assert.equal(decodeProject(mixRefs.inspection.projects.sections[0]).refs[0],0);
assert.equal(mixRefs.inspection.users[0].name,"PLATO");
assert.equal(mixRefs.inspection.library[0].name,"THALES");
console.log("PASS: project refs preserve opaque settings and sound banks, reject missing USER/empty section, support combined USER+LIBRARY+project edits");


const {instance:bootedEdited}=await WebAssembly.instantiate(wasm,{});
const edited=bootedEdited.exports;
edited._initialize();edited.web_nor_erase();
restoreSectors(new Uint8Array(edited.memory.buffer,edited.web_nor(),edited.web_nor_size()),
  decodeEmulatorFlash(mixRefs.snapshot));
edited.web_boot();
assert.equal(edited.web_mpr_test_used(),1,"edited project section lost after independent WASM reboot");
// A cold boot loads the last LIVE state, not a saved project section unless
// RESTORE LAST is enabled. Explicitly recall section A to test its references.
assert.equal(edited.web_mpr_test_load(),0,"saved Project 1 Section A failed to recall in real firmware");
assert.equal(edited.web_mpr_ref(0),1,"project T1 USER 001 reference not restored in real firmware");
assert.equal(edited.web_mpr_ref(1),34,"project T2 reference changed unexpectedly");
console.log("PASS: real WASM fresh boot reads selective USER/LIBRARY changes and remapped MPR1 track references");


/* COMPLETE LEGACY -> FRESH MPR1 EMULATOR -> REBOOT smoke check.
 * Nine distinct, complete 304-byte synthetic drum kits in precisely the
 * occupied positions in the owner's source archive (1–3,5–7,9–11).
 * Crucially the owner's actual custom sounds are NEVER published here.
 * Never migrate old project records or reuse any physical FM1 data.
 */
{
  const oldSlots=[0,1,2,4,5,6,8,9,10];
  const engines=[19,19,19,15,15,15,10,10,10];
  const oldSounds=oldSlots.map((slot,i)=>{
    const sound=Uint8Array.from(thales.recordBytes);
    sound[2]=engines[i];sound.fill(0,4,16);
    sound.set(Buffer.from("TEST-"+String(i+1).padStart(2,"0")),4);
    sound[16+100]=64+i; // distinct editable drum parameter
    sound.fill(0,192);
    for(let x=192;x<303;x++)sound[x]=(i*31+x*19)&255;
    sound[303]=0xd3; // complete drum companion marker
    return {slot,recordBytes:Array.from(sound)};
  });
  const oldBanks=[new Uint8Array(3080),new Uint8Array(3080)];
  for(const bank of oldBanks){
    writeU32(bank,0,0x31425055);writeU16(bank,4,192);writeU16(bank,6,16);
  }
  const oldComp=new Uint8Array(3728);
  writeU32(oldComp,0,0x36465055);writeU16(oldComp,4,1);writeU16(oldComp,6,32);
  let used=0;
  for(const {slot,recordBytes} of oldSounds){
    const rec=Uint8Array.from(recordBytes),bank=Math.floor(slot/16);
    oldBanks[bank].set(rec.slice(0,192),8+(slot%16)*192);
    writeU32(oldComp,16+slot*116,tag(rec.slice(0,192)));
    oldComp.set(rec.slice(192),16+slot*116+4);
    used|=1<<slot;
  }
  writeU32(oldComp,8,used);
  const archived=new Map([[0,Uint8Array.of(1,2,3,4)],
                          [1,Uint8Array.of(9,8,7,6)],
                          [6,oldBanks[0]],[7,oldBanks[1]],[9,oldComp]]);
  const originalBackup={
    format:"felucca-backup",version:1,firmware:"synthetic v0.13",
    objects:BACKUP_IDS.map(id=>{
      const bytes=archived.get(id)??new Uint8Array(0);
      return {id,size:bytes.length,crc:bytes.length?bkCrc(bytes):0,
        data:Buffer.from(bytes).toString("base64")};
    })
  };
  const portable=extractLegacyUserPresets(originalBackup);
  assert.deepEqual(portable.patches.map(p=>p.slot),oldSlots,
    "legacy archive extraction changed the nine original USER positions");
  const firstInstall=freshMpr1EmulatorFromLegacy(originalBackup);
  const fresh={snapshot:firstInstall,inspection:inspectEmulatorFlash(firstInstall)};
  assert.equal(fresh.inspection.users.filter(x=>x.status==="saved").length,9);
  assert.equal(fresh.inspection.library.filter(x=>x.status==="overridden").length,0);
  assert.equal(fresh.inspection.projects.sections.filter(Boolean).length,0,
    "old embedded FUN projects must NEVER be silently migrated");
  const exported=JSON.stringify(fresh.snapshot);
  const reimported=JSON.parse(exported);
  const soundReadback=userSourceRecords(reimported);
  assert.equal(soundReadback.length,9);
  for(let i=0;i<9;i++)
    assert.deepEqual(soundReadback[i].recordBytes,oldSounds[i].recordBytes,
      "USER "+(oldSlots[i]+1)+" lost original drum params or companion bytes");
  const {instance}=await WebAssembly.instantiate(wasm,{});
  const fw=instance.exports;fw._initialize();fw.web_nor_erase();
  restoreSectors(new Uint8Array(fw.memory.buffer,fw.web_nor(),fw.web_nor_size()),
    decodeEmulatorFlash(reimported));
  fw.web_boot();
  const cold=encodeEmulatorFlash(storeSectors(
    new Uint8Array(fw.memory.buffer,fw.web_nor(),fw.web_nor_size())));
  const afterBoot=userSourceRecords(cold);
  assert.equal(afterBoot.length,9);
  for(let i=0;i<9;i++)
    assert.deepEqual(afterBoot[i].recordBytes,oldSounds[i].recordBytes,
      "physical-format synthetic USER "+(oldSlots[i]+1)+" changed during firmware boot");
  assert.equal(inspectEmulatorFlash(cold).projects.sections.filter(Boolean).length,0);
  /* First new MPR1 project: begin with an empty 8xA–D grid, then save a
   * native project and remap three tracks to three preserved USER drums.
   * On cold restart, the MPR1 project must resolve names and kit sidecars,
   * without depending on the retired FUN project serialization. */
  assert.equal(fw.web_mpr_test_save(),0,"new clean project SAVE must succeed");
  const saved=encodeEmulatorFlash(storeSectors(
    new Uint8Array(fw.memory.buffer,fw.web_nor(),fw.web_nor_size())));
  const changes=editEmulatorProjectReferences(saved,[
    {project:1,section:"A",track:1,reference:0}, // USER 001
    {project:1,section:"A",track:2,reference:4}, // USER 005
    {project:1,section:"A",track:3,reference:8}, // USER 009
    {project:1,section:"A",track:4,reference:32}, // factory LIBRARY 033
  ]);
  const editedSection=decodeProject(changes.inspection.projects.sections[0]);
  assert.deepEqual(editedSection.refs.slice(0,4),[0,4,8,32]);
  assert.equal(changes.inspection.projects.sections.filter(Boolean).length,1);
  const {instance:again}=await WebAssembly.instantiate(wasm,{});
  const device=again.exports;device._initialize();device.web_nor_erase();
  restoreSectors(new Uint8Array(device.memory.buffer,device.web_nor(),device.web_nor_size()),
    decodeEmulatorFlash(changes.snapshot));
  device.web_boot();
  assert.equal(device.web_mpr_test_used(),1,"fresh new MPR1 project missing");
  assert.equal(device.web_mpr_test_load(),0,"new MPR1 project A failed to recall");
  assert.deepEqual([0,1,2,3].map(i=>device.web_mpr_ref(i)),[1,5,9,33]);
  const afterNewProject=encodeEmulatorFlash(storeSectors(
    new Uint8Array(device.memory.buffer,device.web_nor(),device.web_nor_size())));
  assert.deepEqual(userSourceRecords(afterNewProject).map(x=>x.recordBytes),
    oldSounds.map(x=>x.recordBytes),"new project RECALL rewrote authored USER drums");
  console.log("PASS: 43-object legacy backup -> nine distinct drum kits -> clean 32+32 MPR1 emulator flash -> boot -> export/import -> native project save -> USER references -> cold recall");
}

const legacyFm6=new Uint8Array(3472);
writeU32(legacyFm6,0,0x42364d46);
writeU16(legacyFm6,4,1);
writeU16(legacyFm6,6,27);
const retiredSnapshot=encodeEmulatorFlash({
  ...decodeEmulatorFlash(initial),
  [0x9f000]:stSector(7,legacyFm6)
});
assert.throws(()=>editEmulatorUserSnapshot(retiredSnapshot,[
  {slot:1,recordBytes:copied[0].recordBytes}
]),/migration first/,"new UPF6 must never silently suppress legacy FM6B migration");
console.log("PASS: retired FM6B migration is protected from selective USER write");
