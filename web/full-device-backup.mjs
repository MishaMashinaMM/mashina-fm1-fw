/* SPDX-License-Identifier: GPL-3.0-only
 * Browser backup of the *entire supported MASHINA performance state*:
 * 32 USER sounds, 32 LIBRARY sounds, all 8 x A-D sections, current live
 * state and global settings. Pure verification comes before any device write.
 * Existing 65/66/67 chunked editor protocol carries the large objects.
 */
import {BACKUP_CMD,bkU32,bkCrc,bkPack,captureBackup,readBackup,restoreBackup} from "./fm1backup.js";
import {LIB_CMD,decodeRecord} from "./presets-library-codec.mjs";
import {captureUser,readLibrary,capability,uploadPrepared} from "./presets-library-sync.mjs";
import {decodeProject,restoreProjectsBackup,projectsBackup,MAX_RECORD} from "./projects-library-codec.mjs";
import {makePerformanceBundle,parsePerformanceBundle} from "./performance-backup.mjs";
const insist=(ok,msg)=>{if(!ok)throw new Error(msg);};
const archiveRequest=request=>([cmd,args],options={})=>request(cmd,args,options);
const dataBytes=object=>object?.bytes??Uint8Array.from(atob(object?.data??""),c=>c.charCodeAt(0));
const records=list=>{
  const arr=Array(32).fill(null);
  for(const entry of list){
    insist(Number.isInteger(entry.slot)&&entry.slot>=0&&entry.slot<32&&!arr[entry.slot],
           "Duplicate or invalid preset slot");
    arr[entry.slot]=decodeRecord(entry.recordBytes).recordBytes;
  }
  return arr;
};
/* Validate the independent 304-byte USER sound captures against the
 * authoritative raw USER banks (IDs 6/7) and FM6/drum companion bank (ID9).
 * Without this, an edited JSON could claim a different preset than the
 * archive actually restores, leaving linked projects with incorrect sounds.
 */
const SIDE_ENGINE=new Set([10,12,15,19,20]);
function byteEq(a,b) {
  return a.length===b.length&&a.every((x,i)=>x===b[i]);
}
function soundTag(record) {
  let h=2166136261;
  for(let i=0;i<192;i++)
    if(i<4||i>=16)h=Math.imul(h^record[i],16777619)>>>0;
  return h;
}
function u32le(b,o) {
  return (b[o]|b[o+1]<<8|b[o+2]<<16|b[o+3]<<24)>>>0;
}
function validateUserArchive(userPresets,archive) {
  const banks=[6,7].map(id=>archive.objects.find(x=>x.id===id));
  const fm=archive.objects.find(x=>x.id===9);
  const fmp=fm?.size?Array.from(dataBytes(fm)):null;
  if(fmp)insist(fmp.length===3728&&u32le(fmp,0)===0x36465055 &&
    (fmp[4]|fmp[5]<<8)===1&&(fmp[6]|fmp[7]<<8)===32,
    "Invalid FM6/drum companion bank");
  for(let slot=0;slot<32;slot++) {
    const record=userPresets[slot];
    const obj=banks[Math.floor(slot/16)];
    insist(!!obj,"Missing USER bank object");
    if(!obj.size) {
      insist(record===null,"USER "+(slot+1)+" is absent from archived banks");
      continue;
    }
    insist(obj.size===3080,"Invalid USER bank length");
    const raw=Array.from(dataBytes(obj));
    insist(u32le(raw,0)===0x31425055 &&
      (raw[4]|raw[5]<<8)===192&&(raw[6]|raw[7]<<8)===16,
      "Invalid USER bank header");
    const bytes=raw.slice(8+(slot%16)*192,8+(slot%16+1)*192);
    if(record===null) {
      insist(bytes[0]!==0xA5,"USER "+(slot+1)+" missing from separate sound capture");
      continue;
    }
    insist(byteEq(record.slice(0,192),bytes),
           "USER "+(slot+1)+" differs from the archived USER bank");
    if(SIDE_ENGINE.has(record[2])) {
      insist(fmp!==null,"Missing companion bank for USER "+(slot+1));
      const used=u32le(fmp,8);
      insist((used&(1<<slot))!==0,"Companion is absent for USER "+(slot+1));
      const off=16+slot*116;
      insist(u32le(fmp,off)===soundTag(record),
             "USER "+(slot+1)+" FM6/drum companion tag mismatch");
      insist(byteEq(fmp.slice(off+4,off+116),record.slice(192,304)),
             "USER "+(slot+1)+" FM6/drum companion differs from archive");
    }
  }
}
function archiveSections(archive) {
  const sections=Array(32).fill(null);
  for(let slot=0;slot<32;slot++) {
    const object=archive.objects.find(x=>x.id===40+slot);
    insist(object!==undefined,"MPR1 project "+(slot+1)+" missing from device archive");
    if(!object.size)continue;
    insist(object.size===MAX_RECORD,"Expected reference-only MPR1 project in slot "+(slot+1));
    const p=decodeProject(Array.from(dataBytes(object)));
    insist(p.section===slot%4,"Project section ID mismatch in backup");
    sections[slot]=p.recordBytes;
  }
  return sections;
}
/* IDs 2..5 are aliases for the currently selected project's A–D
 * sections. IDs 40..71 are the global 8xA–D positions. Verify that both
 * views agree; an inconsistent archive could otherwise overwrite one
 * section twice with different data during the restore.
 */
