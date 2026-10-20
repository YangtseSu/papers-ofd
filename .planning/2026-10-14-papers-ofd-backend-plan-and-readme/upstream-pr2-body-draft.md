<!-- SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com> -->
<!-- SPDX-License-Identifier: GPL-2.0-or-later -->

# Draft body for upstream PR 2 (`feat/font-text-band`)

**Sent as <https://github.com/linuxdeepin/rofd/pull/16> on 2026-10-20**, after #13
merged upstream (`82120f6` + `8672ee6`) and the branch was rebased onto `main`
`f27fb87`. The body that was actually posted is the PR's, not this file's: the
"One deliberate reversal of #13's review" paragraph and the updated verification
list live there. This file is kept as the drafting record.

The text below was written while #13 was still open (hence "stacks on #13"); the
sent body replaced that framing.

---

Stacks on #13 (its branch is this PR's base) and needs it: the band spans the box that
#13 gives its advance.

A highlight that stops on the baseline cuts the bottom off the text it covers. Measured
on a real government document kept outside this repository (six embedded subset fonts,
sizes 4.9–7.8 mm): of 2464 glyphs with ink, **2117 (86%) reach below the baseline** —
CJK overshoot 0.098–0.129 em, Latin descenders up to 0.188 em — by a mean of 0.50 mm and
up to 1.46 mm, while the box top sat 1.45 mm above the painted top on average. A box
built from the font size cannot cover that, and it also leaves the vertical extent of a
box dependent on the glyph inside it, so punctuation and Latin do not line up with CJK
in one line.

Two commits:

- `TextScalarMetrics { advance_mm, ascent_mm, descent_mm }` replaces the advance-only
  payload. `ResolvedFont` reads the face's ascender and descender from FreeType once,
  while it validates the face; `GlyphMetrics` exposes them beside the advance; the
  builder spans the baseline sweep with that pair — the model poppler uses for a PDF
  text box (`TextWord::setInitialBounds`: `yMin = y - ascent`, `yMax = y - descent`).
  Every glyph of one face at one size therefore shares the band, which is what makes a
  line's boxes cover the ink and line up across scripts. A face with no usable metrics,
  a scalar no face owns, and a document loaded without a provider keep the conservative
  font-size footprint.
- A selection region grows that band by an eighth of its height above and below, the
  margin poppler's `TextSelectionSizer::visitLine` gives a PDF selection region, so a
  selected line reads as one band that clears the glyphs instead of ending on them.
  Character boxes and match rectangles keep the plain band, as poppler keeps them.

Alternative considered and not taken: spanning each glyph's own painted control box. It
covers the ink below the baseline as well, but the band then follows each glyph's ink, so
a line's highlight is ragged and does not line up across Latin, CJK and punctuation. The
face metrics are what a PDF reader paints, and they are what makes an OFD selection look
like a PDF one in the same viewer — that is why this branch takes them. The margin commit
is separate, so it can be dropped on its own if the extra growth is unwanted.

Consumers: the in-tree consumer builds one `Word` per character from
`rofd_text_layout_get_char` and paints those rectangles, and never calls the selection
API — so the margin commit changes nothing for it, and boxes that follow the renderer's
own metrics can only agree better with the glyphs it draws than they do today (its
per-character highlight boxes do become slightly taller, since they now include the
descent).

Verified: `cargo fmt --all -- --check`; `cargo clippy -p rofd-core -p rofd-render
-p rofd-ffi --all-targets -- -D warnings`; `cargo test` for the three crates;
`crates/rofd-ffi/tests/run_c_tests.sh` (the export set is unchanged). Two pixel-level
tests fail against the previous model and pass with this one:
`crates/rofd-render/tests/cairo_text.rs::semantic_character_boxes_cover_the_rendered_ink`
(the two scalars of one run now share top and bottom, and the band is strictly taller
than the ink) and
`crates/rofd-ffi/tests/semantic.rs::text_layout_rectangles_cover_the_rendered_ink_below_the_baseline`
(same band for A and 中, and the region equals the band plus the margin).
