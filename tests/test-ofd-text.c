// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-2 acceptance, headless (plan §5): text, layout, area extraction, the
 * glyph mapping, find rectangles and selection (text, region and overlay
 * surface) of the OFD module, each compared against a direct rofd query on the
 * same fixture.
 *
 * The comparisons with rofd are the point: the shell copies text out of
 * get_selected_text, derives search snippets from the agreement between find
 * rectangles and layout rectangles, hits-tests the I-beam cursor against
 * get_text_mapping and draws the highlight from the selection region and
 * surface, so each of those must match what the ABI reports for the same
 * document and the same coordinates. */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <rofd.h>

#include <math.h>

#define MM(v) ((v) * 72.0 / 25.4)

/* The left text column and the right one, in millimetres from the fixture
 * generator (page 0 puts the columns at x=15 and x=120). */
#define LEFT_COLUMN_X1_MM 10.0
#define LEFT_COLUMN_X2_MM 112.0
#define COLUMN_Y1_MM 20.0
#define COLUMN_Y2_MM 60.0

/* Centre of the first layout rectangle whose left edge is at or right of `x_from_mm`,
 * in page points: the tests sample pixels the same way a user points at text. */
static gboolean
glyph_centre_after (const PpsRectangle *areas, guint n_areas, gdouble x_from_mm,
                    gdouble *x, gdouble *y)
{
	guint i;

	for (i = 0; i < n_areas; i++) {
		if (areas[i].x2 <= areas[i].x1 || areas[i].x1 < MM (x_from_mm))
			continue;

		*x = (areas[i].x1 + areas[i].x2) / 2;
		*y = (areas[i].y1 + areas[i].y2) / 2;

		return TRUE;
	}

	return FALSE;
}

static gchar *
oracle_page_text (rofd_document_t *document, gint index)
{
	rofd_page_t *page = NULL;
	rofd_string_t *string = NULL;
	rofd_error_t *rerror = NULL;
	gchar *text;

	g_assert_cmpint (rofd_document_get_page (document, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (rofd_page_get_text (page, &string, &rerror), ==, ROFD_STATUS_OK);

	text = g_strdup (rofd_string_get_data (string));

	rofd_string_free (string);
	rofd_page_free (page);

	return text;
}

static gchar *
oracle_text_in_area (rofd_document_t *document, gint index, PpsRectangle *area_pt)
{
	rofd_page_t *page = NULL;
	rofd_string_t *string = NULL;
	rofd_error_t *rerror = NULL;
	rofd_rect_t area_mm;
	gchar *text;

	g_assert_cmpint (rofd_document_get_page (document, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);

	area_mm.x_mm = MIN (area_pt->x1, area_pt->x2) / 72.0 * 25.4;
	area_mm.y_mm = MIN (area_pt->y1, area_pt->y2) / 72.0 * 25.4;
	area_mm.width_mm = ABS (area_pt->x2 - area_pt->x1) / 72.0 * 25.4;
	area_mm.height_mm = ABS (area_pt->y2 - area_pt->y1) / 72.0 * 25.4;

	g_assert_cmpint (rofd_page_get_text_for_area (page, &area_mm, &string, &rerror),
	                 ==, ROFD_STATUS_OK);

	text = g_strdup (rofd_string_get_data (string));

	rofd_string_free (string);
	rofd_page_free (page);

	return text;
}

static gchar *
oracle_selection_text (rofd_document_t *document,
                       gint index,
                       uint32_t style,
                       PpsRectangle *area_pt)
{
	rofd_page_t *page = NULL;
	rofd_text_selection_t *selection = NULL;
	rofd_error_t *rerror = NULL;
	rofd_rect_t area_mm;
	gchar *text;

	g_assert_cmpint (rofd_document_get_page (document, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);

	area_mm.x_mm = MIN (area_pt->x1, area_pt->x2) / 72.0 * 25.4;
	area_mm.y_mm = MIN (area_pt->y1, area_pt->y2) / 72.0 * 25.4;
	area_mm.width_mm = ABS (area_pt->x2 - area_pt->x1) / 72.0 * 25.4;
	area_mm.height_mm = ABS (area_pt->y2 - area_pt->y1) / 72.0 * 25.4;

	g_assert_cmpint (rofd_page_get_selected_text (page, style, &area_mm, &selection, &rerror),
	                 ==, ROFD_STATUS_OK);

	text = g_strdup (rofd_text_selection_get_text (selection));

	rofd_text_selection_free (selection);
	rofd_page_free (page);

	return text;
}

/* The scale the module maps a render context onto: Papers' rounded pixel size
 * over the page size (plan §4), which is why the oracle below uses it instead
 * of the raw context scale. */
static gdouble
papers_scale (PpsRenderContext *rc, double width_pt, double height_pt)
{
	gint scaled_width = 0;
	gint scaled_height = 0;

	pps_render_context_compute_scaled_size (rc, width_pt, height_pt,
	                                        &scaled_width, &scaled_height);

	return scaled_width / width_pt;
}

/* rofd's own canvas size for the options the module maps a render context onto
 * (plan §4); the selection overlay and its region must agree with the page
 * texture, which is sized by the same call. */
static void
oracle_canvas (rofd_renderer_t *renderer,
               rofd_page_t *page,
               gdouble scale,
               gint *width,
               gint *height)
{
	rofd_render_options_t options;
	rofd_error_t *rerror = NULL;

	rofd_render_options_init (&options, sizeof (options));
	options.dpi = 72.0;
	options.scale = scale;
	options.rotation_degrees = 0;
	options.background_rgba = 0xFFFFFFFF;

	g_assert_cmpint (rofd_renderer_get_pixel_canvas_size (renderer, page, &options,
	                                                      width, height, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_null (rerror);
}

static void
oracle_page_canvas (rofd_document_t *document,
                    rofd_renderer_t *renderer,
                    gint index,
                    gdouble scale,
                    gint *width,
                    gint *height)
{
	rofd_page_t *page = NULL;
	rofd_error_t *rerror = NULL;

	g_assert_cmpint (rofd_document_get_page (document, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);
	oracle_canvas (renderer, page, scale, width, height);
	rofd_page_free (page);
}

static void
oracle_page_size_pt (PpsDocument *document, gint index, double *width, double *height)
{
	PpsPage *page = pps_document_get_page (document, index);

	g_assert_nonnull (page);
	pps_document_get_page_size_uncached (document, page, width, height);
	g_object_unref (page);
}

/* Copied text equals the ABI's canonical text, byte for byte: the clipboard
 * path starts here, and the layout offsets index exactly these bytes. */
static void
test_text_matches_oracle (PpsDocument *document, rofd_document_t *oracle)
{
	static const gchar *needles[] = {
		"Invoice total: 42 EUR",
		"invoice appears lowercase",
	};
	gint i;

	for (i = 0; i < 2; i++) {
		PpsPage *page = pps_document_get_page (document, i);
		g_autofree gchar *ours = NULL;
		g_autofree gchar *theirs = NULL;

		g_assert_nonnull (page);
		ours = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page);
		theirs = oracle_page_text (oracle, i);

		g_assert_nonnull (ours);
		g_assert_cmpstr (ours, ==, theirs);
		g_object_unref (page);
	}

	for (i = 0; i < (gint) G_N_ELEMENTS (needles); i++)
		g_assert_true (g_strstr_len (oracle_page_text (oracle, 0), -1, needles[i]) != NULL);

	/* The CJK page carries the fixture's searchable phrase twice. */
	{
		g_autofree gchar *cjk = oracle_page_text (oracle, 1);

		g_assert_cmpint (g_utf8_strlen (cjk, -1), ==, 25);
		g_assert_nonnull (g_strstr_len (cjk, -1, "发票编号 42"));
	}
}

/* One rectangle per Unicode scalar, in page points, inside the page box; the
 * synthesised object separator has an empty box, like the PDF backend's
 * newline. */
static void
test_text_layout (PpsDocument *document)
{
	gint i;

	for (i = 0; i < 2; i++) {
		PpsPage *page = pps_document_get_page (document, i);
		g_autofree gchar *text = NULL;
		g_autofree PpsRectangle *areas = NULL;
		guint n_areas = 0;
		double width_pt = 0.0, height_pt = 0.0;
		const gchar *cursor;
		gint scalars = 0;

		g_assert_nonnull (page);
		text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page);
		g_assert_nonnull (text);
		g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
		                                                  page, &areas, &n_areas));

		oracle_page_size_pt (document, i, &width_pt, &height_pt);
		g_assert_cmpint (n_areas, ==, g_utf8_strlen (text, -1));

		for (cursor = text; *cursor != '\0'; cursor = g_utf8_next_char (cursor)) {
			PpsRectangle *area = &areas[scalars];

			if (*cursor == '\n') {
				/* The separator between two text objects has no geometry. */
				g_assert_cmpfloat (area->x1, ==, area->x2);
				g_assert_cmpfloat (area->y1, ==, area->y2);
			} else {
				g_assert_cmpfloat (area->x2, >, area->x1);
				g_assert_cmpfloat (area->y2, >, area->y1);
				g_assert_cmpfloat (area->x1, >=, -0.01);
				g_assert_cmpfloat (area->y1, >=, -0.01);
				g_assert_cmpfloat (area->x2, <=, width_pt + 0.01);
				g_assert_cmpfloat (area->y2, <=, height_pt + 0.01);
			}

			scalars++;
		}

		g_object_unref (page);
	}
}

static void
test_text_in_area (PpsDocument *document, rofd_document_t *oracle)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRectangle left = {
		.x1 = MM (LEFT_COLUMN_X1_MM), .y1 = MM (COLUMN_Y1_MM),
		.x2 = MM (LEFT_COLUMN_X2_MM), .y2 = MM (COLUMN_Y2_MM),
	};
	g_autofree gchar *ours = NULL;
	g_autofree gchar *theirs = NULL;

	g_assert_nonnull (page);

	ours = pps_document_text_get_text_in_area (PPS_DOCUMENT_TEXT (document), page, &left);
	theirs = oracle_text_in_area (oracle, 0, &left);

	g_assert_nonnull (ours);
	g_assert_cmpstr (ours, ==, theirs);
	g_assert_nonnull (g_strstr_len (ours, -1, "Invoice total: 42 EUR"));
	g_assert_nonnull (g_strstr_len (ours, -1, "Reference INV-42-ALPHA"));
	g_assert_null (g_strstr_len (ours, -1, "Second column"));

	g_object_unref (page);
}

/* The I-beam cursor and the page-scroll step hit-test this region in page
 * points, so it must cover the glyphs and nothing else. */
static void
test_text_mapping (PpsDocument *document)
{
	PpsPage *page = pps_document_get_page (document, 0);
	cairo_region_t *region = NULL;
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	cairo_rectangle_int_t extents;

	g_assert_nonnull (page);

	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));
	g_assert_cmpint (n_areas, >, 0);

	region = pps_document_text_get_text_mapping (PPS_DOCUMENT_TEXT (document), page);
	g_assert_nonnull (region);
	g_assert_false (cairo_region_is_empty (region));

	/* The first glyph's centre is text; the page corner is not. */
	g_assert_true (cairo_region_contains_point (region,
	                                            (gint) ((areas[0].x1 + areas[0].x2) / 2),
	                                            (gint) ((areas[0].y1 + areas[0].y2) / 2)));
	g_assert_false (cairo_region_contains_point (region, 2, 2));

	cairo_region_get_extents (region, &extents);
	g_assert_cmpint (extents.width, >, 0);
	g_assert_cmpint (extents.height, >, 0);

	cairo_region_destroy (region);
	g_object_unref (page);
}

