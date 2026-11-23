// SPDX-License-Identifier: GPL-3.0-only
// Exact sparse NOR backups for the experimental MASHINA browser emulator.
// This is NOT a physical FM1 / WebMIDI preset-library JSON format.
import {STORE_AREAS} from "./worklet.js";

export const FORMAT="mashina-emulator-flash";
export const VERSION=1;
const SECTOR=4096;
const MAX_SECTORS=STORE_AREAS.reduce((n,[a,b])=>n+(b-a)/SECTOR,0);
const allowed=(offset)=>Number.isInteger(offset) && offset%SECTOR===0 &&
  STORE_AREAS.some(([a,b])=>offset>=a && offset+SECTOR<=b);
function hash(data) {
  let n=2166136261;
  for(const b of data) n=Math.imul(n^b,16777619)>>>0;
  return n.toString(16).padStart(8,"0");
}
function b64(bytes) {
  let s="";
  for(let i=0;i<bytes.length;i+=2048)
    s+=String.fromCharCode(...bytes.subarray(i,i+2048));
  return btoa(s);
}
function unb64(text) {
  if(typeof text!=="string" || text.length!==Math.ceil(SECTOR/3)*4 ||
     !/^[A-Za-z0-9+/]+={0,2}$/.test(text))throw Error("Invalid sector encoding");
  const s=atob(text);
  if(s.length!==SECTOR)throw Error("Invalid sector size");
  return Uint8Array.from(s,c=>c.charCodeAt(0));
}
export function encodeEmulatorFlash(sectors) {
  const out=[];
  for(const [key,raw] of Object.entries(sectors)) {
    const off=Number(key),data=raw instanceof Uint8Array?raw:new Uint8Array(raw);
    if(!allowed(off)||data.length!==SECTOR)
      throw Error("Invalid emulator flash sector "+key);
    if(data.every(v=>v===255))continue;
    out.push({offset:off,bytes:b64(data),checksum:hash(data)});
  }
  out.sort((a,b)=>a.offset-b.offset);
  if(out.length>MAX_SECTORS)throw Error("Too many flash sectors");
  return {format:FORMAT,version:VERSION,sectorSize:SECTOR,
    kind:"experimental-browser-NOR-only",sectors:out};
}
export function decodeEmulatorFlash(value) {
  const obj=typeof value==="string"?JSON.parse(value):value;
  if(!obj || typeof obj!=="object" || Array.isArray(obj) ||
     obj.format!==FORMAT || obj.version!==VERSION ||
     obj.sectorSize!==SECTOR || obj.kind!=="experimental-browser-NOR-only" ||
     !Array.isArray(obj.sectors) || obj.sectors.length>MAX_SECTORS)
    throw Error("Not a compatible MASHINA emulator flash backup");
  const sectors={},seen=new Set();
  for(const row of obj.sectors){
    if(!row||typeof row!=="object"||!allowed(row.offset)||seen.has(row.offset))
      throw Error("Invalid or duplicate flash sector");
    seen.add(row.offset);
    const data=unb64(row.bytes);
    if(typeof row.checksum!=="string"||row.checksum!==hash(data))
      throw Error("Backup checksum mismatch at "+row.offset);
    if(data.every(v=>v===255))throw Error("Unexpected empty sector");
    sectors[row.offset]=data;
  }
  return sectors;
}

/* Existing emulator browser persistence uses a sparse offset -> base64 map.
 * Preserve that exact on-disk representation so ordinary emulator startup
 * reads restored backups without migration or another import step. */
export function persistentEmulatorFlash(sectors) {
  const checked=decodeEmulatorFlash(encodeEmulatorFlash(sectors));
  const data={};
  for(const [off,bytes] of Object.entries(checked))data[off]=b64(bytes);
  return JSON.stringify(data);
}
export function readPersistentEmulatorFlash(raw) {
  if(raw===null)return {};
  const parsed=JSON.parse(raw);
  if(!parsed||typeof parsed!=="object"||Array.isArray(parsed))
    throw Error("Stored emulator flash is invalid");
  const sectors={};
  for(const [key,base64] of Object.entries(parsed)) {
    const off=Number(key);
    if(!allowed(off)||String(off)!==key||typeof base64!=="string")
      throw Error("Invalid stored flash sector");
    sectors[off]=unb64(base64);
  }
  return decodeEmulatorFlash(encodeEmulatorFlash(sectors));
}
export function restorePersistentEmulatorFlash(storage,key,source) {
  // Caller must request explicit confirmation BEFORE calling. Validate and
  // serialize everything before touching the existing stored session.
  const expected=decodeEmulatorFlash(encodeEmulatorFlash(source));
  const serialized=persistentEmulatorFlash(expected);
  const before=storage.getItem(key);
  try{
    storage.setItem(key,serialized); // one atomic browser storage value
    const actual=readPersistentEmulatorFlash(storage.getItem(key));
    const expectedJSON=JSON.stringify(encodeEmulatorFlash(expected));
    if(JSON.stringify(encodeEmulatorFlash(actual))!==expectedJSON)
      throw Error("Restored browser flash does not match the selected backup");
  }catch(error){
    // Restore old bytes (if a mocked/faulty storage engine ever reports a
    // successful write but gives a different read-back).
    try {
      if(before===null)storage.removeItem(key);
      else storage.setItem(key,before);
    }catch {}
    throw Error("Permanent restore failed; existing data recovery attempted: "+error.message);
  }
  return Object.keys(expected).length;
}
