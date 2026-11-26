/* SPDX-License-Identifier: GPL-3.0-only
 * MPR1 reference-only project JSON codec, with fixed flash-record backup support.
 * Carries all bytes of a project-only settings payload without converting
 * sound references into embedded sounds. The firmware's new settings map
 * and WebMIDI commands still have to be implemented.
 */
export const PROJECT_COUNT=8, SECTION_COUNT=4, TRACKS=6, PRESET_COUNT=128;
export const MPR_HEADER=32, MAX_SETTINGS=1792, MAX_RECORD=MPR_HEADER+MAX_SETTINGS;
export const EMPTY_REFERENCE=255;
const MAGIC=[0x4d,0x50,0x52,0x31];
const check=(ok,message)=>{if(!ok)throw new Error(message);};
function bytes(input,max,label){
  check(Array.isArray(input) && input.length<=max &&
        input.every(x=>Number.isInteger(x)&&x>=0&&x<=255),label+": invalid bytes");
  return Uint8Array.from(input);
}
function crcAdd(c,b){
  for(const v of b){
    c^=v;
    for(let i=0;i<8;i++)c=(c>>>1)^((c&1)?0xEDB88320:0);
  }
  return c>>>0;
}
function crc(r,n){
  let c=crcAdd(0xffffffff,r.slice(0,28));
  return (~crcAdd(c,r.slice(MPR_HEADER,MPR_HEADER+n)))>>>0;
}
function put32(r,off,v){for(let i=0;i<4;i++)r[off+i]=(v>>>(8*i))&255;}
function get32(r,off){return (r[off]|r[off+1]<<8|r[off+2]<<16|r[off+3]<<24)>>>0;}
function nameBytes(name){
  check(typeof name==="string" && name.length<=12 && /^[\x20-\x7e]*$/.test(name),
        "project name must be 0..12 printable ASCII");
  return Uint8Array.from([...name].map(c=>c.charCodeAt(0)));
}
export function encodeProject({section,refs,name="",settingsBytes=[]}){
  check(Number.isInteger(section)&&section>=0&&section<SECTION_COUNT,"invalid section");
  check(Array.isArray(refs)&&refs.length===TRACKS&&refs.every(x=>
    Number.isInteger(x)&&(x===EMPTY_REFERENCE||(x>=0&&x<PRESET_COUNT))),"invalid preset references");
  const settings=bytes(settingsBytes,MAX_SETTINGS,"project settings");
  const title=nameBytes(name);
  const r=new Uint8Array(MPR_HEADER+settings.length);
  r.set(MAGIC,0);r[4]=1;r[5]=section;
  r[6]=settings.length&255;r[7]=settings.length>>8;
  r.set(refs,8);r.set(title,14);r.set(settings,MPR_HEADER);
  put32(r,28,crc(r,settings.length));
  return Array.from(r);
}
export function renameProjectRecord(record,name){
  return encodeProject({...decodeProject(record),name});
}
export function decodeProject(input){
  const r=bytes(input,MAX_RECORD,"project record");
  check(r.length>=MPR_HEADER && MAGIC.every((v,i)=>r[i]===v) &&
    r[4]===1 && r[5]<SECTION_COUNT && !r[26]&&!r[27],"invalid MPR1 header");
  const n=r[6]|r[7]<<8;
  check(n<=MAX_SETTINGS && (r.length===MPR_HEADER+n || r.length===MAX_RECORD),
        "invalid MPR1 payload size");
  // Firmware backup stores the fixed 1,824-byte flash slot including zero
  // padding. Validate that padding, but export the canonical compact record.
  if(r.length===MAX_RECORD)
    check(r.slice(MPR_HEADER+n).every(x=>x===0),"MPR1 nonzero trailing padding");
  check(r.slice(8,14).every(x=>x===EMPTY_REFERENCE||x<PRESET_COUNT),"invalid MPR1 references");
  check(get32(r,28)===crc(r,n),"MPR1 checksum mismatch");
  const title=Array.from(r.slice(14,26));
  let zero=false;
  for(const b of title){
    if(b===0)zero=true;
    else check(!zero&&b>=32&&b<=126,"invalid MPR1 name");
  }
  const canonical=r.slice(0,MPR_HEADER+n);
  return {section:r[5],refs:Array.from(r.slice(8,14)),
          name:String.fromCharCode(...title).replace(/\x00+$/,""),
          settingsBytes:Array.from(canonical.slice(MPR_HEADER)),
          recordBytes:Array.from(canonical)};
}
/* Complete project export explicitly includes 8 x 4 positions. Null=empty.
 * Settings are encoded as opaque bytes, not silently discarded. */
export function projectsBackup(slots){
  check(Array.isArray(slots)&&slots.length===PROJECT_COUNT*SECTION_COUNT,
        "projects backup needs exactly 32 section positions");
  const sections=slots.map((x,i)=>{
    if(x===null)return null;
    const p=decodeProject(Array.isArray(x)?x:(x.recordBytes??encodeProject(x)));
    check(p.section===i%SECTION_COUNT,"project section index mismatch");
    return p.recordBytes;
  });
  return {format:"mashina-projects-mpr1",version:1,projects:PROJECT_COUNT,
          sectionsPerProject:SECTION_COUNT,sections};
}
export function restoreProjectsBackup(input){
  check(input?.format==="mashina-projects-mpr1" && input.version===1 &&
    input.projects===PROJECT_COUNT && input.sectionsPerProject===SECTION_COUNT,
    "unsupported projects backup format");
  return projectsBackup(input.sections?.map(x=>x===null?null:{recordBytes:x})).sections;
}
