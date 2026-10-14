// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-config.h"

#include "ofd-document.h"

#include "ofd-page.h"
#include "ofd-metadata.h"
#include "ofd-render.h"
#include "ofd-units.h"

#include <rofd.h>
#include <string.h>

struct _OfdDocument;

typedef struct {
	gboolean set;
	rofd_rect_t box_mm;
} OfdPageGeometry;

struct _OfdDocument {
	PpsDocument parent_instance;

	rofd_document_t *rofd_document;
	/* Created on first render with NULL options: rofd's CJK-capable defaults
	 * (D8). Guarded by `mutex`. */
	rofd_renderer_t *renderer;
	gint n_pages;

	/* Decision D2: one mutex for our own mutable state only; the rofd ABI is
	 * safe for the concurrent read-only calls Papers' worker pool issues. */
	GMutex mutex;
	/* Per-page geometry cache: the physical box in millimetres, origin included,
	 * filled by get_page_size and reused by the rectangle conversion (D3). */
	OfdPageGeometry *geometry;
	gint geometry_len;
};

static void ofd_document_print_iface_init (PpsDocumentPrintInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE (OfdDocument, ofd_document, PPS_TYPE_DOCUMENT,
                               G_IMPLEMENT_INTERFACE (PPS_TYPE_DOCUMENT_PRINT,
                                                      ofd_document_print_iface_init))

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
 * millimetres. The fast path does not parse page content (A17/A18). */
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

/* Cached physical box (millimetres, origin included) for one page; the single
 * lookup used by get_page_size and render (plan §4/D3). */
static gboolean
ofd_document_page_box (OfdDocument *self, gint index, rofd_rect_t *box_mm)
{
	if (index < 0 || index >= self->n_pages)
		return FALSE;

	g_mutex_lock (&self->mutex);
	if (self->geometry[index].set) {
		*box_mm = self->geometry[index].box_mm;
		g_mutex_unlock (&self->mutex);
		return TRUE;
	}
	g_mutex_unlock (&self->mutex);

	if (!ofd_document_query_page_box (self, index, box_mm))
		return FALSE;

	g_mutex_lock (&self->mutex);
	self->geometry[index].box_mm = *box_mm;
	self->geometry[index].set = TRUE;
	g_mutex_unlock (&self->mutex);

	return TRUE;
}

static gboolean
ofd_document_load (PpsDocument *document, const char *uri, GError **error)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	g_autofree gchar *path = NULL;
	rofd_document_t *handle = NULL;
	rofd_error_t *rerror = NULL;
	rofd_status_t status;
	size_t count = 0;

	g_return_val_if_fail (uri != NULL, FALSE);

	/* Local files only: the OFD parser takes a path (plan §3). */
	path = g_filename_from_uri (uri, NULL, NULL);
	if (path == NULL) {
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
		                     "Only local files are supported");
		return FALSE;
	}

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

	g_mutex_lock (&self->mutex);
	if (self->rofd_document != NULL)
		rofd_document_free (self->rofd_document);
	self->rofd_document = handle;
	self->n_pages = (gint) count;
	g_free (self->geometry);
	self->geometry_len = (gint) count;
	self->geometry = count > 0 ? g_new0 (OfdPageGeometry, count) : NULL;
	g_mutex_unlock (&self->mutex);

	return TRUE;
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

	return self->n_pages;
}

static PpsPage *
ofd_document_get_page (PpsDocument *document, gint index)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	rofd_page_t *rpage = NULL;
	rofd_error_t *rerror = NULL;
	PpsPage *page;

	if (index < 0 || index >= self->n_pages)
		return NULL;

	if (rofd_document_get_page (self->rofd_document, (size_t) index,
	                            &rpage, &rerror) != ROFD_STATUS_OK) {
		g_debug ("rofd page %d could not be opened: %s", index,
		         rerror ? rofd_error_get_message (rerror) : "unknown error");
		ofd_rofd_error_free (&rerror);
		return NULL;
	}
	ofd_rofd_error_free (&rerror);

	page = pps_page_new (index);
	ofd_page_attach (page, rpage);

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

	if (page == NULL || !ofd_document_page_box (self, page->index, &box_mm))
		return;

	*width = ofd_mm_to_pt (box_mm.width_mm);
	*height = ofd_mm_to_pt (box_mm.height_mm);
}

