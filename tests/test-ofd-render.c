// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-1 acceptance, headless (plan §5): the rotation matrix and the origin
 * normalization on real rendered pixels, the zoom matrix against Papers'
 * requested size and rofd's own canvas size, byte-equality with a direct FFI
 * render of the same options, and the print path into a PDF surface whose text
 * is still selectable.
 *
 * The view calls pps_document_render and print calls
 * pps_document_print_print_page, so exercising both here covers what the shell
 * shows and what the print operation draws. */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <papers-view.h>
#include <poppler.h>
#include <rofd.h>

#include <cairo-pdf.h>
#include <math.h>
#include <glib/gstdio.h>

/* ARGB32 pixel, and "the marker is black" / "the paper is white" tests with a
 * tolerance for the antialiased edge of the path. */
static gboolean
pixel_is_dark (cairo_surface_t *surface, gint x, gint y)
{
	guint32 *data = (guint32 *) cairo_image_surface_get_data (surface);
	gint stride = cairo_image_surface_get_stride (surface) / 4;
	guint32 pixel = data[y * stride + x];
	guint8 red = (pixel >> 16) & 0xff;
	guint8 green = (pixel >> 8) & 0xff;
	guint8 blue = pixel & 0xff;

	return red < 60 && green < 60 && blue < 60;
}

static gboolean
pixel_is_white (cairo_surface_t *surface, gint x, gint y)
{
	guint32 *data = (guint32 *) cairo_image_surface_get_data (surface);
	gint stride = cairo_image_surface_get_stride (surface) / 4;
	guint32 pixel = data[y * stride + x];
	guint8 red = (pixel >> 16) & 0xff;
	guint8 green = (pixel >> 8) & 0xff;
	guint8 blue = pixel & 0xff;

	return red > 250 && green > 250 && blue > 250;
}

static cairo_surface_t *
render_page (PpsDocument *document,
             gint index,
             gint rotation,
             gdouble scale,
             gint target_width,
             gint target_height)
{
	PpsPage *page = pps_document_get_page (document, index);
	PpsRenderContext *rc;
	cairo_surface_t *surface;

	g_assert_nonnull (page);

	rc = pps_render_context_new (page, rotation, scale, PPS_RENDER_ANNOTS_NONE);
	if (target_width >= 0)
		pps_render_context_set_target_size (rc, target_width, target_height);

	surface = pps_document_render (document, rc);
	g_assert_nonnull (surface);
	g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

	g_object_unref (rc);
	g_object_unref (page);

	return surface;
}

/* Plan §5: the corner marker stays in the page corner it started in while the
 * page turns clockwise — top-left, top-right, bottom-right, bottom-left. */
static void
test_rotation_matrix (PpsDocument *document)
{
	static const struct {
		gint rotation;
		gboolean top;
		gboolean left;
	} cases[] = {
		{ 0, TRUE, TRUE },
		{ 90, TRUE, FALSE },
		{ 180, FALSE, FALSE },
		{ 270, FALSE, TRUE },
	};
	const gint inset = 30; /* inside the marker (14..71 px at 72 dpi) */
	gint upright_width = 0, upright_height = 0;
	guint i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		cairo_surface_t *surface =
			render_page (document, 0, cases[i].rotation, 1.0, -1, -1);
		gint width = cairo_image_surface_get_width (surface);
		gint height = cairo_image_surface_get_height (surface);
		gint marker_x, marker_y;

		/* rofd's canvas size is authoritative (it rounds itself); use the
		 * upright render as the reference for the rotated dimensions. */
		if (i == 0) {
			upright_width = width;
			upright_height = height;
		} else if (cases[i].rotation == 90 || cases[i].rotation == 270) {
			g_assert_cmpint (width, ==, upright_height);
			g_assert_cmpint (height, ==, upright_width);
		} else {
			g_assert_cmpint (width, ==, upright_width);
			g_assert_cmpint (height, ==, upright_height);
		}

		marker_x = cases[i].left ? inset : width - inset;
		marker_y = cases[i].top ? inset : height - inset;

		g_assert_true (pixel_is_dark (surface, marker_x, marker_y));

		/* The other three corners must stay paper-white. */
		g_assert_true (pixel_is_white (surface,
		                               cases[i].left ? width - inset : inset,
		                               cases[i].top ? inset : height - inset));
		g_assert_true (pixel_is_white (surface,
		                               cases[i].left ? inset : width - inset,
		                               cases[i].top ? height - inset : inset));
		g_assert_true (pixel_is_white (surface,
		                               cases[i].left ? width - inset : inset,
		                               cases[i].top ? height - inset : inset));

		cairo_surface_destroy (surface);
	}
}

