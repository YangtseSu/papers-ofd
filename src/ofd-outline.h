// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ofd-document.h"

G_BEGIN_DECLS

/* PpsDocumentLinks, outline half (plan §3, D6): the PpsOutlines tree the
 * sidebar consumes. The model is NULL when the document has no outline, like
 * the PDF backend's. */
GListModel *ofd_outline_get_links_model (OfdDocument *self);

/* Whether the sidebar should offer the outline at all (A11). */
gboolean ofd_outline_has_document_links (OfdDocument *self);

G_END_DECLS
