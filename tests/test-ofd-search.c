// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-2 acceptance, headless (plan §5): the rows the Ctrl+F sidebar shows.
 *
 * Papers builds each snippet from the match rectangle and the page's text
 * layout (pps-search-context.c: get_match_offset pairs the two, then
 * get_surrounding_text_markup wraps the surrounding words), so running the real
 * search context over the fixture proves the acceptance criterion "the snippet
 * is the sentence around the match" — including the CJK page and the
 * case-insensitive case — without a GUI.
 *
 * The context runs the real find job through the job scheduler, so this also
 * exercises our PpsDocumentFind and PpsDocumentText vfuncs on a worker thread
 * exactly as the shell does. */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <papers-view.h>

typedef struct {
	GMainLoop *loop;
	gboolean finished;
	gint first_match_page;
} SearchWait;

/* PpsDocumentModel::constructed unconditionally connects to the annotation model,
 * which only exists when one was passed to the constructor, so
 * pps_document_model_new () logs three GLib-GObject criticals (upstream, with no
 * participation of this module). Swallow that domain so a critical from the
 * search path itself still stands out. */
static void
ignore_gobject_criticals (const gchar *domain, GLogLevelFlags level, const gchar *message,
                          gpointer user_data)
{
	(void) domain;
	(void) level;
	(void) message;
	(void) user_data;
}

static void
on_search_finished (PpsSearchContext *context, gint first_match_page, gpointer user_data)
{
	SearchWait *wait = user_data;

	(void) context;
	wait->finished = TRUE;
	wait->first_match_page = first_match_page;
	g_main_loop_quit (wait->loop);
}

static gboolean
on_search_timeout (gpointer user_data)
{
	SearchWait *wait = user_data;

	g_main_loop_quit (wait->loop);

	return G_SOURCE_REMOVE;
}

/* Runs one query through a fresh context and returns the result model items. */
static GPtrArray *
run_search (PpsDocument *document, const gchar *term, PpsFindOptions options, gint *first_match_page)
{
	/* The shell's order: the context exists before the model gets a document
	 * (the context's constructor requires the model, not the document).
	 *
	 * Both objects are deliberately leaked: PpsSearchContext::dispose calls
	 * g_clear_object () on its GHashTable* per_page_store
	 * (libview/context/pps-search-context.c:421) instead of
	 * g_clear_pointer (..., g_hash_table_unref), and g_object_unref then reads
	 * the hash table as a GTypeInstance and segfaults. The shell never reaches
	 * that path in a normal session; the test ends right after the searches. */
	PpsDocumentModel *model = pps_document_model_new ();
	PpsSearchContext *context = pps_search_context_new (model);

	pps_document_model_set_document (model, document);
	GPtrArray *results = g_ptr_array_new_with_free_func (g_object_unref);
	SearchWait wait = { .first_match_page = -2 };
	GtkSingleSelection *selection;
	guint i, n_items;

	wait.loop = g_main_loop_new (NULL, FALSE);
	g_signal_connect (context, "finished", G_CALLBACK (on_search_finished), &wait);

	/* A term or option change restarts the search job. */
	pps_search_context_set_search_term (context, term);
	pps_search_context_set_options (context, options);
	g_timeout_add_seconds (60, on_search_timeout, &wait);
	g_main_loop_run (wait.loop);

	g_assert_true (wait.finished);
	if (first_match_page != NULL)
		*first_match_page = wait.first_match_page;

	selection = pps_search_context_get_result_model (context);
	n_items = g_list_model_get_n_items (G_LIST_MODEL (selection));
	for (i = 0; i < n_items; i++) {
		g_autoptr (GObject) item = g_list_model_get_item (G_LIST_MODEL (selection), i);

		g_ptr_array_add (results, g_steal_pointer (&item));
	}

	g_main_loop_unref (wait.loop);

	return results;
}

/* The snippet of one result: <span weight="bold"> + the matching source text
 * + the surrounding words with line breaks turned into spaces. */
static void
assert_result (GPtrArray *results, guint index, guint page, const gchar *bold,
               const gchar *surrounding)
{
	PpsSearchResult *result = g_ptr_array_index (results, index);
	const gchar *markup = pps_search_result_get_markup (result);
	g_autofree gchar *expected_bold = g_strdup_printf ("<span weight=\"bold\">%s</span>", bold);

	g_assert_cmpint (pps_search_result_get_page (result), ==, page);
	g_assert_cmpint (pps_search_result_get_index (result), ==, index);
	g_assert_nonnull (markup);
	g_assert_nonnull (g_strstr_len (markup, -1, expected_bold));
	if (surrounding != NULL)
		g_assert_nonnull (g_strstr_len (markup, -1, surrounding));
}

int
main (int argc, char **argv)
{
	g_autofree gchar *module_path = NULL;
	g_autofree gchar *descriptor_path = NULL;
	g_autofree gchar *mime_xml_path = NULL;
	g_autofree gchar *text_fixture = NULL;
	PpsDocument *document;
	GPtrArray *results;
	gint first_match_page = -2;

	if (argc != 5) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <text.ofd>\n", argv[0]);
		return 2;
	}

	module_path = g_canonicalize_filename (argv[1], NULL);
	descriptor_path = g_canonicalize_filename (argv[2], NULL);
	mime_xml_path = g_canonicalize_filename (argv[3], NULL);
	text_fixture = g_canonicalize_filename (argv[4], NULL);

	document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                  text_fixture);

	/* The shell caches every page during the load job; the search context builds
	 * its page labels from that cache (pps-search-context.c). */
	pps_document_setup_cache (document);
	g_log_set_handler ("GLib-GObject", G_LOG_LEVEL_CRITICAL,
	                   ignore_gobject_criticals, NULL);

	/* Case-insensitive ASCII on page 0: two hits, and the snippet carries the
	 * words around each one, not just the query. */
	results = run_search (document, "invoice", 0, &first_match_page);
	g_assert_cmpint (results->len, ==, 2);
	g_assert_cmpint (first_match_page, ==, 0);
	assert_result (results, 0, 0, "Invoice", "total: 42 EUR");
	assert_result (results, 1, 0, "invoice", "appears lowercase");
	g_ptr_array_unref (results);

	/* Case-sensitive on request: only the lowercase occurrence is a hit, and
	 * the case-sensitive path bold-marks the query spelling. */
	results = run_search (document, "invoice", PPS_FIND_CASE_SENSITIVE, &first_match_page);
	g_assert_cmpint (results->len, ==, 1);
	assert_result (results, 0, 0, "invoice", "appears lowercase");
	g_ptr_array_unref (results);

	/* A joint that only whole-word matching excludes: the sidebar is empty. */
	results = run_search (document, "invo", PPS_FIND_WHOLE_WORDS_ONLY, &first_match_page);
	g_assert_cmpint (results->len, ==, 0);
	g_assert_cmpint (first_match_page, ==, -1);
	g_ptr_array_unref (results);

	/* CJK on page 1: two hits, snippets keep the CJK context around them. */
	results = run_search (document, "发票编号", 0, &first_match_page);
	g_assert_cmpint (results->len, ==, 2);
	g_assert_cmpint (first_match_page, ==, 1);
	assert_result (results, 0, 1, "发票编号", "42");
	assert_result (results, 1, 1, "发票编号", "中文搜索测试");
	g_ptr_array_unref (results);

	g_object_unref (document);
	ofd_test_cleanup ();

	g_print ("phase-2 search snippets passed\n");

	return 0;
}
