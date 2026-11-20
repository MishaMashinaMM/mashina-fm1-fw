// SPDX-License-Identifier: GPL-3.0-only
// MASHINA site parity and publication guardrails (no firmware flashing).
import fs from 'node:fs';
import assert from 'node:assert/strict';

const read = p => fs.readFileSync(p, 'utf8');
const home = read('web/drums-home.html');
const installer = read('web/index_pkg.html');
const editor = read('web/editor.html');
const emulator = read('web/emu/index.html');
const workflow = read('.github/workflows/emulator.yml');
const readme = read('README.md');

for (const [name, text] of [['home', home], ['installer', installer],
  ['editor', editor], ['emulator', emulator], ['README', readme]]) {
  assert.match(text, /MASHINA/i, `${name} must name the Drums fork`);
}
for (const href of ['emu/', 'webapp/installer/', 'webapp/editor/', 'manual/']) {
  assert.ok(home.includes(`href="${href}"`), `homepage must link to ${href}`);
}
assert.match(home, /MASHINA v0\.9/, 'homepage must show current v0.9 release');
assert.match(home, /successfully flashed and tested on a physical FM1/i, 'homepage must disclose successful hardware validation');
assert.doesNotMatch(home, /not yet been verified on physical FM1 hardware|has not yet been verified on physical FM1 hardware/i, 'homepage must not claim current firmware is untested');
assert.match(readme, /^# MASHINA v0\.9$/m, 'README must use the release-only title');
assert.doesNotMatch(readme, /this visual discrepancy is \*\*not yet fixed\*\*/i, 'README must not list the corrected LIBRARY display as unresolved');
assert.match(installer, /MASHINA v0\.9: DRUM-X and DRUM-LAB/, 'installer must identify current MASHINA v0.9');
assert.match(editor, /id="backup-export"/, 'editor must retain complete backup export');
assert.match(editor, /id="backup-import"/, 'editor must retain complete backup restore');
assert.doesNotMatch(editor, /<button role="tab" data-tab="sequencer"/, 'Drums editor must not offer disabled sequencer');
assert.doesNotMatch(editor, /const TABS = \[[^\n]*"sequencer"/, 'disabled sequencer must not be navigable');
assert.doesNotMatch(emulator, /id="sample-bank-controls"|id="sample-play"|id="sample-bank"/, 'Drums emulator must not offer sample kits');
assert.doesNotMatch(emulator, /withSampleBank|encodeSampleBank|sampleKit:|factory-voices\.json/, 'Drums emulator must not load sample kits');
assert.match(workflow, /python3 tools\/build\.py/, 'site must package Drums firmware');
assert.match(workflow, /(?:sh|bash) web\/emu\/build\.sh/, 'site must build the Drums emulator');
assert.doesNotMatch(workflow, /\bweb\/(?:STABLE|PREVIEW|CLASSIC)\b/, 'site must not publish pinned upstream firmware');
assert.match(readme, /physical FM1/, 'README must describe device validation');
console.log('PASS: MASHINA site pages, links, metadata and build source');
