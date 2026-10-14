# Task Plan: papers-ofd backend plan and README

## Goal

Deliver an execution-ready `docs/plan.md` and a matching `README.md` for the
papers-ofd OFD backend, grounded in verified Papers 51.1 / rofd ABI v1 behaviour.

## Next Step

Post the CLA signature sentence on PR #11 (user action; `librofd-ffi 0.4.0-2` is
already installed on this machine, verified through `/usr/include/rofd.h` +
`/usr/lib/librofd_ffi.so.0` by `scripts/dev-env.sh`); then start phase 0 of
`docs/plan.md`: the Meson project plus the `OfdDocument` skeleton (works against
stock rofd through the page-size fallback, so upstream review of PR #11 does not
block it).

## Current Phase

All phases complete

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

### Phase 9: Portable dependency paths (machine move)

- [x] Add `scripts/dev-env.sh`: resolves/validates `PAPERS_SRC` + `ROFD_SRC`, exports the ABI pair, reports a same-named wrong repository
- [x] Replace the sibling-path assumptions in `docs/plan.md`, `README.md`, `AGENTS.md`, `patches/README.md` with `$PAPERS_SRC/…` / `$ROFD_SRC/…`
- [x] Fix the structurally wrong `../../rofd/…` link in the plan header; gitignore `local.conf`
- [x] Verify: all resolver modes, `reuse lint`, fence balance, sibling-path audit
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
| Dependency checkouts are located through `scripts/dev-env.sh`, never by assuming a sibling directory | The docs were written on a machine where `../papers`/`../rofd` existed; on this one `../rofd` is `office-rs/rofd`, an unrelated project of the same name, so hardcoded relative paths fail — or worse, resolve to the wrong tree. Environment → `local.conf` → sibling default, with marker validation, serves both machines and the installed package |
| Papers citations are written against upstream tag `51.1`; a local Papers checkout is optional | The module builds against the installed `papers-document-4.0` headers (43 libdocument + 20 libview shipped) and every reader can open `…/papers/-/blob/51.1/<path>`; only grepping and the phase-4 MR want a clone. rofd citations keep `$ROFD_SRC`, because the carried patch and the cross-repo tests run against that checkout |
| The AUR `librofd-ffi` pins an upstream commit when upstream publishes no tag, adds `rofd-ffi.pc`, and applies `patches/*.patch` | 0.4.3 exists only as `140b936` on `main`, so a tag-based source cannot be used; a commit-pinned archive keeps the sha256 check meaningful and the vendored crates make the build fully offline. Pinning also makes the "which ABI is in the field" question answerable, and the `.pc` lets consumers (papers-ofd now, Papers after phase 4) use `dependency('rofd-ffi')` |

## Errors Encountered

| Error | Attempt | Resolution |
|-------|---------|------------|
| `read` returned elided C bodies for `pps-render-context.c` | 1 | Re-read the exact ranges through `sed -n` |
| Failed in-place edit assertion while renumbering plan sections (text mismatch) | 1 | Re-grepped the exact line text, then re-ran the substitutions; script writes only after all assertions pass |
| Machine move: the sibling paths the plan assumed no longer exist (`../papers` missing, `../rofd` = `office-rs/rofd`) | 1 | Added `scripts/dev-env.sh` (env → `local.conf` → sibling default, marker-validated) and rewrote every doc path to `$PAPERS_SRC/…` / `$ROFD_SRC/…`; the build itself uses the installed `librofd-ffi` |

## Notes

- Deliverables: `docs/plan.md`, `README.md`. Working memory: this directory.
- Re-read this file before each phase; update `**Status:**` and `## Next Step` on every change.
