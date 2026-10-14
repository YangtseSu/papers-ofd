// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-1 open-latency measurement (plan §5, §8): wall-clock time from
 * pps_document_load to the first rendered page for a 300-page fixture, split
 * into load, setup_cache (the page-size walk the shell performs inside its load
 * job) and the first render.
 *
 * Both page-size paths ship (D10), so the comparison is a build switch:
 *   meson setup _build      ...            # rofd_page_size=auto   -> fast path
 *   meson setup _build-slow -Drofd_page_size=disabled            -> fallback
 *
 * This is a measurement tool, not a pass/fail test: it prints a line per run and
 * the minimum, and the numbers go into the README limitations. */

#include "ofd-test-utils.h"

#include <papers-document.h>

#include <glib/gstdio.h>

#define MAX_RUNS 16

int
main (int argc, char **argv)
{
	g_autofree gchar *fixture = NULL;
	g_autofree gchar *uri = NULL;
	gint runs = 3;
	gdouble best_total = G_MAXDOUBLE;
	gint run;
	GError *error = NULL;

	if (argc < 5) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <fixture.ofd> "
		            "[runs]\n",
		            argv[0]);
		return 2;
	}
	if (argc >= 6) {
		runs = CLAMP (atoi (argv[5]), 1, MAX_RUNS);
	}

	fixture = g_canonicalize_filename (argv[4], NULL);
	uri = g_filename_to_uri (fixture, NULL, &error);
	g_assert_no_error (error);

	ofd_test_stage_all (argv[1], argv[2], argv[3]);

	for (run = 0; run < runs; run++) {
		PpsDocument *document;
		PpsPage *page;
		PpsRenderContext *rc;
		cairo_surface_t *surface;
		gint64 start, after_load, after_info, after_cache, after_render;
		gdouble load_ms, info_ms, cache_ms, render_ms, total;
		PpsDocumentInfo *info;

		start = g_get_monotonic_time ();

		document = pps_document_factory_get_document (uri, &error);
		g_assert_no_error (error);
		g_assert_true (pps_document_load (document, uri, &error));
		g_assert_no_error (error);

		after_load = g_get_monotonic_time ();

		/* Info walks every page box before the shell's cache exists: the
		 * metadata-only query (D10) is what this phase isolates. */
		info = pps_document_get_info (document);
		g_assert_nonnull (info);
		pps_document_info_free (info);
		after_info = g_get_monotonic_time ();

		pps_document_setup_cache (document);
		after_cache = g_get_monotonic_time ();

		page = pps_document_get_page (document, 0);
		g_assert_nonnull (page);
		rc = pps_render_context_new (page, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
		surface = pps_document_render (document, rc);
		g_assert_nonnull (surface);
		after_render = g_get_monotonic_time ();

		load_ms = (after_load - start) / 1000.0;
		info_ms = (after_info - after_load) / 1000.0;
		cache_ms = (after_cache - after_info) / 1000.0;
		render_ms = (after_render - after_cache) / 1000.0;
		total = (after_render - start) / 1000.0;
		if (total < best_total)
			best_total = total;

		g_print ("run %d: %d pages  load %.1f ms  info walk %.1f ms  "
		         "setup_cache %.1f ms  first render %.1f ms  total %.1f ms\n",
		         run + 1, pps_document_get_n_pages (document),
		         load_ms, info_ms, cache_ms, render_ms, total);

		cairo_surface_destroy (surface);
		g_object_unref (rc);
		g_object_unref (page);
		g_object_unref (document);
	}

	g_print ("best total over %d runs: %.1f ms\n", runs, best_total);

	ofd_test_cleanup ();

	return 0;
}
