/* SPDX-License-Identifier: GPL-3.0-only
 * Real Chrome (headless) end-to-end smoke of the *packaged* Web Library.
 * Verifies modular page loading, file chooser, preset queue and exported JSON.
 */
import assert from "node:assert/strict";
import fs from "node:fs";
import {spawn,spawnSync} from "node:child_process";
import http from "node:http";
import {chromium} from "playwright-core";
import {encodeEmulatorFlash,decodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {editEmulatorUserSnapshot,crc32} from "../web/emulator-snapshot-editor.mjs";
import {encodeProject,decodeProject} from "../web/projects-library-codec.mjs";
import {inspectEmulatorFlash} from "../web/emulator-library-inspector.mjs";
import {BACKUP_IDS,bkCrc} from "../web/fm1backup.js";
import {encodeRecord} from "../web/presets-library-codec.mjs";
const dir=process.argv[2]??"build/presets-library-smoke";
const which=name=>{
  const r=spawnSync("which",[name],{encoding:"utf8"});
  return r.status===0?r.stdout.trim():null;
};
const exe=process.env.CHROME_PATH||which("google-chrome")||which("chromium")||which("chromium-browser");
assert(exe,"Chrome/Chromium executable unavailable for Web Library smoke");
const server=spawn("python3",["-m","http.server","8765","--bind","127.0.0.1","--directory",dir],
  {stdio:"ignore"});
let browser;
try{
  let live=false;
  /* Node 22's built-in undici fetch can assert in Parser.finish when a
   * just-starting http.server closes/renews a keep-alive response. Use an
   * ordinary draining HTTP request for the readiness probe instead. */
  for(let i=0;i<80;i++){
    try {
      live=await new Promise(resolve=>{
        const req=http.get("http://127.0.0.1:8765/",{agent:false},res=>{
          res.resume();res.on("end",()=>resolve(res.statusCode===200));
          res.on("error",()=>resolve(false));
        });
        req.setTimeout(1500,()=>req.destroy());
        req.on("error",()=>resolve(false));
      });
      if(live)break;
    }catch {}
    await new Promise(r=>setTimeout(r,100));
  }
  assert(live,"Web Library HTTP server did not start");
  browser=await chromium.launch({executablePath:exe,headless:true,
    args:["--no-sandbox","--disable-dev-shm-usage"]});
  const page=await browser.newPage({acceptDownloads:true,viewport:{width:1536,height:864}});
  const errors=[];
  page.on("pageerror",e=>errors.push(e.message));
  await page.goto("http://127.0.0.1:8765/",{waitUntil:"networkidle"});
  // New default manager: all controls start locked; imported source data
  // must never cause device writes or unlock restoration offline.
  await page.waitForFunction(()=>document.querySelectorAll("#user-source option").length>100);
  assert.equal(await page.locator("#user-apply").isDisabled(),true);
  assert.equal(await page.locator("#library-apply").isDisabled(),true);
  assert.equal(await page.locator("#user-source optgroup[label='Factory catalog'] option").count(),107);
  assert.equal(await page.locator("#write-state").textContent(),"No verified backup");
  const guide=await page.request.get("http://127.0.0.1:8765/guide.html");
  assert(guide.ok(),"Packaged Web Library guide missing");
  assert.match(await guide.text(),/Verify saved complete backup/);
  assert.equal(await page.locator("#projects-apply").isDisabled(),true);
  assert.equal(await page.locator("#full-apply").isDisabled(),true);
  const rec=Array.from(encodeRecord({name:"UI TEST",engine:0,params:Array(127).fill(2)}));
  await page.locator("#user-import").setInputFiles({name:"test.json",mimeType:"application/json",buffer:Buffer.from(JSON.stringify({patches:[{slot:0,recordBytes:rec}]}))});
  await page.waitForFunction(()=>Array.from(document.querySelectorAll("#user-source option")).some(x=>x.textContent==="Imported · UI TEST"));
  assert.equal(await page.locator("#user-apply").isDisabled(),true);
  for(const tab of ["library","projects","backup","user"]){
    await page.locator("[data-tab="+tab+"]").click();
    assert.equal(await page.locator("#tab-"+tab).isVisible(),true);
  }
  // A 1536x864 physical screen can have a smaller usable Chrome viewport.
  // Check both the requested viewport and a browser-chrome-sized viewport.
  for(const height of [864,728]){
    await page.setViewportSize({width:1536,height});
    for(const tab of ["user","library","projects","backup"]){
      await page.locator("[data-tab="+tab+"]").click();
      const layout=await page.evaluate(tab=>{
        const s=document.querySelector("#tab-"+tab);
        const panel=s.querySelector(".side");
        return {pageHeight:document.documentElement.scrollHeight,viewport:innerHeight,
          pageWidth:document.documentElement.scrollWidth,width:innerWidth,
          rows:[...s.querySelectorAll("tbody")].map(b=>b.rows.length),
          clippedTables:[...s.querySelectorAll(".scroll")].some(b=>b.scrollHeight>b.clientHeight+1),
          clippedSide:panel?panel.scrollHeight>panel.clientHeight+1:false,
          bottom:s.getBoundingClientRect().bottom};
      },tab);
      assert(layout.pageHeight<=height+1 && layout.pageWidth<=1536+1,tab+" page overflow: "+JSON.stringify(layout));
      assert(layout.bottom<=height+1,tab+" content below viewport");
      if(tab!=="backup"){
        assert.deepEqual(layout.rows,[16,16],tab+" must show all slots");
        assert(!layout.clippedTables,tab+" unnecessarily scrolls slot tables");
        assert(!layout.clippedSide,tab+" action controls do not fit: "+JSON.stringify(layout));
      }
    }
  }
  // Test real modal top-layer locking with a pending WebMIDI permission
  // request, a complete simulated 43-object backup, and a failed backup.
  // The fixture fills all 32 project sections with long reference lists.
  // No physical device, private backup, or write command is used.
  await page.evaluate(async()=>{
    const {bkU32,bkR32,bkCrc,bkPack,BACKUP_IDS}=await import("/fm1backup.js");
    const {encodeProject,MAX_RECORD}=await import("/projects-library-codec.mjs");
    const {encodeRecord,pack7}=await import("/presets-library-codec.mjs");
    const pad=r=>{const d=new Uint8Array(MAX_RECORD);d.set(r);return d;};
    const records=Array.from({length:32},(_,i)=>pad(encodeProject({
      section:i%4,refs:[0,32,33,34,35,36],name:"PROJECT "+String(i+1).padStart(2,"0"),
      settingsBytes:[1,2,3]
    })));
    const sound=encodeRecord({name:"TWELVE CHARS",engine:0,params:Array(127).fill(2)});
    const user=new Uint8Array(3080);user.set([85,80,66,49,192,0,16,0]);user.set(sound.slice(0,192),8);
    const data=new Map(BACKUP_IDS.map(id=>[id,
      id===0?records[0]:id===1?Uint8Array.of(1,2,3,4):id>=2&&id<=5?records[id-2]:
      id===6?user:id===10?Uint8Array.of(83,79,78,71,2,0,0,0):id>=40?records[id-40]:new Uint8Array(0)]));
    const input={name:"FM1 UI mock",manufacturer:"Test",state:"connected",onmidimessage:null};
    window.failMockBackup=false;window.mockWrites=0;
    const output={name:input.name,manufacturer:input.manufacturer,state:"connected",
      send(msg){
        const cmd=msg[4],args=msg.slice(5,-1);let reply;
        if(cmd===89)reply=[1,32,1];
        else if(cmd===95)reply=[1,32,1,0];
        else if(cmd===65)reply=window.failMockBackup?[1,2,0]:[1,0,43,...BACKUP_IDS.flatMap(id=>{
          const bytes=data.get(id);return [id,...bkU32(bytes.length),...bkU32(bkCrc(bytes))];})];
        else if(cmd===66){const id=args[0],off=bkR32(args,1),n=args[6]|args[7]<<7;
          reply=[id,0,...bkU32(off),n&127,n>>>7,...bkPack(data.get(id).slice(off,off+n))];}
        else if(cmd===94)reply=args[0]===0?[0,0,...pack7(sound)]:[args[0],1];
        else if(cmd===91)reply=[args[0],3];
        else{window.mockWrites++;throw Error("Unexpected UI test write "+cmd);}
        setTimeout(()=>input.onmidimessage?.({data:[240,125,70,76,cmd,...reply,247]}),cmd===65?150:12);
      }};
    Object.defineProperty(navigator,"requestMIDIAccess",{configurable:true,value:()=>new Promise(resolve=>{
      window.releaseMidi=()=>resolve({inputs:new Map([[1,input]]),outputs:new Map([[1,output]])});
    })});
  });
  await page.locator("#connect").click();
  await page.waitForFunction(()=>typeof window.releaseMidi==="function");
  assert.equal(await page.locator("#operation").evaluate(d=>d.matches(":modal")),true);
  assert.equal(await page.locator("#op-close").isDisabled(),true);
  const rect=await page.locator("#operation").boundingBox();
  assert(Math.abs(rect.x+rect.width/2-1536/2)<2 && Math.abs(rect.y+rect.height/2-728/2)<2,"operation not centered");
  await page.keyboard.press("Escape");
  assert.equal(await page.locator("#operation").evaluate(d=>d.open),true);
  const blocked=await page.locator("[data-tab=user]").click({trial:true,timeout:250}).then(()=>false,()=>true);
  assert(blocked,"background navigation must be blocked by active modal");
  await page.evaluate(()=>window.releaseMidi());
  await page.waitForFunction(()=>!document.querySelector("#op-close").disabled);
  assert.match(await page.locator("#op-title").textContent(),/complete/);
  await page.locator("#op-close").click();
  assert.equal(await page.locator("#tab-backup").isVisible(),true);
  const downloaded=page.waitForEvent("download");
  await page.locator("#backup-capture").click();
  assert.equal(await page.locator("#operation").evaluate(d=>d.matches(":modal")),true);
  assert.equal(await page.locator("#op-close").isDisabled(),true);
  await page.waitForFunction(()=>document.querySelector("#op-progress").hasAttribute("value")&&document.querySelector("#op-progress").value>0);
  assert.match(await page.locator("#op-detail").textContent(),/% of current phase/);
  assert.match(await page.locator("#op-warning").textContent(),/Do not disconnect USB/);
  const captured=await downloaded;
  assert.equal(captured.suggestedFilename(),"mashina-full-fm1-backup.json");
  await page.waitForFunction(()=>!document.querySelector("#op-close").disabled);
  assert.equal(await page.locator("#operation").getAttribute("data-state"),"ok");
  await page.locator("#op-close").click();
  await page.locator("#backup-verify").setInputFiles(await captured.path());
  await page.waitForFunction(()=>document.querySelector("#backup-summary").textContent.includes("Backup verified"));
  assert.match(await page.locator("#write-state").textContent(),/Backup verified · Writes unlocked/);
  assert.equal(await page.locator("#user-source optgroup[label='Device USER'] option").count(),1);
  for(const tab of ["user","library","projects"]){
    await page.locator("[data-tab="+tab+"]").click();
    assert.equal(await page.locator("#tab-"+tab+" tbody tr").count(),32);
    const metrics=await page.locator("#tab-"+tab).evaluate(s=>({
      tables:[...s.querySelectorAll(".scroll")].map(x=>x.scrollHeight-x.clientHeight),
      page:document.documentElement.scrollHeight-innerHeight
    }));
    assert(metrics.page<=1&&metrics.tables.every(x=>x<=1),tab+" populated layout overflow "+JSON.stringify(metrics));
  }
  await page.locator("[data-tab=projects]").click();
  assert.match(await page.locator("#project-rows-second tr").last().textContent(),/P8:D.*PROJECT 32/);
  await page.locator("[data-tab=user]").click();
  assert.match(await page.locator("#user-slots tr").first().textContent(),/TWELVE CHARS/);
  assert.match(await page.locator("#user-slots tr").first().locator("td").last().locator("span").getAttribute("title"),/P8:D\/T1/);
  // Confirm all four write actions use the themed dialog; cancellation must
  // preserve the verified backup, the queue and zero USB writes.
  const pressCancel=async id=>{
    await page.locator(id).click();
    assert.equal(await page.locator("#confirmation").evaluate(d=>d.matches(":modal")),true);
    assert.equal(await page.locator("#confirm-cancel").textContent(),"CANCEL");
    assert.equal(await page.locator("#confirmation input").count(),0,"Confirmation must not require typing");
    await page.locator("#confirm-cancel").click();
    assert.equal(await page.locator("#confirmation").evaluate(d=>d.open),false);
    assert.match(await page.locator("#write-state").textContent(),/Writes unlocked/);
    assert.equal(await page.evaluate(()=>window.mockWrites),0);
  };
  await page.locator("#user-source").selectOption("I0");
  await page.locator("#user-target").selectOption("1");
  await page.locator("#user-add").click();
  await pressCancel("#user-apply");
  assert.match(await page.locator("#user-queue").textContent(),/002/);
  await page.locator("[data-tab=library]").click();
  await page.locator("#library-source").selectOption("F0");
  await page.locator("#library-target").selectOption("31");
  await page.locator("#library-add").click();
  await pressCancel("#library-apply");
  assert.match(await page.locator("#library-queue").textContent(),/064/);
  const baseBackup=JSON.parse(fs.readFileSync(await captured.path(),"utf8"));
  const sections=structuredClone(baseBackup.projects);sections[31]=null;
  await page.locator("[data-tab=projects]").click();
  await page.locator("#projects-import").setInputFiles({name:"changed-projects.json",mimeType:"application/json",buffer:Buffer.from(JSON.stringify({format:"mashina-projects-mpr1",version:1,projects:8,sectionsPerProject:4,sections}))});
  await page.locator("#projects-all").click();
  await pressCancel("#projects-apply");
  const source=structuredClone(baseBackup);
  const old=decodeProject(source.currentPerformance);
  source.currentPerformance=encodeProject({...old,name:"OTHER LIVE",settingsBytes:old.settingsBytes});
  const live=new Uint8Array(1824);live.set(source.currentPerformance);
  Object.assign(source.archive.objects.find(o=>o.id===0),{size:live.length,crc:bkCrc(live),data:Buffer.from(live).toString("base64")});
  await page.locator("[data-tab=backup]").click();
  await page.locator("#full-import").setInputFiles({name:"changed-live.json",mimeType:"application/json",buffer:Buffer.from(JSON.stringify(source))});
  await pressCancel("#full-apply");
  await page.locator("#full-apply").click();
  assert.equal(await page.locator("#confirm-accept").textContent(),"RESTORE");
  assert.equal(await page.locator("#confirm-accept").isEnabled(),true);
  assert.equal(await page.locator("#confirm-message").textContent(),"This may replace USER sounds, LIBRARY overrides, projects and settings. Keep the current backup. Click RESTORE to confirm.");
  await page.keyboard.press("Escape");
  assert.equal(await page.locator("#confirmation").evaluate(d=>d.open),false);
  assert.equal(await page.evaluate(()=>window.mockWrites),0);
  // Prove direct RESTORE starts the protected transfer without typing.
  // Reject the first write on the mock, then verify the error modal.
  await page.locator("#full-apply").click();
  await page.locator("#confirm-accept").click();
  await page.waitForFunction(()=>document.querySelector("#operation").open);
  await page.waitForFunction(()=>!document.querySelector("#op-close").disabled);
  assert.equal(await page.locator("#operation").getAttribute("data-state"),"error");
  assert.equal(await page.evaluate(()=>window.mockWrites),1);
  await page.locator("#op-close").click();
  await page.evaluate(()=>window.failMockBackup=true);
  await page.locator("#backup-capture").click();
  assert.equal(await page.locator("#operation").evaluate(d=>d.matches(":modal")),true);
  assert.equal(await page.locator("#op-close").isDisabled(),true);
  await page.waitForFunction(()=>!document.querySelector("#op-close").disabled);
  assert.equal(await page.locator("#operation").getAttribute("data-state"),"error");
  assert.equal(await page.locator("#op-title").textContent(),"Operation stopped");
  await page.locator("#op-close").click();
  assert.equal(await page.locator("#full-apply").isDisabled(),true);
  await page.locator("#disconnect").click();
  assert.equal(await page.evaluate(()=>window.mockWrites),1);
  console.log("PASS: themed write confirmations, cancel/Escape preserve state, RESTORE starts protected transfer without typing");
  console.log("PASS: compact desktop layout, all 32 slots per tab, native centered operation lock, success/error acknowledgement");
  assert.deepEqual(errors,[],"New manager startup errors");
  await page.goto("http://127.0.0.1:8765/advanced.html",{waitUntil:"networkidle"});
  await page.waitForFunction(()=>document.querySelectorAll("#emu-source-select option").length===107);
  assert.deepEqual(errors,[],"Web Library startup JavaScript errors");
  assert.equal(await page.locator("#backup-legacy").isDisabled(),true);
  assert.equal(await page.locator("#probe-flash").isDisabled(),true);
  /* Offline legacy USER extraction needs no MIDI permission. The browser
   * must process the saved original archive and offer a download without
   * changing any live device slot or activating physical write controls. */
  const legacySound=Uint8Array.from(encodeRecord({
    name:"ORIGINAL",engine:0,params:Array(127).fill(3)
  }));
  const oldUser=new Uint8Array(3080);
  oldUser.set([0x55,0x50,0x42,0x31,192,0,16,0]);
  oldUser.set(legacySound.slice(0,192),8);
  const legacyArchive={
    format:"felucca-backup",version:1,firmware:"original test",
    objects:BACKUP_IDS.map(id=>{
      const data=id===0||id===1?Uint8Array.of(1,2,3,4):
                 id===6?oldUser:new Uint8Array(0);
      return {id,size:data.length,crc:data.length?bkCrc(data):0,
        data:Buffer.from(data).toString("base64")};
    })
  };
  const [legacyDownload]=await Promise.all([
    page.waitForEvent("download"),
    page.locator("#extract-legacy-user").setInputFiles({
      name:"original-backup.json",mimeType:"application/json",
      buffer:Buffer.from(JSON.stringify(legacyArchive))
    })
  ]);
  assert.equal(legacyDownload.suggestedFilename(),"mashina-original-user-sounds-304.json");
  const extracted=JSON.parse(fs.readFileSync(await legacyDownload.path(),"utf8"));
  assert.deepEqual(extracted.patches.map(p=>p.slot),[0]);
  assert.deepEqual(extracted.patches[0].recordBytes,Array.from(legacySound));
  assert.equal(await page.locator("#probe-flash").isDisabled(),true);
  assert.deepEqual(errors,[],"Browser errors after offline USER archive conversion");
  const [freshDownload]=await Promise.all([
    page.waitForEvent("download"),
    page.locator("#prepare-legacy-emulator").setInputFiles({
      name:"original-backup.json",mimeType:"application/json",
      buffer:Buffer.from(JSON.stringify(legacyArchive))
    })
  ]);
  assert.equal(freshDownload.suggestedFilename(),
    "mashina-fresh-mpr1-emulator-with-original-user.json");
  const newNor=JSON.parse(fs.readFileSync(await freshDownload.path(),"utf8"));
  const checked=inspectEmulatorFlash(newNor);
  assert.equal(checked.users[0].name,"ORIGINAL");
  assert.equal(checked.users.filter(x=>x.status==="saved").length,1);
  assert.equal(checked.library.filter(x=>x.status==="factory").length,32);
  assert.equal(checked.projects.sections.filter(Boolean).length,0);
  assert.equal(await page.locator("#probe-flash").isDisabled(),true);
  assert.deepEqual(errors,[],"Browser errors after fresh offline MPR1 snapshot");

  const target=JSON.stringify(encodeEmulatorFlash({}));
  await page.locator("#inspect-emu").setInputFiles({
    name:"target.json",mimeType:"application/json",buffer:Buffer.from(target)});
  await page.waitForFunction(()=>document.querySelector("#emu-import-status").textContent.includes("Loaded target.json"));
  assert.equal(await page.locator("#emu-users tr").count(),32);
  assert.equal(await page.locator("#emu-library tr").count(),32);
  assert.equal(await page.locator("#emu-projects tr").count(),32);
  await page.selectOption("#emu-source-type","factory");
  await page.selectOption("#emu-source-select","F1");
  await page.selectOption("#emu-target-type","library");
  await page.selectOption("#emu-target-select","33");
  assert.equal(await page.locator("#emu-stage").isEnabled(),true);
  await page.locator("#emu-stage").click();
  assert.equal(await page.locator("#emu-pending tr").count(),1);
  const [download]=await Promise.all([
    page.waitForEvent("download"),
    page.locator("#emu-export-patched").click()
  ]);
  assert.match(download.suggestedFilename(),/selective-restore/);
  const patched=JSON.parse(fs.readFileSync(await download.path(),"utf8"));
  const inspection=inspectEmulatorFlash(patched);
  assert.equal(inspection.library[0].name,"THALES");
  assert.equal(inspection.library[0].status,"overridden");
  assert(inspection.library.slice(1).every(x=>x.status==="factory"));
  assert.equal(await page.evaluate(()=>localStorage.getItem("felucca-emu-flash")),null,
    "Read-only editor unexpectedly wrote emulator browser saves");
  assert.deepEqual(errors,[],"Browser errors after queue/download");
  console.log("PASS: real Chrome loads standalone modules, imports emulator JSON, shows 32+32+32, queues THALES and downloads checked NOR without browser writes");
  // Second end-to-end case uses one saved USER sound and one MPR1 section.
  // Queue a USER copy first, then reference that newly saved slot in P1:A.
  const withUser=editEmulatorUserSnapshot(patched,[{
    slot:1,recordBytes:inspection.library[0].recordBytes
  }]).snapshot;
  const sectors=decodeEmulatorFlash(withUser),sec=new Uint8Array(4096).fill(255);
  const body=new Uint8Array(3648);
  body.set(encodeProject({section:0,name:"BROWSER TEST",
    refs:[0,32,255,255,255,255],settingsBytes:[]}));
  const w32=(p,n)=>{for(let i=0;i<4;i++)sec[p+i]=(n>>>(8*i))&255;};
  w32(0,0x3153504d);w32(4,0);w32(8,1);w32(12,3648);
  w32(16,crc32(body));sec.set(body,256);
  sectors[0xa0000]=sec;
  const full=JSON.stringify(encodeEmulatorFlash(sectors));
  assert.equal(inspectEmulatorFlash(full).users[0].name,"THALES");
  await page.locator("#inspect-emu").setInputFiles({
    name:"expanded.json",mimeType:"application/json",buffer:Buffer.from(full)});
  await page.waitForFunction(()=>document.querySelector("#emu-import-status").textContent.includes("Loaded expanded.json"));
  await page.selectOption("#emu-source-type","user");
  await page.selectOption("#emu-source-select","U1");
  await page.selectOption("#emu-target-type","user");
  await page.selectOption("#emu-target-select","2");
  await page.locator("#emu-stage").click();
  assert.equal(await page.locator("#emu-pending tr").count(),1);
  await page.locator("#emu-project-editor summary").click();
  await page.selectOption("#emu-project-position","0");
  await page.selectOption("#emu-project-track","1");
  await page.selectOption("#emu-project-ref","1");
  await page.locator("#emu-project-stage").click();
  assert.equal(await page.locator("#emu-project-pending tr").count(),1);
  const [second]=await Promise.all([
    page.waitForEvent("download"),
    page.locator("#emu-export-patched").click()
  ]);
  const fullModified=JSON.parse(fs.readFileSync(await second.path(),"utf8"));
  const finalView=inspectEmulatorFlash(fullModified);
  assert.equal(finalView.users[0].name,"THALES");
  assert.equal(finalView.users[1].name,"THALES");
  assert.equal(decodeProject(finalView.projects.sections[0]).refs[0],1);
  assert.equal(finalView.library[0].name,"THALES");
  assert.deepEqual(errors,[],"Browser errors after USER/project staging and export");
  console.log("PASS: real Chrome queues a USER-to-USER copy and remaps P1:A/T1 to the newly saved slot; complete JSON is checked");
  // Entirely offline: compare two valid diagnostic fingerprint files. Even
  // identical checksums must never grant permission to program real NOR.
  const fingerprints={format:"mashina-flash-sector-fingerprints",version:1,
    mode:"READ_ONLY_DIAGNOSTIC",sectorBytes:4096,complete:true,
    sectors:Array.from({length:40},(_,i)=>({
      index:i,address:i<32?0xa0000+i*4096:0xc0000+(i-32)*4096,
      readable:true,crc32:(i+1000).toString(16).toUpperCase().padStart(8,"0")
    }))};
  const beforeReport={format:"mashina-flash-diagnostic",version:2,
    audit:{decision:"PHYSICAL_WRITES_BLOCKED"},fingerprints};
  const afterReport=structuredClone(beforeReport);
  await page.locator("#flash-compare-panel summary").click();
  await page.locator("#flash-compare-before").setInputFiles({
    name:"before.json",mimeType:"application/json",
    buffer:Buffer.from(JSON.stringify(beforeReport))});
  await page.locator("#flash-compare-after").setInputFiles({
    name:"after.json",mimeType:"application/json",
    buffer:Buffer.from(JSON.stringify(afterReport))});
  const [comparison]=await Promise.all([
    page.waitForEvent("download"),page.locator("#flash-compare-run").click()
  ]);
  const compared=JSON.parse(fs.readFileSync(await comparison.path(),"utf8"));
  assert.equal(compared.same,true);
  assert.equal(compared.physicalWritesAllowed,false);
  assert.match(await page.locator("#flash-compare-status").textContent(),/remain BLOCKED/);
  assert.deepEqual(errors,[],"Browser errors after CRC comparison");
  console.log("PASS: Chrome compares two full read-only flash reports and never enables physical writes");


}finally{
  if(browser)await browser.close();
  server.kill("SIGTERM");
}
