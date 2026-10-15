// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-render.h"

#include <math.h>

/* D9: diagnostics are informational and never fail a render. */
static void
ofd_render_log_diagnostic (const rofd_render_diagnostic_t *diagnostic)
{
	g_debug ("render diagnostic %u on object %" G_GUINT64_FORMAT ": %s",
	         (guint) diagnostic->kind, (guint64) diagnostic->object_id,
	         diagnostic->message != NULL ? diagnostic->message : "(no message)");
}

rofd_renderer_t *
ofd_renderer_new (void)
{
	const gchar *families_env = g_getenv ("PAPERS_OFD_FALLBACK_FAMILIES");
	rofd_renderer_options_t options;
	rofd_renderer_t *renderer = NULL;
	rofd_error_t *rerror = NULL;
	rofd_status_t status;

	if (families_env == NULL) {
		/* NULL options select rofd's built-in defaults, which already include a
		 * CJK family (D8, rofd renderer.rs DEFAULT_FALLBACK_FAMILIES). */
		status = rofd_renderer_new (NULL, &renderer, &rerror);
	} else {
		g_auto (GStrv) split = g_strsplit (families_env, ":", -1);
		g_autofree const char **families = NULL;
		gsize total = g_strv_length (split);
		gsize count = 0;
		gsize i;

		/* Compact the list into a separate array: g_strfreev must keep owning
		 * exactly one pointer per entry, and rofd requires a non-NULL array both
		 * for "these families" and for the explicit zero-count disable. */
		families = g_new0 (const char *, total + 1);
		for (i = 0; i < total; i++) {
			if (split[i][0] != '\0')
				families[count++] = split[i];
		}

		rofd_renderer_options_init (&options, sizeof (options));
		options.fallback_families = families;
		options.fallback_family_count = count;
		status = rofd_renderer_new (&options, &renderer, &rerror);

		if (status == ROFD_STATUS_OK && count == 0)
			g_debug ("PAPERS_OFD_FALLBACK_FAMILIES set but empty: fallback disabled");
	}

	if (status != ROFD_STATUS_OK) {
		g_debug ("renderer creation failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		renderer = NULL;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	return renderer;
}

void
ofd_render_options_init_print (rofd_render_options_t *options)
{
	rofd_render_options_init (options, sizeof (*options));
	options->dpi = 72.0;
	options->scale = 1.0;
	options->rotation_degrees = 0;
	options->background_rgba = 0xFFFFFFFF;
}

/* R3: rofd charges the worst case of a full-page raster against
 * `max_raster_bytes` — the finished ARGB32 target, its retained A8 clip mask and
 * a clipped ARGB32 temporary, i.e. 2*stride_argb32*h + stride_a8*h ≈ 9 B/px
 * (`cairo_renderer.rs:1246-1257`) — while its default budget is 256 MiB. Papers'
 * own allowance is smaller per pixel: the page cache defaults to 200 MiB and the
 * view derives the maximum zoom from it at 4 B/px (`libview/pps-view.c:5663-5675`,
 * `data/org.gnome.Papers.gschema.xml:25-29`), which is ≈10.2x on A4 — so a zoom
 * Papers itself admits used to exceed rofd's default and every render above ≈7x
 * came back NULL (A16's claim was wrong).
 *
 * The module therefore raises the budget to what the request actually needs, and
 * keeps this cap: the same conversion of Papers' default cache budget (200 MiB at
 * 4 B/px is 2.25x rofd's rate, i.e. 450 MiB). A request below the cap renders at
 * full size; a larger one from a caller Papers' clamp does not cover (a directly
 * set target size, a bigger `page-cache-size`) renders at the largest scale that
 * fits instead of failing, because a degraded page beats no page. */
#define OFD_RENDER_MAX_RASTER_BYTES (450 * G_GUINT64_CONSTANT (1024) * 1024)

/* R3: the byte count rofd will require for one canvas, the same expression as
 * `worst_case_surface_bytes` (stride via cairo, so the A8 row padding matches). */
static guint64
ofd_render_worst_case_bytes (gint width, gint height)
{
	gint argb_stride;
	gint mask_stride;

	if (width <= 0 || height <= 0)
		return G_MAXUINT64;

	argb_stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, width);
	mask_stride = cairo_format_stride_for_width (CAIRO_FORMAT_A8, width);
	if (argb_stride <= 0 || mask_stride <= 0)
		return G_MAXUINT64;

	return (guint64) argb_stride * (guint64) height * 2 +
	       (guint64) mask_stride * (guint64) height;
}

/* R3: an upper bound of the canvas rofd will size for the request. rofd ceils
 * `dimension_mm * (dpi / 25.4) * scale` per axis after swapping the page for a
 * quarter-turn rotation (`cairo_renderer.rs:1186-1199`); with dpi 72 that is
 * `points * scale`, plus one pixel of slack so the budget derived from it can
 * never fall below rofd's own requirement. */
static guint64
ofd_render_required_bytes (double width_pt, double height_pt, double scale)
{
	gint width = (gint) ceil (width_pt * scale) + 1;
	gint height = (gint) ceil (height_pt * scale) + 1;
	guint64 upright = ofd_render_worst_case_bytes (width, height);
	guint64 rotated = ofd_render_worst_case_bytes (height, width);

	return MAX (upright, rotated);
}

/* R14: rofd's renderer rejects any rotation but a quarter turn while Papers'
 * model accepts 0-360, so an oblique request is snapped to the nearest one (45
 * degrees ties upward) rather than handed over to fail. */
static guint32
ofd_render_snap_rotation (gint rotation)
{
	gint normalized = ((rotation % 360) + 360) % 360;

	return (guint32) (((normalized + 45) / 90) * 90 % 360);
}

/* Map a Papers render context onto rofd render options (plan §4), with the
 * raster budget R3 requires: the options are fitted to the module cap here, in
 * the one place both the page texture (ofd_render_page_surface) and every overlay
 * (ofd_render_page_canvas_size, used by the selection paths) take them from, so
 * they cannot disagree about the scale. */
void
ofd_render_options_from_context (PpsRenderContext *rc,
                                 double width_pt,
                                 double height_pt,
                                 rofd_render_options_t *options)
{
	gint scaled_width = 0;
	gint scaled_height = 0;
	double scale;
	guint64 default_budget;
	guint64 required;

	/* rc->scale can be 0.0 when a target size is set (selection paths, the
	 * thumbnailer), so always derive the scale from the computed size. */
	pps_render_context_compute_scaled_size (rc, width_pt, height_pt,
	                                        &scaled_width, &scaled_height);

	rofd_render_options_init (options, sizeof (*options));
	options->dpi = 72.0;
	options->rotation_degrees = ofd_render_snap_rotation (rc->rotation);
	options->background_rgba = 0xFFFFFFFF; /* opaque white, like the PDF backend */
	/* image_interpolation keeps its initialized default, and the budget only ever
	 * moves up from it: rofd charges decoded source images against the same field
	 * (`raster_budget = limits.max_decoded_image_bytes.min(max_raster_bytes)`,
	 * cairo_renderer.rs:1624), so a budget derived from a small canvas would stop
	 * the page's own images from decoding (the seal-bearing invoice fixture did). */
	default_budget = options->max_raster_bytes;

	scale = width_pt > 0.0 ? (double) scaled_width / width_pt : 1.0;
	if (!(scale > 0.0) || !isfinite (scale))
		scale = 1.0;

	required = ofd_render_required_bytes (width_pt, height_pt, scale);
	if (required > OFD_RENDER_MAX_RASTER_BYTES) {
		/* Beyond the cap the scale comes down until the canvas fits: the area is
		 * quadratic in the scale, so the byte ratio is its square root. */
		double fit = sqrt ((double) OFD_RENDER_MAX_RASTER_BYTES / (double) required);

		scale = MAX (scale * fit, DBL_MIN);
		required = ofd_render_required_bytes (width_pt, height_pt, scale);
		g_debug ("render request of %d x %d px exceeds the %" G_GUINT64_FORMAT
		         " byte cap; rendering at scale %.3f instead",
		         scaled_width, scaled_height,
		         (guint64) OFD_RENDER_MAX_RASTER_BYTES, scale);
	}

	options->scale = scale;
	/* At least rofd's initialized default (a zeroed budget fails every render,
	 * A8) and at least what this request's canvas needs. */
	options->max_raster_bytes = MAX (required, default_budget);
}

gboolean
ofd_render_page_onto (rofd_renderer_t *renderer,
                      rofd_page_t *page,
                      cairo_t *cr,
                      const rofd_render_options_t *options)
{
	rofd_render_report_t *report = NULL;
	rofd_error_t *rerror = NULL;
	rofd_status_t status;

	status = rofd_renderer_render_page_cairo (renderer, page, cr, options,
	                                          &report, &rerror);
	if (status != ROFD_STATUS_OK) {
		/* R15: a failed render is the cause of a user-visible blank page, unlike
		 * the object-level diagnostics below, which D9 keeps at g_debug. */
		g_warning ("render failed: %s",
		           rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		if (rerror != NULL)
			rofd_error_free (rerror);
		if (report != NULL)
			rofd_render_report_free (report);
		return FALSE;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	if (report != NULL) {
		rofd_error_t *diagnostic_error = NULL;
		size_t count = 0;
		size_t i;

		if (rofd_render_report_get_count (report, &count,
		                                  &diagnostic_error) == ROFD_STATUS_OK) {
			for (i = 0; i < count; i++) {
				rofd_render_diagnostic_t diagnostic = { 0, };

				diagnostic.struct_size = sizeof (diagnostic);
				if (rofd_render_report_get_diagnostic (report, i, &diagnostic,
				                                       &diagnostic_error) != ROFD_STATUS_OK)
					break;
				ofd_render_log_diagnostic (&diagnostic);
			}
		}
		if (diagnostic_error != NULL)
			rofd_error_free (diagnostic_error);
		rofd_render_report_free (report);
	}

	return TRUE;
}

gboolean
ofd_render_page_canvas_size (rofd_renderer_t *renderer,
                             rofd_page_t *page,
                             PpsRenderContext *rc,
                             double width_pt,
                             double height_pt,
                             gint *width,
                             gint *height)
{
	rofd_render_options_t options;
	rofd_error_t *rerror = NULL;
	int pixel_width = 0;
	int pixel_height = 0;

	*width = 0;
	*height = 0;

	if (renderer == NULL || page == NULL || width_pt <= 0.0 || height_pt <= 0.0)
		return FALSE;

	ofd_render_options_from_context (rc, width_pt, height_pt, &options);

	if (rofd_renderer_get_pixel_canvas_size (renderer, page, &options,
	                                         &pixel_width, &pixel_height,
	                                         &rerror) != ROFD_STATUS_OK) {
		g_debug ("canvas size query failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		if (rerror != NULL)
			rofd_error_free (rerror);
		return FALSE;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	if (pixel_width <= 0 || pixel_height <= 0)
		return FALSE;

	*width = pixel_width;
	*height = pixel_height;

	return TRUE;
}

cairo_surface_t *
ofd_render_page_surface (rofd_renderer_t *renderer,
                         rofd_page_t *page,
                         PpsRenderContext *rc,
                         double width_pt,
                         double height_pt)
{
	rofd_render_options_t options;
	rofd_error_t *rerror = NULL;
	cairo_surface_t *surface = NULL;
	int pixel_width = 0;
	int pixel_height = 0;

	ofd_render_options_from_context (rc, width_pt, height_pt, &options);

	if (rofd_renderer_get_pixel_canvas_size (renderer, page, &options,
	                                         &pixel_width, &pixel_height,
	                                         &rerror) != ROFD_STATUS_OK) {
		g_debug ("canvas size query failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		if (rerror != NULL)
			rofd_error_free (rerror);
		return NULL;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	if (pixel_width <= 0 || pixel_height <= 0)
		return NULL;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
	                                      pixel_width, pixel_height);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
		/* R15: the canvas query above bounds dimensions against i32 only, while
		 * cairo refuses anything above 32767 px (INVALID_SIZE), so this refusal is
		 * the one a request that large meets first — and nothing else would report
		 * it: the render call is never reached. */
		g_warning ("page surface of %d x %d px could not be created: %s",
		           pixel_width, pixel_height,
		           cairo_status_to_string (cairo_surface_status (surface)));
		cairo_surface_destroy (surface);
		return NULL;
	}

	{
		cairo_t *cr = cairo_create (surface);

		if (cairo_status (cr) != CAIRO_STATUS_SUCCESS ||
		    !ofd_render_page_onto (renderer, page, cr, &options)) {
			cairo_destroy (cr);
			cairo_surface_destroy (surface);
			return NULL;
		}
		cairo_destroy (cr);
	}

	return surface;
}
