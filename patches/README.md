<!-- SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com> -->
<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# The rofd dependency: fork release inventory

The rofd ABI Papers-ofd depends on is **built from the fork's own releases**,
not from an upstream archive with patches applied. `YangtseSu/rofd` integrates
our changes on its `dev` branch and tags a release per ABI we ship; the AUR
[`librofd-ffi`](https://aur.archlinux.org/packages/librofd-ffi) package builds
that tag's tarball directly — the AUR carries no patch files.

This file is the inventory: which release, what it contains on top of upstream,
when it is replaced. `docs/plan.md` D11 is the policy; §9 documents each change
with its measurements and evidence ids.

## The release model

The fork's own rules are in its [`FORK.md`](https://github.com/YangtseSu/rofd/blob/dev/FORK.md);
this section is what a consumer of the ABI needs to know.

- **`main`** mirrors [`linuxdeepin/rofd`](https://github.com/linuxdeepin/rofd);
  nothing of ours is committed there.
- **`feat/*` / `fix/*`** carry one change each, based on `main`, proposed
  upstream as a PR — the only exit path is an upstream merge.
- **`dev`** is the integration branch: each change arrives as a
  `git merge --no-ff` of its branch, so the carried inventory is derivable
  rather than narrated:

  ```bash
  git -C "$ROFD_SRC" log --oneline --merges origin/main..dev
  git -C "$ROFD_SRC" diff --stat origin/main..dev
  ```

- A **release** is a tag on `dev`, versioned `<upstream>-fork.<n>` (cargo
  `0.4.3-fork.2`, tag `v0.4.3-fork.2`), published with a `git archive` tarball
  on GitHub Releases.
- The AUR `librofd-ffi` package builds that tarball — `source=` points at the
  tag, `sha256sums` pins it, nothing is replayed. The pkg-config template ships
  in the fork source tree (`debian/rofd-ffi.pc.in`).
- **CI** `.github/workflows/fork-checks.yml` lives on `dev` (a feature branch
  keeps its diff against upstream to the change alone). A push to `dev` runs it
  strictly — nothing skipped, nothing tolerated. `feat/*` / `fix/*` branches are
  checked on demand through `workflow_dispatch`, in the loose mode that keeps
  the two upstream-red tests visible but non-gating.

## Current release

| | |
| --- | --- |
| Tag / version | `v0.4.3-fork.2` (cargo `0.4.3-fork.2`), tag on `dev` |
| Upstream base | `linuxdeepin/rofd` 0.4.3 at `140b936` (upstream published no 0.4.3 tag) |
| Carried changes | see the table below |
| AUR | `librofd-ffi 0.4.3.fork.2-1`, tarball sha256 `6b6c462a…73c5ba6c0` |
| Supersedes | `v0.4.3-fork.1` — withdrawn, its tag and the `release/0.4.3-fork.1` branch are deleted |
| Retirement | when an upstream `linuxdeepin/rofd` release contains **every** carried change, the AUR package switches its `source=` to an upstream-built one (or to the distribution package if one exists) and this table records the switch |

## Carried changes in the release

| Change | Fork branch & upstream PR | Retired from the release when |
| --- | --- | --- |
| `rofd_document_get_page_size_mm` — metadata-only page sizes, so `setup_cache` stops parsing every page | `feat/ffi-page-size-query`, [linuxdeepin/rofd#11](https://github.com/linuxdeepin/rofd/pull/11) | an upstream release exports the function — then rebase `dev` onto it, drop the merge, and the `meson.build` probe keeps the fast path |
| Font-resolved text geometry: a run without an explicit `DeltaX` advances by the resolved font, its character box spans that face's typographic ascent and descent, and a selection region grows by an eighth of its height on both sides | `feat/font-text-advances`, [linuxdeepin/rofd#13](https://github.com/linuxdeepin/rofd/pull/13) | an upstream release resolves advances through the injected provider |
| Image decode allocation budget: the decoder's intermediate buffers are budgeted on top of the finished RGBA8 raster, so seals inside the document limits decode and out-of-limit ones report `RasterBudgetExceeded` | `fix/decoded-image-alloc-budget`, [linuxdeepin/rofd#14](https://github.com/linuxdeepin/rofd/pull/14) | an upstream release budgets the decoder with headroom over the raster |
| `pkg-config` metadata `rofd-ffi.pc` | `feat/pkgconfig`, [linuxdeepin/rofd#12](https://github.com/linuxdeepin/rofd/pull/12) | an upstream release installs (or at least ships) the template |
| Region-AA test tolerance in `crates/rofd-ffi/tests/region.rs` — bounds the pixel diff by its share and per-channel delta instead of absolutes that only hold on Arch's font stack | `fix/region-aa-tolerance`, [linuxdeepin/rofd#15](https://github.com/linuxdeepin/rofd/pull/15) | the PR merges; the tolerance then comes from upstream |

All are additive over ABI v1: the releases keep `librofd_ffi.so.0` and the
78-symbol `expected-symbols.txt` surface, so every released ABI is a strict
supersede of the previous one.

## Release and update routine

The fork-side steps (branching, merging, tagging) are in its
[`FORK.md`](https://github.com/YangtseSu/rofd/blob/dev/FORK.md); this is the
consumer half, which is what this repository owns.

1. Add the change on the fork: a `feat/*` / `fix/*` branch off `main` (diff =
   the change), an upstream PR, then `git merge --no-ff` into `dev`.
2. When a release is due, bump the fork version to `<upstream>-fork.<n>`, push
   `dev`, let its CI pass, tag `v<version>` and publish the `git archive`
   tarball (it includes the vendored crates, so the AUR `--frozen` build is
   offline). **Never move a published tag** — the AUR `source=` and its
   `sha256sums` pin it, and so does every tarball already downloaded.
3. Repackage the AUR deliberately: new `pkgver` (dots for the cargo version's
   hyphens), `pkgrel=1`, the new `_tag`, tarball URL and sha256, regenerate
   `.SRCINFO` (the AUR hook rejects a push that still lists old sources),
   `makepkg -C -f`, and verify the exported symbols against
   `crates/rofd-ffi/tests/c/expected-symbols.txt`. Then `papers-ofd`'s own meson
   suite is the end-to-end check against the packaged library.
4. Refresh this inventory: the current-release table, and any carried-change row
   that was retired or added.

Two tests are red on upstream `main` and green on `dev` (fixed by PRs #14 and
#15), so they gate there; only the loose mode used for `feat/*` / `fix/*`
branches keeps them visible but non-gating. Drop the loose half — the two
`--skip` flags and the two `continue-on-error` steps — once the PRs merge and
upstream `main` carries the fixes.

## Working against a release checkout

```bash
. scripts/dev-env.sh                    # resolves and exports $ROFD_SRC
git -C "$ROFD_SRC" fetch fork
git -C "$ROFD_SRC" switch dev            # tag v0.4.3-fork.2 when pinning a release
cargo build --release --offline -p rofd-ffi   # $ROFD_LIBDIR then points here
```

A stock upstream checkout still works (slow page-size walk, rofd's size-based
text geometry): the configure-time `cc.has_function` probe and the tests'
classification of the linked library report which side is missing.
