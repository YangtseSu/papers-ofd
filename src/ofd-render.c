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

void
ofd_render_options_from_context (PpsRenderContext *rc,
                                 double width_pt,
                                 double height_pt,
                                 rofd_render_options_t *options)
{
	gint scaled_width = 0;
	gint scaled_height = 0;

	/* rc->scale can be 0.0 when a target size is set (selection paths, the
	 * thumbnailer), so always derive the scale from the computed size. */
	pps_render_context_compute_scaled_size (rc, width_pt, height_pt,
	                                        &scaled_width, &scaled_height);

	rofd_render_options_init (options, sizeof (*options));
	options->dpi = 72.0;
	options->scale = width_pt > 0.0 ? (double) scaled_width / width_pt : 1.0;
	options->rotation_degrees = (uint32_t) (rc->rotation % 360 + 360) % 360;
	options->background_rgba = 0xFFFFFFFF; /* opaque white, like the PDF backend */
	/* image_interpolation and max_raster_bytes keep their initialized defaults:
	 * a zeroed max_raster_bytes fails every render (A8). */
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
		g_debug ("render failed: %s",
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
