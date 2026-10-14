# papers-ofd — Implementation Plan

OFD (GB/T 33190-2016) backend for GNOME Papers, shipped out-of-tree as a GObject
module on top of the `rofd-ffi` C ABI (`$ROFD_SRC/crates/rofd-ffi/include/rofd.h`).

- Basis: **Papers 51.1** (tag `51.1` of <https://gitlab.gnome.org/GNOME/papers>;
  installed package `papers 51.1-1.1`, headers through `papers-document-4.0` 51.1,
  backends at `/usr/lib/papers/6/backends`) and **rofd ABI v1**
  (`ROFD_ABI_VERSION == 1`, `rofd-ffi` 0.4.3 = upstream `main` @ `140b936`,
  `$ROFD_SRC`).
- Citation convention: Papers-side claims name files in the upstream repository,
  pinned to tag `51.1` — `https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/<path>` —
  because that is where the code lives for readers and for the phase-4 MR. A local
  Papers checkout (`$PAPERS_SRC`) is a grepping convenience, not a build
  requirement: the module compiles against the installed headers. rofd-side claims
  use `$ROFD_SRC/…`, since the carried patch and the cross-repo tests need that
  checkout.
- `$ROFD_SRC` (and optionally `$PAPERS_SRC`) is resolved by `scripts/dev-env.sh`
  (environment → untracked `local.conf` → siblings `../papers`, `../rofd`); the
  script validates it against a marker file and reports a directory that merely
  shares the name. The ABI itself builds against the installed `librofd-ffi`
  package just as well.
- Every behavioural claim names the file (and usually the line) it came from, so it
  can be re-verified before coding. Symbols move; re-`grep` when in doubt.
- How to use this document: §1 fixes the target, §2–§4 are the constraints that
  cannot be re-litigated without new evidence, §5 is the execution order with
  acceptance criteria, §6 is how to prove each phase, Appendix A indexes the
  evidence, Appendix B records what the previous draft got wrong.
- Research notebook (raw evidence, decisions, dead ends):
  `.planning/2026-10-14-papers-ofd-backend-plan-and-readme/findings.md`.

## 1. Definition of done

A user with Papers ≥ 51 and this module installed can:

| Capability | Observable acceptance | Needs |
| --- | --- | --- |
| Open `*.ofd` by double-click, `papers file.ofd`, `gio open` | document opens, page counter and thumbnails correct | phases 0–3 |
| Scroll, zoom (incl. fit-width/fit-page), rotate all four ways | page pixels correct at every rotation and zoom step | phase 1 |
| Print and print-to-PDF | preview renders; produced PDF has selectable (vector) text | phase 1 |
| Metadata in the properties dialog / Nautilus properties | title, author, subject, keywords, creator, dates, page count | phase 1 |
| Nautilus thumbnail | file manager shows a rendered thumbnail | phases 1, 3 |
| Select, copy, find text | clipboard text matches the document; Ctrl+F snippets are correct; caret navigation works | phase 2 |
| Follow in-page links and browse the outline | GOTO/URI links act; outline sidebar navigates | phase 2 |
| Previewer (space bar in Nautilus) | preview window shows the document | phase 3 |

Non-goals for v1 (no rofd support today): annotations, signatures/seals, form
filling, attachments, layers, transitions, editing/saving. Papers hides the
corresponding UI automatically when an optional interface is absent.
`PpsDocument::save` is present but refuses (§3).

## 2. Architecture and decisions

```
 papers (shell/libview)                libofddocument.so            librofd_ffi.so.0
 ┌──────────────────────┐   vfuncs   ┌──────────────────┐   C ABI  ┌───────────────┐
 │ PpsDocument +        │──────────▶ │ OfdDocument      │────────▶ │ rofd_document │
 │ Text/Find/Links/     │            │  (GObject)       │          │ rofd_page     │
 │ Selection/Print      │◀────────── │ PpsPage▸rofd_page│◀─────────│ rofd_renderer │
 └──────────────────────┘  PpsPage   └──────────────────┘          └───────────────┘
   worker-thread jobs      objects     one GMutex for own state      ABI-safe for
   (GThreadPool)                       no global render lock         concurrent reads
```

Files:

```
papers-ofd/
├── meson.build, meson_options.txt
├── src/
│   ├── ofd-backend.c        pps_backend_query_type() + ABI check
│   ├── ofd-document.[ch]    OfdDocument: PpsDocument + the five interfaces
│   ├── ofd-page.[ch]        PpsPage <-> rofd_page_t attach/detach
│   ├── ofd-units.[ch]       mm<->pt, origin-normalizing rect conversion
│   ├── ofd-render.[ch]      option mapping + report handling (render/print/selection)
│   ├── ofd-metadata.[ch]    PpsDocumentInfo from rofd metadata
│   ├── ofd-outline.[ch]     PpsOutlines tree + links model
│   └── ofd-links.[ch]       PpsMappingList from the page link snapshot
├── data/                    mime xml, backend descriptor, association entry, thumbnailer
├── tests/                   C tests + fixture generator
├── debian/                  packaging
└── docs/plan.md
```

Decisions that constrain everything else:

