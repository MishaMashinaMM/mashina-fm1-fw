/* SPDX-License-Identifier: GPL-3.0-only
 * EXPERIMENTAL addressed restoration of ONLY legacy USER banks 6,7 and
 * the shared FM6/drum companion bank 9. Requires fresh MPR1/LIB32 firmware,
 * explicit owner authorization and a verified original 43-object archive.
 * Never imports old FUN project sections or original device settings.
 * Multi-object restore is not atomic; failures must stop without retries.
 */
import {readBackup,bkU32,bkPack} from "./fm1backup.js";
import {extractLegacyUserPresets} from "./legacy-user-extract.mjs";
import {captureUser} from "./presets-library-sync.mjs";

const requireOk=(ok,message)=>{if(!ok)throw Error(message);};
const SOURCE_IDS=[6,7,9];
function matches(a,b){
  return a.length===b.length&&a.every((v,i)=>v===b[i]);
}
export async function restoreOriginalUserBanks(request,original,opts={}){
  requireOk(opts.authorized===true,
    "Original USER restore requires explicit owner authorization");
  const archive=readBackup(original); // verify every CRC before ANY write
  requireOk(archive.objects.length===43,"Expected the complete original 43-object archive");
  const extracted=extractLegacyUserPresets(original); // verifies all engine sidecars and tags
  const expected=new Map(extracted.patches.map(p=>[p.slot,p.recordBytes]));
  const byId=new Map(archive.objects.map(o=>[o.id,o]));
  const sources=SOURCE_IDS.map(id=>{
    const item=byId.get(id);
    requireOk(item&&item.bytes&&item.bytes.length===item.size,
      "Original archive lacks complete USER data "+id);
    return item;
  });
  /* Physical write access must belong to new reference-only MPR1 firmware,
   * not an older device which happens to accept backup PUT. */
  const lib=Uint8Array.from(await request(89,[],{timeout:5000,retries:0}));
  const projects=Uint8Array.from(await request(95,[],{timeout:5000,retries:0}));
  requireOk(lib.length===3&&lib[0]===1&&lib[1]===32&&lib[2]===1 &&
    (projects.length===3||projects.length===4)&&projects[0]===1&&
    projects[1]===32&&projects[2]===1,
    "Device does not advertise an approved write-capable 32+32 MPR1 build");
  /* A fresh clean-start restore must never silently replace user-created
   * sounds already on the target FM1. Partial failures require an explicit,
   * separately reviewed recovery route, not automatic repeat. */
  if(opts.requireEmpty!==false){
    const before=await captureUser(request);
    requireOk(before.length===0,
      "Target USER bank is not empty: refusing to overwrite existing sounds");
  }
  let wrote=0;
  const put=async(args)=>{
    const r=Uint8Array.from(await request(67,args,{timeout:30000,retries:0}));
    requireOk(r.length===3&&r[0]===args[0]&&r[1]===args[1]&&r[2]===0,
      "Backup PUT refused stage "+args[0]+" for USER object "+args[1]+
      " (response "+Array.from(r).join(",")+")");
  };
  for(const obj of sources){
    await put([0,obj.id,...bkU32(obj.size),...bkU32(obj.crc)]);
    try {
      for(let pos=0;pos<obj.size;pos+=256){
        await put([1,obj.id,...bkU32(pos),...bkPack(obj.bytes.subarray(pos,pos+256))]);
      }
      await put([2,obj.id]);
    } catch(error){
      await put([3,obj.id]).catch(()=>{});
      throw Error("Original USER restore stopped on object "+obj.id+
        "; data may be partially restored. Do not restore old projects/settings. "+error.message);
    }
    wrote++;
    opts.progress?.(wrote,SOURCE_IDS.length);
  }
  const got=await captureUser(request);
  requireOk(got.length===expected.size,
    "Restored USER slots differ in occupancy from the original archive");
  for(const item of got){
    const bytes=expected.get(item.slot);
    requireOk(bytes&&matches(bytes,item.recordBytes),
      "Restored USER "+(item.slot+1)+" differs from original complete 304-byte sound");
  }
  return {verifiedSounds:got.length,restoredObjectIds:[...SOURCE_IDS],
    oldProjectsRestored:false,oldSettingsRestored:false};
}
