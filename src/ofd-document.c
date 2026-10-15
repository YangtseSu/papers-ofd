// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-config.h"

#include "ofd-document.h"

#include "ofd-links.h"
#include "ofd-metadata.h"
#include "ofd-outline.h"
#include "ofd-page.h"
#include "ofd-render.h"
#include "ofd-text.h"
#include "ofd-units.h"

#include <rofd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <glib/gstdio.h>

struct _OfdDocument;

typedef struct {
	gboolean set;
	rofd_rect_t box_mm;
} OfdPageGeometry;

struct _OfdDocument {
	PpsDocument parent_instance;

	rofd_document_t *rofd_document;
	/* Created at load, with NULL options: rofd's CJK-capable defaults (D8).
	 * Guarded by `lock`, like everything else the document owns. */
	rofd_renderer_t *renderer;
	gint n_pages;

	/* load_fd bookkeeping (plan §3): the descriptor the previewer handed over and
	 * the temp copy rofd reads when the descriptor cannot be re-entered through
	 * /proc. -1/NULL when the document came from a URI. Guarded by `lock`. */
	gint source_fd;
	gchar *source_temp_path;

	/* Decision D2/R7: one writer lock for our own mutable state (the handle, the
	 * renderer and the geometry cache), readers around every vfunc — the model of
	 * Papers' PDF backend (libview/pps-poppler.c). The rofd ABI is safe for the
	 * concurrent read-only calls Papers' worker pool issues, so this is not a
	 * render lock: renders run concurrently under the read side. */
	GRWLock lock;
	/* Per-page geometry cache: the physical box in millimetres, origin included,
	 * filled by get_page_size and reused by the rectangle conversion (D3). Only
	 * the writer fills it, so readers cannot race on a slot. */
	OfdPageGeometry *geometry;
	gint geometry_len;
};

static void ofd_document_print_iface_init (PpsDocumentPrintInterface *iface);
static void ofd_document_text_iface_init (PpsDocumentTextInterface *iface);
static void ofd_document_find_iface_init (PpsDocumentFindInterface *iface);
static void ofd_document_selection_iface_init (PpsSelectionInterface *iface);
static void ofd_document_links_iface_init (PpsDocumentLinksInterface *iface);

/* The five interfaces Papers dispatches without a NULL check or that the shell
 * needs for text, search, selection, links and the outline (A9, A10, A11). */
G_DEFINE_FINAL_TYPE_WITH_CODE (OfdDocument, ofd_document, PPS_TYPE_DOCUMENT,
                               G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_PRINT,
                                                      ofd_document_print_iface_init)
                               G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_TEXT,
                                                      ofd_document_text_iface_init)
                               G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_FIND,
                                                      ofd_document_find_iface_init)
                               G_IMPLEMENT_INTERFACE (PPS_TYPE_SELECTION,
                                                      ofd_document_selection_iface_init)
                               G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_LINKS,
                                                      ofd_document_links_iface_init))

/* rofd frees take a bare pointer; NULL handling is ours either way. */
static void
ofd_rofd_error_free (rofd_error_t **error)
{
	if (*error != NULL) {
		rofd_error_free (*error);
		*error = NULL;
	}
}

static void
ofd_document_set_error_from_rofd (GError **error,
                                  rofd_status_t status,
                                  rofd_error_t *rerror)
{
	const gchar *message = rerror ? rofd_error_get_message (rerror) : NULL;
	GIOErrorEnum code;

	switch (status) {
	case ROFD_STATUS_INVALID_ARGUMENT:
	case ROFD_STATUS_PAGE_OUT_OF_RANGE:
		code = G_IO_ERROR_INVALID_ARGUMENT;
		break;
	case ROFD_STATUS_INVALID_DOCUMENT:
		code = G_IO_ERROR_INVALID_DATA;
		break;
	case ROFD_STATUS_UNSUPPORTED:
		code = G_IO_ERROR_NOT_SUPPORTED;
		break;
	default:
		code = G_IO_ERROR_FAILED;
		break;
	}

	g_set_error (error, G_IO_ERROR, code, "rofd: %s (status %u)",
	             message ? message : "unknown error", (guint) status);
}

/* One metadata-only or per-page size query, returning the physical box in
 * millimetres. The fast path does not parse page content (A17/A18). The caller
 * holds a lock covering the handle (read or write). */
