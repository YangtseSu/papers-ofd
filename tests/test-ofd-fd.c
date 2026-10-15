// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-3 fd-loading acceptance (plan §3, §5): the previewer hands Papers a
 * descriptor and a sniffed MIME type instead of a URI, so the document must be
 * built with pps_document_factory_get_document_for_fd and loaded with
 * pps_document_load_fd, and it must behave exactly like the URI path.
 *
 * Observed here: fd and URI documents agree on page count, page sizes, page
 * text, rendered pixels and document info; the descriptor the caller passed is
 * consumed in every outcome; a descriptor of an unlinked file still loads (that
 * is what the previewer's temporary copies look like); the temp-copy fallback
 * for environments without a usable /proc leaves nothing behind; and the
 * descriptors the base class rejects (a pipe, a directory, a write-only file)
 * fail without a crash. */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <papers-view.h>

#include <cairo.h>
#include <errno.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include <string.h>
#include <unistd.h>

static gchar *cache_dir = NULL;

/* ---------------------------------------------------------------- helpers */

static gboolean
fd_is_closed (gint fd)
{
	errno = 0;

	return fcntl (fd, F_GETFD) == -1 && errno == EBADF;
}

/* The load job duplicates the descriptor it keeps behind for reloads (A10). */
static gint
dup_cloexec (gint fd)
{
	gint duplicate = fcntl (fd, F_DUPFD_CLOEXEC, 3);

	g_assert_cmpint (duplicate, >=, 0);

	return duplicate;
}

static void
copy_fixture (const gchar *fixture, const gchar *path)
{
	g_autofree gchar *contents = NULL;
	gsize length = 0;
	GError *error = NULL;

	g_assert_true (g_file_get_contents (fixture, &contents, &length, &error));
	g_assert_no_error (error);
	g_assert_true (g_file_set_contents (path, contents, length, &error));
	g_assert_no_error (error);
}

/* A scratch copy of the fixture and an open descriptor on it. */
static gint
open_fixture_copy (const gchar *fixture, const gchar *dir, const gchar *name,
                   gchar **path_out)
{
	g_autofree gchar *path = g_build_filename (dir, name, NULL);
	gint fd;

	copy_fixture (fixture, path);

	fd = open (path, O_RDONLY | O_CLOEXEC);
	g_assert_cmpint (fd, >=, 0);

	if (path_out != NULL)
		*path_out = g_steal_pointer (&path);

	return fd;
}

/* What the previewer does before the load job runs: sniff the type from the
 * bytes. Note that sniffing reads through the descriptor, so the offset is not
 * at zero when the load starts — which the load must tolerate. */
static gchar *
sniff_mime_type (gint fd)
{
	g_autofree gchar *mime_type = NULL;
	GError *error = NULL;

	mime_type = pps_file_get_mime_type_from_fd (fd, &error);
	g_assert_no_error (error);
	g_assert_cmpstr (mime_type, ==, "application/ofd");

	return g_steal_pointer (&mime_type);
}

/* The document for a MIME type, i.e. what the fd path starts from. */
static PpsDocument *
document_for_fd (gint fd, const gchar *mime_type)
{
	PpsDocument *document;
	GError *error = NULL;

	document = pps_document_factory_get_document_for_fd (fd, mime_type, &error);
	if (document == NULL)
		g_error ("no backend for %s: %s", mime_type,
		         error != NULL ? error->message : "unknown error");
	g_assert_no_error (error);

	return document;
}

static cairo_surface_t *
render_page (PpsDocument *document, PpsPage *page, double scale)
{
	PpsRenderContext *rc;
	cairo_surface_t *surface;

	rc = pps_render_context_new (page, 0, scale, PPS_RENDER_ANNOTS_NONE);
	surface = pps_document_render (document, rc);
	g_assert_nonnull (surface);
	g_assert_cmpint (cairo_surface_status (surface), ==, CAIRO_STATUS_SUCCESS);
	cairo_surface_flush (surface);

	g_object_unref (rc);

	return surface;
}

static void
assert_same_pixels (cairo_surface_t *expected, cairo_surface_t *actual,
                    const gchar *what, gint page_index)
{
	gint width = cairo_image_surface_get_width (expected);
	gint height = cairo_image_surface_get_height (expected);
	gint stride = cairo_image_surface_get_stride (expected);
	gint y;

	g_assert_cmpint (width, ==, cairo_image_surface_get_width (actual));
	g_assert_cmpint (height, ==, cairo_image_surface_get_height (actual));
	g_assert_cmpint (stride, ==, cairo_image_surface_get_stride (actual));

	for (y = 0; y < height; y++) {
		const guint8 *a = cairo_image_surface_get_data (expected) + (gsize) y * stride;
		const guint8 *b = cairo_image_surface_get_data (actual) + (gsize) y * stride;

		if (memcmp (a, b, stride) != 0)
			g_error ("%s: page %d differs at row %d", what, page_index, y);
	}
}

/* Both halves of the contract: the URI document and the fd document are the
 * same document, not merely two documents that open. */
static void
assert_matches_uri_document (PpsDocument *uri_document, PpsDocument *fd_document,
                             const gchar *fixture)
{
	g_autoptr (PpsDocumentInfo) uri_info = pps_document_get_info (uri_document);
	g_autoptr (PpsDocumentInfo) fd_info = pps_document_get_info (fd_document);
	gint n_pages = pps_document_get_n_pages (uri_document);
	gint i;

	g_assert_cmpint (pps_document_get_n_pages (fd_document), ==, n_pages);
	g_assert_cmpint (n_pages, >, 0);

	for (i = 0; i < n_pages; i++) {
		g_autofree gchar *uri_text = NULL;
		g_autofree gchar *fd_text = NULL;
		cairo_surface_t *uri_surface;
		cairo_surface_t *fd_surface;
		PpsPage *uri_page = pps_document_get_page (uri_document, i);
		PpsPage *fd_page = pps_document_get_page (fd_document, i);
		double uri_width = 0.0, uri_height = 0.0, fd_width = 0.0, fd_height = 0.0;

		g_assert_nonnull (uri_page);
		g_assert_nonnull (fd_page);
		g_assert_cmpint (fd_page->index, ==, i);

		pps_document_get_page_size_uncached (uri_document, uri_page,
		                                     &uri_width, &uri_height);
		pps_document_get_page_size_uncached (fd_document, fd_page,
		                                     &fd_width, &fd_height);
		g_assert_cmpfloat (fd_width, ==, uri_width);
		g_assert_cmpfloat (fd_height, ==, uri_height);

		uri_text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (uri_document), uri_page);
		fd_text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (fd_document), fd_page);
		g_assert_cmpstr (fd_text, ==, uri_text);

		uri_surface = render_page (uri_document, uri_page, 1.5);
		fd_surface = render_page (fd_document, fd_page, 1.5);
		assert_same_pixels (uri_surface, fd_surface, fixture, i);
		cairo_surface_destroy (fd_surface);
		cairo_surface_destroy (uri_surface);

		g_object_unref (fd_page);
		g_object_unref (uri_page);
	}

	g_assert_cmpuint (fd_info->fields_mask, ==, uri_info->fields_mask);
	g_assert_cmpint (fd_info->n_pages, ==, uri_info->n_pages);
	g_assert_cmpstr (fd_info->title, ==, uri_info->title);
	g_assert_cmpstr (fd_info->author, ==, uri_info->author);
	g_assert_cmpstr (fd_info->subject, ==, uri_info->subject);
	g_assert_cmpstr (fd_info->keywords, ==, uri_info->keywords);
	g_assert_cmpstr (fd_info->creator, ==, uri_info->creator);

	if (uri_info->creation_datetime != NULL) {
		g_assert_nonnull (fd_info->creation_datetime);
		g_assert_true (g_date_time_equal (fd_info->creation_datetime,
		                                  uri_info->creation_datetime));
	} else {
		g_assert_null (fd_info->creation_datetime);
	}

	if (uri_info->modified_datetime != NULL) {
		g_assert_nonnull (fd_info->modified_datetime);
		g_assert_true (g_date_time_equal (fd_info->modified_datetime,
		                                  uri_info->modified_datetime));
	} else {
		g_assert_null (fd_info->modified_datetime);
	}
}

