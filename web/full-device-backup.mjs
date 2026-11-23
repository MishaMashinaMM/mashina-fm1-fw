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
  return projectsBackup(archiveSections(readBackup(archive)));
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
  for(let i=0;i<32;i++){
    await writeProjectObject(request,40+i,projects[i]);
    progress("PROJECTS",i+1,32);
  }
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
  return {restoredProjects:32,librarySlots:32,userSlots:32,settingsRestored:true};
}
