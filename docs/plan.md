# papers-ofd — Implementation Plan

OFD (GB/T 33190-2016) backend for GNOME Papers, built as an out-of-tree GObject
module that consumes the `rofd-ffi` C ABI.

Decision rationale: see the repository analysis summary. Short version — rofd's
C ABI already covers every PpsDocument vfunc and the optional Text/Find/Links
interfaces, and Papers discovers backends at runtime from a directory, so this
project can be developed, packaged, and used without forking Papers.

## Goal

- `papers file.ofd` opens, scrolls, zooms, rotates, prints, and thumbnails OFD
  pages, with text selection and in-page find.
- Ships as a standalone package that installs one shared module plus one desktop
  file into Papers' backends directory. No Papers fork, no Papers build.
- Later: upstreamable to GNOME/papers with zero glue-code changes.

## Non-goals (phase 1)

- Annotations, signatures/seals, form filling, attachments: rofd does not parse
  or render these yet (`crates/rofd-render/README.md`). Papers degrades silently
  when interfaces are unimplemented.
- Editing/saving OFD files.
- A standalone GTK/gpui viewer (separate possible project, reuses the same ABI).

## Project layout

```
papers-ofd/
├── meson.build
├── src/
│   ├── ofd-document.c        OfdDocument GType (PpsDocument subclass)
│   ├── ofd-document.h
│   ├── ofd-render-context.c  PpsRenderContext -> rofd_render_options_t mapping
│   └── pps-backend-query.c   exported pps_backend_query_type()
├── data/
│   ├── ofd.xml               shared-mime-info definition for application/ofd
│   ├── org.gnome.Papers.OfdDocument.papers-backend.desktop.in
│   └── papers-ofd.metainfo.xml.in (optional, later)
├── debian/                   later; optional for phase 1
├── docs/plan.md
├── COPYING                   GPL-2.0-or-later
└── README.md
```

Installed artifacts:

- `<libdir>/papers/6/backends/libofddocument.so`  (module name per GNOME
  convention: `Module=ofddocument`; discoverable via
  `pps-document-factory.c` desktop-file scan, `pkg-config --variable=backenddir
  papers-document-4.0`)
- same directory: `org.gnome.Papers.OfdDocument.papers-backend` desktop file
  with `MimeType=application/ofd;`
- `/usr/share/mime/packages/ofd.xml` (glob `*.ofd` + zip local-header magic)

## Dependencies

| Dep | Source | Notes |
| --- | --- | --- |
| `papers-document-4.0` | system pkg-config (>= 51, dev host has 51.1) | `-lppsdocument-4.0`, headers under `papers/4.0` |
| `rofd-ffi` | local build of `rofd/` (`cargo build -p rofd-ffi --release`) | `librofd_ffi.so` + `rofd/crates/rofd-ffi/include/rofd.h`; no pkg-config file yet, find via meson option/env |

Build-time includes: meson >= 0.60, pkg-config, glib/gio/gtk4 headers (via
papers-document-4.0), cairo (via papers headers), gtk4 (papers-document Requires
gtk4).

## ABI mapping (from `rofd.h`)

| PpsDocument vfunc | rofd-ffi |
| --- | --- |
| `load` | `rofd_document_open` (path from URI via `g_file_get_path`/`g_filename_from_uri`) |
| `get_n_pages` | `rofd_document_get_page_count` |
| `get_page` | `pps_page_new_with_index` (index boxed) |
| `get_page_size` | `rofd_page_get_size_mm` (mm -> points: mm * 72 / 25.4) |
| `get_page_label` | `g_strdup_printf("%d", index + 1)` — OFD has no page labels |
| `render` | `rofd_renderer_render_page_cairo`, surface sized to `rc->target_width/height`, `dpi=96`, `scale=rc->scale`, `rotation_degrees=rc->rotation` |
| `get_thumbnail` / `get_thumbnail_surface` | same renderer, small target from `rc` |
| `get_info` | `rofd_document_get_metadata` -> title/author/subject/keywords/creator/dates |
| optional `PpsDocumentLinks` | `rofd_page_get_links`, `rofd_link_list_get_region`, `..._get_action_destination` |
| optional `PpsDocumentText` | `rofd_page_get_text` (whole page), `..._get_text_for_area`, `..._get_text_layout` (per-char rects for selection) |
| optional `PpsDocumentFind` | `rofd_page_find_text_with_options` |