static gint
find_match_count (PpsDocument *document, PpsPage *page, const gchar *query, PpsFindOptions options)
{
	GList *matches = pps_document_find_find_text (PPS_DOCUMENT_FIND (document), page,
	                                              query, options);
	gint count = g_list_length (matches);

	g_list_free_full (matches, (GDestroyNotify) pps_find_rectangle_free);

	return count;
}

/* Papers derives a snippet by walking the layout rectangles from the current
 * offset until one agrees with the match rectangle (get_match_offset,
 * libview/context/pps-search-context.c); a match that never agrees degrades to
 * a bare highlighted query, so the agreement is what the search acceptance
 * means. The scalar the walk lands on must also be where the query starts. */
static void
assert_match_agrees_with_layout (const gchar *text,
                                 const PpsRectangle *areas,
                                 guint n_areas,
                                 PpsFindRectangle *match,
                                 const gchar *query)
{
	gint start = -1;
	guint i;

	for (i = 0; i < n_areas; i++) {
		const PpsRectangle *area = &areas[i];
		gdouble center_x = (area->x1 + area->x2) / 2;
		gdouble center_y = (area->y1 + area->y2) / 2;
		gdouble match_y = (match->y1 + match->y2) / 2;

		if (match->x1 >= area->x1 && match->x1 < area->x2 &&
		    match_y >= area->y1 && match_y <= area->y2 &&
		    center_x >= match->x1 && center_x <= match->x2 &&
		    center_y >= match->y1 && center_y <= match->y2) {
			start = (gint) i;
			break;
		}
	}

	g_assert_cmpint (start, >=, 0);

	/* ... and the rectangle it landed on is where the query starts. */
	{
		const gchar *found = g_utf8_offset_to_pointer (text, start);

		g_assert_cmpuint (strlen (found), >=, strlen (query));
		g_assert_cmpint (g_ascii_strncasecmp (found, query, strlen (query)), ==, 0);
	}
}

static void
test_find (PpsDocument *document)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsPage *cjk_page = pps_document_get_page (document, 1);
	g_autofree gchar *text = NULL;
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	GList *matches, *l;

	g_assert_nonnull (page);
	g_assert_nonnull (cjk_page);

	/* The PDF backend's option set, and nothing else. */
	g_assert_cmpint (pps_document_find_get_supported_options (PPS_DOCUMENT_FIND (document)),
	                 ==, (PPS_FIND_CASE_SENSITIVE | PPS_FIND_WHOLE_WORDS_ONLY));

	/* Case-insensitive by default, case-sensitive and whole-word on request. */
	g_assert_cmpint (find_match_count (document, page, "invoice", 0), ==, 2);
	g_assert_cmpint (find_match_count (document, page, "invoice", PPS_FIND_CASE_SENSITIVE), ==, 1);
	g_assert_cmpint (find_match_count (document, page, "INVOICE", 0), ==, 2);
	g_assert_cmpint (find_match_count (document, page, "invo", 0), ==, 2);
	g_assert_cmpint (find_match_count (document, page, "invo", PPS_FIND_WHOLE_WORDS_ONLY), ==, 0);
	g_assert_cmpint (find_match_count (document, page, "invoice", PPS_FIND_WHOLE_WORDS_ONLY), ==, 2);
	g_assert_cmpint (find_match_count (document, page, "absent", 0), ==, 0);
	g_assert_cmpint (find_match_count (document, page, "", 0), ==, 0);

	/* CJK, where the fixture's phrase occurs twice. */
	g_assert_cmpint (find_match_count (document, cjk_page, "发票编号", 0), ==, 2);
	g_assert_cmpint (find_match_count (document, cjk_page, "发票编号 42", 0), ==, 2);
	g_assert_cmpint (find_match_count (document, cjk_page, "invoice", 0), ==, 0);

	text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page);
	g_assert_nonnull (text);
	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));

	matches = pps_document_find_find_text (PPS_DOCUMENT_FIND (document), page,
	                                       "invoice", 0);
	g_assert_cmpint (g_list_length (matches), ==, 2);
	for (l = matches; l != NULL; l = g_list_next (l))
		assert_match_agrees_with_layout (text, areas, n_areas,
		                                 (PpsFindRectangle *) l->data, "invoice");
	g_list_free_full (matches, (GDestroyNotify) pps_find_rectangle_free);

	g_object_unref (cjk_page);
	g_object_unref (page);
}

/* Copy and each selection style answer exactly what a direct FFI selection
 * answers for the same rectangle; the styles map 1:1 onto the ABI's. */
static void
test_selection_text (PpsDocument *document, rofd_document_t *oracle)
{
	static const struct {
		PpsSelectionStyle style;
		uint32_t rofd_style;
	} styles[] = {
		{ PPS_SELECTION_STYLE_GLYPH, ROFD_SELECTION_GLYPH },
		{ PPS_SELECTION_STYLE_WORD, ROFD_SELECTION_WORD },
		{ PPS_SELECTION_STYLE_LINE, ROFD_SELECTION_LINE },
	};
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRectangle whole = { 0, }, left = { 0, };
	gdouble width_pt = 0.0, height_pt = 0.0;
	guint i;

	g_assert_nonnull (page);
	oracle_page_size_pt (document, 0, &width_pt, &height_pt);

	whole.x2 = width_pt;
	whole.y2 = height_pt;
	left.x1 = MM (LEFT_COLUMN_X1_MM);
	left.y1 = MM (COLUMN_Y1_MM);
	left.x2 = MM (LEFT_COLUMN_X2_MM);
	left.y2 = MM (COLUMN_Y2_MM);

	for (i = 0; i < G_N_ELEMENTS (styles); i++) {
		g_autofree gchar *ours = NULL;
		g_autofree gchar *theirs = NULL;

		ours = pps_selection_get_selected_text (PPS_SELECTION (document), page,
		                                        styles[i].style, &whole);
		theirs = oracle_selection_text (oracle, 0, styles[i].rofd_style, &whole);

		g_assert_nonnull (ours);
		g_assert_cmpstr (ours, ==, theirs);
		g_assert_cmpint (strlen (ours), >, 0);
	}

	/* A drag-select over one column copies that column only. */
	{
		g_autofree gchar *ours = NULL;
		g_autofree gchar *theirs = NULL;

		ours = pps_selection_get_selected_text (PPS_SELECTION (document), page,
		                                        PPS_SELECTION_STYLE_GLYPH, &left);
		theirs = oracle_selection_text (oracle, 0, ROFD_SELECTION_GLYPH, &left);

		g_assert_cmpstr (ours, ==, theirs);
		g_assert_nonnull (g_strstr_len (ours, -1, "Invoice total"));
		g_assert_null (g_strstr_len (ours, -1, "Second column"));
	}

	/* A rectangle with no text selects nothing but never returns NULL. */
	{
		PpsRectangle empty = { .x1 = 5, .y1 = 5, .x2 = 20, .y2 = 20 };
		g_autofree gchar *ours = NULL;

		ours = pps_selection_get_selected_text (PPS_SELECTION (document), page,
		                                        PPS_SELECTION_STYLE_GLYPH, &empty);
		g_assert_nonnull (ours);
		g_assert_cmpstr (ours, ==, "");
	}

	g_object_unref (page);
}

