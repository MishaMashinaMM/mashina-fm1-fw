/* SPDX-License-Identifier: GPL-3.0-only
 * Prevent regression to first raw GLO table entry (currently page 6/7).
 * Startup must resolve the same MIXER navigation entry used by GLO button.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
const main=readFileSync("firmware/src/main.c","utf8");
const nav=readFileSync("firmware/src/ui_lens.c","utf8");
const boot=main.match(/\/\* MASHINA boots on GLO[\s\S]*?page_entered\(\);/);
assert.ok(boot,"missing explicit MASHINA GLO boot path");
assert.match(boot[0],/ui\.page\s*=\s*\(uint8_t\)nv_page\(SEC_GLO,\s*0u\)/);
assert.match(boot[0],/ui\.home\s*=\s*0/);
assert.doesNotMatch(boot[0],/page_first\(FAM_GLO\)/);
assert.match(nav,/static const nv_t CE_GLO\[\]\s*=\s*\{NV\("MIXER",\s*0,\s*0,\s*0\)/);
console.log("PASS: boot resolves GLO navigation entry MIXER 1/7, not first raw GLO page");

assert.match(nav,/NAV_DEF\[NAV_KEYS\]\s*=\s*\{0, 0, 5, 0, 0, 2, 0,/);
assert.match(nav,/nav_go\(sec, nav_mem\(nav_key\(sec\)\), 1\);/);
assert.doesNotMatch(nav,/sec == SEC_GLO \? 0u : nav_mem/);
console.log("PASS: session navigation memory for GLO and LFO default page 1");
