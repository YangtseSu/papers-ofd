// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-1 acceptance, headless (plan §5): PpsDocumentInfo carries the document
 * metadata the properties dialog shows, with the fields_mask bits that gate
 * every row, and the page sizes the paper-size row needs (millimetres, the unit
 * pps_document_info_regular_paper_size compares against GTK paper sizes). */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <glib/gstdio.h>

static const gchar *
field_string (const PpsDocumentInfo *info, guint bit, const gchar *value)
{
	g_assert_cmpuint (info->fields_mask & bit, ==, bit);
	g_assert_nonnull (value);

	return value;
}

static void
test_metadata_document (PpsDocument *document)
{
	PpsDocumentInfo *info;
	GDateTime *created;
	GDateTime *modified;

	/* The shell sizes every page during its load job; the module reads the
	 * geometry cache from there (plan §3 get_info). */
	pps_document_setup_cache (document);

	info = pps_document_get_info (document);
	g_assert_nonnull (info);

	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_N_PAGES, ==,
	                  PPS_DOCUMENT_INFO_N_PAGES);
	g_assert_cmpint (info->n_pages, ==, 1);
	g_assert_cmpstr (field_string (info, PPS_DOCUMENT_INFO_TITLE, info->title), ==,
	                 "Phase 1 fixture");
	g_assert_cmpstr (field_string (info, PPS_DOCUMENT_INFO_AUTHOR, info->author), ==,
	                 "Papers OFD tests");
	g_assert_cmpstr (field_string (info, PPS_DOCUMENT_INFO_SUBJECT, info->subject), ==,
	                 "Metadata mapping");
	g_assert_cmpstr (field_string (info, PPS_DOCUMENT_INFO_KEYWORDS, info->keywords), ==,
	                 "alpha, beta");
	g_assert_cmpstr (field_string (info, PPS_DOCUMENT_INFO_CREATOR, info->creator), ==,
	                 "fixture-generator 1.0");
	/* The ABI exposes no producer field, so the row stays hidden. */
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_PRODUCER, ==, 0);
	g_assert_null (info->producer);

	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_CREATION_DATETIME, ==,
	                  PPS_DOCUMENT_INFO_CREATION_DATETIME);
	created = pps_document_info_get_created_datetime (info);
	g_assert_nonnull (created);
	g_assert_cmpint (g_date_time_get_year (created), ==, 2025);
	g_assert_cmpint (g_date_time_get_month (created), ==, 1);
	g_assert_cmpint (g_date_time_get_day_of_month (created), ==, 6);

	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_MOD_DATETIME, ==,
	                  PPS_DOCUMENT_INFO_MOD_DATETIME);
	modified = pps_document_info_get_modified_datetime (info);
	g_assert_nonnull (modified);
	g_assert_cmpint (g_date_time_get_year (modified), ==, 2024);
	g_assert_cmpint (g_date_time_get_month (modified), ==, 10);
	g_assert_cmpint (g_date_time_get_day_of_month (modified), ==, 22);

	/* Millimetres, and the bit that makes the dialog compute the paper size
	 * (pps_document_info_pages and pps_document_info_regular_paper_size both
	 * gate on PPS_DOCUMENT_INFO_PAPER_SIZE). */
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_PAPER_SIZE, ==,
	                  PPS_DOCUMENT_INFO_PAPER_SIZE);
	g_assert_nonnull (info->page_sizes);
	g_assert_cmpfloat_with_epsilon (info->page_sizes[0].width, 210.0, 0.01);
	g_assert_cmpfloat_with_epsilon (info->page_sizes[0].height, 297.0, 0.01);
	{
		gint pages = 0;

		g_assert_true (pps_document_info_pages (info, &pages));
		g_assert_cmpint (pages, ==, 1);
	}

	pps_document_info_free (info);

	/* The window-title path also reads get_info; it must agree. */
	g_assert_cmpstr (pps_document_get_title (document), ==, "Phase 1 fixture");
}

static void
test_geometry_document (PpsDocument *document)
{
	PpsDocumentInfo *info;
	gint i;
	static const struct {
		double width;
		double height;
	} expected[] = {
		{ 210.0, 297.0 },
		{ 148.0, 210.0 },
		{ 297.0, 210.0 },
	};

	pps_document_setup_cache (document);

	info = pps_document_get_info (document);
	g_assert_nonnull (info);

	g_assert_cmpint (info->n_pages, ==, 3);
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_PAPER_SIZE, ==,
	                  PPS_DOCUMENT_INFO_PAPER_SIZE);
	g_assert_nonnull (info->page_sizes);
	for (i = 0; i < 3; i++) {
		g_assert_cmpfloat_with_epsilon (info->page_sizes[i].width,
		                                expected[i].width, 0.01);
		g_assert_cmpfloat_with_epsilon (info->page_sizes[i].height,
		                                expected[i].height, 0.01);
	}

	/* Nothing to show from a document without DocInfo. */
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_TITLE, ==, 0);
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_AUTHOR, ==, 0);
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_KEYWORDS, ==, 0);
	g_assert_cmpuint (info->fields_mask & PPS_DOCUMENT_INFO_CREATION_DATETIME, ==, 0);
	g_assert_null (info->title);
	g_assert_null (info->creator);

	pps_document_info_free (info);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *metadata = NULL;
	g_autofree gchar *geometry = NULL;
	PpsDocument *metadata_document;
	PpsDocument *geometry_document;

	if (argc < 6) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <metadata.ofd> "
		            "<geometry-3pages.ofd>\n",
		            argv[0]);
		return 2;
	}

	metadata = g_canonicalize_filename (argv[4], NULL);
	geometry = g_canonicalize_filename (argv[5], NULL);

	metadata_document = ofd_test_load_fixture (argv[1], argv[2], argv[3], metadata);
	geometry_document = ofd_test_load_fixture (argv[1], argv[2], argv[3], geometry);

	test_metadata_document (metadata_document);
	test_geometry_document (geometry_document);

	g_object_unref (metadata_document);
	g_object_unref (geometry_document);
	ofd_test_cleanup ();

	g_print ("phase-1 metadata fields and fields_mask checks passed\n");

	return 0;
}