/* The region is what the view draws and hit-tests in device pixels, so it must
 * be non-empty, inside the canvas and positioned by rofd's own canvas size. */
static void
test_selection_region (PpsDocument *document, rofd_document_t *oracle, rofd_renderer_t *renderer)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRectangle whole = { 0, };
	cairo_rectangle_int_t extents;
	gdouble width_pt = 0.0, height_pt = 0.0;
	gint canvas_width = 0, canvas_height = 0;
	cairo_region_t *region = NULL;
	g_autoptr (PpsRenderContext) rc = NULL;
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	gdouble glyph_x, glyph_y;
	cairo_rectangle_int_t base_extents;

	g_assert_nonnull (page);
	oracle_page_size_pt (document, 0, &width_pt, &height_pt);
	whole.x2 = width_pt;
	whole.y2 = height_pt;

	rc = pps_render_context_new (page, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
	region = pps_selection_get_selection_region (PPS_SELECTION (document), rc,
	                                             PPS_SELECTION_STYLE_GLYPH, &whole);
	g_assert_nonnull (region);
	g_assert_false (cairo_region_is_empty (region));

	oracle_page_canvas (oracle, renderer, 0, papers_scale (rc, width_pt, height_pt),
	                    &canvas_width, &canvas_height);
	cairo_region_get_extents (region, &extents);
	g_assert_cmpint (extents.x, >=, 0);
	g_assert_cmpint (extents.y, >=, 0);
	g_assert_cmpint (extents.x + extents.width, <=, canvas_width);
	g_assert_cmpint (extents.y + extents.height, <=, canvas_height);
	base_extents = extents;

	/* At scale 1 the region is in page points, so a glyph centre still hits it. */
	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));
	glyph_x = (areas[0].x1 + areas[0].x2) / 2;
	glyph_y = (areas[0].y1 + areas[0].y2) / 2;
	g_assert_true (cairo_region_contains_point (region, (gint) glyph_x, (gint) glyph_y));

	/* A target size makes rofd's canvas (and therefore the region) grow by the
	 * same factor; the render job's selection path passes scale 0 with a target
	 * size, so this is the shape Papers actually uses. */
	{
		gint target_width = canvas_width * 2;
		gint target_height = canvas_height * 2;
		gint scaled_width = 0, scaled_height = 0;
		g_autoptr (PpsRenderContext) target_rc = NULL;
		cairo_region_t *target_region = NULL;

		target_rc = pps_render_context_new (page, 0, 0.0, PPS_RENDER_ANNOTS_NONE);
		pps_render_context_set_target_size (target_rc, target_width, target_height);

		target_region = pps_selection_get_selection_region (PPS_SELECTION (document),
		                                                    target_rc,
		                                                    PPS_SELECTION_STYLE_GLYPH,
		                                                    &whole);
		g_assert_nonnull (target_region);
		g_assert_false (cairo_region_is_empty (target_region));

		oracle_page_canvas (oracle, renderer, 0, (gdouble) target_width / width_pt,
		                    &scaled_width, &scaled_height);
		cairo_region_get_extents (target_region, &extents);
		g_assert_cmpint (extents.x + extents.width, <=, scaled_width);
		g_assert_cmpint (extents.y + extents.height, <=, scaled_height);
		/* The extents follow the target size (rounding differences of a pixel
		 * or two against the doubled scale-1 size are expected). */
		g_assert_cmpint (extents.width, >, base_extents.width);
		g_assert_cmpint (extents.width, <=, scaled_width);

		cairo_region_destroy (target_region);
	}

	cairo_region_destroy (region);
	g_object_unref (page);
}

/* R1 (review 2026-10-15, blocker): Papers hands a *point* rectangle to the
 * selection vfuncs for a double or triple click (libview/pps-view.c:4235-4249;
 * compute_new_selection returns early only for GLYPH, :7190-7192) and for a
 * dead-straight drag (compute_scroll_increment builds x1 == x2, y1 == y2,
 * :920-931), and pps_view_get_selections recovers the selected bounds from the
 * selection region of exactly such a selection (:7498-7520). rofd's hit test is
 * a strict positive-area overlap (rofd-core/src/semantic.rs:504-507) and answers
 * success with an *empty* selection for a zero-area query, so the module widens a
 * degenerate query to a hair box: the point must then select the glyph it sits
 * on, and the word and line that contain it. */
static void
test_selection_point_rectangles (PpsDocument *document)
{
	PpsPage *page = pps_document_get_page (document, 0);
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	PpsRectangle glyph, word, line, point;
	g_autofree gchar *glyph_text = NULL;
	g_autofree gchar *word_text = NULL;
	g_autofree gchar *line_text = NULL;
	guint i;

	g_assert_nonnull (page);
	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));
	g_assert_cmpuint (n_areas, >, 4);

	/* The first glyph of the first line, and a rectangle covering that whole
	 * line of the left column (the right column shares its y band). */
	glyph = areas[0];
	word = areas[0];
	line = areas[0];
	for (i = 0; i < n_areas; i++) {
		if (areas[i].x2 <= areas[i].x1)
			continue; /* a synthesised separator carries no box */
		if (areas[i].y1 < glyph.y1 - 1.0 || areas[i].y1 > glyph.y1 + 1.0)
			continue; /* another line */
		if (areas[i].x1 >= MM (LEFT_COLUMN_X2_MM))
			continue; /* the right column shares the y band */
		line.x2 = MAX (line.x2, areas[i].x2);
	}

	point.x1 = point.x2 = (glyph.x1 + glyph.x2) / 2;
	point.y1 = point.y2 = (glyph.y1 + glyph.y2) / 2;

	glyph_text = pps_selection_get_selected_text (PPS_SELECTION (document), page,
	                                              PPS_SELECTION_STYLE_GLYPH, &glyph);
	word_text = pps_selection_get_selected_text (PPS_SELECTION (document), page,
	                                             PPS_SELECTION_STYLE_WORD, &word);
	line_text = pps_selection_get_selected_text (PPS_SELECTION (document), page,
	                                             PPS_SELECTION_STYLE_LINE, &line);
	g_assert_cmpint (strlen (glyph_text), >, 0);
	g_assert_cmpint (strlen (word_text), >, 0);
	g_assert_cmpint (strlen (line_text), >, 0);
	/* The line starts with its first word, which is what the point must pick. */
	g_assert_true (g_str_has_prefix (line_text, word_text));

	{
		g_autofree gchar *point_glyph = pps_selection_get_selected_text (
			PPS_SELECTION (document), page, PPS_SELECTION_STYLE_GLYPH, &point);
		g_autofree gchar *point_word = pps_selection_get_selected_text (
			PPS_SELECTION (document), page, PPS_SELECTION_STYLE_WORD, &point);
		g_autofree gchar *point_line = pps_selection_get_selected_text (
			PPS_SELECTION (document), page, PPS_SELECTION_STYLE_LINE, &point);

		g_assert_cmpint (strlen (point_glyph), >=, 1);
		g_assert_true (g_str_has_prefix (word_text, point_glyph));
		g_assert_cmpstr (point_word, ==, word_text);
		g_assert_cmpstr (point_line, ==, line_text);
	}

	/* The region of a point selection is what the view recovers the bounds from
	 * and what it hit-tests, so it must cover the same band as the line. */
	{
		g_autoptr (PpsRenderContext) rc = pps_render_context_new (page, 0, 1.0,
		                                                          PPS_RENDER_ANNOTS_NONE);
		cairo_region_t *point_region = pps_selection_get_selection_region (
			PPS_SELECTION (document), rc, PPS_SELECTION_STYLE_LINE, &point);
		cairo_region_t *line_region = pps_selection_get_selection_region (
			PPS_SELECTION (document), rc, PPS_SELECTION_STYLE_LINE, &line);
		cairo_rectangle_int_t point_extents, line_extents;

		g_assert_nonnull (point_region);
		g_assert_false (cairo_region_is_empty (point_region));
		cairo_region_get_extents (point_region, &point_extents);
		/* The line height PageUp/PageDown derives from this region. */
		g_assert_cmpint (point_extents.height, >, 0);

		g_assert_nonnull (line_region);
		cairo_region_get_extents (line_region, &line_extents);
		g_assert_cmpint (ABS (point_extents.x - line_extents.x), <=, 2);
		g_assert_cmpint (ABS (point_extents.width - line_extents.width), <=, 2);
		g_assert_cmpint (ABS (point_extents.y - line_extents.y), <=, 2);
		g_assert_cmpint (ABS (point_extents.height - line_extents.height), <=, 2);

		cairo_region_destroy (point_region);
		cairo_region_destroy (line_region);
	}

	g_object_unref (page);
}