/* D3: rofd keeps absolute millimetre coordinates while the renderer maps the
 * page-box origin to canvas (0,0). A page whose box starts at (10, 20) with a
 * marker covering the box must therefore be black over the whole canvas; a
 * missing origin subtraction would leave it empty. */
static void
test_origin_normalization (PpsDocument *document)
{
	cairo_surface_t *surface = render_page (document, 0, 0, 1.0, -1, -1);
	gint width = cairo_image_surface_get_width (surface);
	gint height = cairo_image_surface_get_height (surface);
	gint x, y;
	gint dark = 0;
	gint samples = 0;

	/* A4 at 1.0x, as the render path rounds it (595 or 596 px wide). */
	g_assert_cmpint (width, >=, 595);
	g_assert_cmpint (width, <=, 596);
	g_assert_cmpint (height, >=, 842);
	g_assert_cmpint (height, <=, 843);

	for (y = 5; y < height; y += 37) {
		for (x = 5; x < width; x += 41) {
			samples++;
			if (pixel_is_dark (surface, x, y))
				dark++;
		}
	}

	g_assert_cmpint (dark, ==, samples);

	cairo_surface_destroy (surface);
}

/* Plan §4: the surface must be exactly the size rofd's own canvas query
 * returns for the options derived from the render context, and within the
 * rounding of what Papers asked for. */
static void
test_zoom_matrix (PpsDocument *document, rofd_renderer_t *renderer)
{
	static const gdouble scales[] = { 0.5, 1.0, 1.5, 2.0, 3.25 };
	guint i;

	for (i = 0; i < G_N_ELEMENTS (scales); i++) {
		PpsPage *page = pps_document_get_page (document, 0);
		double width_pt = 0.0, height_pt = 0.0;
		PpsRenderContext *rc;
		cairo_surface_t *surface;
		rofd_render_options_t options;
		rofd_error_t *rerror = NULL;
		int expected_width = 0, expected_height = 0;
		int canvas_width = 0, canvas_height = 0;

		pps_document_get_page_size_uncached (document, page, &width_pt, &height_pt);
		g_assert_cmpfloat (width_pt, >, 0.0);

		rc = pps_render_context_new (page, 0, scales[i], PPS_RENDER_ANNOTS_NONE);
		pps_render_context_compute_scaled_size (rc, width_pt, height_pt,
		                                        &expected_width, &expected_height);
		surface = pps_document_render (document, rc);
		g_assert_nonnull (surface);

		/* Plan §4: rofd's own canvas size wins over Papers' rounded estimate,
		 * with at most a one-pixel difference. */
		g_assert_cmpint (ABS (cairo_image_surface_get_width (surface) - expected_width),
		                 <=, 1);
		g_assert_cmpint (ABS (cairo_image_surface_get_height (surface) - expected_height),
		                 <=, 1);

		/* Same options, asked from rofd directly. */
		rofd_render_options_init (&options, sizeof (options));
		options.dpi = 72.0;
		options.scale = (double) expected_width / width_pt;
		options.rotation_degrees = 0;
		options.background_rgba = 0xFFFFFFFF;
		g_assert_cmpint (rofd_renderer_get_pixel_canvas_size (renderer,
		                                                      (rofd_page_t *) page->backend_page,
		                                                      &options, &canvas_width,
		                                                      &canvas_height,
		                                                      &rerror),
		                 ==, ROFD_STATUS_OK);
		g_assert_cmpint (cairo_image_surface_get_width (surface), ==, canvas_width);
		g_assert_cmpint (cairo_image_surface_get_height (surface), ==, canvas_height);

		cairo_surface_destroy (surface);
		g_object_unref (rc);
		g_object_unref (page);
	}

	/* rc->scale == 0.0 with a target size is the selection/thumbnail path (A7).
	 * Papers derives those targets from the page size at a scale, so they keep
	 * the page aspect ratio; the surface must be exactly that size. */
	{
		PpsPage *page = pps_document_get_page (document, 0);
		double width_pt = 0.0, height_pt = 0.0;
		const gint target_width = 800;
		gint target_height;
		cairo_surface_t *surface;

		pps_document_get_page_size_uncached (document, page, &width_pt, &height_pt);
		target_height = (gint) ceil (height_pt * target_width / width_pt);

		surface = render_page (document, 0, 0, 0.0, target_width, target_height);
		g_assert_cmpint (cairo_image_surface_get_width (surface), ==, target_width);
		g_assert_cmpint (cairo_image_surface_get_height (surface), ==, target_height);
		cairo_surface_destroy (surface);
		g_object_unref (page);
	}
}