function archiveProjectAliases(archive) {
  const index=archive.objects.find(x=>x.id===10);
  insist(index?.size>=8,"Full backup is missing its selected-project index");
  const bytes=Array.from(dataBytes(index));
  insist(bytes[0]===83&&bytes[1]===79&&bytes[2]===78&&bytes[3]===71 &&
         bytes[4]===2&&bytes[5]<8,"Invalid selected-project index");
  const project=bytes[5];
  for(let section=0;section<4;section++) {
    const alias=archive.objects.find(x=>x.id===2+section);
    const global=archive.objects.find(x=>x.id===40+project*4+section);
    insist(!!alias&&!!global,"Backup is missing a current project section alias");
    insist(alias.size===global.size &&
           byteEq(Array.from(dataBytes(alias)),Array.from(dataBytes(global))),
           "Project "+(project+1)+" section "+("ABCD"[section])+
           " differs between archive aliases");
  }
  return project;
}
function archiveCurrent(archive) {
  const object=archive.objects.find(x=>x.id===0);
  insist(object?.size===MAX_RECORD,"Current performance is not reference-based MPR1");
  return decodeProject(Array.from(dataBytes(object))).recordBytes;
}
/* Read-only physical flash geometry diagnostic; does not erase, write,
 * upload presets or require the experimental FORMAT permission.
 * 0 blank, 1 committed MPR1/LIB1, 2 valid retired project, 3 unknown,
 * 4 flash not accessible. Unknown sectors block first-install formatting.
 */
export async function probeFlash(request) {
  const projects=Uint8Array.from(await request(97,[]));
  const library=Uint8Array.from(await request(98,[]));
  insist(projects.length===17&&projects[0]===16 &&
         library.length===5&&library[0]===4 &&
         projects.slice(1).every(x=>x<=4) &&
         library.slice(1).every(x=>x<=4),
         "FM1 does not report compatible read-only flash diagnostics");
  const p=Array.from(projects.slice(1)),l=Array.from(library.slice(1));
  return {projects:p,library:l,unknown:p.some(x=>x>=3)||l.some(x=>x>=3),
          canInitializeProjects:p.every(x=>x===0||x===2),
          libraryAllBlank:l.every(x=>x===0)};
}
export async function projectCapability(request) {
  const c=Uint8Array.from(await request(95,[]));
  insist((c.length===3||c.length===4)&&c[0]===1&&c[1]===32,
         "FM1 does not support the 32-section reference-only MPR1 project protocol");
  return {version:c[0],sections:c[1],writable:c[2]===1,
          canInitialize:c[3]===1};
}
export function validateFullDevice(file) {
  insist(file?.format==="mashina-full-device" && file.version===1,
         "Not a compatible MASHINA full-device backup");
  const archive=readBackup(file.archive);
  const bundle=makePerformanceBundle(file);
  validateUserArchive(bundle.userPresets,archive);
  const sections=archiveSections(archive),current=archiveCurrent(archive);
  const selectedProject=archiveProjectAliases(archive);
  // Full MPR1 restores must not include legacy migration records or missing live USER dependencies.
  insist(archive.objects.find(x=>x.id===8)?.size===0,"MPR1 backup contains a legacy FM6 migration object");
  insist(decodeProject(current).refs.every(ref=>ref>=32||bundle.userPresets[ref]!==null),
    "Current performance references a missing USER preset");
  insist(sections.every((r,i)=>JSON.stringify(r)===JSON.stringify(bundle.projects[i])),
         "Project JSON and device archive disagree: restoration refused");
  insist(JSON.stringify(current)===JSON.stringify(file.currentPerformance),
         "Current performance data and archive disagree");
  // The USER bank bytes in the archive are the restore authority. The
  // separate 304-byte USER captures are for dependency checks and audit.
  insist(archive.objects.some(x=>x.id===6) &&
         archive.objects.some(x=>x.id===7) &&
         archive.objects.some(x=>x.id===9) &&
         archive.objects.some(x=>x.id===1) &&
         archive.objects.some(x=>x.id===10),
         "Backup is missing USER banks, FM6 companions or device settings");
  return {...bundle,format:"mashina-full-device",version:1,
          created:file.created??null,firmware:file.firmware??"",
          archive:file.archive,currentPerformance:current,
          selectedProject:selectedProject+1};
}
export async function captureFullDevice(request,firmware="",progress=()=>{}) {
  const libcap=await capability(request);
  const projectcap=await projectCapability(request);
  insist(libcap.slots===32&&projectcap.sections===32,"Unexpected MASHINA storage geometry");
  const archive=await captureBackup(archiveRequest(request),firmware,
      (done,total)=>progress("device archive",done,total));
  const user=records(await captureUser(request,(done,total)=>progress("USER",done,total)));
  const library=records(await readLibrary(request,(done,total)=>progress("LIBRARY",done,total)));
  const projects=archiveSections(readBackup(archive));
  const currentPerformance=archiveCurrent(readBackup(archive));
  return validateFullDevice({format:"mashina-full-device",version:1,
    created:new Date().toISOString(),firmware,archive,currentPerformance,
    userPresets:user,libraryPresets:library,projects});
}
function paddedProject(raw) {
  if(raw===null)return new Uint8Array(0);
  const validated=decodeProject(raw).recordBytes;
  const out=new Uint8Array(MAX_RECORD);
  out.set(validated);
  return out;
}
async function writeProjectObject(request,id,raw) {
  const bytes=paddedProject(raw),size=bytes.length,crc=bkCrc(bytes);
  const call=async args=>{
    const reply=Uint8Array.from(await request(BACKUP_CMD.PUT,args,{timeout:5000,retries:0}));
    insist(reply.length>=3&&reply[0]===args[0]&&reply[1]===id,
           "Unexpected project restore reply");
    insist(reply[2]===0,"FM1 rejected project object "+id+" (error "+reply[2]+")");
  };
  await call([0,id,...bkU32(size),...bkU32(crc)]);
  try {
    for(let off=0;off<size;off+=256)
      await call([1,id,...bkU32(off),...bkPack(bytes.slice(off,off+256))]);
    await call([2,id]);
  }catch(err){
    await call([3,id]).catch(()=>{});
    throw err;
  }
}
export async function captureProjectsOnly(request,firmware="",progress=()=>{}) {
  await projectCapability(request);
  const archive=await captureBackup(archiveRequest(request),firmware,
      (done,total)=>progress("PROJECTS",done,total));
  const parsed=readBackup(archive);
  // Do not export an apparently valid projects-only file if the current
  // project's live A–D aliases disagree with their canonical bank records.
  // That mismatch indicates an inconsistent snapshot or in-flight change.
  archiveProjectAliases(parsed);
  return projectsBackup(archiveSections(parsed));
}
/* Project-only restore first scans USER slots and rejects broken references.
 * No project is changed until every imported MPR1 section has been validated
 * and the destination firmware has explicitly advertised write capability. */
