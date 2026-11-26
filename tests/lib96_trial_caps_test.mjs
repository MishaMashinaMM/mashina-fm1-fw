/* SPDX-License-Identifier: GPL-3.0-only */
import assert from "node:assert/strict";
import {lib96TrialCaps} from "../web/fm1diag.js";
assert.equal(lib96TrialCaps([1,32,1],[1,32,1,0]),true);
for(const [a,b] of [[[1,96,1],[1,32,1,0]],[[1,32,0],[1,32,1,0]],[[1,32,1],[1,32,0,1]],[[1,32,1],[1,32,1,1]],[[1,32],[1,32,1,0]],[[1,32,1],[1,32,1]],[[0,32,1],[1,32,1,0]]])assert.equal(lib96TrialCaps(a,b),false);
console.log("PASS: trial installer requires initialized writable 32+32 capabilities");
