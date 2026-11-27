import {readFileSync} from "node:fs";
import {spawnSync} from "node:child_process";
import assert from "node:assert/strict";
const html=readFileSync("web/presets-library-manager.html","utf8");
const script=html.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
const syntax=spawnSync(process.execPath,["--input-type=module","--check"],{input:script,encoding:"utf8"});
assert.equal(syntax.status,0,syntax.stderr);
assert.match(html,/<button data-tab="backup" aria-selected="true">/);
assert.doesNotMatch(html,/id="backup-verify"/);
const connect=script.slice(script.indexOf('$("connect").onclick='),script.indexOf('$("disconnect").onclick='));
assert.match(connect,/await readConnectedDevice\(candidate\)/);assert.doesNotMatch(connect,/download\(/);
const accept=script.slice(script.indexOf("function acceptSnapshot("),script.indexOf("function ready("));
assert.doesNotMatch(accept,/download\(/);assert.match(accept,/captureVerifiedFullDevice/);
const apply=script.slice(script.indexOf("function apply(fn"),script.indexOf('for(const bank of ["user","library"])',script.indexOf("function apply(fn")));
assert.doesNotMatch(apply,/download\(/);
for(const name of ["Write USER changes","Write LIBRARY changes","Write PROJECT changes","Write complete backup","Save complete backup"])assert(html.includes(name));
assert.match(script,/renameSoundRecord\(raw,name\)/);
assert.match(script,/renameProjectRecord\(projectAt\(i\),name\)/);
console.log("PASS: manager syntax, automatic capture, no automatic download, rename staging and write labels");

assert.match(connect,/\},"Connecting FM1"\);/);
assert.doesNotMatch(html,/id="rename-dialog"/);
assert.match(html,/\.inline-name input\[readonly\]\{pointer-events:none;cursor:grab/);
assert.match(script,/input\.readOnly=true/);
assert.match(script,/input\.readOnly=false/);
assert.match(script,/input\.onblur=\(\)=>finish\(true,true\)/);
assert.match(script,/if\(row\)row\.draggable=false/);
assert.match(script,/if\(row\)row\.draggable=wasDraggable/);
assert.match(script,/finish\(e\.key==="Enter"\)/);
console.log("PASS: connection progress dialog, inline name edits, normal drag cursor and keyboard commit/cancel");

// Every visible static label is present in each locale, apart from product/hardware names.
const {catalog,languages,translate}=await import("../web/web-library-locales.mjs");
const {extra}=await import("../web/web-library-messages.mjs");
const enKeys=Object.keys(catalog.es).sort(),messageKeys=Object.keys(extra.es).sort();
for(const lang of languages.slice(1)){
 assert.deepEqual(Object.keys(catalog[lang]).sort(),enKeys);
 assert.deepEqual(Object.keys(extra[lang]).sort(),messageKeys);
 for(const key of [...enKeys,...messageKeys]){
   const value=extra[lang][key]??catalog[lang][key];assert(value&&value.trim(),lang+": "+key);
   assert.deepEqual([...value.matchAll(/\\{\\d+\\}/g)].map(x=>x[0]).sort(),[...key.matchAll(/\\{\\d+\\}/g)].map(x=>x[0]).sort(),lang+": "+key);
 }
 assert.notEqual(translate("Connect FM1",lang),"Connect FM1");
 assert.notEqual(translate("Do not disconnect USB, operate FM1 controls or leave this page while the operation is running.",lang),"Do not disconnect USB, operate FM1 controls or leave this page while the operation is running.");
}
assert.match(script,/localizeStatic\\(document.body\\)/);
assert.match(script,/langSelect.onchange/);
assert.match(script,/location.assign\\(u.href\\)/);
assert.match(script,/guide.html\\?lang=/);
console.log("PASS: all five Web Library translation catalogs, dynamic placeholders and language links");
