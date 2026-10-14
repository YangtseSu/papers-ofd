<!-- SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com> -->
<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# Carried rofd patches and the fork

`patches/` mirrors every change we carry against
[`linuxdeepin/rofd`](https://github.com/linuxdeepin/rofd): the fork branch, the
patch file that reproduces the change on a stock checkout, and the criterion that
retires it. Delivery is the AUR package `librofd-ffi` (ours), which applies these
patch files to the upstream release tarball — see `docs/plan.md` D11. Upstream PRs
stay open as the exit path; they are not on the critical path.

## The fork

- Fork: [`YangtseSu/rofd`](https://github.com/YangtseSu/rofd). Its `main` mirrors
  upstream (`git remote add upstream https://github.com/linuxdeepin/rofd &&
  git fetch upstream`), and every carried change lives on its own `feat/…` branch
  with one upstream PR per branch — `feat/ffi-page-size-query` at `7f6d24d` today.
  The clone used by this project is `$ROFD_SRC` (`~/projects/rofd`); it carries
  both remotes.
- Upstream releases are not always tagged: `0.4.3` exists only as commit
  `140b936` on `main`. The AUR PKGBUILD therefore pins that commit in its source
  URL (with its sha256) instead of a `v0.4.3` tag, and `pkgver` is set by hand.
  Bump the pin deliberately: read the diff, re-run `git apply --check` for every
  patch here, rebuild, and re-check the exported symbol list.
- No long-lived integration branch. The AUR PKGBUILD applies `patches/*.patch` in
  order to the pinned upstream tree, which keeps the shipped ABI identical to a
  stock checkout plus these patches; `git apply --check` stays the only
  integration test needed.
- Sync routine when upstream moves: `git -C "$ROFD_SRC" switch main &&
  git -C "$ROFD_SRC" pull --ff-only upstream main`, rebase each `feat/…` branch,
  re-run `cargo fmt --all -- --check`,
  `cargo clippy -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings`,
  `cargo test -p rofd-ffi`, `crates/rofd-ffi/tests/run_c_tests.sh`, refresh the
  patch file with `git format-patch`, bump the AUR `pkgrel`, rebuild, and re-check
  the exported symbol list (`nm -D --defined-only`).
- The pkg-config metadata is **not** a patch here: it is packaging-side and lives
  in the AUR package as `rofd-ffi.pc.in` (installed to
  `/usr/lib/pkgconfig/rofd-ffi.pc`). The day a non-Arch package is needed, the same
  file goes into the fork's `debian/` as `rofd-ffi-pkgconfig.patch`.

## `rofd-ffi-page-size.patch` — metadata-only page size

Adds one additive entry point to the rofd C ABI:

```c
rofd_status_t rofd_document_get_page_size_mm(const rofd_document_t *document,
                                             size_t page_index,
                                             rofd_rect_t *page_size_mm,
                                             rofd_error_t **error);
```

### Why

Papers asks a document backend for the size of **every** page right after load
(`pps_document_setup_cache`, `libdocument/pps-document.c` in tag `51.1`). The
released ABI only offers `rofd_page_get_size_mm`, which needs an opened page, and
opening a page in rofd parses and caches its whole content — so an unpatched rofd
turns "open a document" into "parse the entire document".

`rofd-core` already implements the metadata-only query (`Document::page_size`,
documented as "Reads the physical page box without loading or caching graphical
objects"); the patch only exposes it over the FFI, with tests, the pinned export
list and the design doc updated.

### How the module uses it

`meson.build` probes the linked library:

```meson
have_page_size = cc.has_function('rofd_document_get_page_size_mm',
                                 prefix: '#include <rofd.h>',
                                 dependencies: rofd_ffi_dep)
```

- present → `HAVE_ROFD_PAGE_SIZE` is defined and `get_page_size` uses the
  metadata-only call;
- absent → the same vfunc falls back to `rofd_document_get_page` +
  `rofd_page_get_size_mm`, so **stock rofd keeps working** (slower on large
  documents; the difference is measured and published per `docs/plan.md` §5).

### How to apply

Resolve the checkout first (`. scripts/dev-env.sh`), then either apply to a stock
checkout:

```bash
git -C "$ROFD_SRC" checkout main
git -C "$ROFD_SRC" apply "$PWD/patches/rofd-ffi-page-size.patch"
# or keep history: git -C "$ROFD_SRC" am "$PWD/patches/rofd-ffi-page-size.patch"
```

or take the shipped ABI — the AUR `librofd-ffi` package already carries it
(`pkgrel` 2 onwards), which is what `scripts/dev-env.sh` finds on a machine without
a checkout.

The patch is generation-compatible with `git format-patch` and applies cleanly to
`rofd` `main` as of the commit it was generated from (verified with
`git apply --check` in a detached worktree).

### Upstream status

Requested as <https://github.com/linuxdeepin/rofd/pull/11> on 2026-10-14 from the
`feat/ffi-page-size-query` branch of the fork (`7f6d24d`, on top of `140b936`).
Generated from `rofd` `main` at `140b936` (`chore: Update version to 0.4.3`) and
verified to apply to that revision with `git apply --check` — re-verified on this
machine against `$ROFD_SRC` (7 files, clean). The AUR package ships it on top of
the same commit as `librofd-ffi 0.4.3-1`. PR state is tracked in
`.planning/2026-10-14-papers-ofd-backend-plan-and-readme/progress.md`.

### Removal criterion

Delete this patch (and let the `cc.has_function` probe keep doing its job) once a
rofd release ships `rofd_document_get_page_size_mm`; then drop the corresponding
AUR `source`/`sha256sums` entries, bump the package to that release, and switch the
fork to tracking upstream. Re-verify after every rofd version bump; if the
function's contract changes upstream, update `docs/plan.md` §9 instead of carrying a
divergent local variant.
