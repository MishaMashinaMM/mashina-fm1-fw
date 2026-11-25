/* SPDX-License-Identifier: GPL-3.0-only
 * Read-only 8-project × A-D reference inspection for the Web Library.
 * Consumes a validated projects-only backup; no WebMIDI writes or mutation.
 */
import {decodeProject,restoreProjectsBackup,PRESET_COUNT} from "./projects-library-codec.mjs";
const pad=n=>String(n).padStart(3,"0");
export function inspectProjectReferences(file) {
  const positions=restoreProjectsBackup(file);
  const usage=Array.from({length:PRESET_COUNT},()=>[]);
  const sections=positions.map((raw,index)=>{
    const project=Math.floor(index/4)+1;
    const section="ABCD"[index%4];
    if(raw===null)return {project,section,empty:true,name:"",references:[],tracks:[]};
    const decoded=decodeProject(raw);
    const tracks=decoded.refs.map((ref,i)=>{
      const label=ref===255?"UNASSIGNED":ref<32?"USER "+pad(ref+1):"LIBRARY "+pad(ref+1);
      if(ref!==255)usage[ref].push({project,section,track:i+1});
      return {track:i+1,ref,label};
    });
    return {project,section,empty:false,name:decoded.name,
            references:decoded.refs,tracks,settingsBytes:decoded.settingsBytes.length};
  });
  return {sections,usage};
}
