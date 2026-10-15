<!-- SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com> -->
<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# The rofd dependency: fork release inventory

The rofd ABI Papers-ofd depends on is **built from the fork's own releases**,
not from an upstream archive with patches applied. `YangtseSu/rofd` publishes a
release per ABI we ship, and the AUR [`librofd-ffi`](https://aur.archlinux.org/packages/librofd-ffi)
package builds that release's tagged tarball directly — the AUR carries no patch
files.

This file is the inventory: which release, what it contains on top of upstream,
when it is replaced. `docs/plan.md` D11 is the policy; §9 documents each change
with its measurements and evidence ids.

## The release model

- **Development branches** `feat/*`, `fix/*` on the fork propose each change
  upstream as a PR against [`linuxdeepin/rofd`](https://github.com/linuxdeepin/rofd)
  (their only exit path remains an upstream merge).
- **`release/*` branches** on the fork are the delivery vehicles: upstream's
  latest release plus the carried changes, versioned `<upstream>-fork.<n>`
  (cargo `0.4.3-fork.1`, tag `v0.4.3-fork.1`), given a real tag and a source
  tarball on GitHub Releases.
- The AUR `librofd-ffi` package builds that tarball — `source=` points at the
  release tag, `sha256sums` pins it, and there is nothing to replay. The
  pkg-config template ships in the fork source tree (`debian/rofd-ffi.pc.in`).
- **CI** `.github/workflows/fork-checks.yml` lives on `release/*` branches only
  (a feature branch keeps its diff against upstream to the change alone).
  Pushing a release branch runs it strictly — nothing skipped, nothing allowed
  to fail. `feat/*` / `fix/*` branches are checked on demand through
  `workflow_dispatch`, in the loose mode that keeps the two upstream-red tests
  visible but non-gating.
- The fork's `main` keeps mirroring upstream `linuxdeepin/rofd`; nothing in
  `main` is ours.

## Current release

| | |
| --- | --- |
| Tag / version | `v0.4.3-fork.1` (cargo `0.4.3-fork.1`), branch `release/0.4.3-fork.1` |
| Upstream base | `linuxdeepin/rofd` 0.4.3 at `140b936` (upstream published no 0.4.3 tag) |
| Carried changes | see the table below |
| AUR | `librofd-ffi 0.4.3.fork.1-1`, tarball sha256 `b19a464e…9f05681dd` |
| Retirement | when an upstream `linuxdeepin/rofd` release contains **every** carried change, the AUR package switches its `source=` to an upstream-built one (or to the distribution package if one exists) and this table records the switch |

## Carried changes in the release

| Change | Fork branch & upstream PR | Retired from the release when |
| --- | --- | --- |
| `rofd_document_get_page_size_mm` — metadata-only page sizes, so `setup_cache` stops parsing every page | `feat/ffi-page-size-query`, [linuxdeepin/rofd#11](https://github.com/linuxdeepin/rofd/pull/11) | an upstream release exports the function — then rebase `release/*` onto it, drop the cherry-pick, and the `meson.build` probe keeps the fast path |
| Font-resolved text geometry: a run without an explicit `DeltaX` advances by the resolved font, its character box spans that face's typographic ascent and descent, and a selection region grows by an eighth of its height on both sides | `feat/font-text-advances`, [linuxdeepin/rofd#13](https://github.com/linuxdeepin/rofd/pull/13) | an upstream release resolves advances through the injected provider |
| Image decode allocation budget: the decoder's intermediate buffers are budgeted on top of the finished RGBA8 raster, so seals inside the document limits decode and out-of-limit ones report `RasterBudgetExceeded` | `fix/decoded-image-alloc-budget`, [linuxdeepin/rofd#14](https://github.com/linuxdeepin/rofd/pull/14) | an upstream release budgets the decoder with headroom over the raster |
| `pkg-config` metadata `rofd-ffi.pc` | `feat/pkgconfig`, [linuxdeepin/rofd#12](https://github.com/linuxdeepin/rofd/pull/12) | an upstream release installs (or at least ships) the template |
| Region-AA test tolerance in `crates/rofd-ffi/tests/region.rs` — bounds the pixel diff by its share and per-channel delta instead of absolutes that only hold on Arch's font stack | `fix/region-aa-tolerance`, [linuxdeepin/rofd#15](https://github.com/linuxdeepin/rofd/pull/15) | the PR merges; the tolerance then comes from upstream |

All are additive over ABI v1: the releases keep `librofd_ffi.so.0` and the
78-symbol `expected-symbols.txt` surface, so every released ABI is a strict
supersede of the previous one.

## Release and update routine

One branch per change, one branch per release:

- a change goes to a `feat/*` / `fix/*` branch based on `main` (which mirrors
  upstream) and is proposed upstream as a PR — its diff is the change alone;
- a release is a `release/<next-version>` branch that starts from the previous
  release branch and cherry-picks the new changes, so each release inherits
  everything already shipped and every carried row below stays traceable to its
  own branch.

1. On the fork, fast-forward nothing: `main` follows upstream. Branch
   `release/<next-version>` from the previous release branch (or from the newest
   upstream release tag for the first release of a new upstream version),
   cherry-pick the new `feat/*` / `fix/*` commits, drop the cherry-picks whose
   PR has merged upstream, and bump the cargo version to `<upstream>-fork.<n>`
   (`Cargo.toml`, the three crates, and the assertion in
   `crates/rofd-core/tests/public_api.rs`).
2. Run the checks locally: `cargo fmt --all -- --check`,
   `cargo clippy --offline -p rofd-core -p rofd-render -p rofd-ffi --all-targets
   -- -D warnings`, `cargo test --offline -p rofd-core -p rofd-render -p
   rofd-ffi`, and `sh crates/rofd-ffi/tests/run_c_tests.sh`. Pushing the
   `release/*` branch runs the same set in CI, strictly: no test is skipped and
   no step may fail, because the release carries the fixes that make the two
   upstream-red tests green. A `feat/*` / `fix/*` branch can be checked on
   demand with `gh workflow run fork-checks.yml --ref <release-branch> -f
   refs=refs/heads/<branch>`; the workflow then runs it loose (the two
   upstream-red tests visible but non-gating).
3. Push the `release/*` branch, tag `v<version>` on the release commit and
   **never move the tag again** (the AUR `source=` URL and its `sha256sums` pin
   it), create the GitHub release with the `git archive` tarball (it includes
   the vendored crates, so the AUR `--frozen` build is offline) and record its
   sha256 here and in the AUR.
4. Bump the AUR deliberately: new `pkgver` (dots for the cargo version's
   hyphens), `pkgrel=1`, new tarball URL and sha256, `makepkg -C -f`, verify
   the exported symbols against `crates/rofd-ffi/tests/c/expected-symbols.txt`,
   commit and push the AUR repository.
5. Refresh this inventory: current-release table and any carried-change rows
   that were retired or added.

Two tests are red on upstream `main` and green in the fork releases (fixed by
PRs #14 and #15). A release branch runs them as hard gates; only the loose mode
(step 2, for branches based on upstream) keeps them visible and non-gating. Drop
the loose half — the two `--skip` flags and the two `continue-on-error` steps —
once the PRs merge and upstream `main` carries the fixes.

## Working against a release checkout

```bash
. scripts/dev-env.sh                    # resolves and exports $ROFD_SRC
git -C "$ROFD_SRC" fetch fork
git -C "$ROFD_SRC" switch release/0.4.3-fork.1
cargo build --release --offline -p rofd-ffi   # $ROFD_LIBDIR then points here
```

A stock upstream checkout still works (slow page-size walk, rofd's size-based
text geometry): the configure-time `cc.has_function` probe and the tests'
classification of the linked library report which side is missing.