/* Lazily create the renderer; the caller must not hold the mutex. */
static rofd_renderer_t *
ofd_document_get_renderer (OfdDocument *self)
{
	rofd_renderer_t *renderer = NULL;

	g_mutex_lock (&self->mutex);
	if (self->renderer == NULL)
		self->renderer = ofd_renderer_new ();
	renderer = self->renderer;
	g_mutex_unlock (&self->mutex);

	return renderer;
}

/* The rofd page handle a page vfunc renders from: the one attached at get_page
 * (D1), or a handle fetched for this call when the caller built its own
 * PpsPage. Sets *borrowed when the caller must free the result. */
static rofd_page_t *
ofd_document_page_handle (OfdDocument *self, PpsPage *page, gboolean *borrowed)
{
	rofd_page_t *rpage;
	rofd_error_t *rerror = NULL;

	*borrowed = FALSE;

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

	if (page == NULL || self->rofd_document == NULL)
		return NULL;

	/* Do not go through pps_document_get_page_size_uncached here: it asserts
	 * that the cache is not loaded, and render runs once it is. */
	{
		rofd_rect_t box_mm = { 0, };

		if (!ofd_document_page_box (self, page->index, &box_mm))
			return NULL;
		width_pt = ofd_mm_to_pt (box_mm.width_mm);
		height_pt = ofd_mm_to_pt (box_mm.height_mm);
	}
	if (width_pt <= 0.0 || height_pt <= 0.0)
		return NULL;

	rpage = ofd_document_page_handle (self, page, &borrowed_page);
	if (rpage == NULL)
		return NULL;

	renderer = ofd_document_get_renderer (self);
	if (renderer != NULL)
		surface = ofd_render_page_surface (renderer, rpage, rc, width_pt, height_pt);

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
	rofd_page_t *rpage;
	rofd_renderer_t *renderer;
	rofd_render_options_t options;
	gboolean borrowed_page = FALSE;

	if (page == NULL || cr == NULL || self->rofd_document == NULL)
		return;

	rpage = ofd_document_page_handle (self, page, &borrowed_page);
	if (rpage == NULL)
		return;

	renderer = ofd_document_get_renderer (self);
	if (renderer != NULL) {
		ofd_render_options_init_print (&options);
		ofd_render_page_onto (renderer, rpage, cr, &options);
	}

	if (borrowed_page)
		rofd_page_free (rpage);
}

static PpsDocumentInfo *
ofd_document_get_info (PpsDocument *document)
{
	OfdDocument *self = OFD_DOCUMENT (document);
	g_autofree PpsDocumentPageSize *sizes = NULL;
	gint resolved = 0;
	gint i;

	/* The shell always sizes every page during the load job (setup_cache), so
	 * this normally reads the geometry cache only; querying is the fallback for
	 * callers that never populated it. */
	if (self->n_pages > 0) {
		sizes = g_new0 (PpsDocumentPageSize, self->n_pages);

		for (i = 0; i < self->n_pages; i++) {
			rofd_rect_t box_mm = { 0, };

			if (!ofd_document_page_box (self, i, &box_mm))
				continue;

			/* Millimetres, like the PDF backend's page_sizes: the properties
			 * dialog compares them against GTK paper sizes in mm (A12). */
			sizes[i].width = box_mm.width_mm;
			sizes[i].height = box_mm.height_mm;
			resolved++;
		}
	}

	return ofd_metadata_new (self->rofd_document, self->n_pages,
	                         resolved > 0 ? sizes : NULL);
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

	if (self->rofd_document != NULL)
		rofd_document_free (self->rofd_document);
	if (self->renderer != NULL)
		rofd_renderer_free (self->renderer);
	g_clear_pointer (&self->geometry, g_free);
	g_mutex_clear (&self->mutex);

	G_OBJECT_CLASS (ofd_document_parent_class)->finalize (object);
}

static void
ofd_document_print_iface_init (PpsDocumentPrintInterface *iface)
{
	iface->print_page = ofd_document_print_page;
}

static void
ofd_document_class_init (OfdDocumentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	PpsDocumentClass *document_class = PPS_DOCUMENT_CLASS (klass);

	gobject_class->finalize = ofd_document_finalize;

	document_class->load = ofd_document_load;
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
	g_mutex_init (&self->mutex);
}
