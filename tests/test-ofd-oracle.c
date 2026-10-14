// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-1 oracle (plan §5): the real invoice fixture rendered through the module
 * at 254 dpi must match rofd's reviewed reference render
 * ($ROFD_SRC/tests/fixtures/reference/invoice-phase3.png), which was produced by
 * rofd-render with the explicit families "Noto Sans CJK SC" and "Noto Sans
 * Mono". The reference is pixel-exact in size and within tolerance in content:
 * glyph antialiasing depends on the installed font versions, so the check is a
 * mean-difference and outlier budget rather than equality.
 *
 * The externals live in the rofd checkout, so this test only exists when Meson
 * is configured with -Drofd_src=<checkout>. */

#include "ofd-test-utils.h"

#include <papers-document.h>

#include <cairo.h>
#include <glib/gstdio.h>
#include <math.h>

/* Budgets with headroom over the measured difference on the reference host
 * (mean 3.3; 2.6 % and 1.9 % of pixels over 8 and 64). */
#define MEAN_DIFF_LIMIT 10.0
#define OVER_EIGHT_LIMIT 0.08
#define OVER_SIXTYFOUR_LIMIT 0.05

static void
check_against_reference (PpsDocument *document, const gchar *reference_path)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRenderContext *rc;
	cairo_surface_t *surface;
	cairo_surface_t *reference;
	gint width, height;
	gint ref_width, ref_height;
	gint x, y;
	gint64 sum = 0;
	gint64 over_eight = 0;
	gint64 over_sixtyfour = 0;
	gdouble mean;
	gdouble fraction_eight;
	gdouble fraction_sixtyfour;
	gint64 pixels;

	g_assert_nonnull (page);

	rc = pps_render_context_new (page, 0, 254.0 / 72.0, PPS_RENDER_ANNOTS_NONE);
	surface = pps_document_render (document, rc);
	g_assert_nonnull (surface);

	reference = cairo_image_surface_create_from_png (reference_path);
	g_assert_cmpint (cairo_surface_status (reference), ==, CAIRO_STATUS_SUCCESS);

	width = cairo_image_surface_get_width (surface);
	height = cairo_image_surface_get_height (surface);
	ref_width = cairo_image_surface_get_width (reference);
	ref_height = cairo_image_surface_get_height (reference);
	g_assert_cmpint (width, ==, ref_width);
	g_assert_cmpint (height, ==, ref_height);

	cairo_surface_flush (surface);
	cairo_surface_flush (reference);

	for (y = 0; y < height; y++) {
		const guint8 *ours =
			cairo_image_surface_get_data (surface) +
			(gsize) y * cairo_image_surface_get_stride (surface);
		const guint8 *theirs =
			cairo_image_surface_get_data (reference) +
			(gsize) y * cairo_image_surface_get_stride (reference);

		for (x = 0; x < width; x++) {
			gint worst = 0;
			gint channel;

			for (channel = 0; channel < 3; channel++) {
				gint diff = ABS ((gint) ours[x * 4 + channel] -
				                 (gint) theirs[x * 4 + channel]);

				sum += diff;
				if (diff > worst)
					worst = diff;
			}
			if (worst > 8)
				over_eight++;
			if (worst > 64)
				over_sixtyfour++;
		}
	}

	pixels = (gint64) width * height;
	mean = (gdouble) sum / (3.0 * (gdouble) pixels);
	fraction_eight = (gdouble) over_eight / (gdouble) pixels;
	fraction_sixtyfour = (gdouble) over_sixtyfour / (gdouble) pixels;

	g_print ("oracle %dx%d: mean channel difference %.3f; over 8: %.2f%%; "
	         "over 64: %.2f%%\n",
	         width, height, mean, 100.0 * fraction_eight, 100.0 * fraction_sixtyfour);

	g_assert_cmpfloat (mean, <, MEAN_DIFF_LIMIT);
	g_assert_cmpfloat (fraction_eight, <, OVER_EIGHT_LIMIT);
	g_assert_cmpfloat (fraction_sixtyfour, <, OVER_SIXTYFOUR_LIMIT);

	cairo_surface_destroy (reference);
	cairo_surface_destroy (surface);
	g_object_unref (rc);
	g_object_unref (page);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *fixture = NULL;
	g_autofree gchar *reference = NULL;
	PpsDocument *document;

	if (argc < 6) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> "
		            "<learning/test.ofd> <invoice-phase3.png>\n",
		            argv[0]);
		return 2;
	}

	fixture = g_canonicalize_filename (argv[4], NULL);
	reference = g_canonicalize_filename (argv[5], NULL);

	/* D8: the reference was generated with these families; the override must be
	 * in place before the module creates its renderer (first render below). */
	g_setenv ("PAPERS_OFD_FALLBACK_FAMILIES", "Noto Sans CJK SC:Noto Sans Mono", TRUE);

	document = ofd_test_load_fixture (argv[1], argv[2], argv[3], fixture);
	check_against_reference (document, reference);
	g_object_unref (document);
	ofd_test_cleanup ();

	return 0;
}
