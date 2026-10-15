// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>
#include <rofd.h>

G_BEGIN_DECLS

#define OFD_TYPE_DOCUMENT (ofd_document_get_type ())

G_DECLARE_FINAL_TYPE (OfdDocument, ofd_document, OFD, DOCUMENT, PpsDocument)

/* Internals shared with the interface implementations in ofd-text.c,
 * ofd-links.c and ofd-outline.c: the document owns the rofd handle, the
 * geometry cache and the renderer, and those modules must not duplicate the
 * access rules (D1, D2, D3).
 *
 * Locking (D2, R7): one GRWLock guards everything the document owns — the handle,
 * the geometry array and the renderer. `load`/`load_fd` write, every vfunc reads,
 * which is the reference backend's model (libview/pps-poppler.c:288-305 for the
 * writer, :354-362 and the rest for the readers) and what rofd.h requires: "no
 * handle is freed while another call is using it" (rofd-ffi/include/rofd.h, the
 * document-lifetime contract). The lock is *not* recursive, so the entry points
 * take it exactly once — a public vfunc holds the read lock and calls the
 * `_locked` helpers below, which never take it again. */

/* Physical box in millimetres, origin included (D3), filling the cache when it
 * is cold. Takes the locks it needs by itself, so it must not be called while
 * either lock is held. */
gboolean ofd_document_page_box_mm (OfdDocument *self,
                                   gint index,
                                   rofd_rect_t *box_mm);

/* The same box without filling the cache — a reader may not write to it — for
 * callers that already hold the read lock (the vfuncs, through their page
 * references). Queries rofd directly on a miss. */
gboolean ofd_document_page_box_mm_locked (OfdDocument *self,
                                          gint index,
                                          rofd_rect_t *box_mm);

/* The lock itself, for the interface implementations outside ofd-document.c
 * (OfdDocument is opaque there). Every vfunc takes the read lock for its whole
 * body; the writer lock belongs to load/load_fd and the geometry fill alone. They
 * are not recursive: never take one twice on the same thread. */
void ofd_document_reader_lock (OfdDocument *self);
void ofd_document_reader_unlock (OfdDocument *self);

/* The rofd document handle, borrowed; NULL before a successful load. The read
 * lock is held by the caller. */
rofd_document_t *ofd_document_borrow_handle_locked (OfdDocument *self);

/* The rofd page handle for a PpsPage: the one attached at get_page (D1), or a
 * handle fetched for this call. Sets *borrowed when the caller must free it —
 * rofd pages are independently owned and outlive their document, so the release
 * may happen after the read lock is dropped. The read lock is held by the
 * caller. */
rofd_page_t *ofd_document_borrow_page_locked (OfdDocument *self,
                                              PpsPage *page,
                                              gboolean *borrowed);

/* The renderer created at load, borrowed; NULL when rofd refused to create one.
 * The read lock is held by the caller. */
rofd_renderer_t *ofd_document_borrow_renderer_locked (OfdDocument *self);

G_END_DECLS