Renderer setup: one `rofd_renderer_t` per document (owned by OfdDocument),
created with `rofd_renderer_options_t.fallback_families` listing CJK families
("Noto Sans CJK SC", ...) so Chinese invoices/e-books render like rofd's phase-3
reference fixture. Each render call borrows the live `rofd_document_t` and a
freshly fetched `rofd_page_t`; ABI forbids conflicting concurrent access, so
serialize renders per document with a `GRecMutex` (Papers renders in worker
jobs, off the main thread).

## Phases

### Phase 0 — loadable skeleton (day 1)

1. meson project, `src/ofd-document.c` with `G_DEFINE_TYPE` subclass of
   `PpsDocument`, `pps_backend_query_type()` returning `OFD_TYPE_DOCUMENT`.
2. vfuncs: `load`, `get_n_pages`, `get_page`, `get_page_size`, `get_page_label`,
   `get_backend_info`.
3. Desktop file + install into `$(pkg-config --variable=backenddir ...)`.
4. MIME definition `data/ofd.xml` (see below).

Acceptance: `PPS_BACKENDS_DIR=_build papers <file>.ofd` opens the document,
scrolls all pages, page counter shows N. `papers --version` unaffected; PDF
still loads (no regression in shared backends dir).

### Phase 1 — rendering, info, thumbnails (day 2)

5. `render` vfunc with rotation/scale/target-size mapping; verify against
   `rofd/learning/test.ofd` and a real invoice fixture at 96 dpi, 1.5x, 2x, all
   four rotations.
6. `get_thumbnail` / `get_thumbnail_surface` for the sidebar and file manager.
7. `get_info` from metadata; backend info ("rofd" + `rofd_library_version()`,
   ABI `rofd_abi_version()` check at load: refuse load if major mismatch).
8. Render diagnostics (`rofd_render_report_get_diagnostic`) -> `g_warning`
   during development only; never fail rendering on diagnostics.

Acceptance: pages pixel-compare against `cargo run -p rofd --features qt-reader`
preview and rofd-render's `real_fixture` reference PNG (same DPI/scale);
thumbnailer produces thumbnails: `papers-thumbnailer <file>.ofd <out>.png`.

### Phase 2 — optional interfaces (day 3)