| # | Decision | Why (evidence in Appendix A) |
| --- | --- | --- |
| D1 | Page handles are attached to `PpsPage.backend_page` in `get_page`; all page vfuncs read them from there | A1, A2: every page vfunc receives `PpsPage`; this is how Papers' own backends work; rofd pages are self-contained and outlive the document |
| D2 | No document-wide render lock; one `GMutex` only for our own mutable state (page-size cache, lazy snapshots, renderer pointer) | A3, A4: the ABI allows concurrent read-only calls and is internally synchronized; Papers calls us from a worker pool |
| D3 | Every rofd rectangle is converted through one helper that subtracts the page-box origin | A5, A6: rofd rects keep absolute coordinates while the renderer maps the box origin to canvas (0,0); Papers' page space starts at the page's top-left |
| D4 | Render scale comes from `pps_render_context_compute_scaled_size`, then `dpi=72`, `scale=scaled_w/width_pt`; surface size from `rofd_renderer_get_pixel_canvas_size` | A7, A8: `rc->scale` can be `0.0` with `target_size` set (selection paths, thumbnailer); rofd rounds with `ceil`, so only it knows the final size |
| D5 | Implement `PpsDocumentPrint` and `PpsSelection` in v1; leave `get_page_label` and `get_thumbnail*` to base-class fallbacks | A9, A10: print/selection are dispatched without NULL checks (crash); labels/thumbnails have real fallbacks |
| D6 | The outline is supported, via `PpsDocumentLinks::get_links_model` | A11: Papers 51 has an outline sidebar fed by that vfunc |
| D7 | Own MIME definition, own thumbnailer entry, own `NoDisplay` association desktop file | A12, A13: Papers derives its MIME lists from in-tree backends only; Nautilus needs a thumbnailer entry |
| D8 | Renderer created with NULL options (rofd's CJK-capable defaults) plus `PAPERS_OFD_FALLBACK_FAMILIES` env override | A14: defaults already include `Noto Sans CJK SC`; the override is needed to reproduce the reference PNG |
| D9 | Lenient rofd strictness (default); parse warnings and render diagnostics logged at `g_debug` | A15: real-world invoices are messy; rofd's limits already bound work; diagnostics must never fail a render |
| D10 | Page sizes come from a configure-time-detected `rofd_document_get_page_size_mm` when the linked rofd has it, otherwise from `rofd_document_get_page` | A17, A18: the fast path already exists in rofd-core but not in the ABI; without detection the module would either need a forked rofd or pay a full-document parse. The fallback keeps stock rofd working |
| D11 | Dependency strategy is fork-first: the `YangtseSu/rofd` fork mirrors `linuxdeepin/rofd` and carries our patches; the AUR `librofd-ffi` package is ours and is the delivery channel for the patched ABI. Upstream PRs stay open as the exit path, not as the plan's critical path | PR #11 has drawn only bot traffic since 2026-10-14 (CLA, ok-to-test, OWNERS approval) and the upstream project serves deepin's own client, so acceptance is unlikely on our schedule. The fork keeps the wait off the critical path while `patches/` mirrors every fork commit, so a machine without the fork reproduces the same ABI; D10's configure-time probe keeps the module working against stock rofd in either case |

## 3. Interface contract

Full mapping (Papers vfunc → rofd call → rule):

| vfunc | rofd | Rule |
| --- | --- | --- |
| `load(uri)` | `g_file_get_path` → `rofd_document_open` | non-local URI → `G_IO_ERROR_NOT_SUPPORTED`; check `rofd_abi_version()` against the header's `ROFD_ABI_VERSION`; keep the document handle |
| `load_fd(fd)` | `dup(fd)` → `rofd_document_open("/proc/self/fd/<dup>")` | fall back to a temp copy in `g_get_user_cache_dir()/papers-ofd/`; unlink at finalize; base class already proved `S_ISREG` |
| `save(uri)` | — | refusal stub: `FALSE` + `G_IO_ERROR_NOT_SUPPORTED` (unguarded upstream) |
| `get_n_pages` | `rofd_document_get_page_count` | cached at load |
| `get_page(i)` | `rofd_document_get_page(i)` | `pps_page_new(i)` + attach handle (D1) |
| `get_page_size(page)` | `rofd_page_get_size_mm` | mm→pt with `*72/25.4`; cache `{w,h,origin}` per page |
| `get_page_label` | — | left NULL (base class prints `"%d"`) |
| `render(rc)` | `rofd_renderer_render_page_cairo` | §4 |
| `get_thumbnail` / `get_thumbnail_surface` | — | left NULL (base class falls back to `render`) |
| `get_info` | `rofd_document_get_metadata` + `rofd_metadata_get_keyword` | map title/author/subject/keywords/creator/producer/dates; OR the "`PPS_DOCUMENT_INFO_*`" bits into `fields_mask` |
| `get_backend_info` | `rofd_library_version()` | `name = "rofd"` (static string); version borrowed (static storage) |
| *Print* `print_page` | `rofd_renderer_render_page_cairo` | `dpi=72, scale=1, rotation=0` into the borrowed `cairo_t` (**required**, unguarded upstream) |
| *Text* `get_text` | `rofd_page_get_text` | copy via `rofd_string_get_data`, then free |
| *Text* `get_text_layout` | `rofd_page_get_text_layout` + `rofd_text_layout_get_char` | one `PpsRectangle` per Unicode scalar, points, origin-normalized; return `FALSE` if scalar count ≠ `g_utf8_strlen(get_text)` |
| *Text* `get_text_in_area` | `rofd_page_get_text_for_area` | points → mm and add the origin back (input direction) |
| *Text* `get_text_mapping` | `rofd_page_get_selected_text(GLYPH, whole page)` | union region rects into a page-point `cairo_region_t` (I-beam cursor, page-scroll-by-line) |
| *Text* `get_text_attrs` | — | NULL (no attribute data in the ABI) |
| *Find* `get_supported_options` | — | `PPS_FIND_CASE_SENSITIVE \| PPS_FIND_WHOLE_WORDS_ONLY` |
| *Find* `find_text` | `rofd_page_find_text_with_options` | one `PpsFindRectangle` per match, `next_line = after_hyphen = FALSE`; `max_results` must be > 0 (policy constant, e.g. 4096 per page) — **unguarded upstream** |
| *Selection* `get_selected_text` | `rofd_page_get_selected_text` | selection style maps 1:1 — **unguarded upstream** |
| *Selection* `get_selection_region` | same call, region rects | mm→pt, scale by the rc scales, round into a `cairo_region_t` in device pixels |
| *Selection* `render_selection` | same call | fill the region rects with `base` into a full-page ARGB32 surface; ABI has no text mask, so the glyph recolour pass cannot be reproduced |
| *Links* `has_document_links` | `rofd_document_get_outline` | outline count > 0 |
| *Links* `get_links_model` | outline snapshot | `GListStore` of `PpsOutlines`; `set_expand(node.expanded)`; link from the first GOTO destination (fit/xyz via presence bits) or a URI action |
| *Links* `get_links` | `rofd_page_get_links` | one `PpsMapping` per region; only `ROFD_ACTION_EVENT_CLICK`; `GOTO`→`pps_link_dest_new_*`, `URI`→`pps_link_action_new_external_uri`, others skipped |
| *Links* `find_link_dest` / `find_link_page` | — | stubs returning `NULL` / `-1` (no ABI counterpart) |

Interface discipline (from the upstream NULL-check survey, Appendix A9/A10):

- If an interface is implemented, **every** vfunc of it is implemented — using
  stubs where the feature does not exist. Unguarded vfuncs: all of
  `PpsDocumentLinks`, `PpsDocumentFind::find_text`, `PpsSelection::get_selected_text`,
  `PpsDocumentPrint::print_page`, `PpsDocument::save`.
- Guarded (may stay NULL): `PpsDocumentText::*`, `PpsDocumentFind::get_supported_options`,
  `PpsSelection::render_selection` / `get_selection_region`, `PpsDocument::get_page_label`,
  `get_thumbnail`, `get_thumbnail_surface`.

## 4. Units, coordinates, and render math

Units: Papers page space is points with the origin at the page's top-left; OFD is
millimetres (`pt = mm * 72 / 25.4`). rofd reports the page as a rect from
`PhysicalBox` and keeps that box origin in `x_mm`/`y_mm`.

Origin rule (D3): the renderer draws the box origin at canvas (0,0) while all
rofd rectangles stay in absolute coordinates, so the single conversion helper is

```c
PpsRectangle out;
out.x1 = (r.x_mm - origin.x_mm) * PT_PER_MM;   /* same for x2 */
out.y1 = (r.y_mm - origin.y_mm) * PT_PER_MM;
out.x2 = out.x1 + r.width_mm  * PT_PER_MM;
out.y2 = out.y1 + r.height_mm * PT_PER_MM;
```

Inputs coming from Papers (e.g. `get_text_in_area`, selection points) are
converted back by adding the origin. With the common `PhysicalBox 0 0 w h` this is
a no-op, so it costs nothing and removes a whole class of misalignment bugs.

Render options (used by `render`; `print_page` passes `dpi=72, scale=1,
rotation=0` and ignores the rest):

```c
int scaled_w, scaled_h;                    /* pre-rotation pixel size */
pps_render_context_compute_scaled_size (rc, width_pt, height_pt, &scaled_w, &scaled_h);

rofd_render_options_t o;
rofd_render_options_init (&o, sizeof o);   /* dpi 96, white background, 256 MiB budget */
o.dpi = 72.0;
o.scale = (double) scaled_w / width_pt;    /* works even when rc->scale == 0 */
o.rotation_degrees = (uint32_t) rc->rotation;
o.background_rgba = 0xFFFFFFFF;            /* opaque white, like the PDF backend */
/* image_interpolation and max_raster_bytes keep their initialized defaults:
   a zeroed max_raster_bytes fails every render with RasterBudgetExceeded */

rofd_renderer_get_pixel_canvas_size (renderer, page, &o, &w, &h);  /* authoritative */
surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
cr = cairo_create (surface);
rofd_renderer_render_page_cairo (renderer, page, cr, &o, &report, &error);
```

Why this shape: `pps_render_context_compute_scaled_size` is the only correct
source for the pixel size (Papers passes `scale = 0` with a target size on the
selection and thumbnail paths), and `dpi = 72` makes rofd's `scale` numerically
identical to Papers' points→pixels factor while keeping the arithmetic in rofd's
millimetre space. rofd's own canvas size wins over Papers' rounded estimate (≤1 px
difference, invisible), because the surface and rofd's internal CTM must agree.

## 5. Phases

Each phase: deliverable → steps → acceptance → evidence to record. A phase is done
only when its acceptance has been *observed* and its evidence written down
(commands + output snippets in `progress.md`).

### Phase 0 — loadable module skeleton

Deliverable: a module Papers discovers and loads, with document/page geometry.

Steps:
1. `meson.build` + `meson_options.txt` (`papers-document-4.0 >= 51`, `rofd_libdir`,
   `rofd_includedir`, `papers_backend_dir` (`backend_dir` is reserved in Meson),
   `tests`); `shared_module('ofddocument', ...)`
   with `gnu_symbol_visibility: 'hidden'`, `-Wl,-z,now`, `-Wl,--no-undefined`.
   Detect the optional `rofd_document_get_page_size_mm` with
   `cc.has_function(..., prefix: '#include <rofd.h>')` → `HAVE_ROFD_PAGE_SIZE`.
2. `ofd-backend.c`: `pps_backend_query_type()` (include `pps-document.h` so the
   `PPS_PUBLIC` visibility declaration applies) + `rofd_abi_version()` check.
3. `ofd-document.c`: `G_DEFINE_TYPE` subclass; `load`, `get_n_pages`, `get_page`,
   `get_page_size` (fast path under `HAVE_ROFD_PAGE_SIZE`, else `get_page` +
   `rofd_page_get_size_mm`), `get_backend_info`, `save` stub; `ofd-units.c` +
   `ofd-page.c`.
4. `data/`: backend descriptor, `papers-ofd.xml`, association desktop file,
   thumbnailer entry.
5. `tests/`: link a C test against `libppsdocument`, load a generated fixture with
   `PPS_BACKENDS_DIR` pointing at the build dir, assert page count and page sizes.

Acceptance:
- `PPS_BACKENDS_DIR=<devdir> papers <fixture>.ofd` opens, scrolls, shows the right
  page count.
- Same session still opens a PDF (dev dir contains the system descriptors).
- A wrong `Module=` name in the descriptor makes Papers report a load failure
  (proves the discovery path is really exercised).
- `meson test -C _build` green.

Evidence to record: the dev-dir recipe, `meson test` summary, the `papers` window
screenshot/or description, the deliberate-failure message.

### Phase 1 — rendering, metadata, printing

Deliverable: correct pixels, working print path, populated document info.

Steps:
1. `ofd-render.c` (`ofd_render_page_onto` primitive + `ofd_render_page_surface`),
   `render` and `print_page` wired to it, report→`g_debug` diagnostics.
   (The `render` half landed in phase 0: `pps_document_render` dereferences the
   vfunc without a NULL check, so the phase-0 acceptance needs it — A20.)
2. `ofd-metadata.c` (`get_info`), parse time (measure on a large fixture, §8).
3. `get_text_mapping` is *not* part of this phase.

Acceptance:
- Rotation matrix: a fixture with a corner marker renders the marker in the
  expected corner at 0/90/180/270, in the view **and** in print output.
  (rofd documents clockwise rotation; if the render disagrees, map
  `rotation_degrees = (360 - r) % 360`.)
- Zoom matrix: 1.0×, 1.5×, 2×, fit-width, fit-page produce the requested pixel
  size; a test asserts surface size == `rofd_renderer_get_pixel_canvas_size`.
- Same-options equivalence: our surface for page 0 is byte-identical to a direct
  FFI render with the same options (regression net for the option mapping).
- Oracle: with `PAPERS_OFD_FALLBACK_FAMILIES="Noto Sans CJK SC:Noto Sans Mono"`,
  `dpi=254`, `scale=1`, rotation 0, `$ROFD_SRC/learning/test.ofd` page 0 matches
  `$ROFD_SRC/tests/fixtures/reference/invoice-phase3.png` within tolerance.
- Print preview and *Print to File* both render; text in the produced PDF is
  selectable (vector glyphs, not a bitmap).
- Properties dialog and Nautilus properties show title/author/subject/keywords/
  creator/dates/page count.
- Open latency: with a generated 300-page fixture, record wall-clock time from
  `pps_document_load` to the first page being visible for both code paths
  (`HAVE_ROFD_PAGE_SIZE` on/off). Both paths ship, so the comparison is a build
  switch, not a rewrite; the number goes into the README limitations.

Evidence to record: rotation and zoom screenshots/description, the equivalence
test name and result, the oracle diff metric, the PDF text-selection check.

### Phase 2 — text, selection, find, links, outline

Deliverable: the five optional interfaces per §3.

Steps:
1. `PpsDocumentText` (all five vfuncs), `PpsSelection` (three), `ofd-links.c`.
2. `PpsDocumentFind` (two) with the per-page cap.
3. `ofd-outline.c` + `PpsDocumentLinks::get_links_model` (stack walk over
   `first_child`/`next_sibling`, not subtree contiguity).

Acceptance:
- Drag-select + Ctrl+C matches `rofd_page_get_text` order for a multi-column and a
  CJK page; the highlight sits on the glyphs (proves rect/layout agreement).
- Ctrl+F on a CJK string and a case-insensitive ASCII string: sidebar snippets are
  the sentence around the match (proves find rects agree with the layout rects),
  `Enter`/`Shift+Enter` cycle across pages.
- F7 caret navigation works on a text page.
- I-beam cursor appears over text (proves `get_text_mapping`).
- GOTO link jumps, URI link opens the browser, outline sidebar navigates.

Evidence to record: copied text vs `rofd_page_get_text` diff, find results
screenshot, cursor/selection observations.

### Phase 3 — fd loading and packaging

Deliverable: previewer support and installable artefacts.

Steps:
1. `load_fd` (`dup` + `/proc/self/fd/<dup>`, temp-copy fallback, cleanup at finalize).
2. `debian/` (or PKGBUILD): module + descriptor + mime xml + thumbnailer +
   association entry; `update-mime-database`, `update-desktop-database` triggers;
   `Depends: librofd-ffi0, papers (>= 51), papers (<< 52)`.
3. Install/uninstall checks on a clean container or VM.

Acceptance:
- GNOME previewer (space bar) shows an OFD page.
- Nautilus shows the OFD icon and a rendered thumbnail;
  `papers-thumbnailer -s 256 file.ofd out.png` works standalone.
- Double-click and `gio open` open Papers; uninstalling leaves Papers working.
- The app grid still lists a single Papers entry and the running window groups as
  before (proves the `NoDisplay` association entry has no side effects).

Evidence to record: install log, `xdg-mime query filetype` output, thumbnailer PNG
dimensions, uninstall regression check (PDF still opens).

### Phase 4 — upstream MR (optional)

Move `src/*` into `libdocument/backend/ofd/` (Papers tree, tag `51.1`), add
`application/ofd` to Papers' MIME list and an `ofd` entry to the `backends` option.

How Papers declares the ABI dependency, in order of preference:

1. `dependency('rofd-ffi', version: '>= 0.4')` — the file already exists: our AUR
   package installs it (§9 item 5), and any package built from the fork's tree can
   ship the same one. No source in the Papers tree, the ABI stays a package
   dependency, as in D11 — the target state.
2. `subprojects/rofd-ffi.wrap` (`[wrap-git]`, revision pinned) plus a small
   `meson.build` that drives `cargo build --release --offline -p rofd-ffi` in a
   `custom_target` and exposes the result through `declare_dependency()` (link args
   + include dirs). Needed only where no distro package exists (e.g. a Flatpak
   source list).
   Meson's native Cargo subproject support cannot interpret rofd-ffi: the
   interpreter refuses any manifest with a `build.rs` ("an actual meson port will be
   required"), and `crates/rofd-ffi/build.rs` is what sets the SONAME.

A wrap file adds no sources to the repository — Meson fetches into `subprojects/`,
which stays untracked — while a git submodule would, pinning the same revision.
Either way the link step is unchanged (`-lrofd_ffi`); a wrap only changes where the
library is built.

Activation: the glue code must not change; if it does, an assumption in §2–§4 was
wrong and needs re-verification instead of a patch.

## 6. Verification playbook

Environment:

```bash
# dependencies: resolve and export PAPERS_SRC/ROFD_SRC/ROFD_INCLUDEDIR/ROFD_LIBDIR
. scripts/dev-env.sh

# rofd ABI library — from the checkout when present and built, otherwise from the
# installed librofd-ffi package (the script already exported the right dirs)
if [ -d "$ROFD_SRC" ]; then
  (cd "$ROFD_SRC" && cargo build --release --offline -p rofd-ffi)
fi
if [ "$ROFD_ABI_SOURCE" = checkout ]; then
  ln -sf librofd_ffi.so "$ROFD_LIBDIR/librofd_ffi.so.0"   # SONAME the module links against
fi

# module
meson setup _build \
  -Drofd_libdir="$ROFD_LIBDIR" \
  -Drofd_includedir="$ROFD_INCLUDEDIR"
meson compile -C _build && meson test -C _build

# dev backend dir: the whole system backend directory (modules + descriptors
# live together; copying descriptors alone breaks PDF/DjVu/TIFF), ours on top
mkdir -p _build/devbackends
cp -a "$(pkg-config --variable=backenddir papers-document-4.0)/." _build/devbackends/
cp _build/src/libofddocument.so _build/data/*.papers-backend _build/devbackends/

# application/ofd is defined by data/papers-ofd.xml; stage it without installing
mkdir -p _build/xdg/mime/packages
cp data/papers-ofd.xml _build/xdg/mime/packages/
update-mime-database _build/xdg/mime

export PPS_BACKENDS_DIR=$PWD/_build/devbackends
export XDG_DATA_DIRS=$PWD/_build/xdg:/usr/share
```

Checks per change:

```bash
papers <fixture>.ofd                                   # open, scroll, zoom, rotate, print
papers <some>.pdf                                      # regression: other backends unaffected
papers-thumbnailer -s 256 <fixture>.ofd /tmp/ofd.png   # thumbnail path
gio open <fixture>.ofd                                 # association path (after install)
```

Fixture matrix (generated by a checked-in Python script so they stay tiny and
deterministic; large or third-party fixtures are referenced by path, not copied —
paths below are relative to `$ROFD_SRC`):

| Fixture | Proves |
| --- | --- |
| CJK text page | text/layout/find/selection/caret |
| image-only scan page | render + thumbnail of a non-text page |
| mixed page sizes (A4/A5/landscape) | non-uniform page-size path in `setup_cache` |
| `PhysicalBox "10 20 210 297"` | origin normalization (§4) |
| links + outline + URI actions | links, outline, navigation, stubs |
| malformed zip / truncated XML | error paths, no crash |
| `$ROFD_SRC/learning/test.ofd` | real invoice with a stamp annotation; oracle image |
| `$ROFD_SRC/tests/ofd_files/6.1.00x…6.2.00x` | spec conformance, embedded fonts, compression variants |
| `$ROFD_SRC/tests/ofdrw-compat/fixtures` | real producer output (CJK, fonts) |

Cross-repository checks when the ABI dependency is bumped (needs `$ROFD_SRC`):

```bash
(cd "$ROFD_SRC" && cargo test -p rofd-ffi && crates/rofd-ffi/tests/run_c_tests.sh)
```

Citation re-verification (Appendix A): Papers files are read at
`https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/<path>` — only the build needs
no checkout, not the evidence audit; rofd files need `$ROFD_SRC`. The installed
`papers` package ships the `libdocument`/`libview` headers, which confirm
declarations but not line numbers or `.c` bodies.

## 7. Packaging and desktop integration

| File installed | Purpose |
| --- | --- |
| `<backenddir>/libofddocument.so` | the module |
| `<backenddir>/org.gnome.Papers.OfdDocument.papers-backend` | discovery (`Module`, `TypeDescription`, `MimeType` all mandatory) |
| `/usr/share/mime/packages/papers-ofd.xml` | registers `application/ofd`: glob `*.ofd`, `sub-class-of application/zip`, magic `PK\x03\x04`@0 + `OFD.xml`@30 |
| `/usr/share/thumbnailers/papers-ofd.thumbnailer` | `Exec=papers-thumbnailer -s %s %u %o`, `MimeType=application/ofd;` |
| `/usr/share/applications/org.gnome.Papers.Ofd.desktop` | `NoDisplay=true`, `Exec=papers %U`, `MimeType=application/ofd;` |

- Use the distinct MIME file name `papers-ofd.xml`: another package may already
  install `ofd.xml` for the same type (the dev host has `deepin-reader`'s). Both
  definitions merge in `update-mime-database`; ours adds the magic rules.
- Association (decided): ship the separate `NoDisplay` entry above; **do not patch
  Papers' own desktop file**.

  ```ini
  [Desktop Entry]
  Type=Application
  Name=Papers
  NoDisplay=true
  Exec=papers %U
  TryExec=papers
  Icon=org.gnome.Papers
  MimeType=application/ofd;
  Categories=GNOME;GTK;Office;Viewer;
  ```

  Why: no file owned by another package is rewritten, nothing must be re-applied
  after a Papers upgrade, and uninstall is a plain file removal. Window grouping
  and the app-grid icon are unaffected, because `papers` is a `GApplication` whose
  id is `org.gnome.Papers` (`meson.build:18,76`) and its icon is installed as
  `org.gnome.Papers.svg`; the extra file only makes Papers an "Open With"
  candidate for `application/ofd`.
- Rejected alternative: appending `application/ofd;` to the installed
  `org.gnome.Papers.desktop` and making it the default — it buys a single desktop
  entry at the cost of an idempotent post-install step, a package trigger to
  survive Papers upgrades, and a revert path on uninstall.
- Validate the shipped files: `desktop-file-validate data/*.desktop` and, for the
  MIME xml, install into a scratch `XDG_DATA_DIRS` tree followed by
  `update-mime-database` + `xdg-mime query filetype` on a real `.ofd`.
- Run `update-mime-database /usr/share/mime` and
  `update-desktop-database /usr/share/applications` after install.
- No AppStream component for an out-of-tree backend.

## 8. Risks and open questions

| Risk / question | Impact | Mitigation / next step |
| --- | --- | --- |
| `setup_cache` fetches and parses every page at load (rofd's FFI has no size-only call; `Document::page_size()` exists in rofd-core but is unexported) | open latency on large documents | request `rofd_document_get_page_size_mm` (§9); meanwhile measure on a 300-page fixture and publish the number |
| Rotation direction mismatch | all rotated views wrong | corner-marker fixture in phase 1; fallback mapping is a one-line change |
| `render_selection` cannot recolour glyphs | highlight covers text on builds without transparent selection | accept; optional "text mask" render mode request (§9). The host's poppler 26.08 has transparent selection, so the degradation is not visible here |
| Multi-line find matches collapse into one rectangle | result rows differ from the PDF backend | accept and document |
| `find_link_dest` / `find_link_page` stubs | named destinations / history restore incomplete | revisit if rofd gains a name→page lookup |
| `application/ofd` may already be defined by another package | duplicate definition | distinct file name + magic rules; verify with `xdg-mime query filetype` |
| No CJK font installed | missing-glyph boxes | rofd defaults already name `Noto Sans CJK SC`; distro packaging should recommend a CJK font |
| Papers 52 bumps `pps_document_current` | module installed under `papers/6/backends` stops being found (fail-soft) | pin `>= 51, < 52`; §3 is the contract to re-verify |
| Fixture licensing | cannot redistribute spec/ofdrw fixtures | generate our own; reference external fixtures by path |
| The carried page-size patch (A18) diverges from upstream or never lands | a patch to maintain in the meantime; stock rofd keeps working through the fallback path | keep it as a patch file, never a fork dependency; re-check at every rofd version bump and drop it once upstream has the function |
| rofd older than 0.4.2 (or a stock package built from `v0.4.0`) fails entire pages whose objects carry a singular transform | blank pages on real-world documents, no error visible in the UI | pin ≥ 0.4.2 (A19): our AUR package builds `0.4.3` + patch; document the requirement in `README.md` |

## 9. Upstream requests to rofd (ranked)

Policy (D11): these are *requests*, not dependencies. Everything below is
implemented in our fork and delivered through our AUR `librofd-ffi` package, so no
item blocks a phase; an upstream merge only deletes the matching entry in
`patches/` (removal criteria there) and lets the fork branch go.

1. `rofd_document_get_page_size_mm(document, index, rofd_rect_t *size_mm, rofd_error_t **error)`
   — exports the existing metadata-only `Document::page_size()`, removing the
   full-document parse from Papers' `setup_cache`. Same value and statuses as
   `rofd_page_get_size_mm` for the same index (`PAGE_OUT_OF_RANGE`,
   `INVALID_DOCUMENT`), documented as not parsing page content and therefore not
   collecting content warnings. Predicted change surface in `$ROFD_SRC`:
   `crates/rofd-ffi/include/rofd.h` (declaration + contract comment),
   `crates/rofd-ffi/src/document.rs` (implementation over `Document::page_size`),
   `crates/rofd-ffi/tests/c/expected-symbols.txt` (+1 line, the C runner diffs the
   exported set at `tests/run_c_tests.sh:74`), one Rust test and one C smoke path,
   plus a `README.md` line. Additive, no struct change, so ABI v1 stays v1 under
   their own versioning rules (`AGENTS.md`: "Extend the ABI additively"); the
   maintainers still decide whether the major stays.
   Status (2026-10-14): **implemented on the fork and shipped** —
   <https://github.com/linuxdeepin/rofd/pull/11> (branch `feat/ffi-page-size-query`
   at `7f6d24d` on `YangtseSu/rofd`) is the request; the delivery is
   `patches/rofd-ffi-page-size.patch`, which our AUR package applies to upstream
   `main` @ `140b936` (`0.4.3`, published without a tag) — that pinned commit is
   the ABI this module is built and tested against. Local verification before
   shipping: `cargo fmt --all -- --check`,
   `cargo clippy -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings`,
   `cargo test -p rofd-ffi` (three new tests), `crates/rofd-ffi/tests/run_c_tests.sh`
   (export list + C/C++ header + smoke); re-checked on this machine with
   `git apply --check` against the fork's `main`. With D10's probe a stock-rofd
   build still works through the slow path, so the patched and stock packages are
   interchangeable by construction.
2. `rofd_document_open_fd(fd, options, document, error)` (or documented
   `/proc/self/fd` support) — removes the `load_fd` shim.
3. Optional render mode emitting a text/glyph mask, so `render_selection` can match
   poppler on builds without transparent selection.
4. Optional: OFD `Version` string for `PpsDocumentInfo.format` (e.g. `ofd-1.0`).
5. A pkg-config file for the ABI (`rofd-ffi.pc`: `Name: rofd-ffi`, `Version:
   0.4.x`, `Cflags: -I${includedir}`, `Libs: -L${libdir} -lrofd_ffi`), so Meson
   projects stop hardcoding build-tree paths: `papers-ofd` today carries
   `-Drofd_libdir`/`-Drofd_includedir`, and Papers after phase 4 could say
   `dependency('rofd-ffi', version: '>= 0.4')` — route 1 there.
   Owned, not blocked (D11): our AUR package installs the file already
   (`rofd-ffi.pc.in` in the package repo); the upstream ask is only so other
   distributors do not have to reinvent it. It is packaging-side, so it is not a
   source patch under `patches/`; if a non-Arch distro package is ever needed, the
   same file goes into the fork's `debian/` as `rofd-ffi-pkgconfig.patch`.

## Appendix A — evidence index

Evidence paths are repository-relative: Papers paths are files in tag `51.1` of
<https://gitlab.gnome.org/GNOME/papers> (open them as
`https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/<path>`, or grep a local
`$PAPERS_SRC` clone); `$ROFD_SRC/<path>` marks the rofd checkout (a bare
`crates/…`/`debian/…` path means the same), resolved by `scripts/dev-env.sh`.

| id | Claim | Evidence |
| --- | --- | --- |
| A1 | Every page vfunc receives a `PpsPage` carrying backend state in public fields | `libdocument/pps-page.h:27-34` (public fields) + `libdocument/pps-document.h:52-88` (vfunc signatures); PDF backend `libdocument/backend/pdf/pps-poppler.c:349-362,368-380,396-444` |
| A2 | rofd pages are independently owned and survive their document; renderer/document handles are borrowed per call | `$ROFD_SRC/crates/rofd-ffi/include/rofd.h` (header contract + `rofd_page_free`) |
| A3 | rofd permits concurrent read-only calls and is internally synchronized | `$ROFD_SRC/crates/rofd-ffi/include/rofd.h`; `$ROFD_SRC/crates/rofd-ffi/README.md:329-335`; `$ROFD_SRC/crates/rofd-core/src/document.rs:155-176,735-767`; `$ROFD_SRC/crates/rofd-render/src/fonts.rs:100,653`, `images.rs:363` |
| A4 | Papers runs document work on a `GThreadPool` | `libview/pps-job-scheduler.c:67-100`; `libview/pps-jobs.c:850-878,383-440,1243-1275,1574-1602` |
| A5 | rofd rectangles are absolute (not re-based on the box origin) | `$ROFD_SRC/crates/rofd-core/tests/links.rs:11-70` |
| A6 | The rofd renderer maps the page-box origin to canvas (0,0) | `$ROFD_SRC/crates/rofd-render/src/cairo_renderer.rs:1190-1215` |
| A7 | `scale` can be 0.0 with `target_size` set; thumbnails omit target size | `libview/pps-jobs.c:1503`; `libview/pps-view.c:916-918`; `libview/pps-pixbuf-cache.c:1043-1044,1117-1118`; `thumbnailer/src/main.rs:138-145` |
| A8 | rofd canvas size is `ceil(dim*pixels_per_mm)`; `dpi/scale/rotation` semantics; render-option init defaults (incl. 256 MiB raster budget) | `$ROFD_SRC/crates/rofd-render/src/cairo_renderer.rs:60-97,1148-1215`; `$ROFD_SRC/crates/rofd-ffi/src/abi.rs:676-712` |
| A9 | Unguarded vfunc dispatches: `save`, all `PpsDocumentLinks`, `find_text`, `PpsSelection::get_selected_text`, `print_page` | `libdocument/pps-document.c:461-468`; `pps-document-links.c:20-78`; `pps-document-find.c:27-34`; `pps-selection.c:54-64`; `pps-document-print.c:25-31` |
| A10 | Guarded vfuncs: `PpsDocumentText::*`, `PpsDocumentFind::get_supported_options`, `PpsSelection::render_selection`/`get_selection_region`, `load_fd`; `get_page_label` + `get_thumbnail*` have base-class fallbacks | `pps-document-text.c:23,46,69,81,102`; `pps-document-find.c:59-66`; `pps-selection.c:38-52,66-78`; `pps-document.c:404-412,548-560,646-695` |
| A11 | Outline sidebar is fed by `has_document_links` + `get_links_model`; page labels are filled by the job | `shell/src/sidebar_links.rs:483`; `shell/src/document_view/signals.rs:410`; `libview/pps-jobs.c:104-140`; `pps-outlines.h` |
| A12 | Papers' MIME lists (desktop + thumbnailer) come from in-tree backends; backend discovery needs `*.papers-backend` with `Module`/`TypeDescription`/`MimeType` | `meson.build:56-64,254-330`; `data/*.desktop.in.in`; `thumbnailer/papers.thumbnailer.in`; `libdocument/pps-backend-info.c:30-70,78-120`; `pps-document-factory.c:100-124,155-166,203-215` |
| A13 | MIME detection is two-pass (filename/content-type, then 1024 sniffed bytes); the previewer's fd path sniffs content | `libdocument/pps-file-helpers.c:300-402`; `pps-document-factory.c:250-283`; `previewer/pps-previewer.c:171-178` |
| A14 | rofd's default fallback families include CJK; the reference PNG is generated at 254 dpi with explicit families | `$ROFD_SRC/crates/rofd-ffi/src/renderer.rs:21`; `$ROFD_SRC/crates/rofd-render/tests/real_fixture.rs:13-20,140-150`; `$ROFD_SRC/crates/rofd-render/README.md` |
| A15 | `setup_cache` walks all pages inside the load job; search snippets are derived from find rects ∩ layout rects; caret needs `get_text`+`get_text_layout`; ink/selection surfaces are full-page ARGB32 at the scaled size | `libdocument/pps-document.c:203-276`; `libview/context/pps-search-context.c:240-300,331-380`; `libview/pps-view.c:3225-3248,3037-3078`; `libdocument/backend/pdf/pps-poppler.c:1888-2010`; rofd resource limits and strictness: `$ROFD_SRC/crates/rofd-core/src/options.rs` |
| A16 | Zoom is clamped by the pixbuf-cache budget and `MAX_IMAGE_SIZE 32767`, so request sizes stay under rofd's raster budget | `libview/pps-view.c:5653,5663-5675` |
| A17 | Pages' size-only query exists in rofd-core but not in the ABI; SONAME/packaging constraints for `librofd_ffi.so.0` | `$ROFD_SRC/crates/rofd-core/src/document.rs:664-735`; `$ROFD_SRC/crates/rofd-ffi/build.rs`; `$ROFD_SRC/crates/rofd-ffi/tests/run_c_tests.sh:36-46`; `$ROFD_SRC/debian/rules:27-36`; `$ROFD_SRC/debian/control` |
| A18 | The exported FFI symbol set is pinned (77 names) and diffed by the C runner, so an added function must update that list; rofd's own rule is to extend the ABI additively | `$ROFD_SRC/crates/rofd-ffi/tests/c/expected-symbols.txt`; `$ROFD_SRC/crates/rofd-ffi/tests/run_c_tests.sh:74`; `$ROFD_SRC/AGENTS.md` ("Extend the ABI additively") |
| A20 | `render` is dispatched without a NULL check as well (`pps_document_render` calls `klass->render` directly), so a module that implements loading but no rendering crashes Papers as soon as it displays the first page — a phase-0 skeleton without `render` segfaults (observed: exit 139) | tag `51.1` `libdocument/pps-document.c:637-643`; observed Papers run in phase 0 |
| A19 | rofd ≤ 0.4.0 **aborts the whole page render** with `ROFD_STATUS_INTERNAL` on a document object carrying a singular transform (degenerate CTM, e.g. the empty text objects some converters emit); `0.4.2`+ maps the diagnostic to `ROFD_DIAGNOSTIC_SINGULAR_TRANSFORM` (7) and rendering continues. ABI v1 and the pinned 77-symbol set are unchanged, so the fix is free for us — but it makes 0.4.2 the minimum that matches D9 ("never fail a render because of a diagnostic") | `$ROFD_SRC/crates/rofd-ffi/include/rofd.h:75,147`; `$ROFD_SRC/crates/rofd-ffi/tests/c/expected-symbols.txt` (77); upstream commit `8cfb2f7` on `main` ("fix(ffi): map SingularTransform diagnostic to C ABI v1") |

## Appendix B — corrections to the previous draft

1. `pps_page_new_with_index` does not exist → `pps_page_new(index)` plus the public
   `backend_page`/`backend_destroy_func` fields (A1).
2. `get_page_label` dropped: the base class already prints `"%d"` and discards
   default-equal labels (A10).
3. `get_thumbnail` / `get_thumbnail_surface` dropped: both fall back to `render` (A10).
4. `PpsDocumentPrint::print_page` is mandatory — the previous draft omitted it and
   printing would have dereferenced a NULL vfunc (A9).
5. `PpsSelection` is required for selection/highlight/copy; `PpsDocumentText` alone
   is not enough (A9, A10, A15).
6. The outline is supported via `PpsDocumentLinks::get_links_model`; the previous
   draft dropped it on a wrong premise (A11).
7. No document-wide `GRecMutex`: the ABI explicitly allows concurrent reads (A3);
   only our own state needs a lock.
8. Rectangle origin normalization added; invisible for `PhysicalBox 0 0 w h`,
   mandatory otherwise (A5, A6).
9. `max_raster_bytes` trap documented: leaving the initializer's field zeroed fails
   every render (A8).
10. The previous draft's host claim "`application/ofd` absent" is only half right:
    `deepin-reader` ships a definition on this host, stock shared-mime-info has
    none. We still ship our own (glob + magic) because the fd path and
    extension-less files need magic (A12, A13).
11. Desktop integration needs more than the backend descriptor: a `.thumbnailer`
    entry and an association desktop file (A12).
12. `load_fd` does not need a temp copy by default: `dup` + `/proc/self/fd/<dup>`
    covers the previewer; keep the copy only as a fallback.
13. Unimplemented-but-required stubs are mandatory wherever the wrapper does not
    NULL-check (A9).
14. No hardcoded fallback font list: rofd's defaults already cover CJK; add only the
    env override for reproducible comparisons (A14).
15. Verification commands corrected: `papers-thumbnailer -s <size> <in> <out>`;
    `PPS_BACKENDS_DIR` replaces the backend set (include the system descriptors);
    the reference PNG needs 254 dpi + `Noto Sans CJK SC`/`Noto Sans Mono` (A13, A14).