/* R2 (review 2026-10-15, blocker): Papers dereferences the returned region
 * without a NULL check — `compute_scroll_increment` calls
 * cairo_region_num_rectangles on it (libview/pps-view.c:925-931) with a context
 * built from the *model rotation*, and cairo's region accessors dereference
 * unconditionally (a NULL argument exits 139). The region answer must therefore
 * always be a region, empty when the selection is declined or unavailable, which
 * is also the PDF backend's contract (pps-poppler.c:2007-2037). */
static void
test_selection_region_is_always_a_region (PpsDocument *document)
{
	static const gint rotations[] = { 0, 90, 180, 270 };
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRectangle whole = { 0, };
	gdouble width_pt = 0.0, height_pt = 0.0;
	guint i;

	g_assert_nonnull (page);
	oracle_page_size_pt (document, 0, &width_pt, &height_pt);
	whole.x2 = width_pt;
	whole.y2 = height_pt;

	for (i = 0; i < G_N_ELEMENTS (rotations); i++) {
		g_autoptr (PpsRenderContext) rc = pps_render_context_new (
			page, rotations[i], 1.0, PPS_RENDER_ANNOTS_NONE);
		cairo_region_t *region = pps_selection_get_selection_region (
			PPS_SELECTION (document), rc, PPS_SELECTION_STYLE_LINE, &whole);

		g_assert_nonnull (region);
		if (rotations[i] == 0)
			g_assert_false (cairo_region_is_empty (region));
		else
			g_assert_true (cairo_region_is_empty (region));

		cairo_region_destroy (region);
	}

	/* No rectangle at all is a caller error, not a crash: still a region. */
	{
		g_autoptr (PpsRenderContext) rc = pps_render_context_new (page, 0, 1.0,
		                                                          PPS_RENDER_ANNOTS_NONE);
		cairo_region_t *region = pps_selection_get_selection_region (
			PPS_SELECTION (document), rc, PPS_SELECTION_STYLE_GLYPH, NULL);

		g_assert_nonnull (region);
		g_assert_true (cairo_region_is_empty (region));
		cairo_region_destroy (region);
	}

	g_object_unref (page);
}

/* The overlay surface carries the base colour over the selection and nothing
 * else, and a reused surface is cleared first (the view keeps one per page). */
static void
test_render_selection (PpsDocument *document, rofd_document_t *oracle, rofd_renderer_t *renderer)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRectangle whole = { 0, }, left = { 0, };
	GdkRGBA base = { 1.0, 0.0, 0.0, 0.5 };
	GdkRGBA text = { 0.0, 0.0, 0.0, 0.0 };
	PpsRectangle right = { 0, };
	gdouble width_pt = 0.0, height_pt = 0.0;
	gint canvas_width = 0, canvas_height = 0;
	g_autoptr (PpsRenderContext) rc = NULL;
	cairo_surface_t *surface = NULL;
	cairo_surface_t *reused;
	guint32 *pixels;
	guint32 inside;
	gint stride;
	gint px, py;

	g_assert_nonnull (page);
	oracle_page_size_pt (document, 0, &width_pt, &height_pt);
	whole.x2 = width_pt;
	whole.y2 = height_pt;
	left.x1 = MM (LEFT_COLUMN_X1_MM);
	left.y1 = MM (COLUMN_Y1_MM);
	left.x2 = MM (LEFT_COLUMN_X2_MM);
	left.y2 = MM (COLUMN_Y2_MM);
	right.x1 = MM (115.0);
	right.y1 = MM (COLUMN_Y1_MM);
	right.x2 = MM (205.0);
	right.y2 = MM (COLUMN_Y2_MM);

	rc = pps_render_context_new (page, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
	oracle_page_canvas (oracle, renderer, 0, papers_scale (rc, width_pt, height_pt),
	                    &canvas_width, &canvas_height);

	pps_selection_render_selection (PPS_SELECTION (document), rc, &surface,
	                                &left, NULL, PPS_SELECTION_STYLE_GLYPH,
	                                &text, &base);
	g_assert_nonnull (surface);
	g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);
	g_assert_cmpint (cairo_image_surface_get_width (surface), ==, canvas_width);
	g_assert_cmpint (cairo_image_surface_get_height (surface), ==, canvas_height);

	cairo_surface_flush (surface);
	pixels = (guint32 *) cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface) / 4;

	/* Inside the left column, at the first glyph's own centre: the base colour
	 * shows through at the requested alpha, and the empty margin stays
	 * transparent. The sample point follows the layout, because the glyphs are
	 * narrower than the column the fixture declares. */
	{
		g_autofree PpsRectangle *areas = NULL;
		guint n_areas = 0;
		gdouble gx = 0.0, gy = 0.0;

		g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
		                                                  page, &areas, &n_areas));
		g_assert_true (glyph_centre_after (areas, n_areas, LEFT_COLUMN_X1_MM, &gx, &gy));
		px = (gint) gx;
		py = (gint) gy;
	}
	inside = pixels[py * stride + px];
	g_assert_cmpint ((inside >> 24) & 0xff, >=, 120);
	g_assert_cmpint ((inside >> 24) & 0xff, <=, 135);
	g_assert_cmpint ((inside >> 16) & 0xff, >=, 120);
	g_assert_cmpint ((inside >> 8) & 0xff, <=, 8);

	px = canvas_width - 5;
	py = canvas_height - 5;
	g_assert_cmpint ((pixels[py * stride + px] >> 24) & 0xff, ==, 0);

	/* Reuse: same surface, previous selection cleared. The second selection
	 * covers the other column, so the first one's pixels must go back to
	 * transparent while the new one lights up. */
	reused = surface;
	pps_selection_render_selection (PPS_SELECTION (document), rc, &reused,
	                                &right, NULL, PPS_SELECTION_STYLE_GLYPH,
	                                &text, &base);
	g_assert_true (reused == surface);

	cairo_surface_flush (surface);
	{
		g_autofree PpsRectangle *areas = NULL;
		guint n_areas = 0;
		gdouble gx = 0.0, gy = 0.0;

		g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
		                                                  page, &areas, &n_areas));

		/* The left column's first glyph must be cleared again... */
		g_assert_true (glyph_centre_after (areas, n_areas, LEFT_COLUMN_X1_MM, &gx, &gy));
		g_assert_cmpint ((pixels[(gint) gy * stride + (gint) gx] >> 24) & 0xff, ==, 0);

		/* ... while the right column's first glyph is covered now. */
		g_assert_true (glyph_centre_after (areas, n_areas, 115.0, &gx, &gy));
		g_assert_cmpint ((pixels[(gint) gy * stride + (gint) gx] >> 24) & 0xff, >=, 120);
	}

	cairo_surface_destroy (surface);

	/* Rotated pages: Papers refuses to start a selection there (pps-view.c:4512),
	 * so the overlay declines instead of drawing rectangles in the wrong place. */
	{
		g_autoptr (PpsRenderContext) rotated = NULL;
		cairo_surface_t *declined = NULL;

		rotated = pps_render_context_new (page, 90, 1.0, PPS_RENDER_ANNOTS_NONE);
		pps_selection_render_selection (PPS_SELECTION (document), rotated, &declined,
		                                &whole, NULL, PPS_SELECTION_STYLE_GLYPH,
		                                &text, &base);
		g_assert_null (declined);
	}

	g_object_unref (page);
}

/* A page whose PhysicalBox origin is "10 20": every rectangle the module
 * converts must be re-based on that origin (plan §4, D3). The fixture's text
 * starts 5 mm right of and 2 mm below the box corner, so a missing subtraction
 * shows up as a rectangle ~28 pt too far right and down. */
