/* SPDX-License-Identifier: GPL-3.0-only
 * Evidence classification never authorizes physical FM1 flash writes.
 */
import assert from "node:assert/strict";
import {auditFlashProbe} from "../web/flash-readiness.mjs";
const blank={projects:Array(16).fill(0),library:Array(4).fill(0)};
const a=auditFlashProbe(blank);
assert.equal(a.decision,"PHYSICAL_WRITES_BLOCKED");
assert.equal(a.stockUpdaterProven,false);
assert.equal(a.sectorOwnershipProven,false);
assert.equal(a.completeBackupVerified,false);
assert.equal(a.physicalRestoreProven,false);
assert.equal(a.count.blank,20);
assert.equal(a.projectBanks[0].address,"0xA0000");
assert.equal(a.projectBanks[15].project,8);
assert.equal(a.projectBanks[15].sections,"C/D");
assert.equal(a.libraryBanks[0].address,"0xC0000");
assert.equal(a.libraryBanks[3].slotStart,57);
assert.equal(a.libraryBanks[3].slotEnd,64);
const retired=auditFlashProbe({...blank,projects:[2,...Array(15).fill(0)]});
assert.equal(retired.needsLegacyMigration,true);
assert.match(retired.messages.join(" "),/backup before any explicit migration/);
const occupied=auditFlashProbe({...blank,projects:[1,...Array(15).fill(0)],library:[1,0,0,0]});
assert.equal(occupied.count.committed,2);
assert(occupied.messages.some(x=>x.includes("Committed experimental data")));
const damaged=auditFlashProbe({...blank,projects:[3,...Array(15).fill(0)]});
assert.equal(damaged.unknownOrUnreadable,true);
const unreadable=auditFlashProbe({...blank,library:[0,4,0,0]});
assert.equal(unreadable.unknownOrUnreadable,true);
for(const audit of [a,retired,occupied,damaged,unreadable])
  assert.equal(audit.decision,"PHYSICAL_WRITES_BLOCKED");
assert.throws(()=>auditFlashProbe({projects:[0],library:blank.library}),/exactly/);
assert.throws(()=>auditFlashProbe({projects:blank.projects,library:[0,5,0,0]}),/status codes/);
assert.throws(()=>auditFlashProbe({projects:blank.projects,library:[0,0.5,0,0]}),/status codes/);
assert.deepEqual(blank.projects,Array(16).fill(0),"readiness audit mutated input");
console.log("PASS: all flash geometry/status combinations remain physical-writes-blocked, label legacy/unknown, never change source");
