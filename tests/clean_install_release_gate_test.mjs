/* SPDX-License-Identifier: GPL-3.0-only
 * Release gate for the experimental first-install formatter.
 * This intentionally tests that a formatter C build is NOT a deployable
 * firmware package. Approval to prepare a trial does not approve enabling
 * physical writes or publishing destructive artifacts.
 */
import assert from "node:assert/strict";
import {readFileSync} from "node:fs";
import {spawnSync} from "node:child_process";

const read=p=>readFileSync(new URL("../"+p,import.meta.url),"utf8");
const build=read("tools/build.py");
const init=read("firmware/src/project_refs_init.c");
const api=read("firmware/src/editor_library.c");
const config=read("firmware/src/felucca.c");
const workflow=read(".github/workflows/presets-library-check.yml");
const flashGate=read("tools/presets_library_flash_map_check.py");
const trialPage=read("web/clean-install-trial.html");
const scriptStart='<script type="module">';
const scriptEnd='</script>';
const begin=trialPage.indexOf(scriptStart),finish=trialPage.indexOf(scriptEnd,begin+scriptStart.length);
assert(begin>=0&&finish>begin,"Trial page must have an ES module script");
const inline=trialPage.slice(begin+scriptStart.length,finish);
const syntax=spawnSync(process.execPath,["--input-type=module","--check"],
  {input:inline,encoding:"utf8"});
assert.equal(syntax.status,0,"Trial browser script syntax invalid: "+syntax.stderr);

for(const id of ["archive","connect","one-unit","rollback","phrase","preflight","commit"])
  assert(trialPage.includes('id="'+id+'"'),"Missing trial safety control "+id);
assert.match(trialPage,/readBackup\(JSON\.parse\(await file\.text\(\)\)\)/);
assert.match(trialPage,/backup\.objects\.length!==43/);
assert.match(trialPage,/authorizedFirmware=lib\.length===3/);
assert.match(trialPage,/project\.length===4/);
assert.match(trialPage,/\$\("phrase"\)\.value==="ERASE THIS FM1"/);
assert.match(trialPage,/await request\(101,\[\]\)/);
assert.match(trialPage,/confirm\("DESTRUCTIVE FIRST INSTALL/);
assert.match(trialPage,/await request\(102,\[\.\.\.phrase,ticket\]\)/);
assert.match(trialPage,/retries:0/);
assert.match(trialPage,/Do not resend COMMIT/);


for(const flag of ["FELUCCA_CLEAN_INSTALL_LAYOUT","FELUCCA_PROJECT_REFS_FORMAT"]) {
  assert.match(build,new RegExp('os\\.environ\\.get\\("'+flag+'"\\) == "1"'));
}
assert.match(build,/if non_distributable:\s*\n\s*print\("SAFE BUILD-ONLY:/);
assert(build.indexOf('if non_distributable:\n        print("SAFE BUILD-ONLY:') <
       build.indexOf("pkg = fm1pkg_make.ufw("),
       "destructive formatter must return before firmware packaging");
assert.match(build,/\(OUT \/ name\)\.unlink\(missing_ok=True\)/);
assert.match(build,/shutil\.rmtree\(OUT \/ f"release-\{a\.release\}"/);
assert.match(workflow,/test ! -e build\/felucca-0\.13\.fwsc/);
assert.match(workflow,/test ! -e build\/release-0\.13/);
assert.match(flashGate,/opt-in clean-layout formatter can compile but cannot produce a physical installer/);
assert.match(config,/#define FELUCCA_CLEAN_INSTALL_LAYOUT 0/);
assert.match(config,/#define FELUCCA_PROJECT_REFS_FORMAT 0/);
assert.match(api,/if \(cmd == 96u\)/);
assert.match(api,/#if FELUCCA_PROJECT_REFS && FELUCCA_PROJECT_REFS_FORMAT && !FELUCCA_CLEAN_INSTALL_LAYOUT/);
assert.match(api,/ed_b\(2u\); \/\* always reject destructive initialization \*\//);
/* The staged 101/102 two-phase protocol compiles ONLY into a build
 * that tools/build.py refuses to package. Ordinary and diagnostic images
 * respond 2 to both commands without calling the formatter. */
assert.match(api,/if \(cmd == 101u\)/);
assert.match(api,/if \(cmd == 102u\)/);
const commitBlock=api.slice(api.indexOf("if (cmd == 102u)"),api.indexOf("if (cmd == 96u)"));
assert.match(commitBlock,/#if FELUCCA_CLEAN_INSTALL_LAYOUT/);
assert.match(commitBlock,/clean_armed && n==10u/);
assert.match(commitBlock,/args\[9\]==clean_arm_nonce/);
assert.match(commitBlock,/\(uint32_t\)\(fm1_ms-clean_arm_ms\)<=15000u/);
assert.match(commitBlock,/clean_armed=0u;/);
assert.match(commitBlock,/clean_layout_initialize\(\)/);
assert.match(commitBlock,/#else\s*ed_b\(2u\);\s*#endif/);

assert.match(init,/static int clean_layout_preflight\(void\)/);
assert.match(init,/uint32_t status=clean_layout_preflight\(\);/);
assert.match(init,/if\(status\)return \(int\)status;/);
const eraseStart=init.indexOf("static int clean_layout_initialize(void)");
const eraseBody=init.slice(eraseStart);
assert(eraseStart>=0);
assert(eraseBody.indexOf("clean_layout_preflight()") < eraseBody.indexOf("mprs_init_clean_break()"));
assert(eraseBody.indexOf("mprs_init_clean_break()") < eraseBody.indexOf("lib_flash_erase("));
assert(eraseBody.indexOf("lib_flash_erase(") < eraseBody.indexOf("st_erase("));
assert(eraseBody.indexOf("st_erase(") < eraseBody.indexOf("mprs_save(0u,proj_wire_u.raw)"));
assert.match(init,/if\(mprs_save\(0u,proj_wire_u\.raw\)\)return 2;/);
console.log("PASS: first-install formatter stays compile-only, guarded, non-downloadable and non-callable over USB");
