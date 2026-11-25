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
const storeHw=read("firmware/src/storage_hw.c");
const projectHw=read("firmware/src/project_refs_hw.c");
const libraryHw=read("firmware/src/preset_library_runtime.c");
const projectBoot=read("firmware/src/project.c");

const scriptStart='<script type="module">';
const scriptEnd='</script>';
const begin=trialPage.indexOf(scriptStart),finish=trialPage.indexOf(scriptEnd,begin+scriptStart.length);
assert(begin>=0&&finish>begin,"Trial page must have an ES module script");
const inline=trialPage.slice(begin+scriptStart.length,finish);
const syntax=spawnSync(process.execPath,["--input-type=module","--check"],
  {input:inline,encoding:"utf8"});
assert.equal(syntax.status,0,"Trial browser script syntax invalid: "+syntax.stderr);

for(const id of ["archive","connect","one-unit","rollback","phrase","preflight","commit","fresh-boot","restore-phrase","restore-users"])
  assert(trialPage.includes('id="'+id+'"'),"Missing trial safety control "+id);
assert.match(trialPage,/const data=JSON\.parse\(await file\.text\(\)\)/);
assert.match(trialPage,/const backup=readBackup\(data\)/);
assert.match(trialPage,/backup\.objects\.length!==43/);
const canPrepare=trialPage.slice(trialPage.indexOf("function canPrepare()"),trialPage.indexOf("function refresh()"));
assert(!canPrepare.includes("validArchive"),
  "Clean installation must not require preserving old sounds or legacy JSON");
assert.match(canPrepare,/authorizedFirmware/);
assert.match(trialPage,/Skip this entirely if you want 32 empty USER slots/);
assert.match(trialPage,/OPTIONAL if you do not wish to retain any existing data/);

assert.match(trialPage,/restoreCapable=lib\.length===3/);
assert.match(trialPage,/authorizedFirmware=restoreCapable/);
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
assert(build.includes('if non_distributable and not trial_authorized:'));
assert(build.includes('and a.release == "0.13-cleantrial"'));
assert(build.includes('os.environ.get("GITHUB_EVENT_NAME") == "workflow_dispatch"'));
assert(build.includes('os.environ.get("GITHUB_EVENT_NAME") == "push"'));
assert(build.includes('os.environ.get("MASHINA_TRIAL_PUSH_AUTH") =='));
assert(build.includes('"AUTHORIZE ONE FM1 CLEAN TRIAL"'));
assert(workflow.includes("github.event.head_commit.message == 'AUTHORIZE ONE FM1 CLEAN TRIAL'"),
  "Destructive trial artifact must require one exact owner-authorized commit");
assert(workflow.includes("github.ref == 'refs/heads/presets-library'"));

assert(build.includes('os.environ.get("GITHUB_REF") == "refs/heads/presets-library"'));
assert(build.includes('os.environ.get("MASHINA_ONE_FM1_CLEAN_TRIAL") == "ERASE-ONE-FM1-32X32"'));
assert(build.includes('and all(os.environ.get(k) == "1"'));
assert(build.includes('os.environ.get("FELUCCA_FLASH_DIAG", "0") == "0"'));
assert(build.indexOf('if non_distributable and not trial_authorized:\n        print("SAFE BUILD-ONLY:') <
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
/* The FIRST FLASH may boot while original FUN/USER sectors are still present.
 * Legacy settings/autosave and experimental banks MUST remain write-locked
 * until a CRC-committed MPS1 marker is verified on a later cold boot. */
assert.match(storeHw,/static uint8_t clean_layout_normal_writes;/);
assert.equal((storeHw.match(/if \(!clean_layout_normal_writes\) return -10;/g)||[]).length,2,
             "both legacy persistent erase and program must refuse pre-format writes");
assert.equal((projectHw.match(/if\(!clean_layout_normal_writes\)return -10;/g)||[]).length,2,
             "MPR1 erases AND programs must be locked on first legacy boot");
assert.equal((libraryHw.match(/if \(!clean_layout_normal_writes\) return -10;/g)||[]).length,2,
             "LIBRARY erases AND programs must be locked on first legacy boot");
assert.match(projectBoot,/clean_layout_normal_writes=\(uint8_t\)\(mprs_valid\(0u,0u,&marker\)/);
assert.match(projectBoot,/mprs_valid\(0u,1u,&marker\)/);
const synchronousClean=api.slice(api.indexOf("if (cmd == 102u)"),api.indexOf("if (cmd == 96u)"));
assert.match(synchronousClean,/clean_layout_normal_writes=1u;\s*const uint32_t result=\(uint32_t\)clean_layout_initialize\(\);\s*clean_layout_normal_writes=0u;/,
             "only synchronous verified clean commit may temporarily unlock state writes");
console.log("PASS: first-install formatter stays compile-only, guarded, non-downloadable and non-callable over USB");

/* Update-only images keep the cold-boot lock, without destructive commands. */
assert.match(config,/#define FELUCCA_INITIALIZED_UPDATE 0/);
assert.match(config,/#if FELUCCA_INITIALIZED_UPDATE && .*FELUCCA_PROJECT_REFS_FORMAT.*FELUCCA_CLEAN_INSTALL_LAYOUT/);
for(const source of [storeHw,projectHw,libraryHw,projectBoot])
  assert(source.includes("#if FELUCCA_CLEAN_INSTALL_LAYOUT || FELUCCA_INITIALIZED_UPDATE"));
assert.match(api,/ed_b\(flash_ok && clean_layout_normal_writes \? 1u : 0u\)/);
assert.match(api,/response\[2\] = flash_ok && clean_layout_normal_writes \? 1u : 0u/);
assert(build.includes('a.release == "0.13-update"'));
const update=workflow.slice(workflow.indexOf("      # Explicitly authorized update"),workflow.indexOf("      # Only a human-initiated Run workflow"));
for(const flag of ["FELUCCA_PROJECT_REFS_FORMAT","FELUCCA_CLEAN_INSTALL_LAYOUT","FELUCCA_FLASH_DIAG"])
  assert(update.includes(flag+': "0"'));
for(const flag of ["FELUCCA_INITIALIZED_UPDATE","FELUCCA_LIB32","FELUCCA_LIB32_WRITE","FELUCCA_PROJECT_REFS","FELUCCA_PROJECT_REFS_WRITE"])
  assert(update.includes(flag+': "1"'));
assert(update.includes("PREPARE ONE FM1 INITIALIZED UPDATE"));
assert(update.includes("build/host/ldr_test"));
assert(update.includes('test ! -e "$OUT/first-install"'));
assert(!update.includes("clean-install-trial.html"));
console.log("PASS: initialized-only update retains boot marker write lock, no formatter, separate artifact");