export async function restoreProjectsOnly(request,file,progress=()=>{}) {
  const projects=restoreProjectsBackup(file);
  const cap=await projectCapability(request);
  insist(cap.writable,"FM1 project storage is read-only");
  const user=records(await captureUser(request));
  const check=makePerformanceBundle({userPresets:user,
    libraryPresets:Array(32).fill(null),projects});
  insist(check.projects.length===32,"Invalid project library");
  // Projects may reference any LIBRARY 033–064 slot. Check the actual device
  // records before the first write: CRC-damaged or malformed occupied slots
  // must never be mistaken for empty positions with factory fallbacks.
  if(projects.some(raw=>raw!==null&&decodeProject(raw).refs.some(ref=>ref>=32&&ref<64))) {
    const libcap=await capability(request);
    insist(libcap.slots===32,"Project restore needs compatible LIB32 storage");
    await readLibrary(request);
  }
  for(let i=0;i<32;i++){
    await writeProjectObject(request,40+i,projects[i]);
    progress("PROJECTS",i+1,32);
  }
  // Verify every section after all writes, including positions cleared to
  // empty. A successful PUT acknowledgement alone is insufficient evidence
  // that the section was persisted correctly in flash.
  const verify=await captureBackup(archiveRequest(request),"",
      (done,total)=>progress("PROJECTS VERIFY",done,total));
  const actual=archiveSections(readBackup(verify));
  insist(actual.every((record,i)=>JSON.stringify(record)===JSON.stringify(projects[i])),
         "Project-only restore verification failed; keep the offline backup and inspect the device");
  // The four current-project aliases (archive IDs 2..5) must resolve to the
  // same sections as global IDs 40..71. Detect a stale selected-project view
  // before claiming that a restored project is ready for live recall.
  archiveProjectAliases(readBackup(verify));
  return {restored:32}; // includes 32 empty-position clears
}
/* Preset, project, and current-performance restoration is deliberately not
 * atomic across all banks. The offline JSON is the recovery point if USB
 * disconnects. Strict old backup restore refuses unsupported object IDs.
 * Empty LIBRARY overrides are actively erased, not left as stale sounds.
 */
export async function restoreFullDevice(request,file,progress=()=>{}) {
  const full=validateFullDevice(file);
  const libcap=await capability(request),prcap=await projectCapability(request);
  insist(libcap.writable && prcap.writable,
         "Connected firmware does not allow both LIBRARY and PROJECT writes");
  // All records/CRC/reference dependencies were validated above. Capture a
  // separate offline backup of the target before calling this function.
  const entries=full.libraryPresets.flatMap((record,slot)=>
    record===null?[]:[{slot,recordBytes:record}]);
  for(let slot=0;slot<32;slot++){
    if(full.libraryPresets[slot]!==null)continue;
    const reply=Uint8Array.from(await request(LIB_CMD.erase,[slot],{timeout:6000}));
    insist(reply.length===2 && reply[0]===slot && reply[1]===0,
           "Could not erase stale LIBRARY slot "+(slot+33));
    progress("LIBRARY erase",slot+1,32);
  }
  await uploadPrepared(request,libcap,entries,
       (done,total)=>progress("LIBRARY upload",done,total));
  await restoreBackup(archiveRequest(request),full.archive,
       (done,total)=>progress("Device archive",done,total),
       {strict:true,skipProjectAliases:true});
  // Read-back checks independently verify the 32 LIBRARY positions and the
  // saved projects. A failed verification requests a retry from offline JSON.
  const after=records(await readLibrary(request));
  insist(JSON.stringify(after)===JSON.stringify(full.libraryPresets),
         "LIBRARY verification failed after restore");
  const usersAfter=records(await captureUser(request));
  insist(JSON.stringify(usersAfter)===JSON.stringify(full.userPresets),
         "USER or FM6/drum-kit sound verification failed after restore");
  const projectVerify=await captureBackup(archiveRequest(request),full.firmware);
  const archiveAfter=readBackup(projectVerify);
  insist(JSON.stringify(archiveSections(archiveAfter))===
         JSON.stringify(full.projects),"Project verification failed after restore");
  insist(JSON.stringify(archiveCurrent(archiveAfter))===
         JSON.stringify(full.currentPerformance),"Current performance verification failed");
  // An acknowledged PUT is not proof that the global configuration or the
  // selected-project index survived in flash. Check their exact raw bytes,
  // not just the MPR1 records and the separately captured USER sounds.
  for(const id of [1,10]) {
    const expected=readBackup(full.archive).objects.find(x=>x.id===id);
    const actual=archiveAfter.objects.find(x=>x.id===id);
    insist(!!expected&&!!actual&&byteEq(Array.from(expected.bytes),Array.from(actual.bytes)),
           (id===1?"Global settings":"Selected project index")+
           " verification failed after restore");
  }
  return {restoredProjects:32,librarySlots:32,userSlots:32,settingsRestored:true};
}

