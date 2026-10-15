// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-config.h"

#include "ofd-text.h"

#include "ofd-page.h"
#include "ofd-render.h"
#include "ofd-units.h"

#include <string.h>

/* Papers' search context materialises one PpsFindRectangle and one
 * PpsSearchResult per match and has no cap of its own, so this bounds one
 * page's contribution to a find job (plan §3). rofd additionally clamps the
 * same query to the page's own resource limit. */
#define OFD_FIND_MAX_RESULTS_PER_PAGE 4096

/* The page's canonical text (the same bytes the layout offsets index). NULL
 * when rofd cannot produce it, which the callers report as "no text". */
static gchar *
ofd_text_page_text (const rofd_page_t *rofd_page)
{
	rofd_string_t *string = NULL;
	rofd_error_t *rerror = NULL;
	gchar *text = NULL;

	if (rofd_page_get_text (rofd_page, &string, &rerror) != ROFD_STATUS_OK) {
		g_debug ("text extraction failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
	} else {
		const gchar *data = rofd_string_get_data (string);

		text = g_strdup (data != NULL ? data : "");
	}

	if (rerror != NULL)
		rofd_error_free (rerror);
	if (string != NULL)
		rofd_string_free (string);

	return text;
}

/* Papers selection styles map 1:1 onto the ABI's (plan §3). */
static gboolean
ofd_text_selection_style (PpsSelectionStyle style, uint32_t *rofd_style)
{
	switch (style) {
	case PPS_SELECTION_STYLE_GLYPH:
		*rofd_style = ROFD_SELECTION_GLYPH;
		return TRUE;
	case PPS_SELECTION_STYLE_WORD:
		*rofd_style = ROFD_SELECTION_WORD;
		return TRUE;
	case PPS_SELECTION_STYLE_LINE:
		*rofd_style = ROFD_SELECTION_LINE;
		return TRUE;
	default:
		return FALSE;
	}
}

/* A query degenerate on either axis is a *point*, which is what Papers hands
 * over for a double or triple click (libview/pps-view.c:4235-4249) and for a
 * straight horizontal drag or a scroll increment (:920-931). rofd's hit test is
 * a strict positive-area overlap (rofd-core/src/semantic.rs:504-507), so a
 * zero-area rectangle selects nothing at all; widening it to a hair box asks the
 * question the caller means: which glyph, word or line is at this point. The box
 * is far smaller than any glyph box (0.02 mm ≈ 0.06 pt), and it is centred on the
 * point so a hit inside a glyph box stays inside it. */
#define OFD_SELECTION_POINT_EPSILON_MM 0.02

/* Selection points arrive in page space; the ABI wants absolute physical-page
 * millimetres, so the box origin goes back in (D3). */
static rofd_text_selection_t *
ofd_text_select_mm (const OfdPageRef *page_ref,
                    PpsSelectionStyle style,
                    const rofd_rect_t *area_mm)
{
	rofd_text_selection_t *selection = NULL;
	rofd_error_t *rerror = NULL;
	rofd_rect_t query = *area_mm;
	uint32_t rofd_style;

	if (!ofd_text_selection_style (style, &rofd_style))
		return NULL;

	if (query.width_mm <= 0.0) {
		query.x_mm -= OFD_SELECTION_POINT_EPSILON_MM / 2;
		query.width_mm = OFD_SELECTION_POINT_EPSILON_MM;
	}
	if (query.height_mm <= 0.0) {
		query.y_mm -= OFD_SELECTION_POINT_EPSILON_MM / 2;
		query.height_mm = OFD_SELECTION_POINT_EPSILON_MM;
	}

	if (rofd_page_get_selected_text (page_ref->rofd_page, rofd_style, &query,
	                                 &selection, &rerror) != ROFD_STATUS_OK) {
		g_debug ("text selection failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		selection = NULL;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	return selection;
}

static rofd_text_selection_t *
ofd_text_select (const OfdPageRef *page_ref,
                 PpsSelectionStyle style,
                 const PpsRectangle *points)
{
	rofd_rect_t area_mm;

	ofd_rect_to_mm (points, &page_ref->box_mm, &area_mm);

	return ofd_text_select_mm (page_ref, style, &area_mm);
}

/* Selection rectangles as a region in the target pixel space. With
 * (scale_x, scale_y) == (1.0, 1.0) the result holds integer page-point
 * rectangles, which is what Papers hit-tests (the I-beam cursor and the
 * page-scroll-by-line step work in document points). */
static cairo_region_t *
ofd_text_selection_region (const OfdPageRef *page_ref,
                           const rofd_text_selection_t *selection,
                           double scale_x,
                           double scale_y)
{
	cairo_region_t *region = cairo_region_create ();
	rofd_error_t *rerror = NULL;
	size_t count = 0;
	size_t i;

	if (rofd_text_selection_get_region_count (selection, &count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("selection region count failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		if (rerror != NULL)
			rofd_error_free (rerror);
		return region;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	for (i = 0; i < count; i++) {
		rofd_rect_t region_mm = { 0, };
		PpsRectangle rect_pt;
		cairo_rectangle_int_t pixel_rect;

		if (rofd_text_selection_get_region (selection, i, &region_mm, &rerror) != ROFD_STATUS_OK) {
			g_debug ("selection region %zu failed: %s", i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			break;
		}
		if (rerror != NULL) {
			rofd_error_free (rerror);
			rerror = NULL;
		}

		ofd_rect_from_mm (&region_mm, &page_ref->box_mm, &rect_pt);
		ofd_pixel_rect_from_points (&rect_pt, scale_x, scale_y, &pixel_rect);
		cairo_region_union_rectangle (region, &pixel_rect);
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	return region;
}

/* Device scales for overlays that must line up with the page texture: rofd's
 * canvas size for the same render context over the page size in points. */
static gboolean
ofd_text_pixel_scales (const OfdPageRef *page_ref,
                       PpsRenderContext *rc,
                       gint *canvas_width,
                       gint *canvas_height,
                       double *scale_x,
                       double *scale_y)
{
	rofd_renderer_t *renderer = ofd_document_borrow_renderer_locked (page_ref->document);
	double width_pt = ofd_page_ref_width_pt (page_ref);
	double height_pt = ofd_page_ref_height_pt (page_ref);

	if (width_pt <= 0.0 || height_pt <= 0.0)
		return FALSE;

	if (!ofd_render_page_canvas_size (renderer, page_ref->rofd_page, rc,
	                                  width_pt, height_pt,
	                                  canvas_width, canvas_height))
		return FALSE;

	*scale_x = *canvas_width / width_pt;
	*scale_y = *canvas_height / height_pt;

	return TRUE;
}

/* Papers disables selection while the page is rotated ("Selection in rotated
 * documents has never worked", libview/pps-view.c:4512), so the selection
 * overlay maps unrotated page points onto the canvas; a rotated request would
 * place rectangles wrong, and refusing is better than drawing them there. */
static gboolean
ofd_text_context_unrotated (PpsRenderContext *rc)
{
	if (rc == NULL)
		return FALSE;

	return ((rc->rotation % 360) + 360) % 360 == 0;
}

gchar *
ofd_text_get_text (PpsDocumentText *document_text, PpsPage *page)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (document_text));
	OfdPageRef page_ref;
	gchar *text;

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return NULL;
	}

	text = ofd_text_page_text (page_ref.rofd_page);
	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return text;
}

/* One rectangle per Unicode scalar of the canonical text, in points with the
 * page-box origin removed. The layout byte offsets index exactly the bytes
 * get_text returns, so a scalar-count mismatch means the two rofd queries
 * disagree and no rectangle array can be published (plan §3). */
gboolean
ofd_text_get_text_layout (PpsDocumentText *document_text,
                          PpsPage *page,
                          PpsRectangle **areas,
                          guint *n_areas)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (document_text));
	OfdPageRef page_ref;
	rofd_text_layout_t *layout = NULL;
	rofd_error_t *rerror = NULL;
	g_autofree gchar *text = NULL;
	PpsRectangle *rects = NULL;
	glong scalars;
	size_t count = 0;
	size_t i;
	gboolean ok = FALSE;

	if (areas == NULL || n_areas == NULL)
		return FALSE;

	*areas = NULL;
	*n_areas = 0;

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return FALSE;
	}

	text = ofd_text_page_text (page_ref.rofd_page);
	if (text == NULL)
		goto out;

	if (rofd_page_get_text_layout (page_ref.rofd_page, &layout, &rerror) != ROFD_STATUS_OK) {
		g_debug ("text layout failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}

	if (rofd_text_layout_get_count (layout, &count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("text layout count failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}

	scalars = g_utf8_strlen (text, -1);
	if (count != (size_t) scalars) {
		g_debug ("text layout has %zu entries for %" G_GINT64_FORMAT
		         " scalars; ignoring the page layout", count, (gint64) scalars);
		goto out;
	}

	rects = count > 0 ? g_new0 (PpsRectangle, count) : NULL;

	for (i = 0; i < count; i++) {
		rofd_text_char_t character = { 0, };

		character.struct_size = sizeof (character);
		if (rofd_text_layout_get_char (layout, i, &character, &rerror) != ROFD_STATUS_OK) {
			g_debug ("text layout entry %zu failed: %s", i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			goto out;
		}
		if (rerror != NULL) {
			rofd_error_free (rerror);
			rerror = NULL;
		}

		ofd_rect_from_mm (&character.rect_mm, &page_ref.box_mm, &rects[i]);
	}

	*areas = g_steal_pointer (&rects);
	*n_areas = (guint) count;
	ok = TRUE;

out:
	g_clear_pointer (&rects, g_free);
	if (rerror != NULL)
		rofd_error_free (rerror);
	if (layout != NULL)
		rofd_text_layout_free (layout);
	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return ok;
}

gchar *
ofd_text_get_text_in_area (PpsDocumentText *document_text,
                           PpsPage *page,
                           PpsRectangle *area)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (document_text));
	OfdPageRef page_ref;
	rofd_string_t *string = NULL;
	rofd_error_t *rerror = NULL;
	rofd_rect_t area_mm;
	gchar *text = NULL;

	if (area == NULL)
		return NULL;

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return NULL;
	}

	ofd_rect_to_mm (area, &page_ref.box_mm, &area_mm);

	if (rofd_page_get_text_for_area (page_ref.rofd_page, &area_mm, &string,
	                                 &rerror) != ROFD_STATUS_OK) {
		g_debug ("text in area failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
	} else {
		const gchar *data = rofd_string_get_data (string);

		text = g_strdup (data != NULL ? data : "");
	}

	if (rerror != NULL)
		rofd_error_free (rerror);
	if (string != NULL)
		rofd_string_free (string);
	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return text;
}

cairo_region_t *
ofd_text_get_text_mapping (PpsDocumentText *document_text, PpsPage *page)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (document_text));
	OfdPageRef page_ref;
	rofd_text_selection_t *selection;
	cairo_region_t *region = NULL;

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return NULL;
	}

	/* Glyph selection over the whole box: the region Papers hit-tests for the
	 * I-beam cursor, the page-scroll-by-line step and the debug borders. */
	selection = ofd_text_select_mm (&page_ref, PPS_SELECTION_STYLE_GLYPH,
	                                &page_ref.box_mm);

	if (selection != NULL) {
		region = ofd_text_selection_region (&page_ref, selection, 1.0, 1.0);
		rofd_text_selection_free (selection);
	}

	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return region;
}

PangoAttrList *
ofd_text_get_text_attrs (PpsDocumentText *document_text, PpsPage *page)
{
	/* No attribute data in the ABI (plan §3); NULL means "no attributes" and
	 * every caller checks for it. */
	(void) document_text;
	(void) page;

	return NULL;
}

/* The view keeps one selection surface per page and rebuilds its texture from
 * whatever that surface holds (libview/pps-pixbuf-cache.c:1054-1068), so a
 * refusal has to drop the previous selection's pixels instead of leaving them to
 * be drawn as a highlight over a page the selection does not belong to (R13). */
static void
ofd_selection_drop_surface (cairo_surface_t **surface)
{
	if (surface == NULL || *surface == NULL)
		return;

	cairo_surface_destroy (*surface);
	*surface = NULL;
}

void
ofd_selection_render_selection (PpsSelection *selection,
                                PpsRenderContext *rc,
                                cairo_surface_t **surface,
                                PpsRectangle *points,
                                PpsRectangle *old_points,
                                PpsSelectionStyle style,
                                GdkRGBA *text,
                                GdkRGBA *base)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (selection));
	OfdPageRef page_ref;
	rofd_text_selection_t *rofd_selection;
	cairo_region_t *region;
	cairo_surface_t *target;
	cairo_t *cr;
	gint canvas_width = 0;
	gint canvas_height = 0;
	double scale_x = 1.0;
	double scale_y = 1.0;
	gint n_rects;
	gint i;

	/* The ABI has no glyph mask, so the pass that recolours the selected text
	 * cannot be reproduced: the surface carries the base colour only (plan §3,
	 * §8). The old points are not needed because the surface is cleared first. */
	(void) old_points;
	(void) text;

	if (surface == NULL)
		return;
	if (points == NULL || base == NULL) {
		ofd_selection_drop_surface (surface);
		return;
	}

	if (!ofd_text_context_unrotated (rc) || rc->page == NULL) {
		ofd_selection_drop_surface (surface);
		return;
	}

	/* The rofd queries borrow the document handle and the geometry, so they run
	 * under the read lock (R7); the surface allocation and fill below need only
	 * the canvas size and the region that came out of them. */
	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, rc->page, &page_ref)) {
		ofd_document_reader_unlock (self);
		ofd_selection_drop_surface (surface);
		return;
	}

	if (!ofd_text_pixel_scales (&page_ref, rc, &canvas_width, &canvas_height,
	                           &scale_x, &scale_y)) {
		ofd_page_ref_release (&page_ref);
		ofd_document_reader_unlock (self);
		ofd_selection_drop_surface (surface);
		return;
	}

	rofd_selection = ofd_text_select (&page_ref, style, points);
	if (rofd_selection == NULL) {
		ofd_page_ref_release (&page_ref);
		ofd_document_reader_unlock (self);
		ofd_selection_drop_surface (surface);
		return;
	}

	region = ofd_text_selection_region (&page_ref, rofd_selection, scale_x, scale_y);
	rofd_text_selection_free (rofd_selection);
	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	if (*surface == NULL) {
		*surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
		                                       canvas_width, canvas_height);
		if (cairo_surface_status (*surface) != CAIRO_STATUS_SUCCESS) {
			cairo_surface_destroy (*surface);
			*surface = NULL;
			cairo_region_destroy (region);
			return;
		}
	}

	target = *surface;
	/* The view reuses the surface across selection updates, so it starts empty
	 * every time, like the PDF backend's memset. */
	cairo_surface_set_device_offset (target, 0, 0);

	cr = cairo_create (target);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy (cr);
		cairo_region_destroy (region);
		return;
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr, base->red, base->green, base->blue, base->alpha);

	n_rects = cairo_region_num_rectangles (region);
	for (i = 0; i < n_rects; i++) {
		cairo_rectangle_int_t box;

		cairo_region_get_rectangle (region, i, &box);
		cairo_rectangle (cr, box.x, box.y, box.width, box.height);
	}
	cairo_fill (cr);
	cairo_destroy (cr);
	cairo_region_destroy (region);
}

