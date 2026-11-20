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
  assert.match(text, /MASHINA/i, `${name} must identify MASHINA`);
}
for (const href of ['emu/', 'webapp/installer/', 'webapp/editor/', 'manual/']) {
  assert.ok(home.includes(`href="${href}"`) || home.includes(`href="${href.replace("installer/", "installer-orange-v010/")}"`), `homepage must link to ${href}`);
}
assert.match(home, /<strong>MASHINA<\/strong><span>FM1 PERFORMANCE FW<\/span>/, 'homepage must have a two-line MASHINA wordmark');
assert.match(home, /data-i18n="safety"/, 'homepage must retain firmware backup warning');
assert.doesNotMatch(home, /not yet been verified on physical FM1 hardware|has not yet been verified on physical FM1 hardware/i, 'homepage must not claim current firmware is untested');
assert.match(readme, /^# MASHINA .* v0\.10$/m, 'README must use the release-only title');
assert.doesNotMatch(readme, /this visual discrepancy is \*\*not yet fixed\*\*/i, 'README must not list the corrected LIBRARY display as unresolved');
assert.match(installer, /MASHINA v0\.10: six-track (?:external )?MIDI/, 'installer must identify the current MASHINA external MIDI performance features');
assert.match(editor, /id="backup-export"/, 'editor must retain complete backup export');
assert.match(editor, /id="backup-import"/, 'editor must retain complete backup restore');
assert.doesNotMatch(editor, /<button role="tab" data-tab="sequencer"/, 'MASHINA editor must not offer disabled sequencer');
assert.doesNotMatch(editor, /const TABS = \[[^\n]*"sequencer"/, 'disabled sequencer must not be navigable');
assert.doesNotMatch(emulator, /id="sample-bank-controls"|id="sample-play"|id="sample-bank"/, 'MASHINA emulator must not offer sample kits');
assert.doesNotMatch(emulator, /withSampleBank|encodeSampleBank|sampleKit:|factory-voices\.json/, 'MASHINA emulator must not load sample kits');
assert.match(workflow, /python3 tools\/build\.py/, 'site must package MASHINA firmware');
assert.match(workflow, /(?:sh|bash) web\/emu\/build\.sh/, 'site must build the MASHINA emulator');
assert.doesNotMatch(workflow, /\bweb\/(?:STABLE|PREVIEW|CLASSIC)\b/, 'site must not publish pinned upstream firmware');
const translations = read('web/mashina-translations.js');
assert.match(home, /What MASHINA adds to JIANT/, 'homepage should introduce MASHINA additions without a changelog');
assert.match(home, /Four 12-sound drum engines/, 'homepage must describe all four drum engines');
assert.match(home, /data-i18n="projectTitle"/, 'homepage should cover live projects');
for (const name of ['DRUM-X', 'DRUM-LAB', 'DRUM-AC', 'DRUM-IND']) {
  assert.match(readme, new RegExp(name), 'README must name all drum engines');
}
for (const lang of ['en', 'es', 'ja', 'ru', 'sr', 'hi']) {
  assert.match(translations, new RegExp('"'+lang+'"\\s*:\\s*\\{'), 'homepage missing '+lang+' translation');
}
assert.equal((translations.match(/DRUM-IND/g)||[]).length, 6, 'homepage must describe DRUM-IND in all six languages');
assert.doesNotMatch(readme, /New in v0\.10|EDIT screen fix|flicker-free|regression tests/i, 'README must describe features, not release history');
console.log('PASS: MASHINA site pages, links, metadata and build source');
