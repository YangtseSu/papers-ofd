# Progress Log

## Session: 2026-10-14

### Phase 1: Restore state and consolidate research

- **Status:** complete
- Actions taken:
  - `resolve-plan-dir.sh` → no plan selected (no root `task_plan.md`); initialized `.planning/2026-10-14-papers-ofd-backend-plan-and-readme/` via `init-session.sh`.
  - `git status --short` / `git diff --stat` → previous draft modified `README.md` (+124) and `docs/plan.md` (+836/-197), uncommitted on top of `b84bced`.
  - Consolidated all Papers/rofd evidence into `findings.md` (30 Papers findings, 14 rofd findings, decisions, issues).
- Files created/modified:
  - `.planning/2026-10-14-papers-ofd-backend-plan-and-readme/{task_plan.md,findings.md,progress.md}`

### Phase 2: Redo docs/plan.md

- **Status:** complete
- Actions taken:
  - Rewrote `docs/plan.md` (668 → 447 lines) with: definition of done (§1), architecture + 9 constraining decisions (§2), full interface contract incl. unguarded-vfunc inventory (§3), units/coordinates/render math (§4), phases 0–4 with steps, acceptance and required evidence (§5), verification playbook + fixture matrix (§6), packaging/desktop integration (§7), risks (§8), upstream requests (§9), evidence index A1–A17 and corrections B1–B15.
- Files created/modified:
  - `docs/plan.md`

### Phase 3: Rewrite README.md

- **Status:** complete
- Actions taken:
  - Rewrote `README.md`: status + plan link, capability/limitation table, discovery description, requirements incl. the SONAME symlink trap, build/try commands, installed-file layout, limitations, roadmap table, upstreaming and license. README anchors link into `docs/plan.md` §5/§6/§7/§9.
- Files created/modified:
  - `README.md`

### Phase 4: Verification

- **Status:** complete
- Actions taken:
  - Structure: 14 fences in the plan, 6 in the README, all balanced; no malformed table rows.
  - Anchors: README → plan anchors resolve (4/4).
  - Paths: 13 relative paths referenced by both documents exist; `pkg-config --variable=backenddir papers-document-4.0` → `/usr/lib/papers/6/backends`.
  - Citations: 29 `file:line` claims probed for the expected token; 26 passed, 3 corrected (`pps-page.h:27-34` for the page struct fields, `pps-document.c:461-468` for the unguarded `save` dispatch, `pps-document-find.c:27-34` for the unguarded `find_text`).
- Files created/modified:
  - `docs/plan.md`, `README.md`, `findings.md` (citation fixes)

### Phase 5: Delivery

- **Status:** complete
- Actions taken:
  - Delivered the Chinese summary of the rework and the unresolved choices.

### Phase 6: Follow-up decisions