static void
test_origin_normalisation (const gchar *module_path,
                           const gchar *descriptor_path,
                           const gchar *mime_xml_path,
                           const gchar *fixture_path)
{
	PpsDocument *document = ofd_test_load_fixture (module_path, descriptor_path,
	                                               mime_xml_path, fixture_path);
	rofd_document_t *oracle = NULL;
	rofd_error_t *rerror = NULL;
	PpsPage *page = pps_document_get_page (document, 0);
	g_autofree gchar *text = NULL;
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	cairo_rectangle_int_t extents;
	cairo_region_t *region;
	GList *matches;

	g_assert_cmpint (rofd_document_open (fixture_path, NULL, &oracle, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_nonnull (page);

	text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page);
	g_assert_nonnull (text);
	g_assert_cmpstr (text, ==, oracle_page_text (oracle, 0));
	g_assert_cmpstr (text, ==, "Origin normalised text");

	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));
	g_assert_cmpint (n_areas, ==, g_utf8_strlen (text, -1));

	/* 15 mm and 22 mm absolute, 10 mm and 20 mm of origin: 5 mm and 2 mm. The box
	 * spans the run's baseline (at 6 mm: 22 mm minus the origin, plus the run's Y of
	 * 6 mm minus the object's Y of 20 mm), its top from the resolved face's ascent
	 * and its bottom from that face's descent. The fixture declares no font file, so
	 * the painter picks a system fallback and only the straddle is fixed. */
	g_assert_cmpfloat_with_epsilon (areas[0].x1, MM (5.0), 1.0);
	g_assert_cmpfloat (areas[0].y1, <, MM (6.0));
	g_assert_cmpfloat (areas[0].y2, >, MM (6.0));

	region = pps_document_text_get_text_mapping (PPS_DOCUMENT_TEXT (document), page);
	g_assert_nonnull (region);
	g_assert_false (cairo_region_is_empty (region));
	cairo_region_get_extents (region, &extents);
	g_assert_cmpint (extents.x, <, (gint) MM (20.0));
	g_assert_cmpint (extents.y, <, (gint) MM (20.0));
	cairo_region_destroy (region);

	/* A search hit sits on the same glyphs, in the same page space. */
	matches = pps_document_find_find_text (PPS_DOCUMENT_FIND (document), page, "Origin", 0);
	g_assert_cmpint (g_list_length (matches), ==, 1);
	{
		PpsFindRectangle *match = matches->data;

		g_assert_cmpfloat_with_epsilon (match->x1, MM (5.0), 1.0);
		g_assert_cmpfloat (match->y1, <, MM (6.0));
		g_assert_cmpfloat (match->y2, >, MM (6.0));
	}
	g_list_free_full (matches, (GDestroyNotify) pps_find_rectangle_free);

	/* And a selection over the whole page answers what the ABI answers for the
	 * same area, the origin going back in on the way to rofd. */
	{
		PpsRectangle whole = { .x2 = MM (210.0), .y2 = MM (297.0) };
		g_autofree gchar *ours = NULL;
		g_autofree gchar *theirs = NULL;

		ours = pps_selection_get_selected_text (PPS_SELECTION (document), page,
		                                        PPS_SELECTION_STYLE_GLYPH, &whole);
		theirs = oracle_selection_text (oracle, 0, ROFD_SELECTION_GLYPH, &whole);
		g_assert_cmpstr (ours, ==, theirs);
	}

	rofd_document_free (oracle);
	g_object_unref (page);
	g_object_unref (document);
}

/* Renders a page through the ABI (same options the module maps onto) so the test
 * can compare the geometry it gets with the pixels the renderer produced. */
