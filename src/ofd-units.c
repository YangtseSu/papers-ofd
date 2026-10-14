// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-units.h"

double
ofd_mm_to_pt (double mm)
{
	return mm * OFD_PT_PER_MM;
}

double
ofd_pt_to_mm (double pt)
{
	return pt / OFD_PT_PER_MM;
}

void
ofd_rect_from_mm (const rofd_rect_t *rect,
                  const rofd_rect_t *origin,
                  PpsRectangle *out)
{
	double x = rect->x_mm - origin->x_mm;
	double y = rect->y_mm - origin->y_mm;

	out->x1 = ofd_mm_to_pt (x);
	out->y1 = ofd_mm_to_pt (y);
	out->x2 = ofd_mm_to_pt (x + rect->width_mm);
	out->y2 = ofd_mm_to_pt (y + rect->height_mm);
}

void
ofd_pixel_rect_from_points (const PpsRectangle *rect,
                            double scale_x,
                            double scale_y,
                            cairo_rectangle_int_t *out)
{
	out->x = (gint) (rect->x1 * scale_x + 0.5);
	out->y = (gint) (rect->y1 * scale_y + 0.5);
	out->width = (gint) ((rect->x2 - rect->x1) * scale_x + 0.5);
	out->height = (gint) ((rect->y2 - rect->y1) * scale_y + 0.5);
}

void
ofd_rect_to_mm (const PpsRectangle *rect,
                const rofd_rect_t *origin,
                rofd_rect_t *out)
{
	double x1 = ofd_pt_to_mm (MIN (rect->x1, rect->x2));
	double y1 = ofd_pt_to_mm (MIN (rect->y1, rect->y2));
	double x2 = ofd_pt_to_mm (MAX (rect->x1, rect->x2));
	double y2 = ofd_pt_to_mm (MAX (rect->y1, rect->y2));

	out->x_mm = x1 + origin->x_mm;
	out->y_mm = y1 + origin->y_mm;
	out->width_mm = x2 - x1;
	out->height_mm = y2 - y1;
}
