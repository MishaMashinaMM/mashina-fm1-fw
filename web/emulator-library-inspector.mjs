/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only inspector for experimental MPR1 emulator flash snapshots.
 * Mirrors the firmware's ST (USER), MPRS and LIB1 A/B commit selection.
 * This module NEVER mutates the input NOR, device, or Web Library's editor.
 */
import {decodeEmulatorFlash} from "./emu/emulator-flash-backup.mjs";
import {decodeProject,projectsBackup} from "./projects-library-codec.mjs";
import {decodeRecord} from "./presets-library-codec.mjs";
import {importFactoryCatalog,defaultFactorySelection} from "./factory-catalog-selection.mjs";
const SECTOR=4096, EMPTY=255;
const u16=(d,p)=>d[p]|d[p+1]<<8;
const u32=(d,p)=>(d[p]|d[p+1]<<8|d[p+2]<<16|d[p+3]<<24)>>>0;
const need=(ok,msg)=>{if(!ok)throw Error(msg);};
function crc32(d){
  let c=0xffffffff;
  for(const b of d){c^=b;for(let j=0;j<8;j++)c=(c>>>1)^((c&1)?0xedb88320:0);}
  return (~c)>>>0;
}
const erased=d=>d.every(x=>x===255);
const zero=d=>d.every(x=>x===0);
const newer=(a,b)=>(a!==b && ((b-a)>>>0)<0x80000000);
function sectorAt(sectors,off){return sectors[off]??new Uint8Array(SECTOR).fill(255);}
function pick(sectors,addresses,parse,label){
  const choices=addresses.map((off,copy)=>{
    const data=sectorAt(sectors,off);
    return {copy,empty:erased(data),record:parse(data,copy)};
  });
  const valid=choices.filter(x=>x.record);
  if(!valid.length){
    need(choices.every(x=>x.empty),label+": no valid committed bank; non-erased data may be damaged");
    return null;
  }
  let winner=valid[0];
  for(const c of valid.slice(1))if(newer(winner.record.sequence,c.record.sequence))winner=c;
  return winner.record.data;
}
function parseMprs(d,copy,bank){
  if(u32(d,0)!==0x3153504d||u32(d,4)!==bank||u32(d,12)!==3648)return null;
  const body=d.slice(256,256+3648);
  return crc32(body)===u32(d,16)?{sequence:u32(d,8),data:body}:null;
}
function parseLib(d,copy,bank){
  if(u32(d,0)!==0x3142494c||u32(d,4)!==1||u32(d,8)!==bank||u32(d,16)!==2432)return null;
  const body=d.slice(256,256+2432);
  return crc32(body)===u32(d,20)?{sequence:u32(d,12),data:body}:null;
}
function parseSt(d,copy,type){
  if(u32(d,0)!==0x554c4546||u16(d,4)!==type||u16(d,6)!==copy)return null;
  const len=u32(d,12);
  if(len>3840||crc32(d.slice(0,28))!==u32(d,28))return null;
  const body=d.slice(256,256+len);
  return crc32(body)===u32(d,16)?{sequence:u32(d,8),data:body}:null;
}
const ascii=d=>String.fromCharCode(...d).replace(/\x00+$/,"");
function userName(d){
  const chars=d.slice(4,16);
  let ended=false;
  for(const c of chars) {
    if(c===0)ended=true;
    else need(!ended&&c>=32&&c<=126,"invalid USER preset name");
  }
  return ascii(chars);
}
export function inspectEmulatorFlash(file){
  const sectors=decodeEmulatorFlash(file),projects=[],library=[],users=[];
  // MPR1 bank: sixteen pairs, each containing two fixed 1824-byte sections.
  for(let bank=0;bank<16;bank++){
    const base=0xa0000+bank*8192;
    const payload=pick(sectors,[base,base+4096],
      (d,c)=>parseMprs(d,c,bank),"PROJECT bank "+bank);
    for(let i=0;i<2;i++){
      const index=bank*2+i;
      const raw=payload?payload.slice(i*1824,(i+1)*1824):new Uint8Array(1824);
      if(zero(raw)){projects.push(null);continue;}
      need(!erased(raw),"PROJECT "+(index+1)+": unexpected erased record");
      try {
        const info=decodeProject(Array.from(raw));
        need(info.section===index%4,"PROJECT "+(index+1)+": wrong section number");
        projects.push(info.recordBytes);
      }catch(e){throw Error("PROJECT "+(index+1)+": "+e.message);}
    }
  }
  // LIB1 bank: four pairs, eight complete 304-byte override sounds each.
  for(let bank=0;bank<4;bank++){
    const base=0xea000+bank*8192;
    const payload=pick(sectors,[base,base+4096],
      (d,c)=>parseLib(d,c,bank),"LIBRARY bank "+bank);
    for(let i=0;i<8;i++){
      const slot=bank*8+i,raw=payload?payload.slice(i*304,(i+1)*304):new Uint8Array(304);
      if(zero(raw)){library.push({slot:slot+33,status:"factory",name:"Factory fallback",engine:null});continue;}
      try{
        const value=decodeRecord(raw);
        library.push({slot:slot+33,status:"overridden",name:value.name,engine:value.engine,recordBytes:Array.from(raw)});
      }catch(e){throw Error("LIBRARY "+String(slot+33).padStart(3,"0")+": "+e.message);}
    }
  }
  // USER: ordinary FELU ST objects 5 and 6 at 0xDC000..0xDFFFF.
  for(let bank=0;bank<2;bank++){
    const base=0xdc000+bank*8192;
    const payload=pick(sectors,[base,base+4096],
      (d,c)=>parseSt(d,c,5+bank),"USER bank "+bank);
    if(payload!==null)need(payload.length===3080&&u32(payload,0)===0x31425055&&
      u16(payload,4)===192&&u16(payload,6)===16,"USER bank "+bank+": invalid bank header");
    for(let i=0;i<16;i++){
      const slot=bank*16+i,raw=payload?payload.slice(8+i*192,8+(i+1)*192):new Uint8Array(192);
      if(raw[0]!==0xa5){
        need(zero(raw),"USER "+(slot+1)+": nonempty invalid record; refusing to discard unrecognized data");
        users.push({slot:slot+1,status:"empty",name:"",engine:null});continue;
      }
      const version=raw[1],count=raw[3];
      need(version>=1&&version<=5&&raw[2]<21&&count>=8&&count<=(version>=4?144:72),
        "USER "+(slot+1)+": invalid preset header");
      if(version>=4)need(raw.slice(16,16+count).every(x=>x<=191),"USER "+(slot+1)+": invalid parameter");
      need(raw[4]!==0,"USER "+(slot+1)+": missing name");
      users.push({slot:slot+1,status:"saved",name:userName(raw),engine:raw[2],version,
        recordBytes:Array.from(raw),
        note:"192-byte USER record; FM6/drum companions may be stored separately"});
    }
  }
  return {format:"mashina-emulator-inspection",version:1,
    users,library,projects:projectsBackup(projects),sectorCount:Object.keys(sectors).length};
}

/* Resolve only *unwritten* LIBRARY slots against the protected, exact
 * 107-source factory catalog. Keep status="factory": the name is compiled
 * into the firmware, not part of the exported flash snapshot. A saved
 * 304-byte override always wins over the compiled fallback. */
export function resolveFactoryFallbacks(inspection,catalogInput){
  const catalog=importFactoryCatalog(catalogInput);
  const selection=defaultFactorySelection(catalog);
  need(Array.isArray(inspection?.library)&&inspection.library.length===32,
       "Expected 32 inspected LIBRARY positions");
  need(selection.length===32&&selection.every(Number.isInteger),
       "Protected catalog does not define all 32 factory defaults");
  const library=inspection.library.map((entry,i)=>{
    need(entry.slot===i+33&&["factory","overridden"].includes(entry.status),
         "Unexpected LIBRARY position or state");
    if(entry.status==="overridden")return {...entry};
    const patch=catalog.presets[selection[i]];
    need(patch?.initialLibrarySlot===entry.slot,
         "Factory catalog slot mapping mismatch at "+entry.slot);
    return {...entry,name:patch.name,engine:patch.engine,
      engineName:patch.engineName,factoryIndex:patch.factoryIndex};
  });
  return {...inspection,library};
}
