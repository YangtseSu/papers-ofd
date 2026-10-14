// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>
#include <rofd.h>

G_BEGIN_DECLS

/* Builds the PpsDocumentInfo for a loaded document (plan §3): the rofd metadata
 * snapshot plus the page count and per-page sizes supplied by the caller (the
 * geometry cache). `page_sizes` holds millimetres, the unit
 * pps_document_info_regular_paper_size expects; pass NULL when no page box is
 * known. Never returns NULL. */
PpsDocumentInfo *ofd_metadata_new (rofd_document_t *document,
                                   gint n_pages,
                                   const PpsDocumentPageSize *page_sizes);

G_END_DECLS
