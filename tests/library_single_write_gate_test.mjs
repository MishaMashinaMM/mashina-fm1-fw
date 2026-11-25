/* Physical trial is allowed to write exactly LIBRARY 064 via one PUT.
 * The general Web Library must retain its physical write lock.
 * Run in CI before publishing a downloadable trial page. */
import {readFileSync} from "node:fs";
import {strict as assert} from "node:assert";
import {spawnSync} from "node:child_process";
const html=readFileSync("web/library-064-single-write.html","utf8");
const general=readFileSync("web/presets-library-lab.html","utf8");
const workflow=readFileSync(".github/workflows/presets-library-check.yml","utf8");
assert.match(general,/const PHYSICAL_WRITES_APPROVED=false;/);
assert.match(html,/const TARGET=31;/);
assert.match(html,/const entry=\{slot:TARGET,recordBytes:item\.recordBytes\};/);
assert.match(html,/await uploadPrepared\(dev\.request,cap,\[entry\]\);/);
assert.match(html,/freshRefs\.usage\[63\]\.length/);
assert.match(html,/JSON\.stringify\(freshLib\)!==JSON\.stringify\(library\)/);
assert.match(html,/JSON\.stringify\(freshItem\.recordBytes\)!==JSON\.stringify\(item\.recordBytes\)/);
assert.match(html,/if\(JSON\.stringify\(parsed\)!==backupText\)/);
assert.match(html,/const after=await readLibrary\(dev\.request\)/);
assert.doesNotMatch(html,/\b(?:erase|restoreFull|restoreProjects|restoreBackup|LIB_CMD\.put)\s*\(/);
assert.doesNotMatch(html,/first-install\/index\.html/);
assert.match(workflow,/cp web\/library-064-single-write\.html build\/presets-library-preview\/library-064-single-write\.html/);
const scripts=[...html.matchAll(/<script type="module">([\s\S]*?)<\/script>/g)];
assert.equal(scripts.length,1);
const syntax=spawnSync(process.execPath,["--input-type=module","--check"],{input:scripts[0][1],encoding:"utf8"});
assert.equal(syntax.status,0,syntax.stderr);
console.log("Single-slot LIBRARY 064 gate: PASS (one destination, one record, backup, reference safety, read-back, JS syntax).");

assert.match(general,/applyVerifiedLibraryChanges\(device\.request,baseline,plan/);
assert.match(general,/projectCap\.canInitialize/);
assert.match(general,/wholeBackup=false;backedUp=false;pendingLibraryBackup=null;pendingFullBackup=null;verifiedFullFile=null;/);
const generalScript=general.match(/<script type="module">([\s\S]*?)<\/script>/)[1];
const generalSyntax=spawnSync(process.execPath,["--input-type=module","--check"],{input:generalScript,encoding:"utf8"});
assert.equal(generalSyntax.status,0,generalSyntax.stderr);
