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

### Phase 20: upstream CI — what fails, what we fixed, the fork's own checks (2026-10-15)

- **Status:** complete — every fork-side check is green (run 4/5) and the record below is
  the whole picture; the only remaining blockers are upstream's (CLA App, no test job)
- What actually fails on the rofd PRs (`gh pr checks`, #11, #12, #13, #14, #15):
  - `clacheck / CLAssistant` fails on every one of them with
    `Could not update the JSON file: Error occured when creating a pull request comment: Resource not accessible by integration`
    — `linuxdeepin/.github/.github/workflows/cla-check.yml@master` calls
    `linuxdeepin/contributor-assistant-github-action` with their GitHub App
    credentials, and that installation can neither comment nor write
    `linuxdeepin/cla`. The job fails before any signature is recorded, so it is an
    upstream-side defect; the contributor side is the signature comment
    (`I have read the CLA Document and I hereby sign the CLA.`), and the registry
    (`signatures/version1/cla.json`, 192 entries) has **no entry** for the account.
  - The `check_job / archlinux-*`, `debian-*` and `deepin-*` entries are **skipped by
    design**: in `linuxdeepin/.github/.github/workflows/build-distribution.yml@master`
    the `archlinux-check`, `debian-setup` and `deepin-setup` jobs carry `if: false`,
    and every build/upload job `needs:` one of them. No `/ok-to-test` gate is
    involved; the robot's boilerplate comment is generic. So the Distribution Builder
    never builds or tests anything.
  - Everything else passes on all five PRs: commitlint, cppcheck,
    license-check (REUSE), Sourcery.
- What those jobs *would* run, reproduced locally per branch (worktrees of
  `feat/ffi-page-size-query`, `feat/pkgconfig`, `feat/font-text-advances` plus
  `fork/main` as the baseline): `cargo fmt --all -- --check` clean, clippy
  `-D warnings` clean, the `debian/rules` command
  (`cargo build --release --offline --locked -p rofd-ffi -p rofd --features
  rofd/qt-reader`) passes, the C ABI runner (`run_c_tests.sh`) exits 0,
  `reuse lint` fails on every revision with the upstream `REUSE.toml` parse error.
- Two real defects found and fixed on fork branches, proposed upstream:
  - **Decoder allocation budget** — `decoded_limits_have_exact_and_one_over_boundaries`
    fails on `main` (`Err(ImageDecode … "Memory limit exceeded")`): `images.rs` sets the
    image crate's `Limits::max_alloc` to the finished RGBA8 size while that budget covers
    the decoder's intermediate buffers too. `fix/decoded-image-alloc-budget` allows three
    times the finished size; `cargo test -p rofd-render --test images` goes 13/14 → 14/14,
    throwaway probes confirm the exact-boundary decode for 16-bit RGBA and full-palette
    PNGs. PR <https://github.com/linuxdeepin/rofd/pull/14>.
  - **Region pixel-diff tolerance** — `real_fixture_region_matches_full_page_and_preserves_cairo_ownership`
    bounds an anti-aliasing difference with absolute numbers tuned to Arch (21 channels,
    delta ≤ 2) that ubuntu-24.04 legitimately exceeds (42 channels, delta ≤ 8), so the
    suite cannot pass on a clean Debian build-dependency environment.
    `fix/region-aa-tolerance` bounds the share of differing channels (1 %) and the
    per-channel delta (16) instead. PR <https://github.com/linuxdeepin/rofd/pull/15>.
- Fork CI (upstream's is inert, so this is the only CI these branches can have):
  - Actions enabled on `YangtseSu/rofd` and the ten inherited workflows disabled — in a
    fork they would fire on push and fail on secrets that forks never inherit
    (`backup to gitlab`, `Call build-distribution`, `deb`, …).
  - `.github/workflows/fork-checks.yml` on the fork-only branch `ci/fork-checks`:
    collects every `feat/*` and `fix/*` branch (or an explicit ref list on dispatch) and
    runs rustfmt, clippy, the crate tests, the exact `debian/rules` build command and the
    C ABI runner per branch. It lives on its own branch so the feature branches stay a
    clean diff against upstream. First run (38684495268) found the region failure in all
    five jobs — the reason PR #15 exists; the workflow then split that one test into a
    visible, non-gating step with the reason inline.
  - Run 3 (38685216400) then failed in **all six jobs** on the third gating test:
    `rofd-render --test real_fixture::repository_invoice_fixture_lowers_and_renders_text_images_and_paths`
    — `real_fixture.rs:98` asserts that lowering the repository's real invoice fixture
    produces **no `MissingGlyph` diagnostics**. The fixture's text names 宋体 / 楷体 /
    Courier New (one occurrence each) and the test's resolver supplies `Noto Sans CJK SC`
    and `Noto Sans Mono` as fallbacks, so CJK coverage comes from those families. The
    ubuntu-24.04 runner has **no CJK font installed** (the workflow installs only
    `debian/control`'s build dependencies: cairo, freetype, jbig2dec, Qt), so those
    glyphs resolve to nothing and the assertion fails. Locally both families resolve
    (`fc-match` → `NotoSansCJK-Regular.ttc` / `NotoSansMono-Regular.ttf`, 900 Noto faces)
    and the test passes, which is why this appears only in CI.
    **Fix applied** (commit `ebb7118` on `ci/fork-checks`): `fonts-noto-cjk` and
    `fonts-noto-core` joined the dependency step. Run 4 (38686177356) came back
    **green for all six branch jobs** — gating rustfmt, clippy, the crate tests (minus the
    two splits), the `debian/rules` release build (1m03s on the runner, the step's first
    ever execution there) and the C ABI runner; the two upstream-red tests still run as
    visible, non-gating steps. Run 5 re-confirms the same tree after the commit-message
    fixup. Note that upstream's own suite would fail the `MissingGlyph` assertion the same
    way on any font-less environment — their Distribution Builder never runs tests, so
    nobody has seen it there.

### Phase 21: the decode-budget fix completed and shipped through the AUR (2026-10-15)

- **Status:** complete
- The user asked for two things: make the fix complete, and let AUR users have it now.
- Completeness: the second decode site was the real hole. `rofd-render` also decodes
  stamp pictures (electronic seals) in `cairo_renderer.rs::decode_seal_raster`, with the
  image crate's *default* `Limits`; the budget there is entropy dependent (the crate
  charges each allocation against `max_alloc`), which is why my first estimate of the
  failing range was wrong. Measured with the real `ImageReader` on flat RGBA8 PNGs and on
  16-bit RGBA PNGs:
  - flat RGBA8: 8192² (256 MiB) decodes, 8704² (289 MiB) fails at the old budget but is
    also outside the 256 MiB raster budget, so only the error *kind* changed;
  - 16-bit RGBA (decoder holds 8 bytes/px, conversion makes 4 more): **6000² (137 MiB
    raster, 274 MiB decoder memory) failed at the old budget although it is inside both
    documented limits** — i.e. a legal seal was silently dropped — and 14336² is still
    rejected after the change.
  `decode_pixels` and `decode_seal_raster` now budget the decoder with three times the
  finished raster and keep the raster checks as the authority (no limit relaxed).
- Regression test: `crates/rofd-render/tests/cairo_stamp.rs::a_large_seal_raster_within_the_document_limits_is_not_called_over_budget`
  with a generated 6000×6000 16-bit RGBA seal (420 KiB, ~11 s). **Fails before the
  change** (the stamp pixel stays page-white, i.e. the seal was dropped) and passes
  after; `cargo test -p rofd-render --test images` is 14/14 and the crate suite is green;
  `cargo fmt --all -- --check` and `cargo clippy -p rofd-render --all-targets --
  -D warnings` clean. Fixture provenance (generation script + sha256) recorded in
  `crates/rofd-render/tests/fixtures/images/PROVENANCE.md`.
- Fork/PR: branch `fix/decoded-image-alloc-budget` at `2983cf6` (force-push, the earlier
  single-site commit was amended); PR #14 retitled and its body rewritten with both sites,
  the probe table and the new test.
- Delivery: `librofd-ffi` `pkgrel` 7 carries it as `rofd-ffi-image-decode-budget.patch`.
  AUR's server-side hook rejects blobs above 488 KiB, so the regression fixture cannot ride
  in the commit: it ships as a checked `source` (`seal-16bit-boundary.png`, sha256
  `264bfb51…`) and `prepare()` installs it after applying the patch. First push attempt was
  rejected because `git add -A` had swept the build tree (`src/`, `pkg/`, the tarballs,
  ~70 MB) into the commit — reset and re-committed with only the four recipe files; the
  build tree is now in `.git/info/exclude`.
- Verification before publishing: `makepkg -C -f` from a wiped `src/`/`pkg/` succeeds;
  the packaged library exports the same 78 symbols (incl. `rofd_document_get_page_size_mm`);
  the patched tree contains `saturating_mul(3)` at both sites; the papers-ofd suite is
  12/12 against the packaged library (`LD_LIBRARY_PATH` to an extracted copy, confirmed
  with `ldd`). AUR `master` is `f97ed5d`; the RPC index lags as usual.
- Documentation: `patches/README.md` gained the carried row, its retirement criterion and a
  renamed section that now states which upstream proposals are carried and which are not;
  `docs/plan.md` §9 item 7 says what ships and what the package needs.

## 2026-10-15: dependency switched to fork releases (no more AUR patches)

The user decided the AUR + patch delivery was unmaintainable: the fork now
releases its own tarballs and the AUR package builds them directly.

- Fork `YangtseSu/rofd`: new branch `release/0.4.3-fork.1` = `feat/aur-carried`
  + cherry-picks `2983cf6` (decode budget) and `714d589` (region-AA test
  tolerance) + merge of `ci/fork-checks` (the workflow is now on the release
  branch; the separate `ci/fork-checks` branch stays until the next release
  rebase trims the two upstream-red splits, both of which are green here).
  Version fields (`Cargo.toml` workspace + the three crates +
  `crates/rofd-core/tests/public_api.rs`) moved to `0.4.3-fork.1`; the
  previously pinned version test was updated with the move. Tag `v0.4.3-fork.1`
  at `baf6ab6`; release published with the `git archive` tarball
  `rofd-0.4.3-fork.1.tar.gz` (69 MB, includes `vendor/`, sha256
  `1ac7525a…a3efd4673` — note the GitHub tag-archive tarball, used by the AUR,
  differs in gzip stream only and hashes `b19a464e…9f05681dd`).
- Checks on the release branch: `cargo fmt --check` clean, clippy `-D warnings`
  clean, full three-crate suite green (the two upstream-red tests
  `decoded_limits_have_exact_and_one_over_boundaries` and the region
  `real_fixture…` included), `run_c_tests.sh` exit 0, distribution
  build `-p rofd --features rofd/qt-reader` green.
- AUR `librofd-ffi 0.4.3.fork.1-1`: `source` = the fork's tag tarball URL
  (sha256 pinned), three patch files and `rofd-ffi.pc.in` deleted,
  `prepare()` emptied, `package()` installs `rofd-ffi.pc` regenerated from the
  tarball's `debian/rofd-ffi.pc.in` with `@MULTIARCH@` replaced by `/lib`.
  Verified: `makepkg -C -f` green; packaged library exports the 78
  `rofd_*` symbols matching `expected-symbols.txt` (page-size query included),
  SONAME `librofd_ffi.so.0`; papers-ofd configure + compile + `meson test`
  12/12 against an extracted copy of the package. SrcINFO regenerated (the AUR
  hook rejects a push whose SRCINFO still lists deleted source files — first
  push was declined for exactly that). AUR `master` is `b8cd1e4`; pushed on the
  user's explicit go-ahead. The tarball source means AUR users now get the fork
  release identically.
- This repository: `patches/README.md` rewritten as the release inventory
  (release-model section, current-release table, per-change rows with new
  retirement semantics, release/update routine); `docs/plan.md` D11 rewritten,
  §9 policy paragraph and the delivery-status lines of items 1, 5, 6, 7, 8 and
  the A24/A27 evidence rows reworded to the release delivery, §8 risk rows
  updated; `README.md` requirements row and the two limitation mentions
  updated. `AGENTS.md` layout row, fork-first rule and the licensing note now
  describe the release model. `scripts/dev-env.sh` unchanged (its marker logic
  never referenced versions; the resolution still prefers pkg-config +
  `rofd-ffi.pc`, which the new pc file satisfies with version
  `0.4.3.fork.1` ≥ the meson `>= 0.4.2` bound).
- Still true: upstream PRs #11–#15 remain open as the exit path; the fork's
  `main` keeps mirroring upstream, so the release branch alone carries our work
  and a future rebase against upstream is a plain cherry-pick replay.

## 2026-10-15 (later): fork CI fixed, branch model documented

The workflow merged onto the release branch still had `on.push.branches:
[ci/fork-checks]`, so the branch that ships had **zero** CI runs
(`gh run list --branch release/0.4.3-fork.1` was empty) and its ref collector
only knew `feat|fix`. Fixed in `71cb816` on `release/0.4.3-fork.1`:

- `push` → `release/*`, and a release ref is **strict**: no `--skip`, both
  upstream-red tests gate like everything else (a release carries the fixes, so
  this is what must be true of what ships).
- `workflow_dispatch` (now actually reachable from a `release/*` ref) covers
  `feat/*` / `fix/*` in **loose** mode: the two upstream-red tests run as
  visible, non-gating steps. The matrix entry carries `{ref, strict}` computed
  with `startswith("release/")`, so one workflow serves both.
- Observed: run 38719087584 (push) success with `crate tests` gating and both
  upstream-red steps skipped; run 38719477550 (dispatch of
  `refs/heads/feat/ffi-page-size-query`) success with the two failing tests
  surfaced as annotations only — the loose mode works as intended.
- Deleted `ci/fork-checks` (local + remote) and its worktree: the file now lives
  on `release/*` only, so nothing else needs to carry it.
- Deleted `feat/aur-carried` (local + remote): it existed to replay the AUR
  patch files, which no longer exist. `release/0.4.3-fork.1` is the delivery
  branch and its history already contains the replayed commits.
- Branch model (user-decided, now in `patches/README.md`): one branch per
  change (`feat/*` / `fix/*` from `main` + upstream PR), one branch per release
  (`release/<next>` branched from the previous release branch and cherry-picking
  the new changes; version `<upstream>-fork.<n>`). A published tag is never
  moved — the AUR `source=` URL and its `sha256sums` pin it.

## 2026-10-15 (final): one integration branch (dev), fork.1 withdrawn

The user rejected the per-release branch + cherry-pick model as too much
mechanical work and asked for a single development branch. Correct call: a
`feat/*` branch based on `main` merged `--no-ff` into a long-lived `dev` keeps
the upstream PR diff clean (verified: the feature branches are 1–3 commits
against `origin/main` with a diff that touches only their own files) while
removing the cherry-pick and release-branch bookkeeping.

- Fork: `dev` = `origin/main` + `git merge --no-ff` of each of
  `feat/ffi-page-size-query`, `feat/font-text-advances`,
  `fix/decoded-image-alloc-budget`, `fix/region-aa-tolerance`, `feat/pkgconfig`.
  The carried inventory is now machine-derived, not hand-written:
  `git log --oneline --merges origin/main..dev` lists exactly those five merges
  and `git diff --stat origin/main..dev` is the whole delta (26 files).
- Version on `dev`: `0.4.3-fork.2`. A new number was required because the tree
  differs from `v0.4.3-fork.1` (CI workflow moved onto `dev`, `FORK.md` added)
  and two trees must never share one version when consumers pin a checksum.
- `FORK.md` added on `dev`: branch table, release steps, CI description, and an
  explicit section overriding this repo's `AGENTS.md` "Versioning and Releases"
  (which still says to prepend `debian/changelog` and match the Debian version —
  that is upstream's flow and contradicted ours). `AGENTS.md` itself stays
  untouched, so upstream merges never conflict.
- CI retargeted from `release/*` to `dev` (strict on `dev`, loose for
  dispatched `feat/*` / `fix/*` refs). Local strict equivalent green: fmt,
  clippy `-D warnings`, 49 test-suite groups, `debian/rules`-style release
  build, `run_c_tests.sh` exit 0.
- Published `v0.4.3-fork.2` with the `git archive` tarball (sha256
  `45478ff4…c5c0f65`; the AUR uses the GitHub tag archive, sha256
  `6b6c462a…73c5ba6c0`). Deleted `v0.4.3-fork.1`: GitHub release + tag + the
  `release/0.4.3-fork.1` branch, remote and local — the old archive URL now
  returns 404.
- AUR `librofd-ffi 0.4.3.fork.2-1` pushed (`704808a`): same PKGBUILD shape, new
  `_tag`, `pkgver`, `_dirname` and sha256; `makepkg -C -f` green, packaged
  library exports exactly the 78 `rofd_*` symbols of `expected-symbols.txt`,
  papers-ofd 12/12 against it.
- This repository's consumer docs follow: `patches/README.md` (release model,
  current-release table, routine, checkout example), `docs/plan.md` D11/§9/§8,
  `README.md` requirements and limitation rows, `AGENTS.md` layout and the
  fork-first rule. All now name `dev` + `FORK.md` and `v0.4.3-fork.2`.

### Follow-up: the dev CI had never actually run

Pushing `dev` failed its own workflow: run 38722644359 died in "collect refs"
with `jq: error: syntax error, unexpected ==` on
`map({ref: ., strict: . == "dev"})` — the runner's jq parses an object value as
an arithmetic expression and needs the parens. Two lessons worth keeping:

- this machine's `jq` is **jaq 3.1.1**, a reimplementation that accepts the
  form, and `/usr/bin/jq` here is 1.8.2, which also accepts it — so a local
  check of the expression proved nothing. Only the run itself did. The fix
  (`e8d1429`, parens around the comparison) is accepted by all three.
- the previously "green" runs on `dev`'s predecessor never exercised this path,
  because the `release/*` runs picked refs from `DISPATCHED_REFS`/`PUSHED_REF`
  and never fell through to the `git ls-remote` collector.

`v0.4.3-fork.2` was already tagged and pinned by the AUR when this surfaced, so
the tag was **not** moved: `dev` carries the CI-only fix one commit ahead
(`e8d1429`), which is the normal relationship between an integration branch and
its last release tag. The AUR tarball and sha256 are unaffected — the built
library is identical.

End-to-end after repackaging: `papers-thumbnailer` renders the fixture to a
256x170 PNG and `papers-previewer --fd 3` exits 0 through the staged backend
tree with the installed `0.4.3.fork.2-1` ABI.

## 2026-10-15 (later): release 51.1.1 — version by the Papers line, tag-based AUR source

The user asked which number the first release should carry. Decision (user: "就
这么干", recorded as plan D13): version by the Papers line rather than by our own
feature count — `<Papers line>.<verified baseline>.<our release>`, so `51.1.1` is
the Papers 51 line (the meson `papers-document-4.0 >= 51, < 52` bound), verified
against Papers 51.1, our first release for that baseline. The `0.2.0` alternative
was rejected: the tree differs from the shipped `0.1.0-6` only in tracked
documentation, so a feature-style bump would announce a change that does not
exist; `51.1.1` states the compatibility line instead.

- Repository: `meson.build` `version: '51.1.1'` (scheme in the comment), README
  (install section plus the scheme), `docs/plan.md` (new D13, phase-3 evidence
  bullet, §6 install example and the pc-hiding trap, §7 current version and the
  tag-source bullet) and `AGENTS.md` (version rule). Release commit `8a4c99a`;
  annotated tag `v51.1.1` and `main` pushed to `origin`; GitHub release
  <https://github.com/YangtseSu/papers-ofd/releases/tag/v51.1.1>.
- Verified on the tagged tree before tagging, against the fork's
  `v0.4.3-fork.2` ABI (rofd-ffi.pc hidden from pkg-config, `-Drofd_libdir` at the
  fork build tree): `meson test -C _build` 13/13 — including `ofd-oracle` and
  `ofd-sushi-plugin` — and `reuse lint` compliant (53/53 files).
- Trap fixed while doing this: `PKG_CONFIG_LIBDIR=<copy>` replaces the *whole*
  search path, so copying `/usr/lib/pkgconfig/*.pc` alone is not enough here —
  `xproto`, `kbproto` and `shared-mime-info` live in `/usr/share/pkgconfig`, and
  `papers-document-4.0` needs them. Plan §6's recipe now copies both directories.
- AUR `papers-ofd 51.1.1-1` (`master` = `c2f0387`): `pkgver` 51.1.1, `pkgrel` 1,
  `source=` is the tag archive
  (`…/archive/refs/tags/v51.1.1.tar.gz`, sha256
  `d9c94f4e3804210bce1ec2cb2acfaa0774e9a8b18e71f59ee438314c29618d08`, computed
  from the same URL `makepkg` uses); the `_commit` pin and its comment are gone
  and `.SRCINFO` was regenerated (the hook rejects a stale one). The first push
  attempt died in a connection reset — `aur.archlinux.org` SSH was flaky today,
  and the same happened to a fetch earlier in the session — the retry landed and
  `git ls-remote origin master` confirms `c2f0387`. The RPC frontend still
  reports `0.1.0-6`, the known index lag.
- Package verification: this machine still has `librofd-ffi 0.4.3-4` installed,
  which fails the recipe's `makedepends` floor (`>=0.4.3-6`, the metrics band
  `check()` needs), so the local run pointed `PKG_CONFIG_PATH` at a fork pc
  (`Version: 0.4.3-fork.2`, the fork build tree) and `LD_LIBRARY_PATH` at its
  libdir, and used `--nodeps`: `makepkg -C -f` green, `check()` 12/12 (the
  recipe's 11 plus `ofd-sushi-plugin`; `ofd-oracle` needs `-Drofd_src`), the file
  list unchanged (six files), `readelf -d` on the module shows DT_NEEDED
  `librofd_ffi.so.0` with no rpath, and namcap reports only the known `gcc-libs`
  advisory.
- Rebuilt the current AUR `librofd-ffi 0.4.3.fork.2-1` from its mirror
  (`~/projects/librofd-ffi`, fast-forwarded to `704808a`) so the machine can be
  brought to the ABI the docs assume: the package exports the 78 `rofd_*` symbols
  including `rofd_document_get_page_size_mm`. The `pacman -U` installs are the
  user's step, for both `librofd-ffi-0.4.3.fork.2-1` and `papers-ofd-51.1.1-1`.
- Still open from the phase-3 acceptance list: `pacman -Rns papers-ofd` as the
  uninstall regression.

## 2026-10-15 (later): CI — the AUR build in an Arch container

The user asked whether the project needs CI and a release workflow; decision:
both, with the AUR push left manual (it needs the maintainer's key, the AUR hook
rejects a stale `.SRCINFO`, and the AUR repository is the recipe's only home).

- `.github/workflows/checks.yml`: an `archlinux:latest` container, `pacman -Syu`
  with the build and test dependencies (cairo, freetype2 and jbig2dec explicit —
  they are what the rofd build probes with pkg-config), the ABI built from the
  fork release tag that `patches/README.md` names (read out of its
  current-release table, so the inventory stays the one home), then
  `meson setup/compile/test` and `reuse lint`. Runs on `main`, on pull requests,
  on demand, and as a reusable workflow.
- `.github/workflows/release.yml`: on a `v*` tag it calls `checks.yml` and then
  appends the tag archive's sha256 — the value the AUR recipe's `source=` pins —
  to the release notes, creating the release when the tag has none; the job
  summary carries the same number, and the manual repackaging step has nothing
  left to compute. `workflow_dispatch` re-runs it for a ref.
- The container runs the suite as a `builder` user, the constraint makepkg works
  under: ofd-fd withdraws read permission from a descriptor and expects the
  `/proc/self/fd` re-open to answer `EACCES`, which root never sees.
- Environment: `runs-on: ubuntu-26.04`, `actions/checkout@v7`,
  `archlinux:latest` — current versions of everything the workflows name.
- What stays manual: the GUI acceptance items (§5 phase 3), the uninstall
  regression and the AUR push. Two tests skip in the container because it has no
  display, which is what they are built to do: `ofd-print-file` and the
  live-load half of `ofd-sushi-plugin`.
- The runs found three real gaps, which is what the CI is for:
  1. `jbig2dec` was missing from the container — `crates/rofd-render/build.rs`
     probes it with pkg-config, and the module build had never been tried on a
     machine without the package installed.
  2. `tests/test-ofd-units` compiles `../src/ofd-units.c` (whose header includes
     `<rofd.h>`) without declaring `rofd_dep`, so under `-Drofd_includedir` the
     ABI include path never reached that compile line; locally
     `/usr/include/rofd.h` from the installed package hid the gap.
  3. The suite cannot run as root (see above): the first container run that got
     past the build failed `ofd-fd` alone, with zero cache files found.
- Observed: run 38743654737 (unprivileged) green in 1m55s, then 38744389444 on
  `ubuntu-26.04` with `actions/checkout@v7` green in 1m45s; the failures on the
  way were 38741809680 (jbig2dec) and 38742980443 (ofd-fd as root).
- The `v51.1.1` release notes now carry the pin block the workflow produces; it
  was backfilled with the same commands, which is also how they were verified.
  The AUR RPC frontend reports `papers-ofd 51.1.1-1`, so the earlier index lag
  has cleared.

## 2026-10-15 (later): whole-project review — baselines, probes, R1–R19

Read-only audit of the module, the tests, the packaging, the CI and the docs
against Papers 51.1 and the fork ABI. No repository file was modified; the probe
programs and staged trees live outside the repository (`/tmp`, never committed).
The defect inventory is in `findings.md` §"Review findings (2026-10-15)";
phases 20–26 in `task_plan.md` are the fix campaign.

### State at review time

- `git status` clean, `git diff --stat` empty; the tree is the 51.1.1 release plus
  the CI commits.
- `scripts/dev-env.sh` resolves `PAPERS_SRC=$PWD/../papers` (ok, 51.1),
  `ROFD_SRC=$PWD/../rofd` (ok, `dev`, `v0.4.3-fork.2`), `ROFD_ABI_SOURCE=checkout`;
  `librofd-ffi 0.4.3.fork.2-1` is installed, so `rofd-ffi.pc` exists and the meson
  `dependency('rofd-ffi')` branch is the one in use here.
- The plan was extended in place (same `.planning` directory) rather than forking a
  second plan: the resolver selects this directory and the repository's working
  memory must stay one file.

### Baseline runs (all reproduced)

- `meson setup --reconfigure _build … && meson compile && meson test`:
  **13/13 OK**; `_build-slow` (`-Drofd_page_size=disabled`): **13/13 OK**.
  `ofd-oracle` printed `oracle 2115x1400: mean channel difference 3.283; over 8:
  2.61%; over 64: 1.86%` — the phase-1 numbers hold.
- `reuse lint`: compliant, 55/55 files with license and copyright information.
- Real Papers stack, `PPS_BACKENDS_DIR` = the system backend directory plus the
  built module and descriptor, `XDG_DATA_DIRS` with a freshly indexed MIME tree:
  `papers-thumbnailer -s 256` → 181×256 for the generated text fixture and
  256×170 for `$ROFD_SRC/learning/test.ofd`, matching the phase-3 record.
- `tests/ofd-latency … 5` in both builds: fast path `load 2.0 ms / info walk
  20.0 ms / setup_cache 70.7 ms / first render 47.4 ms / total 139.6 ms`; slow path
  `load 2.0 ms / walk 70.7 ms / setup_cache 0.1 ms / render 46.6 ms / total
  118.5 ms`. The README's "~120 ms" describes the slow build's sequence, not the
  shipped fast one.
- `ofd-text` log shows the strict branch ran (`layout 28.7 mm, ink 28.2 mm ->
  font-resolved geometry`), so the A24/A27 assertions are live against this ABI.

### Probes (throwaway C programs, public API only)

1. **Degenerate selection (R1).** After `pps_document_text_get_text_layout`, a
   `PpsRectangle` with `x1 == x2, y1 == y2` at the centre of the first glyph:
   `GLYPH/WORD/LINE point-rect -> EMPTY ""`; the same call with a positive-area
   band over the first four scalars: `Invo`, `Invoice`,
   `Invoice total: 42 EUR`; `WORD region: point-rect empty/NULL (rects 0)`,
   `band-rect non-empty (rects 1)`.
2. **Rotated selection region (R2).** With the rc shape
   `compute_scroll_increment` builds (model rotation, `rc->scale = 0`, target
   size 600×850): `rotation 0 → non-NULL`, `90/180/270 → NULL`. A three-line
   cairo probe confirmed `cairo_region_num_rectangles (NULL)` exits 139.
3. **Zoom vs raster budget (R3).** A scale sweep on the A4 text fixture:
   `4.0 … 7.0 → surface`, then `8.0 → NULL` with
   `render failed: raster surfaces require 288662100 bytes but the limit is
   268435456` (and proportionally larger numbers up to 12.0).
4. **Fontless environment (R4).** `unshare -rm --map-root-user` with an empty
   directory bind-mounted over `/usr/share/fonts` (`ls | wc -l` → 0): the text
   test aborts at `../tests/test-ofd-text.c:757`, assertion
   `areas[0].y2 > MM (6.0)`. With fonts present it passes. An empty fontconfig
   config does *not* reproduce it — rofd uses `fontdb`, not fontconfig.
5. **Discovery failure (R6).** A C probe calling
   `pps_document_factory_get_document` with a valid descriptor+module but no
   `pps_init` → `File type 开放式版式文档 (application/ofd) is not supported`
   (identical to the broken-`Module=` case the test asserts); with `pps_init` and
   a broken `Module=` → `Failed to load backend for 'application/ofd'`, and with
   `pps_init` and a valid descriptor → `document=RETURNED`.
6. **`dev-env.sh` sourcing (R5).** `zsh -c 'echo BEFORE; . ./scripts/dev-env.sh;
   echo AFTER'` prints only `BEFORE` (no `AFTER`, no exports);
   `bash -c` prints both and exports `ROFD_SRC`. `bash -c '. scripts/dev-env.sh
   -h; echo AFTER'` also prints only `BEFORE` — the `-h` exit is unguarded.

### Errors hit while probing

| Error | Attempt | Resolution |
|-------|---------|------------|
| `gjs` probe: `Papers.document_factory_get_document` / `factory_get_document` "is not a function" | 1 | GI names it differently from the C symbol; replaced gjs with a 30-line C probe against `pps_document_factory_get_document`, which answered the question directly |
| `cairo_null.c` did not compile (`'NULL' undeclared`) | 1 | Used a `(cairo_region_t *) 0` cast; the probe then exited 139, which is the evidence for R2 |

## 2026-10-15 (later): phase 20 — the two selection blockers (R1, R2)

Both fixed with a regression test that fails before the change and passes after
it, then the whole suite in both page-size builds.

### R1 — a point selection selects the glyph, word or line under it

- Test first: `tests/test-ofd-text.c` gains `test_selection_point_rectangles`,
  which builds the rectangle Papers hands over for a double/triple click
  (`x1 == x2, y1 == y2` at a glyph centre), derives the expected word and line
  rectangles from the module's own layout, and requires the point answers to equal
  them plus a region that matches the line band within 2 px.
- Fail before: `1/1 papers-ofd:ofd-text FAIL`, `Bail out!
  ERROR:../tests/test-ofd-text.c:659:test_selection_point_rectangles: assertion
  failed (strlen (point_glyph) >= 1): (0 >= 1)` — rofd answered success with an
  empty selection for the zero-area query.
- Fix: `ofd_text_select_mm` widens a degenerate axis to `0.02 mm` centred on the
  point (`OFD_SELECTION_POINT_EPSILON_MM`) before the ABI call; the whole-box query
  from `get_text_mapping` is untouched (it is already positive-area).
- A second failure appeared in the test's own expectation: the left-column line
  band also covered the right column, which shares the y band, so the LINE answer
  was both lines. Fixed by restricting the band to `LEFT_COLUMN_X2_MM`.
- Independent confirmation with the original probe against the rebuilt module:
  `GLYPH point-rect -> "I"`, `WORD -> "Invoice"`, `LINE -> "Invoice total: 42 EUR"`,
  `WORD region: point-rect non-empty (rects 1)` — all three were `EMPTY ""` and 0
  region rectangles before.

### R2 — the region answer is always a region

- Test first: `test_selection_region_is_always_a_region` requires a non-NULL region
  for rotations 0/90/180/270 (empty for the rotated ones), and for a NULL points
  argument; `test_broken_page`'s old `g_assert_null` on a page whose handle cannot
  be fetched becomes "non-NULL and empty".
- Fail before: NULL for every rotated context (and the broken-page assertion held
  only because NULL was also what a missing handle answered).
- Fix: every refusal/failure path of `ofd_selection_get_selection_region` returns
  `cairo_region_create ()`; the reason (Papers' unchecked
  `cairo_region_num_rectangles` in `compute_scroll_increment`, cairo's unconditional
  dereference, poppler's contract) is recorded in the code comment and in plan §3.
- Independent confirmation: the rotated-context probe now prints non-NULL for
  0/90/180/270 (before: NULL for 90/180/270).

### Observed

- `meson test -C _build`: **13/13 OK**; `meson test -C _build-slow`
  (`-Drofd_page_size=disabled`): **13/13 OK**.
- `docs/plan.md` §3 gains the point-query rule on the `get_selected_text` row and
  the "always a region" rule on the `get_selection_region` row; §8's rotated-page
  row says the refusal is an empty region, not NULL.
- Still open: the optional GUI pass (double-click/triple-click/Ctrl+C and PageDown
  after a rotation) — the live session was not part of this run.

## 2026-10-15 (later): phase 21 — the mechanical robustness fixes

Every change below was written as a test first and observed failing, then fixed
and observed passing; the full suite is 13/13 in `_build` and `_build-slow` at the
end, `reuse lint` is clean.

### R5 — `scripts/dev-env.sh` killed the caller's shell under zsh

- Evidence before: `zsh -c 'echo B; . ./scripts/dev-env.sh; echo A'` printed only
  `B` (the script's `exit` ran inside the caller and nothing was exported);
  `bash -c '. scripts/dev-env.sh -h; echo A'` printed only `A`'s absence for the
  same reason.
- Fix: sourcing is detected from `ZSH_EVAL_CONTEXT` (`*:file`) or a `BASH_SOURCE`
  that differs from `$0`; the `-h` branch returns when sourced; the tail no longer
  `exit`s at all — the script's status is the last command's status, so
  `scripts/dev-env.sh` still answers 0/1 while a sourced run only sets `$?`.

### R6 — the deliberate discovery failure asserted nothing

- Evidence before: with a *valid* descriptor, a probe without `pps_init ()` also
  answered `File type … is not supported` — the same error the test accepted from a
  broken `Module=`.
- Fix: `pps_init ()` after `PPS_BACKENDS_DIR` is staged, and the error must contain
  `Failed to load backend`, which only the module-open path produces (probe:
  broken `Module=` + init → `Failed to load backend for 'application/ofd'`; valid
  descriptor + init → document returned).

### R8 — an actionless outline row reported page -1

- Evidence before: the fixture's new grouping node made
  `tests/test-ofd-links.c:125` fail (`'action' should not be NULL`), because
  `pps_document_links_get_link_page` answers -1 for an actionless link, which the
  sidebar turns into "print from page 1" (`sidebar_links.rs:686`) and a
  `usize::MAX` page-map key (`:548-551`).
- Fix: `ofd-outline.c` caches the node snapshot, resolves each row's action in a
  reverse-preorder pass (own action, else the first child whose subtree has one),
  and the model is built from that cache. The fixture gained a grouping node
  ("附属内容" with child "4.1 附录", FitV page 2 left 15) and the test asserts both
  rows navigate to page 1 with the child's coordinates.

### R9 — the ABI classifier could be moved by the code it classifies

- Fix: `linked_abi_resolves_font_geometry` now reads the first line's span through
  `rofd_page_get_text_layout` on the oracle handle instead of the module's
  `get_text_layout`, and the verdict is printed with `g_print`
  (`first line: ABI layout 28.7 mm, ink 28.2 mm -> font-resolved geometry`), which
  `meson test` collects into `_build/meson-logs/testlog.txt` — the old
  `g_test_message` calls were no-ops without `g_test_init`, which this program
  cannot call (it takes positional arguments). README and plan §6 corrected.

### R11 — withdrawn after probing the ABI

- Probe (`/tmp/rev/linkprobe.c`, plain rofd.h): the links fixture reports five
  entries, **each with `action_count == 1`** — rofd publishes one entry per action,
  so the object whose first action is an unresolved GOTO still has its URI action
  published as its own entry (`entry 4 … uri=https://example.test/fallback-action`),
  and the unresolved entry (`page=18446744073709551615`, no `HAS_PAGE_INDEX`) is
  dropped, which is the documented rule.
- A defensive "keep scanning actions within an entry" loop was written, found
  unreachable (no fixture can produce `action_count > 1`), and reverted; the
  comment in `ofd-links.c` records the probed model. The fixture and test now
  assert it: 5 oracle entries, 4 published mappings, the drop and the sibling URI
  compared entry by entry.

### R12 — the retained descriptor was inherited by children

- Test first: `test_fd_not_inherited_by_children` hands `pps_document_load_fd` a
  descriptor without `FD_CLOEXEC` and then runs `sh -c 'for f in /proc/self/fd/*; do
  readlink "$f"; done'` with `G_SPAWN_LEAVE_DESCRIPTORS_OPEN` (without that flag
  GLib closes every descriptor above stderr in the child, which is why the first
  version of the test passed against the unfixed code).
- Fail before: `'g_strstr_len (child_fds, -1, base)' should be NULL` — the child
  could see the document file.
- Fix: `fcntl (fd, F_DUPFD_CLOEXEC, 3)`, the same call Papers' own descriptor
  handling uses (`libview/pps-jobs.c:798-811`).

### R13 — a refusal left the previous selection's pixels in the reused surface

- Test first: `test_render_selection_refusal_drops_surface` renders a selection,
  then calls `render_selection` again with a rotated context and with a NULL
  rectangle; fail before: `'surface' should be NULL`.
- Fix: `ofd_selection_drop_surface` destroys the reused surface on every refusal
  path, so the view cannot rebuild a stale highlight from it
  (`pps-pixbuf-cache.c:1054-1068` builds the texture from whatever the surface
  holds).

### R16 — the paper-size bit announced pages that never resolved

- Test first: `test_partial_geometry_document` loads the broken-page fixture and
  asserts the invariant "if `PAPER_SIZE` is set, no page size is 0"; fail before:
  `(info->page_sizes[i].width > 0.0): (0 > 0)`.
- Fix: `get_info` passes the sizes only when every page box resolved, and
  `ofd-metadata.c` sets `N_PAGES` only for a non-negative count.

### Observed

- `meson test -C _build`: **13/13**; `meson test -C _build-slow`
  (`-Drofd_page_size=disabled`): **13/13**; `reuse lint` compliant.
- The shell matrix for R5: source under bash and zsh both export; `-h` under both
  returns to the caller; an executed run exits 0 with a working ABI and 1 without
  one; the report output is unchanged.
- Not verified: no live GUI session in this run (the blockers' manual items and
  the outline sidebar still deserve one).

## 2026-10-15 (later): session A closed — decisions taken, phase 22 handed over

Session A (phases 20 + 21) ends here by the split agreed at the start of the
campaign. The four open decisions were taken by the user at the end of it:

| Question | Decision |
| --- | --- |
| Q7 / R3 render budget | derive `max_raster_bytes` from the request (rofd's worst case ≈9·w·h) under a module cap, and lower the scale beyond it instead of failing |
| Q8 / R4 fonts | declare the fonts the suite measures with in the AUR recipe **and** make the in-repo test name the missing dependency |
| Q9 / R7 concurrency | poppler's model: `GRWLock`, writer on `load`/`load_fd`, reader around each vfunc |
| Q10 release | one release (`51.1.2`) after R3/R4/R10, not the blockers alone |

Session state to pick up from:

- `main` is at `1d0cef5` (phase 21) on top of `3de5b97` (phase 20) and `c4233d8`
  (the review record); the tree is clean and `meson test` is 13/13 in `_build` and
  `_build-slow`.
- Phase 22's test skeleton was written during that decision round and then
  **reverted** (`git checkout -- tests/test-ofd-render.c`), so the tree stays
  green: the assertions below are the design to re-create first, module-side fix
  second. The campaign rule is test-first, and a red test must not be committed.

### Phase 22 test design (re-create in `tests/test-ofd-render.c`)

`test_high_zoom_still_renders (metadata_document)`, scales `{4, 7, 8, 10, 12, 16}`
(7 renders today, 8+ returns NULL):

- `pps_document_render` returns non-NULL with `CAIRO_STATUS_SUCCESS`;
- width/height are `> 0`, `<= requested + 1` (never larger than asked for) and
  `>= requested / 2` (not degraded into uselessness);
- a selection overlay rendered through `pps_selection_render_selection` with the
  *same* render context has exactly the texture's dimensions — the view draws them
  over one another, which is why the degrade must happen in one place (plan §4).

`test_oblique_rotation_snaps (metadata_document)`, rotations 0 / 45 / 90:

- the 45° render has the 90° render's dimensions (snapped to the nearest quarter
  turn) and the upright render's swapped dimensions — i.e. a real rotated page,
  not the blank one rofd's `INVALID_ARGUMENT` used to produce.

Also in phase 22: R15 (a failed `rofd_renderer_render_page_cairo` reports at
`g_warning`, diagnostics stay at `g_debug`) and a README latency/limitation
refresh if the render path's numbers move.

## 2026-10-15 (session B): phase 22 — render robustness (R3, R14, R15)

Session B starts from `1e7fe74` (the session prompt) on top of phase 21's
`1d0cef5`; baseline before the phase: `meson test` **13/13** in `_build` and
`_build-slow`, `reuse lint` compliant.

### Test-first: the failures observed before the fix

Both tests were written first and watched failing; the module was not touched
until they were red.

- `test_oblique_rotation_snaps` (rotations 0/45/90 on the metadata fixture):
  `Bail out! ERROR:../tests/test-ofd-render.c:71:render_page: 'surface' should not
  be NULL` — the 45° render (R14).
- `test_high_zoom_still_renders` (scales 4/7/8/10/12/16): with the oblique test
  temporarily commented out, the same assertion fired at the first scale above 7
  (`ERROR:../tests/test-ofd-render.c:306 ... 'surface' should not be NULL`) — the
  scale-8 request R3 describes, whose budget rofd used to refuse with
  `raster surfaces require 288662100 bytes but the limit is 268435456`.
- The R15 test came third: with the module still logging the failure at `g_debug`,
  `assertion failed (render_warning_count == 1): (0 == 1)` while
  `pps_document_render` already answered NULL.

### R3 — the raster budget is derived from the request

`src/ofd-render.c` maps a render context onto rofd's options in one place
(`ofd_render_options_from_context`), so the page texture and every overlay take the
same fitted scale:

- `ofd_render_worst_case_bytes()` mirrors rofd's
  `worst_case_surface_bytes` (`2*stride_argb32*h + stride_a8*h`, strides through
  cairo) and `ofd_render_required_bytes()` upper-bounds the canvas as
  `ceil(points * scale) + 1` per axis, for both orientations, so the derived budget
  can never fall below rofd's own requirement.
- `max_raster_bytes` becomes `MAX(required, rofd's initialized default)`. The
  default is not merely a fallback: rofd charges **decoded source images** against
  the same field (`raster_budget = limits.max_decoded_image_bytes.min(
  max_raster_bytes)`, `cairo_renderer.rs:1624`), which the first implementation
  missed — a budget fitted to a 2115×1400 canvas (26.6 MB) starved the invoice's
  seal image and `ofd-oracle` failed with `'surface' should not be NULL` until the
  budget stopped dropping below the default.
- Cap: `OFD_RENDER_MAX_RASTER_BYTES = 450 MiB`, i.e. Papers' own default allowance
  (the 200 MiB `page-cache-size` at 4 B/px ≈ 10.2× on A4) converted to rofd's
  ≈9 B/px rate (2.25×). Beyond it the scale is lowered by `sqrt(cap/required)`
  until the canvas fits, at `g_debug`.

Measured (module debug log, `G_MESSAGES_DEBUG=papers-ofd`, metadata fixture, A4):
scales 4/7/8/10 render at full size (no fit message); 12 and 16 are fitted to
`scale 10.227` — the same 10.23× Papers' own clamp allows for the default cache —
with the requested 7143×10103 (12×) and 9524×13470 (16×) px. `test_high_zoom_still_renders`
then asserts, per scale, `0 < size <= requested + 1` and `>= requested / 2`, and
that the selection overlay has the texture's dimensions.

### R14 — an oblique rotation snaps to the nearest quarter turn

`ofd_render_snap_rotation()` normalizes and rounds `rotation / 90` (45° ties
upward) before handing it to rofd, whose renderer accepts only 0/90/180/270
(`cairo_renderer.rs:1158-1163`). The 45° render is compared **pixel by pixel** with
the 90° render and has the upright render's swapped dimensions.

### R15 — a failed render path is a warning, not a debug line

- a failed `rofd_renderer_render_page_cairo` is reported at `g_warning`; the
  object-level diagnostics stay at `g_debug` (D9);
- the same applies to a page surface cairo refuses. That is the reachable shape,
  and it was silently returning NULL before: rofd's canvas query bounds dimensions
  against i32 only, while cairo 1.18.6 refuses anything above 32767 px with
  `CAIRO_STATUS_INVALID_SIZE` (measured: 34016×851 → status 32, 32767×851 → 0), so
  the render call is never reached. The new `strip` fixture (a 1000×25 mm page,
  34016×851 px at 12×) drives exactly that; the test asserts rofd's canvas query
  still succeeds for the same options (so the failure is the surface, not the
  query) and that exactly one warning was emitted.

### Observed

- `meson test -C _build`: **13/13**; `meson test -C _build-slow`: **13/13**
  (`ofd-render` 0.8–0.9 s, the whole suite ≈4 s).
- `ofd-oracle` reproduces the recorded metric unchanged: `oracle 2115x1400: mean
  channel difference 3.283; over 8: 2.61%; over 64: 1.86%`.
- `reuse lint`: compliant, 56/56 files.
- `tests/ofd-latency`, 5 runs, both builds (the render path moved, so it was
  re-measured): fast build info walk 22.5–23.2 ms, `setup_cache` 80–83 ms, first
  render 52.3–58.8 ms, best total 157.4 ms; slow build info walk 66–82 ms,
  `setup_cache` 0.1 ms, first render 43.5–58.3 ms, best total 111.2 ms. The
  run-to-run spread is larger than any difference from R3 (the budget for a 1×
  A4 page is rofd's default, so nothing changes for that path), so the README's
  latency row keeps its numbers.
- No live GUI session in this run: the interactive zoom/rotate items stay on the
  manual list (phase 25/26 and the phase-20 GUI pass).

### Files

`src/ofd-render.c` (budget derivation, rotation snap, R15 warnings),
`tests/test-ofd-render.c` (three new checks), `tests/meson.build` +
`tests/generate_fixture.py` (the `strip` fixture), `docs/plan.md` (§4 render math,
§8 risk rows, A16 corrected), `README.md` (suite description, limitation table).

## 2026-10-15 (session B): phase 23 — concurrency hardening (R7)

### Test first: the failure the review predicted

`tests/test-ofd-concurrency.c` (new, registered in `tests/meson.build` as
`ofd-concurrency`): worker threads issue page requests — `get_text`,
`get_text_layout`, `find_text`, `render`, `get_info` in rotation, plus
`get_links_model` every 16th turn, i.e. every path the review found borrowing the
document handle or the geometry array — while the main thread reloads the *same*
instance, alternating `pps_document_load` (URI) and `pps_document_load_fd`
(descriptor). Bounded on purpose (fixed per-worker request budget and a fixed
reload count) and with a 500 µs pause between reloads: the lock is
reader-preferring, so a spinning worker set would let the writer starve and the
run turn into a scheduling experiment (observed: a spin-loop form took anywhere
between 0.4 s and 16 s).

`_build-asan` (`meson setup _build-asan -Db_sanitize=address -Db_lundef=false ...`),
long form `8 workers × 2000 requests × 200 reloads`, **before** the fix — 5 of 5
runs failed:

```
==502971==ERROR: AddressSanitizer: heap-use-after-free on address 0x7c1756242e90
    #10 ... in rofd_document_get_page
    #11 ... in ofd_document_get_page (/tmp/.../libofddocument.so)
freed by thread T0 here:
    #2 ... in rofd_document_free
    #3 ... in ofd_document_load_path (/tmp/.../libofddocument.so)
    #4 ... in ofd_document_load
```

runs 3–5 aborted as `AddressSanitizer: SEGV` inside `librofd_ffi.so.0` (the freed
handle's fields read as an address like `0x2667`); runs 1–2 produced the clean
use-after-free report above. So R7, which the review recorded as *code evidence*,
is reproduced: `load` freed the handle a concurrent `get_page` was inside.

### The fix: one `GRWLock`, poppler's model

`GRWLock lock` replaces `GMutex mutex` in `OfdDocument`:

- **writer** in `ofd_document_load`/`load_fd` (`load_path` is documented as
  writer-held); it replaces the handle, resets the geometry array, drops a
  previous fd/temp copy;
- **reader** around every vfunc: `get_n_pages`, `get_page`, `render`,
  `print_page`, `get_info`, `has_document_links`, `get_links_model`,
  `get_links`, and — through `ofd_document_reader_lock`/`_unlock`, since the
  struct is opaque there — every `PpsDocumentText`/`PpsDocumentFind`/`PpsSelection`
  vfunc in `ofd-text.c`. `ofd_document_borrow_handle_locked`,
  `_borrow_page_locked`, `_borrow_renderer_locked`, `ofd_page_ref_acquire` and
  `ofd_link_action_from_rofd` all document that the read lock is held by the
  caller; the lock is not recursive and no helper takes it twice.
- the **renderer is created in `load`** instead of lazily on the first render,
  because a creator would be a writer inside a reader section. Consequence for
  the numbers: the 300-page latency run now spends ~26 ms in `load` (rofd's font
  stack) and ~20 ms on the first render instead of ~2 ms + ~48 ms; the total is
  unchanged (~130 ms), and the README's latency row was updated with the new
  split.
- the **geometry cache** is only written under the writer: `page_box_mm` answers
  from the cache under the read lock and, on a miss, re-checks and fills under
  the write lock; `page_box_mm_locked` (used inside reader sections) reads the
  cache and queries rofd directly on a miss, so two readers cannot race on a slot.
- `get_info` collects the page boxes outside the read section and, if a reload
  changed the page count in between, drops the array (R16's rule applied to a
  concurrent reload).
- `rofd.h`'s rule is quoted where it matters: "no handle is freed while another
  call is using it" (rofd-ffi/include/rofd.h, document-lifetime contract), and D2,
  the plan's architecture diagram, §8 and `AGENTS.md` were reworded to this
  single-lock discipline.

### Observed

- Post-fix ASAN, long form `8 2000 200`, with `ASAN_OPTIONS=detect_leaks=0`:
  **5 of 5 runs exit 0, no AddressSanitizer report at all** (`200 reloads with 8
  workers x 2000 requests: 16000 concurrent requests, no crash`, ~20–23 s each).
- With leak detection on, the only report is Papers' own: 99 × 86 bytes
  `g_strdup` in `pps_document_load` (`libdocument/pps-document.c:365` assigns
  `priv->uri` without freeing the previous value; `:95` frees it only in dispose)
  — one URI per reload of the same instance. Recorded as A29; it is an upstream
  leak, not ours, and it is why those runs are judged on memory errors.
  The test harness' own leaks (staged-directory paths in `ofd-test-utils.c`) were
  fixed in this phase so that LeakSanitizer's output is about the module.
- `meson test -C _build`: **14/14** (the concurrency case 1.6 s with
  `4 workers × 100 requests × 25 reloads`); `meson test -C _build-slow`: **14/14**.
- `reuse lint`: compliant, 57/57 files.
- `tests/ofd-latency` re-measured in both builds after moving the renderer
  creation into `load` (see above): fast build load 25.6–26.2 ms, info walk
  18.1–19.5 ms, `setup_cache` 65.6–66.5 ms, first render 19.1–23.3 ms, best total
  129.3 ms; slow build load 27.5 ms, info walk 69.5–70.5 ms, `setup_cache` 0.1 ms,
  first render 20.0–24.4 ms, best total 117.1 ms.
- Not verified: no live GUI session (the interactive reload path in Papers itself
  — a failed-load retry — is not reproducible headlessly).

### Files

`src/ofd-document.[ch]`, `src/ofd-page.[ch]`, `src/ofd-text.c`, `src/ofd-links.[ch]`,
`src/ofd-outline.c` (the lock and the `_locked` accessors),
`tests/test-ofd-concurrency.c` + `tests/meson.build` +
`tests/ofd-test-utils.c` (harness-leak fix), `docs/plan.md` (D2, diagram, §8,
A28/A29), `README.md` (concurrency paragraph, latency split), `AGENTS.md`.

## 2026-10-15 (session B): phase 24 — CI and packaging parity (R4, R10)

### R4 in-repo — the text test names the missing font

Reproduced first, in a mount namespace with `/usr/share/fonts` emptied
(`unshare -rm --map-root-user`, the recipe from the review probes):

```
Bail out! ERROR:../tests/test-ofd-text.c:910:test_origin_normalisation:
  assertion failed (areas[0].y2 > MM (6.0)): (17.007874 > 17.007874)
```

— a millimetre comparison that says nothing about the cause (rofd resolves fonts
through `fontdb`, so no environment variable hides a missing font).

`tests/test-ofd-text.c` now has `require_fixture_fonts()`, run before anything
measures geometry: it renders both fixture pages through the linked ABI and counts
`ROFD_DIAGNOSTIC_MISSING_GLYPH` diagnostics, because a host without fonts still
paints *something* (rofd substitutes a box), so ink alone does not distinguish
"no font" from "different metrics". Observed:

- no fonts at all: `fixture page 0 (Latin): 143 missing glyphs` → `** ERROR **: the
  fixture's Latin page has 143 unresolvable glyphs: the linked rofd resolved no
  usable font. Install the fonts the suite measures with (Arch: noto-fonts; the AUR
  recipe's check() lists them) and re-run.`
- Latin fonts only (a namespace with `/usr/share/fonts/noto-cjk` hidden):
  `fixture page 0 (Latin): 0 missing glyphs`, `fixture page 1 (CJK): 17 missing
  glyphs` → the error names `noto-fonts-cjk`.
- the host's normal environment: `0` and `0`, and the suite passes.

### R4 packaging — the recipe declares them

Prepared in the AUR working copy (`~/projects/ofd-reader/papers-ofd-aur`) and
**committed locally there** (`f64a19c`, `pkgrel` 1 → 2 because the payload is
unchanged — D13; `.SRCINFO` regenerated with `makepkg --printsrcinfo`). The AUR
push itself stays the user's step:

```diff
--- a/PKGBUILD
+++ b/PKGBUILD
@@ -28,6 +28,15 @@ makedepends=(
 # The space-bar preview in GNOME Files is Sushi's; our plugin only takes effect
 # where Sushi is installed (and gjs + Sushi make tests/test-sushi-plugin.js run).
+# The suite measures the geometry of text the fixtures draw with a *system* font
+# (rofd resolves fonts through fontdb, not fontconfig, so no environment variable
+# hides a missing one), and tests/test-ofd-text.c names the missing package instead
+# of failing an opaque assertion. Declared here rather than in makedepends: they
+# are only needed while check() runs.
+checkdepends=(
+  'noto-fonts'      # the Latin pages
+  'noto-fonts-cjk'  # the CJK page of the text fixture
+)
 optdepends=('sushi: space-bar previews in GNOME Files')
```

`makepkg --printsrcinfo` reports the two `checkdepends` lines, so
`.SRCINFO` gains exactly those when the user regenerates and pushes;
`.SRCINFO` currently differs from the working copy in nothing else. `namcap
PKGBUILD` still reports only its pre-existing advisory (the ABI appears in both
`depends` and `makedepends`, which is deliberate: `check()` needs the geometry of
`>= 0.4.3-6` while `depends` needs `>= 0.4.3`). The deployment path itself (the
tag archive, `--buildtype=plain`, `check()` invoking the suite) is unchanged, so
the recipe revision is packaging-only and stays in `pkgrel`.

### R10 — meson.build: an old `.pc` is not a missing one

`dependency('rofd-ffi', version: ['>= 0.4.2'], required: false)` cannot tell the
two apart, so `meson.build` now asks again without the constraint and, when the
`.pc` exists but is older, fails with the versions named instead of "not found"
(which sends the reader to install a package they already have).

**Floor settled: the module floor stays 0.4.2, the packaging floor is 0.4.3.**
0.4.2 is the module's own correctness floor (A19: `<= 0.4.0` aborts a whole page
render on a singular transform, `0.4.2` maps it to a diagnostic); the page-size
query and the font-resolved geometry are *optional* — detected at configure time
and classified at runtime — while the AUR `librofd-ffi>=0.4.3-6` is the fork
release that also ships `rofd-ffi.pc`, which is what a `.pc`-based build needs in
the first place. Both floors are now stated where they belong: `meson.build`,
README's requirements row, plan §8.

Observed locally with a hand-written `.pc` (0.4.1 vs 0.4.3.fork.2 in a scratch
`PKG_CONFIG_PATH`):

```
Dependency rofd-ffi for host machine found: NO. Found 0.4.1 but need: '>= 0.4.2'
Run-time dependency rofd-ffi found: YES 0.4.1
meson.build:54:4: ERROR: Problem encountered: rofd-ffi.pc reports 0.4.1, but the
  module needs >= 0.4.2: install a newer librofd-ffi, or pass
  -Drofd_libdir=<dir> -Drofd_includedir=<dir> for a build tree (see scripts/dev-env.sh)
```

### R10 — CI: the fallback build and the installed-`.pc` build

`.github/workflows/checks.yml` gained two steps after the existing
configure/build/test:

- **page-size fallback**: `meson setup build-slow … -Drofd_page_size=disabled` plus
  its own compile and test run, i.e. the `#else` branch of the page-size detection
  that the ordinary build never compiles;
- **installed `rofd-ffi.pc`**: the ABI is installed into `/rofd/prefix` exactly the
  way the `librofd-ffi` recipe does it (library + SONAME symlink + `rofd.h` + the
  `.pc` generated from the fork's `debian/rofd-ffi.pc.in`), and the module is
  configured *without* `-Drofd_libdir`/`-Drofd_includedir`, so `dependency('rofd-ffi')`
  is what provides headers, library and version. The tag-resolution step now also
  derives the recipe's `pkgver` (`v0.4.3-fork.2` → `0.4.3.fork.2`) for the template
  substitution.

Both were verified locally (a container is not available on this machine) with the
same commands: `_build-slow` is that configuration and passes 14/14; a scratch
prefix with the pc built from the template configured, compiled and passed 13/13
(the AUR package's set, without `-Drofd_src`), and the generated `.pc` is identical
to the one the installed `librofd-ffi 0.4.3.fork.2-1` ships.

### R10 — `ofd-latency` stays a tool, now explicitly

Decision: **not registered with `test()`**. Its output is a wall-clock table, and a
threshold would be machine-dependent (minutes apart between this host and a
container), so an assertion would be vacuous or flaky. It keeps asserting what it
depends on structurally (a non-empty page count — added in this phase — the
document info and the first render), and the status is now stated in three places:
the tool's header, `tests/meson.build` next to the executable, and the README
limitations row that quotes its numbers.

### Observed

- `meson test -C _build`: **14/14**; `meson test -C _build-slow`: **14/14**
  (the fallback configuration CI now builds).
- `reuse lint`: compliant, 57/57 files. `.github/workflows/checks.yml` parses as YAML
  (`yaml.safe_load`).
- Test counts in the docs were made honest again (this session added
  `ofd-concurrency`): README's Install section and plan §6/§7 now say 12–13 tests
  for a package build instead of 11, and the suite lists include the new case.
- Not verified: the workflow itself (no container on this machine); its steps are
  the commands verified locally above, and pushing a branch runs the real one.
- The branch was pushed with the user's approval (`main` → `YangtseSu/papers-ofd`,
  `b90d68d..c87b72e`, run
  [38809559149](https://github.com/YangtseSu/papers-ofd/actions/runs/38809559149)),
  which is the only way to exercise the two new CI steps in the real container. The
  run's steps 8 (the module) and 9 (the page-size fallback) passed — the first
  container proof that the fallback build works there — and step 10 failed:
  `fatal error: rofd.h: No such file or directory`, with no `-I/rofd/prefix/include`
  in the compile line. Cause: the `.pc` was generated from the fork template with
  its `prefix=/usr` left alone, so pkg-config reported `/usr/include` and
  `/usr/lib` — which worked in the local probe *only* because this machine has the
  installed `librofd-ffi`, an accidental pass the container exposed. Fixed by
  rewriting `prefix=` to the CI prefix in the same `sed` chain (the AUR recipe keeps
  `/usr` because it installs into `$pkgdir`), and re-verified locally with the
  system ABI bypassed: the compile line has `-I<prefix>/include`, the suite passes
  13/13, and `ldd` resolves `librofd_ffi.so.0` to `<prefix>/lib` (which is what
  makes the probe meaningful). Chained as another commit on `main`, and the
  follow-up run is **green**: [38810165744](https://github.com/YangtseSu/papers-ofd/actions/runs/38810165744)
  completed `success` in 2m29s, i.e. all three configure/build/test passes (the
  module against `-Drofd_libdir`, the page-size fallback, the installed
  `rofd-ffi.pc`) plus `reuse lint` now pass in the container — the CI half of R10
  is verified, not just mirrored locally.
- AUR publication (the user asked for the push and provided a proxy for the
  network): `f64a19c` was pushed to `ssh://aur@aur.archlinux.org/papers-ofd.git`
  (`c2f0387..f64a19c`, `master`), i.e. **`papers-ofd 51.1.1-2`** — a recipe-only
  revision (`pkgrel` 1 → 2 per D13), whose payload is unchanged and whose
  `checkdepends` now name the fonts. As with the earlier releases, the RPC index
  lags the git push; the authoritative evidence of the publication is the push ref
  above (`git ls-remote` shows it).

## 2026-10-16 (session C): phase 25 — documentation corrections (R19)

Docs-only phase; no module code changed. Every item was re-checked against the
files first, because phases 20–24 had already corrected some of them in passing;
what follows is the state per item and the new evidence.

### Already correct before this session (re-verified, not re-edited)

- **A16's budget claim** — corrected by phase 22: the row now states the disproved
  direction ("request sizes do **not** stay under rofd's budget") and names the
  fix. Re-checked against Papers: `MAX_IMAGE_SIZE 32767` (`libview/pps-view.c:5653`,
  clamped at `:5671-5672`) and `page-cache-size` default 200 MiB
  (`data/org.gnome.Papers.gschema.xml:25-29`) → ≈10.2× on A4 at 4 B/px ≈ 52.4 Mpx,
  against rofd's 256 MiB at ≈9 B/px ≈ 29.8 Mpx (≈7.7×), so "more than that
  default" holds.
- **§3's rotation row** ("empty region, never NULL") — R2's wording landed in phase
  20. The remaining looseness was `render_selection` ("draws nothing"): after R13
  it *drops* the surface it is handed, which is what the row now says
  (`src/ofd-text.c:416-424` sets `*surface = NULL`).
- **README's budget paragraph and its test-count/install rows** — phase 22/24.

### Corrections made (all in this phase's commit)

- `docs/plan.md` §2 file tree: dropped the non-existent `debian/`; added
  `scripts/dev-env.sh`, `patches/README.md`, `.github/workflows/`,
  `COPYING`/`REUSE.toml`/`LICENSES/`, `README.md`, and the Sushi plugin in the
  `data/` row.
- §3 selection/rotation row: "nothing drawn / no region" →
  "declined … `render_selection` drops the surface it is handed (R13)".
- §5 phase 0 acceptance: the deliberate discovery failure is now stated as what it
  is — `ofd-discovery-failure` calls `pps_init ()` and asserts
  `Failed to load backend` (`tests/test-ofd-document.c:153`), i.e. part of
  `meson test` since R6, not an observation.
- §5 phase-1 evidence: added the phase-23 re-measurement after the renderer moved
  into `load` (~130–140 ms shipping against ~115–120 ms fallback) and pointed at
  the README row for the current split.
- §6 fixture matrix: added the `strip` row (1000×25 mm page, 34016×851 at 12×,
  the surface refusal `test_failed_render_is_a_warning` asserts; A28, R15).
- §7: "exactly the five files" → **six** (the table lists six).
- `README.md`: "four small data files … into Papers' existing backend directory" →
  five data files, with only the module and the descriptor in the backend
  directory; the descriptor snippet's `TypeDescription` now matches
  `data/org.gnome.Papers.OfdDocument.papers-backend:3`
  (`Fixed-layout Documents`); the open-latency limitation now gives the numbers
  **per build**; the ABI-classifier note no longer implies `meson test` shows the
  line by default.

### Verified

- The `meson test` claim, before writing it down: `meson test -C _build ofd-text
  --print-errorlogs` printed no test stdout for the passing test; `meson test -C
  _build ofd-text -v` printed `first line: ABI layout 28.7 mm, ink 28.2 mm ->
  font-resolved geometry`, and the same line is in
  `_build/meson-logs/testlog.txt`. The README now names `-v`.
- Open latency re-measured twice (5 runs each, best of), which is the source of
  the README bands: shipping build `_build` best **139.6 / 138.0 ms** (load
  ~28 ms, size walk ~18–20 ms, `setup_cache` ~70 ms, first render ~20–21 ms);
  fallback `_build-slow` best **118.5 / 118.9 ms** (load ~28 ms, size walk
  ~70 ms, `setup_cache` 0.1 ms, first render ~20–21 ms) — the fallback matches
  phase 23's run (117.1 ms) almost exactly, while the shipping build measures
  ~8 % higher than phase 23's 129.3 ms in both of today's passes (the extra time
  sits in `load`, the size walk and `setup_cache`), so the README row states
  bands rather than a single number.
- Structure and cross-references (the phase-4 rule): both files' fences balanced
  (plan 18, README 8), no malformed table rows, all 13 relative markdown links
  (including anchors into `docs/plan.md`) resolve, the repository-relative paths
  named in both documents exist, and the edited citations were spot-checked
  (`libview/pps-view.c:4512` and `:925-931`; `tests/test-ofd-document.c:153`;
  `src/ofd-text.c:416-424`; the strip dimensions against
  `tests/test-ofd-render.c:370-380`).
- `reuse lint`: compliant, 57/57 files. `meson test -C _build`: **14/14** (docs
  only, run as the smoke check).

### Not done (next session)

- Phase 26: `51.1.2` — version bump, tag, GitHub release, AUR `papers-ofd`
  repackage, and the still-open `pacman -Rns papers-ofd` uninstall regression.
  User-gated.
- The two user steps from phase 24 stay open: the AUR `papers-ofd` revision
  (`checkdepends`, prepared uncommitted in the AUR working copy) and, if CI should
  run, a branch push.

## 2026-10-16 (session C): phase 27 — R17, R18 (user-requested)

Asked for by the user right after phase 25 ("if R17/R18 need doing, just do
them"), so they land before the 51.1.2 release. Both are latent defects the
review had recorded from code evidence only; each got the campaign's test-first
treatment.

### R17 — one `PpsLink` per hit region

`ofd_links_get_links` created one link object per *action* and gave every region
mapping a reference to it. The view resolves the mapping behind a link by pointer
identity — `pps_mapping_list_find` (`libdocument/pps-mapping-list.c:31-45`) is
called from `pps_view_get_area_from_mapping` (`libview/pps-view.c:1743`), which
feeds the tooltip tip area (`:3999`) and the link popover's pointing rect
(`:2263`) — so every region of a multi-region action anchored on the first one.

rofd can publish several regions for one action: each `Area` child of an explicit
`Region` becomes one rect of that entry (`$ROFD_SRC/crates/rofd-core/src/links.rs:197-236`,
`$ROFD_SRC/crates/rofd-core/src/navigation/region.rs:104-117`), and the PDF
backend builds one link per mapping (`pps-poppler.c:1358-1371`), which is the
model the module now follows.

Reproduced first: the `links` fixture gained `PathObject ID=74`
(`Boundary="40 220 60 20"`) whose action carries a `Region` with two `Area`
children (40–70 mm and 75–100 mm, both 220–230 mm), and `test_page_links` now
walks the oracle region by region, taking one mapping per region and asserting
`pps_mapping_list_find (links, pps_mapping_get_data (mapping)) == mapping` plus
distinct link pointers across the regions. Observed red with the fixture and the
test in place and the old code:

```
Bail out! ERROR:../tests/test-ofd-links.c:366:test_page_links:
  'pps_mapping_list_find (links, data) == region_mappings[r]' should be TRUE
```

(the second region's link resolved to the first mapping). Fix: `src/ofd-links.c`
creates the link inside the region loop (`link = pps_link_new (NULL, link_action)`)
and hands that reference to the mapping; the action reference is dropped once all
regions are built. Green afterwards, `ofd-links` also clean under
`_build-asan` (LeakSanitizer included), i.e. the per-mapping ownership is right.

### R18 — a `FitR` without a box is a page fit

`ofd_link_dest_from_rofd` forwarded any `FitR` to `pps_link_dest_new_fitr` with
all four coordinates. The ABI zeroes an absent coordinate and clears its `HAS_*`
bit (`$ROFD_SRC/crates/rofd-ffi/include/rofd.h:297-299`;
`$ROFD_SRC/crates/rofd-ffi/src/navigation.rs:146-160`), and `pps_link_dest_new_fitr`
hard-codes `change_left|change_top`, so a `FitR` without `Right`/`Bottom` reached
the view as a 0×0 target: `goto_fitr_dest` (`libview/pps-view.c:1816-1846`) asks
`zoom_for_size_fit_page` (`:6744-6756`) for `inf`, which
`pps_document_model_set_scale` (`libview/context/pps-document-model.c:476-484`)
clamps to `max_scale` — a jump to maximum zoom, reachable with
`allow-links-change-zoom` (default false, `data/org.gnome.Papers.gschema.xml:30-31`).

Reproduced first: the fixture's page-level actions gained a complete `FitR`
(30/40/120/200) and one without `Right`/`Bottom` (30/40), and `test_page_links`
asserts the ABI's own completeness rule — all four `HAS_*` bits and a positive
box stay `PPS_LINK_DEST_TYPE_FITR` with the four coordinates compared (in
millimetres and points), anything else becomes `PPS_LINK_DEST_TYPE_FIT`.
Observed red with the fixture and the test in place and the old code:

```
Bail out! ERROR:../tests/test-ofd-links.c:347:test_page_links: assertion failed
  (pps_link_dest_get_dest_type (dest) == PPS_LINK_DEST_TYPE_FIT): (5 == 2)
```

Fix: the `ROFD_DESTINATION_FIT_R` arm requires
`(flags & (HAS_LEFT|HAS_TOP|HAS_RIGHT|HAS_BOTTOM)) == all` and `right > left &&
bottom > top`; otherwise it logs the reason at `g_debug` and returns
`pps_link_dest_new_fit (page)`.

### Observed

- `meson test -C _build`: **14/14**; `meson test -C _build-slow`: **14/14**.
- `meson test -C _build-asan ofd-links`: OK (the new per-region allocation leaks
  nothing).
- `reuse lint`: compliant, 57/57 files.
- Live `papers` run on the refreshed dev backend directory with
  `G_MESSAGES_DEBUG=all` (`_build/tests/links-2pages.ofd`): the document opens,
  the links job builds page 0's destinations once, the new line
  `FitR destination without a complete box; fitting the page instead` appears
  exactly once, and the log has no CRITICAL/ERROR/WARNING (the only noise is the
  fixture's font-fallback diagnostics at `g_debug`).
- Docs in the same commit: plan §3 (`get_links`, destinations), §6's `links`
  fixture row (it had claimed a 4-node outline; the fixture has 6 nodes in 4
  top-level entries), A30 and A31.

### Not verified

- The tooltip's tip area and the link popover's rect are GTK-internal; the
  headless test pins the mapping-identity contract both consume, not their pixels.
  The live window was left open for a hover check on the fixture's two-region bar
  (left half vs right half of the 40–100 mm / 220–240 mm rectangle, page 1).

  **Done 2026-10-16**: the user ran that hover check on the live window and
  confirmed it, together with phase 20's optional pass; the automation boundary
  (no input injection available in this session, every route observed failing) is
  recorded in §"Item 2" at the end of this file.

### Pushed (2026-10-16)

The user approved the push; `main` went to `origin` (GitHub, the public
repository) as `17c5769..e65bb01` — a fast-forward carrying phase 25 and phase 27.
Privacy review of the outgoing content before pushing: both commit messages and
the full diff (8 files) scanned for secrets, tokens, keys, `.env` values, private
keys, machine paths and personal data — nothing found; the only user-identifying
strings are the repository's own URLs.

CI (`.github/workflows/checks.yml`, the Arch container) ran on the push and is
**green**: [run 38816196820](https://github.com/YangtseSu/papers-ofd/actions/runs/38816196820),
157.8 s, all steps — dependencies, the pinned fork ABI build, the module
configure/build/test, the page-size fallback build, the installed-`rofd-ffi.pc`
build, `reuse lint`. This also closes phase 24's open user step 2 ("a branch push,
if CI should run the new steps"): the two extra CI steps are now verified in the
real container on a commit that carries them, not only mirrored locally.

## 2026-10-16 (session C): phase 26 — release 51.1.2

### The release commit

- `61affd3` ("release 51.1.2: version bump and the docs that name the current
  release") bumps `meson.build` to `51.1.2` (D13: the fixes move the last
  component) and updates every tracked statement of the current release: the
  README status/install/version paragraphs and the `checkdepends` note (the fonts
  landed in `51.1.1-2`), plan D13 and §6's packaging block and §7's Arch-package
  section, and `AGENTS.md`'s version bullet.
- The test counts were made exact first, because a plain build shows fewer groups
  than the earlier "14/14" lines claimed: the suite has **13 groups** without
  `-Drofd_src` (the 14th, `ofd-oracle`, exists only when a rofd checkout with the
  two fixtures is configured). Observed on the release tree: `meson test -C _build`
  **13/13**, `meson test -C _build-slow` **13/13**, `meson test -C _build-oracle`
  (`-Drofd_src=$ROFD_SRC`) **14/14**, `reuse lint` compliant 57/57.
- The checkout here was 23 commits behind `origin/main` (session A/B/C work) and
  was fast-forwarded first; the push `edeb0a0..61affd3` triggered checks
  [38844929158](https://github.com/YangtseSu/papers-ofd/actions/runs/38844929158)
  — green, every step.

### The tag, and R20 in the release workflow

- Tag `v51.1.2` (annotated) pushed; its workflow run
  [38845166083](https://github.com/YangtseSu/papers-ofd/actions/runs/38845166083)
  came back **green in `checks` and red in `pin`** (R20): the `pin` job has no
  `actions/checkout` on purpose — it only needs the archive — so `gh` had no
  repository to infer and the step died with `failed to run git: fatal: not a git
  repository (or any of the parent directories): .git` right after computing the
  checksum. No release was created. This was the first run of the `pin` path
  (there is no `release`-workflow run for `v51.1.1`, whose release was created by
  hand), so nothing had exercised it.
- Fix in `release.yml`: every `gh` call names the repository
  (`-R "$GITHUB_REPOSITORY"`), with the reason inline, and the release title
  becomes `papers-ofd $tag` like the `v51.1.1` release. Verified by extracting the
  fixed step and running it **from a non-git directory** (the failing condition)
  with `GITHUB_REPOSITORY`/`GITHUB_REF_NAME` set: it created
  <https://github.com/YangtseSu/papers-ofd/releases/tag/v51.1.2> with the generated
  notes plus the pin block, printing `f79eb7798d3a940c18e540bd3dcc2a23178fd151bf2473300036eef0855d9558`.
- That checksum is independently reproducible: `curl -fsSL
  https://github.com/YangtseSu/papers-ofd/archive/refs/tags/v51.1.2.tar.gz |
  sha256sum` gives the same value (and the same method reproduces `v51.1.1`'s pin,
  `d9c94f4e…`), and the AUR build verifies it. The tag itself was not moved.

### AUR `papers-ofd 51.1.2-1`

- The AUR working copy here was still on the `0.1.0-6` commit pin while the remote
  was already at the tag-sourced `51.1.1-2` recipe; it was fast-forwarded to
  `f64a19c` first, so the release needed only `pkgver` 51.1.1 → 51.1.2, `pkgrel`
  2 → 1, the new `source=` URL and checksum, the version comment and a regenerated
  `.SRCINFO`.
- `makepkg -C -f --noconfirm` from a cleaned `src/`/`pkg/` is green: the anonymous
  download's checksum is verified, `check()` is **13/13** — `ofd-print-file` ran
  and passed (this session has a display; it skips itself without one) and
  `ofd-sushi-plugin` passed — and the package's file list is the same six paths as
  `51.1.1-1`. namcap reports the `gcc-libs` advisory for the package and the
  pre-existing "make dependency already included" note for the PKGBUILD (the
  runtime floor is in `depends`, the `>=0.4.3-6` check() floor in `makedepends`).
- Pushed `f64a19c..05b9e2d` to `ssh://aur@aur.archlinux.org/papers-ofd.git`;
  `git ls-remote` confirms `05b9e2d`, the RPC index still reports `51.1.1-2` (the
  usual lag).
- The pushed commit message claims `ofd-print-file` skipped itself; the test log
  shows it ran. The corrected message was **rejected** — the AUR hook declines a
  non-fast-forward push ("hook declined"), i.e. an AUR revision cannot be rewritten
  in place — so the clone was reset to the published commit and this log carries
  the accurate record. Bumping `pkgrel` for a message-only change is not justified.

### Still open (user)

- `pacman -Rns papers-ofd` — the phase-3 uninstall regression needs a password, and
  this machine still has `0.1.0-6` installed, so both installing `51.1.2-1` and the
  uninstall check after it are the user's step. **Done 2026-10-16, see below.**

### Install and uninstall acceptance (user, 2026-10-16)

- `pacman.log`: `upgraded papers-ofd (0.1.0-6 -> 51.1.2-1)` at 12:08:12, then
  `pacman -Rns papers-ofd`, whose transaction removed **exactly**
  `papers-ofd (51.1.2-1)` and completed with no other package in it — the phase-3
  uninstall regression, open since the first package: nothing outside our six paths
  is touched — and then `installed papers-ofd (51.1.2-1)` at 12:09:11.
- `pacman -Ql papers-ofd` lists the same six paths as the package build.
- From `/usr` with no overrides: `xdg-mime query filetype _build/tests/text-2pages.ofd`
  → `application/ofd`; `papers-thumbnailer -s 256` → PNG 181×256, exit 0;
  `papers-previewer --fd 3` opened and stayed up (killed by `timeout` after 8 s,
  exit 124) with zero CRITICAL/ERROR; `G_MESSAGES_DEBUG=all papers <fixture>` ran
  for 15 s and logged **931 `papers-ofd-DEBUG` lines** — all render diagnostics for
  the fixture's font fallback — with zero CRITICAL/ERROR, i.e. the installed module
  is what served the document; the same run on `_build/phase1-print-to-file.pdf`
  rendered through the PDF backend (a render job finished and disposed) with zero
  CRITICAL/ERROR, so the regression holds. The only warnings are this session's own
  GTK 4 `settings.ini` keys (`gtk-button-images`, `gtk-menu-images`, `gtk-modules`).
- Association change on this machine, worth keeping: `deepin-reader 6.6.2-2` is
  installed and its desktop file claims `application/ofd` as well, so
  `xdg-mime query default application/ofd` now answers `deepin-reader.desktop`
  (`~/.config/mimeapps.list` sets no default, and `gio mime application/ofd` lists
  only the visible competitor): our `NoDisplay=true` entry — deliberate, plan §7 —
  lost the default it had during the phase-3 acceptance, and double-click opens
  deepin-reader here. That is the documented cost of the `NoDisplay` route and one
  of the things phase 4 removes (Papers' own MIME list would carry the type with a
  visible entry); the choice is the user's —
  `xdg-mime default org.gnome.Papers.Ofd.desktop application/ofd` flips it back.

### The record commit's CI run: a mirror flake, and R21

- The record commit (`8d833a2`, `.planning/` and the two workflows only) went **red**
  in `checks` ([38845938669](https://github.com/YangtseSu/papers-ofd/actions/runs/38845938669)),
  but not on code: the dependency step died with
  `error: failed retrieving file 'djvulibre-3.5.30.1-1-x86_64.pkg.tar.zst' from fastly.mirror.pkgbuild.com :`
  (no reason given), then `too many errors from fastly.mirror.pkgbuild.com, skipping
  for the remainder of this transaction` and
  `failed to commit transaction (failed to retrieve some files)`. The same step was
  green in [38844929158](https://github.com/YangtseSu/papers-ofd/actions/runs/38844929158)
  two minutes earlier, with the same commit's parent tree, so this is the container
  mirror, not the module.
- R21: the step now retries the transaction up to three times, 20 s apart, clearing
  `/var/lib/pacman/db.lck` between attempts, and still fails when every attempt
  fails — a single mirror hiccup no longer decides a run. Verified by running the
  extracted step against a fake `pacman`: the transient case (fails twice) exits 0
  on the third attempt, the permanently broken case exits 1.
- The failed run was also re-run to show the flake passes on its own, and the
  hardened step is exercised by the push that carries it.

## 2026-10-16: plan audit (what is still open)

Requested by the user ("查看当前有没有未完成的plan"). Read-only apart from the
bookkeeping corrections listed at the end; nothing in `src/`, `tests/`, `data/` or
the workflows changed.

What the audit found, in the order of how much it matters:

1. **`docs/plan.md` phase 4 — the upstream MR — is not started.** It is the last
   planned module work and explicitly optional: move `src/*` into Papers'
   `libdocument/backend/ofd/`, add `application/ofd` to Papers' MIME list, an
   `ofd` entry to the `backends` option, and declare the ABI through
   `dependency('rofd-ffi', version: '>= 0.4')` (plan §5 phase 4, §9 item 5). It
   is also where the two upstream-report candidates go (A23: a `PpsSearchContext`
   cannot be finalized; A29: `pps_document_load` leaks the previous URI on
   reload), and it retires our `NoDisplay` association entry and the Sushi
   plugin.
2. **Unverified acceptance, GUI-only.** — *run by the user and confirmed 2026-10-16, see §"Item 2" below.* Phase 27's hover check on the `links`
   fixture's two-region bar (tooltip tip area and link-popover rect are
   GTK-internal, so the headless test pins the mapping identity instead), and
   phase 20's optional pass (double-click selects the word, triple-click the
   line, Ctrl+C copies it, PageDown after Ctrl+R). Both need a live window; both
   are recorded as open in `task_plan.md` (phase 27's own box stays as the phase
   status, the phase-20 box stays unchecked).
3. **The `release.yml` pin fix is verified outside CI only.** — *closed 2026-10-16, see §"Item 3" below: the fixed job ran green in CI.* The `v51.1.2` tag
   run stays red (`pin`: R20, no release created by the workflow) — the release
   was created by running the fixed step by hand; the next tag push is the first
   CI-verified run of the fixed job. Recorded as R20 in `findings.md` and in the
   phase-26 section above.
4. **The five upstream rofd PRs are still open** — *the sign sentence was posted on all five 2026-10-16 (§"Item 4"); the check stays red on an upstream infra defect, R22.* (`needs-ok-to-test`, red
   `clacheck / CLAssistant`, no approval): #11 page size, #12 `.pc`, #13 font
   advances, #14 decode budget, #15 region-AA tolerance. Nothing here blocks a
   release — the carried changes ship through the fork tag — but the CLA sentence
   on each PR is the user's, still open since the phase-1 handoff.
5. **Machine-local, documented, not a defect:** `deepin-reader` (installed here)
   holds the `application/ofd` default because our entry is `NoDisplay`;
   `xdg-mime default org.gnome.Papers.Ofd.desktop application/ofd` flips it back,
   and phase 4 makes it moot. A stale prunable worktree remains in the rofd
   checkout, which `git worktree prune` clears.

Bookkeeping corrections made (all in `task_plan.md`):

- Phase 17's optional `pacman -Rns` box is ticked — the user ran it on 2026-10-16
  (see §"Install and uninstall acceptance" above), it had been left unchecked.
- Phase 15's status line said "GUI acceptance open" while its own acceptance box
  records the user's 2026-10-15 confirmation; corrected to `complete`.
- `## Current Phase` still ended at "Next: phase 26"; it now records that phase 26
  closed and names the four open items above.
- The open CLA action names all five PRs instead of #11/#12.

Verification of this audit: `main` is `0458f96` = `origin/main` with a clean tree;
`gh run list` shows every `main` push green (last: `38849701753`, 2m10s) with only
the `v51.1.2` release run red (R20); the AUR `papers-ofd` working copy is clean at
`05b9e2d` = `origin/master`; `librofd-ffi 0.4.3.fork.2-1` and `papers-ofd 51.1.2-1`
are the installed pair; the fork checkout is clean and level with `origin/dev`
(`0 0`); `meson test -C _build` is 14/14 on this tree.

## 2026-10-16: the three remaining open items (2, 3, 4)

### Item 2 — the GUI-only acceptances, and why they had to be hands-on

Input injection was attempted first, so the boundary is on the record. This
machine is GNOME on Wayland (`XDG_SESSION_TYPE=wayland`, one 4608x2592 output at
scale 2), and every automation route failed, each with an observation:

- **XTEST** (python-xlib 0.33, already installed): `XWarpPointer` moves the X
  pointer inside XWayland (2462,1988 as asked) but the click never reaches the
  window — the page did not navigate; plain `XTestFakeMotionEvent` was a no-op
  (`query_pointer` returned the same position before and after). Mutter does not
  forward XTEST input to XWayland clients.
- **`/dev/uinput`** (the ydotool route; the node carries an ACL entry for the
  session's account and the open-for-write test passes): a virtual device is accepted by the kernel
  (`/proc/bus/input/devices` lists the probe's own device name, with the
  mouse/keyboard handlers) and udev tags it `ID_INPUT=1 ID_INPUT_MOUSE=1
  ID_INPUT_KEY=1`, but nothing arrives in the app: with the Papers window focused
  (X input focus on a child of its toplevel, checked before injecting) `Ctrl+R`
  left the window byte-identical (0 changed pixels), and relative and absolute
  motions alike left the X pointer where it was, even after three seconds of
  device-open delay.
- **Screenshots**: `org.gnome.Shell.Screenshot` answers
  `AccessDenied: Screenshot is not allowed`, so a Wayland output capture is
  unavailable; an X window of an XWayland client *is* capturable with
  `import -window <id>`, which is what the probes used.
- **Nothing else available**: no `xdotool`/`ydotool`/`grim`/`Xvfb`/`weston`/
  `Xephyr`, no passwordless `sudo`, `org.gnome.Shell.Eval` disabled
  (`(false, '')`), and the a11y bus exposes only `org.a11y.atspi.Registry`.
- The injected virtual devices were destroyed in the same run (`grep -c
  live-acceptance /proc/bus/input/devices` is 0 afterwards); the X windows the
  probes opened are the app's own.

The two checks were therefore run by the user, on the app's own windows, and
both confirmed on 2026-10-16. What they cover, so the confirmation is
reproducible from this file: the `links` fixture's four bars are at page y
100-120 / 140-160 / 180-200 / 220-240 mm, the fourth carrying one action with two
explicit `Region` areas (left half page x 40-70 mm, right half 75-100 mm); the
hover check is that each half's tooltip anchors over *that* half (the pre-R17
symptom was the second region anchoring on the first), that the second and third
bars show their `URI`s (the third falling back past the unresolved GOTO to page
99 — R11), and that either half of the fourth bar navigates to page 2. Phase
20's optional pass is the same window discipline on `text-2pages.ofd`:
double-click selects the word, triple-click the line, `Ctrl+C` copies it, and
`Ctrl+R` followed by `PageDown` neither crashes nor blanks the page.

### Item 4 — the CLA sentence on the five upstream PRs

Posted on 2026-10-16 with the string the bot itself asks for, taken from its own
workflow input (`custom-pr-sign-comment`, not invented):
`I have read the CLA Document and I hereby sign the CLA.` — on
[#11](https://github.com/linuxdeepin/rofd/pull/11#issuecomment-6217430802),
[#12](https://github.com/linuxdeepin/rofd/pull/12#issuecomment-6217431395),
[#13](https://github.com/linuxdeepin/rofd/pull/13#issuecomment-6217431955),
[#14](https://github.com/linuxdeepin/rofd/pull/14#issuecomment-6217432599) and
[#15](https://github.com/linuxdeepin/rofd/pull/15#issuecomment-6217433327).

It does **not** turn the check green, and that is an upstream defect, not a
missing signature: the comment-triggered re-run
([38914310384](https://github.com/linuxdeepin/rofd/actions/runs/38914310384))
fails exactly like the original one, with `Could not update the JSON file: Error
occured when creating a pull request comment: Resource not accessible by
integration` — the reusable `cla-check` workflow's GitHub App has no write access
in `linuxdeepin/rofd`, so it can neither post its comment nor persist a
signature. Recorded as R22 in `findings.md`; the PRs stay blocked on
`needs-ok-to-test` (an org member's approval) as well. Nothing here blocks a
release: the carried ABI ships through the fork tag, which is what the AUR
package builds.

### Item 3 — `release.yml`'s `pin` job, now CI-verified

A plain re-run cannot verify the fix, and that was established by observation
before building anything: `gh run rerun 38845166083 --failed` re-ran the job with
the workflow file **as of the tag's commit** (the log's script starts at
`tag="$GITHUB_REF_NAME"` with no `repo="$GITHUB_REPOSITORY"` line, i.e. R20's
code) and failed identically — `failed to run git: fatal: not a git repository`,
`pin` started 2026-10-16T13:51:36Z, conclusion `failure`. Re-running re-exercises
the old code, not the fix.

What does exercise the fix is a ref whose tree carries it, which is what the
workflow's own `workflow_dispatch` input is for ("Re-runs the release path for the
ref you pick"). Harness, 2026-10-16:

1. `git tag ci-release-pin-check 0458f96` — the commit that is `origin/main`, so
   the tag's tree carries the fixed workflow and nothing new is published — and
   pushed it. A non-`v` name, so the tag-push trigger does not fire and the
   dispatch is the only thing that runs.
2. `gh workflow run release.yml --ref ci-release-pin-check` →
   [38914235518](https://github.com/YangtseSu/papers-ofd/actions/runs/38914235518):
   **green in both jobs** — `checks` 14:15:07Z→14:17:26Z, `pin`
   14:17:29Z→14:17:36Z. The first CI run of the fixed job, and the only red left
   in the history is the original `v51.1.2` run, which stays red by design (the
   tag's tree predates the fix; a published tag is never moved, D13).
3. The `pin` job created the release for that tag and wrote the pin block into
   it: `sha256 14638114bb7a49cca61df900f6e8a2119b58b58c345eb0269d6ccdcb6a405d42`,
   byte-identical to an independent
   `curl -fsSL https://github.com/YangtseSu/papers-ofd/archive/refs/tags/ci-release-pin-check.tar.gz | sha256sum`
   — i.e. the job's placeholder-free path (new release, generated notes, pin
   appended) and its `-R "$GITHUB_REPOSITORY"` calls both work on the runner.
4. Cleanup: `gh release delete ci-release-pin-check --yes --cleanup-tag` and
   `git tag -d ci-release-pin-check`. Verified afterwards: `git ls-remote --tags
   origin` has no such ref, and the release list is `v51.1.2` (`latest`) and
   `v51.1.1` again.

This closes audit item 3: the fix is no longer "verified outside CI only".

## 2026-10-19: the four upstream merges, and the text-geometry request split in two

Requested by the user after they tested the fork's `librofd-ffi` against
`deepin-reader` (the other consumer of the same ABI): find out how deepin-reader
handles the horizontal geometry, and split the font-text response upstream so the
two bug fixes can land without the behaviour change riding along.

- **Upstream took four of the five requests.** All on 2026-10-19 (GitHub's
  `merged_at`, one commit each on `linuxdeepin/rofd` `main`, `140b936` →
  `2f5013d`): `16eaa8d` = #11 page-size query, `bef8a3e` = #14 decoder budget,
  `f6fb312` = #15 region-AA tolerance, `2f5013d` = #12 `rofd-ffi.pc`. Each
  upstream commit's patch text is byte-identical to our branch commit's (`diff
  <(git diff base ours) <(git diff theirs upstream)` empty for all four), so
  dropping our merges loses nothing. #13 stays open (`needs-ok-to-test`, no
  maintainer review yet; CLA signed 2026-10-16).
- **Fork `dev` rebased** onto `2f5013d`: the four merges dropped, the two
  loose-CI halves (the `--skip` flags and the two `continue-on-error` steps,
  which existed only for the two tests #14/#15 fixed) and the workflow's
  strict/loose ref distinction removed with them, `FORK.md` updated. `dev` is now
  upstream `main` + the fork's own three commits + **two** `--no-ff` merges, and
  `git log --merges origin/main..dev` lists exactly those two. No archive branch was
  made: the pre-rebase tip and the old three-commit branch are either upstream now or
  reproduced by the two new branches, and both stay reachable in this checkout's
  reflogs (`dev`, `feat/font-text-advances`) plus the detached `pr13-font-advances`
  worktree. The published tag `v0.4.3-fork.2` is untouched and stays what the AUR pins.
- **The split.** The third commit of the old branch bundled two different things:
  covering the glyphs below the baseline (a bug: the highlight cut the bottom off
  CJK overshoot and Latin descenders) and the *model* that does it (a band shared
  by every glyph of one face, plus the 1/8 selection margin — the poppler model
  Papers' PDF backend paints). Recut as two branches, each diff against upstream
  being the change alone:
  - `feat/font-text-advances` (`8db94f9`, `f0572f2`) — the horizontal half, i.e.
    PR #13 narrowed to "resolve missing text deltas through font advances" and
    "size a resolved glyph box by its advance": the provider seam plus the box
    width. No vertical geometry, no model change, no ABI surface change (the
    export set is untouched).
  - `feat/font-text-band` (`a5102c1`, `a57fd63`) — stacked on the above: the box
    spanning the face's typographic ascent and descent, and, as its own second
    commit, the selection margin. The margin is separable on purpose: a reviewer
    who wants the ink covered but not the extra growth can drop that one commit.
    The alternative model (each glyph's own painted control box — the shape
    released locally as `librofd-ffi 0.4.3-5`) is still recoverable from the
    branch's reflog and is documented in the PR body as the option not taken.
  - Review comments answered: Sourcery's "explicit advances still load fonts" is
    fixed in commit 1 (the provider is asked only when a run declares no
    `DeltaX`; #13's body and thread say so); its "font errors become silent
    fallback" is answered with the contract — `None` means "no resolved glyph",
    the conservative footprint is what a font-free build produces, and a font
    problem must not become a query error. In the band commit the call is
    unconditional again, with a comment saying why: the vertical extents come
    from the font whatever the deltas say.
  - PR-2 is **not open yet**: a GitHub PR cannot use a fork's branch as its base,
    so it would have to target `main` and show both changes at once. It opens on
    `main` (band + margin) once #13 lands; the branch is pushed and its body is
    drafted in `upstream-pr2-body-draft.md` (this directory). The upstream
    repository is unchanged in this respect by the API's own rules, not by choice.
- **deepin-reader 6.6.2, the other consumer** (`~/projects/ofd-reader/deepin-reader`,
  Arch recipe with `OFD_SUPPORT_ENABLED`): its OFD backend has **no** horizontal
  compensation at all, which is why our patch is harmless to it and why the
  difference the user saw is not a fix on their side:
  - one `Word` per scalar straight from `rofd_text_layout_get_char`
    (`reader/document/OfdModel.cpp:836-867`, `toPixels` at `:884-895` is a single
    uniform mm→px factor), one `QGraphicsItem` per word, painted 1 px inside its
    rect (`reader/browser/BrowserWord.cpp:45-48,62-65`) and hit-tested with the
    same items (`reader/browser/BrowserPage.cpp:1349-1363`);
  - search highlights are the library's match rects scaled, nothing else
    (`OfdModel.cpp:784-788`, `BrowserPage.cpp:259-265`); annotations merge those
    per-character rects into lines with a ±1 px tolerance (`BrowserPage.cpp:835-872`);
  - **no** `select()` / selection-region call anywhere — the 1/8 margin touches
    no code of theirs;
  - no `QFontMetrics`, no `adjusted()`, no snapping: nothing to disagree with the
    boxes we return. Because their highlight *is* the hit region, a drift reads
    as "the whole square is a bit off" rather than as a rectangle floating in
    blank space, and a full-width CJK advance equals the font size, so the
    font-size-floored box coincides with the character cell — the horizontal
    problem is invisible on CJK-heavy documents and on any document whose
    `TextCode` declares `DeltaX` (the font-size fallback never fires there).
    That is the answer to "how do they solve it": they do not, and nothing in
    their tree compensates.
- Verification on this machine (all at the recut tips, trees byte-identical to
  the previously verified ones): `cargo fmt --all -- --check`;
  `cargo clippy --offline -p rofd-core -p rofd-render -p rofd-ffi --all-targets
  -- -D warnings`; `cargo test` for the three crates (49 test binaries, 0
  failures); `crates/rofd-ffi/tests/run_c_tests.sh` rc=0; the fork CI's strict
  `dev` run green ([39223113934](https://github.com/YangtseSu/rofd/actions/runs/39223113934));
  papers-ofd `meson test -C _build` 13/13 against the rebuilt release library;
  `reuse lint` clean. Both reworded branches and `dev` were force-pushed to
  `YangtseSu/rofd` after a privacy sweep of the exact diffs (no paths, secrets or
  personal data; the diffs are source, tests and docs).
- Records refreshed for the new state: `patches/README.md` (two carried rows, a
  "retired from the carry" table with the four upstream commits, the loose-CI
  paragraph replaced), `docs/plan.md` (§9 items 1/5/6/7/8 statuses, the §9 policy
  intro, the §8 stock-rofd row, A18/A19 symbol counts, A24/A27 delivery), and
  `README.md` (dependency row, the "Text geometry follows the ABI" bullet).

