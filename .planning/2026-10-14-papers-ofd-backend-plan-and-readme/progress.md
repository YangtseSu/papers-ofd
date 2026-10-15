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
    `office-rs/rofd` — a different OFD project with the same name (corrected
    2026-10-15, see findings P31) — so the documented build commands and every rofd
    citation silently pointed at the wrong tree.
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

### Phase 15: docs/plan.md phase 2 — text, selection, find, links, outline (implementation)

- **Status:** complete except the GUI acceptance on the live session
- Actions taken:
  - `src/ofd-text.[ch]` (new): `PpsDocumentText` (`get_text`, `get_text_layout`,
    `get_text_in_area`, `get_text_mapping`, `get_text_attrs` = NULL because the ABI
    has no attribute data), `PpsSelection` (`get_selected_text`, `get_selection_region`,
    `render_selection` = base colour only, no glyph mask) and `PpsDocumentFind`
    (`get_supported_options`, `find_text` with `max_results = 4096`). Shared plumbing
    went to `ofd-page.[ch]` (`OfdPageRef`: page handle + physical box in one acquire/
    release pair), `ofd-render.[ch]` (`ofd_render_page_canvas_size`) and `ofd-units.[ch]`
    (`ofd_pixel_rect_from_points`, the poppler-style half-pixel rounding used for both
    the page-point text mapping and the device-pixel selection region).
  - `src/ofd-links.[ch]` (new): `ofd_links_get_links` (one `PpsMapping` per region of
    the first CLICK action, source order, `PpsMappingList` owns one reference per
    mapping), plus `ofd_link_action_from_rofd`/`ofd_link_dest_from_rofd` shared with the
    outline: destination coordinates are re-based on the **target** page's box origin,
    presence bits → change flags, zoom → `change_zoom` only when non-zero (GB/T 33190
    table 54), unresolved GOTO → no link, relative URIs resolved against `Base`.
  - `src/ofd-outline.[ch]` (new): the `PpsOutlines` model rebuilt from
    `parent`/`first_child`/`next_sibling` (one children store per node with children),
    `NULL` when there is no outline; every node gets a `PpsLink` because the sidebar
    dereferences the selected row's link unconditionally.
  - `src/ofd-document.c`: the four interfaces registered next to `PpsDocumentPrint`
    (every vfunc, stubs for `find_link_dest`/`find_link_page`), `ofd_document_borrow_handle`,
    and `ofd_document_page_box_mm`/`ofd_document_borrow_page`/`ofd_document_borrow_renderer`
    exposed to the new modules.
  - Fixtures (`tests/generate_fixture.py`): kinds `text` (two-column ASCII page + CJK
    page), `scan` (embedded PNG, no text), `links` (page-root URI, object GOTO, object
    URI with `Base`, 4-node outline, named bookmark), `origin-text`
    (`PhysicalBox "10 20 210 297"` with text **and** a link), `broken-page` (truncated
    content XML), `malformed-zip`; Meson targets for all of them.
  - Tests: `test-ofd-text.c`, `test-ofd-links.c`, `test-ofd-errors.c` — every text,
    selection, find, link and outline answer is compared against a direct rofd query of
    the same fixture and the same coordinates, plus NULL/out-of-range page robustness,
    origin normalisation on both text and link geometry, and the byte-only MIME
    detection.
  - Two real defects found while wiring this:
    1. `pps_outlines_new` and `pps_outlines_set_link` are **declared in the installed
       header but not defined/exported** (`nm -D` on `libppsdocument-4.0.so.6`), so the
       module failed to link; `g_object_new (PPS_TYPE_OUTLINES, NULL)` and the `link`
       property are the usable paths (A21).
    2. The shipped MIME file had magic priority 50, below `application/zip`'s 60, so
       **content-only detection answered `application/zip`** for every OFD file — which
       is the only detection the previewer and `load_fd` (phase 3) have. Raised to 70
       (the shared-mime-info container-format priority) and asserted from a test (A22).
    Also corrected the phase-1 latency claim's counterpart: `get_text_layout` returns
    TRUE with zero areas on a text-less page (every consumer guards on `!areas`).
  - Test-fixture detail worth keeping: rofd advances every glyph by ≈ `0.96 × Size`
    millimetres when the named font is absent, so the two-column fixture uses `Size=3`
    to stay inside its columns; the geometry is asserted by the tests.
  - `test-ofd-search.c`: runs Papers' **own** search context (the Ctrl+F sidebar's
    pipeline: find job → per-page results → `get_match_offset` pairing with the text
    layout → snippet markup) over the text fixture, so the acceptance criterion
    "the snippet is the sentence around the match" is checked headlessly for the
    case-insensitive ASCII page, a case-sensitive query, a whole-word query that must
    find nothing, and the CJK page. Two upstream behaviours surfaced while wiring it:
    the context builds page labels from the document's page cache (the test calls
    `pps_document_setup_cache`, as the shell's load job does), and **a
    `PpsSearchContext` cannot be finalized at all** — its dispose passes the
    `GHashTable *per_page_store` to `g_clear_object ()`, so `g_object_unref` reads the
    hash table as a `GTypeInstance` and segfaults (A23). The test therefore leaks the
    context and the model on purpose; the shell never reaches that path in a normal
    session.
- Files created/modified:
  - `src/{ofd-text.[ch],ofd-links.[ch],ofd-outline.[ch],ofd-document.[ch],ofd-page.[ch],ofd-render.[ch],ofd-units.[ch],meson.build}`,
    `tests/{generate_fixture.py,meson.build,test-ofd-text.c,test-ofd-links.c,test-ofd-errors.c,ofd-print-file.c}`,
    `data/papers-ofd.xml`, `docs/plan.md` (§2/§3/§5/§6/§7/§8, A21/A22), `README.md`

### Phase 16: rofd font-resolved text geometry (the Ctrl+F highlight offset)

- **Status:** complete — user-confirmed on the live session (2026-10-15): after the
  first fix the rectangles were aligned but visibly wider than the match, and after
  the box-width commit `812073e` (installed here as `librofd-ffi 0.4.3-4`) the user
  reports the highlight is right. The `on` box narrowed from 4.82 mm to 3.65 mm (the
  sum of the two glyph advances).
- User decision (2026-10-15): the AUR package stays **local** (not pushed to the AUR)
  until rofd#13 is reviewed or merged. The user installed the locally built package on
  this machine instead: `librofd-ffi 0.4.3-4` is what the module now links (`ldd`
  resolves `/usr/lib/librofd_ffi.so.0`), so no `LD_LIBRARY_PATH` is needed and both
  suites run with the strict geometry assertions green.
- Reported by the user during the phase-2 GUI acceptance: with Ctrl+F, the highlight
  for English/number matches did not sit on the searched text but to its right; CJK
  was fine. Not a module bug — a rofd geometry model mismatch:
  - `rofd-core` advanced every scalar of a run **without** an explicit `DeltaX` by the
    font size (`semantic.rs`: `dx = … else size`), while `rofd-render`'s positioner
    places such a scalar by the resolved font's advance ("only a genuinely absent
    axis uses the selected glyph advance"), i.e. the layer that draws and the layer
    that reports rectangles disagreed for every document whose producers rely on font
    metrics. CJK aligns because a full-width glyph's advance *is* the font size.
  - Measured on our own fixture (21-character line at size 3 mm, no `DeltaX`): boxes
    spanned 63.0 mm, the drawn ink 28.2 mm, and the `42` match rectangle sat at
    60.0–66.0 mm while the line ends at 43.4 mm → the highlight landed in blank space
    4.5 mm right of the text (A24).
