/* SPDX-License-Identifier: GPL-3.0-only
 * Real Chrome (headless) end-to-end smoke of the *packaged* Web Library.
 * Verifies modular page loading, file chooser, preset queue and exported JSON.
 */
import assert from "node:assert/strict";
import fs from "node:fs";
import {spawn,spawnSync} from "node:child_process";
import {chromium} from "playwright-core";
import {encodeEmulatorFlash} from "../web/emu/emulator-flash-backup.mjs";
import {inspectEmulatorFlash} from "../web/emulator-library-inspector.mjs";
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
  for(let i=0;i<80;i++){
    try {const r=await fetch("http://127.0.0.1:8765/");if(r.ok){live=true;break;}}
    catch {}
    await new Promise(r=>setTimeout(r,100));
  }
  assert(live,"Web Library HTTP server did not start");
  browser=await chromium.launch({executablePath:exe,headless:true,
    args:["--no-sandbox","--disable-dev-shm-usage"]});
  const page=await browser.newPage({acceptDownloads:true});
  const errors=[];
  page.on("pageerror",e=>errors.push(e.message));
  await page.goto("http://127.0.0.1:8765/",{waitUntil:"networkidle"});
  await page.waitForFunction(()=>document.querySelectorAll("#emu-source-select option").length===107);
  assert.deepEqual(errors,[],"Web Library startup JavaScript errors");
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
}finally{
  if(browser)await browser.close();
  server.kill("SIGTERM");
}