static gboolean
ofd_document_query_page_box (OfdDocument *self, gint index, rofd_rect_t *box_mm)
{
	rofd_error_t *rerror = NULL;
	gboolean ok = FALSE;

#ifdef HAVE_ROFD_PAGE_SIZE
	ok = rofd_document_get_page_size_mm (self->rofd_document, (size_t) index,
	                                     box_mm, &rerror) == ROFD_STATUS_OK;
#else
	rofd_page_t *rpage = NULL;

	if (rofd_document_get_page (self->rofd_document, (size_t) index,
	                            &rpage, &rerror) == ROFD_STATUS_OK) {
		ok = rofd_page_get_size_mm (rpage, box_mm, &rerror) == ROFD_STATUS_OK;
		rofd_page_free (rpage);
	}
#endif

	if (!ok && rerror != NULL) {
		g_debug ("rofd page %d size query failed: %s", index,
		         rofd_error_get_message (rerror));
	}
	ofd_rofd_error_free (&rerror);

	return ok;
}

/* Physical box for one page, filling the cache (D3, R3). This is the entry point
 * for the vfuncs that do not hold a lock yet; it takes the read lock to answer
 * from the cache, and the write lock to fill it, because a cache slot is shared
 * state two readers could otherwise race on. It is not recursive: callers that
 * already hold the read lock use ofd_document_page_box_mm_locked instead. */
gboolean
ofd_document_page_box_mm (OfdDocument *self, gint index, rofd_rect_t *box_mm)
{
	gboolean cached;

	g_return_val_if_fail (OFD_IS_DOCUMENT (self), FALSE);

	g_rw_lock_reader_lock (&self->lock);
	cached = index >= 0 && index < self->n_pages && self->geometry[index].set;
	if (cached) {
		*box_mm = self->geometry[index].box_mm;
		g_rw_lock_reader_unlock (&self->lock);
		return TRUE;
	}
	g_rw_lock_reader_unlock (&self->lock);

	/* Cold: the box has to be queried, which needs the handle to stay put and
	 * ends in a write to the cache. The reload that the write lock excludes may
	 * have run while the read lock was dropped, so the bounds are re-checked. */
	g_rw_lock_writer_lock (&self->lock);

	if (index < 0 || index >= self->n_pages) {
		g_rw_lock_writer_unlock (&self->lock);
		return FALSE;
	}
	if (self->geometry[index].set) {
		*box_mm = self->geometry[index].box_mm;
		g_rw_lock_writer_unlock (&self->lock);
		return TRUE;
	}

	if (!ofd_document_query_page_box (self, index, box_mm)) {
		g_rw_lock_writer_unlock (&self->lock);
		return FALSE;
	}
	self->geometry[index].box_mm = *box_mm;
	self->geometry[index].set = TRUE;
	g_rw_lock_writer_unlock (&self->lock);

	return TRUE;
}

/* The same box for a caller that already holds the read lock: the cache is read,
 * never written (a reader may not), and a miss is answered by querying rofd
 * directly — the handle is stable while the read lock is held. */
gboolean
ofd_document_page_box_mm_locked (OfdDocument *self, gint index, rofd_rect_t *box_mm)
{
	if (index < 0 || index >= self->n_pages)
		return FALSE;

	if (self->geometry[index].set) {
		*box_mm = self->geometry[index].box_mm;
		return TRUE;
	}

	return ofd_document_query_page_box (self, index, box_mm);
}

/* Drops the fd and the temp copy of a previous load_fd; the caller holds the
 * write lock. rofd reads the whole file during rofd_document_open, but the ABI only
 * promises that call, so the descriptor and the copy stay alive for as long as
 * the document does (plan §3). */
static void
ofd_document_drop_source (OfdDocument *self)
{
	if (self->source_fd >= 0) {
		close (self->source_fd);
		self->source_fd = -1;
	}

	if (self->source_temp_path != NULL) {
		if (g_unlink (self->source_temp_path) != 0)
			g_debug ("temp copy %s could not be removed: %s",
			         self->source_temp_path, g_strerror (errno));
		g_clear_pointer (&self->source_temp_path, g_free);
	}
}

/* Opens one path and makes it the current document, whatever the caller had
 * loaded before. The caller holds the write lock (R7): replacing the handle and
 * the geometry array is what a concurrent reader must not see half-done. */
