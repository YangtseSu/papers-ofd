# Repository Guidelines

`papers-ofd` is an out-of-tree GNOME Papers backend for OFD documents, written in C
against the `papers-document-4.0` API and the `rofd-ffi` C ABI. `docs/plan.md` is
the authoritative plan; read it before changing code.

## Layout

| Path | Purpose |
| --- | --- |
| `docs/plan.md` | Interface contract, decisions D1–D12, phases 0–4 with acceptance criteria, evidence index (every claim cites `file:line`) |
| `src/`, `data/`, `tests/` | Module sources, installed data files, tests (to be created by phase 0) |
| `scripts/dev-env.sh` | Resolves the machine-local dependency checkouts and the rofd ABI pair (`PAPERS_SRC`, `ROFD_SRC`, `ROFD_INCLUDEDIR`, `ROFD_LIBDIR`); run it bare for a report, source it for the exported environment |
| `local.conf` | Untracked, optional: `PAPERS_SRC=` / `ROFD_SRC=` for this machine |
| `patches/README.md` | Inventory of the rofd changes this backend carries (fork branch, upstream PR, retirement criterion, sync routine). The patch payloads live in the AUR `librofd-ffi` repository, never here |
| `.planning/<PLAN_ID>/` | Working memory: phase plan, research notes, progress log. Keep it current; it is tracked so parallel machines see the same state |

## Hard rules

- Verify behaviour against the real sources before asserting it. Papers-side claims
  cite files in the upstream repository pinned to tag `51.1`
  (<https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/>); only grep against a local
  `$PAPERS_SRC` when one exists — the build needs just the installed
  `papers-document-4.0` headers. rofd-side claims use `$ROFD_SRC`. Cite `file:line`
  when adding a claim to `docs/plan.md`; never invent an API from memory.
- Dependency checkouts are machine-local and are never assumed: `scripts/dev-env.sh`
  resolves them (environment → untracked `local.conf` → sibling defaults `../papers`,
  `../rofd`), validates them against a marker file, and exports `PAPERS_SRC`,
  `ROFD_SRC`, `ROFD_INCLUDEDIR`, `ROFD_LIBDIR`. A checkout directory that is missing,
  empty, or a different OFD project of the same name (e.g. `office-rs/rofd`, which
  has a different API and is not a substitute; plan §10) is reported, never used
  silently.
- Never patch a local Papers checkout or the installed Papers files: Papers-side
  changes land as an upstream MR (plan §5 phase 4). rofd is fork-first (plan D11):
  changes go to a `feat/…` branch of `YangtseSu/rofd` (its `main` mirrors
  `linuxdeepin/rofd`) and ship through our AUR `librofd-ffi` package, whose own
  repository carries the patch files the package applies. Open the upstream PR
  too, but never wait on it; when it merges, delete the patch from the AUR package
  and drop the branch. `patches/README.md` is the inventory and the sync routine.
- Keep every recipe and payload in exactly one place: the Arch recipes
  (`librofd-ffi`, `papers-ofd`) are maintained in their AUR repositories only, and
  the rofd patch files live in the `librofd-ffi` repository — this repository
  carries no PKGBUILD and no patch payload.
- Never commit anything that identifies the user or their machine: no names of
  real documents (they stay outside the repository), no absolute home paths (write
  `~`-relative at most), no build artifacts or caches (`__pycache__/`,
  `_build/`, generated logs — they embed machine paths). Refer to such material by
  role only: "the user's invoice, kept outside the repository". `.planning/` is
  tracked and public, so it obeys the same rule as `docs/`: it records what was
  verified and the numbers, never the file names.
- Implement the phases in order. A phase counts as done only when its acceptance
  items have been *observed* and the evidence is recorded in
  `.planning/<PLAN_ID>/progress.md`.
- If an interface is implemented, populate **every** vfunc: `PpsDocumentLinks`,
  `PpsDocumentFind::find_text`, `PpsSelection::get_selected_text`,
  `PpsDocumentPrint::print_page` and `PpsDocument::save` are dispatched without
  NULL checks upstream (plan §3).
- Convert every rofd rectangle through the origin-normalizing helper (plan §4);
  never open-code millimetre/point conversions.
- Call `rofd_render_options_init` before setting render options and never leave
  `max_raster_bytes` zero (plan §4).
- Do not add a document-wide render lock: the ABI permits concurrent read-only
  calls; guard only our own mutable state (plan D2).

## Build and verify

```bash
. scripts/dev-env.sh         # resolve dependencies, report what this machine has

# rofd ABI library: built from the checkout when it exists, otherwise the script
# points at the installed librofd-ffi package (a checkout is not required)
if [ -d "$ROFD_SRC" ]; then
  (cd "$ROFD_SRC" && cargo build --release --offline -p rofd-ffi)
fi
if [ "$ROFD_ABI_SOURCE" = checkout ]; then
  ln -sf librofd_ffi.so "$ROFD_LIBDIR/librofd_ffi.so.0"   # SONAME name
fi
meson setup _build -Drofd_libdir="$ROFD_LIBDIR" \
                   -Drofd_includedir="$ROFD_INCLUDEDIR"
meson compile -C _build && meson test -C _build
reuse lint
```

Runtime checks (see plan §6 for the full playbook): copy the whole system backend
directory (modules *and* descriptors — descriptors alone break the other backends)
and overlay this module, stage `data/papers-ofd.xml` into a scratch
`XDG_DATA_DIRS` tree so `application/ofd` resolves, then run
`PPS_BACKENDS_DIR=$PWD/_build/devbackends papers <fixture>.ofd`,
`papers <some>.pdf` (regression) and
`papers-thumbnailer -s 256 <fixture>.ofd /tmp/out.png`. External fixtures live
under `$ROFD_SRC` (plan §6).

## Conventions

- Code, comments, commits, docs: English. Commit subjects: imperative, one line,
  body explaining motivation and listing the verification commands.
- Licensing: REUSE-compliant, `GPL-2.0-or-later` for this repository (see
  `REUSE.toml`). A patch file added back under `patches/` is rofd-derived and
  follows the upstream project's license (`LGPL-2.1-or-later`).
- Prefer `g_debug` for parse warnings and render diagnostics; never fail a render
  because of a diagnostic.
