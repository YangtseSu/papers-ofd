# Repository Guidelines

`papers-ofd` is an out-of-tree GNOME Papers backend for OFD documents, written in C
against the `papers-document-4.0` API and the `rofd-ffi` C ABI. `docs/plan.md` is
the authoritative plan; read it before changing code.

## Layout

| Path | Purpose |
| --- | --- |
| `docs/plan.md` | Interface contract, decisions D1–D10, phases 0–4 with acceptance criteria, evidence index (every claim cites `file:line`) |
| `src/`, `data/`, `tests/` | Module sources, installed data files, tests (to be created by phase 0) |
| `patches/` | Diffs carried against an upstream dependency; each has a `README.md` with the removal criterion |
| `.planning/<PLAN_ID>/` | Working memory: phase plan, research notes, progress log. Keep it current; it is tracked so parallel machines see the same state |

## Hard rules

- Verify behaviour against the real sources before asserting it: Papers lives in
  `../papers`, rofd in `../rofd`. Cite `file:line` when adding a claim to
  `docs/plan.md`; never invent an API from memory.
- Never edit `../papers`. Changes to `../rofd` go through a fork branch plus an
  upstream PR, and land here as a patch under `patches/`.
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
cd ../rofd && cargo build --release --offline -p rofd-ffi
ln -sf librofd_ffi.so target/release/librofd_ffi.so.0        # SONAME name
cd ../papers-ofd
meson setup _build -Drofd_libdir=$PWD/../rofd/target/release \
                   -Drofd_includedir=$PWD/../rofd/crates/rofd-ffi/include
meson compile -C _build && meson test -C _build
reuse lint
```

Runtime checks (see plan §6 for the full playbook): build a dev backend directory
containing this module plus the system `*.papers-backend` descriptors, then run
`PPS_BACKENDS_DIR=$PWD/_build/devbackends papers <fixture>.ofd`,
`papers <some>.pdf` (regression) and
`papers-thumbnailer -s 256 <fixture>.ofd /tmp/out.png`.

## Conventions

- Code, comments, commits, docs: English. Commit subjects: imperative, one line,
  body explaining motivation and listing the verification commands.
- Licensing: REUSE-compliant, `GPL-2.0-or-later` for this repository (see
  `REUSE.toml`); files under `patches/` follow the upstream project's license.
- Prefer `g_debug` for parse warnings and render diagnostics; never fail a render
  because of a diagnostic.