static gboolean
ofd_document_load_path (OfdDocument *self, const gchar *path, GError **error)
{
	rofd_document_t *handle = NULL;
	rofd_error_t *rerror = NULL;
	rofd_status_t status;
	size_t count = 0;

	if (rofd_abi_version () != ROFD_ABI_VERSION) {
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
		             "rofd ABI mismatch: module built for v%u, library reports v%u",
		             (guint) ROFD_ABI_VERSION, (guint) rofd_abi_version ());
		return FALSE;
	}

	status = rofd_document_open (path, NULL, &handle, &rerror);
	if (status != ROFD_STATUS_OK) {
		ofd_document_set_error_from_rofd (error, status, rerror);
		ofd_rofd_error_free (&rerror);
		return FALSE;
	}
	ofd_rofd_error_free (&rerror);

	status = rofd_document_get_page_count (handle, &count, &rerror);
	if (status != ROFD_STATUS_OK) {
		ofd_document_set_error_from_rofd (error, status, rerror);
		ofd_rofd_error_free (&rerror);
		rofd_document_free (handle);
		return FALSE;
	}
	ofd_rofd_error_free (&rerror);

	if (self->rofd_document != NULL)
		rofd_document_free (self->rofd_document);
	self->rofd_document = handle;
	self->n_pages = (gint) count;
	g_free (self->geometry);
	self->geometry_len = (gint) count;
	self->geometry = count > 0 ? g_new0 (OfdPageGeometry, count) : NULL;
	ofd_document_drop_source (self);

	/* The renderer belongs to the loaded document, not to the first render:
	 * creating it here keeps the render paths read-only, so two of them cannot
	 * race on the pointer (R7). */
	if (self->renderer != NULL)
		rofd_renderer_free (self->renderer);
	self->renderer = ofd_renderer_new ();

	return TRUE;
}

static gboolean
ofd_document_load (PpsDocument *document, const char *uri, GError **error)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	g_autofree gchar *path = NULL;
	gboolean ok;

	g_return_val_if_fail (uri != NULL, FALSE);

	/* Local files only: the OFD parser takes a path (plan §3). */
	path = g_filename_from_uri (uri, NULL, NULL);
	if (path == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
		                     "Only local files are supported");
		return FALSE;
	}

	g_rw_lock_writer_lock (&self->lock);
	ok = ofd_document_load_path (self, path, error);
	g_rw_lock_writer_unlock (&self->lock);

	return ok;
}

#define OFD_FD_COPY_CHUNK 65536

/* Copies the descriptor's bytes into a fresh file below the user cache, which is
 * where rofd can read them from when the descriptor itself is not reachable
 * through /proc (plan §3). The copy is removed again by drop_source. */
static gboolean
ofd_document_write_fd_copy (gint fd, gchar **temp_path_out, GError **error)
{
	g_autofree gchar *dir = g_build_filename (g_get_user_cache_dir (), "papers-ofd", NULL);
	g_autofree gchar *temp_path = g_build_filename (dir, "ofd-fd-XXXXXX", NULL);
	guchar buffer[OFD_FD_COPY_CHUNK];
	gint temp_fd;
	gint errsv;
	gboolean ok = FALSE;

	if (g_mkdir_with_parents (dir, 0700) != 0) {
		errsv = errno;
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errsv),
		             "Cannot create %s: %s", dir, g_strerror (errsv));
		return FALSE;
	}

	temp_fd = g_mkstemp (temp_path);
	if (temp_fd == -1) {
		errsv = errno;
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errsv),
		             "Cannot create a temporary copy in %s: %s", dir, g_strerror (errsv));
		return FALSE;
	}

	/* The caller's descriptor may sit anywhere in the file; the copy is the whole
	 * document. Reads and writes are looped over partial results and EINTR. */
	if (lseek (fd, 0, SEEK_SET) == (off_t) -1) {
		errsv = errno;
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errsv),
		             "Cannot rewind the document descriptor: %s", g_strerror (errsv));
		goto out;
	}

	for (;;) {
		ssize_t nread = read (fd, buffer, sizeof buffer);
		gsize written = 0;

		if (nread < 0) {
			if (errno == EINTR)
				continue;
			errsv = errno;
			g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errsv),
			             "Cannot read the document descriptor: %s", g_strerror (errsv));
			goto out;
		}
		if (nread == 0)
			break;

		while (written < (gsize) nread) {
			ssize_t nwritten = write (temp_fd, buffer + written,
			                          (gsize) nread - written);

			if (nwritten < 0) {
				if (errno == EINTR)
					continue;
				errsv = errno;
				g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errsv),
				             "Cannot write %s: %s", temp_path, g_strerror (errsv));
				goto out;
			}
			written += (gsize) nwritten;
		}
	}

	ok = TRUE;

