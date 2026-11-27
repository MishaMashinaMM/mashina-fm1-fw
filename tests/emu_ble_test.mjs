import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const {instance}=await WebAssembly.instantiate(readFileSync("build/emu/felucca.wasm"),{});
const e=instance.exports;
e.web_nor_erase();e.web_boot();
for(let i=0;i<180;i++) e.web_render(1024);
const ch=e.web_test_rx_channel(0)-1;assert.ok(ch>=0&&ch<16);
const pc=192|ch;
const send=(bytes)=>{new Uint8Array(e.memory.buffer,e.web_ble_buf(),bytes.length).set(bytes);return e.web_ble_packet(bytes.length)};
assert.equal(e.web_ble_control(3),0);
assert.equal(send([128,128,pc,127]),0);
assert.equal(e.web_ble_control(1),1);
assert.equal(e.web_ble_control(2),2);
assert.equal(e.web_ble_control(3),3);
assert.equal(send([128,128,pc,127]),1);
for(let i=0;i<12;i++)e.web_render(1024);
assert.equal(e.web_mpr_ref(0),128);
assert.equal(e.web_ble_count(),1);
assert.equal(send([0,128,pc,32]),0);
assert.equal(e.web_ble_count(),1);
assert.equal(send([128,128,144,60,100,129,128,60,0]),1);
assert.equal(e.web_ble_count(),3);
// Use an actual active channel and an accepted note from the current preset.
let note=-1;
for(let n=36;n<96;n++){
    e.web_midi(144|ch,n,100);
    e.web_render(1024);
    if(e.web_ble_owner(1,ch,n)&63){note=n;break;}
    e.web_midi(128|ch,n,0);e.web_render(1024);
}
assert.ok(note>=0,"find a playable note");
assert.equal(send([128,128,144|ch,note,100]),1);
e.web_render(1024);
assert.ok(e.web_ble_owner(1,ch,note)&63,"wired ownership before disconnect");
assert.ok(e.web_ble_owner(3,ch,note)&63,"BLE ownership on the same pitch");
assert.equal(e.web_ble_second(144|ch,note,100),1);
e.web_render(1024);
assert.ok(e.web_ble_owner(4,ch,note)&63,"second BLE peer owns same pitch");
assert.equal(e.web_ble_control(4),1);
e.web_render(1024);
assert.ok(e.web_ble_owner(4,ch,note)&63,"peer 1 disconnect preserves peer 2");
assert.equal(e.web_ble_second(0,0,0),1);
e.web_render(1024);
assert.equal(e.web_ble_owner(4,ch,note),0,"peer 2 disconnect releases its own note");
assert.ok(e.web_ble_owner(1,ch,note)&63,"disconnect preserves wired note");
assert.equal(e.web_ble_owner(3,ch,note),0,"disconnect releases BLE note");
e.web_midi(128|ch,note,0);e.web_render(1024);
assert.equal(e.web_ble_owner(1,ch,note),0);
assert.equal(send([128,128,pc,32]),0);
e.web_render(1024);
e.web_midi(pc,32,0);
for(let i=0;i<12;i++)e.web_render(1024);
assert.equal(e.web_mpr_ref(0),33);
assert.equal(e.web_ble_control(0),0);
console.log("PASS: simulated one-link lifecycle, actual BLE decoder, routing, disconnect and independent wired MIDI");

// Navigate through actual GLO pages and exercise panel encoders, then send packets on both links.
e.web_buttons(1<<5);e.web_render(1024);e.web_buttons(0);e.web_render(1024);
for(let i=0;i<16&&!e.web_ble_page();i++){e.web_enc(0,1);for(let j=0;j<4;j++)e.web_render(1024);}
assert.equal(e.web_ble_page(),1,"GLO exposes Bluetooth page");
const knob=(k,n)=>{e.web_enc(k,n);for(let j=0;j<4;j++)e.web_render(1024);};
knob(3,1);assert.equal(e.web_ble_status()&1,1);
knob(4,1);assert.equal(e.web_ble_status()&2,2);
knob(6,1);assert.equal((e.web_ble_status()>>2)&3,1);
knob(4,1);knob(5,1);knob(6,1);assert.equal((e.web_ble_status()>>2)&3,3);
const packet=[128,128,144|ch,note,100];new Uint8Array(e.memory.buffer,e.web_ble_buf(),packet.length).set(packet);
assert.equal(e.web_ble_packet_peer(0,packet.length),1);assert.equal(e.web_ble_packet_peer(1,packet.length),1);e.web_render(1024);
assert.ok(e.web_ble_owner(3,ch,note)&63);assert.ok(e.web_ble_owner(4,ch,note)&63);
knob(6,-1);assert.equal((e.web_ble_status()>>2)&3,1);assert.equal(e.web_ble_owner(4,ch,note),0);assert.ok(e.web_ble_owner(3,ch,note)&63);
knob(3,-1);assert.equal(e.web_ble_status()&15,0);assert.equal(e.web_ble_owner(3,ch,note),0);
console.log("PASS: Bluetooth GLO navigation, knob controls and two independent simulated controllers");