- Fix (fork-first, D11): `feat/font-text-advances` on `YangtseSu/rofd`, commit
  `6f98928`, upstream PR <https://github.com/linuxdeepin/rofd/pull/13>:
  - `rofd-core`: `TextAdvanceProvider` trait, `Document::from_bytes_with_advances` /
    `open_with_advances`, and the builder asks it per text object when a run has no
    explicit delta; without a provider nothing changes (the font-size footprint
    stays, still flagged conservative). Explicit deltas stay authoritative.
  - `rofd-render`: `PositionedGlyph::advance` (the advance each glyph contributed)
    and `FontTextAdvances`, which reuses `position_glyph_runs` so the provider cannot
    drift from the positioner.
  - `rofd-ffi`: documents load with a provider built from the default font stack
    (the one a renderer created with NULL options uses); `rofd.h` documents the
    caveat for renderers configured with other families.
  - Verified on the rofd side: `cargo fmt --all -- --check`,
    `cargo clippy -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings`,
    `cargo test` for the three crates (new tests: provider seam in the core builder,
    provider/positioner agreement on an embedded subset font, ABI layout + search
    rectangles following font metrics), `crates/rofd-ffi/tests/run_c_tests.sh` (rc=0,
    symbol list unchanged). Pre-existing and unrelated: `rofd-render`'s
    `decoded_limits_have_exact_and_one_over_boundaries` fails on `main` too.
  - Carried as `patches/rofd-ffi-font-advances.patch` (applies cleanly to the fork's
    `main`, verified with `git apply --check`) and applied by the AUR package
    `librofd-ffi 0.4.3-3`; the packaged library reproduces the font-aware geometry
    (44.84 mm span instead of 78.00 mm for the fixture's first line).
- Module side: no code change was needed (the ABI is unchanged; only behaviour). New
  regression test `test_find_rectangles_cover_rendered_ink` renders the page through
  rofd, measures the ink of the line, classifies the linked library (font-resolved vs
  size-based) and asserts that every left-column match rectangle overlaps the ink of
  its own line — which fails on the pre-fix geometry (`42` at 170 pt vs ink ≤ 123 px)
  and passes after it (96.1–109.3 pt). The selection test now samples glyph centres
  from the layout instead of the fixture's declared column boxes, because the glyphs
  are narrower than those boxes.
- Files modified (here): `patches/rofd-ffi-font-advances.patch` (new),
  `patches/README.md`, `tests/test-ofd-text.c`, `docs/plan.md` (§3/§6/§8/§9 + A24),
  `README.md`, `.planning/.../{progress,task_plan}.md`
- Files modified (in `~/projects/rofd`): `crates/rofd-core/src/{semantic,document,lib}.rs`,
  `crates/rofd-render/src/{fonts,lib}.rs`, `crates/rofd-ffi/src/{document,renderer}.rs`,
  `crates/rofd-ffi/include/rofd.h`, tests in all three crates, the semantic-API design doc

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
| Phase-2 build (both page-size paths) | `meson setup` + `compile` on `_build` and `_build-slow` | module + 10 tests | built without warnings; `meson test` 10/10 in both | pass |
| Text vs the ABI | `text-2pages.ofd` via `pps_document_text_get_text` | byte-identical to `rofd_page_get_text` | equal on both pages (148 and 25 scalars) | pass |
| Text layout | same fixture | one rect per Unicode scalar, inside the page box, separators empty | counts equal `g_utf8_strlen`, object separators have empty rects, all rects inside the box | pass |
| Text in area | left column rectangle | equals `rofd_page_get_text_for_area` for the same mm rect | equal; contains the left column, no right-column text | pass |
| Text mapping (I-beam) | whole page glyph selection | region covers glyphs, not empty space | glyph centre contained, page corner (2, 2) not, extents inside the page | pass |
| Find counts | page 0 / page 1 | case-insensitive, case-sensitive, whole-word, CJK | `invoice` 2, case-sensitive 1, `INVOICE` 2, `invo`+whole-words 0, `发票编号` 2, absent 0, empty query 0 | pass |
| Find/layout agreement | every match rectangle | the search context's `get_match_offset` finds a layout rect, and the query starts there | satisfied for all matches; the scalar found starts with the query (case-insensitive) | pass |
| Selection text | three styles, whole page | equals `rofd_page_get_selected_text` per style | GLYPH/WORD/LINE all byte-equal; one column ≠ the other; empty rect → "" (never NULL) | pass |
| Selection region | scale 1 and a doubled target size | inside rofd's canvas, contains glyph centres, grows with the target | inside the canvas in both cases, glyph centre contained, extents > scale-1 extents and ≤ the larger canvas | pass |
| Selection overlay | base `{1, 0, 0, 0.5}` | canvas-sized surface, base colour inside, transparent outside, cleared on reuse | size == rofd canvas; inside alpha/red ≈ 0x80; outside alpha 0; after reuse for the other column the first column is transparent again; rotated page → no surface | pass |
| Scan page (no text) | `scan-1page.ofd` | empty text, 0 layout areas, empty regions, no matches | `""` (non-NULL), TRUE with `areas == NULL`, empty mapping/selection regions, `find_text` NULL | pass |
| Broken page / NULL page | `broken-page-2pages.ofd` + hand-built `PpsPage(1)` + NULL | no crash, "no text" answers | page 0 text OK, page 1 unreachable, every vfunc returns NULL/FALSE/"" without a critical | pass |
| Page links vs the ABI | `links-2pages.ofd` page 0/1 | three CLICK actions in source order, regions and destinations equal to rofd's | regions equal to the ABI's mm→pt conversion, GOTO page/left/top/zoom equal, URI with `Base` resolved to `https://example.test/base/relative/target`, page 1 whole-page XYZ 20/2 | pass |
| Link hit test | centre of the GOTO box | the smaller mapping wins over the full-page URI action | returns the GOTO mapping (`pps_mapping_list_get` smallest-area rule) | pass |
| Outline model | `links-2pages.ofd` | 4-node preorder tree, 3 roots, one child, expand flags | model has 3 roots, node 0 has 1 child, `Expanded=false` only there, markup escaped, every row has a link, `link_page` returns 0/1/-1 as expected | pass |
| Outline destinations | Fit/FitH/XYZ+bookmark | type, page, coordinates, zoom from the ABI | FIT page 0; FIT_H page 1 top 25 mm; XYZ via bookmark page 1 left 10 top 30 zoom 1.5 | pass |
| Links/outline stubs and bounds | `find_link_dest`/`find_link_page`, NULL/out-of-range page, document without outline | NULL/-1, no mappings, no model | as specified | pass |
| Origin normalisation (D3) | `origin-text.ofd` (`PhysicalBox "10 20 210 297"`) | text/layout/find/selection and link region + destination all relative to the box origin | first glyph rect x1 ≈ 5 mm, y1 ≈ 2 mm; mapping extents < 20 mm; link area 0/80 mm; destination left 5 mm, top 6 mm, `change_zoom` FALSE for the explicit `Zoom="0"` | pass |
| Load error paths | `malformed-zip.ofd`, a truncated copy, a missing path | `G_IO_ERROR_INVALID_DATA` (or no backend), no crash | malformed and truncated fail with `INVALID_DATA`; missing file has no backend; `n_pages` stays -1; reload after a failure recovers to 2 pages | pass |
| Byte-only MIME detection | first 1024 bytes of a fixture and of a hand-built plain zip | `application/ofd` / `application/zip` | `application/ofd` at the new priority 70, plain zip stays `application/zip` | pass |
| Papers session (phase 2) | `papers` on the text, scan and links fixtures with the dev backend dir | jobs run, no criticals | links job and page-data jobs for every visible page (our outline/text/mapping/link vfuncs) with 0 CRITICAL/ERROR; PDF in the same dev dir still opens | pass |
| `reuse lint` after phase 2 | repository | compliant | 50/50 files with copyright + license | pass |
| Search snippets (Ctrl+F rows) | `text-2pages.ofd` through `PpsSearchContext` | 2 hits for `invoice` on page 0 with the surrounding words, 1 for the case-sensitive query, 0 for `invo` with whole words, 2 CJK hits on page 1 | markup contains `<span weight="bold">` + the source spelling + `total: 42 EUR` / `appears lowercase` / `中文搜索测试` as applicable; `first_match_page` 0, 0, -1, 1 | pass |
| Memory checking | the two new test programs under `G_SLICE=always-malloc MALLOC_PERTURB_` | no crash | pass; valgrind is not installed on this host, so no leak report was produced | pass (limited) |
| rofd geometry model (diagnosis) | `text-2pages.ofd` page 0 rendered + ink-scanned; layout spans compared | decide whether the offset is ours or rofd's | rofd's semantic span 63.0 mm vs ink 28.2 mm (`dx = size`), patched build 29.8 mm vs 28.2 mm; `42` match at 60.0–66.0 mm before, 33.9–38.6 mm after | root cause in rofd |
| rofd fork tests after the fix | `cargo test -p rofd-core -p rofd-render -p rofd-ffi` | all green apart from the pre-existing image test | green; the images test fails on `main` as well | pass |
| rofd C ABI runner | `crates/rofd-ffi/tests/run_c_tests.sh` | rc=0, symbols unchanged | rc=0 (export list, C/C++ headers, smoke) | pass |
| Our suites against the patched ABI | `_build-fork` (checkout tier, `PKG_CONFIG_LIBDIR` hides the `.pc`, `LD_LIBRARY_PATH` at run time) | 11/11 with the strict ink alignment | 11/11; the ink test reports `font-resolved geometry` (29.8 vs 28.2 mm) and every match overlaps its ink | pass |
| Our suites against stock rofd | `_build`, `_build-slow` | 11/11 without alignment assertions | 11/11; the ink test reports `font-size footprint geometry` and checks only the well-formedness invariants | pass |
| AUR `librofd-ffi 0.4.3-3` | PKGBUILD with both patches, clean `src/`+`pkg/` | builds, packaged library carries both fixes | `makepkg -f --noconfirm` rc=0; the packaged `librofd_ffi.so.0` exports 78 symbols incl. `rofd_document_get_page_size_mm` and reproduces the font-aware geometry under `LD_LIBRARY_PATH` | pass |
| AUR `librofd-ffi 0.4.3-4` | PKGBUILD with both patches (the font one carrying the box-width commit) | builds; the packaged library reproduces the tightened boxes | `makepkg -f --noconfirm` rc=0; the packaged `librofd_ffi.so.0` gives the `on` match a 3.65 mm box (4.82 mm before that commit) | pass |
| Suites with the installed `0.4.3-4` | `meson test` on `_build` and `_build-slow` | 11/11 including ink alignment and box tiling | 11/11; the classifier reports `layout 28.7 mm, ink 28.2 mm -> font-resolved geometry` | pass |
| Selection region vs rendered ink | phase-2 text fixture, second left-column line at 4× zoom, region from `pps_selection_get_selection_region` compared with the ink rows rofd renders | the region covers the ink's bottom row (426..461 inside 420..461); the same test fails against librofd-ffi 0.4.3-4 (region ends at 453) | pass |
| Phase-2 GUI acceptance (live session) | `papers` on the text and links fixtures from the installed package | every acceptance item observable | user-confirmed 2026-10-15: selection/copy, Ctrl+F rows, caret, I-beam, links and outline are correct, and the Ctrl+F highlight sits on the searched text after the geometry fix | pass |
| Papers session with the patched library | `LD_LIBRARY_PATH=$ROFD_SRC/target/release papers text-2pages.ofd` | loads, no criticals | ran 12 s, 0 CRITICAL/ERROR (only the expected font-fallback diagnostics) | pass |

| Phase-3 build (three configurations) | `meson setup --reconfigure` + compile on `_build`, `_build-slow`, `_build-fork` | module + the new test build without warnings | built in all three; `meson test` 12/12 in each (the new `ofd-fd` included) | pass |
| fd vs URI equivalence | `test-2pages.ofd` and `metadata.ofd` through `pps_document_factory_get_document_for_fd` + `pps_document_load_fd` | the same document as the URI path | page count, page sizes, page text, rendered pixels (memcmp at scale 1.5) and document info (fields, mask, datetimes) identical; the caller's descriptor is closed, the load job's duplicate too | pass |
| fd ownership and rejection | pipe, directory, write-only regular file, plus the success path | descriptor consumed in every outcome | `G_FILE_ERROR_BADF` for all three rejections ("Not a regular file."/"Not a readable file descriptor.") with the fd closed; on success the passed fd is closed and the document still renders | pass |
| Unlinked file over fd | copy of the fixture, `unlink` before the load | loads through `/proc/self/fd`, no temp copy | 2 pages, page text equal to the URI document, `$XDG_CACHE_HOME/papers-ofd` never created | pass |
| Temp-copy fallback | descriptor whose read permission was withdrawn (`chmod 000`) after the open | copy made, used, removed | loads and matches the URI document; exactly one `ofd-fd-XXXXXX` in `$XDG_CACHE_HOME/papers-ofd` while in use, none after a URI reload or at finalize | pass |
| fd error parity | truncated copy of the text fixture | same error as the URI path | both `G_IO_ERROR_INVALID_DATA` with an identical message; the same object then loads a good descriptor (2 pages) | pass |
| fd test under MALLOC_PERTURB_ | `G_SLICE=always-malloc MALLOC_PERTURB_=165 ./tests/test-ofd-fd …` | clean | exit 0, no criticals | pass |
| Package build | `packaging/PKGBUILD`, `makepkg -f --noconfirm` | package with module + 4 data files | `papers-ofd 0.1.0-1` (47.5 KB); file list: `usr/lib/papers/6/backends/{libofddocument.so,org.gnome.Papers.OfdDocument.papers-backend}`, `usr/share/{mime/packages/papers-ofd.xml,thumbnailers/papers-ofd.thumbnailer,applications/org.gnome.Papers.Ofd.desktop}` | pass |
| Package `check()` | the suite inside `makepkg` | tests pass in the build | 11/11 (`ofd-oracle` needs `-Drofd_src`); `ofd-print-file` ran for real (exit 0, no display skip) | pass |
| `namcap` | package tarball + PKGBUILD | no errors | package: only the `gcc-libs` advisories; PKGBUILD: silent; the `shared-mime-info`/`desktop-file-utils` warnings disappeared once they were dropped (both arrive through `papers`) | pass |
| Source pin | anonymous download of the pinned commit archive | sha256 matches | `64c1797d6bcc5fb4baa66400f8e2f3cd0d6de6ab22ef8a595fbaac259b28853b` from an unauthenticated `curl`, and a `makepkg` run that downloaded it itself (empty `SRCDIR`) | pass |
| AUR publication | `git push ssh://aur@aur.archlinux.org/papers-ofd.git` | package appears | `master` = `86e4929` (`papers-ofd 0.1.0-1`), package page HTTP 200; the RPC index lagged at first (`resultcount 0`) and now reports `papers-ofd 0.1.0-1` by `yangtsesu` | pass |
| Staged install checks | package extracted to a prefix + `update-mime-database`/`update-desktop-database` | MIME and association resolve | `desktop-file-validate` clean; `xdg-mime query filetype` → `application/ofd`; `xdg-mime query default application/ofd` → `org.gnome.Papers.Ofd.desktop`, also the recommended application in `gio mime` | pass |
| Thumbnailer (packaged module) | `papers-thumbnailer -s 256` with `PPS_BACKENDS_DIR` = system dir + the package's files | PNG with a rendered page | 181x256 (text fixture), 256x170 with mean 63934/65535 and sd 6312 (real invoice); the PDF regression thumbnail is 198x256 | pass |
| Previewer fd path | `papers-previewer --fd 3 < text-2pages.ofd` (dev dir, then installed) | document loads and renders | both runs: 334 lines of this module's render diagnostics, 0 CRITICAL/ERROR, window up until `timeout` | pass |
| `papers` and `gio open` | installed package, no env overrides | OFD opens | `papers <fixture>`: 670 debug lines from the module, 0 CRITICAL/ERROR; `gio open` starts `/usr/bin/papers …/text-2pages.ofd` | pass |
| Installed package | `pacman -U /tmp/ofd-makepkg/papers-ofd-0.1.0-1-x86_64.pkg.tar.zst` (user) | clean install | installed 03:15:37 from the current recipe (`Depends On: cairo gcc-libs glib2 glibc librofd-ffi>=0.4.3 papers>=51 papers<52`), no file conflicts; installed `libofddocument.so` md5 matches the built package; `xdg-mime query filetype/default` and `papers-thumbnailer` work from `/usr` with no overrides | pass |
| GUI acceptance (phase 3) | live session: Nautilus space bar, Nautilus icon/thumbnail, double-click, app grid | visible behaviour | **user-confirmed** (see the session section) | pass |

| Space-bar preview, first acceptance run | user, Nautilus 51.1 + Sushi 51.2, space on an OFD file | preview window with the page | **no preview**: only the file-information panel | observed (user) |
| Previewer root cause | Sushi 51.2 sources + the installed resource | decide whether it is ours | not ours and not `load_fd`: Sushi renders documents in-process via `gi://PapersDocument`/`PapersView` and loads by URI; its document renderer claims only the MIME types of `org.gnome.Papers.desktop`, which lacks `application/ofd` → `FallbackRenderer` (A26) | root cause identified |
| Sushi plugin mechanism | gjs against the installed Sushi 51.2 | the selector can be taught the type | with `data/papers-ofd-sushi.js` imported as Sushi does (plugins load before the selector's map is built) `selectRenderer('application/ofd')` returns the Papers-backed `PdfRenderer` instead of the fallback, and the renderer loads the fixture (2 pages) | pass |
| `ofd-sushi-plugin` test | `meson test -C _build` | the coupling is covered | 13/13; the test prints "the selector routes application/ofd to the Papers-backed document renderer" and "Sushi's document renderer loaded the OFD fixture: 2 pages" | pass |
| Duplicate "Open With" row | user report + `gio mime application/ofd`, nautilus-app-chooser-widget.c | explain | `application/ofd` has exactly one registered entry (ours, also the default and recommended); Nautilus' chooser lists `g_app_info_get_all()` on top of default/recommended/fallback, so Papers' own entry ("文档查看器") appears as a second row that runs the same binary | explained (A26) |

| AUR `papers-ofd 0.1.0-2` | `git push` to the AUR with the plugin release | the package carries the plugin | local mirror `c4952ad` (`pkgrel = 2`, `optdepends sushi`), `git ls-remote origin master` confirms it on the server; the AUR *web* index still rendered `0.1.0-1` a minute later (frontend/RPC lag, as with the first push) | pass |
| Package `0.1.0-2` build | `makepkg -f --noconfirm`, empty `SRCDIR` | plugin installed, tests pass | anonymous download verified (sha256 `a8ec59ce…`), 12/12 tests in the build (ofd-sushi-plugin included), file list = the five previous paths + `usr/lib/sushi/plugins-1/papers-ofd-sushi.js`, namcap clean apart from the gcc-libs advisories | pass |

| Packaged plugin alone | `gjs` probe with only `/usr/lib/sushi/plugins-1/papers-ofd-sushi.js` (temporary `~/.local/share/sushi/plugins-1` copy removed) | the package carries the feature | selector returns the Papers-backed renderer, the renderer loads the fixture (2 pages) | pass |
| Phase-3 GUI acceptance | live session, `papers-ofd 0.1.0-2` installed | the four session items | **user-confirmed 2026-10-15**: space bar shows the page, Nautilus shows the OFD icon and a rendered thumbnail, double-click opens Papers, the app grid still lists one Papers entry; the temporary user-level plugin copy was removed afterwards | pass |
| AUR `papers-ofd 0.1.0-2` indexed | AUR RPC + package page | visible to users | RPC reports `papers-ofd 0.1.0-2` by `yangtsesu` with `OptDepends ['sushi']`; the page renders `Package Details: papers-ofd 0.1.0-2` | pass |

## Error Log

| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-10-14 | `read` elided C function bodies (declarations-only view) | 1 | Re-read the exact byte ranges with `sed -n` |
| 2026-10-14 | Python in-place edit assertion failed during plan section renumbering | 1 | Re-grepped the exact strings, re-ran the script; it only writes after all assertions pass |
| 2026-10-15 | Module failed to link: `undefined reference to pps_outlines_new` | 1 | The function is declared in the installed header but not exported by `libppsdocument-4.0.so.6` (nor defined in Papers' source); switched to `g_object_new (PPS_TYPE_OUTLINES, NULL)` and, for the link, the `link` property, whose setter takes its own reference (A21) |
| 2026-10-15 | New test programs reported `File type Zip archive (application/zip) is not supported` for every fixture | 1 | My manual runs passed `../src/libofddocument.so` while the tests resolve argv paths against the build directory (meson passes `src/libofddocument.so`), so the staged symlink dangled and `g_module_open` failed. Confirmed by comparing a passing test; the tests themselves were fine |
| 2026-10-15 | `test_text_layout` failed: a glyph rect 17 mm outside the page box | 1 | rofd's fallback font advances every glyph by ≈ `0.96 × Size` mm, so the fixture's right column ran off the page; sized the text columns to `Size=3` (left column ≤ 93 mm, right column 120–195 mm) and measured with a throwaway rofd dump before asserting |
| 2026-10-15 | `test_render_selection` expected a 596 px surface, got 595 | 1 | Papers rounds the pixel size first (`pps_render_context_compute_scaled_size`) and the module derives the scale from that, exactly as §4 specifies; the test's oracle now computes the scale the same way |
| 2026-10-15 | `test-ofd-links` segfaulted in `g_list_model_get_item` and logged 4 `g_object_unref` criticals | 2 | Two test-side ownership bugs: a borrowed children model held in a `g_autoptr` (freed the store the outlines still pointed at), and `g_autoptr` on `pps_outlines_get_link`/`pps_link_action_get_dest`, which are `transfer none`. Both replaced with plain borrowed pointers; found with gdb backtraces |
| 2026-10-15 | `test-ofd-errors` asserted `n_pages == 0` after a failed load, got -1 | 1 | The base class keeps -1 until a load succeeds (`pps-document.c:155,446`); the assertion now checks `< 0` |
| 2026-10-15 | `test-ofd-search` segfaulted while dropping the `PpsSearchContext` (frames: `g_object_unref` → `pps_search_context_dispose` → `g_type_check_instance_is_fundamentally_a`) | 2 | Papers' dispose calls `g_clear_object ()` on a `GHashTable *` (`pps-search-context.c:421`) instead of `g_clear_pointer (..., g_hash_table_unref)`; `g_object_unref` then reads the hash table as a `GTypeInstance` and dies. Reviewed the field (`:433`) and the tag-51.1 source, bisected it to the dispose (the test passed once the context was no longer freed), and recorded it as A23; the test now leaks the context deliberately |
| 2026-10-15 | `test-ofd-search` logged `pps_document_get_page_label: assertion 'priv->cache_loaded == TRUE' failed` | 1 | The search context labels its pages from the document's page cache, which the shell fills in the load job; the test now calls `pps_document_setup_cache` after loading, like the shell does |
| 2026-10-15 | `test-ofd-search` logged three GLib-GObject criticals before any of its own code ran | 1 | `PpsDocumentModel::constructed` connects to its annotation model unconditionally, and `pps_document_model_new ()` passes none; upstream noise, swallowed by a scoped log handler with a comment (the test's own criticals still show) |
| 2026-10-15 | Building against `$ROFD_SRC` silently linked the *installed* library, so the font-advance fix looked ineffective | 3 | `rofd-ffi.pc` (installed by our AUR package) wins over `-Drofd_libdir` by design, and at run time the module's SONAME resolves to `/usr/lib`; a checkout build needs `PKG_CONFIG_LIBDIR` without `rofd-ffi.pc` plus `LD_LIBRARY_PATH="$ROFD_LIBDIR"`. Both traps are now in the plan playbook (§6) |
| 2026-10-15 | My first ink measurement compared a whole-row ink span (both columns) with one object's geometry and "proved" the renderer was wrong | 1 | Restricted the scan to the column under test (`INK_XMAX`), which showed the geometry, not the renderer, was at fault — the numbers for A24 come from the corrected measurement |
| 2026-10-15 | `test_render_selection` failed against the font-aware geometry (its sample point used the declared column box centre, now blank) | 1 | The test samples the *layout* centre of a glyph (new `glyph_centre_after` helper) for both the "covered" and the "cleared on reuse" pixels |


| 2026-10-14 | Oracle run failed with "File type Zip archive (application/zip) is not supported" on the real invoice | 1 | The `OFD.xml`@30 magic only matched archives whose first entry is `OFD.xml`; the invoice starts with `Doc_0/Annots/Annotations.xml` (`OFD.xml` sits at byte 21707), so GLib's data-only guess fell back to `application/zip`. Added a `Doc_0/`@30 alternative to `data/papers-ofd.xml` and verified with `g_content_type_guess(NULL, first 1024 bytes)` and `xdg-mime query filetype`; a plain zip still reports `application/zip` |
| 2026-10-14 | `ofd-latency` printed the same value for every phase | 1 | `milliseconds_since()` was called with the wrong base at print time; the tool now records a timestamp per phase and computes explicit deltas |
| 2026-10-14 | `ofd-render` target-size case failed (1132 px instead of 600) | 1 | rofd's options carry a single uniform scale, so a target with a different aspect ratio than the page cannot be reproduced (poppler scales the axes independently). Papers always derives target sizes from the page size (`pps-pixbuf-cache.c:1041-1044,1113-1118`), so the test now does too; the deviation is documented in plan §4 and the README limitations |
| 2026-10-14 | Headless GUI rendering via GTK broadway abandoned | 3 | `broadwayd` binds its TCP port but creates no unix socket in this environment and GTK4's broadway client needs it (`BROADWAY_DISPLAY=:N` → `$XDG_RUNTIME_DIR/broadwayN`); TCP display strings are not accepted. GNOME Shell's screenshot D-Bus API answers "Access denied", and no Xvfb/nested compositor is installed. GUI acceptance is therefore observed on the live session (log-level) plus the user's visual check |

| 2026-10-15 | `test-ofd-fd` aborted with `application/x-zerosize` for the MIME type it sniffed | 1 | The rejected-descriptor cases (pipe, directory, write-only) called the sniffing path, which legitimately reads nothing from them. Split the helper: real fixtures sniff the type (that is the previewer's step and is asserted), the rejection cases pass `application/ofd` explicitly |
| 2026-10-15 | `test-ofd-fd` segfaulted in `open_fixture_copy` from `main` | 1 | The helper dereferenced its optional `path_out` unconditionally; guarded it |
| 2026-10-15 | `curl` of the pinned source archive returned 404 although `git ls-remote` showed the commit | 1 | The GitHub repository was still **private** (`gh repo view` → `isPrivate: true`), so archives need auth and the AUR source would be unfetchable; the user chose to publish the repository, after which the anonymous download matched the checksum exactly |
| 2026-10-15 | `/tmp/pc-norofd` was not writable when re-creating the checkout-tier pkg-config directory | 1 | The directory was a leftover from the previous session with different contents; used a fresh `/tmp/pc-norofd3` instead of fighting it |

| 2026-10-15 | Recorded the phase-3 previewer evidence as "the invocation Nautilus uses" for `papers-previewer --fd 3` | 1 | Wrong: Nautilus/Sushi never pass a descriptor — the space bar is Sushi rendering in-process through Papers by URI, and `--fd` is the previewer's own contract (Papers' print preview passes paths). The user's acceptance run exposed it; the plan, README and this log now say what was measured (A26) |

## 5-Question Reboot Check

| Question | Answer |
|----------|--------|
| Where am I? | `docs/plan.md` phases 0–3 implemented: skeleton, rendering/printing/metadata, text/selection/find/links/outline, and now fd loading plus the Arch package (AUR `papers-ofd 0.1.0-1`, installed on this machine at 03:15). `meson test` is 12/12 in `_build`, `_build-slow` and `_build-fork`; the packaged module passes the previewer fd path, `papers-thumbnailer` and the MIME/desktop association checks |
| Where am I going? | Phase 3 is code-complete and shipped; what is left is the user's visual acceptance on the live session (Nautilus icon/thumbnail, space-bar preview window, double-click, single app-grid entry) and optionally the `pacman -Rns papers-ofd` uninstall regression. Then phase 4 (the upstream MR) is the only planned work left |
| What's the goal? | Ship the OFD backend for Papers 51.x, out-of-tree, on the rofd ABI we maintain (fork-first, AUR package) |
| What have I learned? | Phase 3: `/proc/self/fd/<n>` re-enters a file the caller already unlinked and always starts at offset 0, but it re-checks the file's permission — withdrawing read permission after the open is a deterministic trigger for the temp-copy fallback (A25). An AUR package cannot fetch from a private GitHub repository, so the repository had to become public before the source pin was publishable. Phase 16: rofd's semantic text geometry and its renderer disagreed for runs without explicit `DeltaX` (font size vs font advance) — a third rofd gap we now carry a patch for, and a reminder that rectangle-producing vfuncs must be validated against *pixels*. Phase 1: one uniform render scale (plan §4), rofd's rotation is clockwise and matches `rc->rotation` as-is, the invoice's first zip entry is `Doc_0/…`, and `setup_cache` fetches every page object. Phase 2: `PpsOutlines` has declared-but-unexported functions (A21), a nested MIME rule below `application/zip`'s priority silently loses content-only detection (A22), Papers refuses selection on rotated pages, rofd's fallback font advances ≈ `0.96 × Size` mm per glyph, and layout/find/selection rectangles must be re-based on the *target* page's box origin for GOTO destinations |
| What have I done? | Plan/README machine-portable and REUSE-compliant; rofd patches shipped via our AUR packages and proposed upstream (#11/#12/#13); phases 0–3 implemented and tested (12/12 in three build configurations), with the A21/A22/A24/A25 findings recorded; `packaging/PKGBUILD` published to the AUR as `papers-ofd 0.1.0-1` and installed here; the phase-3 visual acceptance is the user's call |

---

*Update this file after completing a phase, running validation, or encountering an error.*

---

## AUR publication and the carried-ABI branch (2026-10-15)

- The user asked for the AUR push: `librofd-ffi 0.4.3-4` is published —
  `afab2b7` on `ssh://aur@aur.archlinux.org/librofd-ffi.git` (`master`), the AUR RPC
  reports `0.4.3-4` (modified 02:26). Content: pinned upstream commit `140b936`,
  `rofd-ffi-page-size.patch`, `rofd-ffi-font-advances.patch`, `rofd-ffi.pc.in`; the
  packaged library was re-verified after a clean `makepkg -f` (exports
  `rofd_document_get_page_size_mm`, `on` match 3.65 mm).
- Our own repository is pushed to `YangtseSu/papers-ofd` (`7e5a573..0801cb4`).
- New fork branch `feat/aur-carried` (the user asked for it): `main` plus both patch
  files replayed with `git am`, i.e. exactly the tree the AUR package builds —
  verified with `diff -r` against the PKGBUILD build tree (identical apart from a
  `rofd.h.orig` backup that `patch` leaves in the build tree; the package installs
  `rofd.h` by path and is unaffected). Documented in `patches/README.md` and D11.

## Session close (2026-10-15)

- Delivered: phase 2 complete and user-accepted (text/selection/find/links/outline
  with the ink-alignment and tiling regression tests), the rofd text-geometry fix
  (`feat/font-text-advances`, PR #13, `patches/rofd-ffi-font-advances.patch`,
  installed as `librofd-ffi 0.4.3-4`), the MIME magic priority fix (A22), the
  headless Ctrl+F snippet test, and the plan/README/working-memory updates.
- Repository: clean tree, `meson test` 11/11 in `_build`, `_build-slow` and
  `_build-fork`; `reuse lint` compliant.
- Open for the user: nothing blocking; the AUR push of `librofd-ffi 0.4.3-4` stays
  deferred, and upstream reviews of rofd #11/#13 are opportunistic.

## Phase 3 — fd loading, packaging, install checks (2026-10-15, new session)

- **Status:** complete except the user's visual acceptance.

### fd loading

- `src/ofd-document.c` gained `load_fd`: `dup(fd)` (the base class hands the
  descriptor over), `rofd_document_open("/proc/self/fd/<dup>")`, then a temp copy
  below `g_get_user_cache_dir()/papers-ofd/ofd-fd-XXXXXX` when that path cannot be
  re-entered; the descriptor and the copy are dropped on reload and at finalize
  (`ofd_document_drop_source`). `load` was split into `load` (URI → path) and the
  shared `load_path`, so both entry points swap the document identically.
- `tests/test-ofd-fd.c` drives the previewer's sequence
  (`pps_file_get_mime_type_from_fd` → `pps_document_factory_get_document_for_fd` →
  `pps_document_load_fd`) and compares the result with the URI document in page
  count, page sizes, page text, rendered pixels and document info; it also covers
  the unlinked-file case, the temp-copy fallback with its lifetime, the three
  descriptor rejections of the base class and error parity with the URI path. The
  fallback is reached deterministically by withdrawing read permission after the
  open (A25) — no production code exists just for the test.

### Packaging

- `packaging/PKGBUILD`: Arch recipe, source = commit-pinned archive of this
  repository (`6936c96`, sha256 `64c1797d…b28853b`), `--buildtype=plain` so the
  makepkg flags apply, `check()` = the suite, `depends` = `librofd-ffi>=0.4.3`,
  `papers>=51`, `papers<52`, `cairo`, `gcc-libs`, `glib2`, `glibc`. The
  `shared-mime-info`/`desktop-file-utils` dependencies were dropped after
  `pactree -u papers` showed both arrive through `papers` and namcap called them
  unneeded; the hooks they own still index our files.
- The repository went **public** (the user's decision) so the pinned source is
  fetchable; the anonymous download then matched the checksum, and a `makepkg` run
  with an empty `SRCDIR` built and tested the package from that URL.
- Published to the AUR: `papers-ofd 0.1.0-1`, `master` = `86e4929`, mirror in
  `~/projects/papers-ofd-aur` (same working copy convention as `librofd-ffi`).
- `plan §7` now documents the Arch route (recipe, depends, hooks, verification),
  and `plan §6` gained the packaging/staged-install recipes plus the previewer's
  own invocation.

### Install and runtime checks

- The user installed the package (`pacman -U /tmp/ofd-makepkg/…-0.1.0-1-….pkg.tar.zst`,
  03:15:37) — no file conflicts, `Depends On` as designed, installed module md5
  identical to the built one.
- From `/usr` with no environment overrides: `xdg-mime query filetype` →
  `application/ofd` (the pacman hook indexed our xml), `xdg-mime query default
  application/ofd` → `org.gnome.Papers.Ofd.desktop`, `papers-thumbnailer -s 256`
  → 181×256, `papers-previewer --fd 3` → the document loads and renders with zero
  CRITICAL/ERROR, `papers <fixture>` likewise, `gio open` starts Papers with the
  file.
- Staged-tree checks (before the install): `desktop-file-validate` clean,
  `xdg-mime` and `gio mime` resolve as above, thumbnails 181×256 / 256×170, and the
  PDF regression thumbnail 198×256 from a backend directory that mixes the system
  backends with the package's module.

### Open for the user

- Visual acceptance on the live session (Nautilus is running): the OFD icon and a
  rendered thumbnail in Files, the space-bar preview window, a double-click, and
  the single Papers entry in the app grid.
- Optional: `sudo pacman -Rns papers-ofd` as the uninstall regression (the five
  paths are ours alone, so Papers and its other backends are unaffected).

## Phase 3 follow-up — the space bar (2026-10-15)

The user's acceptance run found the preview showing only file information, which
turned out to be a wrong premise in the plan rather than a module defect: the space
bar is Sushi, Sushi renders documents in-process through Papers' libraries and
loads them **by URI**, and it picks the renderer from the MIME types of
`org.gnome.Papers.desktop` — which does not list `application/ofd` out of tree.
`load_fd` (the phase's first step) is the previewer binary's own contract and is
exercised by nothing on this stack.

Fix, shipped in the package: `data/papers-ofd-sushi.js` is installed into Sushi's
system plugin directory (option `sushi_plugin_dir`). Sushi imports plugins before
it builds its renderer map, so adding the type to the document renderer's content
types makes the space bar render OFD files like PDFs, through PapersView.JobLoad
and therefore through this backend. `tests/test-sushi-plugin.js` drives Sushi's
own selector and renderer against the fixture (2 pages) and is skipped where gjs
or Sushi is missing; the plugin and the test go away once Papers lists the type in
its own desktop file (phase 4), which is also when our association entry can be
retired.

Before the package is rebuilt, the plugin was copied into
`~/.local/share/sushi/plugins-1/` so the user can press space immediately; the
copy is redundant once the package carries it and should be removed then.

## Phase 3 closed (2026-10-15)

The user installed `0.1.0-2` and confirmed the session items: space bar shows the
page (with the packaged Sushi plugin), Nautilus shows the OFD icon and a rendered
thumbnail, double-click opens Papers, and the app grid keeps a single Papers
entry. The temporary `~/.local/share/sushi/plugins-1/papers-ofd-sushi.js` is gone,
and the packaged
`/usr/lib/sushi/plugins-1/papers-ofd-sushi.js` alone reproduces the behaviour in a
headless probe (selector → document renderer, fixture loads with 2 pages).

Phase 3 is therefore accepted. Left in the plan, both optional: `pacman -Rns
papers-ofd` as the uninstall regression (the five package paths plus the plugin
are ours alone, so Papers and its other backends are unaffected) and phase 4, the
in-tree move that also lets our association entry and the Sushi plugin retire.

## Phase 17: the selection highlight missed the bottom of the text (2026-10-15)

- **Status:** fixed on the fork, packaged as `librofd-ffi 0.4.3-5` (built and
  installed locally, not pushed to the AUR per the earlier decision), verified
  against rendered pixels and on the user's real document. The GUI confirmation is
  the user's (the module suite and the pixel checks pass here).
- Reported by the user: after the horizontal fix, the *vertical* geometry is still
  wrong — "the highlight's top is complete, it does not cover the lower end of the
  text". Reproduced before touching code:
  - Static analysis of the user's real document (kept outside this repository,
    56 entries, six embedded subset
    fonts, sizes 4.9–7.8 mm) with fontTools: of 2464 glyphs with ink, **2117
    (86%) reach below the baseline** — CJK overshoot 0.098–0.129 em, Latin
    descenders up to 0.188 em — mean **0.50 mm**, max **1.46 mm** below the box
    bottom, while the box top sat 1.45 mm above the painted top on average.
  - Root cause: `crates/rofd-core/src/semantic.rs` built every character box as
    `top = y - size`, `bottom = y` (the font-size footprint), while the renderer
    paints the *glyph's own* extent around the baseline. The horizontal half of the
    same mismatch was fixed in phase 16; the vertical half was untouched.
- Fix (fork-first, D11): third commit on `feat/font-text-advances`,
  <https://github.com/linuxdeepin/rofd/pull/13>:
  - `rofd-core`: the provider seam becomes `TextMetricsProvider::metrics_mm`,
    answering `TextScalarMetrics { advance_mm, ascent_mm, descent_mm }` per scalar;
    the builder spans the baseline sweep with those extents. Without a provider, and
    for a scalar whose glyph no face owns or that paints nothing (a space), the
    conservative font-size footprint stays (size above the baseline, nothing below).
  - `rofd-render`: `ResolvedFont` caches the glyph's FreeType control box next to its
    advance (one `FT_Load_Glyph`, no extra face loads);
    `PositionedGlyph::metrics()` replaces `advance()` and exposes advance, ascent and
    descent; `FontTextMetrics` (was `FontTextAdvances`) owns its resolver in an `Arc`
    instead of borrowing it, which also removes the FFI's `Box::leak`.
  - `rofd-ffi`: loads with the metrics provider built from the default font stack
    (`LazyLock<Arc<SystemFontResolver>>`).
  - Verified on the rofd side: `cargo fmt --all -- --check`,
    `cargo clippy -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings`,
    `cargo test` for the three crates, `crates/rofd-ffi/tests/run_c_tests.sh` (rc=0,
    symbol list unchanged, 77 entries). Two new pixel-level tests fail with the
    builder change reverted and pass with it:
    `crates/rofd-render/tests/cairo_text.rs::semantic_character_boxes_cover_the_rendered_ink`
    (box bottom 80 px vs ink bottom 83 px before) and
    `crates/rofd-ffi/tests/semantic.rs::text_layout_rectangles_cover_the_rendered_ink_below_the_baseline`
    (box bottom 40 px vs baseline 40/ink 43 px before). rofd-render's pre-existing
    `decoded_limits_have_exact_and_one_over_boundaries` failure is unrelated (it
    fails with these changes stashed too).
- Module side:
  - `patches/rofd-ffi-font-advances.patch` regenerated with the third commit
    (`git format-patch 140b936..feat/font-text-advances`); `git apply --check` on a
    clean 140b936 worktree passes and a real `patch -Np1` reproduces the branch tree
    byte for byte (`diff -r` clean).
  - `tests/test-ofd-text.c`: the ABI classifier moved into
    `linked_abi_resolves_font_geometry` (used by the find test and the new one), and
    `test_selection_region_covers_rendered_ink` selects the second left-column line
    at 4× zoom and fails when the region does not cover the ink's bottom row:
    "selection region ends at row 453 while the glyphs paint down to 461: … Install
    librofd-ffi >= 0.4.3-5". `test_origin_normalisation` now asserts the box
    *straddles* the baseline (the fixture has no embedded font, so its ink extent is
    machine-dependent) instead of pinning the box top to the font-size footprint.
  - Docs: `docs/plan.md` (§2 geometry paragraph, §8 item 6, new evidence row A27, §6
    playbook note), `README.md` (ABI row → `0.4.3-5`, text-geometry caveat),
    `patches/README.md` row.
- Verification on this machine:
  - `meson test` 12/12 in `_build` and `_build-slow` with the packaged 0.4.3-5
    library (`LD_LIBRARY_PATH` on the extracted package), `reuse lint` clean.
  - The new test fails against the installed pre-fix `0.4.3-4` and passes against
    `0.4.3-5` — the same command, one environment variable apart.
  - Visual: a throwaway cairo harness rendered page 1 of the user's real document
    with the ABI's selection regions overlaid, against the packaged 0.4.3-5 library
    and the installed 0.4.3-4 one; before, the bottom strokes of 填写说明 and the
    punctuation hung outside the band, after they are inside it. The harness, the
    analysis script and the images stay in `/tmp` (the page is a user document, so
    nothing of it is committed).
  - Numerically on that page (title band 87.7–122.3 × 29.0–38.2 mm): the box went
    from y 28.01..35.77 (font-size footprint) to 29.22..36.89 against ink
    29.19..36.69 — the bottom now clears the ink by 0.20 mm instead of stopping
    0.92 mm above it, and the top moved from 1.18 mm above the painted top to
    0.03 mm. The body line's band went from y 43.80..49.45 to 44.68..50.13 against
    ink 44.71..50.01 (the CJK overshoot below the baseline is what the old box cut).
- The fork's `feat/aur-carried` convenience branch was regenerated from the
  current patch files (`git switch -C feat/aur-carried fork/main && git am
  patches/rofd-ffi-page-size.patch patches/rofd-ffi-font-advances.patch`, four
  commits, `74d32fb`), rebuilt (`cargo build --release --offline -p rofd-ffi`), its
  exported set checked against `crates/rofd-ffi/tests/c/expected-symbols.txt`
  (unchanged, 77 `rofd_*` symbols including the page-size query), and the module
  suite run against that library: 12/12 — the checkout build of the carried ABI and
  the packaged one behave identically on `_build`. Pushed to
  `YangtseSu/rofd feat/aur-carried` (forced update, it is a regenerated convenience
  branch).
- Open: the local package `librofd-ffi-0.4.3-6-x86_64.pkg.tar.zst` is built in
  `~/projects/ofd-reader/librofd-ffi` (**supersedes 0.4.3-5**); installing it needs root
  (`pacman -U`), and the AUR push stays deferred with PR #13 as before.

## Phase 17 follow-up: the highlight band is the font's metrics, like the PDF backend (2026-10-15)

- **Status:** supersedes the "painted control box" model of the phase-17 entry above;
  built and verified as `librofd-ffi 0.4.3-6` (local package, AUR push still deferred).
- Reported by the user after 0.4.3-5: in Papers' PDF backend a selection highlight is
  one band per line that sticks out above and below the text and lines up across Latin,
  CJK and punctuation, while the OFD highlight "only covers the character's height" —
  per-glyph ink boxes make the band ragged and flush with the glyphs. Their reading of
  the earlier state was right too: the font-size footprint was uniform, it just stopped
  at the baseline.
- Ground truth from poppler, which Papers' PDF backend consumes
  (`libdocument/backend/pdf/pps-poppler.c:1889,2010`: `poppler_page_render_transparent_selection`
  and `poppler_page_get_selected_region`), fetched from poppler master on 2026-10-15:
  - `poppler/TextOutputDev.cc` `TextWord::setInitialBounds` (lines ~501-505):
    `yMin = y - ascent; yMax = y - descent`, where ascent/descent are the *font's*
    (`TextFontInfo::getAscent/Descent` → `GfxFont`, lines 403-410), so every glyph of one
    font at one size shares the band, regardless of its own ink.
  - `TextSelectionSizer::visitLine` (lines ~4387-4400): the region rect is
    `y1 = line->yMin - margin; y2 = line->yMax + margin` with
    `margin = (line->yMax - line->yMin) / 8` — the visible "sticks out" of a PDF
    highlight. Character boxes and find rectangles keep the plain band (find rects have
    no margin).
  - Measured on a real-world PDF kept outside this repository:
    a body CJK word box 15.87 pt tall for a 15.95 pt em (0.995 em = the font metrics),
    its line's selection region 20 pt (528..548, i.e. ±2 pt ≈ height/8), its find
    rectangle for the same text 15.87 pt.
- Fix (fork-first, D11, third commit of `feat/font-text-advances` amended,
  <https://github.com/linuxdeepin/rofd/pull/13>):
  - `rofd-render`: `ResolvedFont` reads `ascender`/`descender` from FreeType while it
    validates the face and `typographic_extents_mm` scales them; every glyph of a face
    reports the same `ascent`/`descent` through `GlyphMetrics` (the per-glyph control box
    of 0.4.3-5 is gone). A face with no usable metrics keeps the font-size footprint.
  - `rofd-core`: the builder is unchanged in shape (it already spanned the baseline with
    the resolved extents); `selection_region` now grows each per-line union by an eighth
    of its height above and below, with the poppler citation in the doc comment. Match
    rectangles and character boxes keep the plain band.
- Module side (this repository):
  - `tests/test-ofd-text.c::test_selection_region_covers_rendered_ink` now asserts the
    full model on the fixture at 4× zoom: the region covers the ink's bottom row, sticks
    out above and below it, and a paragraph selection returns one region per line with
    *one* height (3 regions, 62 px each). Both new assertions fail against 0.4.3-5 and
    0.4.3-4 ("selection region rows 420..461 do not clear the ink rows 426..461 … Install
    librofd-ffi >= 0.4.3-6").
  - `test_origin_normalisation` no longer bounds the box top by the font size (a face may
    declare a larger ascent: the fixture's fallback, Noto Sans CJK SC, rises 1.16 em) and
    asserts the band straddles the baseline instead.
  - Docs: A27 rewritten with the poppler citations and the measurements, the §2 geometry
    paragraph and the §6 playbook note, `README.md` (ABI row → `0.4.3-6`).
- Verification:
  - rofd: `cargo fmt --all -- --check`, `cargo clippy -p rofd-core -p rofd-render
    -p rofd-ffi --all-targets -- -D warnings`, `cargo test` for the three crates (the two
    pixel-level tests and the core selection tests encode the new model and fail against
    the old one), `crates/rofd-ffi/tests/run_c_tests.sh` (rc=0, 77 symbols).
  - Patch series replayed onto a clean `140b936` checkout with `patch -Np1`; the result is
    byte-identical to the branch tree (`diff -r`).
  - Module suite 12/12 in `_build` against the packaged 0.4.3-6 (`LD_LIBRARY_PATH` on the
    extracted package), `reuse lint` 57/57.
  - Visual: the throwaway cairo overlay on page 1 of the user's real document (0.4.3-5
    above, 0.4.3-6 below) shows tight per-glyph bands becoming one band per line that
    clears the glyphs. Numerically on that page: the four objects of the second body line
    all report region y 43.89..50.94 mm for ink 44.71..50.01 (top 0.82 mm above the ink,
    bottom 0.93 mm below), and the title band 27.90..39.27 for ink 29.19..36.69.

## Publications: librofd-ffi 0.4.3-6 and papers-ofd 0.1.0-4 (2026-10-15)

- The user asked for the deferred AUR push after the highlight-band work, so the
  earlier "keep it local until PR #13 is reviewed" decision is overridden.
- `librofd-ffi 0.4.3-6` (the metrics band + eighth-height selection margin):
  - `git push origin master` in `~/projects/ofd-reader/librofd-ffi`:
    `afab2b7..1b11bd5  master -> master`; the authoritative git protocol returns
    `1b11bd5df6d4cbf48c2a5e88578692958211ad22` for `refs/heads/master`, and
    `git show origin/master:{PKGBUILD,.SRCINFO}` on the fetched ref shows `pkgrel=6`
    with the new patch hash `0c7fe14c4659ec56292475964adbf748d02f80318dc332db7314424e4a5b4bcb`.
  - The cgit/RPC frontends still reported `0.4.3-4` right after the push (the same
    index lag every earlier publication showed); the manifest at
    `https://aur.archlinux.org/cgit/aur.git/plain/PKGBUILD?h=librofd-ffi` is what
    catches up.
  - The user installed the package on this machine: `pacman -Q librofd-ffi` reports
    `0.4.3-6`, and `/usr/lib/librofd_ffi.so.0` is byte-identical to the `.so` inside
    `librofd-ffi-0.4.3-6-x86_64.pkg.tar.zst` (sha256
    `614da176539fb8ed97a2db079ff6a23db8f97129279e1016b5538cb0b377e1bd`), i.e. the live
    session now runs the verified library.
- `papers-ofd 0.1.0-4` (module package, pin `ffca9aa`, this repository's HEAD):
  - Why a release: the published `0.1.0-3` pinned `c1f0565`, whose
    `test_origin_normalisation` bounds the box top by the font size; against
    `librofd-ffi 0.4.3-6` that assertion fails (a face may declare a larger ascent —
    the fixture's Noto fallback rises 1.16 em), so a rebuild of the older pin would
    fail `check()`. The module binary is unaffected; the package now also declares
    `makedepends=('librofd-ffi>=0.4.3-6')`.
  - `makepkg -f` in a scratch directory (`/tmp/po-build`) with the repository tarball
    of `ffca9aaaeea0c7401982a61a36ad1c074d03c410`
    (sha256 `41ada59a41d864925a3819092375d2e14ffc46e1da948cacfe8305590edd1556`) built
    `papers-ofd-0.1.0-4-x86_64.pkg.tar.zst` and ran `check()`: 12/12 against the
    installed 0.4.3-6.
  - Pushed from `~/projects/ofd-reader/papers-ofd-aur/papers-ofd` after switching the
    remote to `ssh://aur@aur.archlinux.org/papers-ofd.git` (the https remote answered
    403): `9e0e4ab..e9af873  master -> master`, confirmed with `git ls-remote` and by
    fetching the ref back (`git show origin/master:.SRCINFO` → `pkgrel = 4`,
    `makedepends = librofd-ffi>=0.4.3-6`). The first attempt was rejected by the AUR hook
    — "the repository must not contain subdirectories" — because `git add -A` had swept
    in the previous build's `pkg/` and `src/` output; the artifacts are removed and the
    commit carries only `PKGBUILD` and `.SRCINFO`. Both packages' RPC entries still
    reported the previous versions right after the pushes (the known index lag).

## Sushi test hermeticity (2026-10-15)

Rebuilding the package (`makepkg -f` in the AUR tree) failed in
`ofd-sushi-plugin`: the selector assertions passed, then the renderer reported the
factory's unsupported-type error — `File type Zip archive (application/zip) is not
supported`, `libdocument/pps-document-factory.c:86`, rendered in zh_CN — and
"-2 pages". The test was the only one that left its environment to the caller — it
drives Sushi's renderer into Papers' *own* load path, without setting
`PPS_BACKENDS_DIR` or a MIME rule for `application/ofd` — and a package build has
neither: the module is not installed yet, and the type is known only where some
package defines it.

Papers reports the type its **second** pass guessed: `pps_document_factory_get_document`
runs `fast=TRUE` and, after any failure, `fast=FALSE`
(`libdocument/pps-document-factory.c:281-282`), the passes being filename-only and
data-only (`libdocument/pps-file-helpers.c:401`, `:344`, the latter
`g_content_type_guess (NULL, …)` at `:369`). Without `PPS_BACKENDS_DIR`
(`pps-document-factory.c:230-231`) the first pass finds no backend for
`application/ofd`, and the second, with only zip bytes to look at, ends at the
unsupported-type error (`pps-document-factory.c:81-86`). Whether the MIME database
knows the type at all is the other half: with no package defining it, GIO resolves
the fixture to `application/zip` before Papers is involved, and Sushi's renderer
does not even start the load.

Reproduced before the fix: `env -u PPS_BACKENDS_DIR gjs -m tests/test-sushi-plugin.js …`
in `_build` printed the same two FAIL lines as the build log.

Fix (`97b89d7`): the test takes the module, the descriptor and
`data/papers-ofd.xml` through Meson (`tests/meson.build:190-196`) and stages
backend directory and MIME database itself (`tests/test-sushi-plugin.js:49-119`):
a temporary `PPS_BACKENDS_DIR` holding `libofddocument.so` and the descriptor, and
a temporary `XDG_DATA_DIRS` entry that `update-mime-database` indexes, both before
Sushi's renderer initializes Papers in its constructor. The tree is removed on
success. This is the staging the C tests always did (`tests/ofd-test-utils.c`),
so the test no longer depends on an installed package, a prepared shell, or a
MIME rule from another package.

Verification:
- `meson test` with `PPS_BACKENDS_DIR` unset: 12/12 in `_build` and `_build-slow`
  (`-Drofd_page_size=disabled`), `ofd-sushi-plugin` printing both acceptance
  lines.
- Chroot-like environment (user namespace, a copy of the system MIME tree with no
  `application/ofd` rule bind-mounted over `/usr/share/mime`, empty
  `XDG_DATA_HOME`, no `PPS_BACKENDS_DIR`): the fixed test passes; the pre-fix test
  fails there too.
- `makepkg -f` on a tarball of this tree: `check()` runs 12/12 including
  `ofd-sushi-plugin` and the package is built — the path that failed.
- `reuse lint`: 57/57.

Published as `papers-ofd 0.1.0-3`: the pin in `packaging/PKGBUILD` moved to
`c1f0565` (the test fix) with the repository tarball's sha256, AUR commit `9e0e4ab`
(`git ls-remote` confirms it on the server, and `makepkg -f` in the AUR clone
built 0.1.0-3 with 12/12 tests). The installed file list is unchanged, so a
rebuild is a no-op for the module, the plugin and the MIME xml; the AUR RPC still
reported 0.1.0-2 a minute after the push, the same frontend lag the earlier
publications showed. The index caught up the same day: the user confirmed
0.1.0-3 on the AUR side (the git repository, which is what `makepkg` clones, had
it from the push).

## Annotation editing research and deferral (2026-10-15)

The user asked whether Papers' edit button (annotation tooling) could work for OFD —
does `rofd` have it, or is it only an unexported ABI, and how hard would it be. Answer,
all three layers checked:

- **`linuxdeepin/rofd` has neither half in the ABI.** `rofd-core` parses annotation
  appearances and draws them (`crates/rofd-core/src/content.rs:192-257`,
  `crates/rofd-render/src/display_list.rs:263`) but has no writer (`ZipWriter` appears
  only inside `#[cfg(test)]`, `crates/rofd-core/src/container.rs:139-188`), and none of
  the 78 exported C symbols touches annotations or saving
  (`crates/rofd-ffi/tests/c/expected-symbols.txt`); the header's only annotation name is
  the `ROFD_WARNING_ANNOTATION_SKIPPED` constant. Upstream documents editing, form
  filling and seal creation as a separate sub-project
  (`docs/superpowers/specs/2026-09-03-rofd-library-reader-design.md:256`), and
  deepin-reader's own OFD backend refuses to save for that reason
  (`reader/document/OfdModel.cpp:152-168` in the `deepin-reader` checkout).
- **Papers' contract** (tag `51.1`): the whole toolset is gated on
  `PpsDocumentAnnotations` plus a non-NULL `add_annotation`
  (`shell/src/document_view.rs:486-516`, `libdocument/pps-document-annotations.c:53-58`);
  `get_annotations` (called for every page at setup, `transfer none`) and
  `PpsDocument::save` are dispatched without NULL checks; persistence is save-as only
  through `libview/pps-jobs.c:1105-1183`; committed annotations are shown by the
  document render (`libview/pps-pixbuf-cache.c:616-628` plus the re-render on add,
  `libview/pps-view.c:2885-2898`), i.e. in-memory annotations must appear in `render`.
- **OFD side**: clause 15 and annex A.3/A.4 were read from the scanned standard in
  `$ROFD_SRC/learning/` (no text layer — clause pages were rendered and read as images).
  The entry file needs `PageID`+`FileLoc`, `Annot` needs `ID`/`Type`/`Creator`/
  `LastModDate` and a required `Appearance`, and `Type` is restricted to the five values
  `Link`/`Path`/`Highlight`/`Stamp`/`Watermark`, so Papers' variants have to ride on
  `Subtype`.
- **`office-rs/rofd`** (the same-named sibling; github.com/office-rs/rofd, v0.1.6,
  Apache-2.0) is the only checkout with an annotation writer, and its engine crates
  (`rofd-dom`/`rofd-io`/`rofd-editor`) are UI-free and on crates.io — so it was surveyed
  in full as a craft reference (mapping, three-level geometry, id minting with
  `MaxUnitID` write-back, surgical save, drawing constants) and rejected as the engine:
  it rasterizes through vello/wgpu with no cairo or pixel-export path, has no C ABI, and
  declares full-fidelity rendering (template inheritance/JBIG2/tiles) plus find/links/
  outline/print out of scope.

Outcome (user's call): annotation editing stays deferred; a future phase writes the OFD
side against the standard inside the fork rather than depending on `office-rs/rofd`.
Recorded as plan D12 + plan §10, with the full evidence in `findings.md`
§"Annotation editing" (N1-N29), including the pitfalls not to copy (an `<Appearance>`
per quad pair, `Type="FreeText"`, quote-unaware escaping, and the rest).

Housekeeping from the same session: the three places that called `office-rs/rofd` "an
unrelated project of the same name" now say what it is — a different OFD project of the
same name (`AGENTS.md:29-32`, `scripts/dev-env.sh:23-25`, `findings.md` P31,
`task_plan.md` line 253, plus a correction note in this log's Phase 8 entry).

Verification: no code changed; `bash -n scripts/dev-env.sh` clean and a bare run still
reports the same resolution table (the edit is inside the header comment); `reuse lint`
57/57.

### Phase 18: one home per artefact (2026-10-15)

- **Status:** complete
- Actions taken (user's decision: the in-repo `packaging/PKGBUILD` duplicated the AUR
  recipe for no benefit, and the carried rofd patches existed in three places — this
  repository, the fork branches, the AUR `librofd-ffi` repository):
  - Removed `packaging/PKGBUILD` — verified byte-identical to
    `~/projects/ofd-reader/papers-ofd-aur/PKGBUILD` with `diff -u` before deletion.
    The recipe is maintained in the AUR repository only; the AUR RPC reported
    `papers-ofd 0.1.0-4` and `librofd-ffi 0.4.3-6` at the time of this change.
  - Removed `patches/rofd-ffi-page-size.patch` and
    `patches/rofd-ffi-font-advances.patch` — `sha256sum` identical to the copies the
    AUR package applies (`812bebb544844164d0004f9854fde1714aa9da97b4fa9ff8d0fa6a8eea96564d`
    and `0c7fe14c4659ec56292475964adbf748d02f80318dc332db7314424e4a5b4bcb`) — and
    `LICENSES/LGPL-2.1-or-later.txt`, which `reuse lint` had just flagged as unused
    (no LGPL file remains in the tree; `REUSE.toml` records what adding one back
    needs).
  - `patches/README.md` rewritten as the inventory (fork branch + commit, upstream PR,
    AUR patch file, retirement criterion, `feat/aur-carried` regeneration from the AUR
    patch files, sync routine). No payload lives here any more.
  - Retargeted: AGENTS.md (layout row, fork-first rule, new rule that recipes and
    payloads have one home, licensing note), README.md (status, ABI and sources rows,
    install section, both limitations — `papers-ofd 0.1.0-4` / `librofd-ffi 0.4.3-6`),
    docs/plan.md (D11 both cells, phase-3 step 2 and package evidence, §6 package
    recipe now `git clone https://aur.archlinux.org/papers-ofd.git`, §7 Arch-package
    section and source-pin paragraph, §8 patch risk row, §9 policy and items 1/5/6,
    A24/A27 pointers), `scripts/dev-env.sh` (header and missing-checkout note),
    `tests/test-ofd-text.c` (four install hints).
- Verification: `reuse lint` compliant (54/54 files); `meson compile -C _build` +
  `meson test -C _build` 12/12 after the test-string edits; a repository-wide
  `git grep` for `packaging/` and the payload paths finds only this log's history and
  the new AUR-file references in `patches/README.md` / `docs/plan.md`.
- Not touched: the AUR repositories (no payload or pin changed — this is a
  repository-hygiene change), the fork branches, and the code itself.

### Phase 19: privacy incident — private names scrubbed, history rewritten (2026-10-15)

- **Status:** complete
- Problem (user-reported): the evidence trail in `docs/plan.md` A27 and in this log
  named the user's local documents (a real invoice and a reference PDF) and one
  absolute home path; a tracked `tests/__pycache__/` artifact embedded machine paths.
  The repository is public, so the names were exposed in the working tree *and* in
  every historical revision that carried them.
- Actions taken:
  - Named material replaced by role descriptions in `docs/plan.md` and this log
    ("a real-world invoice kept outside this repository", "a real-world PDF kept
    outside this repository"); home paths written as `~`; the cache removed and
    `__pycache__/` added to `.gitignore`; AGENTS.md gained the hard rule that nothing
    identifying the user or the machine may enter the repository or the working
    memory (which is tracked and public too).
  - History rewritten with `git filter-repo` (50 commits): `--replace-text` and
    `--replace-message` over a literal mapping (document names → role descriptions,
    home prefix → `~`) plus `--path tests/__pycache__/ --invert-paths`. Every commit
    SHA changed; the AUR `papers-ofd` pin is bumped to the rewritten HEAD.
  - Verification: `git rev-list --all | xargs git grep -F -l` returns zero hits for
    every removed literal, in blobs and in commit messages; no `__pycache__` path
    exists in any revision; `git fsck --strict` clean; `git count-objects` reports no
    loose objects left after the rewrite's repack.
  - Fork audit (`YangtseSu/rofd`, the user's request): clean — no document names,
    home paths or machine paths in any of its six refs, in any commit message, or in
    the upstream PR text/comments of #11/#12/#13.
- Pre-rewrite history: a `git bundle` was kept in `/tmp` while the rewrite was
  verified and **deleted** afterwards, together with the replacement-pattern file and
  the downloaded tarballs; the rewritten history is the only copy.