out:
	close (temp_fd);
	if (!ok) {
		g_unlink (temp_path);
		return FALSE;
	}

	*temp_path_out = g_steal_pointer (&temp_path);
	return TRUE;
}

/* PpsDocument::load_fd — the previewer path (plan §3). The wrapper has already
 * checked that @fd is a readable regular file and hands its ownership over,
 * including when this vfunc fails. The whole body holds the write lock: it
 * installs a handle like `load` does, and the temp-copy fallback copies the file
 * — a reload that is already committed to its path must not be interleaved with
 * readers of the previous document. */
static gboolean
ofd_document_load_fd (PpsDocument *document, int fd, GError **error)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	g_autofree gchar *proc_path = NULL;
	g_autofree gchar *temp_path = NULL;
	gint dup_fd;
	gint errsv;
	gboolean ok = FALSE;

	/* Keep a descriptor of our own for the fallback below, and for the document's
	 * lifetime: rofd opens by path, so the file must stay reachable. F_DUPFD_CLOEXEC
	 * because the copy lives as long as the document and a plain dup () would be
	 * inherited by every child the shell spawns (Papers' own descriptor handling
	 * uses the same call, libview/pps-jobs.c:798-811). */
	dup_fd = fcntl (fd, F_DUPFD_CLOEXEC, 3);
	if (dup_fd < 0) {
		errsv = errno;
		g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errsv),
		             "Cannot duplicate the document descriptor: %s", g_strerror (errsv));
		close (fd);
		return FALSE;
	}
	close (fd);

	g_rw_lock_writer_lock (&self->lock);

	/* /proc/self/fd re-enters the same file, including one the caller has already
	 * unlinked — which is what the previewer's temporary copies are. */
	proc_path = g_strdup_printf ("/proc/self/fd/%d", dup_fd);
	if (ofd_document_load_path (self, proc_path, NULL)) {
		self->source_fd = dup_fd;
		ok = TRUE;
		goto out;
	}

	/* No usable /proc (a chroot, a sandbox) or no permission to re-enter the
	 * file: fall back to a copy in the user cache. Its error is the one reported,
	 * because it describes the document the caller wanted to open. */
	if (!ofd_document_write_fd_copy (dup_fd, &temp_path, error)) {
		close (dup_fd);
		goto out;
	}
	close (dup_fd);

	if (!ofd_document_load_path (self, temp_path, error)) {
		g_unlink (temp_path);
		goto out;
	}

	self->source_temp_path = g_steal_pointer (&temp_path);
	ok = TRUE;

out:
	g_rw_lock_writer_unlock (&self->lock);

	return ok;
}

static gboolean
ofd_document_save (PpsDocument *document, const char *uri, GError **error)
{
	/* Dispatched without a NULL check upstream (A9), so it must exist. */
	(void) document;
	(void) uri;

	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
	                     "Saving OFD documents is not supported");
	return FALSE;
}

static gint
ofd_document_get_n_pages (PpsDocument *document)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	gint n_pages;

	g_rw_lock_reader_lock (&self->lock);
	n_pages = self->n_pages;
	g_rw_lock_reader_unlock (&self->lock);

	return n_pages;
}

static PpsPage *
ofd_document_get_page (PpsDocument *document, gint index)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	rofd_page_t *rpage = NULL;
	rofd_error_t *rerror = NULL;
	PpsPage *page = NULL;

	g_rw_lock_reader_lock (&self->lock);

	if (index < 0 || index >= self->n_pages || self->rofd_document == NULL) {
		g_rw_lock_reader_unlock (&self->lock);
		return NULL;
	}

	if (rofd_document_get_page (self->rofd_document, (size_t) index,
	                            &rpage, &rerror) != ROFD_STATUS_OK) {
		g_debug ("rofd page %d could not be opened: %s", index,
		         rerror ? rofd_error_get_message (rerror) : "unknown error");
		ofd_rofd_error_free (&rerror);
		g_rw_lock_reader_unlock (&self->lock);
		return NULL;
	}
	ofd_rofd_error_free (&rerror);

	page = pps_page_new (index);
	ofd_page_attach (page, rpage);

	g_rw_lock_reader_unlock (&self->lock);

	return page;
}