static void
assert_surfaces_identical (cairo_surface_t *ours, cairo_surface_t *theirs)
{
	gint width = cairo_image_surface_get_width (ours);
	gint height = cairo_image_surface_get_height (ours);
	gint stride = cairo_image_surface_get_stride (ours);
	gint y;

	g_assert_cmpint (width, ==, cairo_image_surface_get_width (theirs));
	g_assert_cmpint (height, ==, cairo_image_surface_get_height (theirs));
	g_assert_cmpint (stride, ==, cairo_image_surface_get_stride (theirs));

	for (y = 0; y < height; y++) {
		const guint8 *a = cairo_image_surface_get_data (ours) + (gsize) y * stride;
		const guint8 *b = cairo_image_surface_get_data (theirs) + (gsize) y * stride;

		if (memcmp (a, b, stride) != 0)
			g_error ("page pixels differ from the direct FFI render at row %d", y);
	}
}

/* R3: Papers' own clamp admits about 10x on A4 — the page cache defaults to
 * 200 MiB and the view sizes the maximum zoom so a page's ARGB32 pixbuf fits it,
 * i.e. 4 B/px (libview/pps-view.c:5663-5675, data/org.gnome.Papers.gschema.xml:25-29)
 * — while rofd charges its worst case, 2*stride_argb32*h + stride_a8*h ≈ 9 B/px
 * against `max_raster_bytes`, whose default is 256 MiB
 * (cairo_renderer.rs:1246-1257). Every scale here is one Papers may ask for, so
 * every one must come back with a surface; a request beyond the module's own cap
 * may be scaled down, but not below half the requested size, and the selection
 * overlay the view draws on top has to have the texture's dimensions — which is
 * why the fit happens in one place (plan §4). */
static void
test_high_zoom_still_renders (PpsDocument *document)
{
	static const gdouble scales[] = { 4.0, 7.0, 8.0, 10.0, 12.0, 16.0 };
	guint i;

	for (i = 0; i < G_N_ELEMENTS (scales); i++) {
		PpsPage *page = pps_document_get_page (document, 0);
		double width_pt = 0.0, height_pt = 0.0;
		PpsRenderContext *rc;
		cairo_surface_t *surface;
		cairo_surface_t *overlay = NULL;
		PpsRectangle whole;
		GdkRGBA text = { 0.0, 0.0, 0.0, 1.0 };
		GdkRGBA base = { 0.0, 0.4, 1.0, 0.5 };
		gint requested_width = 0, requested_height = 0;
		gint texture_width, texture_height;

		g_assert_nonnull (page);
		pps_document_get_page_size_uncached (document, page, &width_pt, &height_pt);
		g_assert_cmpfloat (width_pt, >, 0.0);

		rc = pps_render_context_new (page, 0, scales[i], PPS_RENDER_ANNOTS_NONE);
		pps_render_context_compute_scaled_size (rc, width_pt, height_pt,
		                                        &requested_width, &requested_height);

		surface = pps_document_render (document, rc);
		g_assert_nonnull (surface);
		g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);

		texture_width = cairo_image_surface_get_width (surface);
		texture_height = cairo_image_surface_get_height (surface);
		g_assert_cmpint (texture_width, >, 0);
		g_assert_cmpint (texture_height, >, 0);
		/* Never larger than asked for, and never degraded into uselessness. */
		g_assert_cmpint (texture_width, <=, requested_width + 1);
		g_assert_cmpint (texture_height, <=, requested_height + 1);
		g_assert_cmpint (texture_width, >=, requested_width / 2);
		g_assert_cmpint (texture_height, >=, requested_height / 2);

		/* Both surfaces together are a few hundred megabytes at the top of this
		 * list, so the texture goes before the overlay is built and only its
		 * dimensions are kept. */
		cairo_surface_destroy (surface);

		whole.x1 = 0.0;
		whole.y1 = 0.0;
		whole.x2 = width_pt;
		whole.y2 = height_pt;

		pps_selection_render_selection (PPS_SELECTION (document), rc, &overlay, &whole,
		                                NULL, PPS_SELECTION_STYLE_GLYPH, &text, &base);
		g_assert_nonnull (overlay);
		g_assert_cmpint (cairo_image_surface_get_width (overlay), ==, texture_width);
		g_assert_cmpint (cairo_image_surface_get_height (overlay), ==, texture_height);

		cairo_surface_destroy (overlay);
		g_object_unref (rc);
		g_object_unref (page);
	}
}

