// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
// Felucca in an AudioWorklet: felucca.wasm is the whole FM-1 firmware against a simulated FM-1
// (web/emu/felucca_web.c). Each render quantum asks it for 128 frames, which runs the device's clock forward;
// between quanta the page's input goes in and, about 30 times a second (4 when the page is hidden), the screen,
// the lights and any changed flash go out. After X0X's web/emu/worklet.js (charlesvestal/fm1-x0x, GPL-3.0).

// The flash the page keeps: former FUN project and USER sectors (0x97000..
// 0xDFFFF), including reclaimed LIBRARY A/B at 0xC0000..0xC7FFF;
// autosave (0xE5000..0xE6FFF); global settings (0xFC000..0xFFFFF).
// The OLD experimental LIBRARY sectors 0xEA000..0xF1FFF are retained here
// solely so historical offline emulator JSON can still be imported. The
// relocated firmware NEVER treats those old USR addresses as LIBRARY storage.
// MPR1 project banks are inside the former FUN project interval.
// Only non-erased 4 KiB sectors are retained; OTA/bootloader sectors remain
// intentionally excluded. This does NOT enable experimental firmware flags.
export const STORE_AREAS = [[0x97000, 0xE0000], [0xE5000, 0xE7000],
                            [0xEA000, 0xF2000], [0xFC000, 0x100000]];
export function storeSectors(nor) {
  const out = {};
  for (const [a, b] of STORE_AREAS)
    for (let off = a; off < b; off += 4096) {
      const s = nor.subarray(off, off + 4096);
      if (s.some((v) => v !== 0xFF)) out[off] = s.slice();
    }
  return out;
}
export function restoreSectors(nor, sectors) {
  for (const [k, v] of Object.entries(sectors || {})) {
    const off = +k, b = v instanceof Uint8Array ? v : new Uint8Array(v);
    if (STORE_AREAS.some(([a, e]) => off >= a && off + 4096 <= e && off % 4096 === 0) && b.length === 4096)
      nor.set(b, off);
  }
}

export const HOLD_MS = 24;

