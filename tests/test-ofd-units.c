// SPDX-License-Identifier: GPL-2.0-or-later

/* Unit test for the single rectangle-conversion point (decision D3) and the
 * millimetre/point helpers (plan §4). */

#include "ofd-units.h"

#include <math.h>

static void
test_mm_pt_roundtrip (void)
{
	g_assert_cmpfloat_with_epsilon (ofd_mm_to_pt (25.4), 72.0, 1e-9);
	g_assert_cmpfloat_with_epsilon (ofd_pt_to_mm (72.0), 25.4, 1e-9);
	g_assert_cmpfloat_with_epsilon (ofd_pt_to_mm (ofd_mm_to_pt (210.0)), 210.0, 1e-9);
}

static void
test_rect_origin_zero (void)
{
	rofd_rect_t box = { 0.0, 0.0, 210.0, 297.0 };
	rofd_rect_t origin = { 0.0, 0.0, 0.0, 0.0 };
	PpsRectangle out;

	ofd_rect_from_mm (&box, &origin, &out);

	g_assert_cmpfloat_with_epsilon (out.x1, 0.0, 1e-9);
	g_assert_cmpfloat_with_epsilon (out.y1, 0.0, 1e-9);
	g_assert_cmpfloat_with_epsilon (out.x2, ofd_mm_to_pt (210.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.y2, ofd_mm_to_pt (297.0), 1e-9);
}

static void
test_rect_normalizes_origin (void)
{
	/* The plan's non-zero-origin fixture: PhysicalBox "10 20 210 297". */
	rofd_rect_t box = { 0.0, 0.0, 210.0, 297.0 };
	rofd_rect_t origin = { 10.0, 20.0, 0.0, 0.0 };
	rofd_rect_t roundtrip;
	PpsRectangle out;
	PpsRectangle in;

	ofd_rect_from_mm (&box, &origin, &out);
	g_assert_cmpfloat_with_epsilon (out.x1, ofd_mm_to_pt (-10.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.y1, ofd_mm_to_pt (-20.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.x2, ofd_mm_to_pt (200.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.y2, ofd_mm_to_pt (277.0), 1e-9);

	in.x1 = ofd_mm_to_pt (5.0);
	in.y1 = ofd_mm_to_pt (7.0);
	in.x2 = ofd_mm_to_pt (15.0);
	in.y2 = ofd_mm_to_pt (19.0);
	ofd_rect_to_mm (&in, &origin, &roundtrip);
	g_assert_cmpfloat_with_epsilon (roundtrip.x_mm, 15.0, 1e-9);
	g_assert_cmpfloat_with_epsilon (roundtrip.y_mm, 27.0, 1e-9);
	g_assert_cmpfloat_with_epsilon (roundtrip.width_mm, 10.0, 1e-9);
	g_assert_cmpfloat_with_epsilon (roundtrip.height_mm, 12.0, 1e-9);
}

static void
test_rect_to_mm_normalizes_input_order (void)
{
	rofd_rect_t origin = { 0.0, 0.0, 0.0, 0.0 };
	rofd_rect_t out;
	PpsRectangle in = { 10.0, 20.0, 2.0, 5.0 /* x1 > x2, y1 > y2 */ };

	ofd_rect_to_mm (&in, &origin, &out);

	g_assert_cmpfloat_with_epsilon (out.x_mm, ofd_pt_to_mm (2.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.y_mm, ofd_pt_to_mm (5.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.width_mm, ofd_pt_to_mm (8.0), 1e-9);
	g_assert_cmpfloat_with_epsilon (out.height_mm, ofd_pt_to_mm (15.0), 1e-9);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/ofd/units/mm-pt-roundtrip", test_mm_pt_roundtrip);
	g_test_add_func ("/ofd/units/rect-origin-zero", test_rect_origin_zero);
	g_test_add_func ("/ofd/units/rect-normalizes-origin", test_rect_normalizes_origin);
	g_test_add_func ("/ofd/units/rect-to-mm-order", test_rect_to_mm_normalizes_input_order);

	return g_test_run ();
}