/* Optional diagnostic-image-only command 99. Every batch returns:
 * [version=1,start,count, (status,crc32-as-5-seven-bit-bytes)*count].
 * 32 MPR1, 8 LIBRARY, 4 USER, 2 companion, and 7 settings/index/autosave/voice sectors are fingerprinted without transmitting
 * raw preset data or authorizing writes. CRC32 detects ordinary unintended
 * changes, NOT deliberate tampering or ownership/recovery compatibility.
 */
const FINGERPRINT_EXTRA = Object.freeze([
  0xdc000,0xdd000,0xde000,0xdf000,0x9f000,0xfe000,
  0xfc000,0xfd000,0xd8000,0xd9000,0xe5000,0xe6000,0xe7000
]);
const fingerprintAddress=(index,version=3)=>
  index<32 ? 0xa0000+index*4096 :
  index<40 ? 0xc0000+(index-32)*4096 :
  index<46 && version>=2 ? FINGERPRINT_EXTRA[index-40] :
  version>=3 ? FINGERPRINT_EXTRA[index-40] : undefined;
export async function captureFlashFingerprints(request,progress=()=>{}){
  const sectors=[];
  for(let start=0;start<53;start+=8){
    const count=Math.min(8,53-start);
    const reply=Uint8Array.from(await request(99,[start,count]));
    insist(reply.length===3+6*count&&reply[0]===1&&
      reply[1]===start&&reply[2]===count,
      "FM1 does not support compatible read-only flash fingerprint diagnostics");
    for(let j=0;j<count;j++){
      const at=3+j*6,status=reply[at];
      insist(status===0||status===4,
        "Flash fingerprint sector returned an unknown read status");
      let crc=0;
      for(let k=0;k<5;k++){
        const b=reply[at+1+k];
        insist(b<=127&&(k!==4||b<=15),
          "Flash fingerprint includes invalid 7-bit CRC value");
        crc=(crc | (b << (7*k)))>>>0;
      }
      insist(status!==4||crc===0,
        "Unreadable sector must never claim a validated CRC");
      const index=start+j;
      const address=fingerprintAddress(index);
      sectors.push({index,address,
        readable:status===0,
        crc32:status===0?crc.toString(16).toUpperCase().padStart(8,"0"):null});
    }
    progress(sectors.length,53);
  }
  return {format:"mashina-flash-sector-fingerprints",version:3,
    mode:"READ_ONLY_DIAGNOSTIC",sectorBytes:4096,sectors,
    complete:sectors.every(x=>x.readable)};
}
/* Diagnostic firmware command 100 performs EXACT read-only format-preflight
 * validation of all 53 whitelisted sectors. Unlike the CRC32 summary, the
 * firmware checks the expected storage object type/copy, committed header,
 * payload and extension CRCs (or native MPR1 format) for each physical
 * sector. A matching CRC alone must NEVER stand in for this validation.
 *
 * 0 erased, 1 new format, 2 verified retired source-owned record,
 * 3 invalid/foreign, 4 unavailable. This diagnostic is not a write grant.
 */