/* The temp-copy fallback's directory. */
static gchar *
module_cache_dir (void)
{
	return g_build_filename (cache_dir, "papers-ofd", NULL);
}

static gint
count_cache_files (void)
{
	g_autofree gchar *dir_path = module_cache_dir ();
	g_autoptr (GDir) dir = NULL;
	const gchar *name;
	gint count = 0;

	dir = g_dir_open (dir_path, 0, NULL);
	if (dir == NULL)
		return 0;

	while ((name = g_dir_read_name (dir)) != NULL)
		count++;

	return count;
}

/* ------------------------------------------------------------ test cases */

static void
test_fd_matches_uri (const gchar *module_path, const gchar *descriptor_path,
                     const gchar *mime_xml_path, const gchar *fixture,
                     const gchar *scratch_dir)
{
	g_autofree gchar *copy_path = NULL;
	g_autofree gchar *name = NULL;
	g_autofree gchar *mime_type = NULL;
	g_autoptr (PpsDocument) uri_document = NULL;
	PpsDocument *fd_document;
	GError *error = NULL;
	gint fd;
	gint load_fd;

	uri_document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                      fixture);

	name = g_strdup_printf ("fd-%s", strrchr (fixture, '/') + 1);
	fd = open_fixture_copy (fixture, scratch_dir, name, &copy_path);

	mime_type = sniff_mime_type (fd);
	fd_document = document_for_fd (fd, mime_type);
	/* Building the document for a MIME type does not consume the descriptor;
	 * the load does (this is the load job's duplication, A10). */
	g_assert_false (fd_is_closed (fd));

	load_fd = dup_cloexec (fd);
	g_assert_true (pps_document_load_fd (fd_document, load_fd, &error));
	g_assert_no_error (error);
	g_assert_true (fd_is_closed (load_fd));
	g_assert_false (fd_is_closed (fd));

	assert_matches_uri_document (uri_document, fd_document, name);

	g_object_unref (fd_document);
	close (fd);
	g_remove (copy_path);
}

