// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-test-utils.h"

#include <glib/gstdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

static gchar *staged_mime_data_dir = NULL;
static gchar *staged_backend_dir = NULL;
static gboolean papers_initialized = FALSE;

static void
remove_mime_staging (void)
{
	g_autofree gchar *mime_dir = NULL;
	g_autofree gchar *packages = NULL;
	g_autofree gchar *globs = NULL;
	g_autofree gchar *globs2 = NULL;
	g_autofree gchar *icons = NULL;

	if (staged_mime_data_dir == NULL)
		return;

	mime_dir = g_build_filename (staged_mime_data_dir, "mime", NULL);
	packages = g_build_filename (mime_dir, "packages", "papers-ofd.xml", NULL);
	globs = g_build_filename (mime_dir, "globs", NULL);
	globs2 = g_build_filename (mime_dir, "globs2", NULL);
	icons = g_build_filename (mime_dir, "icons", NULL);

	g_remove (packages);
	g_remove (globs);
	g_remove (globs2);
	g_rmdir (icons);
	g_rmdir (mime_dir);
	g_rmdir (staged_mime_data_dir);
	g_clear_pointer (&staged_mime_data_dir, g_free);
}

static void
remove_backend_staging (void)
{
	g_autofree gchar *module_link = NULL;
	g_autofree gchar *descriptor = NULL;

	if (staged_backend_dir == NULL)
		return;

	module_link = g_build_filename (staged_backend_dir, "libofddocument.so", NULL);
	descriptor = g_build_filename (staged_backend_dir,
	                               "org.gnome.Papers.OfdDocument.papers-backend", NULL);

	g_remove (module_link);
	g_remove (descriptor);
	g_rmdir (staged_backend_dir);
	g_clear_pointer (&staged_backend_dir, g_free);
}

gchar *
ofd_test_prepare_mime_database (const gchar *mime_xml_path)
{
	g_autofree gchar *packages = NULL;
	g_autofree gchar *mime_dir = NULL;
	g_autofree gchar *target = NULL;
	g_autofree gchar *contents = NULL;
	g_autofree gchar *data_dirs = NULL;
	g_autoptr (GSubprocess) process = NULL;
	GError *error = NULL;
	gsize length = 0;

	if (staged_mime_data_dir != NULL)
		return staged_mime_data_dir;

	staged_mime_data_dir = g_dir_make_tmp ("papers-ofd-xdg-XXXXXX", &error);
	g_assert_no_error (error);

	packages = g_build_filename (staged_mime_data_dir, "mime", "packages", NULL);
	g_assert_cmpint (g_mkdir_with_parents (packages, 0755), ==, 0);

	target = g_build_filename (packages, "papers-ofd.xml", NULL);
	g_assert_true (g_file_get_contents (mime_xml_path, &contents, &length, &error));
	g_assert_no_error (error);
	g_assert_true (g_file_set_contents (target, contents, length, &error));
	g_assert_no_error (error);

	mime_dir = g_build_filename (staged_mime_data_dir, "mime", NULL);

	/* Keep the system directories: only application/ofd is ours to add here.
	 * Set before update-mime-database so it does not warn that the tree it is
	 * writing is outside the search path. */
	data_dirs = g_strdup_printf ("%s:/usr/share:/usr/local/share", staged_mime_data_dir);
	g_setenv ("XDG_DATA_DIRS", data_dirs, TRUE);

	process = g_subprocess_new (G_SUBPROCESS_FLAGS_STDOUT_SILENCE, &error,
	                            "update-mime-database", mime_dir, NULL);
	g_assert_no_error (error);
	g_assert_true (g_subprocess_wait_check (process, NULL, &error));
	g_assert_no_error (error);

	return staged_mime_data_dir;
}

gchar *
ofd_test_stage_backend_dir (const gchar *module_path,
                            const gchar *descriptor_path)
{
	g_autofree gchar *canonical_module = NULL;
	g_autofree gchar *module_link = NULL;
	g_autofree gchar *descriptor_copy = NULL;
	g_autofree gchar *contents = NULL;
	GError *error = NULL;
	gsize length = 0;

	g_assert_null (staged_backend_dir);

	staged_backend_dir = g_dir_make_tmp ("papers-ofd-backends-XXXXXX", &error);
	g_assert_no_error (error);

	/* The descriptor says Module=ofddocument; GModule resolves that to
	 * libofddocument.so inside PPS_BACKENDS_DIR. */
	module_link = g_build_filename (staged_backend_dir, "libofddocument.so", NULL);
	canonical_module = g_canonicalize_filename (module_path, NULL);
	g_assert_cmpint (symlink (canonical_module, module_link), ==, 0);

	descriptor_copy = g_build_filename (staged_backend_dir,
	                                    "org.gnome.Papers.OfdDocument.papers-backend", NULL);
	g_assert_true (g_file_get_contents (descriptor_path, &contents, &length, &error));
	g_assert_no_error (error);
	g_assert_true (g_file_set_contents (descriptor_copy, contents, length, &error));
	g_assert_no_error (error);

	return staged_backend_dir;
}

void
ofd_test_stage_all (const gchar *module_path,
                    const gchar *descriptor_path,
                    const gchar *mime_xml_path)
{
	if (staged_backend_dir == NULL) {
		ofd_test_stage_backend_dir (module_path, descriptor_path);
		g_setenv ("PPS_BACKENDS_DIR", staged_backend_dir, TRUE);
	}
	ofd_test_prepare_mime_database (mime_xml_path);

	if (!papers_initialized) {
		g_assert_true (pps_init ());
		papers_initialized = TRUE;
	}
}

PpsDocument *
ofd_test_load_fixture (const gchar *module_path,
                       const gchar *descriptor_path,
                       const gchar *mime_xml_path,
                       const gchar *fixture_path)
{
	g_autofree gchar *canonical_fixture = NULL;
	g_autofree gchar *uri = NULL;
	GError *error = NULL;
	PpsDocument *document;

	ofd_test_stage_all (module_path, descriptor_path, mime_xml_path);

	canonical_fixture = g_canonicalize_filename (fixture_path, NULL);
	uri = g_filename_to_uri (canonical_fixture, NULL, &error);
	g_assert_no_error (error);

	document = pps_document_factory_get_document (uri, &error);
	if (document == NULL)
		g_error ("backend did not load %s: %s", fixture_path,
		         error != NULL ? error->message : "unknown error");
	g_assert_no_error (error);

	g_assert_true (pps_document_load (document, uri, &error));
	g_assert_no_error (error);

	return document;
}

void
ofd_test_cleanup (void)
{
	remove_backend_staging ();
	remove_mime_staging ();
}