export async function captureCleanLayoutPreflight(request,progress=()=>{}){
  const statusNames=["erased","new format","validated retired record",
                     "foreign/corrupt","unreadable"];
  const sectors=[];
  for(let start=0;start<53;start+=8){
    const count=Math.min(8,53-start);
    const reply=Uint8Array.from(await request(100,[start,count]));
    insist(reply.length===3+count&&reply[0]===1&&
      reply[1]===start&&reply[2]===count,
      "FM1 does not support compatible read-only clean-install preflight");
    for(let i=0;i<count;i++){
      const code=reply[3+i];
      insist(code<=4,"Invalid clean-install preflight status");
      const index=start+i;
      sectors.push({index,address:fingerprintAddress(index),code,
                    status:statusNames[code]});
    }
    progress(sectors.length,53);
  }
  const counts=statusNames.map((name,code)=>({
    name,count:sectors.filter(s=>s.code===code).length
  }));
  const allowedContent=sectors.every(s=>s.code===0||s.code===2);
  return {format:"mashina-clean-layout-preflight",version:1,
    mode:"READ_ONLY",sectorCount:sectors.length,complete:true,
    allowedContent,physicalWritesAllowed:false,
    counts,sectors,
    note:allowedContent
      ? "Sector content is compatible with the proposed clean layout; physical flash ownership, backup and recovery remain unproven. Writes are BLOCKED."
      : "At least one sector contains unrecognized, new-format or unreadable data. Clean initialization MUST refuse. Writes are BLOCKED."};
}
/* Compare only equal, complete sector maps. Does not authorize flash writes. */
export function compareFlashFingerprints(before,after){
  const validate=x=>x?.format==="mashina-flash-sector-fingerprints"&&
    (x.version===1||x.version===2||x.version===3)&&x.sectorBytes===4096&&
    Array.isArray(x.sectors)&&x.sectors.length===(x.version===1?40:x.version===2?46:53)&&
    x.complete===true&&
    x.sectors.every((s,i)=>s.index===i&&s.readable===true&&
      /^[0-9A-F]{8}$/.test(s.crc32));
  const compatible=(x,y)=>x.version===y.version &&
    x.sectors.every((s,i)=>s.address===y.sectors[i].address);
  insist(validate(before)&&validate(after)&&compatible(before,after),
    "Complete, compatible fingerprints from both diagnostic sessions are required");
  const changes=before.sectors.filter((s,i)=>
    s.crc32!==after.sectors[i].crc32)
    .map(x=>({index:x.index,address:x.address,
      before:x.crc32,after:after.sectors[x.index].crc32}));
  return {same:changes.length===0,changed:changes,
    note:"CRC32 comparisons do not prove bootloader ownership or guarantee collision-free integrity."};
}


/* Initialized-device physical LIBRARY management. No USER, project, global
 * or archive PUT commands. Refuse a change to ANY referenced slot, including
 * the current unsaved performance. A fresh complete snapshot must equal the
 * verified offline backup before writing; all unselected logical bytes must
 * still match after the write. No automatic retry or rollback. */
function sameFullPayload(a,b) {
  for(const key of ["userPresets","libraryPresets","projects","currentPerformance","selectedProject"])
    insist(JSON.stringify(a[key])===JSON.stringify(b[key]),key+" changed");
  const ao=readBackup(a.archive).objects,bo=readBackup(b.archive).objects;
  insist(ao.length===bo.length && ao.every((o,i)=>o.id===bo[i].id &&
    byteEq(o.bytes,bo[i].bytes)),"Archive payload changed");
}
export function planLibraryChanges(file,changes,{allowSavedReferences=false}={}) {
  const full=validateFullDevice(file);
  insist(Array.isArray(changes)&&changes.length>0&&changes.length<=32,"Select LIBRARY changes");
  const seen=new Set(),planned=[];
  for(const {slot,recordBytes} of changes) {
    insist(Number.isInteger(slot)&&slot>=0&&slot<32&&!seen.has(slot),"Duplicate or invalid LIBRARY slot");
    seen.add(slot);
    const record=recordBytes===null?null:decodeRecord(recordBytes).recordBytes;
    if(JSON.stringify(record)===JSON.stringify(full.libraryPresets[slot]))continue;
    const ref=slot+32,uses=[];
    for(let i=0;i<32;i++)if(full.projects[i]!==null) {
      decodeProject(full.projects[i]).refs.forEach((r,t)=>{
        if(r===ref)uses.push("P"+(Math.floor(i/4)+1)+":"+"ABCD"[i%4]+"/T"+(t+1));
      });
    }
    const live=[];
    decodeProject(full.currentPerformance).refs.forEach((r,t)=>{
      if(r===ref)live.push("Current performance/T"+(t+1));
    });
    const blocked=allowSavedReferences?live:[...uses,...live];
    insist(!blocked.length,"LIBRARY "+String(slot+33).padStart(3,"0")+" is referenced by "+blocked.join(", ")+" — change blocked");
    planned.push({slot,recordBytes:record});
  }
  return planned;
}
export async function applyVerifiedLibraryChanges(request,verifiedFile,changes,progress=()=>{},options={}) {
  const baseline=validateFullDevice(verifiedFile);
  const plan=planLibraryChanges(baseline,changes,options);
  const libcap=await capability(request),prcap=await projectCapability(request);
  insist(libcap.writable&&prcap.writable&&!prcap.canInitialize,
    "Use the initialized update firmware with formatting disabled");
  const fresh=await captureFullDevice(request,baseline.firmware,progress);
  try{sameFullPayload(baseline,fresh);}catch(e){
    throw Error("Device changed since verified backup: "+e.message+". Capture and verify a new complete backup. No writes sent.");
  }
  if(!plan.length)return {changed:0,after:fresh};
  const expected=structuredClone(fresh);
  for(const {slot,recordBytes} of plan)expected.libraryPresets[slot]=recordBytes;
  for(let i=0;i<plan.length;i++){
    const entry=plan[i],slot=entry.slot;
    try {
    if(entry.recordBytes===null){
      const reply=Uint8Array.from(await request(LIB_CMD.erase,[slot],{timeout:6000,retries:0}));
      insist(reply.length===2&&reply[0]===slot&&reply[1]===0,"LIBRARY reset rejected at "+(slot+33));
      const got=Uint8Array.from(await request(LIB_CMD.get,[slot]));
      insist(got.length===2&&got[0]===slot&&got[1]===3,"LIBRARY factory fallback verification failed at "+(slot+33));
    }else await uploadPrepared(request,libcap,[entry]);
    }catch(e){throw Error("LIBRARY "+(slot+33)+" failed after "+i+" of "+plan.length+" verified changes: "+e.message+". The failed slot may have changed; do not retry.");}
    progress("LIBRARY WRITE",i+1,plan.length);
  }
  const after=await captureFullDevice(request,baseline.firmware,progress);
  try{sameFullPayload(expected,after);}catch(e){
    throw Error("STOP — post-write full-state verification failed: "+e.message+". Keep the backup; do not retry.");
  }
  return {changed:plan.length,after};
}