- **Status:** complete
- Actions taken:
  - User answers applied: (1) association route locked to the separate `NoDisplay` desktop file (`Icon=org.gnome.Papers` verified installed, app id `org.gnome.Papers` from `meson.build:18,76`), patch-the-system-desktop route recorded as rejected with its cost; (2) `rofd_document_get_page_size_mm` scheduled *now* on a fork branch + upstream PR, carried as `patches/rofd-ffi-page-size.patch` with `cc.has_function` detection and a working fallback (decision D10, evidence A18 = the pinned 77-symbol list and rofd's additive-extension rule); (3) `.planning/` explained and added to `.gitignore`.
  - Fixture reality check: every available fixture is 1–5 pages, so the open-latency number requires the generated 300-page fixture already scheduled in phase 1.
- Files created/modified:
  - `docs/plan.md`, `README.md`, `.gitignore`, `task_plan.md`

### Phase 7: rofd `rofd_document_get_page_size_mm` + hosting

- **Status:** in_progress
- Actions taken:
  - Added the metadata-only page-size query to the rofd C ABI: declaration + contract comment in `crates/rofd-ffi/include/rofd.h`, implementation in `crates/rofd-ffi/src/document.rs` (wraps `rofd_core::Document::page_size` through `boundary`/`ScalarOutput`), re-export in `crates/rofd-ffi/src/lib.rs`, `rofd_document_get_page_size_mm` appended to `tests/c/expected-symbols.txt`, a paragraph in `crates/rofd-ffi/README.md`, and the signature in the v1 design doc.
  - Added three tests in `crates/rofd-ffi/tests/document.rs`: value parity with `rofd_page_get_size_mm` (fixture `learning/test.ofd`), metadata-only behaviour on a generated two-page document whose second page references a missing font resource (size query succeeds, `rofd_document_get_page` fails with `INVALID_DOCUMENT`), and argument/index/aliasing rejections (NULL handle, NULL output, out-of-range index, scalar output aliasing the error slot).
  - User decisions recorded: `.planning/` stays in the repo for two-machine work (`.gitignore` entry reverted); rofd changes go to a fork branch + upstream PR; papers-ofd gets a private GitHub repo.
- Files modified (in `../rofd`):
  - `crates/rofd-ffi/include/rofd.h`, `crates/rofd-ffi/src/document.rs`, `crates/rofd-ffi/src/lib.rs`, `crates/rofd-ffi/tests/document.rs`, `crates/rofd-ffi/tests/c/expected-symbols.txt`, `crates/rofd-ffi/README.md`, `docs/superpowers/specs/2026-09-06-rofd-c-abi-v1-design.md`
- Files modified (here):
  - `.gitignore`

### Phase 8: REUSE, AGENTS.md, PR triage, patched AUR package

- **Status:** complete
- Actions taken:
  - Added `LICENSES/{GPL-2.0-or-later,LGPL-2.1-or-later}.txt` and `REUSE.toml`; repository default `GPL-2.0-or-later`, the carried rofd patch overridden to `LGPL-2.1-or-later` with both upstream and local copyright lines. `reuse lint` → compliant (10/10 files).
  - Wrote `AGENTS.md` (layout, hard rules, build/verify commands, conventions).
  - Triaged PR #11: `clacheck / CLAssistant` failed with an action-side error ("Could not update the JSON file: … Resource not accessible by integration"), so the bot never posted its instructions; its configuration requires the comment `I have read the CLA Document and I hereby sign the CLA.` (`https://github.com/linuxdeepin/cla/blob/master/cla.md`). `deepin-ci-robot` additionally needs a linuxdeepin member to comment `/ok-to-test`, and APPROVALNOTIFIER needs an approver from `debian/deepin/OWNERS` to `/approve`. Everything code-related is green: commitlint, license-check, cppcheck, Sourcery; build jobs skip until ok-to-test.
  - Found an unrelated upstream defect: `../rofd/REUSE.toml` line 24 has a stray `SPDX-License-Identifier = "MIT"` outside any `[[annotations]]` block, so `reuse lint` fails on their `main` ("Key already exists"). Our PR does not touch it.
  - AUR: `~/projects/librofd-ffi/PKGBUILD` now `pkgrel=2` with `rofd-ffi-page-size.patch` applied in `prepare()`; `.SRCINFO` regenerated; `makepkg -f` succeeded → `librofd-ffi-0.4.0-2-x86_64.pkg.tar.zst`. Packaged `librofd_ffi.so.0` exports 78 symbols including `rofd_document_get_page_size_mm`; packaged `rofd.h` carries the declaration. Installation left to the user (manual `pacman -U`).
- Files created/modified:
  - `LICENSES/`, `REUSE.toml`, `AGENTS.md` (committed as `32b15e8`); `~/projects/librofd-ffi/{PKGBUILD,.SRCINFO,rofd-ffi-page-size.patch}` (local, uncommitted)

## Test Results

| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| Markdown structure check (fences, table rows) | `docs/plan.md`, `README.md` | balanced fences, well-formed tables | 14 + 6 fences balanced, 0 malformed rows | pass |
| Cross-reference consistency | `§N` refs + README anchors | every reference resolves to an existing section | 4/4 anchors found; no missing | pass |
| Path existence spot-check | 13 `../...` paths in both docs | all exist on this host | none missing; backenddir = `/usr/lib/papers/6/backends` | pass |
| Citation spot-check | 29 `file:line` claims | each cited range contains the claim | 26 OK, 3 corrected, 3/3 re-verified | pass |
| Markdown in docs (`read` round-trip) | both files | render as intended | headings, tables, code fences intact | pass |
| Re-verification after phase-6 edits | both docs | structure/anchors/paths/citations still consistent | fences 14/6 balanced, anchors 4/4, paths 0 missing, citations re-probed | pass |
| rofd `cargo fmt --all -- --check` | `../rofd` | no diff | clean (first run also reformatted the new test file) | pass |
| rofd `cargo clippy -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings` | `../rofd` | no warnings | exit 0 after removing one `unused_mut` in the new test | pass |
| rofd `cargo test -p rofd-ffi` | `../rofd` | all pass, incl. 3 new tests | `tests/document.rs` 12 passed; document_page_size_* all ok | pass |
| rofd `crates/rofd-ffi/tests/run_c_tests.sh` | `../rofd` | symbol list diff clean | exit 0 (export list, C/C++ header, smoke) | pass |
| `patches/rofd-ffi-page-size.patch` applies to clean `rofd` `main` | detached worktree | applies cleanly | `git apply --check` + `git apply` OK (7 files, +326/-6) | pass |
| papers-ofd private repo push | `YangtseSu/papers-ofd` | main pushed, visibility PRIVATE | pushed, default branch `main`, topics ofd/papers/gnome/rofd | pass |
| `reuse lint` (papers-ofd) | tracked files | compliant | 10/10 files, licenses GPL-2.0-or-later + LGPL-2.1-or-later | pass |
| `reuse lint` (rofd main) | upstream checkout | compliant | fails to parse `REUSE.toml` (duplicate key, pre-existing) | upstream defect |
| `makepkg -f` (librofd-ffi 0.4.0-2 with PR patch) | PKGBUILD + cached v0.4.0 tarball | package builds, carries the symbol | built; exports 78 symbols incl. `rofd_document_get_page_size_mm` | pass |
| rofd PR CI (`gh pr checks 11`) | `linuxdeepin/rofd` | green | commitlint, license-check, cppcheck, Sourcery pass; `clacheck / CLAssistant` fails (Deepin CLA, contributor action); build jobs skipped until approval; APPROVALNOTIFIER wants an approver from `debian/deepin/OWNERS` | needs human action |

## Error Log

| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-10-14 | `read` elided C function bodies (declarations-only view) | 1 | Re-read the exact byte ranges with `sed -n` |
| 2026-10-14 | Python in-place edit assertion failed during plan section renumbering | 1 | Re-grepped the exact strings, re-ran the script; it only writes after all assertions pass |

## 5-Question Reboot Check

| Question | Answer |
|----------|--------|
| Where am I? | Phase 2 (redoing `docs/plan.md`) |
| Where am I going? | Phases 3–5: README, verification, delivery |
| What's the goal? | Execution-ready `docs/plan.md` + matching `README.md` grounded in verified Papers 51.1 / rofd ABI v1 behaviour |
| What have I learned? | See `findings.md` (evidence index with `file:line`) |
| What have I done? | State restored, research consolidated, plan + README reworked and verified; delivery in progress |

---

*Update this file after completing a phase, running validation, or encountering an error.*
