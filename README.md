# papers-ofd

OFD (GB/T 33190-2016) backend for [GNOME Papers](https://gitlab.gnome.org/GNOME/papers),
built out-of-tree on the [`rofd-ffi`](https://github.com/linuxdeepin/rofd) C ABI
(rofd's OFD parser and Cairo renderer).

Ships one shared module and four small data files into Papers' existing backend
directory. No Papers fork, no Papers rebuild, no patching of Papers sources.

## Status

Phases 0, 1 and 2 are implemented and tested against **Papers 51.1** ([tag `51.1`](https://gitlab.gnome.org/GNOME/papers/-/tree/51.1);
the build needs only the installed `papers-document-4.0` headers) and **rofd ABI v1**:
the module loads through Papers' own backend discovery, renders pages correctly at
every rotation and zoom step, prints (vector text), reports metadata, and serves
text, selection, search, page links and the outline.

- `meson test -C _build` runs the unit conversions, the factory round-trip with a
  deliberate discovery failure, the rotation/zoom/origin render matrix, the
  same-options equivalence against a direct FFI render, a real GTK print-to-file
  whose PDF text is verified with poppler, the metadata fields and mask bits, the
  text/layout/find/selection checks and the link/outline model — each compared
  against a direct rofd query of the same fixture — the load error paths, and
  (with `-Drofd_src=<rofd checkout>`) the invoice oracle against rofd's reference
  render.
- Remaining: fd loading, packaging and desktop integration (phase 3), upstream MR
  (phase 4).

Implementation plan: [`docs/plan.md`](docs/plan.md) — phases, interface contract,
acceptance criteria, verification playbook, risks ([§5](docs/plan.md#5-phases)).
Research notebook (raw evidence with `file:line`): `.planning/` in the working
copy.

## Capabilities

| Capability | Notes |
| --- | --- |
| Open `*.ofd` (double-click, `papers file.ofd`, `gio open`) | local files; the fd/previewer path is phase 3 |
| Scroll, zoom, rotate, fit-width/fit-page | full-page Cairo raster from `rofd-render` |
| Print and print-to-PDF | direct Cairo rendering into the print context, vector text |
| Thumbnails (sidebar and Nautilus) | reuse of the page renderer |
| Metadata | title, author, subject, keywords, creator, dates, page count |
| Select/copy text, Ctrl+F, caret navigation | rofd's text layout + search; the I-beam cursor comes from the glyph region |
| In-page links and the outline sidebar | GOTO/URI links; the outline model feeds the links sidebar |

Not supported (rofd has no support for them, and Papers hides the corresponding
UI automatically): annotations, signatures/seals, form filling, attachments,
layers, transitions, editing/saving.

## How it works

Papers discovers backends at runtime by scanning
`<libdir>/papers/<api>/backends` (`pkg-config --variable=backenddir
papers-document-4.0`, currently `/usr/lib/papers/6/backends`) for
`*.papers-backend` key files:

```ini
[Papers Backend]
Module=ofddocument
TypeDescription=Open Fixed-layout Documents
MimeType=application/ofd;
```

It loads `libofddocument.so`, calls the exported `pps_backend_query_type()` and
gets an `OfdDocument` — a `PpsDocument` subclass that also implements
`PpsDocumentText`, `PpsDocumentFind`, `PpsDocumentLinks`, `PpsSelection` and
`PpsDocumentPrint`. Each `PpsPage` carries its own `rofd_page_t`, and every call is
a thin translation into rofd's C ABI (units, coordinates, options, errors). No
document-wide render lock: the ABI is safe for the concurrent read-only calls that
Papers' worker pool issues.

## Requirements

| Dependency | Notes |
| --- | --- |
| Papers ≥ 51, < 52 (`papers-document-4.0`) | system pkg-config; backends' ABI version is the `papers/6` directory component |
| rofd-ffi (ABI v1, ≥ 0.4.2) | `librofd_ffi.so.0` + `rofd.h`; no pkg-config file from upstream, but our AUR package `librofd-ffi` (currently `0.4.3-1`) installs `rofd-ffi.pc` plus the patched ABI, so `-Drofd_libdir` / `-Drofd_includedir` are only needed for a hand-built checkout — `scripts/dev-env.sh` fills them in either way. The patched build carries `patches/rofd-ffi-page-size.patch` (also proposed as [linuxdeepin/rofd#11](https://github.com/linuxdeepin/rofd/pull/11)); stock rofd works too, falling back to the slow page-size path. **Do not go below 0.4.2**: earlier rofd fails whole pages whose objects carry a singular transform ([A19](docs/plan.md#appendix-a--evidence-index)). Delivery policy: [plan D11](docs/plan.md#2-architecture-and-decisions) |
| Papers/rofd sources (optional) | **Not needed to build**: the module compiles against `papers-document-4.0` (headers + `libppsdocument-4.0`) and `rofd.h` + `librofd_ffi.so.0`. `$ROFD_SRC` is required to carry or re-verify `patches/` and to run rofd's own test suite; a Papers checkout (`$PAPERS_SRC`) only speeds up grepping — its citations are written against the upstream repository at tag `51.1`. `scripts/dev-env.sh` reports what this machine has (environment → untracked `local.conf` → siblings `../papers`, `../rofd`) and flags a same-named directory that is not the expected repository |
| Meson ≥ 1.0, pkg-config, C compiler, GLib/GIO/GTK4/Cairo | pulled in through the Papers dependency |
| Cairo + FreeType/fontconfig, a CJK font for CJK documents | rofd resolves glyphs through system fonts; its defaults name `Noto Sans CJK SC` |

`rofd-ffi` sets the SONAME `librofd_ffi.so.0`. A plain `cargo build -p rofd-ffi`
tree needs a `librofd_ffi.so.0` symlink next to `librofd_ffi.so`, otherwise
Papers' `g_module_open` fails with "Failed to load backend for …".

## Build and try it

```bash
# 0. resolve dependencies for this machine (see Requirements)
. scripts/dev-env.sh

# 1. rofd ABI library — only when you have the checkout; the installed
#    librofd-ffi package works too, and step 2 then uses its header/library
if [ -d "$ROFD_SRC" ]; then
  (cd "$ROFD_SRC" && cargo build --release --offline -p rofd-ffi)
fi
if [ "$ROFD_ABI_SOURCE" = checkout ]; then
  ln -sf librofd_ffi.so "$ROFD_LIBDIR/librofd_ffi.so.0"
fi

# 2. this module (-Drofd_src=<checkout> also builds the invoice oracle test;
#    -Drofd_page_size=disabled builds the per-page fallback for latency runs)
meson setup _build \
  -Drofd_libdir="$ROFD_LIBDIR" \
  -Drofd_includedir="$ROFD_INCLUDEDIR" \
  -Drofd_src="$ROFD_SRC"
meson compile -C _build
meson test -C _build

# 3. dev backend directory: the whole system backend directory first — modules and
#    descriptors live together, so descriptors alone break PDF/DjVu/TIFF — then ours
mkdir -p _build/devbackends
cp -a "$(pkg-config --variable=backenddir papers-document-4.0)/." _build/devbackends/
cp _build/src/libofddocument.so _build/data/*.papers-backend _build/devbackends/

# 4. application/ofd comes from our own MIME file; stage it without installing
mkdir -p _build/xdg/mime/packages
cp data/papers-ofd.xml _build/xdg/mime/packages/
update-mime-database _build/xdg/mime

export PPS_BACKENDS_DIR=$PWD/_build/devbackends
export XDG_DATA_DIRS=$PWD/_build/xdg:/usr/share
papers "$ROFD_SRC/learning/test.ofd"
papers-thumbnailer -s 256 "$ROFD_SRC/learning/test.ofd" /tmp/ofd.png   # thumbnails
```

`scripts/dev-env.sh` prints the resolution table on its own
(`PAPERS_SRC`, `ROFD_SRC`, `ROFD_INCLUDEDIR`, `ROFD_LIBDIR`, the ABI tier in use)
and exits non-zero when neither the installed package nor a checkout provides the
ABI. Fixtures named in `docs/plan.md` §6 live under `$ROFD_SRC`.

`PPS_BACKENDS_DIR` replaces the whole backend directory: keep the system
`.papers-backend` files present (as above) or PDF/DjVu/TIFF stop working for that
session. The full validation matrix — fixtures, checks per phase, cross-repo ABI
tests — is in [`docs/plan.md` §6](docs/plan.md#6-verification-playbook).

## Installed files

```
<backenddir>/libofddocument.so                            the module
<backenddir>/org.gnome.Papers.OfdDocument.papers-backend  discovery descriptor
/usr/share/mime/packages/papers-ofd.xml                   application/ofd (glob + zip magic)
/usr/share/thumbnailers/papers-ofd.thumbnailer            Nautilus thumbnails
/usr/share/applications/org.gnome.Papers.Ofd.desktop      association entry (NoDisplay)
```

Papers builds its own desktop and thumbnailer MIME lists from in-tree backends
only, hence the separate association and thumbnailer entries. The MIME magic is a
nested rule at priority 70: below `application/zip`'s priority it would lose every
content-only guess, which is the only detection the previewer has. Details and the
patch-free vs patch-the-desktop tradeoff:
[`docs/plan.md` §7](docs/plan.md#7-packaging-and-desktop-integration).

## Limitations

- **Open latency** on a 300-page generated fixture (content on every page,
  5 runs, best of): load 2 ms, then ~70 ms for the page walk (`setup_cache`,
  which fetches every page object) and ~48 ms for the first rendered page —
  ~120 ms from `pps_document_load` to the first page being visible. The
  metadata-only page-size query (`patches/`, `rofd_document_get_page_size_mm`)
  makes the size walk itself 20 ms instead of 71 ms, but the end-to-end time is
  the same in the shell's flow because `setup_cache` fetches the page objects
  either way; the fast path pays off for callers that only need sizes
  (`tests/ofd-latency.c`, both builds).
- Selection highlights are drawn as filled rectangles; the glyphs underneath are
  not recoloured, because rofd's ABI exposes no text mask. Invisible on systems
  whose poppler has transparent-selection support (the common case).
- A wrapped multi-line search match is reported as a single result rectangle.
- Named destinations (`find_link_dest`/`find_link_page`) are stubs.
- Selection is declined on rotated pages, which is where Papers refuses to start
  one itself ("Selection in rotated documents has never worked", `pps-view.c`).
- At most 4096 search results per page (`OFD_FIND_MAX_RESULTS_PER_PAGE`).
- rofd's render options carry a **single uniform scale**: a render target whose
  aspect ratio differs from the page is not stretched (poppler's backend scales
  the axes independently). Papers always derives target sizes from the page size,
  so this is unobservable there ([§4](docs/plan.md#4-units-coordinates-and-render-math)).

## Roadmap

| Phase | Content | Status |
| --- | --- | --- |
| 0 | Loadable module, discovery, page geometry | done |
| 1 | Rendering, rotation/zoom matrix, metadata, printing | done |
| 2 | Text, selection, find, links, outline | done |
| 3 | `load_fd` (previewer), packaging, install checks | planned |
| 4 | Upstream MR into Papers' `libdocument/backend/ofd/` | planned |

Per-phase deliverables, steps and acceptance criteria:
[`docs/plan.md` §5](docs/plan.md#5-phases).

## Upstreaming

The module uses public `PpsDocument` API only (the private-but-exported helper
symbols are avoided), so phase 4 is a file move plus a MIME-list addition in
Papers' `meson.build`. If the glue code has to change, an assumption in the plan's
interface contract was wrong and should be re-verified instead of patched around.

## License

GPL-2.0-or-later (see `COPYING`), matching Papers. Links rofd, which is
LGPL-2.1-or-later; the combination is compatible.