static void
ofd_document_get_page_size (PpsDocument *document, PpsPage *page,
                            double *width, double *height)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	rofd_rect_t box_mm = { 0, };

	*width = 0.0;
	*height = 0.0;

	if (page == NULL || !ofd_document_page_box_mm (self, page->index, &box_mm))
		return;

	*width = ofd_mm_to_pt (box_mm.width_mm);
	*height = ofd_mm_to_pt (box_mm.height_mm);
}

/* The read side of the document lock, for the interface implementations whose
 * translation unit cannot see the struct (R7). */
void
ofd_document_reader_lock (OfdDocument *self)
{
	g_return_if_fail (OFD_IS_DOCUMENT (self));

	g_rw_lock_reader_lock (&self->lock);
}

void
ofd_document_reader_unlock (OfdDocument *self)
{
	g_return_if_fail (OFD_IS_DOCUMENT (self));

	g_rw_lock_reader_unlock (&self->lock);
}

/* The rofd document handle, borrowed; NULL before a successful load. The read
 * lock is held by the caller (R7), which is what keeps the handle alive for as
 * long as the caller uses it — rofd.h requires that no handle is freed while
 * another call is using it. */
rofd_document_t *
ofd_document_borrow_handle_locked (OfdDocument *self)
{
	g_return_val_if_fail (OFD_IS_DOCUMENT (self), NULL);

	return self->rofd_document;
}

/* The renderer created at load; NULL when rofd refused to create one. Read-only,
 * so the read lock suffices (R7). */
rofd_renderer_t *
ofd_document_borrow_renderer_locked (OfdDocument *self)
{
	g_return_val_if_fail (OFD_IS_DOCUMENT (self), NULL);

	return self->renderer;
}

/* The rofd page handle a page vfunc renders from: the one attached at get_page
 * (D1), or a handle fetched for this call when the caller built its own
 * PpsPage. Sets *borrowed when the caller must free the result. The read lock is
 * held by the caller; the handle it returns is independently owned (A2), so the
 * caller may free it after dropping the lock. */
rofd_page_t *
ofd_document_borrow_page_locked (OfdDocument *self, PpsPage *page, gboolean *borrowed)
{
	rofd_page_t *rpage;
	rofd_error_t *rerror = NULL;

	*borrowed = FALSE;

	g_return_val_if_fail (page != NULL, NULL);

	rpage = ofd_page_get_rofd_page (page);
	if (rpage != NULL)
		return rpage;

	if (self->rofd_document == NULL || page->index < 0 ||
	    page->index >= self->n_pages)
		return NULL;

	if (rofd_document_get_page (self->rofd_document, (size_t) page->index,
	                            &rpage, &rerror) != ROFD_STATUS_OK) {
		g_debug ("rofd page %d could not be opened: %s", page->index,
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		if (rerror != NULL)
			rofd_error_free (rerror);
		return NULL;
	}

	*borrowed = TRUE;
	return rpage;
}

static cairo_surface_t *
ofd_document_render (PpsDocument *document, PpsRenderContext *rc)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	PpsPage *page = rc->page;
	rofd_page_t *rpage = NULL;
	gboolean borrowed_page = FALSE;
	rofd_renderer_t *renderer;
	cairo_surface_t *surface = NULL;
	double width_pt = 0.0;
	double height_pt = 0.0;
	rofd_rect_t box_mm = { 0, };

	if (page == NULL)
		return NULL;

	/* The geometry cache is filled before the read section, because filling it
	 * writes (see ofd_document_page_box_mm). Do not go through
	 * pps_document_get_page_size_uncached here: it asserts that the cache is not
	 * loaded, and render runs once it is. */
	if (!ofd_document_page_box_mm (self, page->index, &box_mm))
		return NULL;
	width_pt = ofd_mm_to_pt (box_mm.width_mm);
	height_pt = ofd_mm_to_pt (box_mm.height_mm);
	if (width_pt <= 0.0 || height_pt <= 0.0)
		return NULL;

	g_rw_lock_reader_lock (&self->lock);

	if (self->rofd_document != NULL) {
		rpage = ofd_document_borrow_page_locked (self, page, &borrowed_page);
		if (rpage != NULL) {
			renderer = ofd_document_borrow_renderer_locked (self);
			if (renderer != NULL)
				surface = ofd_render_page_surface (renderer, rpage, rc,
				                                   width_pt, height_pt);
		}
	}

	g_rw_lock_reader_unlock (&self->lock);

	if (borrowed_page)
		rofd_page_free (rpage);

	return surface;
}

