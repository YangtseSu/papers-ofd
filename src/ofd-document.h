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
 * access rules (D1, D2, D3). */

/* Cached physical box in millimetres, origin included (D3). */
gboolean ofd_document_page_box_mm (OfdDocument *self,
                                   gint index,
                                   rofd_rect_t *box_mm);

/* The rofd document handle, borrowed; NULL before a successful load. */
rofd_document_t *ofd_document_borrow_handle (OfdDocument *self);

/* The rofd page handle for a PpsPage: the one attached at get_page (D1), or a
 * handle fetched for this call. Sets *borrowed when the caller must free it. */
rofd_page_t *ofd_document_borrow_page (OfdDocument *self,
                                       PpsPage *page,
                                       gboolean *borrowed);

/* The lazily created renderer, borrowed; NULL when rofd refuses to create one. */
rofd_renderer_t *ofd_document_borrow_renderer (OfdDocument *self);

G_END_DECLS
