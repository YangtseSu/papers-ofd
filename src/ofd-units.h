// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>
#include <rofd.h>

G_BEGIN_DECLS

/* Papers page space is points, OFD is millimetres (plan §4). */
#define OFD_PT_PER_MM (72.0 / 25.4)

double ofd_mm_to_pt (double mm);
double ofd_pt_to_mm (double pt);

/* The single conversion point between rofd rectangles and Papers rectangles
 * (decision D3): rofd keeps absolute coordinates while its renderer maps the
 * page-box origin to canvas (0,0), so every rectangle crossing the boundary
 * subtracts (or, for inputs, adds back) that origin. */
void ofd_rect_from_mm (const rofd_rect_t *rect,
                       const rofd_rect_t *origin,
                       PpsRectangle *out);
void ofd_rect_to_mm (const PpsRectangle *rect,
                     const rofd_rect_t *origin,
                     rofd_rect_t *out);

/* Map a page-space rectangle (points, top-left origin) onto a pixel grid with
 * the half-pixel bias Papers' own backends use for regions
 * (`create_region_from_poppler_region`, libdocument/backend/pdf/pps-poppler.c),
 * so a region built here covers the same pixels as the page texture. With
 * (1.0, 1.0) it produces the integer page-point rectangles a cairo_region_t
 * needs for hit testing. */
void ofd_pixel_rect_from_points (const PpsRectangle *rect,
                                 double scale_x,
                                 double scale_y,
                                 cairo_rectangle_int_t *out);

G_END_DECLS