gchar *
ofd_selection_get_selected_text (PpsSelection *selection,
                                 PpsPage *page,
                                 PpsSelectionStyle style,
                                 PpsRectangle *points)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (selection));
	OfdPageRef page_ref;
	rofd_text_selection_t *rofd_selection;
	const gchar *data;
	gchar *text = NULL;

	/* Dispatched without a NULL check upstream (A9), so never NULL: an empty
	 * string means "nothing selected". */
	if (points == NULL)
		return g_strdup ("");

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return g_strdup ("");
	}

	rofd_selection = ofd_text_select (&page_ref, style, points);
	if (rofd_selection != NULL) {
		data = rofd_text_selection_get_text (rofd_selection);
		text = g_strdup (data != NULL ? data : "");
		rofd_text_selection_free (rofd_selection);
	}

	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return text != NULL ? text : g_strdup ("");
}

cairo_region_t *
ofd_selection_get_selection_region (PpsSelection *selection,
                                    PpsRenderContext *rc,
                                    PpsSelectionStyle style,
                                    PpsRectangle *points)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (selection));
	OfdPageRef page_ref;
	rofd_text_selection_t *rofd_selection;
	cairo_region_t *region;
	gint canvas_width = 0;
	gint canvas_height = 0;
	double scale_x = 1.0;
	double scale_y = 1.0;

	/* Every refusal answers an empty region rather than NULL: Papers
	 * dereferences the result without a check (`compute_scroll_increment` calls
	 * cairo_region_num_rectangles on it, libview/pps-view.c:925-931, with a
	 * context built from the model rotation), and cairo's region accessors
	 * dereference unconditionally. The PDF backend keeps the same contract
	 * (pps-poppler.c:2007-2037): a region is always a region. */
	if (points == NULL || !ofd_text_context_unrotated (rc) || rc->page == NULL)
		return cairo_region_create ();

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, rc->page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return cairo_region_create ();
	}

	if (!ofd_text_pixel_scales (&page_ref, rc, &canvas_width, &canvas_height,
	                           &scale_x, &scale_y)) {
		ofd_page_ref_release (&page_ref);
		ofd_document_reader_unlock (self);
		return cairo_region_create ();
	}

	rofd_selection = ofd_text_select (&page_ref, style, points);
	if (rofd_selection == NULL) {
		ofd_page_ref_release (&page_ref);
		ofd_document_reader_unlock (self);
		return cairo_region_create ();
	}

	region = ofd_text_selection_region (&page_ref, rofd_selection, scale_x, scale_y);
	rofd_text_selection_free (rofd_selection);
	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return region;
}

