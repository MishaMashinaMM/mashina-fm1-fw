/* SPDX-License-Identifier: GPL-3.0-only
 * Browser USB WebMIDI adapter for the experimental LIB32 page.
 * Never sends a command unless the user explicitly clicks Connect or Upload.
 */
export async function connectLibraryMidi(){
  if(!navigator.requestMIDIAccess)throw new Error("WebMIDI is unavailable; use Chrome or Edge over HTTPS.");
  const access=await navigator.requestMIDIAccess({sysex:true});
  const outputs=[...access.outputs.values()],inputs=[...access.inputs.values()];
  if(!outputs.length||!inputs.length)throw new Error("No MIDI input/output pair. Connect the FM1 by USB.");
  const matches=outputs.map((out,index)=>{
    let input=inputs.find(x=>x.name===out.name&&x.manufacturer===out.manufacturer)
      ||inputs.find(x=>x.name===out.name)||inputs[index]||null;
    return {out,input,name:out.name||"MIDI output"};
  }).filter(x=>x.input);
  if(!matches.length)throw new Error("No usable MIDI port pair found.");
  const ranked=matches.map((x,i)=>({x,i,hit:/fm.?1|jiant|mashina|felucca|m.vave/i.test(x.name)}));
  let pick=ranked.find(x=>x.hit)?.i??(matches.length===1?0:-1);
  if(matches.length>1){
    const message=matches.map((x,i)=>`${i+1}. ${x.name} / input: ${x.input.name}`).join("\n");
    const choice=prompt("Choose the FM1 MIDI port pair:\n"+message, pick>=0?String(pick+1):"");
    if(choice===null)throw new Error("Connection cancelled.");
    const n=Number(choice);
    if(!Number.isInteger(n)||n<1||n>matches.length)throw new Error("Invalid MIDI port selection.");
    pick=n-1;
  }
  if(pick<0)throw new Error("Select a MIDI port pair first.");
  const {out,input}=matches[pick];
  let waiting=null,closed=false;
  const old=input.onmidimessage;
  input.onmidimessage=e=>{
    if(!waiting||closed)return;
    const d=Uint8Array.from(e.data||[]);
    if(d.length<6||d[0]!==240||d[1]!==125||d[2]!==70||d[3]!==76||
       d[d.length-1]!==247||d[4]!==waiting.cmd)return;
    const body=d.slice(5,-1);
    if(waiting.match&&!waiting.match(body))return; // late chunk from an older GET
    const w=waiting;waiting=null;clearTimeout(w.timer);
    w.resolve(body);
  };
  /* Compatible with both request(cmd,args) and the chunked full-backup
   * adapter request(cmd,args,{timeout,retries,match}). Only read commands
   * should be retried: writes remain single-shot to avoid duplicate saves. */
  async function request(cmd,args=[],options={}){
    if(closed||out.state==="disconnected"||input.state==="disconnected")
      throw new Error("FM1 MIDI connection lost.");
    if(waiting)throw new Error("Another library transfer is in progress.");
    const payload=Uint8Array.from(args);
    if(payload.some(x=>x>127))throw new Error("A WebMIDI payload byte exceeds 127.");
    const retries=options.retries??0;
    const timeout=options.timeout??3500;
    for(let attempt=0;attempt<=retries;attempt++){
      try{
        return await new Promise((resolve,reject)=>{
          const timer=setTimeout(()=>{
            if(waiting?.cmd===cmd){
              waiting=null;
              const error=new Error("FM1 did not respond to command "+cmd);
              error.timeout=true;
              reject(error);
            }
          },timeout);
          waiting={cmd,match:options.match,resolve,reject,timer};
          try{out.send([240,125,70,76,cmd,...payload,247]);}
          catch(error){clearTimeout(timer);waiting=null;reject(error);}
        });
      }catch(error){
        if(!error.timeout||attempt===retries||closed)throw error;
      }
    }
  }
  return {name:out.name,request,close(){
    if(closed)return;closed=true;input.onmidimessage=old;
    if(waiting){const w=waiting;waiting=null;clearTimeout(w.timer);w.reject(new Error("MIDI connection closed"));}
  }};
}
