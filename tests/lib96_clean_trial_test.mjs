import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {runInstall} from "../web/fm1diag.js";
let installed=0;
const ui={say(){},step(){},openStock(){throw Error("unexpected stock detour")}};
for(const text of ["FM-1_015","FM-1_913","FM-1_914","FM-1_701","FM-1_010"]){
  const up={t:{findTries:1},find:async predicate=>predicate({model:"FM-1",text})?{id:{model:"FM-1",text},name:"FM1",link:{close(){}}}:null,install:async()=>{installed++}};
  await runInstall({up,ui,state:{},cleanLib96Trial:true,confirmTrialBackup:()=>true});
}
assert.equal(installed,5);
await assert.rejects(runInstall({up:{},ui,state:{},cleanLib96Trial:true,confirmTrialBackup:()=>false}),/cancelled/);
const site=readFileSync(new URL("../web/make_site.py",import.meta.url),"utf8");assert(site.includes('(library / "initialize.html").unlink()'));
console.log("PASS: clean installer accepts compatible FM1 regardless of previous firmware or storage; explicit confirmation required");