static cairo_surface_t *
render_with_rofd_scale (rofd_document_t *document, gint index, gdouble scale)
{
	rofd_page_t *page = NULL;
	rofd_renderer_t *renderer = NULL;
	rofd_render_options_t options;
	rofd_render_report_t *report = NULL;
	rofd_error_t *rerror = NULL;
	cairo_surface_t *surface;
	cairo_t *cr;
	gint width = 0;
	gint height = 0;

	g_assert_cmpint (rofd_document_get_page (document, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (rofd_renderer_new (NULL, &renderer, &rerror), ==, ROFD_STATUS_OK);

	rofd_render_options_init (&options, sizeof (options));
	options.dpi = 72.0; /* one pixel per point times the scale below */
	options.scale = scale;
	options.rotation_degrees = 0;
	options.background_rgba = 0xFFFFFFFF;
	g_assert_cmpint (rofd_renderer_get_pixel_canvas_size (renderer, page, &options,
	                                                      &width, &height, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_null (rerror);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	g_assert_cmpint (rofd_renderer_render_page_cairo (renderer, page, cr, &options,
	                                                  &report, &rerror),
	                 ==, ROFD_STATUS_OK);
	cairo_destroy (cr);
	if (report != NULL)
		rofd_render_report_free (report);
	if (rerror != NULL)
		rofd_error_free (rerror);
	rofd_renderer_free (renderer);
	rofd_page_free (page);

	return surface;
}

/* One pixel per point, the space the page-point geometry uses. */
static cairo_surface_t *
render_with_rofd (rofd_document_t *document, gint index)
{
	return render_with_rofd_scale (document, index, 1.0);
}

/* Ink extents in pixels of one band, limited to a column of the page. FALSE when
 * the band is blank there. */
static gboolean
ink_span (cairo_surface_t *surface, gint x_from, gint x_to, gint y_from, gint y_to,
          gint *x_first, gint *x_last)
{
	guint32 *pixels;
	gint stride;
	gint x, y;
	gboolean found = FALSE;

	cairo_surface_flush (surface);
	pixels = (guint32 *) cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface) / 4;

	*x_first = G_MAXINT;
	*x_last = -1;

	for (y = y_from; y <= y_to; y++) {
		for (x = x_from; x <= x_to; x++) {
			guint32 pixel = pixels[y * stride + x];
			guint8 red = (pixel >> 16) & 0xff;
			guint8 green = (pixel >> 8) & 0xff;
			guint8 blue = pixel & 0xff;

			if (red < 160 && green < 160 && blue < 160) {
				*x_first = MIN (*x_first, x);
				*x_last = MAX (*x_last, x);
				found = TRUE;
			}
		}
	}

	return found;
}

/* Ink rows in a column band of the page, the space the geometry uses. FALSE when
 * the band is blank there. */
static gboolean
ink_rows (cairo_surface_t *surface, gint x_from, gint x_to, gint y_from, gint y_to,
          gint *y_first, gint *y_last)
{
	guint32 *pixels;
	gint stride;
	gint x, y;
	gboolean found = FALSE;

	cairo_surface_flush (surface);
	pixels = (guint32 *) cairo_image_surface_get_data (surface);
	stride = cairo_image_surface_get_stride (surface) / 4;

	*y_first = G_MAXINT;
	*y_last = -1;

	for (y = y_from; y <= y_to; y++) {
		for (x = x_from; x <= x_to; x++) {
			guint32 pixel = pixels[y * stride + x];
			guint8 red = (pixel >> 16) & 0xff;
			guint8 green = (pixel >> 8) & 0xff;
			guint8 blue = pixel & 0xff;

			if (red < 160 && green < 160 && blue < 160) {
				*y_first = MIN (*y_first, y);
				*y_last = MAX (*y_last, y);
				found = TRUE;
			}
		}
	}

	return found;
}

/* Classifies the linked ABI by comparing the first line's ABI *layout* span with the
 * ink the same library draws for it: TRUE when character geometry follows the font,
 * FALSE when it spreads the glyphs by the font size (a stock 0.4.x library). The
 * layout is read from rofd directly, not from the module under test, so a
 * module-side geometry regression cannot move the classifier into the lenient
 * branch (R9). The measured spans come back through the optional outputs, for the
 * report and for callers that have to relax their pixel checks. */
static gboolean
linked_abi_resolves_font_geometry (rofd_document_t *oracle,
                                   gdouble *layout_span_mm,
                                   gdouble *ink_span_mm)
{
	rofd_page_t *page = NULL;
	rofd_text_layout_t *layout = NULL;
	rofd_error_t *rerror = NULL;
	cairo_surface_t *surface;
	gint x_first = 0, x_last = 0;
	gdouble first_x = 0.0, last_x = 0.0, line_y = 0.0;
	gboolean have_first = FALSE;
	gboolean font_aware;
	size_t count = 0, i;
	gdouble layout_mm;
	gdouble ink_mm;

	g_assert_cmpint (rofd_document_get_page (oracle, 0, &page, &rerror), ==,
	                 ROFD_STATUS_OK);
	g_assert_cmpint (rofd_page_get_text_layout (page, &layout, &rerror), ==,
	                 ROFD_STATUS_OK);
	g_assert_cmpint (rofd_text_layout_get_count (layout, &count, &rerror), ==,
	                 ROFD_STATUS_OK);
	g_assert_cmpuint (count, >, 0);

	/* The first line's run: characters that carry a box, up to the first empty box
	 * (the synthesised object separator) or the first character on another line.
	 * The fixture's page box starts at (0, 0), so rofd's absolute millimetres are
	 * page coordinates. */
	for (i = 0; i < count; i++) {
		rofd_text_char_t character = { 0, };

		character.struct_size = sizeof (character);
		g_assert_cmpint (rofd_text_layout_get_char (layout, i, &character, &rerror),
		                 ==, ROFD_STATUS_OK);
		if (character.rect_mm.width_mm <= 0.0)
			break;
		if (!have_first) {
			first_x = character.rect_mm.x_mm;
			line_y = character.rect_mm.y_mm;
			have_first = TRUE;
		} else if (fabs (character.rect_mm.y_mm - line_y) > 1.0) {
			break;
		}
		last_x = character.rect_mm.x_mm + character.rect_mm.width_mm;
	}
	g_assert_true (have_first);
	layout_mm = last_x - first_x;

	surface = render_with_rofd (oracle, 0);
	g_assert_nonnull (surface);
	g_assert_true (ink_span (surface, (gint) MM (LEFT_COLUMN_X1_MM),
	                         (gint) MM (LEFT_COLUMN_X2_MM),
	                         (gint) MM (COLUMN_Y1_MM) + 1,
	                         (gint) MM (COLUMN_Y1_MM + 8.0), &x_first, &x_last));
	ink_mm = (x_last - x_first) / 72.0 * 25.4;
	cairo_surface_destroy (surface);

	rofd_text_layout_free (layout);
	rofd_page_free (page);
	if (rerror != NULL)
		rofd_error_free (rerror);

	font_aware = fabs (layout_mm - ink_mm) / ink_mm < 0.25;
	g_print ("first line: ABI layout %.1f mm, ink %.1f mm -> %s geometry\n",
	         layout_mm, ink_mm,
	         font_aware ? "font-resolved" : "font-size footprint");
	if (layout_span_mm != NULL)
		*layout_span_mm = layout_mm;
	if (ink_span_mm != NULL)
		*ink_span_mm = ink_mm;

	return font_aware;
}

/* The rectangles the Ctrl+F sidebar highlights must sit on the glyphs the renderer
 * drew. rofd's text geometry is only font-aware for runs without an explicit
 * `DeltaX` when the linked ABI resolves font advances (the branch our AUR package
 * carries); a stock 0.4.x library reports the font-size footprint instead, which
 * this fixture is wide enough to detect. Either way the geometry has to be one of
 * those two models — anything else is a defect here. */
static void
test_find_rectangles_cover_rendered_ink (const gchar *module_path,
                                         const gchar *descriptor_path,
                                         const gchar *mime_xml_path,
                                         const gchar *fixture_path)
{
	const gdouble tolerance_mm = 2.0;
	static const gchar *queries[] = { "invoice", "42", "EUR", "INV-42-ALPHA" };
	PpsDocument *document = ofd_test_load_fixture (module_path, descriptor_path,
	                                               mime_xml_path, fixture_path);
	rofd_document_t *oracle = NULL;
	rofd_error_t *rerror = NULL;
	cairo_surface_t *surface;
	PpsPage *page = pps_document_get_page (document, 0);
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	gdouble layout_span_mm = 0.0;
	gdouble ink_span_mm = 0.0;
	gint x_first = 0, x_last = 0;
	gboolean font_aware;
	guint i;

	g_assert_nonnull (page);
	g_assert_cmpint (rofd_document_open (fixture_path, NULL, &oracle, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_nonnull (oracle);

	font_aware = linked_abi_resolves_font_geometry (oracle, &layout_span_mm,
	                                                &ink_span_mm);

	/* Glyph boxes tile their run when the advance is resolved: a box that is
	 * wider than its glyph widens every match rectangle by the overlap. A library
	 * that resolves advances but still floors the box at the font size (our
	 * librofd-ffi below 0.4.3-4) is reported instead of failing silently. */
	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));
	{
		guint i;

		for (i = 0; i + 1 < n_areas; i++) {
			if (areas[i].x2 <= areas[i].x1 || areas[i + 1].x2 <= areas[i + 1].x1)
				continue; /* a separator or the gap between two runs */

			g_assert_cmpfloat (areas[i + 1].x1, >=, areas[i].x1 - MM (tolerance_mm));
			if (areas[i].x2 > areas[i + 1].x1 + MM (0.5)) {
				g_error ("glyph boxes overlap by %.2f mm at scalar %u: this library "
				         "resolves font advances but does not size the boxes by them. "
				         "Install librofd-ffi >= 0.4.3-4 (upstream PR #13).",
				         (areas[i].x2 - areas[i + 1].x1) / 72.0 * 25.4, i);
			}
		}
	}

	surface = render_with_rofd (oracle, 0);
	g_assert_nonnull (surface);

	if (!font_aware) {
		/* The linked ABI reports a conservative box per scalar instead of the
		 * font advance for runs without deltas, so its boxes cannot sit on the
		 * glyphs; the module must still publish a well-formed page space. */
		g_print ("linked rofd reports size-based text geometry: highlight "
		         "alignment is not checked (our AUR package carries the "
		         "font-advance fix)\n");
		g_assert_cmpfloat (areas[0].x1, <=, (gdouble) x_first + MM (tolerance_mm));
		g_assert_cmpfloat (areas[0].x1, >=, (gdouble) x_first - MM (tolerance_mm));
		g_assert_cmpfloat (layout_span_mm, >=, ink_span_mm);
		cairo_surface_destroy (surface);
		rofd_document_free (oracle);
		g_object_unref (page);
		g_object_unref (document);
		return;
	}

	for (i = 0; i < G_N_ELEMENTS (queries); i++) {
		GList *matches = pps_document_find_find_text (PPS_DOCUMENT_FIND (document), page,
		                                              queries[i], 0);
		GList *l;

		g_assert_nonnull (matches);

		for (l = matches; l != NULL; l = g_list_next (l)) {
			PpsFindRectangle *match = l->data;
			gboolean in_column;

			/* Only the left column's own hits have ink in this window. */
			in_column = match->x2 >= MM (LEFT_COLUMN_X1_MM) - tolerance_mm &&
			            match->x1 <= MM (LEFT_COLUMN_X2_MM) + tolerance_mm &&
			            match->y1 >= MM (COLUMN_Y1_MM) - tolerance_mm &&
			            match->y2 <= MM (COLUMN_Y2_MM) + tolerance_mm;
			if (!in_column)
				continue;

			g_assert_true (ink_span (surface, (gint) MM (LEFT_COLUMN_X1_MM),
			                         (gint) MM (LEFT_COLUMN_X2_MM),
			                         MAX (0, (gint) match->y1 - 2),
			                         (gint) match->y2 + 2,
			                         &x_first, &x_last));

			/* The match must overlap the drawn glyphs of its own line. */
			g_assert_cmpfloat (match->x1, <=, (gdouble) x_last + MM (tolerance_mm));
			g_assert_cmpfloat (match->x2, >=, (gdouble) x_first - MM (tolerance_mm));
			g_print ("\"%s\" at %.1f..%.1f pt vs ink %d..%d px\n",
			         queries[i], match->x1, match->x2, x_first, x_last);
		}

		g_list_free_full (matches, (GDestroyNotify) pps_find_rectangle_free);
	}

	cairo_surface_destroy (surface);
	rofd_document_free (oracle);
	g_object_unref (page);
	g_object_unref (document);
}

/* A selection region is what the view paints over the selected text and what it
 * hit-tests, so it has to cover the glyphs the renderer drew - including the part
 * of a glyph that reaches below its baseline, which a box sized by the font size
 * alone clips (CJK overshoot and Latin descenders). */
static void
test_selection_region_covers_rendered_ink (const gchar *module_path,
                                           const gchar *descriptor_path,
                                           const gchar *mime_xml_path,
                                           const gchar *fixture_path)
{
	PpsDocument *document = ofd_test_load_fixture (module_path, descriptor_path,
	                                               mime_xml_path, fixture_path);
	rofd_document_t *oracle = NULL;
	rofd_error_t *rerror = NULL;
	cairo_surface_t *surface;
	PpsPage *page = pps_document_get_page (document, 0);
	g_autoptr (PpsRenderContext) rc = NULL;
	/* The second line of the left column: "Payment received on Friday" carries a
	 * descender, while the first and third lines sit on their baseline. The band
	 * covers that line only (boundary 15 37 90 8, baseline at 40 mm). */
	PpsRectangle line = { .x1 = MM (LEFT_COLUMN_X1_MM),
	                      .y1 = MM (COLUMN_Y1_MM + 15.0),
	                      .x2 = MM (LEFT_COLUMN_X2_MM),
	                      .y2 = MM (COLUMN_Y1_MM + 26.0) };
	cairo_region_t *region;
	const gdouble scale = 4.0; /* zoom in, so a descender spans several pixels */
	gint x_first = 0, x_last = 0;
	gint y_first = 0, y_last = 0;
	gint column;
	gint x_from, x_to, y_from, y_to;
	cairo_rectangle_int_t extents;

	g_assert_nonnull (page);
	g_assert_cmpint (rofd_document_open (fixture_path, NULL, &oracle, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_nonnull (oracle);

	if (!linked_abi_resolves_font_geometry (oracle, NULL, NULL)) {
		/* A stock 0.4.x library sizes every box by the font size, which stops at
		 * the baseline, so its regions cannot cover the ink below it. */
		g_print ("linked rofd reports size-based text geometry: vertical "
		         "coverage is not checked (our AUR package carries the "
		         "fix)\n");
		rofd_document_free (oracle);
		g_object_unref (page);
		g_object_unref (document);
		return;
	}

	/* The region and the pixels share one space: 72 dpi times the render context's
	 * scale puts the same number of pixels on each page point. */
	surface = render_with_rofd_scale (oracle, 0, scale);
	g_assert_nonnull (surface);
	x_from = (gint) (MM (LEFT_COLUMN_X1_MM) * scale);
	x_to = (gint) (MM (LEFT_COLUMN_X2_MM) * scale);
	y_from = (gint) (MM (COLUMN_Y1_MM + 15.0) * scale);
	y_to = (gint) (MM (COLUMN_Y1_MM + 26.0) * scale);
	g_assert_true (ink_rows (surface, x_from, x_to, y_from, y_to, &y_first, &y_last));
	g_assert_true (ink_span (surface, x_from, x_to, y_first, y_last, &x_first, &x_last));
	column = (x_first + x_last) / 2;

	rc = pps_render_context_new (page, 0, scale, PPS_RENDER_ANNOTS_NONE);
	region = pps_selection_get_selection_region (PPS_SELECTION (document), rc,
	                                             PPS_SELECTION_STYLE_LINE, &line);
	g_assert_nonnull (region);
	g_assert_false (cairo_region_is_empty (region));

	cairo_region_get_extents (region, &extents);
	g_print ("second line: ink rows %d..%d, region rows %d..%d at column %d\n",
	         y_first, y_last, extents.y, extents.y + extents.height - 1, column);

	/* One band per selected logical line, covering the whole first line's width. */
	g_assert_cmpint (extents.x, <=, x_first);
	g_assert_cmpint (extents.x + extents.width, >=, x_last);
	g_assert_true (cairo_region_contains_point (region, column, y_first));

	/* The bottom of the ink is the regression: a box that stops at the baseline
	 * leaves the rows below it unhighlighted. */
	if (!cairo_region_contains_point (region, column, y_last)) {
		g_error ("selection region ends at row %d while the glyphs paint down to "
		         "%d: the linked rofd reports a character box that stops at the "
		         "baseline. Install librofd-ffi >= 0.4.3-6 (upstream PR #13).",
		         extents.y + extents.height - 1, y_last);
	}

	/* And it sticks out above and below the ink, the way the PDF backend's
	 * highlight does: the band comes from the face's metrics and then grows by an
	 * eighth of its height on both sides. */
	if (extents.y >= y_first || extents.y + extents.height <= y_last + 1) {
		g_error ("selection region rows %d..%d do not clear the ink rows %d..%d: "
		         "the linked rofd sizes a box by the font size, or by each glyph's "
		         "own ink, instead of the face's metrics plus the eighth-height "
		         "highlight margin. Install librofd-ffi >= 0.4.3-6 (upstream PR #13).",
		         extents.y, extents.y + extents.height - 1, y_first, y_last);
	}

	/* A paragraph over several text objects gives one region per line, and every
	 * region has the same height: Latin, CJK and punctuation line up instead of
	 * each hugging its own glyph. The fixture's left column is one size, so the
	 * bands have to match to the pixel. */
	{
		PpsRectangle paragraph = { .x1 = MM (LEFT_COLUMN_X1_MM),
		                           .y1 = MM (COLUMN_Y1_MM),
		                           .x2 = MM (LEFT_COLUMN_X2_MM),
		                           .y2 = MM (COLUMN_Y2_MM) };
		cairo_region_t *bands;
		gint count, i, first_height = 0;

		bands = pps_selection_get_selection_region (PPS_SELECTION (document), rc,
		                                            PPS_SELECTION_STYLE_LINE,
		                                            &paragraph);
		g_assert_nonnull (bands);
		count = cairo_region_num_rectangles (bands);
		g_assert_cmpint (count, >, 1);

		for (i = 0; i < count; i++) {
			cairo_rectangle_int_t box;

			cairo_region_get_rectangle (bands, i, &box);
			g_assert_cmpint (box.height, >, 0);
			if (i == 0) {
				first_height = box.height;
			} else if (box.height != first_height) {
				g_error ("paragraph regions differ in height (%d vs %d px at "
				         "%.0fx): the linked rofd sizes a character box by each "
				         "glyph's own ink instead of the face's metrics. Install "
				         "librofd-ffi >= 0.4.3-6 (upstream PR #13).",
				         box.height, first_height, scale);
			}
		}

		g_print ("paragraph: %d regions, each %d px tall at %.0fx\n",
		         count, first_height, scale);
		cairo_region_destroy (bands);
	}

	cairo_region_destroy (region);
	cairo_surface_destroy (surface);
	rofd_document_free (oracle);
	g_object_unref (page);
	g_object_unref (document);
}

/* A page with no text at all: empty text, no layout rectangles, an empty
 * mapping region and no find results, without any of them failing. */
static void
test_scan_page (PpsDocument *document)
{
	PpsPage *page = pps_document_get_page (document, 0);
	g_autofree gchar *text = NULL;
	g_autofree PpsRectangle *areas = NULL;
	guint n_areas = 0;
	cairo_region_t *region = NULL;
	g_autoptr (PpsRenderContext) rc = NULL;
	PpsRectangle whole = { 0, };
	GList *matches;

	g_assert_nonnull (page);

	text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page);
	g_assert_nonnull (text);
	g_assert_cmpstr (text, ==, "");

	g_assert_true (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                  page, &areas, &n_areas));
	g_assert_null (areas);
	g_assert_cmpint (n_areas, ==, 0);

	region = pps_document_text_get_text_mapping (PPS_DOCUMENT_TEXT (document), page);
	g_assert_nonnull (region);
	g_assert_true (cairo_region_is_empty (region));

	matches = pps_document_find_find_text (PPS_DOCUMENT_FIND (document), page, "scan", 0);
	g_assert_null (matches);

	/* A glyph selection over the whole page is empty but well formed. */
	oracle_page_size_pt (document, 0, &whole.x2, &whole.y2);
	rc = pps_render_context_new (page, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
	region = pps_selection_get_selection_region (PPS_SELECTION (document), rc,
	                                             PPS_SELECTION_STYLE_GLYPH, &whole);
	g_assert_nonnull (region);
	g_assert_true (cairo_region_is_empty (region));

	cairo_region_destroy (region);
	g_object_unref (page);
}

/* R13 (review 2026-10-15): the view keeps one selection surface per page and
 * rebuilds its texture from whatever that surface holds
 * (libview/pps-pixbuf-cache.c:1054-1068), so a refusal must not leave the
 * previous selection's pixels behind — it drops the surface instead. */
static void
test_render_selection_refusal_drops_surface (PpsDocument *document)
{
	PpsPage *page = pps_document_get_page (document, 0);
	PpsRectangle whole = { 0, };
	gdouble width_pt = 0.0, height_pt = 0.0;
	cairo_surface_t *surface = NULL;
	GdkRGBA text = { 0, };
	GdkRGBA base = { 0.2, 0.4, 0.6, 0.5 };

	g_assert_nonnull (page);
	oracle_page_size_pt (document, 0, &width_pt, &height_pt);
	whole.x2 = width_pt;
	whole.y2 = height_pt;

	{
		g_autoptr (PpsRenderContext) rc = pps_render_context_new (page, 0, 1.0,
		                                                          PPS_RENDER_ANNOTS_NONE);

		pps_selection_render_selection (PPS_SELECTION (document), rc, &surface, &whole,
		                                NULL, PPS_SELECTION_STYLE_LINE, &text, &base);
		g_assert_nonnull (surface);
	}

	/* The same reused surface, now with a rotated context: nothing drawn, and the
	 * stale pixels must be gone with it. */
	{
		g_autoptr (PpsRenderContext) rotated = pps_render_context_new (
			page, 90, 1.0, PPS_RENDER_ANNOTS_NONE);

		pps_selection_render_selection (PPS_SELECTION (document), rotated, &surface,
		                                &whole, NULL, PPS_SELECTION_STYLE_LINE,
		                                &text, &base);
		g_assert_null (surface);
	}

	/* And a caller error (no rectangle) is treated the same way. */
	{
		g_autoptr (PpsRenderContext) rc = pps_render_context_new (page, 0, 1.0,
		                                                          PPS_RENDER_ANNOTS_NONE);

		pps_selection_render_selection (PPS_SELECTION (document), rc, &surface, &whole,
		                                NULL, PPS_SELECTION_STYLE_LINE, &text, &base);
		g_assert_nonnull (surface);
		pps_selection_render_selection (PPS_SELECTION (document), rc, &surface, NULL,
		                                NULL, PPS_SELECTION_STYLE_LINE, &text, &base);
		g_assert_null (surface);
	}

	g_object_unref (page);
}

/* A page whose content cannot be parsed must not reach any vfunc as a crash:
 * Papers' page-data job calls these with whatever pps_document_get_page
 * returned, including NULL. */
static void
test_broken_page (const gchar *module_path,
                  const gchar *descriptor_path,
                  const gchar *mime_xml_path,
                  const gchar *fixture_path)
{
	PpsDocument *document = ofd_test_load_fixture (module_path, descriptor_path,
	                                               mime_xml_path, fixture_path);
	PpsPage *reachable = pps_document_get_page (document, 0);
	PpsPage *unparsable = pps_page_new (1);
	g_autofree gchar *text = NULL;
	PpsRectangle whole = { .x1 = 0.0, .y1 = 0.0, .x2 = 500.0, .y2 = 500.0 };
	g_autoptr (PpsRenderContext) rc = NULL;

	g_assert_cmpint (pps_document_get_n_pages (document), ==, 2);

	/* Page 0 is intact. */
	g_assert_nonnull (reachable);
	text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), reachable);
	g_assert_cmpstr (text, ==, "Page zero text");

	/* Page 1 cannot be opened at all, so Papers' own cache gets NULL. */
	g_assert_null (pps_document_get_page (document, 1));

	/* A PpsPage the caller built itself: the module fetches and fails to fetch
	 * the handle, which must look like "no text" rather than an abort. */
	g_assert_nonnull (unparsable);
	g_assert_null (pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), unparsable));
	g_assert_false (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                   unparsable, NULL, NULL));
	g_assert_cmpstr (pps_selection_get_selected_text (PPS_SELECTION (document), unparsable,
	                                                  PPS_SELECTION_STYLE_GLYPH, &whole),
	                 ==, "");
	g_assert_null (pps_document_find_find_text (PPS_DOCUMENT_FIND (document), unparsable,
	                                            "text", 0));
	g_assert_null (pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), unparsable));

	/* And the NULL page Papers hands over when get_page failed. */
	g_assert_null (pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), NULL));
	g_assert_false (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document),
	                                                   NULL, NULL, NULL));
	g_assert_null (pps_document_text_get_text_in_area (PPS_DOCUMENT_TEXT (document), NULL, &whole));
	g_assert_null (pps_document_text_get_text_mapping (PPS_DOCUMENT_TEXT (document), NULL));
	g_assert_null (pps_document_text_get_text_attrs (PPS_DOCUMENT_TEXT (document), NULL));
	g_assert_cmpstr (pps_selection_get_selected_text (PPS_SELECTION (document), NULL,
	                                                  PPS_SELECTION_STYLE_GLYPH, &whole),
	                 ==, "");
	g_assert_null (pps_document_find_find_text (PPS_DOCUMENT_FIND (document), NULL, "text", 0));
	g_assert_null (pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), NULL));

	/* The ABI has no glyph mask and no attribute data: NULL is the contract. */
	g_assert_null (pps_document_text_get_text_attrs (PPS_DOCUMENT_TEXT (document), reachable));

	/* Running through the cache is what the shell does on every page. A page
	 * whose handle cannot be fetched answers an empty region, not NULL: Papers
	 * dereferences the region without a check (R2). */
	rc = pps_render_context_new (unparsable, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
	{
		cairo_region_t *region = pps_selection_get_selection_region (
			PPS_SELECTION (document), rc, PPS_SELECTION_STYLE_GLYPH, &whole);

		g_assert_nonnull (region);
		g_assert_true (cairo_region_is_empty (region));
		cairo_region_destroy (region);
	}

	g_object_unref (unparsable);
	g_object_unref (reachable);
	g_object_unref (document);
}