/* PpsDocumentPrint is dispatched without a NULL check upstream (A9), so it must
 * draw from this vfunc alone: no target size, no rotation, 1 point per point —
 * the print operation has already transformed the context. */
static void
ofd_document_print_page (PpsDocumentPrint *document_print,
                         PpsPage *page,
                         cairo_t *cr)
{
	OfdDocument *self = OFD_DOCUMENT (document_print);
	rofd_page_t *rpage = NULL;
	rofd_renderer_t *renderer;
	rofd_render_options_t options;
	gboolean borrowed_page = FALSE;

	if (page == NULL || cr == NULL)
		return;

	g_rw_lock_reader_lock (&self->lock);

	if (self->rofd_document != NULL) {
		rpage = ofd_document_borrow_page_locked (self, page, &borrowed_page);
		if (rpage != NULL) {
			renderer = ofd_document_borrow_renderer_locked (self);
			if (renderer != NULL) {
				ofd_render_options_init_print (&options);
				ofd_render_page_onto (renderer, rpage, cr, &options);
			}
		}
	}

	g_rw_lock_reader_unlock (&self->lock);

	if (borrowed_page)
		rofd_page_free (rpage);
}

static PpsDocumentInfo *
ofd_document_get_info (PpsDocument *document)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	g_autofree PpsDocumentPageSize *sizes = NULL;
	PpsDocumentInfo *info;
	gint n_pages;
	gint resolved = 0;
	gint i;

	/* The shell always sizes every page during the load job (setup_cache), so
	 * this normally reads the geometry cache only; querying is the fallback for
	 * callers that never populated it. Each box takes the locks it needs, so this
	 * loop runs outside the read section (a concurrent reload would otherwise be
	 * waiting on us). */
	g_rw_lock_reader_lock (&self->lock);
	n_pages = self->n_pages;
	g_rw_lock_reader_unlock (&self->lock);

	if (n_pages > 0) {
		sizes = g_new0 (PpsDocumentPageSize, n_pages);

		for (i = 0; i < n_pages; i++) {
			rofd_rect_t box_mm = { 0, };

			if (!ofd_document_page_box_mm (self, i, &box_mm))
				continue;

			/* Millimetres, like the PDF backend's page_sizes: the properties
			 * dialog compares them against GTK paper sizes in mm (A12). */
			sizes[i].width = box_mm.width_mm;
			sizes[i].height = box_mm.height_mm;
			resolved++;
		}
	}

	/* The metadata query has to hold the read lock: it borrows the handle. A
	 * reload that ran while the sizes were collected would make the array refer to
	 * other pages, so it is dropped in that case (R16 keeps the partial-array rule
	 * as well: only a fully resolved array is published). */
	g_rw_lock_reader_lock (&self->lock);

	info = ofd_metadata_new (self->rofd_document, self->n_pages,
	                         (n_pages > 0 && self->n_pages == n_pages &&
	                          resolved == n_pages)
	                             ? sizes
	                             : NULL);

	g_rw_lock_reader_unlock (&self->lock);

	return info;
}

static gboolean
ofd_document_get_backend_info (PpsDocument *document, PpsDocumentBackendInfo *info)
{
	(void) document;
	/* Both strings are borrowed: "rofd" is static, the version comes from the
	 * library's static storage (rofd.h contract). */
	info->name = "rofd";
	info->version = rofd_library_version ();

	return TRUE;
}

static void
ofd_document_finalize (GObject *object)
{
	OfdDocument *self = OFD_DOCUMENT (object);

	/* No reader can be inside the vfuncs while the last reference is dropped, so
	 * the frees need no lock — the same shape as the PDF backend's finalize
	 * (pps-poppler.c:185). */
	if (self->rofd_document != NULL)
		rofd_document_free (self->rofd_document);
	ofd_document_drop_source (self);
	if (self->renderer != NULL)
		rofd_renderer_free (self->renderer);
	g_clear_pointer (&self->geometry, g_free);
	g_rw_lock_clear (&self->lock);

	G_OBJECT_CLASS (ofd_document_parent_class)->finalize (object);
}