/* Selective USER restore is deliberately one EMPTY slot in this hardware
 * trial. Build the expected archive first, then transfer only changed
 * objects 6/7 and 9. Never touch existing USER records or project refs.
 * Companion first: interruption before the base bank commit leaves the
 * formerly empty destination empty, with at most an unused tagged sidecar. */
export function planUserRestore(file,slot,recordBytes) {
  const full=validateFullDevice(file);
  insist(Number.isInteger(slot)&&slot>=0&&slot<32,"Invalid USER destination");
  insist(full.userPresets[slot]===null,"USER destination is occupied — first trial permits empty slots only");
  const record=decodeRecord(recordBytes).recordBytes;
  // Current stored record layout; older records may be normalized on PUT.
  insist((record[1]===4||record[1]===5)&&record[3]===127&&record[159]===0x47,
    "Use a complete current-format MASHINA sound record");
  // Non-companion engines are captured with zero sidecar bytes.
  insist(SIDE_ENGINE.has(record[2])||record.slice(192).every(x=>x===0),
    "Unexpected companion bytes for non-companion engine");
  const refUsed=full.projects.some(p=>p&&decodeProject(p).refs.includes(slot))||
    decodeProject(full.currentPerformance).refs.includes(slot);
  insist(!refUsed,"USER destination is referenced");
  const expected=structuredClone(full);
  const raw=id=>Array.from(dataBytes(readBackup(full.archive).objects.find(x=>x.id===id)));
  const set32=(a,o,v)=>{for(let i=0;i<4;i++)a[o+i]=(v>>>(8*i))&255;};
  const writeObject=(id,bytes)=>{
    const o=expected.archive.objects.find(x=>x.id===id);
    o.size=bytes.length;o.crc=bkCrc(bytes);o.data=btoa(String.fromCharCode(...bytes));
  };
  const id=6+Math.floor(slot/16),base=raw(id);
  const bank=base.length?base:Array(3080).fill(0);
  if(!base.length){set32(bank,0,0x31425055);bank[4]=192;bank[6]=16;}
  const pos=8+(slot%16)*192;
  // An unknown/inert record must not be silently overwritten.
  insist(bank.slice(pos,pos+192).every(v=>v===0),
    "Empty USER destination contains nonzero raw bytes; refusing trial overwrite");
  bank.splice(pos,192,...record.slice(0,192));
  writeObject(id,bank);
  const existing=raw(9),side=existing.length?existing.slice():Array(3728).fill(0);
  if(!existing.length){set32(side,0,0x36465055);side[4]=1;side[6]=32;}
  const off=16+slot*116,mask=u32le(side,8);
  if(SIDE_ENGINE.has(record[2])){
    set32(side,off,soundTag(record));
    side.splice(off+4,112,...record.slice(192));
    set32(side,8,(mask|(1<<slot))>>>0);
  }else if(existing.length){
    side.fill(0,off,off+116);
    set32(side,8,(mask&~(1<<slot))>>>0);
  }
  const objectIds=[];
  if((existing.length||SIDE_ENGINE.has(record[2]))&&!byteEq(existing,side)){
    writeObject(9,side);objectIds.push(9);
  }
  objectIds.push(id);
  expected.userPresets[slot]=record;
  return {expected:validateFullDevice(expected),objectIds,slot};
}
async function putUserObject(request,object) {
  insist([6,7,9].includes(object.id),"Forbidden selective USER object");
  const call=async args=>{
    const r=Uint8Array.from(await request(BACKUP_CMD.PUT,args,{timeout:30000,retries:0}));
    insist(r.length===3&&r[0]===args[0]&&r[1]===object.id&&r[2]===0,
      "USER object "+object.id+" PUT rejected or mismatched (phase "+args[0]+")");
  };
  await call([0,object.id,...bkU32(object.size),...bkU32(object.crc)]);
  try{
    for(let off=0;off<object.size;off+=256)
      await call([1,object.id,...bkU32(off),...bkPack(object.bytes.subarray(off,off+256))]);
    await call([2,object.id]);
  }catch(e){await call([3,object.id]).catch(()=>{});throw e;}
}
export async function applyVerifiedUserRestore(request,verifiedFile,slot,recordBytes,progress=()=>{}) {
  const baseline=validateFullDevice(verifiedFile);
  const plan=planUserRestore(baseline,slot,recordBytes);
  const libcap=await capability(request),prcap=await projectCapability(request);
  insist(libcap.writable&&prcap.writable&&!prcap.canInitialize,
    "Use the initialized update firmware with formatting disabled");
  const fresh=await captureFullDevice(request,baseline.firmware,progress);
  try{sameFullPayload(baseline,fresh);}catch(e){
    throw Error("Device changed since verified backup: "+e.message+". No writes sent.");
  }
  const archive=readBackup(plan.expected.archive);
  for(let i=0;i<plan.objectIds.length;i++){
    const id=plan.objectIds[i];
    try{await putUserObject(request,archive.objects.find(o=>o.id===id));}
    catch(e){throw Error("USER restore stopped on object "+id+" after "+i+" acknowledged objects: "+e.message+". Partial state possible; do not retry.");}
    progress("USER RESTORE",i+1,plan.objectIds.length);
  }
  const after=await captureFullDevice(request,baseline.firmware,progress);
  try{sameFullPayload(plan.expected,after);}catch(e){
    throw Error("Post-restore full-state verification failed: "+e.message+". Keep backup; do not retry.");
  }
  return {slot,after,objectIds:plan.objectIds};
}


