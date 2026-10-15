# papers-ofd

OFD (GB/T 33190-2016) backend for [GNOME Papers](https://gitlab.gnome.org/GNOME/papers),
built out-of-tree on the [`rofd-ffi`](https://github.com/linuxdeepin/rofd) C ABI
(rofd's OFD parser and Cairo renderer).

Ships one shared module and four small data files into Papers' existing backend
directory. No Papers fork, no Papers rebuild, no patching of Papers sources.

## Status

Phases 0, 1, 2 and 3 are implemented and tested against **Papers 51.1** ([tag `51.1`](https://gitlab.gnome.org/GNOME/papers/-/tree/51.1);
the build needs only the installed `papers-document-4.0` headers) and **rofd ABI v1**:
the module loads through Papers' own backend discovery, renders pages correctly at
every rotation and zoom step, prints (vector text), reports metadata, serves text,
selection, search, page links and the outline, opens documents from a file
descriptor (the Nautilus previewer's path) and ships as an Arch package on the AUR.

- `meson test -C _build` runs the unit conversions, the factory round-trip with a
  deliberate discovery failure, the concurrency case (workers using a document
  while another thread reloads it), the rotation/zoom/origin render matrix
  (including the 4–16× budget fit and the oblique-rotation snap), the
  same-options equivalence against a direct FFI render, a real GTK print-to-file
  whose PDF text is verified with poppler, the metadata fields and mask bits, the
  text/layout/find/selection checks and the link/outline model — each compared
  against a direct rofd query of the same fixture — the load error paths, the fd
  path against the URI path (page geometry, text, pixels, info, temp-copy
  lifetime), and (with `-Drofd_src=<rofd checkout>`) the invoice oracle against
  rofd's reference render.
- Install: [`papers-ofd` on the AUR](https://aur.archlinux.org/packages/papers-ofd)
  (`51.1.1-1`), built from the `v51.1.1` tag archive by the AUR recipe, which
  is maintained in the AUR repository — this repository carries no PKGBUILD.
  Remaining: the upstream MR (phase 4).

Implementation plan: [`docs/plan.md`](docs/plan.md) — phases, interface contract,
acceptance criteria, verification playbook, risks ([§5](docs/plan.md#5-phases)).
Research notebook (raw evidence with `file:line`): `.planning/` in the working
copy.

## Capabilities

| Capability | Notes |
| --- | --- |
| Open `*.ofd` (double-click, `papers file.ofd`, `gio open`) | local files, by URI or by file descriptor |
| Scroll, zoom, rotate, fit-width/fit-page | full-page Cairo raster from `rofd-render` |
| Print and print-to-PDF | direct Cairo rendering into the print context, vector text |
| Thumbnails (Papers sidebar, Nautilus, `papers-thumbnailer`) | reuse of the page renderer |
| Metadata | title, author, subject, keywords, creator, dates, page count |
| Select/copy text, Ctrl+F, caret navigation | rofd's text layout + search; the I-beam cursor comes from the glyph region |
| In-page links and the outline sidebar | GOTO/URI links; the outline model feeds the links sidebar |
| Nautilus previewer (space bar) | Sushi renders documents through Papers' own libraries; `data/papers-ofd-sushi.js` makes it claim `application/ofd`, which Papers' own desktop file does not list out of tree |
| Loading from a file descriptor | `load_fd`: the descriptor is re-entered through `/proc/self/fd`, or copied into the user cache when `/proc` cannot be used |

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
a thin translation into rofd's C ABI (units, coordinates, options, errors). One
reader/writer lock guards the state the document owns — the rofd handle, the
renderer and the page-geometry cache — with `load`/`load_fd` as the writer and
every vfunc as a reader, the model of Papers' PDF backend
(`libview/pps-poppler.c`). Renders therefore run concurrently, as the ABI's
concurrent read-only contract allows; only a reload excludes them, which is what
keeps a handle from being freed under a call that is using it.

## Requirements

| Dependency | Notes |
| --- | --- |
| Papers ≥ 51, < 52 (`papers-document-4.0`) | system pkg-config; backends' ABI version is the `papers/6` directory component |
| rofd-ffi (ABI v1, ≥ 0.4.2) | `librofd_ffi.so.0` + `rofd.h`. The floor is the module's own: rofd ≤ 0.4.0 aborts a whole page render on a document object carrying a singular transform, and 0.4.2 turns that into a diagnostic ([A19](docs/plan.md#appendix-a--evidence-index)); the AUR package requires `≥ 0.4.3`, the fork release that also carries the pkg-config file, the page-size query and the font-resolved text geometry, and the build distinguishes a `rofd-ffi.pc` that is too old from a missing one. The pkg-config file and the carried changes both come from the fork's own release: [`YangtseSu/rofd`](https://github.com/YangtseSu/rofd) integrates them on its `dev` branch (one `feat/*` / `fix/*` branch and upstream PR per change) and tags releases there (`v0.4.3-fork.2` — upstream `0.4.3` plus the page-size query [#11](https://github.com/linuxdeepin/rofd/pull/11), the font-resolved text geometry [#13](https://github.com/linuxdeepin/rofd/pull/13), the image decode budget [#14](https://github.com/linuxdeepin/rofd/pull/14), the region-AA test tolerance [#15](https://github.com/linuxdeepin/rofd/pull/15) and `rofd-ffi.pc` [#12](https://github.com/linuxdeepin/rofd/pull/12); the fork's own rules are its [`FORK.md`](https://github.com/YangtseSu/rofd/blob/dev/FORK.md)), and the AUR `librofd-ffi` (`0.4.3.fork.2-1`) builds that tarball, so it installs `rofd-ffi.pc` and the fork ABI with no patch files ([`patches/README.md`](patches/README.md) is the inventory). Stock upstream rofd works too, falling back to the slow page-size path and to rofd's size-based text geometry. **Do not go below 0.4.2**: earlier rofd fails whole pages whose objects carry a singular transform ([A19](docs/plan.md#appendix-a--evidence-index)). Delivery policy: [plan D11](docs/plan.md#2-architecture-and-decisions) |
| Papers/rofd sources (optional) | **Not needed to build**: the module compiles against `papers-document-4.0` (headers + `libppsdocument-4.0`) and `rofd.h` + `librofd_ffi.so.0`. `$ROFD_SRC` is required to carry or re-verify the rofd changes we ship ([`patches/README.md`](patches/README.md)) and to run rofd's own test suite; a Papers checkout (`$PAPERS_SRC`) only speeds up grepping — its citations are written against the upstream repository at tag `51.1`. `scripts/dev-env.sh` reports what this machine has (environment → untracked `local.conf` → siblings `../papers`, `../rofd`) and flags a same-named directory that is not the expected repository |
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

# the previewer's invocation (Nautilus passes --fd plus the sniffed type)
papers-previewer --fd 3 3< "$ROFD_SRC/learning/test.ofd"
```

`scripts/dev-env.sh` prints the resolution table on its own
(`PAPERS_SRC`, `ROFD_SRC`, `ROFD_INCLUDEDIR`, `ROFD_LIBDIR`, the ABI tier in use)
and exits non-zero when neither the installed package nor a checkout provides the
ABI. Sourcing it (`. scripts/dev-env.sh`) exports those variables; bash and zsh are
supported and the script hands control back without ending the caller's shell.

Fixtures named in `docs/plan.md` §6 live under `$ROFD_SRC`.

`PPS_BACKENDS_DIR` replaces the whole backend directory: keep the system
`.papers-backend` files present (as above) or PDF/DjVu/TIFF stop working for that
session. The full validation matrix — fixtures, checks per phase, cross-repo ABI
tests — is in [`docs/plan.md` §6](docs/plan.md#6-verification-playbook).

## Install

Arch Linux: [`papers-ofd` on the AUR](https://aur.archlinux.org/packages/papers-ofd)
(`51.1.1-1`) — `yay -S papers-ofd`, `paru -S papers-ofd`, or

```bash
git clone https://aur.archlinux.org/papers-ofd.git && cd papers-ofd
makepkg -si
```

The recipe lives in that AUR repository (this repository carries no PKGBUILD, so
there is no copy to keep in sync); its source is the `v51.1.1` tag archive of this
repository, so `makepkg` verifies a checksum, and its
`check()` runs the test suite (13 tests on a host with Sushi, 12 without it — the
invoice oracle needs `-Drofd_src`, which a package build does not have;
`ofd-print-file` skips itself when GTK has no display). The fonts the suite
measures with are the recipe's `checkdepends` (`noto-fonts`, `noto-fonts-cjk`; the
change lands with the next package revision): the fixtures carry text but no font
file, so a host without them fails `ofd-text` — with the missing package named
instead of an opaque assertion (R4, plan §8). Dependencies are
`librofd-ffi>=0.4.3` (the ABI plus its `rofd-ffi.pc`), `papers>=51` and `papers<52`,
`cairo`, `gcc-libs`, `glib2`, `glibc`. `shared-mime-info` and `desktop-file-utils`
are not listed because their pacman hooks (which index the MIME xml and the desktop
file) arrive through `papers` already.

Versions track the Papers line rather than our own feature count (plan D13):
`51.1.1` is the Papers 51 line that the dependency bound asserts, verified against
Papers 51.1, our first release for that baseline; only the last component moves with
our releases, and packaging-only changes stay in the AUR `pkgrel`.

The package also installs the Sushi plugin below (into Sushi's system plugin
directory; `-Dsushi_plugin_dir` moves or skips it) and lists `sushi` as an optional
dependency — without Sushi the file is inert and only the space-bar preview is
missing.

Uninstalling removes only the files below — none is owned by another package, so
Papers and its other backends are untouched.

## Installed files

```
<backenddir>/libofddocument.so                            the module
<backenddir>/org.gnome.Papers.OfdDocument.papers-backend  discovery descriptor
/usr/share/mime/packages/papers-ofd.xml                   application/ofd (glob + zip magic)
/usr/share/thumbnailers/papers-ofd.thumbnailer            Nautilus thumbnails
/usr/share/applications/org.gnome.Papers.Ofd.desktop      association entry (NoDisplay)
/usr/lib/sushi/plugins-1/papers-ofd-sushi.js              space-bar previews (see Limitations)
```

Papers builds its own desktop and thumbnailer MIME lists from in-tree backends
only, hence the separate association and thumbnailer entries. The Sushi plugin
exists for the same reason: Sushi's document renderer claims exactly the MIME
types of Papers' own desktop file, so `application/ofd` needs a plugin until
Papers carries it itself ([§7](docs/plan.md#7-packaging-and-desktop-integration),
A26 in [Appendix A](docs/plan.md#appendix-a--evidence-index)). The MIME magic is a
nested rule at priority 70: below `application/zip`'s priority it would lose every
content-only guess, which is the only detection the previewer has. Details and the
patch-free vs patch-the-desktop tradeoff:
[`docs/plan.md` §7](docs/plan.md#7-packaging-and-desktop-integration).

## Limitations

- **Open latency** on a 300-page generated fixture (content on every page,
  5 runs, best of): load ~26 ms — the renderer and rofd's font stack are created
  when the document loads, which is also why the first render is ~20 ms rather
  than ~48 ms — then ~66 ms for the page walk (`setup_cache`, which fetches every
  page object) and ~20 ms for the first rendered page: ~130 ms from
  `pps_document_load` to the first page being visible. The
  metadata-only page-size query (`rofd_document_get_page_size_mm`, shipped in
  the fork's release) makes the size walk itself ~18 ms instead of ~70 ms, but the
  end-to-end time is
  about the same in the shell's flow because `setup_cache` fetches the page objects
  either way; the fast path pays off for callers that only need sizes
  (`tests/ofd-latency.c`, both builds).
- **Render budget.** rofd charges the worst case of a page raster
  (≈9 B/px: the finished ARGB32 target, its A8 clip mask and a clipped
  temporary) against `max_raster_bytes`, where Papers' own allowance is 4 B/px —
  the `page-cache-size` setting, 200 MiB by default, which is what clamps the
  maximum zoom (≈10.2× on A4). The module therefore derives the budget from each
  request and caps it at 450 MiB (that allowance converted to rofd's rate), so
  every zoom the view admits renders; a request beyond the cap — only reachable
  from a caller that sets a target size itself, or a larger `page-cache-size` —
  renders at the largest scale that fits instead of failing. The page texture and
  the selection overlay share the fitted scale, so they cannot disagree. One shape
  the fit cannot save is a canvas dimension above cairo's 32767-pixel limit (a page
  far wider than it is tall, at a high zoom): it has no surface to draw into, and
  that refusal is reported at `g_warning` rather than silently. Rotation
  is snapped to the nearest quarter turn: rofd's renderer accepts nothing else,
  while Papers' model allows any 0–360.
- Selection highlights are drawn as filled rectangles; the glyphs underneath are
  not recoloured, because rofd's ABI exposes no text mask. Invisible on systems
  whose poppler has transparent-selection support (the common case).
- A wrapped multi-line search match is reported as a single result rectangle.
- **Text geometry follows the ABI.** For a text run that declares no explicit
  `DeltaX`, rofd takes each scalar's advance from the resolved font and sizes its box
  by that face's typographic ascent and descent — and grows a selection region by an
  eighth of its height on both sides — only in the fork's release
  ([#13](https://github.com/linuxdeepin/rofd/pull/13), shipped in `v0.4.3-fork.2`
  and built by the AUR `librofd-ffi` package). That is the model of a PDF reader's highlight: all glyphs
  of a line share one band that clears
  the text above and below. With a stock `librofd-ffi`, rofd's semantic layer uses the
  font size instead, so search highlights, caret positions and selection regions on
  such text can sit right of the glyphs, and the box stops at the baseline, leaving the
  bottom of the text (CJK overshoot, Latin descenders) unhighlighted. Everything else is
  unaffected, and the test reports which model the linked library uses: the line
  `first line: ABI layout … mm, ink … mm -> font-resolved|font-size footprint
  geometry` is printed to stdout by `ofd-text`, so it lands in
  `_build/meson-logs/testlog.txt` (or on screen with
  `meson test -C _build --print-errorlogs`).
- The space-bar preview needs the shipped Sushi plugin, which depends on Sushi
  internals (its plugin directory, the document renderer's content-type list).
  It is removed together with the plugin once Papers lists `application/ofd` in
  its own desktop file, i.e. after the phase-4 move in-tree; if Sushi changes, the
  plugin loads with an error and previews of other types are unaffected.
- "Open With" in Nautilus lists every installed application, so both desktop
  entries that run `papers` (Papers' own and this package's `NoDisplay`
  association entry) appear as rows there. `application/ofd` itself is registered
  on one entry only; the duplicate disappears with the phase-4 move, which also
  retires the association entry.
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
| 3 | `load_fd` (previewer), packaging, install checks, Sushi plugin | done |
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
