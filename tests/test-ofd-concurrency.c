// SPDX-License-Identifier: GPL-2.0-or-later

/* R7: one document instance used from several threads while another thread
 * reloads it. Papers' job pool is one thread today
 * (libview/pps-job-scheduler.c:93-100) and a reload normally builds a *new*
 * document, so the window is narrow — the only reachable reuse is the failed-load
 * retry (libview/pps-jobs.c:832-852). It is narrow, not empty: this module used to
 * replace the rofd handle, the renderer and the page-geometry array in `load`
 * without excluding readers, so such a reload could free a handle another thread
 * was inside (rofd.h: "no handle is freed while another call is using it") — the
 * reference backend takes a writer lock for exactly this (pps-poppler.c:288-305).
 *
 * The program drives both halves and is meant to be run under a sanitizer as well
 * as in the ordinary suite:
 *
 *   meson test -C _build ofd-concurrency                     # short form
 *   LD_LIBRARY_PATH=... _build-asan/tests/test-ofd-concurrency \
 *     ... text-2pages.ofd 8 2000 200                         # the long form
 *
 * Workers render pages, walk the text layout, search, fetch the outline model and
 * read the document info (every path the review found unlocked); the main thread
 * alternates `pps_document_load` (URI) and `pps_document_load_fd` (descriptor) on
 * the same instance. Nothing is asserted about a single call's result: a reload
 * legitimately changes what a page answers, and `get_page` may fail while a
 * reload is half-done once the locking is in place. What must hold is that no
 * thread crashes and no call reads freed memory. */

#include "ofd-test-utils.h"

#include <papers-document.h>

#include <cairo.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEFAULT_RELOADS 25
#define DEFAULT_WORKERS 4
#define DEFAULT_ITERATIONS 100

typedef struct {
	PpsDocument *document;
	gint n_pages;
	gint id;
	gint iterations;
	gint operations;
} Worker;

static gint
dup_cloexec (gint fd)
{
	gint duplicate = fcntl (fd, F_DUPFD_CLOEXEC, 3);

	g_assert_cmpint (duplicate, >=, 0);

	return duplicate;
}

/* One request from each family that borrows the document handle or the geometry
 * array, rotating so a given worker never blocks on the same call shape twice in
 * a row. */
static void
worker_request (Worker *worker, gint turn)
{
	PpsDocument *document = worker->document;
	PpsPage *page = pps_document_get_page (document, turn % worker->n_pages);

	if (page == NULL)
		return;

	switch (turn % 5) {
	case 0:
		g_free (pps_document_text_get_text (PPS_DOCUMENT_TEXT (document), page));
		break;
	case 1: {
		PpsRectangle *areas = NULL;
		guint n_areas = 0;

		if (pps_document_text_get_text_layout (PPS_DOCUMENT_TEXT (document), page,
		                                       &areas, &n_areas))
			g_free (areas);
		break;
	}
	case 2: {
		GList *matches =
			pps_document_find_find_text (PPS_DOCUMENT_FIND (document), page,
			                             "invoice", 0);

		g_list_free_full (matches, (GDestroyNotify) pps_find_rectangle_free);
		break;
	}
	case 3: {
		PpsRenderContext *rc =
			pps_render_context_new (page, 0, 1.0, PPS_RENDER_ANNOTS_NONE);
		cairo_surface_t *surface = pps_document_render (document, rc);

		if (surface != NULL)
			cairo_surface_destroy (surface);
		g_object_unref (rc);
		break;
	}
	default: {
		PpsDocumentInfo *info = pps_document_get_info (document);

		if (info != NULL)
			pps_document_info_free (info);
		break;
	}
	}

	/* The outline path is the longest borrow of the document handle: the model is
	 * rebuilt from an rofd snapshot node by node. */
	if (turn % 16 == 0) {
		GListModel *model = pps_document_links_get_links_model (
			PPS_DOCUMENT_LINKS (document));

		if (model != NULL)
			g_object_unref (model);
	}

	worker->operations++;

	g_object_unref (page);
}

static gpointer
worker_main (gpointer data)
{
	Worker *worker = data;
	gint turn = worker->id;
	gint i;

	/* A fixed budget rather than a stop flag: the main thread's reloads are
	 * bounded too, so the run cannot turn into a race between a reader that keeps
	 * re-acquiring the lock and a writer waiting for it. */
	for (i = 0; i < worker->iterations; i++) {
		worker_request (worker, turn);
		turn += worker->id + 1;
	}

	return NULL;
}

int
main (int argc, char **argv)
{
	g_autofree gchar *fixture = NULL;
	g_autofree gchar *uri = NULL;
	PpsDocument *document;
	GThread **threads;
	Worker *workers;
	gint reloads = DEFAULT_RELOADS;
	gint n_workers = DEFAULT_WORKERS;
	gint iterations = DEFAULT_ITERATIONS;
	gint fixture_fd;
	gint operations = 0;
	gint i;
	GError *error = NULL;

	if (argc < 5) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <text.ofd> "
		            "[workers] [iterations] [reloads]\n",
		            argv[0]);
		return 2;
	}
	if (argc >= 6)
		n_workers = CLAMP (atoi (argv[5]), 1, 32);
	if (argc >= 7)
		iterations = CLAMP (atoi (argv[6]), 1, 100000);
	if (argc >= 8)
		reloads = CLAMP (atoi (argv[7]), 1, 100000);

	fixture = g_canonicalize_filename (argv[4], NULL);
	uri = g_filename_to_uri (fixture, NULL, &error);
	g_assert_no_error (error);

	fixture_fd = open (fixture, O_RDONLY | O_CLOEXEC);
	g_assert_cmpint (fixture_fd, >=, 0);

	document = ofd_test_load_fixture (argv[1], argv[2], argv[3], fixture);
	g_assert_cmpint (pps_document_get_n_pages (document), >, 0);

	workers = g_new0 (Worker, n_workers);
	threads = g_new0 (GThread *, n_workers);
	for (i = 0; i < n_workers; i++) {
		workers[i].document = document;
		workers[i].n_pages = pps_document_get_n_pages (document);
		workers[i].id = i;
		workers[i].iterations = iterations;
		threads[i] = g_thread_new ("ofd-worker", worker_main, &workers[i]);
	}

	/* Both loading paths on the same instance: the URI path frees and rebuilds
	 * the handle (and the geometry array), the fd path additionally swaps the
	 * retained descriptor for the temp copy. The short pause keeps the readers
	 * from starving this writer for the whole run — the lock is reader-preferring,
	 * like the reference backend's. */
	for (i = 0; i < reloads; i++) {
		gboolean ok;

		if (i % 2 == 0)
			ok = pps_document_load (document, uri, &error);
		else
			ok = pps_document_load_fd (document, dup_cloexec (fixture_fd), &error);

		g_assert_true (ok);
		g_assert_no_error (error);

		g_usleep (500);
	}

	for (i = 0; i < n_workers; i++)
		g_thread_join (threads[i]);

	for (i = 0; i < n_workers; i++)
		operations += workers[i].operations;

	g_print ("%d reloads with %d workers x %d requests: %d concurrent requests, "
	         "no crash\n",
	         reloads, n_workers, iterations, operations);
	g_assert_cmpint (operations, >, 0);

	close (fixture_fd);
	g_free (threads);
	g_free (workers);
	g_object_unref (document);
	ofd_test_cleanup ();

	return 0;
}