/* New manager restore operations. The old lab helpers are retained for
 * compatibility with existing offline tests; physical controls only call
 * these guarded, delta-based operations. */
const equalJson=(a,b)=>JSON.stringify(a)===JSON.stringify(b);
function setArchiveObject(full,id,bytes){
  const o=full.archive.objects.find(x=>x.id===id);
  insist(o,"Missing archive object "+id);
  o.size=bytes.length;o.crc=bkCrc(bytes);o.data=btoa(String.fromCharCode(...bytes));
}
function objectDiff(a,b,id){
  const x=a.archive.objects.find(o=>o.id===id),y=b.archive.objects.find(o=>o.id===id);
  return !byteEq(dataBytes(x),dataBytes(y));
}
async function verifiedTarget(request,verified,progress){
  const baseline=validateFullDevice(verified);
  const libcap=await capability(request),prcap=await projectCapability(request);
  insist(libcap.writable&&prcap.writable&&!prcap.canInitialize,
    "Initialized write-capable firmware with formatting disabled required");
  const fresh=await captureFullDevice(request,baseline.firmware,progress);
  try{sameFullPayload(baseline,fresh);}catch(e){
    throw Error("Device changed since verified backup: "+e.message+". No writes sent.");
  }
  return {fresh,libcap};
}
async function transferArchiveObject(request,object){
  const id=object.id;
  insist(id===0||id===1||[6,7,9,10].includes(id)||(id>=40&&id<72),
    "Forbidden archive object "+id);
  const call=async args=>{
    const r=Uint8Array.from(await request(BACKUP_CMD.PUT,args,{timeout:30000,retries:0}));
    insist(r.length===3&&r[0]===args[0]&&r[1]===id&&r[2]===0,
      "Archive object "+id+" rejected or mismatched (phase "+args[0]+", reply "+Array.from(r)+")");
  };
  await call([0,id,...bkU32(object.size),...bkU32(object.crc)]);
  try{
    for(let off=0;off<object.size;off+=256)
      await call([1,id,...bkU32(off),...bkPack(object.bytes.subarray(off,off+256))]);
    await call([2,id]);
  }catch(e){await call([3,id]).catch(()=>{});throw e;}
}
async function transferRestorePlan(request,verified,plan,progress){
  const {libcap}=await verifiedTarget(request,verified,progress);
  const archive=readBackup(plan.expected.archive),total=plan.library.length+plan.objectIds.length;
  let done=0,label="";
  try{
    for(const {slot,recordBytes} of plan.library){
      label="LIBRARY "+(slot+33);
      if(recordBytes===null){
        const r=Uint8Array.from(await request(LIB_CMD.erase,[slot],{timeout:6000,retries:0}));
        insist(r.length===2&&r[0]===slot&&r[1]===0,"Reset rejected");
        const got=Uint8Array.from(await request(LIB_CMD.get,[slot]));
        insist(got.length===2&&got[0]===slot&&got[1]===3,"Reset verification failed");
      }else await uploadPrepared(request,libcap,[{slot,recordBytes}]);
      progress("RESTORE",++done,total);
    }
    for(const id of plan.objectIds){
      label="archive object "+id;
      await transferArchiveObject(request,archive.objects.find(o=>o.id===id));
      progress("RESTORE",++done,total);
    }
    label="full-state read-back";
    const after=await captureFullDevice(request,verified.firmware,progress);
    sameFullPayload(plan.expected,after);
    return {changed:done,after};
  }catch(e){
    throw Error("STOP — "+label+" failed after "+done+"/"+total+
      " acknowledged operations: "+e.message+". State may be partial. Keep the original backup; no automatic retry.");
  }
}
export function planUserBatch(verified,changes){
  insist(Array.isArray(changes)&&changes.length>0&&changes.length<=32,"Select 1–32 USER sounds");
  let expected=validateFullDevice(verified);
  const seen=new Set(),ids=new Set();
  for(const {slot,recordBytes} of changes){
    insist(!seen.has(slot),"Duplicate USER destination");seen.add(slot);
    const plan=planUserRestore(expected,slot,recordBytes);
    expected=plan.expected;plan.objectIds.forEach(id=>ids.add(id));
  }
  return {expected,library:[],objectIds:[9,6,7].filter(id=>ids.has(id)),userSlots:[...seen]};
}
export async function applyVerifiedUserBatch(request,verified,changes,progress=()=>{}){
  return transferRestorePlan(request,verified,planUserBatch(verified,changes),progress);
}
export function planProjectRestore(verified,file,selected=null){
  const baseline=validateFullDevice(verified),source=restoreProjectsBackup(file);
  const indexes=selected===null?Array.from({length:32},(_,i)=>i):selected;
  insist(Array.isArray(indexes)&&indexes.length>0&&indexes.length<=32&&
    new Set(indexes).size===indexes.length&&indexes.every(i=>Number.isInteger(i)&&i>=0&&i<32),
    "Select valid project sections");
  const expected=structuredClone(baseline),objectIds=[],sections=[];
  for(const i of indexes){
    const record=source[i];
    if(equalJson(record,baseline.projects[i]))continue;
    if(record)insist(decodeProject(record).refs.every(ref=>ref>=32||baseline.userPresets[ref]!==null),
      "P"+(Math.floor(i/4)+1)+":"+("ABCD"[i%4])+" references an empty USER slot");
    expected.projects[i]=record;
    const bytes=paddedProject(record);
    setArchiveObject(expected,40+i,bytes);
    if(Math.floor(i/4)===baseline.selectedProject-1)setArchiveObject(expected,2+i%4,bytes);
    objectIds.push(40+i);sections.push(i);
  }
  return {expected:validateFullDevice(expected),library:[],objectIds,sections};
}
export async function applyVerifiedProjectRestore(request,verified,file,selected,progress=()=>{}){
  return transferRestorePlan(request,verified,planProjectRestore(verified,file,selected),progress);
}
export function planFullRestore(verified,file){
  const baseline=validateFullDevice(verified),expected=validateFullDevice(file);
  // This is a same-layout logical restore, not a legacy format migrator.
  for(const id of [1,10]){
    const a=dataBytes(baseline.archive.objects.find(o=>o.id===id));
    const b=dataBytes(expected.archive.objects.find(o=>o.id===id));
    insist(a.length===b.length&&byteEq(a.slice(0,4),b.slice(0,4)),
      "Incompatible device settings/project index layout (object "+id+")");
  }
  const library=expected.libraryPresets.flatMap((recordBytes,slot)=>
    equalJson(recordBytes,baseline.libraryPresets[slot])?[]:[{slot,recordBytes}]);
  const objectIds=[9,6,7,10,...Array.from({length:32},(_,i)=>40+i),1].filter(id=>objectDiff(baseline,expected,id));
  // Firmware always captures a valid companion header after upf_empty().
  // A zero-length source cannot round-trip byte-for-byte on that firmware.
  if(objectIds.includes(9))insist(expected.archive.objects.find(o=>o.id===9).size===3728,
    "Source has no compatible companion bank; use a complete current firmware backup");
  // Reload live performance last, even when its reference bytes are
  // identical but a referenced sound changed, so runtime sound caches match.
  const liveChanged=decodeProject(expected.currentPerformance).refs.some(ref=>ref<64&&
    !equalJson(ref<32?baseline.userPresets[ref]:baseline.libraryPresets[ref-32],
               ref<32?expected.userPresets[ref]:expected.libraryPresets[ref-32]));
  if(liveChanged||objectDiff(baseline,expected,0)||objectIds.includes(1)||objectIds.includes(10))
    objectIds.push(0);
  return {expected,library,objectIds,
    users:expected.userPresets.map((r,i)=>equalJson(r,baseline.userPresets[i])?null:i).filter(i=>i!==null),
    sections:expected.projects.map((r,i)=>equalJson(r,baseline.projects[i])?null:i).filter(i=>i!==null)};
}
export async function applyVerifiedFullRestore(request,verified,file,progress=()=>{}){
  return transferRestorePlan(request,verified,planFullRestore(verified,file),progress);
}


