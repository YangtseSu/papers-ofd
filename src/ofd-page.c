// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-config.h"

#include "ofd-page.h"

#include "ofd-units.h"

#include <string.h>

void
ofd_page_attach (PpsPage *page, rofd_page_t *rofd_page)
{
	g_return_if_fail (PPS_IS_PAGE (page));
	g_return_if_fail (page->backend_page == NULL);

	page->backend_page = rofd_page;
	page->backend_destroy_func = (PpsBackendPageDestroyFunc) rofd_page_free;
}

rofd_page_t *
ofd_page_get_rofd_page (PpsPage *page)
{
	g_return_val_if_fail (PPS_IS_PAGE (page), NULL);

	return (rofd_page_t *) page->backend_page;
}

gboolean
ofd_page_ref_acquire (OfdDocument *self, PpsPage *page, OfdPageRef *ref)
{
	memset (ref, 0, sizeof (*ref));

	if (self == NULL || !OFD_IS_DOCUMENT (self) || !PPS_IS_PAGE (page))
		return FALSE;

	ref->document = self;
	ref->rofd_page = ofd_document_borrow_page_locked (self, page, &ref->free_page);
	if (ref->rofd_page == NULL)
		return FALSE;

	if (!ofd_document_page_box_mm_locked (self, page->index, &ref->box_mm)) {
		ofd_page_ref_release (ref);
		return FALSE;
	}

	return TRUE;
}

void
ofd_page_ref_release (OfdPageRef *ref)
{
	if (ref->free_page && ref->rofd_page != NULL)
		rofd_page_free (ref->rofd_page);

	ref->rofd_page = NULL;
	ref->free_page = FALSE;
}

double
ofd_page_ref_width_pt (const OfdPageRef *ref)
{
	return ofd_mm_to_pt (ref->box_mm.width_mm);
}

double
ofd_page_ref_height_pt (const OfdPageRef *ref)
{
	return ofd_mm_to_pt (ref->box_mm.height_mm);
}
