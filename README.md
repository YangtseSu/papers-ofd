# papers-ofd

OFD (GB/T 33190-2016) backend for [GNOME Papers](https://github.com/GNOME/papers),
built out-of-tree on the [`rofd-ffi`](https://github.com/linuxdeepin/rofd) C ABI
(rofd's OFD parser and Cairo renderer).

Ships one shared module and four small data files into Papers' existing backend
directory. No Papers fork, no Papers rebuild, no patching of Papers sources.

## Status

Plan reviewed against **Papers 51.1** (`../papers`) and **rofd ABI v1**
(`../rofd`); implementation not started.

- Implementation plan: [`docs/plan.md`](docs/plan.md) — phases, interface contract,
  acceptance criteria, verification playbook, risks ([§5](docs/plan.md#5-phases)).
- Research notebook (raw evidence with `file:line`): `.planning/` in the working
  copy.

## What it will do

When the plan is complete:

| Works | Notes |
| --- | --- |
| Open `*.ofd` (double-click, `papers file.ofd`, `gio open`) | local files; fd/previewer path in phase 3 |
| Scroll, zoom, rotate, fit-width/fit-page | full-page Cairo raster from `rofd-render` |
| Print and print-to-PDF | direct Cairo rendering into the print context, vector text |
| Thumbnails (sidebar and Nautilus) | reuse of the page renderer |
| Metadata | title, author, subject, keywords, creator, dates, page count |
| Select/copy text, Ctrl+F, caret navigation | via rofd's text layout + search |
| In-page links and the outline sidebar | GOTO/URI links; outline from `rofd_document_get_outline` |

Not supported (rofd has no support yet; Papers hides the UI automatically):
annotations, signatures/seals, form filling, attachments, layers, transitions,
editing/saving.

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
| rofd-ffi (ABI v1) | `librofd_ffi.so.0` + `crates/rofd-ffi/include/rofd.h`; no pkg-config file yet, so Meson takes explicit `-Drofd_libdir` / `-Drofd_includedir`. Stock rofd works; a build carrying `patches/rofd-ffi-page-size.patch` (upstream [linuxdeepin/rofd#11](https://github.com/linuxdeepin/rofd/pull/11)) additionally gets the fast page-size path |
| Meson ≥ 1.0, pkg-config, C compiler, GLib/GIO/GTK4/Cairo | pulled in through the Papers dependency |
| Cairo + FreeType/fontconfig, a CJK font for CJK documents | rofd resolves glyphs through system fonts; its defaults name `Noto Sans CJK SC` |

`rofd-ffi` sets the SONAME `librofd_ffi.so.0`. A plain `cargo build -p rofd-ffi`
tree needs a `librofd_ffi.so.0` symlink next to `librofd_ffi.so`, otherwise
Papers' `g_module_open` fails with "Failed to load backend for …".

## Build and try it

```bash
# 1. rofd ABI library (offline: dependencies are vendored)
git clone https://github.com/linuxdeepin/rofd && cd rofd
cargo build --release --offline -p rofd-ffi
ln -sf librofd_ffi.so target/release/librofd_ffi.so.0

# 2. this module
cd ../papers-ofd
meson setup _build \
  -Drofd_libdir=$PWD/../rofd/target/release \
  -Drofd_includedir=$PWD/../rofd/crates/rofd-ffi/include
meson compile -C _build
meson test -C _build

# 3. dev backend directory: ours plus the system descriptors, so PDF keeps working
mkdir -p _build/devbackends
cp _build/src/libofddocument.so _build/data/*.papers-backend _build/devbackends/
cp "$(pkg-config --variable=backenddir papers-document-4.0)"/org.gnome.Papers.*.papers-backend _build/devbackends/
PPS_BACKENDS_DIR=$PWD/_build/devbackends papers ../rofd/learning/test.ofd

# 4. thumbnails
PPS_BACKENDS_DIR=$PWD/_build/devbackends papers-thumbnailer -s 256 ../rofd/learning/test.ofd /tmp/ofd.png
```

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
only, hence the separate association and thumbnailer entries. Details and the
patch-free vs patch-the-desktop tradeoff: [`docs/plan.md` §7](docs/plan.md#7-packaging-and-desktop-integration).

## Limitations

- Opening a large document parses every page up front: Papers asks for every page
  size and the released ABI has no size-only call. The patch in `patches/` adds
  `rofd_document_get_page_size_mm` (rofd-core already implements the query); Meson
  detects it and the module falls back to the per-page fetch when it is absent.
  Open latency is measured and published per [§5](docs/plan.md#5-phases) phase 1.
- Selection highlights are drawn as filled rectangles; the glyphs underneath are
  not recoloured, because rofd's ABI exposes no text mask. Invisible on systems
  whose poppler has transparent-selection support (the common case).
- A wrapped multi-line search match is reported as a single result rectangle.
- Named destinations (`find_link_dest`/`find_link_page`) are stubs.

## Roadmap

| Phase | Content | Plan |
| --- | --- | --- |
| 0 | Loadable module, discovery, page geometry | [§5](docs/plan.md#5-phases) |
| 1 | Rendering, rotation/zoom matrix, metadata, printing | [§5](docs/plan.md#5-phases) |
| 2 | Text, selection, find, links, outline | [§5](docs/plan.md#5-phases) |
| 3 | `load_fd` (previewer), packaging, install checks | [§5](docs/plan.md#5-phases) |
| 4 | Upstream MR into `papers/libdocument/backend/ofd/` | [§5](docs/plan.md#5-phases) |

## Upstreaming

The module uses public `PpsDocument` API only (the private-but-exported helper
symbols are avoided), so phase 4 is a file move plus a MIME-list addition in
Papers' `meson.build`. If the glue code has to change, an assumption in the plan's
interface contract was wrong and should be re-verified instead of patched around.

## License

GPL-2.0-or-later (see `COPYING`), matching Papers. Links rofd, which is
LGPL-2.1-or-later; the combination is compatible.