PpsFindOptions
ofd_find_get_supported_options (PpsDocumentFind *document_find)
{
	(void) document_find;

	return (PpsFindOptions) (PPS_FIND_CASE_SENSITIVE | PPS_FIND_WHOLE_WORDS_ONLY);
}

GList *
ofd_find_text (PpsDocumentFind *document_find,
               PpsPage *page,
               const gchar *text,
               PpsFindOptions options)
{
	OfdDocument *self = OFD_DOCUMENT (PPS_DOCUMENT (document_find));
	OfdPageRef page_ref;
	rofd_text_search_t *search = NULL;
	rofd_error_t *rerror = NULL;
	rofd_find_options_t find_options;
	GList *matches = NULL;
	size_t count = 0;
	size_t i;

	/* Dispatched without a NULL check upstream (A9); an empty result list is
	 * the "not found" answer. */
	if (text == NULL || text[0] == '\0')
		return NULL;

	ofd_document_reader_lock (self);

	if (!ofd_page_ref_acquire (self, page, &page_ref)) {
		ofd_document_reader_unlock (self);
		return NULL;
	}

	rofd_find_options_init (&find_options, sizeof (find_options));
	if (options & PPS_FIND_CASE_SENSITIVE)
		find_options.flags |= ROFD_FIND_CASE_SENSITIVE;
	if (options & PPS_FIND_WHOLE_WORDS_ONLY)
		find_options.flags |= ROFD_FIND_WHOLE_WORDS;
	find_options.max_results = OFD_FIND_MAX_RESULTS_PER_PAGE;

	if (rofd_page_find_text_with_options (page_ref.rofd_page, text, &find_options,
	                                      &search, &rerror) != ROFD_STATUS_OK) {
		g_debug ("find \"%s\" failed: %s", text,
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}

	if (rofd_text_search_get_count (search, &count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("find result count failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}

	/* The ABI bounds the result count by the requested max_results. */
	for (i = 0; i < count; i++) {
		rofd_text_match_t match = { 0, };
		PpsFindRectangle *rectangle;
		PpsRectangle rect_pt;

		match.struct_size = sizeof (match);
		if (rofd_text_search_get_match (search, i, &match, &rerror) != ROFD_STATUS_OK) {
			g_debug ("find match %zu failed: %s", i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			break;
		}
		if (rerror != NULL) {
			rofd_error_free (rerror);
			rerror = NULL;
		}

		ofd_rect_from_mm (&match.rect_mm, &page_ref.box_mm, &rect_pt);

		rectangle = pps_find_rectangle_new ();
		rectangle->x1 = rect_pt.x1;
		rectangle->y1 = rect_pt.y1;
		rectangle->x2 = rect_pt.x2;
		rectangle->y2 = rect_pt.y2;
		/* rofd reports one rectangle per match, so a match that spans lines
		 * never reports "continued on the next line" (plan §8). */
		rectangle->next_line = FALSE;
		rectangle->after_hyphen = FALSE;

		matches = g_list_prepend (matches, rectangle);
	}

out:
	if (rerror != NULL)
		rofd_error_free (rerror);
	if (search != NULL)
		rofd_text_search_free (search);
	ofd_page_ref_release (&page_ref);
	ofd_document_reader_unlock (self);

	return g_list_reverse (matches);
}
