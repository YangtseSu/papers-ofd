# Task Plan: papers-ofd backend plan and README

## Goal

Deliver an execution-ready `docs/plan.md` and a matching `README.md` for the
papers-ofd OFD backend, grounded in verified Papers 51.1 / rofd ABI v1 behaviour.

## Next Step

**Phase 3 starts in a new session** (the user's call, 2026-10-15). The paste-ready
hand-off prompt is the block below; the detailed state is in *Phase 3 handoff*.

```text
在 ~/projects/papers-ofd 开始 docs/plan.md §5 的 phase 3：fd 加载（预览器）、打包、安装检查。

先按序读：AGENTS.md、docs/plan.md（§3 的 load_fd 行、§5 phase 3、§6 playbook、§7 打包）、
.planning/2026-10-14-papers-ofd-backend-plan-and-readme/{task_plan.md,progress.md}；
task_plan.md 的 "Phase 3 handoff" 一节是全部现状，本提示词只列要点。

现状（已核对，不用重查）
- phases 0–2 完成、提交、用户实机验收；工作树干净，origin/main == 本地。
- 已安装 librofd-ffi 0.4.3-4（含 rofd_document_get_page_size_mm 与字体 advance 几何），
  模块直接链接 /usr/lib 下的它，无需 LD_LIBRARY_PATH。
- meson test：_build / _build-slow / _build-fork 各 11/11；reuse lint 合规。
- rofd fork YangtseSu/rofd：main=140b936、feat/ffi-page-size-query=7f6d24d、
  feat/pkgconfig=137442f、feat/font-text-advances=812073e、
  feat/aur-carried=21dbf8f（= main + patches/*.patch，即 AUR 包构建的树）。
  上游 PR #11/#12/#13 均 open，不阻塞。AUR librofd-ffi 0.4.3-4 已发布（afab2b7）。
- 我们的 PKGBUILD（papers-ofd 本体）还没做。

本阶段三步
1. load_fd：dup(fd) → rofd_document_open("/proc/self/fd/<dup>")；失败回退到
   g_get_user_cache_dir()/papers-ofd/ 临时副本，finalize 时 unlink。注意 load_fd 上游有
   NULL 保护（返回 G_IO_ERROR_NOT_SUPPORTED），所以预览器现在打不开 OFD —— 这正是要补的。
   测试：经 pps_document_load_fd 打开夹具与 URI 路径行为一致、临时文件被清理、坏 fd 报错。
2. 打包：先做 papers-ofd 的 PKGBUILD（本机 Arch、AUR 已有我们的 librofd-ffi）；装 module +
   descriptor + MIME xml + thumbnailer + 关联 desktop 文件；depends: librofd-ffi、
   papers>=51、papers<52。debian/ 只在出现非 Arch 消费者时再加。
3. 安装检查（plan §5 的验收）：预览器（Nautilus 空格）显示 OFD 页；
   papers-thumbnailer -s 256 <file> <out> 与 Nautilus 缩略图；双击 / gio open 打开 Papers；
   应用网格仍只有一个 Papers 条目。全局规则禁止拉容器镜像：用本机 pacman -U（先问用户）、
   DESTDIR/暂存前缀安装或现有 dev-backend 方案，并在文档里写明用了哪种。

已知坑（plan §6 与 progress.md 有完整记录）
- 依赖定位：. scripts/dev-env.sh。
- 对 checkout 构建时 rofd-ffi.pc 优先于 -Drofd_libdir：需 PKG_CONFIG_LIBDIR 指向去掉
  rofd-ffi.pc 的 .pc 目录，运行时再给 LD_LIBRARY_PATH。
- 测试的 argv 相对路径按构建目录解析（传 src/libofddocument.so，不是 ../src/...）。
- dev backend 目录要整体复制系统目录（只放 descriptor 会破坏 PDF/DjVu/TIFF）；
  application/ofd 需要暂存 XDG_DATA_DIRS + update-mime-database。
- 纯内容 MIME 探测（预览器 fd 路径依赖）靠 magic priority 70（A22），
  tests/test-ofd-errors.c 有断言；几何类断言需要 librofd-ffi >= 0.4.3-4（A24）。
- A23：PpsSearchContext 无法析构（上游 bug），ofd-search 测试故意泄漏 context。
- 需要人眼的验收项按 phase 1/2 的做法交给用户实机确认（本机没有可用的 headless GUI）。

完成标准
- 每项验收都"观察到"并写进 .planning/.../progress.md（命令 + 输出片段）；同步更新
  plan §5 phase 3 evidence、§6/§7、README 的状态/能力/限制、task_plan 的 Status 与 Next Step。
- 新文件带 REUSE 头（reuse lint 必须过）；提交信息英文祈使句、正文列验证命令。
- 结束前跑 meson test（_build、_build-slow，必要时 _build-fork）与 reuse lint，并
  git push origin main。
```

### Phase 3 handoff (2026-10-15)

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

`docs/plan.md` phases 0–2 implemented (phase 2 awaiting the GUI acceptance); phase 3 next

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