/* R14: Papers' model accepts any rotation in 0-360
 * (pps-document-model.c:257-264,609-626) while rofd's renderer rejects everything
 * but a quarter turn (cairo_renderer.rs:1158-1163), so an oblique request used to
 * come back as NULL. It snaps to the nearest quarter turn: 45 degrees ties and
 * snaps up, which the 90-degree render proves pixel by pixel *and* by its swapped
 * dimensions (a blank page would have matched neither). */
static void
test_oblique_rotation_snaps (PpsDocument *document)
{
	cairo_surface_t *upright = render_page (document, 0, 0, 1.0, -1, -1);
	cairo_surface_t *quarter = render_page (document, 0, 90, 1.0, -1, -1);
	cairo_surface_t *oblique = render_page (document, 0, 45, 1.0, -1, -1);

	g_assert_cmpint (cairo_image_surface_get_width (quarter), ==,
	                 cairo_image_surface_get_height (upright));
	g_assert_cmpint (cairo_image_surface_get_height (quarter), ==,
	                 cairo_image_surface_get_width (upright));

	g_assert_cmpint (cairo_image_surface_get_width (oblique), ==,
	                 cairo_image_surface_get_width (quarter));
	g_assert_cmpint (cairo_image_surface_get_height (oblique), ==,
	                 cairo_image_surface_get_height (quarter));
	assert_surfaces_identical (oblique, quarter);

	cairo_surface_destroy (oblique);
	cairo_surface_destroy (quarter);
	cairo_surface_destroy (upright);
}

/* R15: a hard failure of the render path is the cause of a user-visible blank
 * page and must reach the log at g_warning, not at the g_debug the object-level
 * diagnostics of D9 use. The reachable shape is an oversized canvas: rofd's canvas
 * query bounds the dimensions against i32 only (region.rs:29-50 →
 * `CairoRenderer::pixel_canvas_size`), the module's own budget covers this request
 * (≈261 MB of the 450 MiB cap, so nothing is fitted), and cairo refuses to
 * allocate a surface above 32767 px with INVALID_SIZE (cairo 1.18, measured) —
 * which is where the failure is reported, because the render call is never
 * reached. rofd's own render-time failures (budget, decoded images, internal
 * errors) keep the same g_warning. The "strip" fixture is exactly that request: a
 * 1000x25 mm page at 12x is a 34016x851 pixel canvas. */
static gint render_warning_count;

static void
count_render_warning (const gchar *log_domain,
                      GLogLevelFlags log_level,
                      const gchar *message,
                      gpointer user_data)
{
	(void) log_domain;
	(void) log_level;
	(void) message;

	*((gint *) user_data) += 1;
}

