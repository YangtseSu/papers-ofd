// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-page.h"

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