/* PpsDocumentLinks is dispatched without a NULL check upstream (A9), so every
 * vfunc exists: the two that have no ABI counterpart are stubs. All of them hold
 * the read lock while the outline snapshot borrows the handle (R7). */
static gboolean
ofd_document_has_document_links (PpsDocumentLinks *document_links)
{
	OfdDocument *self = OFD_DOCUMENT (document_links);
	gboolean has_links;

	g_rw_lock_reader_lock (&self->lock);
	has_links = ofd_outline_has_document_links (self);
	g_rw_lock_reader_unlock (&self->lock);

	return has_links;
}

static GListModel *
ofd_document_get_links_model (PpsDocumentLinks *document_links)
{
	OfdDocument *self = OFD_DOCUMENT (document_links);
	GListModel *model;

	g_rw_lock_reader_lock (&self->lock);
	model = ofd_outline_get_links_model (self);
	g_rw_lock_reader_unlock (&self->lock);

	return model;
}

static PpsMappingList *
ofd_document_get_links (PpsDocumentLinks *document_links, PpsPage *page)
{
	OfdDocument *self = OFD_DOCUMENT (document_links);
	PpsMappingList *links;

	if (!PPS_IS_PAGE (page))
		return NULL;

	g_rw_lock_reader_lock (&self->lock);
	links = ofd_links_get_links (self, page);
	g_rw_lock_reader_unlock (&self->lock);

	return links;
}

static PpsLinkDest *
ofd_document_find_link_dest (PpsDocumentLinks *document_links, const gchar *link_name)
{
	/* No name-to-destination lookup in the ABI (plan §3). */
	(void) document_links;
	(void) link_name;

	return NULL;
}

static gint
ofd_document_find_link_page (PpsDocumentLinks *document_links, const gchar *link_name)
{
	/* No name-to-page lookup in the ABI (plan §3). */
	(void) document_links;
	(void) link_name;

	return -1;
}

static void
ofd_document_print_iface_init (PpsDocumentPrintInterface *iface)
{
	iface->print_page = ofd_document_print_page;
}

static void
ofd_document_text_iface_init (PpsDocumentTextInterface *iface)
{
	iface->get_text = ofd_text_get_text;
	iface->get_text_layout = ofd_text_get_text_layout;
	iface->get_text_in_area = ofd_text_get_text_in_area;
	iface->get_text_mapping = ofd_text_get_text_mapping;
	iface->get_text_attrs = ofd_text_get_text_attrs;
}

static void
ofd_document_find_iface_init (PpsDocumentFindInterface *iface)
{
	iface->get_supported_options = ofd_find_get_supported_options;
	iface->find_text = ofd_find_text;
}

static void
ofd_document_selection_iface_init (PpsSelectionInterface *iface)
{
	iface->render_selection = ofd_selection_render_selection;
	iface->get_selected_text = ofd_selection_get_selected_text;
	iface->get_selection_region = ofd_selection_get_selection_region;
}

static void
ofd_document_links_iface_init (PpsDocumentLinksInterface *iface)
{
	iface->has_document_links = ofd_document_has_document_links;
	iface->get_links_model = ofd_document_get_links_model;
	iface->get_links = ofd_document_get_links;
	iface->find_link_dest = ofd_document_find_link_dest;
	iface->find_link_page = ofd_document_find_link_page;
}

static void
ofd_document_class_init (OfdDocumentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	PpsDocumentClass *document_class = PPS_DOCUMENT_CLASS (klass);

	gobject_class->finalize = ofd_document_finalize;

	document_class->load = ofd_document_load;
	document_class->load_fd = ofd_document_load_fd;
	document_class->save = ofd_document_save;
	document_class->get_n_pages = ofd_document_get_n_pages;
	document_class->get_page = ofd_document_get_page;
	document_class->get_page_size = ofd_document_get_page_size;
	document_class->render = ofd_document_render;
	document_class->get_backend_info = ofd_document_get_backend_info;
	document_class->get_info = ofd_document_get_info;
}

static void
ofd_document_init (OfdDocument *self)
{
	self->source_fd = -1;
	g_rw_lock_init (&self->lock);
}
