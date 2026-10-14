// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-2 error-path acceptance (plan §5, fixture matrix): a file that Papers
 * hands to the OFD backend but that cannot be parsed fails with a GError
 * instead of a crash, and reloading the same document recovers.
 *
 * The malformed-zip fixture carries the OFD magic bytes, so MIME detection
 * really does select this backend; the truncated fixture is a valid document
 * whose archive is cut short after the local entries, so the central directory
 * is gone. */

#include "ofd-test-utils.h"

#include <papers-document.h>

#include <glib/gstdio.h>

static PpsDocument *
open_and_expect_failure (const gchar *uri)
{
	PpsDocument *document;
	GError *error = NULL;

	document = pps_document_factory_get_document (uri, &error);
	if (document == NULL) {
		/* Detection itself failed, which is also a clean answer; the plan's
		 * deliberate-failure test covers the other half of this path. */
		g_assert_nonnull (error);
		g_printerr ("no backend for %s: %s\n", uri, error->message);
		g_clear_error (&error);
		return NULL;
	}

	g_assert_false (pps_document_load (document, uri, &error));
	g_assert_nonnull (error);
	g_printerr ("load of %s failed with: %s\n", uri, error->message);
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA);
	g_clear_error (&error);

	return document;
}

/* The previewer and the fd loading path have bytes and no file name, so the
 * shipped MIME definition must detect an OFD from its content alone: the nested
 * magic rule has to outrank application/zip's plain "PK\x03\x04" rule, and a
 * plain zip must stay a zip. */
static void
test_detection_from_bytes (const gchar *ofd_path, const gchar *scratch_dir)
{
	g_autofree gchar *contents = NULL;
	g_autofree gchar *plain_zip_path = NULL;
	g_autofree gchar *plain_zip = NULL;
	g_autofree gchar *content_type = NULL;
	gsize length = 0;
	GError *error = NULL;

	g_assert_true (g_file_get_contents (ofd_path, &contents, &length, &error));
	g_assert_no_error (error);
	g_assert_cmpuint (length, >, 1024);

	content_type = g_content_type_guess (NULL, (const guchar *) contents, 1024, NULL);
	g_assert_cmpstr (content_type, ==, "application/ofd");
	g_clear_pointer (&content_type, g_free);

	/* "OFD.xml" is what the nested rule looks for, so a zip whose first entry is
	 * something else must not become application/ofd. */
	plain_zip = g_malloc0 (1024);
	memcpy (plain_zip, "PK", 2);
	plain_zip[2] = '\x03';
	plain_zip[3] = '\x04';
	memcpy (plain_zip + 30, "hello.txt", strlen ("hello.txt"));
	plain_zip_path = g_build_filename (scratch_dir, "plain.zip", NULL);
	g_assert_true (g_file_set_contents (plain_zip_path, plain_zip, 1024, &error));
	g_assert_no_error (error);

	content_type = g_content_type_guess (NULL, (const guchar *) plain_zip, 1024, NULL);
	g_assert_cmpstr (content_type, ==, "application/zip");
	g_clear_pointer (&content_type, g_free);

	g_remove (plain_zip_path);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *module_path = NULL;
	g_autofree gchar *descriptor_path = NULL;
	g_autofree gchar *mime_xml_path = NULL;
	g_autofree gchar *malformed_fixture = NULL;
	g_autofree gchar *text_fixture = NULL;
	g_autofree gchar *truncated_path = NULL;
	g_autofree gchar *truncated_uri = NULL;
	g_autofree gchar *malformed_uri = NULL;
	g_autofree gchar *missing_uri = NULL;
	g_autofree gchar *output_dir = NULL;
	g_autofree gchar *contents = NULL;
	g_autofree gchar *tmp_dir = NULL;
	gsize length = 0;
	PpsDocument *document;
	GError *error = NULL;

	if (argc != 6) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <malformed-zip.ofd> "
		            "<text.ofd>\n", argv[0]);
		return 2;
	}

	module_path = g_canonicalize_filename (argv[1], NULL);
	descriptor_path = g_canonicalize_filename (argv[2], NULL);
	mime_xml_path = g_canonicalize_filename (argv[3], NULL);
	malformed_fixture = g_canonicalize_filename (argv[4], NULL);
	text_fixture = g_canonicalize_filename (argv[5], NULL);

	ofd_test_stage_all (module_path, descriptor_path, mime_xml_path);

	tmp_dir = g_dir_make_tmp ("papers-ofd-errors-XXXXXX", &error);
	g_assert_no_error (error);

	/* 1. Garbage that still looks like an OFD container. */
	malformed_uri = g_filename_to_uri (malformed_fixture, NULL, &error);
	g_assert_no_error (error);
	document = open_and_expect_failure (malformed_uri);
	g_assert_nonnull (document);
	/* The base class keeps -1 until a load succeeds (pps-document.c:155,446). */
	g_assert_cmpint (pps_document_get_n_pages (document), <, 0);
	g_assert_null (pps_document_get_page (document, 0));

	/* 2. A valid document whose archive is truncated. */
	truncated_path = g_build_filename (tmp_dir, "truncated.ofd", NULL);
	g_assert_true (g_file_get_contents (text_fixture, &contents, &length, &error));
	g_assert_no_error (error);
	g_assert_true (g_file_set_contents (truncated_path, contents, length / 2, &error));
	g_assert_no_error (error);

	truncated_uri = g_filename_to_uri (truncated_path, NULL, &error);
	g_assert_no_error (error);
	document = open_and_expect_failure (truncated_uri);
	g_assert_nonnull (document);

	/* 3. A path that does not exist at all: the factory may refuse to pick a
	 * backend, and either way nothing crashes and no document is loaded. */
	missing_uri = g_filename_to_uri (g_build_filename (tmp_dir, "missing.ofd", NULL),
	                                 NULL, &error);
	g_assert_no_error (error);
	document = open_and_expect_failure (missing_uri);
	if (document != NULL)
		g_object_unref (document);

	/* 4. Detection without a file name, which is what the previewer sees. */
	test_detection_from_bytes (text_fixture, tmp_dir);

	/* 5. Reloading the same document object after a failure works: the load
	 * path replaces (and frees) whatever handle was there. */
	{
		g_autofree gchar *uri = g_filename_to_uri (text_fixture, NULL, &error);
		PpsDocument *reloaded;

		g_assert_no_error (error);
		reloaded = pps_document_factory_get_document (uri, &error);
		g_assert_nonnull (reloaded);
		g_assert_no_error (error);

		g_assert_false (pps_document_load (reloaded, malformed_uri, &error));
		g_clear_error (&error);

		g_assert_true (pps_document_load (reloaded, uri, &error));
		g_assert_no_error (error);
		g_assert_cmpint (pps_document_get_n_pages (reloaded), ==, 2);

		g_object_unref (reloaded);
	}

	g_remove (truncated_path);
	g_rmdir (tmp_dir);
	ofd_test_cleanup ();

	g_print ("phase-2 load error paths passed\n");

	return 0;
}
