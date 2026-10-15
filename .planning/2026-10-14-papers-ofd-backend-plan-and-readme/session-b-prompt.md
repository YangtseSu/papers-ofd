# Session B prompt — phases 22–24

Paste the block below into a fresh session started in this repository. It is
self-contained on purpose: the session has no conversation history.

---

## papers-ofd — session B: phases 22–24 of the review-fix campaign

Repository and working directory: the `papers-ofd` checkout you were started in
(the GNOME Papers OFD backend). Nothing carries over from the previous session;
everything you need is on disk. Answer the user in Chinese; write code, comments,
commits and docs in English.

### 0. Restore state before anything else

1. `PLAN_ID` is deliberately unset; run
   `sh ~/.agents/skills/planning-with-files/scripts/resolve-plan-dir.sh`. It must
   select `.planning/2026-10-14-papers-ofd-backend-plan-and-readme`. If `PLAN_ID`
   is set in your environment it is a binding: resolve it or stop, never fall back
   to another plan.
2. Read, in this order:
   - `.planning/<plan>/task_plan.md` — `## Next Step`, phases 22–24,
     Key Questions 7–10 (all four **decided**), Decisions Made;
   - `.planning/<plan>/progress.md` — §"Phase 22 test design" and the two session-A
     entries (phase 20, phase 21) for the evidence format;
   - `.planning/<plan>/findings.md` — §"Review findings (2026-10-15)" (R1–R19) and
     §"Closed by phases 20–21";
   - `docs/plan.md` §3 (interface contract) and §4 (render math) — the authority.
3. `git log --oneline -5` and `git status --short`: `main` is at `4b56b0d` with a
   clean tree; `meson test` is 13/13 in `_build` and `_build-slow`.
4. `. scripts/dev-env.sh` (bash and zsh both work now), then the
   `meson setup --reconfigure _build -Drofd_libdir=… -Drofd_includedir=… -Drofd_src=…`
   line from README's "Build and try it".

### 1. Scope: session B = phases 22, 23, 24 only

Decisions already taken by the user — implement them, do not re-litigate:

| Item | Decision |
| --- | --- |
| R3 render budget | derive `max_raster_bytes` from the request (rofd's worst case is ≈9·w·h, against Papers' 4 B/px allowance) under a module cap; beyond the cap lower the scale instead of returning NULL; the page texture and every overlay use the same fitted options |
| R4 fonts | declare in the AUR `papers-ofd` recipe the fonts the suite measures with, and make the in-repo test name the missing font/package instead of failing with an opaque assertion |
| R7 concurrency | `GRWLock`: writer in `load`/`load_fd`, reader around every vfunc — poppler's model (`libview/pps-poppler.c`) |
| Release | one `51.1.2` after R3/R4/R10. Do **not** tag, release or bump the version in this session |

Session A's rule stays in force: **test-first**. Every behavioural change gets a
test that fails before it and passes after, and a failing test is never committed.

#### Phase 22 — render robustness (R3, R14, R15)

Start by re-creating the two tests described in `progress.md` §"Phase 22 test
design" (`test_high_zoom_still_renders`, `test_oblique_rotation_snaps` in
`tests/test-ofd-render.c`), observe them fail, then implement in
`src/ofd-render.c`:

- R3: budget derivation plus the degrade path, in one place so
  `ofd_render_page_surface` and `ofd_render_page_canvas_size` cannot disagree;
- R14: snap a non-quarter-turn rotation to the nearest quarter turn (Papers'
  model accepts any 0–360, rofd rejects anything but 0/90/180/270);
- R15: a failed `rofd_renderer_render_page_cairo` reports at `g_warning`;
  object-level diagnostics stay at `g_debug` (D9).

Then re-measure `tests/ofd-latency` if the render path's numbers move, and update
`docs/plan.md` §4/§8 plus README's limitation table with the budget rule.

#### Phase 23 — concurrency hardening (R7)

Replace the single `GMutex` with the decided `GRWLock` (writer in
`load`/`load_fd`, reader around every vfunc, including the outline and links
paths that borrow the handle). Write the stress test first: worker threads
rendering pages while the main thread reloads the same document instance over the
URI path and the fd path. Show the pre-fix failure under
`meson setup _build-asan -Db_sanitize=address …` (a use-after-free is what the
review predicted; if the pre-fix run is not deterministic, record that honestly)
and the post-fix clean run; keep a short deterministic version in the suite.
Quote rofd.h's handle-lifetime rule in the code comment and reword D2/plan §2.

#### Phase 24 — CI and packaging parity (R4, R10)

- the in-repo half of R4: the text test's failure names the missing font/package;
- `meson.build`: distinguish "rofd-ffi.pc too old" from "not found", and settle
  whether the documented floor moves to 0.4.3;
- CI: a second configure/build/test for `-Drofd_page_size=disabled`, and a
  configure pass over the installed `rofd-ffi.pc` path (or a recorded reason why
  it stays out);
- `ofd-latency`: either a registered check with an assertion or an explicitly
  documented tool outside `meson test`.

The AUR recipes (`papers-ofd`, `librofd-ffi`) live in their own repositories;
this repository carries no PKGBUILD (AGENTS.md, "one home per recipe").
Prepare the recipe change as a diff or a checklist, but the AUR push is the
user's step — ask for it, never push.

### 2. Rules that bite in this repository

- `docs/plan.md` is the authoritative contract; cite `file:line` for every claim,
  `$ROFD_SRC` for rofd, upstream tag `51.1` for Papers (a local `$PAPERS_SRC` is
  only a grepping convenience). Verify before asserting; never invent an API.
- Docs are updated in the same commit as the behaviour.
- Commits: imperative subject, body explains motivation and lists the verification
  commands. Never leave the tree red or the suite failing.
- Exactly one `GMutex`-style lock discipline for our own state (now the rwlock),
  never a document-wide render lock beyond it.
- Never commit anything identifying the user or their machine.
- Keep `.planning/<plan>/` current: phase status, `## Next Step`, one `progress.md`
  entry per phase, and a `findings.md` note for anything that contradicts the
  review (R11 was withdrawn that way — probing beats assuming).

### 3. Acceptance for session B

- `meson test` 13/13 plus the new cases in `_build` and `_build-slow`;
  `reuse lint` compliant.
- The high-zoom render returns a surface at every tested scale, with the overlay
  matching its dimensions; a 45° rotation renders as the nearest quarter turn.
- The concurrency stress test passes, with its pre-fix failure (ASAN or a
  documented non-deterministic result) on record.
- The CI/recipe changes are either verified locally in the container-equivalent
  sense or listed as user steps in `progress.md`.
- Report in Chinese, terse and evidence-first: what ran, what it printed, what is
  left.

### 4. Out of scope in this session

Phases 25 (documentation corrections R19) and 26 (the `51.1.2` release, AUR
repackage, the still-open `pacman -Rns` uninstall regression), and the GUI
acceptance items (double-click/triple-click/Ctrl+C and PageDown after a rotation).
Name them as remaining work; do not start them.
