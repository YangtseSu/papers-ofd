// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-1 print acceptance, through GTK's own print machinery: a real
 * GtkPrintOperation with ACTION_EXPORT ("Print to File") writes a PDF, and the
 * draw-page handler applies the same transform as
 * pps_print_operation_print_draw_page (PPS_SCALE_NONE with autorotate: the page
 * is centered at 1 point per point) before calling
 * pps_document_print_print_page. Extraction with poppler afterwards proves the
 * produced PDF has selectable text rather than a bitmap.
 *
 * Needs a display (the print backend is the GTK PDF surface); exits 77, which
 * Meson reports as a skipped test, when gtk_init_check fails. */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <papers-view.h>
#include <poppler.h>

#include <gtk/gtk.h>
#include <glib/gstdio.h>

/* Mirrors pps_print_operation_print_draw_page for PPS_SCALE_NONE: the print
 * context is in points, so the page is translated to the center and drawn
 * unscaled. */
static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext *context,
           gint page_number,
           gpointer user_data)
{
	PpsDocument *document = user_data;
	PpsPage *page = pps_document_get_page (document, page_number);
	cairo_t *cr = gtk_print_context_get_cairo_context (context);
	gdouble context_width = gtk_print_context_get_width (context);
	gdouble context_height = gtk_print_context_get_height (context);
	gdouble width = 0.0;
	gdouble height = 0.0;

	g_assert_nonnull (page);
	pps_document_get_page_size_uncached (document, page, &width, &height);

	cairo_translate (cr, (context_width - width) / 2.0,
	                 (context_height - height) / 2.0);
	pps_document_print_print_page (PPS_DOCUMENT_PRINT (document), page, cr);

	g_object_unref (page);
}

static void
check_pdf_text (const gchar *pdf_path, const gchar *const *needles)
{
	g_autofree gchar *uri = g_filename_to_uri (pdf_path, NULL, NULL);
	GError *error = NULL;
	PopplerDocument *pdf;
	PopplerPage *page;
	g_autofree gchar *text = NULL;
	guint i;

	g_assert_true (g_file_test (pdf_path, G_FILE_TEST_IS_REGULAR));

	pdf = poppler_document_new_from_file (uri, NULL, &error);
	g_assert_no_error (error);
	g_assert_nonnull (pdf);
	g_assert_cmpint (poppler_document_get_n_pages (pdf), ==, 1);

	page = poppler_document_get_page (pdf, 0);
	g_assert_nonnull (page);
	text = poppler_page_get_text (page);
	g_assert_nonnull (text);

	for (i = 0; needles[i] != NULL; i++) {
		if (g_strstr_len (text, -1, needles[i]) == NULL)
			g_error ("printed PDF text is missing \"%s\"; extracted: %s",
			         needles[i], text);
	}

	g_object_unref (page);
	g_object_unref (pdf);
}

int
main (int argc, char **argv)
{
	static const gchar *const needles[] = { "Invoice", "发票编号", NULL };
	g_autofree gchar *fixture = NULL;
	g_autofree gchar *pdf_path = NULL;
	g_autofree gchar *output_dir = NULL;
	GtkPrintOperation *operation;
	GtkPrintOperationResult result;
	GError *error = NULL;
	PpsDocument *document;

	if (argc < 6) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <fixture.ofd> "
		            "<output.pdf>\n",
		            argv[0]);
		return 2;
	}

	if (!gtk_init_check ()) {
		g_printerr ("no display for GTK: skipping print-to-file check\n");
		return 77;
	}

	fixture = g_canonicalize_filename (argv[4], NULL);
	pdf_path = g_canonicalize_filename (argv[5], NULL);
	output_dir = g_path_get_dirname (pdf_path);
	g_assert_cmpint (g_mkdir_with_parents (output_dir, 0755), ==, 0);

	document = ofd_test_load_fixture (argv[1], argv[2], argv[3], fixture);

	operation = gtk_print_operation_new ();
	gtk_print_operation_set_export_filename (operation, pdf_path);
	gtk_print_operation_set_n_pages (operation, 1);
	gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
	g_signal_connect (operation, "draw-page", G_CALLBACK (draw_page), document);

	result = gtk_print_operation_run (operation, GTK_PRINT_OPERATION_ACTION_EXPORT,
	                                  NULL, &error);
	g_assert_no_error (error);
	if (result != GTK_PRINT_OPERATION_RESULT_APPLY) {
		g_error ("GTK print to file did not apply (result %d)", (gint) result);
	}

	check_pdf_text (pdf_path, needles);

	g_print ("print to file wrote %s with selectable text\n", pdf_path);

	g_object_unref (operation);
	g_object_unref (document);
	ofd_test_cleanup ();

	return 0;
}
