<!-- SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com> -->
<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# Carried rofd changes: the inventory

The rofd changes this backend relies on are **not** stored in this repository.
Each one lives in exactly two places, both derived from the fork branch that
implements it:

- [`YangtseSu/rofd`](https://github.com/YangtseSu/rofd) — the `feat/…` branch
  carrying the change on top of upstream `main`, proposed upstream as a PR. It is
  the source of truth.
- the AUR [`librofd-ffi`](https://aur.archlinux.org/packages/librofd-ffi)
  repository — the patch file (`git format-patch` output) that the package's
  `prepare()` applies to its pinned upstream archive. That file is what ships.

This file is the inventory: which change, where it comes from, when it goes away.
`docs/plan.md` D11 is the delivery policy; §9 documents each change with its
measurements and evidence ids.

## Carried changes

| Change | Fork branch (`feat/…`) and upstream PR | AUR patch file in `librofd-ffi` | Retired when |
| --- | --- | --- | --- |
| `rofd_document_get_page_size_mm` — metadata-only page sizes, so `setup_cache` stops parsing every page | `feat/ffi-page-size-query` at `7f6d24d`, [linuxdeepin/rofd#11](https://github.com/linuxdeepin/rofd/pull/11) | `rofd-ffi-page-size.patch` | a rofd release exports the function — then `meson.build`'s `cc.has_function` probe keeps the fast path and the patch disappears |
| Font-resolved text geometry: a run without an explicit `DeltaX` advances by the resolved font, its character box spans that face's typographic ascent and descent, and a selection region grows by an eighth of its height on both sides, so page geometry, search rectangles and selection highlights match a PDF reader's | `feat/font-text-advances` at `a364014`, [linuxdeepin/rofd#13](https://github.com/linuxdeepin/rofd/pull/13) | `rofd-ffi-font-advances.patch` | a rofd release loads documents with a font-resolved metrics provider — then the ink-alignment test asserts on every library |
| Image decode allocation budget: the image crate's `Limits::max_alloc` covers the decoder's own buffers on top of the finished RGBA8 raster, so budgeting it with the raster size alone dropped **seals (stamp annotations)** that are inside the document limits and reported a decode error where `RasterBudgetExceeded` is designed | `fix/decoded-image-alloc-budget` at `2983cf6`, [linuxdeepin/rofd#14](https://github.com/linuxdeepin/rofd/pull/14) | `rofd-ffi-image-decode-budget.patch` | a rofd release budgets the decoder with headroom over the raster — then the boundary test and the seal test assert on every library |

All three are additive to ABI v1 and all are optional: a stock `librofd-ffi` still
builds and runs this backend (slow page-size walk; rofd's size-based text
geometry), which the configure-time probe and the tests' classification of the
linked library report. The measurements live in `docs/plan.md` §9 items 1 and 6
(evidence A18, A24, A27).

The `pkg-config` metadata is **not** a carried patch: it ships in the AUR package
as `rofd-ffi.pc.in` and upstream as
[linuxdeepin/rofd#12](https://github.com/linuxdeepin/rofd/pull/12)
(`feat/pkgconfig`). The day a non-Arch consumer needs it, it goes into the fork's
`debian/` as `rofd-ffi-pkgconfig.patch` — still not this directory.

## Using the branches instead of the package

```bash
. scripts/dev-env.sh                    # resolves and exports $ROFD_SRC
git -C "$ROFD_SRC" fetch fork
git -C "$ROFD_SRC" switch feat/font-text-advances    # or feat/ffi-page-size-query
cargo build --release -p rofd-ffi       # $ROFD_INCLUDEDIR/$ROFD_LIBDIR then point here
```

`feat/aur-carried` on the fork is a **build convenience**, not a development
branch: upstream `main` with the AUR patch files replayed by `git am`, so a single
checkout reproduces the ABI the package ships. Regenerate it whenever the upstream
pin or a patch moves (`$AUR_LIBROFD` = the AUR `librofd-ffi` checkout):

```bash
git -C "$ROFD_SRC" switch -C feat/aur-carried main
git -C "$ROFD_SRC" am "$AUR_LIBROFD"/rofd-ffi-page-size.patch \
                     "$AUR_LIBROFD"/rofd-ffi-font-advances.patch
```

and verify it against an actual package build tree with
`diff -r --exclude=.git --exclude=target "$ROFD_SRC" "$srcdir/rofd-$_commit"`.
Never merge it upstream: the per-change branches and their PRs are the upstream
path, and the fork's `main` keeps mirroring upstream.

To reproduce the shipped ABI on a stock checkout outside Arch:

```bash
git -C "$ROFD_SRC" switch main
git -C "$ROFD_SRC" apply "$AUR_LIBROFD/rofd-ffi-page-size.patch"
# or keep history: git -C "$ROFD_SRC" am "$AUR_LIBROFD"/rofd-ffi-page-size.patch
```

## Sync routine when upstream moves

1. `git -C "$ROFD_SRC" fetch --all`, then fast-forward `main` to upstream
   (this checkout has `origin` = `linuxdeepin/rofd`, `fork` = `YangtseSu/rofd`),
   and rebase every `feat/…` branch onto it.
2. Re-run the checks in the fork: `cargo fmt --all -- --check`,
   `cargo clippy -p rofd-core -p rofd-render -p rofd-ffi --all-targets -- -D warnings`,
   `cargo test -p rofd-ffi`, `crates/rofd-ffi/tests/run_c_tests.sh`.
3. Regenerate each patch file (`git format-patch`), copy it into `$AUR_LIBROFD`,
   re-run `git apply --check` for every patch against the new upstream revision.
4. Bump the AUR pin deliberately: read the diff, rebuild (`makepkg -C -f`),
   re-check the exported symbol list (`nm -D --defined-only` on the packaged
   library, against `crates/rofd-ffi/tests/c/expected-symbols.txt`), commit and
   push the AUR repository.
5. Rebase `feat/aur-carried` as above and refresh this inventory if a change was
   added, retired or renamed.

## The fork's own CI (`ci/fork-checks`)

Upstream's `Distribution Builder` gates every job behind `if: false` and its
CLA/BUILD gates need a linuxdeepin member, so no check ever runs against a PR. The
fork therefore carries `.github/workflows/fork-checks.yml` on its own branch
`ci/fork-checks`: on every push to that branch (or a manual dispatch with an
explicit ref list) it collects the fork's `feat/*` and `fix/*` branches and runs
`cargo fmt --check`, clippy with `-D warnings`, the crate tests, the exact build
command `debian/rules` uses (`… -p rofd --features rofd/qt-reader`) and
`crates/rofd-ffi/tests/run_c_tests.sh` per branch. Actions are enabled on the fork
and the ten inherited workflows are disabled — in a fork they would fire on push
and fail on secrets that forks never inherit. The dependency step installs the
`debian/control` build deps plus Qt and the two font families the crate tests need:
`rofd-render --test real_fixture` lowers the repository invoice fixture (its text
names SimSun/KaiTi/Courier New) through a resolver whose fallbacks are
`Noto Sans CJK SC`/`Noto Sans Mono`, so a runner without `fonts-noto-cjk` and
`fonts-noto-core` fails it with `MissingGlyph` diagnostics.

Two tests are red on upstream `main` today; they run as their own visible,
non-gating steps (with the fixing PR inline) so that a branch's own regressions
still fail the run: the decoded-image limit boundary ([#14](#upstream-proposals-and-their-delivery-status))
and the region pixel diff ([#15](#upstream-proposals-and-their-delivery-status)).
Remove both splits once those PRs are merged and the branches are rebased.

## Upstream proposals and their delivery status

[linuxdeepin/rofd#14](https://github.com/linuxdeepin/rofd/pull/14)
(`fix/decoded-image-alloc-budget`) is **carried** by the AUR package
(`rofd-ffi-image-decode-budget.patch`, `pkgrel` 7 onwards) because it repairs rendered
output, not just a test: `crates/rofd-render/tests/images.rs` was red on `main` and
`cairo_renderer.rs::decode_seal_raster` silently dropped seals whose decoded RGBA8 raster
sits between roughly half and all of the document byte limit. The patch raises both
sites' decoder budget to three times the raster; the raster checks stay the authority, so
no limit is relaxed. Retirement: a rofd release that budgets the decoder with headroom
over the raster — then drop the patch and the `sha256sums` entry. The AUR patch carries
the two source hunks only (the package builds the library, not rofd-render's tests), so
the regression test and its 430 KiB fixture are not part of it; they live on the fork
branch and in the PR, regenerated from the same commit with the test files excluded.

[linuxdeepin/rofd#15](https://github.com/linuxdeepin/rofd/pull/15)
(`fix/region-aa-tolerance`) makes `crates/rofd-ffi/tests/region.rs` bound the
anti-aliasing difference between a full-page and a region rendering by its share of the
tile's channels and the per-channel delta, instead of the absolute numbers that only hold
on Arch's font stack (plan §9 item 8). **Not carried**: test-only, no shipped behaviour —
but it is the reason the fork CI can run that test gated again once merged.
