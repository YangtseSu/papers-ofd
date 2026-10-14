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

### Phase 8b: AUR publication (maintainer decision)

- **Status:** complete
- Actions taken:
  - The maintainer (repository owner) chose to publish the patch-carrying package; the concern that an unmerged upstream patch now ships through a community package is recorded here for transparency.
  - `makepkg -C -f` from a wiped `src/` and `pkg/` succeeded (rc=0) — the PKGBUILD reproduces from the cached v0.4.0 tarball plus the pinned patch, offline.
  - Committed `librofd-ffi 0.4.0-2` (`2391d47`, files `PKGBUILD`, `.SRCINFO`, `rofd-ffi-page-size.patch`) and pushed to the AUR; `git ls-remote origin master` returns the same commit.
  - The AUR RPC metadata still reported `0.4.0-1` a few seconds after the push (their cache lags); the git ref is authoritative.
- Files created/modified:
  - `~/projects/librofd-ffi/{PKGBUILD,.SRCINFO,rofd-ffi-page-size.patch}` (pushed to the AUR)

### Phase 9: Portable dependency paths (machine move)

- **Status:** complete
- Actions taken:
  - Problem: `docs/plan.md`, `README.md`, `AGENTS.md` and `patches/README.md` located both
    dependency checkouts as `../papers` and `../rofd`, and `docs/plan.md` opened with a
    structurally wrong link (`../../rofd/…` from `docs/`, resolving outside the projects
    directory). On this machine `../papers` does not exist and `../rofd` is
    `office-rs/rofd` — an unrelated project of the same name — so the documented build
    commands and every rofd citation silently pointed at the wrong tree.
  - Added `scripts/dev-env.sh`: resolves `PAPERS_SRC`/`ROFD_SRC` (environment → untracked
    `local.conf` → sibling defaults), validates each against a marker file
    (`libdocument/pps-document.h`, `crates/rofd-ffi/include/rofd.h`), reports a same-named
    directory that is not the expected repository, and exports the ABI pair
    `ROFD_INCLUDEDIR`/`ROFD_LIBDIR` from a built checkout or from the installed
    `librofd-ffi` package. Bare run prints the resolution table and exits non-zero when
    the ABI is unresolved; sourced, it only warns and never changes shell options.
  - Docs now use `$PAPERS_SRC/…` / `$ROFD_SRC/…`: plan header, §6 environment block,
    fixture matrix and cross-repo check, phase 4, §9, Appendix A (legend + rofd rows);
    README status/requirements/build/roadmap; AGENTS.md layout table, hard rules and
    build block; `patches/README.md` apply step. `/local.conf` is gitignored.
- Files created/modified:
  - `scripts/dev-env.sh` (new), `.gitignore`, `docs/plan.md`, `README.md`, `AGENTS.md`,
    `patches/README.md`

### Phase 10: Dependency requirements answered, Papers cited upstream

- **Status:** complete
- Actions taken:
  - Verified on this host what the implementation actually needs: `papers-document-4.0` 51.1 ships 43 `libdocument` + 20 `libview` headers, `libppsdocument-4.0.so.6` and pkg-config (incl. `backenddir`); `pps_backend_query_type` is declared in `pps-document.h:261` and exported by each backend module (verified on all four installed modules); `pps-backend-info.h` is not installed and is not needed to compile a backend. `librofd-ffi 0.4.0-2` provides `/usr/include/rofd.h` + `/usr/lib/librofd_ffi.so.0` with the page-size symbol. → neither checkout is needed to build phases 0–3.
  - Reclassified the checkouts in the docs: `$ROFD_SRC` is needed only to carry/re-verify `patches/` and to run rofd's own test suite; a Papers checkout is optional (grepping + phase 4).
  - Papers citations now name upstream tag `51.1` (`https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/<path>`), matching the user's instruction; tag `51.1` = commit `2c3976fb`, confirmed through the GitLab API. Plan header/§6/Appendix A legend, README (link, status, requirements, roadmap), AGENTS.md and the resolver's hint/header follow the convention.
- Files modified:
  - `docs/plan.md`, `README.md`, `AGENTS.md`, `scripts/dev-env.sh`, `findings.md`, `task_plan.md`

