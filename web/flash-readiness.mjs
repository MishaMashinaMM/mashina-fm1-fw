/* SPDX-License-Identifier: GPL-3.0-only
 * Physical-device flash readiness interpretation. PURE / READ-ONLY.
 * A clean probe is NOT proof of sector ownership, updater behavior or
 * backup recoverability. Never use this report to authorize flash writes.
 */
const STATUSES=["blank","committed","retired project","unknown","unavailable"];
const fail=(ok,message)=>{if(!ok)throw Error(message);};
export const FLASH_READINESS_FORMAT="mashina-flash-readiness";
export function auditFlashProbe(probe){
  fail(probe&&typeof probe==="object"&&
       Array.isArray(probe.projects)&&probe.projects.length===16&&
       Array.isArray(probe.library)&&probe.library.length===4,
       "Flash probe must contain exactly 16 project banks and 4 LIBRARY banks");
  const valid=x=>Number.isInteger(x)&&x>=0&&x<=4;
  fail(probe.projects.every(valid)&&probe.library.every(valid),
       "Flash probe contains unknown status codes");
  const projectBanks=probe.projects.map((status,i)=>({
    bank:i,project:Math.floor(i/2)+1,sections:i%2===0?"A/B":"C/D",
    address:"0x"+(0xa0000+i*8192).toString(16).toUpperCase(),
    status:STATUSES[status],code:status
  }));
  const libraryBanks=probe.library.map((status,i)=>({
    bank:i,slotStart:33+i*8,slotEnd:40+i*8,
    address:"0x"+(0xea000+i*8192).toString(16).toUpperCase(),
    status:STATUSES[status],code:status
  }));
  const all=[...projectBanks,...libraryBanks];
  const counts=Object.fromEntries(STATUSES.map((s,i)=>
    [s,all.filter(x=>x.code===i).length]));
  const blocked=counts.unknown+counts.unavailable>0;
  const retired=counts["retired project"]>0;
  const libraryOccupied=libraryBanks.some(x=>x.code===1);
  const projectCommitted=projectBanks.some(x=>x.code===1);
  const messages=[];
  if(blocked)messages.push("Unknown, damaged or unreadable data detected; never initialize, erase, migrate or upload to these regions.");
  if(retired)messages.push("Older project records occupy candidate MPR1 sectors; preserve and independently verify the complete legacy project backup before any explicit migration decision.");
  if(projectCommitted||libraryOccupied)messages.push("Committed experimental data detected; preserve it and verify a complete compatible backup before any destructive action.");
  if(!blocked&&!retired)messages.push("Sector signatures are interpretable, but this is NOT proof of ownership, persistence across stock recovery/OTA, or backup completeness.");
  messages.push("This audit never grants permission for physical writes. Bootloader/vendor update ownership and complete backup/restore must be independently verified on the specific FM1 before experimental writes can be enabled.");
  return {
    format:FLASH_READINESS_FORMAT,version:1,decision:"PHYSICAL_WRITES_BLOCKED",
    sectorOwnershipProven:false,stockUpdaterProven:false,
    completeBackupVerified:false,physicalRestoreProven:false,
    count:counts,projectBanks,libraryBanks,
    needsLegacyMigration:retired,unknownOrUnreadable:blocked,
    messages
  };
}
