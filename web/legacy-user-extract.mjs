/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only extraction of complete 304-byte USER sound records from an
 * original 43-object MASHINA/FELUCCA backup. NO device access or writes.
 * Old FUN project snapshots are deliberately NOT migrated to MPR1.
 */
import {readBackup} from "./fm1backup.js";
import {decodeRecord} from "./presets-library-codec.mjs";
import {libraryBackupFile} from "./presets-library-sync.mjs";
import {encodeEmulatorFlash} from "./emu/emulator-flash-backup.mjs";
import {editEmulatorUserSnapshot,userSourceRecords} from "./emulator-snapshot-editor.mjs";
import {inspectEmulatorFlash} from "./emulator-library-inspector.mjs";

const ensure=(ok,msg)=>{if(!ok)throw Error(msg);};
const u32=(a,i)=>(a[i]|a[i+1]<<8|a[i+2]<<16|a[i+3]<<24)>>>0;
const data=o=>Uint8Array.from(atob(o.data),c=>c.charCodeAt(0));
const SIDE=new Set([10,12,15,19,20]);
const UP_BANK_MAGIC=0x31425055,UPF_MAGIC=0x36465055;
function tag(rec){
  let h=2166136261;
  for(let i=0;i<192;i++)
    if(i<4||i>=16)h=Math.imul(h^rec[i],16777619)>>>0;
  return h;
}
export function extractLegacyUserPresets(input){
  const archive=readBackup(input); // 43 IDs and every per-object CRC32
  const byId=new Map(archive.objects.map(x=>[x.id,x]));
  for(const id of [6,7,9])ensure(byId.has(id),"Missing USER/companion backup object "+id);
  const rawBanks=[6,7].map(id=>{
    const o=byId.get(id);
    if(o.size===0)return null; // entirely empty bank
    ensure(o.size===3080,"Unexpected USER bank "+id+" size");
    const a=data(o);
    ensure(u32(a,0)===UP_BANK_MAGIC && (a[4]|a[5]<<8)===192 &&
      (a[6]|a[7]<<8)===16,"USER bank "+id+" has unknown record layout");
    return a;
  });
  const sf=byId.get(9);
  const companion=sf.size?data(sf):null;
  if(companion)ensure(companion.length===3728&&u32(companion,0)===UPF_MAGIC&&
    (companion[4]|companion[5]<<8)===1 &&
    (companion[6]|companion[7]<<8)===32,
    "Unknown FM6/drum companion bank format");
  const used=companion?u32(companion,8):0;
  const patches=[];
  for(let slot=0;slot<32;slot++){
    const bank=rawBanks[Math.floor(slot/16)];
    if(!bank)continue;
    const head=bank.subarray(8+(slot%16)*192,8+(slot%16+1)*192);
    if(head[0]!==0xA5)continue;
    const record=new Uint8Array(304);
    record.set(head);
    if(SIDE.has(record[2])){
      ensure(!!companion,"USER "+(slot+1)+": missing FM6/drum sidecar bank");
      ensure((used&(1<<slot))!==0,
        "USER "+(slot+1)+": missing required sound sidecar");
      const off=16+slot*116;
      ensure(u32(companion,off)===tag(record),
        "USER "+(slot+1)+": sidecar tag does not match its sound");
      record.set(companion.subarray(off+4,off+116),192);
    }
    let decoded;
    try{decoded=decodeRecord(record);}
    catch(error){throw Error("USER "+(slot+1)+": "+error.message);}
    patches.push({slot,...decoded});
  }
  /* Empty source slots are NOT assigned fake replacement sounds.
   * This is an export-only artifact; upload/restore is separately gated. */
  const output=libraryBackupFile(patches,"legacy-user-capture");
  output.originalFirmware=archive.firmware??"";
  output.originalArchiveCreated=archive.created??"";
  return output;
}

/* Make a brand-new, fully offline MPR1 emulator NOR snapshot from the legacy
 * 43-object archive. Only complete USER 304-byte records are carried over:
 * original embedded FUN projects, original global settings, and all flash
 * sectors from the physical device are intentionally NOT copied. New MPR1
 * projects begin empty and LIBRARY 033–064 use compiled factory fallbacks.
 *
 * Never use this JSON as a physical FM1 firmware/restore image.
 */
export function freshMpr1EmulatorFromLegacy(input){
  const archive=extractLegacyUserPresets(input);
  const edits=archive.patches.map(p=>({slot:p.slot+1,recordBytes:p.recordBytes}));
  const blank=encodeEmulatorFlash({});
  const snapshot=edits.length
    ?editEmulatorUserSnapshot(blank,edits).snapshot
    :blank;
  const view=inspectEmulatorFlash(snapshot);
  const records=userSourceRecords(snapshot);
  ensure(view.users.filter(x=>x.status==="saved").length===edits.length,
    "Clean emulator USER slot count mismatch");
  ensure(view.library.length===32 &&
    view.library.every(x=>x.status==="factory"),
    "Clean emulator must use 32 original compiled factory fallback sounds");
  ensure(view.projects.sections.length===32 &&
    view.projects.sections.every(x=>x===null),
    "Clean emulator must start with eight empty A–D projects");
  for(const p of archive.patches){
    const got=records.find(x=>x.id==="U"+(p.slot+1));
    ensure(got?.available && JSON.stringify(got.recordBytes)===
      JSON.stringify(p.recordBytes),
      "Clean emulator USER "+(p.slot+1)+" differs from the original archive");
  }
  return snapshot;
}
