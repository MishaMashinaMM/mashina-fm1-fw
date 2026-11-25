/* SPDX-License-Identifier: GPL-3.0-only
 * Experimental 32-slot library format and WebMIDI packet encoder.
 * Safe for use in a browser, but NOT connected to a physical FM1 yet.
 * A JSON file with full binary records round-trips without losing FM6/kit data.
 */
export const USER_SLOTS=32, LIBRARY_SLOTS=32, MAX_LIBRARY_SLOTS=96, ENTRY_BYTES=304, PACKED_BYTES=348;
export const validLibrarySize=n=>n===LIBRARY_SLOTS||n===MAX_LIBRARY_SLOTS;
export const LIB_CMD=Object.freeze({cap:89,list:90,get:91,put:92,erase:93,captureUser:94});
const KIT=new Set([10,15,19,20]);
function required(ok,msg){if(!ok)throw new Error(msg);}
function bytes(v,n,label){
  required(Array.isArray(v)&&v.length===n, `${label}: requires ${n} bytes`);
  required(v.every(x=>Number.isInteger(x)&&x>=0&&x<=255),`${label}: invalid byte`);
  return Uint8Array.from(v);
}
function nameBytes(name){
  required(typeof name==="string"&&name.length>0&&name.length<=12&&/^[\x20-\x7e]+$/.test(name),"name: use 1..12 printable ASCII characters");
  return Uint8Array.from(name.split("").map(c=>c.charCodeAt(0)));
}
function validateRecord(r){
  required(r.length===ENTRY_BYTES,"record: invalid length");
  required(r[0]===0xA5&&r[1]>=1&&r[1]<=5&&r[2]<21&&r[3]>=8&&r[3]<=144,"record: invalid header");
  required(r[4]>=32&&r[4]<=126,"record: missing name");
  for(let i=4;i<16;i++)required(!r[i]||(r[i]>=32&&r[i]<=126),"record: invalid name");
  if(r[1]>=4)for(let i=0;i<r[3];i++)required(r[16+i]<=191,"record: invalid packed parameter");
  if(KIT.has(r[2]))required(r[303]===0xD3,"complete drum kit companion missing");
  return r;
}
/* FM6 stores 128 seven-bit operator bytes as exactly 112 eight-bit bytes. */
export function packFm6(patch){
  required(Array.isArray(patch)&&patch.length===128&&patch.every(x=>Number.isInteger(x)&&x>=0&&x<128),
    "FM6: 128 seven-bit operator bytes required");
  const out=new Uint8Array(112);let acc=0,bits=0,k=0;
  for(const v of patch){acc|=v<<bits;bits+=7;while(bits>=8){out[k++]=acc&255;acc>>>=8;bits-=8;}}
  required(k===112,"FM6: packed length mismatch");
  return out;
}
export function unpackFm6(packed){
  const d=packed instanceof Uint8Array?packed:bytes(packed,112,"FM6");
  const out=[];let acc=0,bits=0,k=0;
  for(let i=0;i<128;i++){
    while(bits<7){acc|=d[k++]<<bits;bits+=8;}
    out.push(acc&127);acc>>>=7;bits-=7;
  }
  return out;
}
export function encodeRecord(p){
  required(p&&typeof p==="object","patch required");
  if(p.recordBytes)return validateRecord(bytes(p.recordBytes,ENTRY_BYTES,"record"));
  const e=p.engine, params=p.params??p.p;
  required(Number.isInteger(e)&&e>=0&&e<21,"invalid engine");
  required(Array.isArray(params)&&params.length===127,"127 full parameters required");
  if(KIT.has(e))throw new Error("Full drum-kit binary sidecar missing: use an exported 304-byte record; parameter JSON alone is not sufficient.");
  if(e===12)required(Array.isArray(p.fm6),"FM6 patch is missing from JSON");
  const nm=nameBytes(p.name);const r=new Uint8Array(ENTRY_BYTES);
  r[0]=0xA5;r[1]=4;r[2]=e;r[3]=127;r.set(nm,4);
  for(let i=0;i<127;i++){
    const v=params[i];required(Number.isInteger(v)&&v>=-64&&v<=127,`parameter ${i} outside -64..127`);
    r[16+i]=v+64;
  }
  r[159]=0x47; /* same unused v0.4 mark as existing USER record */
  if(p.pattern){
    required(Array.isArray(p.pattern)&&p.pattern.length===16,"pattern must contain 16 pairs");
    for(let i=0;i<16;i++){
      const pair=p.pattern[i];
      required(Array.isArray(pair)&&pair.length===2&&pair.every(v=>Number.isInteger(v)&&v>=0&&v<128),
        `pattern step ${i} invalid`);
      r[160+i]=pair[0];r[176+i]=pair[1];
    }
  }
  if(e===12)r.set(packFm6(p.fm6),192);
  return validateRecord(r);
}
export function decodeRecord(record){
  const r=validateRecord(record instanceof Uint8Array?record:bytes(record,ENTRY_BYTES,"record"));
  const name=String.fromCharCode(...r.slice(4,16)).replace(/\x00+$/,"");
  const result={name,engine:r[2],params:Array.from(r.slice(16,16+r[3]),v=>v-64),
    pattern:Array.from({length:16},(_,i)=>[r[160+i],r[176+i]]),
    recordBytes:Array.from(r)};
  if(r[2]===12)result.fm6=unpackFm6(r.slice(192,304));
  return result;
}
export function pack7(src){
  const a=src instanceof Uint8Array?src:Uint8Array.from(src);const out=[];
  for(let i=0;i<a.length;i+=7){
    const part=a.slice(i,i+7);let mask=0;
    for(let j=0;j<part.length;j++)mask|=(part[j]>>7)<<j;
    out.push(mask,...part.map(x=>x&127));
  }
  return Uint8Array.from(out);
}
export function unpack7(input,size=ENTRY_BYTES){
  const a=Uint8Array.from(input),out=new Uint8Array(size);let i=0,k=0;
  while(k<size){
    required(i<a.length,"packed payload truncated");
    const m=a[i++],n=Math.min(7,size-k);
    required(i+n<=a.length&&!(m>>n),"packed payload malformed");
    for(let j=0;j<n;j++){required(a[i]<128,"invalid SysEx byte");out[k++]=a[i++]|(((m>>j)&1)<<7);}
  }
  required(i===a.length,"trailing bytes");
  return out;
}
export function putRequest(slot,patch){
  required(Number.isInteger(slot)&&slot>=0&&slot<MAX_LIBRARY_SLOTS,"slot must be 0..95");
  const payload=pack7(encodeRecord(patch));
  required(payload.length===PACKED_BYTES,"bad PUT payload size");
  return Uint8Array.from([0xF0,0x7D,0x46,0x4C,LIB_CMD.put,slot,...payload,0xF7]);
}
export function parseImportedLibrary(obj){
  required(obj&&Array.isArray(obj.patches),"library JSON must include patches array");
  required(obj.patches.length<=MAX_LIBRARY_SLOTS,"maximum 96 patches");
  const seen=new Set();
  return obj.patches.map((p,i)=>{
    const slot=Number.isInteger(p.slot)?p.slot:i;
    required(slot>=0&&slot<MAX_LIBRARY_SLOTS&&!seen.has(slot),"Invalid or duplicate preset slot");
    seen.add(slot);
    return {slot,record:encodeRecord(p)};
  });
}
