// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ofd-document.h"

G_BEGIN_DECLS

/* Decision D1: a PpsPage owns its rofd_page_t through the public
 * PpsPage.backend_page / backend_destroy_func fields, so every page vfunc can
 * reach the handle from the PpsPage it is handed. */
void ofd_page_attach (PpsPage *page, rofd_page_t *rofd_page);
rofd_page_t *ofd_page_get_rofd_page (PpsPage *page);

/* A page plus what every page-scoped query needs: the rofd handle (D1) and the
 * physical box that rectangles crossing the boundary are re-based on (D3).
 * ofd_page_ref_acquire fetches the handle when the caller built its own
 * PpsPage; release frees only a handle acquired that way. */
typedef struct {
	OfdDocument *document;
	rofd_page_t *rofd_page;
	gboolean free_page;
	rofd_rect_t box_mm;
} OfdPageRef;

gboolean ofd_page_ref_acquire (OfdDocument *self, PpsPage *page, OfdPageRef *ref);
void ofd_page_ref_release (OfdPageRef *ref);

double ofd_page_ref_width_pt (const OfdPageRef *ref);
double ofd_page_ref_height_pt (const OfdPageRef *ref);

G_END_DECLS