/* Offline queue proposal. Imported USER sounds already present verbatim
 * are skipped. Prefer the original USER position when empty, otherwise
 * allocate the first free slot. Never overwrite an occupied/queued slot,
 * and refuse the entire proposed addition when it cannot fit. */
export function proposeUserImport(verified,patches,queued=[]){
  const full=validateFullDevice(verified);
  insist(Array.isArray(patches)&&patches.length<=32,"Invalid imported sound pack");
  const unavailable=new Set(queued.map(x=>x.slot));
  const records=[...full.userPresets.filter(Boolean),...queued.map(x=>x.recordBytes)];
  const changes=[],skipped=[];
  for(const p of patches){
    const r=decodeRecord(p.recordBytes).recordBytes;
    if(records.some(x=>equalJson(x,r))){skipped.push(decodeRecord(r).name);continue;}
    const eligible=i=>Number.isInteger(i)&&i>=0&&i<32&&!unavailable.has(i)&&full.userPresets[i]===null;
    const slot=eligible(p.slot)?p.slot:Array.from({length:32},(_,i)=>i).find(eligible);
    insist(slot!==undefined,"Not enough empty USER slots for this imported pack");
    // Validate all destinations and record layouts before returning a queue.
    planUserRestore(full,slot,r);
    unavailable.add(slot);records.push(r);changes.push({slot,recordBytes:r});
  }
  return {changes,skipped};
}
