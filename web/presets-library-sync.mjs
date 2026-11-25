/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only backup and verified restore operations for LIB32 WebMIDI.
 * Transport callback: request(cmd, 7bitArgs) -> 7-bit response payload.
 * Nothing writes until uploadPrepared() explicitly called with write-enabled
 * capabilities. Source JSON contains full 304-byte binary sound records.
 */
import {LIB_CMD,LIBRARY_SLOTS,USER_SLOTS,validLibrarySize,ENTRY_BYTES,PACKED_BYTES,encodeRecord,decodeRecord,pack7,unpack7} from "./presets-library-codec.mjs";
function invariant(ok,msg){if(!ok)throw new Error(msg);}
function response(payload,slot,label){
  const x=Uint8Array.from(payload);
  invariant(x.length>=2&&x[0]===slot,`${label}: response slot mismatch`);
  return {rc:x[1],data:x.slice(2)};
}
export async function capability(request){
  const r=Uint8Array.from(await request(LIB_CMD.cap,[]));
  invariant(r.length===3&&r[0]===1&&validLibrarySize(r[1]),"FM1 does not advertise compatible 32/96-slot library support");
  return {version:r[0],slots:r[1],writable:r[2]===1};
}
export async function readLibrary(request,progress=()=>{},slots=LIBRARY_SLOTS){
  invariant(validLibrarySize(slots),"Unsupported LIBRARY geometry");
  const list=[];
  for(let slot=0;slot<slots;slot++){
    const r=response(await request(LIB_CMD.get,[slot]),slot,"LIB_GET");
    if(r.rc===0){
      invariant(r.data.length===PACKED_BYTES,"LIB_GET: incomplete payload");
      const record=unpack7(r.data,ENTRY_BYTES);
      list.push({slot, ...decodeRecord(record)});
    } else invariant(r.rc===3,"LIB_GET: flash/data error in slot "+(slot+33));
    progress(slot+1,slots);
  }
  return list;
}
export async function captureUser(request,progress=()=>{}){
  const list=[];
  for(let slot=0;slot<USER_SLOTS;slot++){
    const r=response(await request(LIB_CMD.captureUser,[slot]),slot,"CAPTURE_USER");
    if(r.rc===0){
      invariant(r.data.length===PACKED_BYTES,"CAPTURE_USER: incomplete sound+sidecar");
      const record=unpack7(r.data,ENTRY_BYTES);
      list.push({slot,...decodeRecord(record)});
    } else invariant(r.rc===1,"USER "+(slot+1)+" has an incomplete FM6 or drum kit; resave it before export (rc "+r.rc+")");
    progress(slot+1,USER_SLOTS);
  }
  return list;
}
export async function uploadPrepared(request,cap,entries,progress=()=>{}){
  invariant(cap?.writable===true,"Connected firmware is read-only: library upload disabled");
  invariant(validLibrarySize(cap.slots),"Unsupported LIBRARY geometry");
  /* Validate the ENTIRE batch before the first hardware write. A duplicate
   * destination or a truncated kit must never leave a partially changed bank. */
  const seen=new Set();
  const prepared=[];
  for(const item of entries){
    const slot=item.slot;
    invariant(Number.isInteger(slot)&&slot>=0&&slot<cap.slots&&!seen.has(slot),"Duplicate/invalid destination slot");
    seen.add(slot);
    prepared.push({slot,rec:encodeRecord(item)});
  }
  let completed=0;
  for(const {slot,rec} of prepared){
    const put=response(await request(LIB_CMD.put,[slot,...pack7(rec)]),slot,"LIB_PUT");
    invariant(put.rc===0,`LIB_PUT slot ${slot+33}: rejected (rc ${put.rc})`);
    const got=response(await request(LIB_CMD.get,[slot]),slot,"LIB_GET verify");
    invariant(got.rc===0&&got.data.length===PACKED_BYTES,`LIB_GET verification unavailable at slot ${slot+33}`);
    const check=unpack7(got.data,ENTRY_BYTES);
    invariant(check.every((b,i)=>b===rec[i]),`LIB_GET verification mismatch at slot ${slot+33}`);
    progress(++completed,prepared.length);
  }
  return completed;
}
export function libraryBackupFile(patches,kind="library",slots=LIBRARY_SLOTS){
  invariant(validLibrarySize(slots),"Unsupported LIBRARY geometry");
  return {format:"mashina-preset-library-"+slots,version:1,kind,
          exported:new Date().toISOString(),patches};
}