if (typeof registerProcessor === "function") {
  const clock = globalThis.performance ? () => globalThis.performance.now() : () => Date.now();
  class Felucca extends AudioWorkletProcessor {
    constructor() {
      super();
      this.ex = null;
      this.sentDraws = -1;
      this.sentWrites = -1;
      this.sentLeds = "";
      this.sentDrumPads = "";
      this.lastFrame = 0;
      this.period = 1 / 30;
      this.busy = 0;
      this.frames = 0;
      this.queue = [];                                // button / key changes, each held >= HOLD_MS of device time
      this.nextOk = 0;
      this.port.onmessage = (e) => this.onMessage(e.data);
    }

    async onMessage(m) {
      if (m.type === "load") {
        try {
          const boot = async (sectors) => {
            const { instance } = await WebAssembly.instantiate(m.wasm, {});
            const ex = instance.exports;
            ex._initialize();
            ex.web_nor_erase();
            restoreSectors(new Uint8Array(ex.memory.buffer, ex.web_nor(), ex.web_nor_size()), sectors);
            ex.web_master(m.master ?? 700);
            if (ex.web_seed) ex.web_seed((Math.random() * 4294967296) >>> 0);
            ex.web_boot();
            return ex;
          };
          let ex = await boot(m.sectors);
          if (m.backupObjects) {
            const byId = new Map(m.backupObjects.map(o => [o.id, o.bytes]));
            const put = (id) => {
              const bytes = byId.get(id);
              if (!bytes || !bytes.length) return;
              if (bytes.length > ex.web_bk_max())
                throw new Error(`backup object ${id}: too large`);
              new Uint8Array(ex.memory.buffer, ex.web_bk_buf(), bytes.length).set(bytes);
              const rc = ex.web_bk_put(id, bytes.length);
              if (rc) throw new Error(`backup object ${id}: firmware refused it (code ${rc})`);
            };
            put(1);                             // PER4/PER5/PER6 settings, migrated in the firmware
            put(10);                            // song index
            for (let id = 40; id < 72; id++) put(id); // all saved song sections
            put(6); put(7); put(9);              // user presets and FM6 patches
            const imported = storeSectors(new Uint8Array(ex.memory.buffer, ex.web_nor(), ex.web_nor_size()));
            ex = await boot(imported);          // rebuild RAM caches from restored flash
            put(0);                            // unsaved live music belongs in RAM, after the reboot
            this.port.postMessage({type:"backup-ready", objects: m.backupObjects.length});
          }
          const samples = ex.web_sample_count ? ex.web_sample_count() : 0;
          const kitReady = !!(samples && m.sampleKit && ex.web_sample_kit && !ex.web_sample_kit());
          this.ex = ex;
          this.sentWrites = ex.web_flash_writes_count();
          this.port.postMessage({ type: "ready", rate: ex.web_sample_rate(), samples, kitReady });
        } catch (err) {
          this.port.postMessage({ type: "error", message: String(err && err.message || err) });
        }
        return;
      }
      const ex = this.ex;
      if (!ex) return;
      if (m.type === "flash-snapshot") {
        // Snapshot actual WASM NOR, not potentially stale browser storage.
        // Read-only: never changes an active sound, flash or project.
        const snapshot = storeSectors(new Uint8Array(ex.memory.buffer, ex.web_nor(), ex.web_nor_size()));
        this.port.postMessage({type:"flash-snapshot", sectors:snapshot});
      }
      else if (m.type === "sample-kit") {
        const samples = ex.web_sample_count ? ex.web_sample_count() : 0;
        const kitReady = !!(samples && ex.web_sample_kit && !ex.web_sample_kit());
        this.port.postMessage({type:"sample-ready", samples, kitReady});
      }
      else if (m.type === "buttons" || m.type === "keys") this.queue.push(m);
      else if (m.type === "enc") ex.web_enc(m.role, m.n | 0);
      else if (m.type === "master") ex.web_master(m.value | 0);
      else if (m.type === "ble-control" && ex.web_ble_control) {
        if(ex.web_ble_select && m.peer!==undefined)ex.web_ble_select(m.peer|0);
        if(m.cmd!==undefined)ex.web_ble_control(m.cmd|0);
        this.port.postMessage({type:"ble-state",state:ex.web_ble_status()});
      }
      else if (m.type === "ble-packet" && ex.web_ble_packet) {
        const data = m.data;
        if (data.length > 0 && data.length <= 20) {
          new Uint8Array(ex.memory.buffer,ex.web_ble_buf(),data.length).set(data);
          ex.web_ble_packet_peer(m.peer|0,data.length);
        }
        this.port.postMessage({type:"ble-count",count:ex.web_ble_count()});
      }
      else if (m.type === "midi") ex.web_midi(m.data[0] | 0, m.data[1] | 0, m.data[2] | 0);
      else if (m.type === "visible") this.period = m.on ? 1 / 30 : 1 / 4;
    }

    publish() {
      const ex = this.ex, mem = ex.memory.buffer;
      const msg = { type: "frame" };
      if(ex.web_ble_status){const state=ex.web_ble_status();if(state!==this.sentBle){this.sentBle=state;this.port.postMessage({type:"ble-state",state});}}
      if (ex.web_drum_pad_note) {
        const notes = Array.from({ length: 12 }, (_, i) => ex.web_drum_pad_note(i));
        const track = ex.web_drum_selected_track();
        const key = `${track}:${notes.join(",")}`;
        if (key !== this.sentDrumPads) {
          this.sentDrumPads = key;
          msg.drumPads = { track, notes };
        }
      }
      const leds = [ex.web_lit_buttons(), ex.web_lit_keys(), ex.web_dim_buttons(), ex.web_dim_keys(), ex.web_dim_level(),
                    ex.web_breath_buttons(), ex.web_breath_keys(), ex.web_mid_keys ? ex.web_mid_keys() : 0];
      const an = ex.web_anim_levels ? ex.web_anim_levels() : 0;   // the power-on sweep: each LED's level
      const anim = an ? Array.from(new Uint8Array(mem, an, 41)) : null;
      const key = leds.join(",") + (anim ? ";" + anim.join(",") : "");
      if (key !== this.sentLeds) {
        this.sentLeds = key;
        msg.leds = leds;
        msg.anim = anim;
      }
      if (this.frames >= 44100) {                     // the share of real time spent running the device
        msg.load = this.busy / (this.frames / 44.1);
        this.busy = this.frames = 0;
      }
      const transfer = [];
      const draws = ex.web_screen_draws();
      if (draws !== this.sentDraws) {
        this.sentDraws = draws;
        msg.fb = new Uint16Array(mem, ex.web_screen(), 240 * 240).slice();
        transfer.push(msg.fb.buffer);
      }
      const writes = ex.web_flash_writes_count();
      if (writes !== this.sentWrites) {               // a save is done within one main-loop pass: whole here
        this.sentWrites = writes;
        msg.sectors = storeSectors(new Uint8Array(mem, ex.web_nor(), ex.web_nor_size()));
      }
      if (msg.leds || msg.fb || msg.sectors || msg.drumPads || msg.load !== undefined) this.port.postMessage(msg, transfer);
    }

    // A click's down and up can arrive within one render quantum, but the FM-1's contacts never close for less than
    // its debounce, and the UI reads them once a frame: each change of the buttons or the keys stays HOLD_MS of the
    // device's time before the next one goes in (a tap: down for a frame and more, then up).
    input() {
      const ex = this.ex, now = ex.web_now_ms();
      while (this.queue.length && now - this.nextOk >= 0) {
        const m = this.queue.shift();
        if (m.type === "buttons") ex.web_buttons(m.mask >>> 0);
        else ex.web_keys(m.mask >>> 0);
        this.nextOk = now + HOLD_MS;
      }
    }

    process(inputs, outputs) {
      const ex = this.ex;
      if (!ex) return true;
      if (this.queue.length) this.input();
      const out = outputs[0], n = out[0].length;
      const t0 = clock();
      ex.web_render(n);
      this.busy += clock() - t0;
      this.frames += n;
      out[0].set(new Float32Array(ex.memory.buffer, ex.web_out_l(), n));
      if (out[1]) out[1].set(new Float32Array(ex.memory.buffer, ex.web_out_r(), n));
      if (currentTime - this.lastFrame >= this.period) {
        this.lastFrame = currentTime;
        this.publish();
      }
      return true;
    }
  }
  registerProcessor("felucca", Felucca);
}