/* R12 (review 2026-10-15): the descriptor the module keeps for the document's
 * lifetime must not be inherited by a child process — Papers spawns helpers
 * (print services, URI handlers), and a plain dup () would carry the document
 * bytes into them, keeping an unlinked file alive. Papers' own descriptor
 * handling uses F_DUPFD_CLOEXEC (libview/pps-jobs.c:798-811), and the previewer
 * hands over a descriptor that is not CLOEXEC either. */
static void
test_fd_not_inherited_by_children (const gchar *module_path, const gchar *descriptor_path,
                                   const gchar *mime_xml_path, const gchar *fixture,
                                   const gchar *scratch_dir)
{
	g_autofree gchar *copy_path = NULL;
	g_autofree gchar *mime_type = NULL;
	g_autofree gchar *base = NULL;
	g_autofree gchar *child_fds = NULL;
	PpsDocument *fd_document;
	GError *error = NULL;
	gchar *argv[] = { (gchar *) "sh", (gchar *) "-c",
	                  (gchar *) "for f in /proc/self/fd/*; do readlink \"$f\" || true; done",
	                  NULL };
	gint status = 0;
	gint fd, handed;

	ofd_test_stage_all (module_path, descriptor_path, mime_xml_path);

	fd = open_fixture_copy (fixture, scratch_dir, "inherit.ofd", &copy_path);
	mime_type = sniff_mime_type (fd);
	fd_document = document_for_fd (fd, mime_type);

	/* Hand over a descriptor without FD_CLOEXEC, like the previewer does. */
	handed = fcntl (fd, F_DUPFD, 3);
	g_assert_cmpint (handed, >=, 0);
	g_assert_false (fcntl (handed, F_GETFD) & FD_CLOEXEC);
	g_assert_true (pps_document_load_fd (fd_document, handed, &error));
	g_assert_no_error (error);

	base = g_path_get_basename (copy_path);
	{
		/* LEAVE_DESCRIPTORS_OPEN, or GLib closes every descriptor above
		 * stderr in the child and the probe could never see a leak. */
		gboolean spawned = g_spawn_sync (NULL, argv, NULL,
		                                 G_SPAWN_SEARCH_PATH | G_SPAWN_LEAVE_DESCRIPTORS_OPEN,
		                                 NULL, NULL, &child_fds, NULL, &status, &error);

		g_assert_no_error (error);
		g_assert_true (spawned);
	}
	g_assert_cmpint (status, ==, 0);
	/* What a child process can see must not include the document file. */
	g_assert_null (g_strstr_len (child_fds, -1, base));

	g_object_unref (fd_document);
	close (fd);
	g_remove (copy_path);
}