### Phase 11: Meson subproject/wrap clarified for the Papers move

- **Status:** complete
- Actions taken:
  - Answered with checked facts instead of assuming the mechanism works: rofd upstream has no `meson.build` and no `.pc` file, and `crates/rofd-ffi/build.rs` makes rofd-ffi a build-script crate; Meson 1.12.1's Cargo interpreter refuses any manifest with a `build.rs` ("no good way for us to convert them … an actual meson port will be required"), so a `subprojects/rofd-ffi.wrap` alone gives Meson nothing to consume.
  - Plan §5 phase 4 now specifies how Papers declares the ABI dependency: (1) `dependency('rofd-ffi', version: '>= 0.4')` once a `.pc` exists (target state), (2) otherwise a `[wrap-git]` file pinned to a revision plus a small `meson.build` that drives `cargo build --release --offline -p rofd-ffi` in a `custom_target` and exposes it via `declare_dependency()`. A wrap adds no sources to the repository (`subprojects/` stays untracked; a submodule would vendor them) and does not change the link step.
  - Added plan §9 item 5: request `rofd-ffi.pc` upstream (`Name`/`Version`/`Cflags`/`Libs`), which also retires the hand-rolled library lookup in `scripts/dev-env.sh`; README requirement row links to it.
- Files modified: `docs/plan.md`, `README.md`, `findings.md` (R15), `progress.md`

### Phase 12: Fork-first delivery — 0.4.3 package with pkg-config metadata

- **Status:** complete
- Actions taken:
  - User decisions applied: rofd dependency is fork-first (upstream PRs are opportunistic), the `.pc` and the AUR package are ours to write and maintain, and later upstream commits may be used when they help.
  - Evaluated upstream `v0.4.0...main` (10 commits): `0.4.3` exists only as commit `140b936` (no tag); the load-bearing one for us is `fix(ffi): map SingularTransform diagnostic to C ABI v1` (before it, an unmapped diagnostic failed the whole page with `ROFD_STATUS_INTERNAL`), plus crate vendoring (`net.offline = true`, offline builds) and licence/README chores. `ROFD_ABI_VERSION` is still `1u`, `expected-symbols.txt` still 77 names.
  - Rewrote the AUR package to `librofd-ffi 0.4.3-1`: source pinned to the commit archive (sha256 `0d6300fb…`), page-size patch on top, `rofd-ffi.pc` installed at `/usr/lib/pkgconfig/rofd-ffi.pc`, `prepare()`/`build()` now offline (vendor + `--frozen`). `makepkg -f --noconfirm` → package built in 35 s; verification below. Local commit made; not pushed to the AUR.
  - `scripts/dev-env.sh` gained a pkg-config tier (built checkout > `rofd-ffi.pc` > `/usr/include` + `/usr/lib`, per-variable env overrides); verified against a staged `.pc`.
  - Docs: plan D11 already covers the policy; §9 item 1/5 and phase-4 route 1 updated to "owned, not blocked"; risks gained the "rofd < 0.4.2 renders whole pages blank" row; appendix A19 added; README requirement row now `≥ 0.4.2` with the AUR pin; `patches/README.md` documents the fork, the untagged-release pinning rule and the sync routine. `$ROFD_SRC` (`~/projects/rofd`) is a complete clone of the fork at `140b936`, with `upstream` remote added; `git apply --check` of the page-size patch against it is clean (7 files).
- Files modified: `docs/plan.md`, `README.md`, `patches/README.md`, `scripts/dev-env.sh`, `findings.md`, `task_plan.md`; `~/projects/librofd-ffi/{PKGBUILD,.SRCINFO,rofd-ffi.pc.in}`

### Phase 0: loadable module skeleton (implementation)

