/* SPDX-License-Identifier: GPL-3.0-only
 * Execute the actual MPR1 browser WASM against a blank emulated 1 MiB NOR.
 * The first four boot sounds must be genuine unchanged LIBRARY references.
 * A project Section A SAVE must succeed without manual preset selection.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const {instance}=await WebAssembly.instantiate(readFileSync("build/emu/felucca.wasm"),{});
const ex=instance.exports;
for(const name of ["_initialize","web_nor_erase","web_boot","web_render",
  "web_mpr_ref","web_mpr_dirty","web_mpr_test_save","web_mpr_test_used","web_mpr_test_edit"]){
  assert.equal(typeof ex[name],"function","missing WASM export "+name);
}
ex._initialize();
ex.web_nor_erase();
ex.web_boot();
// Advance beyond the splash and exercise the normal 15-ms UI/main-loop
// processing that a static project-host test cannot reproduce.
for(let k=0;k<220;k++)ex.web_render(1024);
for(let k=0;k<4;k++){
  assert.equal(ex.web_mpr_ref(k),33+k,"T"+(k+1)+" is not LIBRARY 0"+(33+k));
  assert.equal(ex.web_mpr_dirty(k),0,"T"+(k+1)+" appears edited after idle boot");
}
assert.equal(ex.web_mpr_test_save(),0,"untouched boot project SAVE refused");
assert.equal(ex.web_mpr_test_used(),1,"saved Section A still reports EMPTY");
// A genuine edit must still be rejected: do not disable the sound integrity
// guard merely to get the default four-track SAVE working.
ex.web_mpr_test_edit(1);
assert.equal(ex.web_mpr_dirty(1),1,"T2 sound edit was not detected");
assert.notEqual(ex.web_mpr_test_save(),0,"unsaved sound edit incorrectly saved");
assert.equal(ex.web_mpr_test_used(),1,"failed overwrite deleted saved Section A");
console.log("PASS: boot four factory refs, immediate Section A SAVE, and genuine edit rejection");
