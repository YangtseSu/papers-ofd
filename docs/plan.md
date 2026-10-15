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
  use `$ROFD_SRC/…`, since the carried changes and the cross-repo tests need that
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
| Previewer (space bar in Nautilus) | preview window shows the document | phases 1, 3 |

Non-goals for v1 (no rofd support today): annotations, signatures/seals, form
filling, attachments, layers, transitions, editing/saving. Papers hides the
corresponding UI automatically when an optional interface is absent.
`PpsDocument::save` is present but refuses (§3). Annotation editing is researched
and deferred, not forgotten: §10 records the contract, the writer craft and the
decision a future phase starts from.

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
│   ├── ofd-text.[ch]        PpsDocumentText, PpsSelection, PpsDocumentFind vfuncs
│   ├── ofd-outline.[ch]     PpsOutlines tree + links model
│   └── ofd-links.[ch]       PpsMappingList from the page link snapshot + PpsLink from a rofd action
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
| D11 | Dependency strategy is fork-first: the `YangtseSu/rofd` fork mirrors `linuxdeepin/rofd` for development and delivers through its own tagged releases on `release/*` branches (`<upstream>-fork.<n>`, e.g. `v0.4.3-fork.1`); the AUR `librofd-ffi` package builds a release tarball directly and carries no patch files. `patches/README.md` is the release inventory. Upstream PRs stay open as the exit path, not as the plan's critical path | PR #11 has drawn only bot traffic since 2026-10-14 (CLA, ok-to-test, OWNERS approval) and the upstream project serves deepin's own client, so acceptance is unlikely on our schedule. The fork releases keep the wait off the critical path and the AUR package a plain `source=` + `sha256sums`; `patches/README.md` indexes what each release carries, so a machine without the fork reproduces the same ABI; D10's configure-time probe keeps the module working against stock rofd in either case |
| D12 | Annotation editing is out of v1; if taken up it is implemented against GB/T 33190-2016 clause 15 and annex A.3/A.4 inside the fork, with **no** dependency on `office-rs/rofd`, which stays a read-only craft reference. Future scope: a writer in `rofd-core`, additive ABI calls (list/add/remove annotations, save), then the module's `PpsDocumentAnnotations` and a real `PpsDocument::save` | The whole toolset is gated on that one interface, and both `get_annotations` and `PpsDocument::save` are dispatched without NULL checks (`libdocument/pps-document-annotations.c:26-32`, `pps-document.c:461-469`); our reader drops any `Annot` whose `Type` is outside the standard's five values, so the writer must stay inside them (`crates/rofd-core/src/document.rs:1055-1080`); the engine cannot change (cairo rasterization, find/links/outline, our patched ABI), and `office-rs/rofd` cannot embed the image a Papers stamp needs. Detail: §10 |

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
| *Text* `get_text` | `rofd_page_get_text` | copy via `rofd_string_get_data`, then free; `NULL` when the query fails, so `get_text` and `get_text_layout` always agree on whether the page has text |
| *Text* `get_text_layout` | `rofd_page_get_text_layout` + `rofd_text_layout_get_char` | one `PpsRectangle` per Unicode scalar, points, origin-normalized; `FALSE` on any query failure or if the scalar count ≠ `g_utf8_strlen(get_text)` (the two rofd queries would disagree). A page without text yields `TRUE` with `*areas == NULL` and 0 areas, which is what every consumer checks (`libview/pps-view.c:3049-3060` guards on `!areas`) |
| *Text* `get_text_in_area` | `rofd_page_get_text_for_area` | points → mm and add the origin back (input direction) |
| *Text* `get_text_mapping` | `rofd_page_get_selected_text(GLYPH, whole page)` | union region rects into a page-point `cairo_region_t` (I-beam cursor, page-scroll-by-line); region rects are rounded like `create_region_from_poppler_region`, so scale 1.0 gives the integer page-point rects the hit tests expect |
| *Text* `get_text_attrs` | — | NULL (no attribute data in the ABI); the accessible-text path checks for it (`libview/pps-view-page.c:1244`) |
| *Find* `get_supported_options` | — | `PPS_FIND_CASE_SENSITIVE \| PPS_FIND_WHOLE_WORDS_ONLY` |
| *Find* `find_text` | `rofd_page_find_text_with_options` | one `PpsFindRectangle` per match, `next_line = after_hyphen = FALSE`; `max_results = 4096` (`OFD_FIND_MAX_RESULTS_PER_PAGE`: the ABI rejects 0 and the search context has no cap of its own) — **unguarded upstream**, returns NULL for an empty query or no match |
| *Selection* `get_selected_text` | `rofd_page_get_selected_text` | selection style maps 1:1 (GLYPH/WORD/LINE) — **unguarded upstream**, returns an empty string rather than NULL for a page it cannot query |
| *Selection* `get_selection_region` | same call, region rects | mm→pt (origin-normalized), scaled by rofd's canvas size over the page size, rounded into a `cairo_region_t` in device pixels |
| *Selection* `render_selection` | same call | fill the region rects with `base` into an ARGB32 surface sized by rofd (`ofd_render_page_canvas_size`), cleared first because the view reuses it; ABI has no text mask, so the glyph recolour pass cannot be reproduced |
| *Selection* any | — | rotation ≠ 0 → nothing drawn / no region: Papers refuses to start a selection on a rotated page ("Selection in rotated documents has never worked", `libview/pps-view.c:4512`), and the mapping is unrotated page points → canvas |
| *Links* `has_document_links` | `rofd_document_get_outline` | outline count > 0 |
| *Links* `get_links_model` | outline snapshot | `GListStore` of `PpsOutlines`, rebuilt from `parent`/`first_child`/`next_sibling` (one children store per node that has children), `NULL` when there is no outline like the PDF backend; `set_expand(node.expanded)`; every node gets a `PpsLink` (the sidebar dereferences the selected row's link), whose action is the first usable GOTO/URI — a node without one keeps an actionless link |
| *Links* `get_links` | `rofd_page_get_links` | one `PpsMapping` per region of the first `ROFD_ACTION_EVENT_CLICK` action, in rofd's source order; `GOTO`→`pps_link_dest_new_*` (skipped when the destination has no resolved page), `URI`→`pps_link_action_new_external_uri`; attachment/unknown kinds skipped; `NULL` when the page handle is unusable |
| *Links* destinations | `rofd_*_get_action_destination` | coordinates are absolute in the **target** page's space, so they are re-based on that page's box origin (D3); presence bits become `change_left`/`change_top`; zoom becomes `change_zoom` only when the ABI reports it and it is non-zero (GB/T 33190 table 54: zero/absent keeps the current zoom); an unresolved GOTO yields no link instead of page zero |
| *Links* URIs | `rofd_action_t.uri` / `.uri_base` | rofd preserves both unresolved; the module resolves a relative URI against `Base` with `g_uri_resolve_relative` (the shell hands the string to the desktop), falling back to the raw string when resolution fails |
| *Links* `find_link_dest` / `find_link_page` | — | stubs returning `NULL` / `-1` (no ABI counterpart) |

All rectangles the text, find and selection vfuncs publish follow rofd's page-space
geometry, in the model a PDF reader uses: a scalar whose metrics the ABI resolves gets
exactly the advance of its glyph as the box width and the *face's* typographic ascent
and descent as the box's vertical extents (poppler's text box,
`TextWord::setInitialBounds`: `yMin = y - ascent`, `yMax = y - descent`), so glyph boxes
tile a run, a match rectangle hugs the matched text, and the boxes of Latin, CJK and
punctuation share one band instead of each hugging its own ink. A selection region then
grows that band by an eighth of its height above and below, which is the margin
`TextSelectionSizer::visitLine` gives a PDF selection region, so a selected line reads
as one band that clears the glyphs (A27). A run that declares no `DeltaX` is placed by
its font's advance, which stock rofd resolves only for the renderer, and its box spanned
one font size above the baseline and stopped there: the semantic layer disagreed with
the drawn glyphs, so highlights, caret positions and selection regions drifted off their
text (A24) and cut the bottom off it (A27). Our fork carries the fix
(upstream PR #13, shipped in the fork's `v0.4.3-fork.1` release the AUR package builds); with a stock library the
module still publishes rofd's model unchanged, and `tests/test-ofd-text.c` classifies
the linked ABI against the rendered ink and only asserts alignment and coverage when the
library resolves font metrics.

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

rofd has a **single uniform scale**, so the height follows the width-derived
scale: a target size with a different aspect ratio than the page is not stretched
(poppler's backend scales x and y independently). No Papers caller does that —
every target size is derived from the page size at a scale
(`libview/pps-pixbuf-cache.c:1041-1044,1113-1118`, `libview/pps-jobs.c:382-385`)
— so the difference is unobservable, and the alternative (render, then stretch the
surface) would resample and break the same-options equivalence test.

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
   `PpsDocumentPrint` is implemented as an interface (`print_page` at
   `dpi=72, scale=1, rotation=0` into the borrowed context), and the renderer is
   created from rofd's defaults with the `PAPERS_OFD_FALLBACK_FAMILIES` override
   (D8) used by the oracle check.
2. `ofd-metadata.c` (`get_info`): metadata snapshot → title/author/subject/
   keywords (joined with `", "`)/creator (`Creator` + `CreatorVersion`, since the
   ABI has no producer field)/creation and modification datetimes, plus
   `n_pages` and the per-page sizes in millimetres with the
   `PPS_DOCUMENT_INFO_N_PAGES`/`PAPER_SIZE` bits the properties dialog gates on.
3. `tests/`: rotation matrix (corner-marker fixture, 0/90/180/270), origin
   normalization (non-zero `PhysicalBox` origin), zoom matrix (paper-requested
   size ±1 px and exact equality with `rofd_renderer_get_pixel_canvas_size`),
   same-options byte equality against a direct FFI render, print-to-PDF text
   selectability (cairo PDF surface and a real GTK print-to-file run, both
   verified with poppler), metadata fields and mask bits, the oracle comparison
   against `$ROFD_SRC/tests/fixtures/reference/invoice-phase3.png`, and the
   open-latency tool for both page-size paths (`-Drofd_page_size=disabled`
   forces the fallback).
4. `get_text_mapping` is *not* part of this phase.

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
test name and result, the oracle diff metric, the PDF text-selection check, and
the latency table for both page-size paths.

Phase-1 evidence as observed (2026-10-14, `meson test` 7/7 in both builds,
`.planning/2026-10-14-papers-ofd-backend-plan-and-readme/progress.md`):
`ofd-render` (rotation/origin/zoom/equivalence/PDF), `ofd-print-file` (GTK
print-to-file, text verified with poppler and `pdftotext`), `ofd-metadata`,
`ofd-oracle` (2115×1400, mean channel difference 3.28, 2.6 % of pixels over 8),
`ofd-latency` (300 content pages: the metadata-only size walk before any page
object exists takes 20 ms on the fast path against 71 ms on the fallback, which
parses every page for its size; the end-to-end sequence load → metadata walk →
`setup_cache` → first render is ~120–145 ms in both builds, because
`setup_cache` fetches every page object either way. The fast path therefore pays
off for callers that query sizes without fetching pages, not for the shell's own
open path).

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

Phase-2 evidence as observed (2026-10-15, `meson test` 10/10 in both page-size
builds, `.planning/2026-10-14-papers-ofd-backend-plan-and-readme/progress.md`):

- `ofd-text` — for the two fixture pages, `get_text` is byte-identical to
  `rofd_page_get_text`, the layout has exactly `g_utf8_strlen` entries (each
  inside the page box, synthesised object separators empty), `get_text_in_area`
  over the left column equals `rofd_page_get_text_for_area` for the same
  rectangle, the mapping region covers a glyph centre and not the page corner,
  find returns 2/1/2/0 hits for `invoice`/case-sensitive/`invo`+whole-words and 2
  for `发票编号`, every match rectangle satisfies the search context's agreement
  test and lands on the scalar where the query starts, all three selection styles
  equal `rofd_page_get_selected_text` for the same rectangle, the selection region
  contains the glyph centres and stays inside rofd's canvas at scale 1 and at a
  target size, and the overlay surface is the canvas size with the base colour at
  the requested alpha inside the selection and alpha 0 outside — including after
  reuse for a different selection.
- `ofd-links` — the outline model is the fixture's 4-node preorder tree (3 roots,
  one child, `Expanded="false"` only on the first), every row carries a link whose
  destination page/type/coordinates/zoom match the ABI, the sibling and nested
  URI actions resolve as expected, and `link_page` returns the page the sidebar
  navigates to; page 0's mapping list is rofd's three CLICK actions in source
  order with regions and destinations equal to the ABI's (mm → points), the
  centre of the GOTO box hit-tests to the smaller mapping rather than the
  full-page URI action behind it, page 1's whole-page XYZ link keeps
  left/top/zoom 20/2, and the two vfuncs without an ABI counterpart are stubs.
- `ofd-errors` — the malformed-zip fixture is detected as `application/ofd` (from
  bytes alone for the fd path) and fails the load with
  `G_IO_ERROR_INVALID_DATA`; a truncated copy of a valid document fails the same
  way; a missing file fails without a backend; reloading a document that failed
  recovers; `n_pages` stays -1 until a load succeeds.
- Origin normalization (D3) is asserted on a non-zero `PhysicalBox` origin for
  text/layout/find/selection *and* for a link region plus its destination, where
  the fixture's absolute millimetres and the page-point answers differ by the
  origin (`origin-text` fixture, both test programs).
- GUI session (log level): `papers` on the text, scan and links fixtures runs the
  links job and the page-data jobs for every visible page (our
  `get_links_model`/`has_document_links`, `get_text`, `get_text_layout`,
  `get_text_mapping`, `get_links`) with zero CRITICAL/ERROR lines; a PDF in the
  same dev backend directory still opens cleanly.
- GUI acceptance (live session, user-confirmed 2026-10-15): select/copy matches
  the drawn text, the Ctrl+F rows and the search highlight sit on the searched
  text (after the geometry fix of A24), F7 caret navigation works, the I-beam
  cursor appears over text, the in-page GOTO link jumps, the whole-page link
  returns, and the outline sidebar lists and navigates the fixture's tree.

### Phase 3 — fd loading and packaging

Deliverable: previewer support and installable artefacts.

Steps:
1. `load_fd` (`dup` + `/proc/self/fd/<dup>`, temp-copy fallback, cleanup at
   finalize) — the previewer binary's contract. Note that the *space bar* does not
   go through it: Sushi renders documents in-process through Papers' libraries and
   loads them by URI, which is what step 4 addresses.
2. Packaging, Arch route (decided): the AUR repository
   [`papers-ofd`](https://aur.archlinux.org/packages/papers-ofd) is the only home
   of the recipe — this repository carries no PKGBUILD; module + descriptor + mime
   xml + thumbnailer + association entry; `depends: librofd-ffi, papers>=51,
   papers<52`.
   `update-mime-database`/`update-desktop-database` come from the pacman hooks of
   `shared-mime-info`/`desktop-file-utils`, which `papers` already requires.
   `debian/` follows only when a non-Arch consumer appears.
3. Install/uninstall checks: a real install on this machine (`pacman -U`) plus the
   staged-tree checks below — no container or VM, which the machine's rules rule
   out.
4. Sushi (the space bar) picks its document renderer from the MimeType line of
   `org.gnome.Papers.desktop`, which does not list `application/ofd` out of tree,
   so the preview falls back to the file-information panel (A26). Ship
   `data/papers-ofd-sushi.js` into Sushi's system plugin directory and a test that
   drives Sushi's own selector and renderer; drop both once Papers lists the type
   itself (phase 4).

Acceptance:
- GNOME previewer (space bar) shows an OFD page: Sushi's document renderer loads
  the file through Papers (step 4; observed headlessly against the fixture and
  user-confirmed on the live session).
- Nautilus shows the OFD icon and a rendered thumbnail;
  `papers-thumbnailer -s 256 file.ofd out.png` works standalone.
- Double-click and `gio open` open Papers; uninstalling leaves Papers working.
- The app grid still lists a single Papers entry and the running window groups as
  before (proves the `NoDisplay` association entry has no side effects).

Evidence to record: install log, `xdg-mime query filetype` output, thumbnailer PNG
dimensions, uninstall regression check (PDF still opens).

Phase-3 evidence as observed (2026-10-15, `meson test` 12/12 in `_build`,
`_build-slow` and `_build-fork`; full command/output records in
`.planning/2026-10-14-papers-ofd-backend-plan-and-readme/progress.md`):

- `ofd-fd` (new test program) drives the previewer's path end to end: sniff the
  type from the descriptor with `pps_file_get_mime_type_from_fd`, build the
  document with `pps_document_factory_get_document_for_fd`, then
  `pps_document_load_fd`. The fd document equals the URI document in page count,
  page sizes, page text, rendered pixels (memcmp at scale 1.5) and document info
  for both the text and the metadata fixture; the descriptor the caller passed is
  consumed in every outcome (also on the base class's rejections: a pipe, a
  directory and a write-only file all answer `G_FILE_ERROR_BADF`); a descriptor
  whose file the caller unlinked still loads through `/proc/self/fd/<dup>` and
  leaves no temp copy; a truncated document fails with the same
  `G_IO_ERROR_INVALID_DATA` message as the URI path, and the same document object
  loads a good descriptor afterwards.
- The fallback is observed, not just written: a descriptor whose read permission
  was withdrawn after the open cannot be re-entered through `/proc` (EACCES,
  A25), so the module streams the bytes into
  `$XDG_CACHE_HOME/papers-ofd/ofd-fd-XXXXXX`. The test counts exactly one such
  file while the document uses it and none after a reload from a URI or after the
  document is finalized; the loaded document matches the URI document in the same
  five ways.
- Package: `papers-ofd 0.1.0-1` built by `makepkg` from the AUR recipe,
  source = pinned commit archive of the now-public
  `YangtseSu/papers-ofd` (`6936c96`, sha256
  `64c1797d6bcc5f…b28853b`), verified by an anonymous download and by a build
  with an empty `SRCDIR`. `namcap` reports only the `gcc-libs` advisory for the
  package and nothing for the PKGBUILD. File list: `libofddocument.so` +
  descriptor under `usr/lib/papers/6/backends`, MIME xml, thumbnailer entry,
  `NoDisplay` association entry under `usr/share`; none of the five paths is owned
  by another package, so uninstalling removes only our files.
- Published to the AUR: `papers-ofd 0.1.0-1`
  (<https://aur.archlinux.org/packages/papers-ofd>, `master` = `86e4929`), the
  mirror living in `~/projects/papers-ofd-aur` like the `librofd-ffi` package.
- Staged-tree checks (the package extracted to a prefix, `update-mime-database` /
  `update-desktop-database` run on it, `XDG_DATA_DIRS` pointing there):
  `desktop-file-validate` clean; `xdg-mime query filetype text-2pages.ofd` →
  `application/ofd`; `xdg-mime query default application/ofd` and `gio mime` →
  `org.gnome.Papers.Ofd.desktop`, which is also the recommended application.
- Runtime with the packaged module (`PPS_BACKENDS_DIR` = the system backend
  directory plus the package's two files): `papers-thumbnailer -s 256` gives
  181×256 for the text fixture and 256×170 (mean 63934/65535, sd 6312) for the
  real invoice; the PDF regression thumbnail is 198×256, i.e. the other backends
  still work from that directory.
- Previewer binary: `papers-previewer --fd 3 < text-2pages.ofd` (MIME sniffed
  from the descriptor) loads and renders the document — 334 log lines of render
  diagnostics from this module, zero CRITICAL/ERROR. This is the previewer's own
  `--fd` contract, not the space bar: nothing in the stack passes a descriptor
  (Papers' print preview spawns `papers-previewer` with paths,
  `libview/pps-print-operation.c:556`; A26).
- Space bar: the first acceptance run showed the file-information panel instead
  of a preview, and A26 is the diagnosis — Sushi's document renderer claims only
  the types of `org.gnome.Papers.desktop`, which lacks `application/ofd`. With
  `data/papers-ofd-sushi.js` in place, `ofd-sushi-plugin` drives Sushi's own
  selector and renderer against the fixture: the selector returns the
  Papers-backed renderer instead of the fallback, and the renderer's load job
  finishes with the fixture's 2 pages (`meson test -C _build`, 13/13). The
  *packaged* plugin alone carries it: with only
  `/usr/lib/sushi/plugins-1/papers-ofd-sushi.js` present (the temporary user-level
  copy removed), the selector returns the document renderer and the renderer loads
  the fixture's 2 pages. User-confirmed on the live session (2026-10-15): the
  space bar shows the page.
- `papers text-2pages.ofd` and `gio open text-2pages.ofd` both start Papers with
  that file (`/usr/bin/papers …/text-2pages.ofd`), the latter through the
  association entry.
- Install and visual acceptance (2026-10-15): the user installed `0.1.0-1`
  (`pacman -U`, no file conflicts) and then `0.1.0-2`, which carries the Sushi
  plugin, and confirmed the four session items — the space-bar preview shows the
  page, Nautilus shows the OFD icon and a rendered thumbnail, a double-click opens
  Papers, and the app grid still lists a single Papers entry. Still open from the
  acceptance list: `pacman -Rns papers-ofd` as the uninstall regression, and the
  AUR index lag (the publication is confirmed by `git ls-remote` and, later, by
  the AUR itself reporting `0.1.0-2`).

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

# module (-Drofd_src=<rofd checkout> additionally builds the oracle test, which
# needs $ROFD_SRC/learning/test.ofd and the reference PNG; -Drofd_page_size=disabled
# builds the per-page fallback for the open-latency comparison)
meson setup _build \
  -Drofd_libdir="$ROFD_LIBDIR" \
  -Drofd_includedir="$ROFD_INCLUDEDIR" \
  -Drofd_src="$ROFD_SRC"
meson compile -C _build && meson test -C _build
# ofd-units, ofd-document, ofd-discovery-failure, ofd-render, ofd-print-file,
# ofd-metadata, ofd-text, ofd-links, ofd-search, ofd-errors, ofd-fd, and
# ofd-oracle with -Drofd_src (the AUR package build therefore
# runs 11). ofd-search runs Papers' own search context over the text fixture,
# i.e. the snippets the Ctrl+F sidebar shows; it leaks that context on purpose
# (A23). ofd-text compares find rectangles with the ink rofd renders and asserts
# alignment and tiling when the linked library resolves font metrics (A24), and, with
# the same fix, that the selection region is the face's band grown by the eighth-height
# highlight margin: it must cover the ink of its own line, stick out above and below it,
# and keep one height per paragraph (A27). That needs librofd-ffi >= 0.4.3-6 (or a build
# of the fork branch); an older library is reported instead of failing silently, and a
# stock one is classified as size-based footprint geometry. ofd-fd drives the previewer's path (MIME sniffing from the
# descriptor, pps_document_factory_get_document_for_fd, pps_document_load_fd) and
# compares the result with the URI document pixel by pixel; it sets XDG_CACHE_HOME
# to a scratch directory to watch the temp-copy fallback (A25) come and go.

# Building against a *checkout* instead of the installed package: rofd-ffi.pc wins
# over -Drofd_libdir, so hide it from pkg-config, and set LD_LIBRARY_PATH at
# runtime (the module's DT_NEEDED resolves the SONAME from the default paths):
#   mkdir -p /tmp/pc-norofd && cp /usr/lib/pkgconfig/*.pc /tmp/pc-norofd/ \
#     && rm /tmp/pc-norofd/rofd-ffi.pc
#   PKG_CONFIG_LIBDIR=/tmp/pc-norofd meson setup _build-fork \
#     -Drofd_libdir="$ROFD_LIBDIR" -Drofd_includedir="$ROFD_INCLUDEDIR"
#   PKG_CONFIG_LIBDIR=/tmp/pc-norofd LD_LIBRARY_PATH="$ROFD_LIBDIR" meson test -C _build-fork

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
xdg-mime query filetype <fixture>.ofd                  # MIME detection (installed or staged tree)

# The previewer's own invocation (Nautilus passes --fd and the sniffed type);
# fd 3 is the fixture, inherited across exec:
papers-previewer --fd 3 3< <fixture>.ofd
```

Packaging and install checks (Arch; no container, the package is built and
installed on this machine):

```bash
# build + verify the package from the AUR recipe (source = pinned commit archive
# of the public repository; the checksum is checked on every build)
git clone https://aur.archlinux.org/papers-ofd.git /tmp/ofd-pkg && cd /tmp/ofd-pkg
makepkg -f --noconfirm          # runs check(): 11 tests, ofd-print-file skips headless
namcap papers-ofd-*.pkg.tar.zst # expect only the gcc-libs advisory

# staged-tree checks without installing: extract, index, query
mkdir -p /tmp/ofd-stage && bsdtar -xf papers-ofd-*.pkg.tar.zst -C /tmp/ofd-stage
mkdir -p /tmp/ofd-xdg/mime/packages /tmp/ofd-xdg/applications
cp /tmp/ofd-stage/usr/share/mime/packages/papers-ofd.xml /tmp/ofd-xdg/mime/packages/
cp /tmp/ofd-stage/usr/share/applications/org.gnome.Papers.Ofd.desktop /tmp/ofd-xdg/applications/
update-mime-database /tmp/ofd-xdg/mime && update-desktop-database /tmp/ofd-xdg/applications
export XDG_DATA_DIRS=/tmp/ofd-xdg:/usr/share
desktop-file-validate /tmp/ofd-stage/usr/share/applications/org.gnome.Papers.Ofd.desktop
xdg-mime query filetype <fixture>.ofd          # application/ofd
xdg-mime query default application/ofd         # org.gnome.Papers.Ofd.desktop

# install (password needed: the user's step) and uninstall regression
sudo pacman -U papers-ofd-0.1.0-1-x86_64.pkg.tar.zst
sudo pacman -Rns papers-ofd                    # then: papers <some>.pdf still opens
```

Fixture matrix (generated by `tests/generate_fixture.py <kind> <output.ofd>` so
they stay tiny and deterministic; large or third-party fixtures are referenced by
path, not copied — paths below are relative to `$ROFD_SRC`). Text fixtures are
sized for rofd's *fallback* metrics: with the named font absent it advances every
glyph by about `0.96 × Size` millimetres, so a line of 27 ASCII characters at
`Size=3` spans ~78 mm — geometry the text/selection tests rely on:

| Fixture (kind) | Proves |
| --- | --- |
| `geometry` (`geometry-3pages.ofd`) | page count and non-uniform page-size path in `setup_cache` (A4/A5/landscape) |
| `corner-marker` | rotation matrix in the view and in print output |
| `origin-offset` (`PhysicalBox "10 20 210 297"`) | origin normalization (§4) |
| `metadata` (title/author/subject/keywords/creator/dates + ASCII and CJK text) | metadata mapping, same-options equivalence, print text selectability |
| `latency` (`latency-300pages.ofd`, content per page) | open-latency comparison for both page-size paths |
| `text` (`text-2pages.ofd`) | text/layout/find/selection/caret: a two-column ASCII page (left column inside x ≤ 112 mm, right column 120–195 mm) and a CJK page with a phrase that occurs twice |
| `scan` (`scan-1page.ofd`) | render + thumbnail of a non-text page: one embedded PNG and no text |
| `links` (`links-2pages.ofd`) | page links (page-root URI, object GOTO, object URI with `Base`), a 4-node outline with a nested entry, a URI entry and a named-bookmark GOTO |
| `origin-text` (`origin-text.ofd`) | origin normalization for text, layout, find, selection *and* link regions/destinations (`PhysicalBox "10 20 210 297"`) |
| `broken-page` (`broken-page-2pages.ofd`) | a page whose content XML is truncated: page 0 works, page 1 is unreachable and every vfunc tolerates NULL |
| `malformed-zip` (`malformed-zip.ofd`) | zip magic with `OFD.xml` at byte 30 but no valid archive: detected as `application/ofd`, load fails with `G_IO_ERROR_INVALID_DATA` |
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
| `/usr/share/mime/packages/papers-ofd.xml` | registers `application/ofd`: glob `*.ofd`, `sub-class-of application/zip`, nested magic at **priority 70**: `PK\x03\x04`@0 plus `OFD.xml`@30 **or** `Doc_0/`@30 (producers differ in what the first zip entry is; the invoice fixture starts with `Doc_0/Annots/Annotations.xml`, so an `OFD.xml`-only rule would fall back to `application/zip` on real invoices) |
| `/usr/share/thumbnailers/papers-ofd.thumbnailer` | `Exec=papers-thumbnailer -s %s %u %o`, `MimeType=application/ofd;` |
| `/usr/share/applications/org.gnome.Papers.Ofd.desktop` | `NoDisplay=true`, `Exec=papers %U`, `MimeType=application/ofd;` |
| `/usr/lib/sushi/plugins-1/papers-ofd-sushi.js` | makes Sushi's document renderer claim `application/ofd`, so the space-bar preview shows the page instead of file information (A26); `-Dsushi_plugin_dir` moves it, empty skips it |

- Use the distinct MIME file name `papers-ofd.xml`: another package may already
  install `ofd.xml` for the same type (the dev host has `deepin-reader`'s). Both
  definitions merge in `update-mime-database`; ours adds the magic rules.
- The magic **priority matters**: `application/zip` matches `PK\x03\x04` at
  priority 60, so a nested `application/ofd` rule below that loses every
  content-only guess and content sniffing answers `application/zip` — which is the
  only detection the previewer and `load_fd` have (A22). 70 is the priority the
  shared-mime-info container formats use (EPUB, ODF). `tests/test-ofd-errors.c`
  asserts both halves of this: an OFD file is `application/ofd` from its bytes
  alone, and a zip whose first entry is not `OFD.xml`/`Doc_0/` stays a zip.
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

### Arch package (the delivery route)

The recipe is the AUR repository
[`papers-ofd`](https://aur.archlinux.org/packages/papers-ofd) (`0.1.0-4` as of
2026-10-15) — maintained there and nowhere else, so there is no copy to keep in
sync (the same holds for `librofd-ffi` and its `release/*` branches). `debian/` follows only
when a non-Arch consumer appears.

- `depends`: `librofd-ffi>=0.4.3` (the ABI plus its `rofd-ffi.pc`, which the build
  uses instead of `-Drofd_libdir`), `papers>=51` and `papers<52` (the backend-ABI
  major is the `papers/6` directory component), `cairo`, `gcc-libs`, `glib2`,
  `glibc`. `shared-mime-info` and `desktop-file-utils` are deliberately **not**
  listed: their pacman hooks index our MIME xml and desktop file, and `papers`
  already requires both (`pactree -u papers` shows them) — namcap agrees.
- `makedepends`: `meson`, `gtk4`, `poppler-glib` (the test suite links them).
- `source`: the commit-pinned archive of `YangtseSu/papers-ofd` (public since
  2026-10-15). A recipe cannot pin the commit that contains it, so the pinned tree
  is one revision behind these docs; the AUR repository is the published recipe and
  its only copy.
  `makepkg` verifies the sha256, builds with `--buildtype=plain` so
  the makepkg `CFLAGS`/`LDFLAGS` apply, and runs `check()` (`meson test`: 11 tests,
  `ofd-oracle` needs `-Drofd_src`; `ofd-print-file` skips itself when GTK has no
  display). The package was verified by a build with an empty `SRCDIR`, i.e. from
  the anonymous download, and by `namcap` (only the `gcc-libs` advisory).
- Install layout is exactly the five files in the table above; none is owned by
  another package, so uninstall (`pacman -Rns papers-ofd`) removes only our files
  and cannot break Papers' own backends.

## 8. Risks and open questions

| Risk / question | Impact | Mitigation / next step |
| --- | --- | --- |
| `setup_cache` fetches and parses every page at load (rofd's FFI has no size-only call; `Document::page_size()` exists in rofd-core but is unexported) | open latency on large documents | request `rofd_document_get_page_size_mm` (§9); meanwhile measure on a 300-page fixture and publish the number |
| Rotation direction mismatch | all rotated views wrong | corner-marker fixture in phase 1; fallback mapping is a one-line change |
| `render_selection` cannot recolour glyphs | highlight covers text on builds without transparent selection | accept; optional "text mask" render mode request (§9). The host's poppler 26.08 has transparent selection, so the degradation is not visible here |
| Multi-line find matches collapse into one rectangle | result rows differ from the PDF backend | accept and document |
| Selection on a rotated page | no highlight and no region | Papers never starts one (it requires rotation 0, `libview/pps-view.c:4512`); the module declines instead of drawing rectangles in the wrong place |
| A find result cap of 4096 per page | documents with more occurrences on one page lose the tail on that page | documented constant (`OFD_FIND_MAX_RESULTS_PER_PAGE`); rofd itself bounds a page's matches by its own resource limit, and the shell materialises a result row per match |
| `find_link_dest` / `find_link_page` stubs | named destinations / history restore incomplete | revisit if rofd gains a name→page lookup |
| `application/ofd` may already be defined by another package | duplicate definition | distinct file name + magic rules; verify with `xdg-mime query filetype` |
| Stock rofd sizes text runs without an explicit `DeltaX` by the font size while the renderer draws their font advances | search highlights, caret positions and selection regions drifts off the glyphs (observed: a `42` match rectangle 4.5 mm right of the end of the line) | fixed on the fork and shipped in release `v0.4.3-fork.1` (A24, upstream PR #13); a stock library keeps rofd's model, documented in `README.md`, and the ink-alignment test only asserts when the linked ABI is font-aware |
| `PpsSearchContext` cannot be finalized (A23) | a crash in any Papers process that disposes a search context — not reachable from this module, but the shell may hit it when it drops a context | documented, worked around in the headless search test (no dispose), worthy of an upstream report in phase 4 |
| A nested magic rule below `application/zip`'s priority | content-only detection (previewer, `load_fd`) answers `application/zip` and the document never reaches the module | priority 70 (A22) plus the byte-level assertion in `tests/test-ofd-errors.c` |
| rofd's ABI opens by path, so fd loading needs `/proc/self/fd`; a sandbox without `/proc` (or a descriptor whose file cannot be re-entered) breaks it | the previewer would not open any OFD file | temp-copy fallback below `g_get_user_cache_dir()/papers-ofd`, removed on reload and at finalize (§3); both halves are observed by `tests/test-ofd-fd.c` (A25) |
| Sushi derives its renderer's content types from `org.gnome.Papers.desktop`, so out of tree `application/ofd` has no preview renderer (A26) | the space bar shows only file information for OFD files | `data/papers-ofd-sushi.js` adds the type from Sushi's plugin directory; the coupling to Sushi internals is bounded by `tests/test-sushi-plugin.js`, which drives Sushi's own selector and renderer, and the plugin disappears with the phase-4 move |
| Two desktop entries that both run `papers` (`org.gnome.Papers.desktop` and our `NoDisplay` association entry) show up as two rows in Nautilus' "Open With" chooser, which lists every installed application (`nautilus-app-chooser-widget.c:301-330`, A26) | cosmetic duplicate in that dialog only: the app grid, the window grouping and the context menu's single "Open" entry show one Papers | accepted for the out-of-tree phase: `application/ofd` is registered on exactly one entry (`gio mime application/ofd`), and the duplicate disappears when Papers' own MimeType line carries the type (phase 4), which also retires our entry |
| rofd falls back to a uniform glyph advance when a font is missing | fixture text is wider than a real font would set it | size fixture text for `0.96 × Size` per glyph (§6); real documents still render through the font fallback chain |
| No CJK font installed | missing-glyph boxes | rofd defaults already name `Noto Sans CJK SC`; distro packaging should recommend a CJK font |
| Papers 52 bumps `pps_document_current` | module installed under `papers/6/backends` stops being found (fail-soft) | pin `>= 51, < 52`; §3 is the contract to re-verify |
| Fixture licensing | cannot redistribute spec/ofdrw fixtures | generate our own; reference external fixtures by path |
| The carried page-size query (A18) diverges from upstream or never lands | a release-only change to maintain; stock rofd keeps working through the fallback path | ship it in the fork's `release/*` branch (`patches/README.md`), never as an AUR dependency; re-check at every release rebase and drop it once upstream has the function |
| rofd older than 0.4.2 (or a stock package built from `v0.4.0`) fails entire pages whose objects carry a singular transform | blank pages on real-world documents, no error visible in the UI | pin ≥ 0.4.2 (A19): our AUR package builds the fork release (upstream `0.4.3` + carried changes); document the requirement in `README.md` |

## 9. Upstream requests to rofd (ranked)

Policy (D11): these are *requests*, not dependencies. Everything below is
implemented in our fork and delivered in the fork's tagged releases
(`release/*` branches, `patches/README.md` is the inventory); the AUR
`librofd-ffi` package builds a release tarball, so no item blocks a phase and
the AUR carries no patch files. An upstream merge only feeds into the
next `release/*` rebase: the change stops being a carried row in
`patches/README.md` and comes from upstream with the next release.

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
   Status (2026-10-15): **implemented on the fork and shipped** —
   <https://github.com/linuxdeepin/rofd/pull/11> (branch `feat/ffi-page-size-query`
   at `7f6d24d` on `YangtseSu/rofd`) is the request; the delivery is the fork's
   release `v0.4.3-fork.1` (`release/0.4.3-fork.1`, upstream `140b936` =
   `0.4.3` plus the carried changes), which the AUR `librofd-ffi`
   `0.4.3.fork.1-1` builds — that release is the ABI this module is built and
   tested against. Verification at release time: `cargo fmt --all -- --check`,
   `cargo clippy --offline -p rofd-core -p rofd-render -p rofd-ffi
   --all-targets -- -D warnings`, `cargo test` for the three crates,
   `crates/rofd-ffi/tests/run_c_tests.sh` (78-symbol export list, C/C++
   header, smoke); the packaged library's exported set matches
   `expected-symbols.txt` and papers-ofd's meson suite is 12/12 against it.
   With D10's probe a stock-rofd build still works through the slow path, so
   the release and stock packages are interchangeable by construction.
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
   Owned, not blocked (D11): the template ships in the fork source tree
   (`debian/rofd-ffi.pc.in`, so the Debian package and our release tarball
   describe the same file) and the AUR package installs it from there.
   Status (2026-10-15): submitted upstream as
   <https://github.com/linuxdeepin/rofd/pull/12> (branch `feat/pkgconfig`: the
   template + the `librofd-ffi-dev` install rule, version from
   `dpkg-parsechangelog`, `DEB_HOST_MULTIARCH`, `Requires: cairo`); carried in
   release `v0.4.3-fork.1` and installed by AUR `0.4.3.fork.1-1` with Arch's
   single-arch `libdir` substituted for the Debian `@MULTIARCH@`.

6. **Font-resolved metrics for runs without an explicit `DeltaX`.** rofd's page
   text geometry must agree with the glyphs the renderer draws; today the semantic
   layer advances such scalars by the font size and bounds their box by it too (A24,
   A27), so every consumer that draws or hit-tests from those rectangles is offset
   for producers that rely on font metrics. Shape agreed with upstream in the PR: the
   core keeps its font-free design and takes the geometry through an injected
   `TextMetricsProvider`, which `rofd-render` implements over the positioner's own
   resolution and `rofd-ffi` supplies from the default font stack.
   Status (2026-10-15): **implemented on the fork and shipped** —
   <https://github.com/linuxdeepin/rofd/pull/13> (branch `feat/font-text-advances`)
   is the request; the delivery is the fork's release `v0.4.3-fork.1`, built
   by AUR `librofd-ffi 0.4.3.fork.1-1`. Three commits:
   the provider seam, a resolved box sized by its glyph's advance instead of floored
   at the font size, and the box spanning that *face's* typographic ascent and descent
   with a selection region grown by an eighth of its height on both sides, the model
   poppler gives a PDF text box and selection region (measured on a 3 mm line: the `on`
   match box went from 4.82 mm to 3.65 mm; on the user's real invoice 86% of 2464
   glyphs reach below their baseline, mean 0.50 mm and up to 1.46 mm, which the band
   now covers and clears).
   Verified at release time: `cargo fmt --all -- --check`,
   `cargo clippy --offline -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings`,
   `cargo test` for the three crates (including two pixel-level tests that fail with
   the box change reverted), `crates/rofd-ffi/tests/run_c_tests.sh`, and
   papers-ofd's 12/12 meson suite against the packaged library.
   One caveat documented in `rofd.h`: a renderer created with fallback families
   other than the defaults may draw glyphs the semantic layer did not resolve.

7. **Decoder allocation budget** — a bug fix, proposed upstream and carried until it lands.
   `crates/rofd-render/tests/images.rs::decoded_limits_have_exact_and_one_over_boundaries`
   is red on upstream `main` (no CI job runs the suite): `images.rs` hands the caller's
   `max_decoded_image_bytes` to the image crate's `Limits::max_alloc`, which budgets
   *every* allocation the decoder makes — its own sample buffer, palette and row buffers
   included — so an image whose decoded RGBA8 buffer exactly fills the byte budget is
   rejected with "Memory limit exceeded". The `fix/decoded-image-alloc-budget` branch
   (<https://github.com/linuxdeepin/rofd/pull/14>) allows three times the finished size
   (decoder buffer ≤ 8 bytes per pixel for 16-bit channels plus the 4-byte-per-pixel
   RGBA8 conversion), at both decode sites — `ImageDecoder::decode` and
   `cairo_renderer::decode_seal_raster`, the stamp-annotation path, where the old budget
   silently dropped seals whose decoded raster sits between roughly half and all of the
   document byte limit (measured: a 6000×6000 16-bit seal, 137 MiB raster, fails at the
   old budget and decodes now; 14336² still fails).
   **Carried** in the fork's release `v0.4.3-fork.1` (cherry-picked whole from
   `fix/decoded-image-alloc-budget`), because it changes rendered output —
   so the regression test and its 430 KiB binary fixture ship in the release
   tarball, and at release time the previously red
   `decoded_limits_have_exact_and_one_over_boundaries` passes (1/14 →
   14/14 on `--test images`). No limit is relaxed: the raster checks stay
   the authority. `patches/README.md` has the retirement
   criterion.

8. **Region pixel-diff tolerance** — the same class of upstream-side repair, proposed as
   <https://github.com/linuxdeepin/rofd/pull/15> (`fix/region-aa-tolerance`).
   `crates/rofd-ffi/tests/region.rs` bounds the anti-aliasing difference between a
   full-page and a region rendering with absolute numbers (32 channels, delta ≤ 2) that
   only hold on one font stack: Arch/FreeType 2.13 gives 21 channels at delta ≤ 2,
   ubuntu-24.04 with the same FreeType gives 42 at delta ≤ 8, so a clean Debian
   build-dependency environment fails the test. The branch bounds the difference by its
   share of the tile's channels (1 %) and the per-channel delta (16); a stitching or crop
   error moves whole pixels, i.e. orders of magnitude more channels with far larger
   deltas, so the check keeps its meaning. Carried in release `v0.4.3-fork.1`
   (cherry-picked whole); at release time the previously red region diff asserts
   gated again, and the fork CI's non-gating split for it becomes a leftover to
   trim on the next release.

## 10. Deferred: annotation editing

Papers' bottom-right edit button and the whole annotation toolset appear as soon as
a backend implements `PpsDocumentAnnotations` with a non-NULL `add_annotation`
(`shell/src/document_view.rs:486-516`; `can_add_annotation` is literally "the vfunc
is assigned", `libdocument/pps-document-annotations.c:53-58`). The interface is
small — five vfuncs, of which `get_annotations` and `PpsDocument::save` are
dispatched without NULL checks (`pps-document-annotations.h:37-48`,
`pps-document.c:461-469`) — but the feature behind it is not: the UI offers
highlight/underline/strikeout/squiggly, a note, free text, ink, an eraser and a
handwritten stamp at once; committed annotations are displayed by the document
render, so the render must include in-memory ones (`libview/pps-pixbuf-cache.c:616-628`
plus the re-render on add, `libview/pps-view.c:2885-2898`); and persistence is
save-as only, through `pps_document_save` with a temp path
(`libview/pps-jobs.c:1105-1183`).

Neither half exists today. `linuxdeepin/rofd` parses and renders annotation
appearances but has **no** writer and exposes **no** annotation function among its 78
C symbols (`crates/rofd-ffi/tests/c/expected-symbols.txt`). `office-rs/rofd` is the
only checkout with a writer — annotation model, appearance synthesis, surgical save,
transactions — and was therefore surveyed in full, but it is not a candidate engine:
it rasterizes through vello/wgpu with no cairo or pixel-export path
(`crates/render/src/lib.rs:57`, `crates/xilem-view/src/ofd_widget.rs:544-545`),
declares full-fidelity rendering (template inheritance/JBIG2/tiles) a non-goal, has
no find/links/outline/print, and has no C ABI (`AGENTS.md:16`, `rust-toolchain.toml:3`).

D12 fixes the shape of that future phase: write the OFD side against the standard —
clause 15's entry file with required `PageID`+`FileLoc` and per-page `PageAnnot`
files, annex A.3/A.4's `Annot` (required `ID`/`Type`/`Creator`/`LastModDate`,
optional `Subtype`/`Visible`/`Print`/`NoZoom`/`NoRotate`/`ReadOnly`, children
`Remark?`, `Parameters?`, and a required `Appearance` `CT_PageBlock` with `Boundary`),
`Type` limited to `Link`/`Path`/`Highlight`/`Stamp`/`Watermark` with the variant in
`Subtype` (spec clause 15, tables 60-62 and annex A.4; the standard's PDF in
`$ROFD_SRC/learning/` is a page scan) — inside the fork, keeping `office-rs/rofd` as
a read-only reference for the craft: kind→`Type`/`Subtype` mapping
(`crates/io/src/serialize/annotation.rs:135-153`), the three-level geometry
(`Appearance@Boundary` page-absolute, object `Boundary` `0 0 w h`, `AbbreviatedData`
object-local), object-id minting with `MaxUnitID` write-back and the surgical save
(`crates/io/src/save.rs:14-79,137-247`), and the drawing constants
(`crates/render/src/annotation_scene.rs:47,56,62,66`).

Work streams when taken up: (1) `rofd-core` writer — annotation draft → appearance
`PathObject`/`ImageObject`, surgical package rewrite, in-memory annotations in the
display list; (2) additive `rofd-ffi` calls (list/add/remove annotations, save) with
`expected-symbols.txt` and C tests; (3) the module — all five
`PpsDocumentAnnotations` vfuncs, the `PpsAnnotation`↔OFD mapping, a real `save`, and
in-memory drawing that honours `rc->annot_flags`. Acceptance for that phase: an OFD
saved from Papers reopens in Papers and renders in a third-party reader, the real
invoice's own annotations still render, and the eraser/undo paths work on
not-yet-saved annotations.

Known risks: in-memory versus baked-in annotations (rofd draws the file's
annotations into the page, so the module must not draw them twice); the stamp's new
image resource (MultiMedia entry + `Res.xml` + `ImageObject@ResourceID`, which
`office-rs/rofd` deliberately does not write); `Type` tolerance (our reader skips an
`Annot` whose `Type` is outside the five standard values —
`crates/rofd-core/src/document.rs:1055-1080` — so a `Type="FreeText"` like the one
real producers emit needs a decision); and the verification gap (no third-party OFD
reader is automated here). Full evidence, including the pitfalls not to copy, is in
`.planning/2026-10-14-papers-ofd-backend-plan-and-readme/findings.md`
(§"Annotation editing", N1-N29).

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
| A19 | rofd ≤ 0.4.0 **aborts the whole page render** with `ROFD_STATUS_INTERNAL` on a document object carrying a singular transform (degenerate CTM, e.g. the empty text objects some converters emit); `0.4.2`+ maps the diagnostic to `ROFD_DIAGNOSTIC_SINGULAR_TRANSFORM` (7) and rendering continues. ABI v1 and the pinned 77-symbol set are unchanged, so the fix is free for us — but it makes 0.4.2 the minimum that matches D9 ("never fail a render because of a diagnostic") | `$ROFD_SRC/crates/rofd-ffi/include/rofd.h:75,147`; `$ROFD_SRC/crates/rofd-ffi/tests/c/expected-symbols.txt` (77); upstream commit `8cfb2f7` on `main` ("fix(ffi): map SingularTransform diagnostic to C ABI v1") |
| A20 | `render` is dispatched without a NULL check as well (`pps_document_render` calls `klass->render` directly), so a module that implements loading but no rendering crashes Papers as soon as it displays the first page — a phase-0 skeleton without `render` segfaults (observed: exit 139) | tag `51.1` `libdocument/pps-document.c:637-643`; observed Papers run in phase 0 |
| A21 | `PpsOutlines` has declared-but-undefined public functions: `pps_outlines_new` and `pps_outlines_set_link` are in the installed `pps-outlines.h` but neither the source nor the library provides them, so a module that calls them fails to link (`-Wl,--no-undefined`, observed). Use `g_object_new (PPS_TYPE_OUTLINES, NULL)` and the `link` property (whose setter takes its own reference); `pps_outlines_set_children` **is** defined and takes a reference, while the `children` property setter stores a borrowed pointer | `libdocument/pps-outlines.[ch]` (`new`/`set_link` declared, no definition); `nm -D --defined-only /usr/lib/libppsdocument-4.0.so.6 \| grep outlines` lists `get_type`, `get/set_children`, `get_expand`, `get_link`, `set_expand`, `set_label`, `set_markup` only; the module link error on `pps_outlines_new` |
| A22 | MIME detection from bytes alone (no file name) answers `application/zip` for OFD files when the nested `application/ofd` magic rule is below `application/zip`'s priority 60: the nested rule only wins at 70, which is what the shared-mime-info container formats use. Measured with `g_content_type_guess (NULL, first 1024 bytes, n)` after `update-mime-database` for a generated fixture, the real invoice and a malformed zip (all `application/ofd` at 70, all `application/zip` at 50); a plain zip and a zip whose first entry is `hello.txt` stay `application/zip` at 70. The previewer and `load_fd` only have bytes (`libdocument/pps-file-helpers.c:337-378`, `previewer/pps-previewer.c`) | `data/papers-ofd.xml` (priority), `tests/test-ofd-errors.c` (`test_detection_from_bytes`), `/usr/share/mime/packages/freedesktop.org.xml` (`application/zip` at priority 60, `application/epub+zip` nested at 70) |
| A23 | `PpsSearchContext` cannot be finalized: its dispose calls `g_clear_object ()` on the `GHashTable *per_page_store` instead of `g_clear_pointer (..., g_hash_table_unref)`, and `g_object_unref` then reads the hash table as a `GTypeInstance` and segfaults. Any process that drops the last reference to a search context crashes; the headless search test therefore leaks the context and the model on purpose | `libview/context/pps-search-context.c:421` (dispose) vs `:433` (the field is a `GHashTable *` created in `init`); observed: `tests/test-ofd-search` segfaults in `g_type_check_instance_is_fundamentally_a` under `g_object_unref` from that dispose, with the installed `libppsview-4.0.so.5`; the same code is in tag `51.1` (the only `libview` difference between the checkout and the tag is `pps-print-operation.c`) |
| A24 | rofd's page text geometry positioned a scalar without an explicit `DeltaX` by the font size, while the renderer positions it by the resolved font's advance ("only a genuinely absent axis uses the selected glyph advance"), so layout, search and selection rectangles drifted from the drawn glyphs. Measured on the phase-2 text fixture (21-character line, size 3 mm, no `DeltaX`): boxes spanned 63.0 mm and the ink 28.2 mm, and the `42` match rectangle sat at 60.0–66.0 mm while the drawn line ends at 43.4 mm. Fixed on the fork by resolving advances through an injected provider and sizing a resolved box by that advance (`patches/README.md`; the fork's release `v0.4.3-fork.1`, upstream PR #13): the same line then spans 29.8 mm and the `42` match rectangle sits at 33.9–38.6 mm inside the ink, while a two-scalar match like `on` narrows from 4.82 mm to 3.65 mm — the sum of the two glyph advances | `crates/rofd-core/src/semantic.rs` (`text_object`: `dx = … else size`) vs `crates/rofd-render/src/fonts.rs` (`position_glyph_runs`, advance from the selected face); measured with a throwaway renderer + ink scan against the installed `librofd_ffi.so.0` and the fork build; guarded by `tests/test-ofd-text.c::test_find_rectangles_cover_rendered_ink` |
| A27 | rofd's character box spanned exactly one font size above the baseline and stopped at it, while a renderer paints the glyph below it and a PDF text box is built from the font's metrics — so a highlight cut the bottom off its text (CJK overshoot, Latin descenders) and its vertical extent depended on the glyph inside it, leaving punctuation and Latin unaligned with CJK. Measured on a real-world invoice kept outside this repository (six embedded subset fonts, sizes 4.9–7.8 mm): of 2464 glyphs with ink, 2117 (86%) reach below the baseline — CJK 0.098–0.129 em, Latin descenders 0.188 em — mean 0.50 mm and up to 1.46 mm below the old box, while the box top sat 1.45 mm above the painted top on average. Fixed on the fork by taking the box's extents from the face's FreeType metrics and growing a selection region by an eighth of its height on both sides, both as poppler does (the fork's release `v0.4.3-fork.1`, third commit, upstream PR #13) | `crates/rofd-core/src/semantic.rs` (`text_object`: `top = … - size`, `bottom = y.max (next_y)`; `selection_region`) vs `crates/rofd-render/src/fonts.rs` (`typographic_extents_mm`, FreeType `ascender`/`descender`); the model is poppler's: `TextWord::setInitialBounds` `yMin = y - ascent`, `yMax = y - descent` and `TextSelectionSizer::visitLine` `margin = (yMax - yMin) / 8` (poppler `poppler/TextOutputDev.cc`, master 2026-10-15, lines ~501 and ~4395), which Papers' PDF backend consumes through `poppler_page_get_selected_region` (`libdocument/backend/pdf/pps-poppler.c:2010-2035`); measured on a real-world PDF kept outside this repository (body band 15.87 pt for a 15.95 pt em, its selection region 20 pt, its find rectangle 15.87 pt) and on the OFD with a throwaway cairo overlay harness (page 1: body regions y 43.89..50.94 mm for ink 44.71..50.01, identical across the four objects of the line); guarded by `crates/rofd-render/tests/cairo_text.rs::semantic_character_boxes_cover_the_rendered_ink`, `crates/rofd-ffi/tests/semantic.rs::text_layout_rectangles_cover_the_rendered_ink_below_the_baseline` and `tests/test-ofd-text.c::test_selection_region_covers_rendered_ink` |
| A25 | `/proc/self/fd/<n>` re-enters the file a descriptor points at, which is what lets `rofd_document_open` (a path-based API) read a document the caller has already unlinked; the re-open starts at offset 0 no matter where the descriptor itself sits, and it fails with `EACCES` once read permission has been withdrawn after the `open` (the descriptor keeps reading, because permissions are checked at `open` time). The first half is why `load_fd` hands `dup(fd)` to rofd through that path; the second is why a temp copy exists for environments without a usable `/proc`, and it is also the test's deterministic trigger for that fallback | Measured on this machine (Linux 7.2.10-1-cachyos): a deleted file, a `memfd` and a descriptor seeked to offset 7 all return the whole file when reopened through `/proc/self/fd/<n>`; after `chmod 000`, `open` on that path fails with `EACCES` while `pread` on the open descriptor still returns the bytes. `tests/test-ofd-fd.c` covers the deleted-file case (loads, and no temp copy is created) and the `chmod 000` case (the copy appears in `$XDG_CACHE_HOME/papers-ofd` and is gone after a reload or at finalize) |
| A26 | The space bar in GNOME Files is **Sushi**, and Sushi renders documents in-process through Papers' `libdocument`/`libview` (its `js/viewers/pdf.js` imports `gi://PapersDocument`/`PapersView` and loads with `PapersView.JobLoad.set_uri`, i.e. by URI — no file descriptor, so `load_fd` is not on that path). Which renderer a file gets is decided by content type: the selector matches `Gio.content_type_is_a` against every renderer's `contentTypes`, and the document renderer derives its list from the MimeType line of `org.gnome.Papers.desktop`; `application/ofd` is not in it out of tree, so the type falls through to the file-information renderer. Sushi loads plugins from its built-in resources, then `SYSTEM_PLUGIN_DIRECTORY` (`/usr/lib/sushi/plugins-1`), then `$XDG_DATA_HOME/sushi/plugins-1`, importing them before it builds the map, so a plugin can add the type (the renderer's `contentTypes` array is mutable and is the same array its own load gate reads). Nautilus' "Open With" chooser lists `g_app_info_get_all()` on top of default/recommended/fallback, which is why both desktop entries that run `papers` appear there while `gio mime application/ofd` reports exactly one registered entry | Sushi 51.2 sources (`src/core/rendererSelector.js`, `src/viewers/pdf.js:46,122-129`, `src/config.js.in`) and the installed resource (`gresource list /usr/share/sushi/org.gnome.NautilusPreviewer.src.gresource`); measured with gjs against the installed 51.2: without the plugin `selectRenderer ("application/ofd")` is `FallbackRenderer`, with it the Papers-backed `PdfRenderer` and the fixture loads with 2 pages; `xdg-mime query filetype`, `gio mime application/ofd` (default and recommended: `org.gnome.Papers.Ofd.desktop`), `nautilus-app-chooser-widget.c:301-330`; `tests/test-sushi-plugin.js` is the regression net |

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