/* R4: how many glyphs of one page the renderer could not resolve. rofd reports
 * that per glyph (ROFD_DIAGNOSTIC_MISSING_GLYPH), which is the signal that
 * distinguishes "no font installed" from "the font is there, the metrics differ":
 * a host without fonts still draws something (rofd substitutes a box), so ink
 * alone does not tell the two apart. */
static gint
count_missing_glyphs (rofd_document_t *oracle, gint index)
{
	rofd_page_t *page = NULL;
	rofd_renderer_t *renderer = NULL;
	rofd_render_options_t options;
	rofd_render_report_t *report = NULL;
	rofd_error_t *rerror = NULL;
	cairo_surface_t *surface;
	cairo_t *cr;
	gint width = 0, height = 0;
	gint missing = 0;
	size_t count = 0;
	size_t i;

	g_assert_cmpint (rofd_document_get_page (oracle, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (rofd_renderer_new (NULL, &renderer, &rerror), ==, ROFD_STATUS_OK);

	rofd_render_options_init (&options, sizeof (options));
	options.dpi = 72.0;
	options.scale = 1.0;
	options.rotation_degrees = 0;
	options.background_rgba = 0xFFFFFFFF;

	g_assert_cmpint (rofd_renderer_get_pixel_canvas_size (renderer, page, &options,
	                                                      &width, &height, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (width, >, 0);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
	cr = cairo_create (surface);
	g_assert_cmpint (rofd_renderer_render_page_cairo (renderer, page, cr, &options,
	                                                  &report, &rerror),
	                 ==, ROFD_STATUS_OK);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	if (report != NULL) {
		if (rofd_render_report_get_count (report, &count, &rerror) == ROFD_STATUS_OK) {
			for (i = 0; i < count; i++) {
				rofd_render_diagnostic_t diagnostic = { 0, };

				diagnostic.struct_size = sizeof (diagnostic);
				if (rofd_render_report_get_diagnostic (report, i, &diagnostic,
				                                       &rerror) != ROFD_STATUS_OK)
					break;
				if (diagnostic.kind == ROFD_DIAGNOSTIC_MISSING_GLYPH)
					missing++;
			}
		}
		rofd_render_report_free (report);
	}
	if (rerror != NULL)
		rofd_error_free (rerror);
	rofd_renderer_free (renderer);
	rofd_page_free (page);

	return missing;
}

/* R4: the fixtures carry text but declare no font file, so rofd resolves a system
 * font (through fontdb, not fontconfig — no environment variable hides a missing
 * font). On a host without one every geometry assertion below fails on a
 * millimetre comparison, which says nothing about the cause; this preflight
 * renders both fixture pages through the linked ABI and names the dependency
 * instead. The AUR recipe's check dependencies and the CI image install
 * noto-fonts and noto-fonts-cjk for exactly this. */
static void
require_fixture_fonts (rofd_document_t *oracle)
{
	gint missing;

	missing = count_missing_glyphs (oracle, 0);
	g_print ("fixture page 0 (Latin): %d missing glyphs\n", missing);
	if (missing > 0)
		g_error ("the fixture's Latin page has %d unresolvable glyphs: the linked rofd "
		         "resolved no usable font. Install the fonts the suite measures with "
		         "(Arch: noto-fonts; the AUR recipe's check() lists them) and re-run.",
		         missing);

	missing = count_missing_glyphs (oracle, 1);
	g_print ("fixture page 1 (CJK): %d missing glyphs\n", missing);
	if (missing > 0)
		g_error ("the fixture's CJK page has %d unresolvable glyphs: the linked rofd "
		         "resolved no usable CJK font. Install one (Arch: noto-fonts-cjk; the AUR "
		         "recipe's check() lists it) and re-run.", missing);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *module_path = NULL;
	g_autofree gchar *descriptor_path = NULL;
	g_autofree gchar *mime_xml_path = NULL;
	g_autofree gchar *text_fixture = NULL;
	g_autofree gchar *scan_fixture = NULL;
	g_autofree gchar *broken_fixture = NULL;
	g_autofree gchar *origin_fixture = NULL;
	PpsDocument *text_document;
	PpsDocument *scan_document;
	rofd_document_t *oracle = NULL;
	rofd_renderer_t *renderer = NULL;
	rofd_error_t *rerror = NULL;

	if (argc != 8) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <text.ofd> <scan.ofd> "
		            "<broken-page.ofd> <origin-text.ofd>\n", argv[0]);
		return 2;
	}

	module_path = g_canonicalize_filename (argv[1], NULL);
	descriptor_path = g_canonicalize_filename (argv[2], NULL);
	mime_xml_path = g_canonicalize_filename (argv[3], NULL);
	text_fixture = g_canonicalize_filename (argv[4], NULL);
	scan_fixture = g_canonicalize_filename (argv[5], NULL);
	broken_fixture = g_canonicalize_filename (argv[6], NULL);
	origin_fixture = g_canonicalize_filename (argv[7], NULL);

	text_document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                       text_fixture);
	scan_document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                       scan_fixture);

	/* rofd's own view of the same file, for the comparisons. */
	g_assert_cmpint (rofd_document_open (text_fixture, NULL, &oracle, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_nonnull (oracle);
	g_assert_cmpint (rofd_renderer_new (NULL, &renderer, &rerror), ==, ROFD_STATUS_OK);
	g_assert_nonnull (renderer);

	/* Before anything measures geometry: a host without fonts fails here, with the
	 * dependency named (R4). */
	require_fixture_fonts (oracle);

	test_text_matches_oracle (text_document, oracle);
	test_text_layout (text_document);
	test_text_in_area (text_document, oracle);
	test_text_mapping (text_document);
	test_find (text_document);
	test_selection_text (text_document, oracle);
	test_selection_region (text_document, oracle, renderer);
	test_selection_point_rectangles (text_document);
	test_selection_region_is_always_a_region (text_document);
	test_render_selection (text_document, oracle, renderer);
	test_render_selection_refusal_drops_surface (text_document);
	test_scan_page (scan_document);
	test_broken_page (module_path, descriptor_path, mime_xml_path, broken_fixture);
	test_origin_normalisation (module_path, descriptor_path, mime_xml_path, origin_fixture);
	test_find_rectangles_cover_rendered_ink (module_path, descriptor_path, mime_xml_path,
	                                         text_fixture);
	test_selection_region_covers_rendered_ink (module_path, descriptor_path,
	                                           mime_xml_path, text_fixture);

	rofd_renderer_free (renderer);
	rofd_document_free (oracle);
	g_object_unref (scan_document);
	g_object_unref (text_document);
	ofd_test_cleanup ();

	g_print ("phase-2 text, layout, find and selection checks passed\n");

	return 0;
}