/* The previewer opens a temporary copy and unlinks it before Papers gets to the
 * descriptor; /proc/self/fd re-enters the file all the same (plan §3). */
static void
test_fd_of_unlinked_file (const gchar *module_path, const gchar *descriptor_path,
                          const gchar *mime_xml_path, const gchar *fixture,
                          const gchar *scratch_dir)
{
	g_autofree gchar *copy_path = NULL;
	g_autofree gchar *mime_type = NULL;
	g_autoptr (PpsDocument) uri_document = NULL;
	PpsDocument *fd_document;
	g_autofree gchar *uri_text = NULL;
	g_autofree gchar *fd_text = NULL;
	GError *error = NULL;
	gint fd;
	gint load_fd;
	PpsPage *page;

	uri_document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                      fixture);
	fd = open_fixture_copy (fixture, scratch_dir, "unlinked.ofd", &copy_path);
	g_assert_cmpint (g_unlink (copy_path), ==, 0);

	mime_type = sniff_mime_type (fd);
	fd_document = document_for_fd (fd, mime_type);
	load_fd = dup_cloexec (fd);
	g_assert_true (pps_document_load_fd (fd_document, load_fd, &error));
	g_assert_no_error (error);
	g_assert_cmpint (pps_document_get_n_pages (fd_document), ==, 2);

	page = pps_document_get_page (fd_document, 0);
	g_assert_nonnull (page);
	fd_text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (fd_document), page);
	g_object_unref (page);

	page = pps_document_get_page (uri_document, 0);
	g_assert_nonnull (page);
	uri_text = pps_document_text_get_text (PPS_DOCUMENT_TEXT (uri_document), page);
	g_object_unref (page);

	g_assert_cmpstr (fd_text, ==, uri_text);
	g_assert_cmpint (count_cache_files (), ==, 0);

	g_object_unref (fd_document);
	close (fd);
}

/* Without a usable /proc the module copies the bytes into the user cache. A
 * descriptor whose file cannot be re-entered (read permission withdrawn after
 * the open, i.e. exactly the re-open failure the fallback exists for) is the
 * deterministic way to reach that path. */
static void
test_fd_copy_fallback (const gchar *module_path, const gchar *descriptor_path,
                       const gchar *mime_xml_path, const gchar *fixture,
                       const gchar *scratch_dir)
{
	g_autofree gchar *copy_path = NULL;
	g_autofree gchar *uri = NULL;
	g_autofree gchar *mime_type = NULL;
	g_autoptr (PpsDocument) uri_document = NULL;
	PpsDocument *fd_document;
	GError *error = NULL;
	gint fd;
	gint load_fd;

	uri_document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                      fixture);
	fd = open_fixture_copy (fixture, scratch_dir, "unreadable.ofd", &copy_path);
	g_assert_cmpint (g_chmod (copy_path, 0), ==, 0);

	/* The descriptor still reads — the permission was checked when it was
	 * opened — but nothing can open the file through its path any more. */
	g_assert_false (fd_is_closed (fd));

	mime_type = sniff_mime_type (fd);
	fd_document = document_for_fd (fd, mime_type);
	load_fd = dup_cloexec (fd);
	g_assert_true (pps_document_load_fd (fd_document, load_fd, &error));
	g_assert_no_error (error);

	assert_matches_uri_document (uri_document, fd_document, "unreadable.ofd");
	g_assert_cmpint (count_cache_files (), ==, 1);

	/* Reloading from a URI drops the copy the fd path had made. */
	uri = g_filename_to_uri (fixture, NULL, &error);
	g_assert_no_error (error);
	g_assert_true (pps_document_load (fd_document, uri, &error));
	g_assert_no_error (error);
	g_assert_cmpint (count_cache_files (), ==, 0);

	/* And destroying a document that is using one drops it as well. */
	g_object_unref (fd_document);
	fd_document = document_for_fd (fd, mime_type);
	g_assert_true (pps_document_load_fd (fd_document, dup_cloexec (fd), &error));
	g_assert_no_error (error);
	g_assert_cmpint (pps_document_get_n_pages (fd_document), ==, 2);
	g_assert_cmpint (count_cache_files (), ==, 1);
	g_object_unref (fd_document);
	g_assert_cmpint (count_cache_files (), ==, 0);

	close (fd);
	g_chmod (copy_path, 0600);
	g_remove (copy_path);
}

