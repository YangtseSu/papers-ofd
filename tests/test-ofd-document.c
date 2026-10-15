// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-0 acceptance, headless: the module is discovered through Papers' own
 * backend factory with PPS_BACKENDS_DIR pointing at a directory that holds the
 * built module and its descriptor, the fixture loads, and page count and page
 * sizes match the boxes the generator wrote.
 *
 * application/ofd is not registered on a machine that has neither our package
 * nor another OFD package installed, and OFD files are zip archives, so the
 * detection step needs our MIME definition: the test installs it into a scratch
 * XDG_DATA_DIRS tree with update-mime-database, exactly as plan §7 prescribes
 * for validating the shipped file. */

#include <papers-document.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/* A4 portrait, A5 portrait, A4 landscape in points (mm * 72 / 25.4). */
static const double expected_sizes[][2] = {
	{ 595.27559055118115, 841.88976377952766 },
	{ 419.52755905511810, 595.27559055118115 },
	{ 841.88976377952766, 595.27559055118115 },
};

static gboolean
prepare_mime_database (const gchar *mime_xml, gchar **data_dir_out, GError **error)
{
	g_autofree gchar *data_dir = g_dir_make_tmp ("papers-ofd-xdg-XXXXXX", error);
	g_autofree gchar *packages = NULL;
	g_autofree gchar *mime_dir = NULL;
	g_autofree gchar *target = NULL;
	g_autofree gchar *contents = NULL;
	g_autofree gchar *data_dirs = NULL;
	g_autoptr (GSubprocess) process = NULL;
	gsize length = 0;

	if (data_dir == NULL)
		return FALSE;

	packages = g_build_filename (data_dir, "mime", "packages", NULL);
	if (g_mkdir_with_parents (packages, 0755) != 0) {
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
		             "cannot create %s", packages);
		return FALSE;
	}

	target = g_build_filename (packages, "papers-ofd.xml", NULL);
	if (!g_file_get_contents (mime_xml, &contents, &length, error))
		return FALSE;
	if (!g_file_set_contents (target, contents, length, error))
		return FALSE;

	mime_dir = g_build_filename (data_dir, "mime", NULL);
	process = g_subprocess_new (G_SUBPROCESS_FLAGS_STDOUT_SILENCE,
	                            error, "update-mime-database",
	                            mime_dir, NULL);
	if (process == NULL)
		return FALSE;
	if (!g_subprocess_wait_check (process, NULL, error))
		return FALSE;

	/* Keep the system directories: only application/ofd is ours to add here. */
	data_dirs = g_strdup_printf ("%s:/usr/share:/usr/local/share", data_dir);
	g_setenv ("XDG_DATA_DIRS", data_dirs, TRUE);

	*data_dir_out = g_steal_pointer (&data_dir);
	return TRUE;
}

static gchar *
stage_backend_dir (const gchar *module_path, const gchar *descriptor_path,
                   GError **error)
{
	g_autofree gchar *dir = g_dir_make_tmp ("papers-ofd-backends-XXXXXX", error);
	g_autofree gchar *module_link = NULL;
	g_autofree gchar *descriptor_copy = NULL;
	g_autofree gchar *contents = NULL;
	gsize length = 0;

	if (dir == NULL)
		return NULL;

	/* The descriptor says Module=ofddocument; GModule resolves that to
	 * libofddocument.so inside PPS_BACKENDS_DIR. */
	module_link = g_build_filename (dir, "libofddocument.so", NULL);
	if (symlink (module_path, module_link) != 0) {
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
		             "cannot link %s to %s", module_path, module_link);
		return NULL;
	}

	descriptor_copy = g_build_filename (dir, "org.gnome.Papers.OfdDocument.papers-backend", NULL);
	if (!g_file_get_contents (descriptor_path, &contents, &length, error))
		return NULL;
	if (!g_file_set_contents (descriptor_copy, contents, length, error))
		return NULL;

	return g_steal_pointer (&dir);
}

/* The plan's deliberate-failure check: a descriptor whose Module= does not
 * resolve must leave Papers without a backend for application/ofd, which proves
 * the discovery path (descriptor -> module -> query_type) is really exercised
 * and not bypassed. */
static int
run_discovery_failure_test (const gchar *mime_xml, const gchar *fixture_path)
{
	g_autofree gchar *data_dir = NULL;
	g_autofree gchar *backend_dir = NULL;
	g_autofree gchar *descriptor = NULL;
	g_autofree gchar *canonical_fixture = NULL;
	g_autofree gchar *uri = NULL;
	GError *error = NULL;
	PpsDocument *document;
	const gchar *broken =
		"[Papers Backend]\n"
		"Module=doesnotexist\n"
		"TypeDescription=Fixed-layout Documents\n"
		"MimeType=application/ofd;\n";

	if (!prepare_mime_database (mime_xml, &data_dir, &error)) {
		g_error ("MIME staging failed: %s", error->message);
	}

	backend_dir = g_dir_make_tmp ("papers-ofd-broken-XXXXXX", &error);
	g_assert_no_error (error);
	descriptor = g_build_filename (backend_dir,
	                               "org.gnome.Papers.OfdDocument.papers-backend", NULL);
	g_assert_true (g_file_set_contents (descriptor, broken, -1, &error));
	g_assert_no_error (error);
	g_setenv ("PPS_BACKENDS_DIR", backend_dir, TRUE);

	/* Initialise the library *after* the backend directory is set: the factory's
	 * backend list is built by pps_init (), and without it the factory answers
	 * "File type … is not supported" for every MIME type, which asserts nothing
	 * about the discovery path (R6). */
	g_assert_true (pps_init ());

	canonical_fixture = g_canonicalize_filename (fixture_path, NULL);
	uri = g_filename_to_uri (canonical_fixture, NULL, &error);
	g_assert_no_error (error);

	document = pps_document_factory_get_document (uri, &error);
	g_assert_null (document);
	g_assert_nonnull (error);
	/* The descriptor is read (so the MIME type resolves to it) and the module
	 * lookup then fails: proof that descriptor -> g_module_open -> query_type is
	 * the path being exercised. */
	g_assert_nonnull (g_strstr_len (error->message, -1, "Failed to load backend"));
	g_clear_error (&error);

	g_remove (descriptor);
	g_rmdir (backend_dir);

	return 0;
}

