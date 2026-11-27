import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const archive=JSON.parse(readFileSync("factory/final-performance.json"));
const {instance}=await WebAssembly.instantiate(readFileSync("build/emu/felucca.wasm"),{});
const e=instance.exports;
e.web_nor_erase();e.web_boot();
assert.equal(e.web_lib_slots(),96);
for(const object of archive.archive.objects.filter(o=>[0,1,10].includes(o.id))){
  const ptr=e.web_factory_object(object.id);assert.ok(ptr);
  assert.deepEqual(Buffer.from(new Uint8Array(e.memory.buffer,ptr,object.size)),Buffer.from(object.data,"base64"),"exact factory archive object "+object.id);
}
assert.ok(archive.userPresets.every(x=>x===null),"source USER bank must be empty");
for(let i=0;i<32;i++)assert.equal(e.web_test_user_preset(i),0,"fresh USER "+(i+1)+" must be empty");
for(let i=0;i<96;i++){
  const p=e.web_factory_record(i);assert.ok(p);
  assert.deepEqual(Array.from(new Uint8Array(e.memory.buffer,p,304)),archive.libraryPresets[i],"exact slot "+(i+33));
}
assert.deepEqual([0,1,2,3].map(i=>e.web_mpr_ref(i)),[95,117,39,119]);
assert.equal(e.web_mpr_test_used(),0);
for(let i=0;i<180;i++)e.web_render(1024);
for(let i=0;i<96;i++){assert.equal(e.web_mpr_assign(0,i+32),0,"playable slot "+(i+33));}
assert.equal(e.web_mpr_ref(0),128);
for(let i=0;i<32;i++)assert.equal(e.web_test_user_preset(i),0,"browsing must not create USER "+(i+1));
assert.match(readFileSync("build/emu/index.html","utf8"),process.env.EMU_PUBLIC_STORAGE==="1"?/const STORE = .*\+"";/:/const STORE = .*"-final-factory-v1";/);
console.log("PASS: 96 exact factory sounds, fresh startup references, all slots playable, all 32 USER slots empty, isolated browser storage");