static void
test_fd_rejected_descriptors (const gchar *scratch_dir)
{
	PpsDocument *document;
	GError *error = NULL;
	gint fds[2];
	gint fd;
	gint directory_fd;

	/* A pipe: not a regular file (rejected by the base wrapper). */
	g_assert_cmpint (pipe (fds), ==, 0);
	document = document_for_fd (fds[0], "application/ofd");
	g_assert_false (pps_document_load_fd (document, fds[0], &error));
	g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF);
	g_printerr ("pipe descriptor: %s\n", error->message);
	g_clear_error (&error);
	g_assert_true (fd_is_closed (fds[0]));
	g_object_unref (document);
	close (fds[1]);

	/* A directory. */
	directory_fd = open (scratch_dir, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
	g_assert_cmpint (directory_fd, >=, 0);
	document = document_for_fd (directory_fd, "application/ofd");
	g_assert_false (pps_document_load_fd (document, directory_fd, &error));
	g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF);
	g_clear_error (&error);
	g_assert_true (fd_is_closed (directory_fd));
	g_object_unref (document);

	/* A write-only descriptor on a regular file. */
	{
		g_autofree gchar *path = g_build_filename (scratch_dir, "write-only.ofd", NULL);

		g_assert_true (g_file_set_contents (path, "PK\x03\x04", 4, &error));
		g_assert_no_error (error);
		fd = open (path, O_WRONLY | O_CLOEXEC);
		g_assert_cmpint (fd, >=, 0);
		document = document_for_fd (fd, "application/ofd");
		g_assert_false (pps_document_load_fd (document, fd, &error));
		g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_BADF);
		g_printerr ("write-only descriptor: %s\n", error->message);
		g_clear_error (&error);
		g_assert_true (fd_is_closed (fd));
		g_object_unref (document);
		g_remove (path);
	}
}

/* A descriptor carrying bytes that are not a document fails the same way the
 * URI path does, and the document object stays usable afterwards. */