9. `PpsDocumentText`: `get_text`, `get_text_in_area`, `get_text_layout` ->
   `TRUE` (enables Papers' selection machinery over per-char rects);
   `get_text_mapping`/`get_text_attrs` left to defaults.
10. `PpsDocumentFind`: `pps_document_find_find_text` via
    `rofd_page_find_text_with_options` (case-insensitive default, map
    `PpsFindOptions` flags to `rofd_find_options_t.flags`).
11. `PpsDocumentLinks`: per-page links with regions and Goto/URI actions.

Acceptance: select+copy text into clipboard matches `rofd_page_get_text`
canonical order; Ctrl+F finds and highlights a CJK string; clicking an internal
link jumps, external link opens in browser.

### Phase 3 — packaging & distribution (day 4)

12. `debian/` (or PKGBUILD alternative): `papers-ofd` package installing the
    module, desktop file, mime xml; `librofd-ffi` (or vendored rofd) as
    dependency; `update-mime-database`, `update-desktop-database` triggers.
13. Double-click handling: append `application/ofd;` to the installed
    `org.gnome.Papers.desktop` MimeType list (Papers generates it from in-tree
    backends only, `meson.build:322`), then
    `xdg-mime default org.gnome.Papers.desktop application/ofd`.
14. `load_fd` via temp-copy fallback (see "Confirmed load paths") so the GNOME
    previewer works; unlink on document finalize.

Acceptance: fresh container/VM install of the deb + rofd lib -> file manager
shows OFD icon, double-click opens Papers, thumbnails appear in Nautilus.

### Phase 4 — upstream MR (optional, after phase 3 is stable)

14. Move `src/ofd-document.c` into `papers/libdocument/backend/ofd/` with meson
    entry + `ofd` added to the `backends` option; rofd becomes a Papers meson
    subproject/wrap dependency. MR to GNOME/papers.

Acceptance: MR green on GNOME CI, OFD opens in nightly Papers unmodified.

## Confirmed load paths (Papers 51, verified in tree)

- **Papers shell**: `shell/src/window.rs:691,867` — `JobLoad::set_uri(uri)` →
  `load` vfunc. Our path-based `load` covers the main app.
- **Thumbnailer**: `thumbnailer/src/main.rs:86-92` —
  `factory_get_document(uri)` + `Document::load(uri)`. Also path-based;
  thumbnails work with `load`.
- **Previewer**: `previewer/pps-previewer.c:103` — `pps_job_load_take_fd` →
  `load_fd` vfunc (fd only). rofd exposes no fd entry point
  (`rofd_document_open` takes a path), so space-preview of OFD fails unless we
  either (b) or (a):
  1. implement `load_fd` by streaming the fd into a temp copy under
     `g_get_user_cache_dir()/papers-ofd/`, opening that by path, keeping the copy
     for the document lifetime and unlinking at finalize; or
  2. add `rofd_document_open_fd` to rofd-ffi (needs rofd upstream change).
  Decision: do (a) in phase 3, guarded by an env/opt-out; it also future-proofs
  sandboxed open flows.
- **Bookmarks/outline**: not applicable. Papers 51 exposes no outline vfunc on
  `PpsDocument`; `pps-outlines.h` is a UI helper with no consumers in shell, and
  the shell has no bookmarks sidebar. `rofd_document_get_outline` therefore has
  no Papers consumer to wire — drop from scope.

## Remaining open questions

- **Rotation semantics**: `PpsRenderContext.rotation` is 0/90/180/270 — matches
  rofd's `rotation_degrees`; confirm sign/direction against a rotated fixture.
- **CJK fallback config**: whether to hardcode fallback families or read from
  GSettings font config; start hardcoded, move to settings later.

## Risks

| Risk | Mitigation |
| --- | --- |
| `application/ofd` absent from shared-mime-info (confirmed absent on dev host) | ship `data/ofd.xml`: glob `*.ofd` plus zip magic (`PK\x03\x04` at 0, `OFD.xml` at offset 30) |
| Font fallback wrong on minimal systems | explicit fallback family list; log `rofd_document_get_warnings` in dev builds |
| Papers renders on worker threads; ABI forbids concurrent access | per-document `GRecMutex` around page fetch + render |
| rofd ABI evolution | check `rofd_abi_version()` major at module load; fail with clear message; pin rofd version in packaging |
| GPL/LGPL mix | backend GPL-2.0-or-later (matches Papers); rofd is LGPL-2.1+ so linking is compatible |

## Verification (repeat at each phase)

```
meson setup _build -Dbackends_dir=$(pkg-config --variable=backenddir papers-document-4.0)
meson compile -C _build
PPS_BACKENDS_DIR=_build/src papers rofd/learning/test.ofd   # visual check
papers-thumbnailer rofd/learning/test.ofd /tmp/ofd.png
# regression: PDF backend unaffected
papers <some.pdf>
```

Plus: `cargo test -p rofd-ffi` and `crates/rofd-ffi/tests/run_c_tests.sh` in the
rofd tree whenever the ABI dependency is bumped.
