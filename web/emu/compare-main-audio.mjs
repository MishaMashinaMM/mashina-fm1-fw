// Compare identical seeded WebAssembly audio streams from main and panic-mode.
// Run with: node web/emu/compare-main-audio.mjs MAIN.wasm PANIC.wasm
import fs from "node:fs";
import crypto from "node:crypto";
const paths = process.argv.slice(2);
if (paths.length !== 2) throw Error("Expected main and panic-mode WASM paths");
async function boot(path) {
  const {instance} = await WebAssembly.instantiate(fs.readFileSync(path), {});
  const e = instance.exports;
  e._initialize();
  e.web_nor_erase();
  e.web_seed(123456789);
  e.web_boot();
  return e;
}
function render(e, ms) {
  const hash = crypto.createHash("sha256");
  let peak = 0, sum = 0, count = 0;
  const n = Math.round(ms * 44.1);
  for (let off = 0; off < n; off += 128) {
    const k = Math.min(128, n - off);
    e.web_render(k);
    for (const ptr of [e.web_out_l(), e.web_out_r()]) {
      const samples = new Float32Array(e.memory.buffer, ptr, k);
      hash.update(Buffer.from(samples.buffer, samples.byteOffset, samples.byteLength));
      for (const v of samples) { peak = Math.max(peak, Math.abs(v)); sum += v*v; count++; }
    }
  }
  return {hash: hash.digest("hex"), peak, rms: Math.sqrt(sum/count)};
}
const e = await Promise.all(paths.map(boot));
const cases = [
  ["boot", () => {}],
  ["default note 60 on", x => x.web_midi(0x90,60,110)],
  ["note 60 off", x => x.web_midi(0x80,60,0)],
  ["long release", () => {}],
  ["heavy preset", x => x.web_test_heavy()],
  ["heavy note 60 on", x => x.web_midi(0x90,60,110)],
  ["heavy note 60 off", x => x.web_midi(0x80,60,0)],
  ["heavy long release", () => {}]
];
const durations = [2300,400,400,2000,100,500,400,3000];
let mismatch = false;
for (let i=0;i<cases.length;i++) {
  for (const x of e) cases[i][1](x);
  const [a,b] = e.map(x=>render(x,durations[i]));
  const same = a.hash === b.hash;
  console.log(JSON.stringify({case:cases[i][0],same,main:a,panic:b}));
  if (!same) mismatch = true;
}
if (mismatch) { console.error("Audio samples differ before Panic Mode was triggered"); process.exitCode=1; }
else console.log("PASS: identical seeded audio samples before Panic Mode");
