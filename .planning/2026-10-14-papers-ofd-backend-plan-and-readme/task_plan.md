# Task Plan: papers-ofd backend plan and README

## Goal

Deliver an execution-ready `docs/plan.md` and a matching `README.md` for the
papers-ofd OFD backend, grounded in verified Papers 51.1 / rofd ABI v1 behaviour.

## Next Step

**Phase 3 is complete and accepted** (2026-10-15): fd loading, the Arch package, its install checks
and — after the first acceptance run — the Sushi plugin that makes the space-bar
preview work out of tree. The user installed `0.1.0-2` and confirmed the session
items (space bar with the packaged plugin, Nautilus icon and thumbnail,
double-click, single app-grid entry); the temporary user-level plugin copy is
gone, and only the optional `pacman -Rns papers-ofd` uninstall regression is
left from the acceptance list. What remains after that is phase 4 (the upstream
MR into Papers' `libdocument/backend/ofd/`) is the only planned work left; it is
optional in the plan and can wait for a Papers-side slot.

Phase 4 also retires our `NoDisplay` association entry and the Sushi plugin:
Papers' own MIME list then carries `application/ofd`, so Sushi picks its document
renderer without help and `gio mime application/ofd` has a single Papers entry.

State to pick up from: `.planning/…/progress.md`, sections *Phase 3 — fd loading,
packaging, install checks* and *Phase 3 follow-up — the space bar* (evidence), plus
*Test Results* / *Error Log* rows.
Repository main is at the phase-3 commits plus the artefact-consolidation commit
(phase 18): the Arch recipes live in their AUR repositories (`papers-ofd 0.1.0-4`
published and installed on this machine, `librofd-ffi 0.4.3-6`) and the carried rofd
patch files live there too — this repository carries no PKGBUILD and no patch payload.

### Phase 3 handoff (2026-10-15)

*Executed in the same session: the three steps below were carried out; see
`progress.md` for the observed evidence and keep this section as the record of
the state phase 3 started from.*

State of the machine and the repository:

- Phases 0–2 of `docs/plan.md` are complete, committed and user-accepted; the
  branch is `main` with a clean tree.
- `librofd-ffi 0.4.3-4` is **installed** (page-size query + font-resolved text
  geometry, both from our fork branches). The module links it normally, no
  `LD_LIBRARY_PATH` needed. rofd PRs [#11](https://github.com/linuxdeepin/rofd/pull/11)
  and [#13](https://github.com/linuxdeepin/rofd/pull/13) are open upstream; the AUR
  push of `0.4.3-4` is deferred by the user (the PKGBUILD lives in
  `~/projects/librofd-ffi`, built and verified).
- `meson test` is 11/11 in `_build` (fast page-size path), `_build-slow`
  (`-Drofd_page_size=disabled`) and `_build-fork` (fork checkout; see §6 for the
  `PKG_CONFIG_LIBDIR`/`LD_LIBRARY_PATH` traps).
- Two `papers` windows may still be open from the acceptance run
  (`pkill -f "papers _build/tests/"` if not).
- Content-only MIME detection works (A22), which the previewer's fd path needs;
  `tests/test-ofd-errors.c` asserts it.

Phase-3 work, in order:

1. `load_fd`: `load_fd` is *guarded* upstream (A10) but the previewer cannot open
   OFD without it — the base wrapper answers `G_IO_ERROR_NOT_SUPPORTED` when the
   vfunc is NULL. Implement `dup (fd)` → `rofd_document_open ("/proc/self/fd/<dup>")`
   with the temp-copy fallback in `g_get_user_cache_dir()/papers-ofd/` (unlink at
   finalize) per plan §3; the previewer passes the fd plus a sniffed MIME type
   (`previewer/pps-previewer.c`, A13). Test: a fixture opened through
   `pps_document_load_fd` behaves like the URI path, and the temp file is cleaned up.
2. Packaging: **PKGBUILD for `papers-ofd`** is the route that fits this machine and
   the AUR presence we already have (`librofd-ffi`) — `debian/` follows only if a
   non-Arch consumer appears, like the plan allows for the pkg-config metadata.
   Ship module + descriptor + MIME xml + thumbnailer + association entry; depends
   on `librofd-ffi`, `papers>=51`, `papers<52`; install/uninstall checks are the
   plan's acceptance but note the global rule **never pull container images**: use
   `pacman -U` on this machine (ask the user), a `DESTDIR`/staged-prefix install, or
   the existing dev-backend recipe, and say which one was used.
3. Acceptance to observe: previewer (space bar in Nautilus) shows an OFD page;
   Nautilus icon + rendered thumbnail and `papers-thumbnailer -s 256 <file> <out>`
   standalone; double-click and `gio open` open Papers; app grid keeps a single
   Papers entry.

Phase 1 of `docs/plan.md` is complete and committed: rendering (rotation/zoom
matrix on pixels, origin normalization, byte-equivalence with a direct FFI
render), printing (cairo PDF and a real GTK print-to-file, both verified with
poppler), metadata (`get_info` + mask bits), the invoice oracle (mean channel
difference 3.28) and the open-latency table for both page-size paths; `meson test`
7/7 in both builds; the four GUI items were user-confirmed on the live session.
Evidence is in `progress.md`.

Open user action (not blocking): post the CLA sentence on upstream rofd PRs #11
and #12 when convenient.

## Current Phase

`docs/plan.md` phases 0–3 implemented, packaged and shipped; phase 3 awaits only the user's visual acceptance; phase 4 (upstream MR) is the remaining planned work

## Phases

### Phase 1: Restore state and consolidate research

- [x] Resolve the planning directory (`resolve-plan-dir.sh` → none, initialized `2026-10-14-papers-ofd-backend-plan-and-readme`)
- [x] Check `git status`/`git diff --stat` (previous draft touched `README.md`, `docs/plan.md`, uncommitted)
- [x] Record all verified Papers/rofd evidence in `findings.md`
- **Status:** complete

### Phase 2: Redo docs/plan.md

- [x] Restructure around goal → decisions → phases → verification → risks
- [x] Keep every claim traceable to `file:line` (appendix evidence index, ids A1–A17)
- [x] Each phase has deliverables, steps, acceptance criteria, and required evidence
- **Status:** complete

### Phase 3: Rewrite README.md

- [x] Status, capability matrix, install layout, build/run, limitations, roadmap
- [x] Cross-references point at sections that exist in `docs/plan.md` (anchors verified)
- **Status:** complete

### Phase 4: Verification

- [x] Markdown structure valid (14/6 fences balanced, no malformed table rows)
- [x] Every referenced path exists (13 relative paths resolved; `pkg-config --variable=backenddir` → `/usr/lib/papers/6/backends`)
- [x] Spot-check of 29 `file:line` citations; 3 failed and were corrected (`pps-page.h:27-34`, `pps-document.c:461-468`, `pps-document-find.c:27-34`)
- [x] README anchors resolve against plan headings (4/4)
- **Status:** complete

### Phase 5: Delivery

- [x] Summarize the rework in Chinese for the user, listing the corrections that matter
- **Status:** complete

### Phase 6: Follow-up decisions (association route, rofd page-size path, .planning policy)

- [x] Lock the desktop-association route to the `NoDisplay` entry; add the rejected alternative and its cost to plan §7
- [x] Add decision D10 + evidence A18 for the page-size fast path with configure-time detection; expand plan §9 item 1 with the change surface, the fork/carry strategy and the sequencing
- [x] Add phase-1 latency measurement and phase-3 app-grid acceptance to the plan
- [x] Add `.planning/` to `.gitignore` (working memory stays out of the deliverables)
- [x] Re-verify structure, anchors, paths, citations after the edits
- **Status:** complete

### Phase 7: rofd `rofd_document_get_page_size_mm` + hosting

- [x] Implement the ABI function in `../rofd` (header, `document.rs`, `lib.rs`, symbol list, README, design doc)
- [x] Add FFI tests (value parity, metadata-only on unloadable content, invalid args/indices/aliased outputs)
- [x] `cargo fmt --all -- --check`, `cargo clippy ... -D warnings`, `cargo test -p rofd-ffi` (3 new tests), `crates/rofd-ffi/tests/run_c_tests.sh` all green
- [x] Commit `7f6d24d` on `feat/ffi-page-size-query`, fork `YangtseSu/rofd`, push, PR <https://github.com/linuxdeepin/rofd/pull/11>
- [x] `patches/rofd-ffi-page-size.patch` (+ `patches/README.md`); verified with `git apply --check` in a detached worktree of `rofd` `main`
- [x] Private repo <https://github.com/YangtseSu/papers-ofd> created and pushed (`.planning/` included)
- **Status:** complete

### Phase 8: REUSE, AGENTS.md, PR triage, patched AUR package

- [x] REUSE compliance for this repository (`LICENSES/`, `REUSE.toml`, `reuse lint` clean)
- [x] Concise `AGENTS.md`
- [x] Triage the upstream PR auto-replies and CI (CLA + ok-to-test + OWNERS approval are the only blockers)
- [x] Found and documented an unrelated upstream defect: rofd's `REUSE.toml` does not parse
- [x] AUR `librofd-ffi` built with the PR patch (`0.4.0-2`), verified to export the new symbol; install left to the user
- [x] Clean rebuild (`makepkg -C -f`) from a wiped `src/`+`pkg/`: rc=0, package re-verified (78 symbols incl. `rofd_document_get_page_size_mm`)
- [x] Published to the AUR at the maintainer's request: commit `2391d47` pushed to `ssh://aur@aur.archlinux.org/librofd-ffi.git` (ref confirmed with `git ls-remote`)
- **Status:** complete

### Phase 13: fork pushes, upstream PR #12, AUR publication

- [x] `librofd-ffi 0.4.3-2` committed and pushed to the AUR (`master` = `846dc9a`; RPC reports `0.4.3-2`)
- [x] Fork verified in sync with upstream (`0 0` at `140b936`) and both feature branches present
- [x] `feat/pkgconfig` pushed to `YangtseSu/rofd`; upstream PR <https://github.com/linuxdeepin/rofd/pull/12> opened
- [x] Policy recorded in `docs/plan.md` §9 item 5 and `patches/README.md`: every rofd change lives on a fork branch, is pushed to our GitHub and proposed upstream
- **Status:** complete

### Phase 15: docs/plan.md phase 2 — text, selection, find, links, outline

- [x] `ofd-text.[ch]`: `PpsDocumentText` (five vfuncs), `PpsSelection` (three), `PpsDocumentFind` (two, 4096-result cap); `ofd-page.[ch]` gained `OfdPageRef`, `ofd-render.[ch]` the canvas-size helper, `ofd-units.[ch]` the pixel-rect mapping
- [x] `ofd-links.[ch]` (page `PpsMappingList` + `PpsLinkAction`/`PpsLinkDest` from a rofd action, URI base resolution) and `ofd-outline.[ch]` (`PpsOutlines` model rebuilt from parent/first_child/next_sibling)
- [x] Five interfaces registered on `OfdDocument`; every vfunc present, stubs where the ABI has no counterpart
- [x] Fixtures `text`, `scan`, `links`, `origin-text`, `broken-page`, `malformed-zip` in the generator (+ Meson targets)
- [x] Tests `test-ofd-text.c`, `test-ofd-links.c`, `test-ofd-search.c`, `test-ofd-errors.c`; 11/11 in both page-size builds
- [x] Headless Ctrl+F acceptance: Papers' own search context over the text fixture (snippets for ASCII/CJK/case-sensitive/whole-word), which also surfaced an upstream defect (A23: a `PpsSearchContext` cannot be finalized)
- [x] Two real defects found and fixed: `pps_outlines_new`/`pps_outlines_set_link` are declared but not exported (A21), and the MIME magic priority made content-only detection answer `application/zip` (A22, the previewer/fd path)
- [x] GUI acceptance on the live session (user-confirmed 2026-10-15, incl. the Ctrl+F highlight after phase 16)
- **Status:** implementation complete, GUI acceptance open

### Phase 17: docs/plan.md phase 3 — fd loading and packaging

- [x] `src/ofd-document.c`: `load_fd` (`dup` + `/proc/self/fd/<dup>`, streamed temp copy below `g_get_user_cache_dir()/papers-ofd`, dropped on reload and at finalize); `load` split into `load`/`load_path`
- [x] `tests/test-ofd-fd.c` (fd vs URI equivalence incl. pixels and info, descriptor ownership, unlinked file, temp-copy lifetime, base-class rejections, error parity) + meson wiring
- [x] `meson test` 12/12 in `_build`, `_build-slow` and `_build-fork`; G_SLICE/MALLOC_PERTURB run clean
- [x] `packaging/PKGBUILD` (Arch recipe: module + descriptor + mime xml + thumbnailer + `NoDisplay` association entry; `--buildtype=plain`; `check()` runs the suite); namcap clean apart from the `gcc-libs` advisory
- [x] Repository made public (the user's call) so the commit-pinned source is fetchable; anonymous download matches the checksum; build from an empty `SRCDIR` passes
- [x] AUR publication: `papers-ofd 0.1.0-1` (`master` = `86e4929`), mirror in `~/projects/papers-ofd-aur`
- [x] Install (user, `pacman -U`) verified: no file conflicts, designed `Depends On`, installed module md5 matches the package
- [x] Runtime from `/usr` with no overrides: `xdg-mime query filetype/default`, `papers-thumbnailer -s 256`, `papers-previewer --fd 3`, `papers <fixture>`, `gio open` — all with zero CRITICAL/ERROR
- [x] Docs: plan §1 (previewer row), §5 phase-3 steps/evidence, §6 (test list, packaging recipes, previewer invocation), §7 (Arch package section), §8 (fd fallback risk), Appendix A25; README status/capabilities/install/roadmap
- [x] Sushi plugin (`data/papers-ofd-sushi.js`, option `sushi_plugin_dir`) + `tests/test-sushi-plugin.js`: the first acceptance run showed the space bar falling back to the file-information panel because Sushi derives its document renderer's types from Papers' own desktop file (A26)
- [x] User installed `0.1.0-2` and confirmed the session items (space bar with the packaged plugin, Nautilus icon/thumbnail, double-click, single app-grid entry); the temporary user-level plugin copy is removed and the packaged file alone reproduces the behaviour
- [ ] Optional: `pacman -Rns papers-ofd` as the uninstall regression
- **Status:** complete and accepted

### Phase 16: rofd font-resolved text geometry (Ctrl+F highlight offset)

- [x] Diagnosed the reported offset as a rofd model mismatch (semantic layer: font size per scalar; renderer: font advance), measured against rendered ink (A24)
- [x] Implemented on the fork `feat/font-text-advances` (`6f98928`): `TextAdvanceProvider`, `Document::*_with_advances`, `PositionedGlyph::advance`, `FontTextAdvances`, FFI loads with the default font stack
- [x] Tests in all three rofd crates + C ABI runner green; `cargo fmt`/`clippy -D warnings` clean
- [x] Carried as `patches/rofd-ffi-font-advances.patch`; upstream PR <https://github.com/linuxdeepin/rofd/pull/13>
- [x] Module side: ink-alignment regression test that classifies the linked ABI and asserts match rectangles overlap the rendered glyphs; selection test samples glyph centres
- [x] AUR `librofd-ffi 0.4.3-3` built with both patches and verified (packaged library reproduces the font-aware geometry)
- [x] AUR publication deferred by the user: `0.4.3-3` stays local until rofd#13 is reviewed or merged
- [x] User re-checked the live session after the box-width commit: the highlight is correct; the installed package is `librofd-ffi 0.4.3-4`
- **Status:** complete

### Phase 0: loadable module skeleton

- [x] `meson.build`/`meson_options.txt`, `shared_module('ofddocument')`, `HAVE_ROFD_PAGE_SIZE` probe
- [x] `ofd-backend.c` (`pps_backend_query_type` + ABI check), `ofd-document.c` (load/n_pages/get_page/get_page_size/get_backend_info/save), `ofd-page.c`, `ofd-units.c`
- [x] `ofd-render.c` + `render` wired (needed by the acceptance: A20)
- [x] `data/` (descriptor, MIME xml, thumbnailer, association entry) with install rules
- [x] `tests/`: units, factory round-trip (3 pages, geometry), deliberate discovery failure
- [x] Acceptance observed: `meson test` 3/3, dev-dir run in Papers with no criticals, PDF regression clean
- **Status:** complete

### Phase 9: Portable dependency paths (machine move)

- [x] Add `scripts/dev-env.sh`: resolves/validates `PAPERS_SRC` + `ROFD_SRC`, exports the ABI pair, reports a same-named wrong repository
- [x] Replace the sibling-path assumptions in `docs/plan.md`, `README.md`, `AGENTS.md`, `patches/README.md` with `$PAPERS_SRC/…` / `$ROFD_SRC/…`
- [x] Fix the structurally wrong `../../rofd/…` link in the plan header; gitignore `local.conf`
- [x] Verify: all resolver modes, `reuse lint`, fence balance, sibling-path audit
- **Status:** complete

### Phase 14: docs/plan.md phase 1 — rendering, metadata, printing

- [x] `PpsDocumentPrint::print_page` wired to `ofd_render_page_onto` (dpi 72/scale 1/rotation 0) + `PpsDocumentPrint` registered on `OfdDocument`
- [x] `PAPERS_OFD_FALLBACK_FAMILIES` override in the renderer factory, `get_info`/`ofd-metadata.c` with the `fields_mask` bits, `n_pages` and mm page sizes
- [x] Fixtures: `corner-marker`, `origin-offset`, `metadata`, `latency [count]` kinds in `generate_fixture.py`; `rofd_src`/`rofd_page_size` meson options
- [x] Tests: rotation matrix, origin normalization, zoom matrix, same-options equivalence, print into a cairo PDF, GTK print-to-file with poppler text extraction, metadata fields/mask, invoice oracle, latency tool
- [x] Acceptance observed: `meson test` 7/7 in both page-size builds; oracle 2115×1400 mean 3.28; GTK print-to-file PDF text extractable by poppler and `pdftotext`; Papers session run with 0 criticals; latency recorded in README limitations
- [x] GUI acceptance observed on the live session (user-confirmed: rotation corners, zoom/fit, print preview + *Print to File* selectable text, properties dialog rows)
- [x] Fixed the MIME magic for the real invoice layout (`Doc_0/`@30) — found by the oracle run
- [x] Docs updated: plan §4 (uniform scale), §5 (phase-1 steps/evidence), §6 (setup command, fixture matrix), §7 (magic); README (status, build, latency limitation, roadmap)
- **Status:** complete

### Phase 18: one home per artefact (AUR recipes and patch payloads)

- [x] User decision applied: each artefact gets exactly one home — the Arch recipes live in their AUR repositories only, the carried rofd changes in the fork's `feat/…` branches, and their patch files in the AUR `librofd-ffi` repository; this repository keeps neither payload
- [x] Removed `packaging/PKGBUILD` (byte-identical to the AUR copy), `patches/rofd-ffi-page-size.patch` and `patches/rofd-ffi-font-advances.patch` (sha256-identical to the `librofd-ffi` files) and the then-unused `LICENSES/LGPL-2.1-or-later.txt`
- [x] `patches/README.md` rewritten as the inventory: fork branch and commit, upstream PR, AUR patch file, retirement criterion, `feat/aur-carried` regeneration, sync routine
- [x] Retargeted AGENTS.md (layout row, fork-first rule, new "one home per recipe" rule), README.md (`0.1.0-4`), docs/plan.md (D11, phase 3, §6 recipe now clones the AUR repository, §7, §8, §9, A24/A27), `scripts/dev-env.sh`, `tests/test-ofd-text.c`
- [x] Verified: `reuse lint` compliant, `meson test -C _build` 12/12, no dangling `packaging/` or patch-payload references outside `.planning` history
- **Status:** complete

## Key Questions

1. Does rofd's ABI allow concurrent read-only calls? → **Yes** (`rofd.h` header, `rofd-ffi/README.md:329-335`, internal locks in rofd-core/rofd-render).
2. Do text/link rectangles share the page-box origin? → **Yes, absolute**; the renderer re-bases the box origin onto canvas (0,0), so the backend must subtract the origin (`rofd-core/tests/links.rs:11-70`, `cairo_renderer.rs:1190-1215`).
3. Does Papers 51 expose an outline path? → **Yes**, `PpsDocumentLinks::get_links_model` feeds `shell/src/sidebar_links.rs`.
4. Is `PpsDocumentPrint`/`PpsSelection` optional? → **No in practice**: both wrappers are dispatched without NULL checks.
5. Can `get_thumbnail*`/`get_page_label` be skipped? → **Yes**, base-class fallbacks exist.
6. Is a cheap page-size path available for `setup_cache`? → **No in the ABI**; `Document::page_size()` exists in rofd-core but is unexported → upstream request.

## Decisions Made

| Decision | Rationale |
|----------|-----------|
| Plan is phase-executable (status, deliverables, acceptance, evidence) instead of a review narrative | The repository's plan must be followable by the next agent; findings/evidence live in an appendix and in `.planning/*/findings.md` |
| No document-wide render lock | ABI permits concurrency; keep a small mutex only for our own state |
| Print/selection in scope for v1; thumbnails/page labels out of scope | Mandatory-vs-optional split by upstream NULL-check behaviour |
| Origin-normalizing rect helper as the single conversion point | Correct for non-zero `PhysicalBox` origins; testable in isolation |
| Render scale derived from `compute_scaled_size` with `dpi=72` | Handles `scale == 0` + target size; keeps Papers' points→pixels semantics |
| Desktop integration: own MIME xml + own thumbnailer + `NoDisplay` association entry | Papers builds MIME lists from in-tree backends only; avoids patching system files |
| Dependency checkouts are located through `scripts/dev-env.sh`, never by assuming a sibling directory | The docs were written on a machine where `../papers`/`../rofd` existed; on this one `../rofd` is `office-rs/rofd`, a different OFD project with the same name, so hardcoded relative paths fail — or worse, resolve to the wrong tree. Environment → `local.conf` → sibling default, with marker validation, serves both machines and the installed package |
| Papers citations are written against upstream tag `51.1`; a local Papers checkout is optional | The module builds against the installed `papers-document-4.0` headers (43 libdocument + 20 libview shipped) and every reader can open `…/papers/-/blob/51.1/<path>`; only grepping and the phase-4 MR want a clone. rofd citations keep `$ROFD_SRC`, because the carried patch and the cross-repo tests run against that checkout |
| The AUR `librofd-ffi` pins an upstream commit when upstream publishes no tag, adds `rofd-ffi.pc`, and applies its own patch files | 0.4.3 exists only as `140b936` on `main`, so a tag-based source cannot be used; a commit-pinned archive keeps the sha256 check meaningful and the vendored crates make the build fully offline. Pinning also makes the "which ABI is in the field" question answerable, and the `.pc` lets consumers (papers-ofd now, Papers after phase 4) use `dependency('rofd-ffi')` |

## Errors Encountered

| Error | Attempt | Resolution |
|-------|---------|------------|
| `read` returned elided C bodies for `pps-render-context.c` | 1 | Re-read the exact ranges through `sed -n` |
| Failed in-place edit assertion while renumbering plan sections (text mismatch) | 1 | Re-grepped the exact line text, then re-ran the substitutions; script writes only after all assertions pass |
| Machine move: the sibling paths the plan assumed no longer exist (`../papers` missing, `../rofd` = `office-rs/rofd`) | 1 | Added `scripts/dev-env.sh` (env → `local.conf` → sibling default, marker-validated) and rewrote every doc path to `$PAPERS_SRC/…` / `$ROFD_SRC/…`; the build itself uses the installed `librofd-ffi` |

## Notes

- Deliverables: `docs/plan.md`, `README.md`. Working memory: this directory.
- Re-read this file before each phase; update `**Status:**` and `## Next Step` on every change.