int
main (int argc, char **argv)
{
	if (argc >= 4 && g_strcmp0 (argv[1], "--expect-discovery-failure") == 0)
		return run_discovery_failure_test (argv[2], argv[3]);

	PpsDocument *document = NULL;
	PpsDocumentBackendInfo info = { 0, };
	GError *error = NULL;
	g_autofree gchar *module_path = NULL;
	g_autofree gchar *descriptor_path = NULL;
	g_autofree gchar *fixture_path = NULL;
	g_autofree gchar *mime_xml_path = NULL;
	g_autofree gchar *data_dir = NULL;
	g_autofree gchar *backend_dir = NULL;
	g_autofree gchar *uri = NULL;
	gint n_pages;

	(void) argc;

	/* Meson passes relative paths for build-tree targets and the test copies the
	 * module into a temporary directory, so resolve everything first. */
	module_path = g_canonicalize_filename (argv[1], NULL);
	descriptor_path = g_canonicalize_filename (argv[2], NULL);
	fixture_path = g_canonicalize_filename (argv[3], NULL);
	mime_xml_path = g_canonicalize_filename (argv[4], NULL);

	g_assert_true (g_file_test (fixture_path, G_FILE_TEST_IS_REGULAR));
	g_assert_true (g_file_test (mime_xml_path, G_FILE_TEST_IS_REGULAR));

	g_assert_true (prepare_mime_database (mime_xml_path, &data_dir, &error));
	g_assert_no_error (error);

	backend_dir = stage_backend_dir (module_path, descriptor_path, &error);
	g_assert_no_error (error);
	g_assert_nonnull (backend_dir);
	g_assert_cmpint (g_setenv ("PPS_BACKENDS_DIR", backend_dir, TRUE), !=, 0);

	g_assert_true (pps_init ());

	uri = g_filename_to_uri (fixture_path, NULL, &error);
	g_assert_no_error (error);

	document = pps_document_factory_get_document (uri, &error);
	if (document == NULL) {
		g_error ("backend did not load the fixture: %s",
		         error != NULL ? error->message : "unknown error");
	}

	/* The factory only picks the backend by MIME type; loading is the caller's
	 * step (that is how the shell's load job uses it). */
	g_assert_true (pps_document_load (document, uri, &error));
	g_assert_no_error (error);

	g_assert_cmpint (pps_document_get_n_pages (document), ==, 3);

	g_assert_true (pps_document_get_backend_info (document, &info));
	g_assert_cmpstr (info.name, ==, "rofd");
	g_assert_nonnull (info.version);
	g_assert_cmpint (info.version[0], !=, '\0');

	for (gint i = 0; i < 3; i++) {
		PpsPage *page = pps_document_get_page (document, i);
		double width = 0.0, height = 0.0;

		g_assert_nonnull (page);
		g_assert_cmpint (page->index, ==, i);

		pps_document_get_page_size_uncached (document, page, &width, &height);
		g_assert_cmpfloat_with_epsilon (width, expected_sizes[i][0], 0.01);
		g_assert_cmpfloat_with_epsilon (height, expected_sizes[i][1], 0.01);

		g_object_unref (page);
	}

	/* Reachable pages only. */
	g_assert_null (pps_document_get_page (document, 3));
	g_assert_null (pps_document_get_page (document, -1));

	/* save() is dispatched without a NULL check upstream (A9). */
	g_assert_false (pps_document_save (document, "file:///tmp/out.ofd", &error));
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED);
	g_clear_error (&error);

	/* Batch access through the cached path too: this is what setup_cache and the
	 * view use, so it must not assert. */
	pps_document_setup_cache (document);
	g_assert_cmpint (pps_document_get_n_pages (document), ==, 3);

	n_pages = pps_document_get_n_pages (document);
	g_object_unref (document);
	document = NULL;

	g_assert_cmpint (n_pages, ==, 3);

	/* Leave no directory behind. */
	{
		g_autofree gchar *descriptor = g_build_filename (backend_dir,
		                                                 "org.gnome.Papers.OfdDocument.papers-backend",
		                                                 NULL);
		g_autofree gchar *module_link = g_build_filename (backend_dir, "libofddocument.so", NULL);
		g_remove (descriptor);
		g_remove (module_link);
	}
	g_rmdir (backend_dir);
	{
		g_autofree gchar *mime_xml = g_build_filename (data_dir, "mime", "packages",
		                                              "papers-ofd.xml", NULL);
		g_autofree gchar *mime_dir = g_build_filename (data_dir, "mime", NULL);
		g_autofree gchar *glob_cache = g_build_filename (mime_dir, "globs", NULL);
		g_autofree gchar *globs2_cache = g_build_filename (mime_dir, "globs2", NULL);
		g_autofree gchar *icons = g_build_filename (mime_dir, "icons", NULL);
		g_remove (mime_xml);
		g_remove (glob_cache);
		g_remove (globs2_cache);
		g_rmdir (icons);
		g_rmdir (mime_dir);
	}
	g_rmdir (data_dir);

	return 0;
}