static void
test_failed_render_is_a_warning (PpsDocument *document, rofd_renderer_t *renderer)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRenderContext *rc;
	rofd_render_options_t options;
	rofd_error_t *rerror = NULL;
	int canvas_width = 0, canvas_height = 0;
	guint handler;

	g_assert_nonnull (page);

	rc = pps_render_context_new (page, 0, 12.0, PPS_RENDER_ANNOTS_NONE);

	/* The options the module maps (dpi 72, the context's scale) must still pass
	 * rofd's canvas query, or the failure would come from there — a diagnostic at
	 * g_debug — instead of from the canvas itself. */
	rofd_render_options_init (&options, sizeof (options));
	options.dpi = 72.0;
	options.scale = 12.0;
	options.rotation_degrees = 0;
	options.background_rgba = 0xFFFFFFFF;
	g_assert_cmpint (rofd_renderer_get_pixel_canvas_size (renderer,
	                                                      (rofd_page_t *) page->backend_page,
	                                                      &options, &canvas_width,
	                                                      &canvas_height, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (canvas_width, ==, 34016);
	g_assert_cmpint (canvas_height, ==, 851);

	render_warning_count = 0;
	handler = g_log_set_handler ("papers-ofd", G_LOG_LEVEL_WARNING,
	                             count_render_warning, &render_warning_count);
	g_assert_null (pps_document_render (document, rc));
	g_log_remove_handler ("papers-ofd", handler);

	g_assert_cmpint (render_warning_count, ==, 1);

	g_object_unref (rc);
	g_object_unref (page);
}

/* Regression net for the option mapping (plan §5): rendering through the module
 * with dpi 72/scale 1/rotation 0 must be byte-identical to a direct rofd render
 * with those options, glyphs included (the metadata fixture carries text). */
static void
test_same_options_equivalence (PpsDocument *document, rofd_renderer_t *renderer)
{
	PpsPage *page = pps_document_get_page (document, 0);
	double width_pt = 0.0, height_pt = 0.0;
	PpsRenderContext *rc;
	cairo_surface_t *ours;
	cairo_surface_t *theirs;
	cairo_t *cr;
	rofd_page_t *rpage;
	rofd_render_options_t options;
	rofd_render_report_t *report = NULL;
	rofd_error_t *rerror = NULL;
	int canvas_width = 0, canvas_height = 0;

	pps_document_get_page_size_uncached (document, page, &width_pt, &height_pt);

	rc = pps_render_context_new (page, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
	ours = pps_document_render (document, rc);
	g_assert_nonnull (ours);

	rpage = (rofd_page_t *) page->backend_page;
	g_assert_nonnull (rpage);

	rofd_render_options_init (&options, sizeof (options));
	options.dpi = 72.0;
	options.scale = (double) cairo_image_surface_get_width (ours) / width_pt;
	options.rotation_degrees = 0;
	options.background_rgba = 0xFFFFFFFF;

	g_assert_cmpint (rofd_renderer_get_pixel_canvas_size (renderer, rpage, &options,
	                                                      &canvas_width, &canvas_height,
	                                                      &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (canvas_width, ==, cairo_image_surface_get_width (ours));
	g_assert_cmpint (canvas_height, ==, cairo_image_surface_get_height (ours));

	theirs = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, canvas_width, canvas_height);
	cr = cairo_create (theirs);
	g_assert_cmpint (rofd_renderer_render_page_cairo (renderer, rpage, cr, &options,
	                                                  &report, &rerror),
	                 ==, ROFD_STATUS_OK);
	cairo_destroy (cr);
	if (report != NULL)
		rofd_render_report_free (report);
	if (rerror != NULL)
		rofd_error_free (rerror);

	assert_surfaces_identical (ours, theirs);

	cairo_surface_destroy (theirs);
	cairo_surface_destroy (ours);
	g_object_unref (rc);
	g_object_unref (page);
}

/* Plan §5: printing must produce selectable (vector) text, not a bitmap. A PDF
 * surface gets exactly what PpsDocumentPrint::print_page draws; extraction with
 * poppler then proves the glyphs are text objects in the produced PDF. */
static void
test_print_to_pdf (PpsDocument *document, const gchar *output_dir)
{
	g_autofree gchar *pdf_path = g_build_filename (output_dir, "phase1-print.pdf", NULL);
	g_autofree gchar *uri = NULL;
	PpsPage *page = pps_document_get_page (document, 0);
	double width_pt = 0.0, height_pt = 0.0;
	cairo_surface_t *surface;
	cairo_t *cr;
	GError *error = NULL;
	PopplerDocument *pdf;
	PopplerPage *pdf_page;
	g_autofree gchar *text = NULL;

	g_assert_true (PPS_IS_DOCUMENT_PRINT (document));
	g_assert_true (pps_print_operation_exists_for_document (document));

	/* The print context is in points; the print operation transforms it. */
	pps_document_get_page_size_uncached (document, page, &width_pt, &height_pt);
	g_assert_cmpfloat (width_pt, >, 0.0);

	surface = cairo_pdf_surface_create (pdf_path, width_pt, height_pt);
	g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);
	cr = cairo_create (surface);
	cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
	cairo_paint (cr);
	pps_document_print_print_page (PPS_DOCUMENT_PRINT (document), page, cr);
	cairo_show_page (cr);
	cairo_destroy (cr);
	cairo_surface_finish (surface);
	g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);
	cairo_surface_destroy (surface);

	uri = g_filename_to_uri (pdf_path, NULL, &error);
	g_assert_no_error (error);
	pdf = poppler_document_new_from_file (uri, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (pdf);
	g_assert_cmpint (poppler_document_get_n_pages (pdf), ==, 1);

	pdf_page = poppler_document_get_page (pdf, 0);
	g_assert_nonnull (pdf_page);
	text = poppler_page_get_text (pdf_page);
	g_assert_nonnull (text);
	g_assert_nonnull (g_strstr_len (text, -1, "Invoice"));
	g_assert_nonnull (g_strstr_len (text, -1, "发票编号"));

	g_object_unref (pdf_page);
	g_object_unref (pdf);
	g_object_unref (page);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *corner_marker = NULL;
	g_autofree gchar *origin_offset = NULL;
	g_autofree gchar *metadata = NULL;
	g_autofree gchar *strip = NULL;
	g_autofree gchar *output_dir = NULL;
	PpsDocument *corner_marker_document;
	PpsDocument *origin_offset_document;
	PpsDocument *metadata_document;
	PpsDocument *strip_document;
	rofd_renderer_t *renderer = NULL;
	rofd_error_t *rerror = NULL;
	GError *error = NULL;

	if (argc < 8) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> "
		            "<corner-marker.ofd> <origin-offset.ofd> <metadata.ofd> "
		            "<strip.ofd>\n",
		            argv[0]);
		return 2;
	}

	corner_marker = g_canonicalize_filename (argv[4], NULL);
	origin_offset = g_canonicalize_filename (argv[5], NULL);
	metadata = g_canonicalize_filename (argv[6], NULL);
	strip = g_canonicalize_filename (argv[7], NULL);
	output_dir = g_dir_make_tmp ("papers-ofd-render-XXXXXX", &error);
	g_assert_no_error (error);

	corner_marker_document = ofd_test_load_fixture (argv[1], argv[2], argv[3], corner_marker);
	origin_offset_document = ofd_test_load_fixture (argv[1], argv[2], argv[3], origin_offset);
	metadata_document = ofd_test_load_fixture (argv[1], argv[2], argv[3], metadata);
	strip_document = ofd_test_load_fixture (argv[1], argv[2], argv[3], strip);

	/* A renderer with rofd's defaults, next to the module's own: the
	 * equivalence check compares the module against this one. */
	g_assert_cmpint (rofd_renderer_new (NULL, &renderer, &rerror), ==, ROFD_STATUS_OK);
	g_assert_nonnull (renderer);

	test_rotation_matrix (corner_marker_document);
	test_oblique_rotation_snaps (metadata_document);
	test_origin_normalization (origin_offset_document);
	test_zoom_matrix (metadata_document, renderer);
	test_high_zoom_still_renders (metadata_document);
	test_failed_render_is_a_warning (strip_document, renderer);
	test_same_options_equivalence (metadata_document, renderer);
	test_print_to_pdf (metadata_document, output_dir);

	rofd_renderer_free (renderer);
	g_object_unref (corner_marker_document);
	g_object_unref (origin_offset_document);
	g_object_unref (metadata_document);
	g_object_unref (strip_document);
	ofd_test_cleanup ();
	g_rmdir (output_dir);

	g_print ("phase-1 render matrix, budget fit, rotation snap, equivalence and "
	         "print checks passed\n");

	return 0;
}