- **Status:** complete
- Actions taken:
  - Meson project (`meson.build`, `meson_options.txt`): `papers-document-4.0 >= 51 < 52`, `rofd-ffi` by pkg-config with `-Drofd_libdir`/`-Drofd_includedir` as the fallback, `papers_backend_dir` (`backend_dir` is reserved in Meson), `tests`; module built with `gnu_symbol_visibility: 'hidden'`, `-Wl,-z,now`, `-Wl,--no-undefined`; `HAVE_ROFD_PAGE_SIZE` from `cc.has_function` (probe now passes cairo, see P38).
  - `src/ofd-backend.c` (`pps_backend_query_type` + ABI-mismatch warning), `src/ofd-document.c` (load/save stub/get_n_pages/get_page/get_page_size with the fast path and a per-page geometry cache/get_backend_info + the renderer), `src/ofd-page.c` (PpsPage<->rofd_page_t attach, D1), `src/ofd-units.c` (mm<->pt and the origin-normalizing rect conversions, D3), `src/ofd-render.c` (render options per §4, canvas size from rofd, render + diagnostic logging per D9).
  - `data/`: backend descriptor, `papers-ofd.xml`, `papers-ofd.thumbnailer`, `org.gnome.Papers.Ofd.desktop` + install rules.
  - `tests/`: `generate_fixture.py` (deterministic 3-page OFD: A4/A5/landscape boxes), `test-ofd-units.c` (4 unit tests incl. the non-zero-origin case), `test-ofd-document.c` (stages module + descriptor + a scratch MIME database, loads through `pps_document_factory_get_document` + `pps_document_load`, asserts 3 pages and the exact point sizes, page index, backend info, out-of-range pages, the `save` refusal) and its `--expect-discovery-failure` mode (broken `Module=` → no backend for `application/ofd`, the plan's deliberate-failure check).
  - Runtime acceptance observed on this host: `meson test -C _build` 3/3; `PPS_BACKENDS_DIR=_build/devbackends papers _build/tests/geometry-3pages.ofd` with the staged MIME tree ran until killed (`timeout` → 124) with **zero** critical/failed lines; the same setup opened a PDF cleanly (dev dir built by copying the whole system backend directory first).
- Files created: `meson.build`, `meson_options.txt`, `src/{meson.build,ofd-backend.c,ofd-document.[ch],ofd-page.[ch],ofd-render.[ch],ofd-units.[ch]}`, `data/{meson.build,*.papers-backend,papers-ofd.xml,papers-ofd.thumbnailer,org.gnome.Papers.Ofd.desktop}`, `tests/{meson.build,generate_fixture.py,test-ofd-document.c,test-ofd-units.c}`

### Phase 13: fork pushes, upstream PR #12, AUR publication

- **Status:** complete
- Actions taken:
  - AUR: committed `librofd-ffi 0.4.3-2` (adds `Requires: cairo` to the generated `.pc`) and pushed to `ssh://aur@aur.archlinux.org/librofd-ffi.git`; remote `master` = `846dc9a` and the AUR RPC reports `0.4.3-2`.
  - Fork: confirmed `YangtseSu/rofd` `main` is identical to `linuxdeepin/rofd` `main` (`0 0` at `140b936`) and that `feat/ffi-page-size-query` is present; fetched `upstream/main` to make the comparison real rather than assumed.
  - New rofd change: `feat/pkgconfig` — `debian/rofd-ffi.pc.in` plus the `librofd-ffi-dev` install rule in `debian/rules` (version from `dpkg-parsechangelog`, `DEB_HOST_MULTIARCH`, `Requires: cairo`). Pushed to the fork and proposed upstream as <https://github.com/linuxdeepin/rofd/pull/12>; PR #11 remains open.
  - Recorded the delivery policy in the plan (§9 item 5) and `patches/README.md` (upstream requests in flight, with the drop criterion for each).

### Phase 1: rendering, metadata, printing (implementation)

- **Status:** complete
- Actions taken:
  - `src/ofd-metadata.[ch]` (new): `get_info` mapping from the rofd metadata
    snapshot — title/author/subject/keywords (joined `", "`, empties skipped, since
    rofd distinguishes missing from explicitly empty), creator (`Creator` +
    `CreatorVersion`; the ABI has no producer field, so the producer row stays
    unset), creation/modification datetimes (ISO 8601 with offset, naive values
    read as UTC, plus the date-only form the invoice and the GB/T 33190 examples
    use), `n_pages`, and the per-page sizes in millimetres with the
    `PPS_DOCUMENT_INFO_N_PAGES`/`PAPER_SIZE` bits — `pps_document_info_pages` and
    `_regular_paper_size` both gate on `PAPER_SIZE`, which is what makes the
    properties dialog's page count and paper-size rows appear.
  - `src/ofd-document.c`: `OfdDocument` now implements `PpsDocumentPrint`
    (`G_DEFINE_FINAL_TYPE_WITH_CODE` + `G_IMPLEMENT_INTERFACE`); `print_page`
    draws at dpi 72/scale 1/rotation 0 into the borrowed context (the print
    operation owns the transform). Page-handle acquisition is factored into
    `ofd_document_page_handle`, shared by `render` and `print_page`. The renderer
    comes from `ofd_renderer_new` (below) and `get_info` is wired to the metadata
    module, walking the geometry cache and falling back to per-page queries.
  - `src/ofd-render.[ch]`: `ofd_renderer_new` reads
    `PAPERS_OFD_FALLBACK_FAMILIES` (colon-separated; an empty value disables
    fallback via rofd's non-NULL/zero-count contract) and otherwise passes NULL
    options for rofd's defaults; `ofd_render_options_init_print` is the print
    option set.
  - `meson_options.txt`/`meson.build`: `rofd_page_size` = `auto|disabled` (build
    switch for the phase-1 latency comparison) and `rofd_src` (enables the oracle
    test against `$ROFD_SRC`); the oracle test is only defined when both the
    fixture and the reference exist, with a warning otherwise.
  - `data/papers-ofd.xml`: the magic rules now cover both producer layouts —
    `PK\x03\x04`@0 plus `OFD.xml`@30 **or** `Doc_0/`@30. Found while wiring the
    oracle: the real invoice starts with `Doc_0/Annots/Annotations.xml`, so the
    `OFD.xml`-only rule matched nothing and GLib's data-only guess returned
    `application/zip` (`OFD.xml` only appears at byte 21707). Verified with
    `g_content_type_guess(NULL, first 1024 bytes)` and `xdg-mime query filetype`.
  - `tests/`: `ofd-test-utils.[ch]` (shared staging: MIME database +
    `PPS_BACKENDS_DIR` + `pps_init`), `generate_fixture.py` now generates
    `geometry`, `corner-marker`, `origin-offset`, `metadata` and `latency [count]`
    fixtures; `test-ofd-render.c` (rotation matrix on pixels, origin
    normalization, zoom matrix, same-options byte equivalence, print into a cairo
    PDF surface), `ofd-print-file.c` (real GTK `GtkPrintOperation` with
    `ACTION_EXPORT` and the same transform as
    `pps_print_operation_print_draw_page`, then poppler text extraction),
    `test-ofd-metadata.c`, `test-ofd-oracle.c`, `ofd-latency.c`.
- Files created/modified:
  - `src/{ofd-metadata.[ch],ofd-document.c,ofd-render.[ch],meson.build}`,
    `meson.build`, `meson_options.txt`, `data/papers-ofd.xml`,
    `tests/{ofd-test-utils.[ch],generate_fixture.py,test-ofd-render.c,ofd-print-file.c,test-ofd-metadata.c,test-ofd-oracle.c,ofd-latency.c,meson.build}`,
    `docs/plan.md` (§4 uniform-scale note, §5 phase-1 steps/evidence, §6 setup
    command + fixture matrix, §7 magic rules), `README.md` (status, build, latency
    limitation, roadmap status).
- GUI acceptance: observed by the user on the live session (dev backend dir +
  staged MIME) — rotation via `rotate right` twice moves the corner marker
  top-left → top-right → bottom-right, zoom in/out and fit modes resize cleanly,
  `Ctrl+P` shows the print preview and *Print to File* writes a PDF whose text
  selects, and the properties dialog lists Title/Author/Subject/Keywords/"Source
  file created with" plus the Created/Modified dates.
  - Non-fatal observation from the same session: using the print dialog three
    times logged `Gtk-WARNING: GtkPrintOperation::create-custom-widget not
    supported with portal` — Papers' page-scale/autorotate options are hidden when
    the print dialog runs through the xdg-desktop-portal (Papers 51 + GTK 4.24
    behaviour, unrelated to this module; the preview and the produced PDF were
    correct).

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
| `makepkg -C -f` (clean, wiped src/pkg) | `~/projects/librofd-ffi` | builds offline from the cached tarball + pinned patch | rc=0, `librofd-ffi-0.4.0-2-x86_64.pkg.tar.zst`, 78 exported symbols incl. the new one | pass |
| AUR push | `ssh://aur@aur.archlinux.org/librofd-ffi.git` | `master` advances to the new commit | `1e431d5..2391d47`, `git ls-remote` matches | pass |
| rofd PR CI (`gh pr checks 11`) | `linuxdeepin/rofd` | green | commitlint, license-check, cppcheck, Sourcery pass; `clacheck / CLAssistant` fails (Deepin CLA, contributor action); build jobs skipped until approval; APPROVALNOTIFIER wants an approver from `debian/deepin/OWNERS` | needs human action |
| `scripts/dev-env.sh` bare run | this host | report the resolution; exit 0 iff the ABI is available | `PAPERS_SRC ~/projects/papers missing`; `ROFD_SRC …/rofd` first `wrong repository (origin https://github.com/office-rs/rofd)`, later `incomplete checkout (no crates/rofd-ffi/include/rofd.h)` after the sibling directory was replaced by an unfinished clone; `ROFD_INCLUDEDIR /usr/include` (`ROFD_ABI_VERSION 1`); `ROFD_LIBDIR /usr/lib` with page-size fast path present; exit 0 | pass |
| `scripts/dev-env.sh` modes | fixture checkout, env override, broken override, `local.conf` | each mode behaves as documented | checkout → both dirs from the tree, SONAME warning + exit 1 while `librofd_ffi.so.0` is absent, exit 0 after `ln -s`; valid override → `ABI_SOURCE override`, exit 0; `ROFD_INCLUDEDIR=/nonexistent ROFD_LIBDIR=/nonexistent` → exit 1; `local.conf` values honored; sourced mode warns only, exports all four variables | pass |
| `bash -n scripts/dev-env.sh` | resolver | clean | clean | pass |
| Sibling-path audit (`git grep -E '(\.\./)+(papers\|rofd)/'`) | tracked files | no bare sibling paths outside the documented resolution-order default | one hit, the historical Phase-8 log line about the old rofd checkout (kept as history); all other references use `$PAPERS_SRC`/`$ROFD_SRC` | pass |
| `reuse lint` with `scripts/` added | repo | compliant | 11/11 files with copyright + license, GPL-2.0-or-later + LGPL-2.1-or-later | pass |
| Markdown fence balance | `docs/plan.md`, `README.md`, `AGENTS.md`, `patches/README.md` | even fence counts | 14 / 6 / 2 / 6, all balanced | pass |
| Papers dev surface (does the build need the source?) | installed `papers 51.1-1.1` | full backend API available without a checkout | `papers-document-4.0` 51.1; 43 `libdocument` + 20 `libview` headers under `/usr/include/papers/4.0/`; `-lppsdocument-4.0`, SONAME `libppsdocument-4.0.so.6`; `pps_backend_query_type` declared in `pps-document.h:261` and exported by all four installed backend modules (not by the library); `pps-backend-info.h` absent — Papers-side discovery only, no backend needs it | pass |
| rofd ABI surface (does the build need the source?) | installed `librofd-ffi 0.4.0-2` | header + library with the page-size fast path | `/usr/include/rofd.h` (`ROFD_ABI_VERSION 1u`), `/usr/lib/librofd_ffi.so.0` exporting `rofd_document_get_page_size_mm` | pass |
| Papers upstream tag for citations | GitLab API `projects/GNOME%2Fpapers/repository/tags?search=51` | tag `51.1` resolvable | <https://gitlab.gnome.org/GNOME/papers> tag `51.1` = commit `2c3976fb` (2026-10-09); blob base `…/-/blob/51.1/` | pass |
| Resolver report after the convention change | bare run | Papers described as optional, hint points upstream | `Papers sources are optional: … cites Papers files at https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/<path>`; rofd hint unchanged; exit 0 | pass |
| Meson Cargo-subproject capability for rofd | Meson 1.12.1 `mesonbuild/cargo/interpreter.py`, `modules/rust.py` | what Meson can consume without a meson port | interpreter refuses manifests with a `build.rs` ("an actual meson port will be required"): the rust module compiles sources through rustc, Cargo itself is never driven | pass |
| rofd upstream packaging surface | GitHub API root + `crates/rofd-ffi` listing | no Meson build, no `.pc` | root has Cargo/`debian`/`learning`/`resources`/`src`/`tests`/docs only; `crates/rofd-ffi/` = `Cargo.toml`, `build.rs` (88 B, SONAME), `include/`, `src/`, `tests/`, `README.md` | pass |
| Upstream `v0.4.0...main` review | GitHub compare API | know what later commits change | 10 commits; `0.4.3` = `140b936`, no tag; load-bearing: `fix(ffi): map SingularTransform diagnostic to C ABI v1` + crate vendoring; `ROFD_ABI_VERSION` still `1u`, `expected-symbols.txt` still 77 | pass |
| `librofd-ffi 0.4.3-1` build | pinned commit archive + page-size patch + `.pc.in` | package builds, offline | `makepkg -f --noconfirm` rc 0 in 35 s (`cargo build --frozen`, vendored crates); 1.4 MB package | pass |
| Package contents | `bsdtar`, `nm -D` | ABI + header + `.pc` | `usr/lib/librofd_ffi.so{,.0}`, `usr/include/rofd.h`, `usr/lib/pkgconfig/rofd-ffi.pc`, licence; `.so.0` exports 78 symbols incl. `rofd_document_get_page_size_mm`; header has `ROFD_ABI_VERSION 1u` (:75) and `ROFD_DIAGNOSTIC_SINGULAR_TRANSFORM 7u` (:147) | pass |
| `.pc` metadata | staged `PKG_CONFIG_PATH` | valid, queryable | `--modversion` 0.4.3; `includedir` `/usr/include`; `libdir` `/usr/lib`; libs `-lrofd_ffi`; `--validate` reports nothing for the file | pass |
| Pkg-config tier in the resolver | staged `.pc` from the built package | tier selects pkg-config and exports its dirs | `ABI_SOURCE pkg-config — rofd-ffi.pc 0.4.3`; `ROFD_INCLUDEDIR`/`ROFD_LIBDIR` from the file | pass |
| Page-size patch against fork `main` | `$ROFD_SRC` @ `140b936` | applies cleanly | `git apply --check` OK, 7 files, no fuzz | pass |
| `namcap` | package tarball + PKGBUILD | no errors | package: only the standard `gcc-libs` "may not be needed" warning; PKGBUILD clean | pass |
| Phase-0 build | `meson setup _build` + `meson compile` | module + fixture + tests build | `libofddocument.so` (exports `pps_backend_query_type`), descriptor copy and `geometry-3pages.ofd` produced; no warnings | pass |
| Phase-0 unit tests | `test-ofd-units` | conversions and origin normalization correct | mm↔pt exact, zero-origin and `PhysicalBox "10 20 …"` cases, inverted input order — 4/4 | pass |
| Phase-0 factory round-trip | `test-ofd-document` | module discovered by MIME, document loads | 3 pages, A4/A5/landscape sizes within 0.01 pt, `backend_info.name == "rofd"`, out-of-range pages NULL, `save` → `G_IO_ERROR_NOT_SUPPORTED`, `setup_cache` clean | pass |
| Phase-0 deliberate discovery failure | `test-ofd-document --expect-discovery-failure` | broken `Module=` leaves no backend for `application/ofd` | `pps_document_factory_get_document` → NULL with `PPS_DOCUMENT_ERROR_INVALID` ("File type … is not supported") | pass |
| Phase-0 runtime (OFD) | `PPS_BACKENDS_DIR=_build/devbackends papers geometry-3pages.ofd` (+ staged MIME tree) | opens and renders without errors | ran until killed by `timeout` (124); 0 critical/failed lines; earlier SIGSEGV traced to the missing `render` vfunc (A20) and fixed | pass |
| Phase-0 runtime (PDF regression) | same dev dir, ImageMagick tutorial PDF | other backends unaffected | ran until killed (124), 0 critical/failed lines | pass |
| AUR publication | `git push` to `ssh://aur@aur.archlinux.org/librofd-ffi.git` | `master` advances to `846dc9a` | push reported `456e65b..846dc9a`; a fresh `git ls-remote` returns the same commit; RPC `Version` = `0.4.3-2` | pass |
| Fork sync | `git fetch upstream main`, `rev-list --left-right --count` | fork `main` mirrors upstream | `0 0` at `140b936`; branches `feat/ffi-page-size-query` (`7f6d24d`) and `feat/pkgconfig` on the fork | pass |
| Upstream PR #12 | `gh pr create` on `linuxdeepin/rofd` | PR opens with the diff | <https://github.com/linuxdeepin/rofd/pull/12> (`feat/pkgconfig`, OPEN); `gh pr list --author YangtseSu` shows #11 and #12 both open | pass |
| Generated `.pc` (Debian path) | same `sed` as `debian/rules` with `0.4.3` / `x86_64-linux-gnu` | metadata correct | `Version: 0.4.3`, `Requires: cairo`, `--libs` → `-L/usr/lib/x86_64-linux-gnu -lrofd_ffi -lcairo`; `--validate` clean; recipe lines tab-indented (`cat -A`) | pass |
| Phase-1 build (fast path) | `meson setup _build -Drofd_src=$ROFD_SRC` + compile | module, 6 test programs, 5 fixtures | built without warnings; `rofd_document_get_page_size_mm` probe passes | pass |
| Phase-1 build (fallback path) | `meson setup _build-slow -Drofd_page_size=disabled` | same tests on the per-page path | built; `meson test` 7/7 | pass |
| `meson test -C _build` | 7 tests | all pass | 7/7 OK (units, document, discovery-failure, render, print-file, metadata, oracle) | pass |
| Rotation matrix | `corner-marker.ofd` via `pps_document_render` at 0/90/180/270 | marker in the expected corner, others white | marker top-left/top-right/bottom-right/bottom-left; canvas dims swap at 90/270; rofd's clockwise rotation matches `rotation_degrees = rc->rotation` (no flip needed) | pass |
| Origin normalization | `origin-offset.ofd` (`PhysicalBox "10 20 210 297"`, marker covering the box) | whole canvas painted | 220/220 sampled pixels dark (a missing origin subtraction would leave it blank) | pass |
| Zoom matrix | scales 0.5/1.0/1.5/2.0/3.25 and `target_size` 800×ceil | surface == `rofd_renderer_get_pixel_canvas_size`, Papers' request ±1 px | exact match with rofd's canvas in every case; ≤1 px from `pps_render_context_compute_scaled_size` | pass |
| Same-options equivalence | `metadata.ofd` (glyphs included) | byte-identical to a direct FFI render | identical (row-wise `memcmp` over the whole surface) | pass |
| Print into cairo PDF | `print_page` → `cairo_pdf_surface` | vector text extractable | poppler text extraction returns "Invoice 42" and "发票编号 42" | pass |
| GTK print to file | `ofd-print-file` (real `GtkPrintOperation`, `ACTION_EXPORT`) | PDF with selectable text | wrote `_build/phase1-print-to-file.pdf`; poppler in-test and `pdftotext` both extract "Invoice 42 / 发票编号 42" | pass |
| Metadata mapping | `metadata.ofd` + `geometry-3pages.ofd` | fields, mask bits, page sizes | title/author/subject/keywords "alpha, beta"/creator "fixture-generator 1.0", creation 2025-01-06, mod 2024-10-22; `PAPER_SIZE`+`N_PAGES` set, `pps_document_info_pages` → 1; three pages 210×297/148×210/297×210 mm; no bits set without DocInfo | pass |
| Oracle vs `invoice-phase3.png` | 254 dpi, `PAPERS_OFD_FALLBACK_FAMILIES="Noto Sans CJK SC:Noto Sans Mono"` | same size, within tolerance | 2115×1400 both; mean channel difference 3.283, >8 2.61 %, >64 1.86 % (limits 10 / 8 % / 5 %) | pass |
| MIME magic | invoice (first entry `Doc_0/Annots/…`) and generated fixtures | `application/ofd` from data alone | `g_content_type_guess(NULL, 1024 bytes)` → `application/ofd` for both; a plain zip stays `application/zip` | pass |
| Open latency | `latency-300pages.ofd`, 5 runs, both builds | record load → first page | fast: load 2.0 ms, metadata walk 20 ms, `setup_cache` 71 ms, first render 46 ms; fallback: load 2.0 ms, metadata walk 71 ms, `setup_cache` 0.1 ms, first render 47 ms | pass (recorded, no regression) |
| Papers session run (phase 1) | `papers _build/tests/metadata.ofd` with the dev backend dir | no criticals | ran 15 s until `timeout` (124); 0 CRITICAL/ERROR; one unrelated `Gdk-WARNING` about frame timings; our diagnostics show CJK-capable font fallback on both text objects | pass |
| GUI acceptance (visual) | live session, dev backend dir | rotation/zoom/print/properties behave | user-confirmed: marker top-left → top-right → bottom-right on `rotate right`, zoom and fit modes clean, `Ctrl+P` preview renders and *Print to File* text selects, properties dialog shows title/author/subject/keywords/creator/dates | pass |
| REUSE after phase 1 | repository | compliant | 41/41 files with copyright + license | pass |

## Error Log

| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-10-14 | `read` elided C function bodies (declarations-only view) | 1 | Re-read the exact byte ranges with `sed -n` |
| 2026-10-14 | Python in-place edit assertion failed during plan section renumbering | 1 | Re-grepped the exact strings, re-ran the script; it only writes after all assertions pass |
| 2026-10-14 | Oracle run failed with "File type Zip archive (application/zip) is not supported" on the real invoice | 1 | The `OFD.xml`@30 magic only matched archives whose first entry is `OFD.xml`; the invoice starts with `Doc_0/Annots/Annotations.xml` (`OFD.xml` sits at byte 21707), so GLib's data-only guess fell back to `application/zip`. Added a `Doc_0/`@30 alternative to `data/papers-ofd.xml` and verified with `g_content_type_guess(NULL, first 1024 bytes)` and `xdg-mime query filetype`; a plain zip still reports `application/zip` |
| 2026-10-14 | `ofd-latency` printed the same value for every phase | 1 | `milliseconds_since()` was called with the wrong base at print time; the tool now records a timestamp per phase and computes explicit deltas |
| 2026-10-14 | `ofd-render` target-size case failed (1132 px instead of 600) | 1 | rofd's options carry a single uniform scale, so a target with a different aspect ratio than the page cannot be reproduced (poppler scales the axes independently). Papers always derives target sizes from the page size (`pps-pixbuf-cache.c:1041-1044,1113-1118`), so the test now does too; the deviation is documented in plan §4 and the README limitations |
| 2026-10-14 | Headless GUI rendering via GTK broadway abandoned | 3 | `broadwayd` binds its TCP port but creates no unix socket in this environment and GTK4's broadway client needs it (`BROADWAY_DISPLAY=:N` → `$XDG_RUNTIME_DIR/broadwayN`); TCP display strings are not accepted. GNOME Shell's screenshot D-Bus API answers "Access denied", and no Xvfb/nested compositor is installed. GUI acceptance is therefore observed on the live session (log-level) plus the user's visual check |

## 5-Question Reboot Check

| Question | Answer |
|----------|--------|
| Where am I? | `docs/plan.md` phases 0 and 1 complete: module skeleton, rendering/rotation/zoom matrix, printing (vector text), metadata, oracle comparison and the latency table; `meson test` 7/7 in both page-size builds |
| Where am I going? | Phase 2: `PpsDocumentText` (five vfuncs), `PpsSelection`, `PpsDocumentFind`, links and the outline; then phase 3 (fd loading, packaging, install checks) |
| What's the goal? | Ship the OFD backend for Papers 51.x, out-of-tree, on the rofd ABI we maintain (fork-first, AUR package) |
| What have I learned? | See `findings.md` + the phase-1 log: the render path needs one uniform scale (plan §4), rofd's rotation is clockwise and matches `rc->rotation` as-is, the real invoice's first zip entry is `Doc_0/…` (so the MIME magic needs both layouts), and `setup_cache` fetches every page object — which is why the metadata-only page-size query does not change the shell's open time (plan §5 phase 1) |
| What have I done? | Plan/README machine-portable and REUSE-compliant; rofd page-size patch shipped via our AUR package and proposed upstream (#11), `rofd-ffi.pc` proposed upstream (#12) and shipped; phases 0 and 1 implemented, tested and committed; GUI acceptance run on the live session |

---

*Update this file after completing a phase, running validation, or encountering an error.*