static void
test_fd_bad_content (const gchar *module_path, const gchar *descriptor_path,
                     const gchar *mime_xml_path, const gchar *fixture,
                     const gchar *scratch_dir)
{
	g_autofree gchar *truncated_path = g_build_filename (scratch_dir, "truncated.ofd", NULL);
	g_autofree gchar *contents = NULL;
	g_autofree gchar *uri = NULL;
	g_autofree gchar *mime_type = NULL;
	g_autoptr (PpsDocument) uri_document = NULL;
	PpsDocument *document;
	g_autofree gchar *uri_error = NULL;
	g_autofree gchar *fd_error_message = NULL;
	gsize length = 0;
	GError *error = NULL;
	gint fd;
	gint load_fd;

	ofd_test_stage_all (module_path, descriptor_path, mime_xml_path);

	g_assert_true (g_file_get_contents (fixture, &contents, &length, &error));
	g_assert_no_error (error);
	g_assert_true (g_file_set_contents (truncated_path, contents, length / 2, &error));
	g_assert_no_error (error);

	/* The URI path's answer for the same bytes. */
	uri = g_filename_to_uri (truncated_path, NULL, &error);
	g_assert_no_error (error);
	uri_document = pps_document_factory_get_document (uri, &error);
	g_assert_nonnull (uri_document);
	g_assert_no_error (error);
	g_assert_false (pps_document_load (uri_document, uri, &error));
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA);
	uri_error = g_strdup (error->message);
	g_clear_error (&error);

	fd = open_fixture_copy (truncated_path, scratch_dir, "truncated-fd.ofd", NULL);
	mime_type = sniff_mime_type (fd);
	document = document_for_fd (fd, mime_type);
	load_fd = dup_cloexec (fd);
	g_assert_false (pps_document_load_fd (document, load_fd, &error));
	g_assert_error (error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA);
	fd_error_message = g_strdup (error->message);
	g_clear_error (&error);
	g_assert_true (fd_is_closed (load_fd));
	g_assert_cmpint (pps_document_get_n_pages (document), <, 0);

	/* The fallback parse reports the document's own error, not an internal
	 * path-probing one. */
	g_assert_cmpstr (fd_error_message, ==, uri_error);

	/* The same object loads a good descriptor afterwards. */
	close (fd);
	fd = open (fixture, O_RDONLY | O_CLOEXEC);
	g_assert_cmpint (fd, >=, 0);
	load_fd = dup_cloexec (fd);
	g_assert_true (pps_document_load_fd (document, load_fd, &error));
	g_assert_no_error (error);
	g_assert_cmpint (pps_document_get_n_pages (document), ==, 2);

	g_object_unref (document);
	close (fd);
	g_remove (truncated_path);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *module_path = NULL;
	g_autofree gchar *descriptor_path = NULL;
	g_autofree gchar *mime_xml_path = NULL;
	g_autofree gchar *text_fixture = NULL;
	g_autofree gchar *metadata_fixture = NULL;
	g_autofree gchar *scratch_dir = NULL;
	g_autofree gchar *cache_home = NULL;
	g_autofree gchar *module_cache = NULL;
	GError *error = NULL;

	if (argc != 6) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <text.ofd> "
		            "<metadata.ofd>\n", argv[0]);
		return 2;
	}

	/* The fd fallback writes below g_get_user_cache_dir(), so keep that tree
	 * inside the test's scratch space; set before anything can cache it. */
	cache_home = g_build_filename (g_get_tmp_dir (), "papers-ofd-cache-XXXXXX", NULL);
	g_assert_nonnull (g_mkdtemp (cache_home));
	g_setenv ("XDG_CACHE_HOME", cache_home, TRUE);
	cache_dir = g_strdup (cache_home);

	module_path = g_canonicalize_filename (argv[1], NULL);
	descriptor_path = g_canonicalize_filename (argv[2], NULL);
	mime_xml_path = g_canonicalize_filename (argv[3], NULL);
	text_fixture = g_canonicalize_filename (argv[4], NULL);
	metadata_fixture = g_canonicalize_filename (argv[5], NULL);

	scratch_dir = g_dir_make_tmp ("papers-ofd-fd-XXXXXX", &error);
	g_assert_no_error (error);

	ofd_test_stage_all (module_path, descriptor_path, mime_xml_path);

	module_cache = module_cache_dir ();
	g_assert_false (g_file_test (module_cache, G_FILE_TEST_EXISTS));

	test_fd_matches_uri (module_path, descriptor_path, mime_xml_path,
	                     text_fixture, scratch_dir);
	test_fd_matches_uri (module_path, descriptor_path, mime_xml_path,
	                     metadata_fixture, scratch_dir);
	test_fd_of_unlinked_file (module_path, descriptor_path, mime_xml_path,
	                          text_fixture, scratch_dir);
	test_fd_not_inherited_by_children (module_path, descriptor_path, mime_xml_path,
	                                   text_fixture, scratch_dir);
	test_fd_copy_fallback (module_path, descriptor_path, mime_xml_path,
	                       text_fixture, scratch_dir);
	test_fd_rejected_descriptors (scratch_dir);
	test_fd_bad_content (module_path, descriptor_path, mime_xml_path,
	                     text_fixture, scratch_dir);

	/* Everything the module created below the cache directory is gone. */
	g_assert_cmpint (count_cache_files (), ==, 0);
	if (g_file_test (module_cache, G_FILE_TEST_EXISTS))
		g_rmdir (module_cache);
	g_rmdir (cache_dir);
	g_rmdir (scratch_dir);

	ofd_test_cleanup ();
	g_print ("phase-3 fd loading passed\n");

	return 0;
}
