# Carried rofd patch: metadata-only page size

`rofd-ffi-page-size.patch` adds one additive entry point to the rofd C ABI:

```c
rofd_status_t rofd_document_get_page_size_mm(const rofd_document_t *document,
                                             size_t page_index,
                                             rofd_rect_t *page_size_mm,
                                             rofd_error_t **error);
```

## Why

Papers asks a document backend for the size of **every** page right after load
(`pps_document_setup_cache`, `libdocument/pps-document.c`). The released ABI only
offers `rofd_page_get_size_mm`, which needs an opened page, and opening a page in
rofd parses and caches its whole content — so an unpatched rofd turns "open a
document" into "parse the entire document".

`rofd-core` already implements the metadata-only query (`Document::page_size`,
documented as "Reads the physical page box without loading or caching graphical
objects"); the patch only exposes it over the FFI, with tests, the pinned export
list and the design doc updated.

## How the module uses it

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

## How to apply

Against a clean `linuxdeepin/rofd` checkout:

```bash
git -C ../rofd checkout main
git -C ../rofd apply /absolute/path/to/papers-ofd/patches/rofd-ffi-page-size.patch
# or keep history: git -C ../rofd am patches/rofd-ffi-page-size.patch
```

The patch is generation-compatible with `git format-patch` and applies cleanly to
`rofd` `main` as of the commit it was generated from (verified with
`git apply --check` in a detached worktree).

## Upstream status

Submitted as <https://github.com/linuxdeepin/rofd/pull/11> on 2026-10-14, from the
`YangtseSu/rofd` fork, branch `feat/ffi-page-size-query`. Generated from `rofd`
`main` at `140b936` (`chore: Update version to 0.4.3`) and verified to apply to that
revision with `git apply --check`.

## Removal criterion

Delete this patch (and let the `cc.has_function` probe keep doing its job) once
released rofd ships `rofd_document_get_page_size_mm`. Re-verify after every rofd
version bump; if the function's contract changes upstream, update
`docs/plan.md` §9 instead of carrying a divergent local variant.
