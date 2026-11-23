/* SPDX-License-Identifier: GPL-3.0-only
 * Boot genuine experimental WASM twice with the same simulated 1 MiB NOR.
 * Verify RESTORE LAST preserves actual USER/LIBRARY slot references.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const wasm=readFileSync("build/emu/felucca.wasm");
async function boot(nor){
  const {instance}=await WebAssembly.instantiate(wasm,{});
  const ex=instance.exports;
  ex._initialize();
  ex.web_nor_erase();
  if(nor)new Uint8Array(ex.memory.buffer,ex.web_nor(),ex.web_nor_size()).set(nor);
  ex.web_boot();
  return ex;
}
const first=await boot();
assert.equal(first.web_mpr_ref(0),33,"factory ref T1 initial");
first.web_mpr_restore_enable();
assert.equal(first.web_mpr_restore_flag(),1,"RESTORE LAST option enabled");
assert.equal(first.web_mpr_assign(0,37),0,"assign different LIBRARY preset");
assert.equal(first.web_mpr_ref(0),37+1,"T1 reference changed");
// Simulate >10 s of quiet operation, including splash, input and settings polling.
for(let i=0;i<780;i++)first.web_render(1024);
console.log("RESTORE LAST first boot:",JSON.stringify({
  enabled:first.web_mpr_restore_flag(),writes:first.web_mpr_autosave_writes(),
  record:first.web_mpr_autosave_record(),ref:first.web_mpr_ref(0),
  bootClean:first.web_mpr_boot_clean()
}));
assert.equal(first.web_mpr_autosave_record(),1,"no MPR1 autosave persisted after quiet time");
const snapshot=new Uint8Array(first.memory.buffer,first.web_nor(),first.web_nor_size()).slice();
const second=await boot(snapshot);
console.log("RESTORE LAST second boot:",JSON.stringify({
  enabled:second.web_mpr_restore_flag(),ref:second.web_mpr_ref(0),
  bootClean:second.web_mpr_boot_clean()
}));
assert.equal(second.web_mpr_restore_flag(),1,"RESTORE LAST setting failed to persist");
assert.equal(second.web_mpr_ref(0),37+1,"LAST project preset not restored on fresh boot");
console.log("PASS: MPR1 RESTORE LAST persists through full WASM shutdown/restart");
