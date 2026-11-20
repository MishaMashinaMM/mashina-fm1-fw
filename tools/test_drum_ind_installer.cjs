/* Smoke-check the exact standalone DRUM-IND flasher ZIP as a real browser UI.
 * The firmware is never flashed. A blank localization panel fails the build.
 * Run after packaging: node tools/test_drum_ind_installer.cjs <artifact-root>
 */
const assert = require("node:assert/strict");
const fs = require("node:fs");
const http = require("node:http");
const path = require("node:path");
const {pathToFileURL} = require("node:url");
const {chromium} = require("playwright");

(async () => {
  const root = path.resolve(process.argv[2] || "build/drum-ind-web-installer");
  const files = [
    "webapp/installer/index.html",
    "webapp/installer-orange-v09/index.html",
    "mashina-nav.js", "mashina-nav.css",
    "firmware/versions.json", "firmware/RELEASE_SHA256"
  ];
  for (const f of files) assert(fs.statSync(path.join(root, f)).size > 0, "Missing " + f);
  const versions = JSON.parse(fs.readFileSync(path.join(root,"firmware/versions.json")));
  assert.equal(versions.length,1,"Experimental installer must offer one version");
  assert(versions[0].version.includes("drum-ind"),"Wrong firmware version");
  assert(fs.statSync(path.join(root,"firmware",versions[0].file)).size > 50000);

  const server = http.createServer((req, res) => {
    const pathname = decodeURIComponent(new URL(req.url,"http://localhost").pathname);
    let f = path.resolve(root, "." + pathname);
    if (!f.startsWith(root + path.sep) && f !== root) {
      res.writeHead(403);return res.end();
    }
    try {if(fs.statSync(f).isDirectory())f=path.join(f,"index.html");} catch {}
    const ext = path.extname(f);
    res.setHeader("Content-Type", {
      ".html":"text/html; charset=utf-8",".js":"text/javascript; charset=utf-8",
      ".css":"text/css; charset=utf-8",".json":"application/json",
      ".fwsc":"application/octet-stream"
    }[ext] || "application/octet-stream");
    fs.createReadStream(f).on("error",()=>{if(!res.headersSent)res.writeHead(404);res.end();}).pipe(res);
  });
  await new Promise(resolve => server.listen(0,"127.0.0.1",resolve));
  const port = server.address().port;
  const browser = await chromium.launch({headless:true,args:["--no-sandbox"]});
  try {
    const page = await browser.newPage();
    /* Headless Chrome does not offer Web MIDI on GitHub runners. Advertise
     * API presence only, so this test can check that the real local firmware
     * package loads and the button becomes labeled/enabled. Never press it. */
    await page.addInitScript(() => {
      Object.defineProperty(navigator,"requestMIDIAccess",{
        configurable:true,value:async()=>({inputs:new Map(),outputs:new Map()})
      });
    });
    const errors=[], missing=[];
    page.on("pageerror",e=>errors.push(e.message));
    page.on("response",r=>{if(r.status()>=400)missing.push(r.url()+" -> "+r.status());});
    await page.goto("http://127.0.0.1:"+port+"/webapp/installer/",{waitUntil:"load"});
    try {
      await page.waitForFunction(()=>document.getElementById("installer-startup-warning")===null,
        null,{timeout:8000});
    } catch (err) {
      console.error("INSTALLER INIT FAILED",{
        errors, missing,
        banner:await page.locator("#installer-startup-warning").innerText().catch(()=>"(absent)"),
        intro:await page.locator('[data-t="intro"]').innerText().catch(()=>"(absent)"),
        status:await page.locator("#status").innerText().catch(()=>"(absent)"),
        console:await page.evaluate(()=>({origin:location.origin,
          ready:document.readyState,
          textCount:[...document.querySelectorAll("[data-t]")].filter(x=>x.textContent.trim()).length}))
      });
      throw err;
    }
    for (const selector of [
      '[data-t="intro"]','[data-t="warn"]','[data-t="firstTitle"]',
      '[data-t="firstText"]','#first-stock','[data-t="verLabel"]',
      '#go','[data-t="stockTitle"]','[data-t="reportTitle"]'
    ]){
      assert((await page.locator(selector).innerText()).trim().length>4,
        "Empty or missing translated UI: "+selector);
    }
    await page.waitForFunction(()=>!document.querySelector("#go").disabled,
      {timeout:15000});
    const status=await page.locator("#status").innerText();
    assert(status.includes("MASHINA"),"Firmware not loaded: "+status);
    assert((await page.locator("#ver option").allTextContents()).join(" ").includes("drum-ind"));
    for(const lang of ["es","sr","ja","ru","hi","en"]){
      await page.locator('#lang button[data-lang="'+lang+'"]').click();
      assert((await page.locator('[data-t="intro"]').innerText()).length>10,
        "Language failed: "+lang);
      assert((await page.locator("#go").innerText()).length>2,
        "Install label absent: "+lang);
    }
    assert.deepEqual(errors,[],"JS errors in standalone browser");
    assert.deepEqual(missing,[],"Missing packaged asset");
    console.log("PASS: complete installer labels, 6 languages, firmware download, browser JS and bundled assets");
    await page.close();

    const local = await browser.newPage();
    await local.goto(pathToFileURL(path.join(root,"webapp/installer/index.html")).href);
    assert(await local.locator("#installer-startup-warning").isVisible(),
      "file:// must visibly refuse initialization");
    assert(await local.locator("#go").isDisabled(),
      "file:// must never enable flashing");
    console.log("PASS: file:// fallback visibly refuses unsafe flashing");
    await local.close();
  } finally {
    await browser.close();
    await new Promise(resolve=>server.close(resolve));
  }
})().catch(e=>{console.error(e);process.exitCode=1;});
