// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>
#include <rofd.h>

G_BEGIN_DECLS

/* Map a Papers render context onto rofd render options (plan §4).
 * pps_render_context_compute_scaled_size is the only correct source of the
 * pixel size, and dpi=72 makes rofd's scale numerically identical to Papers'
 * points-to-pixels factor. */
void ofd_render_options_from_context (PpsRenderContext *rc,
                                      double width_pt,
                                      double height_pt,
                                      rofd_render_options_t *options);

/* Render one page into a new ARGB32 surface sized by rofd itself, so the
 * surface and rofd's internal CTM cannot disagree (plan §4). Returns NULL on
 * failure. */
cairo_surface_t *ofd_render_page_surface (rofd_renderer_t *renderer,
                                          rofd_page_t *page,
                                          PpsRenderContext *rc,
                                          double width_pt,
                                          double height_pt);

/* Draw a page onto an existing target (printing): dpi=72, scale=1, no rotation. */
gboolean ofd_render_page_onto (rofd_renderer_t *renderer,
                               rofd_page_t *page,
                               cairo_t *cr,
                               const rofd_render_options_t *options);

G_END_DECLS
