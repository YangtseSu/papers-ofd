// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>
#include <rofd.h>

G_BEGIN_DECLS

/* Decision D1: a PpsPage owns its rofd_page_t through the public
 * PpsPage.backend_page / backend_destroy_func fields, so every page vfunc can
 * reach the handle from the PpsPage it is handed. */
void ofd_page_attach (PpsPage *page, rofd_page_t *rofd_page);
rofd_page_t *ofd_page_get_rofd_page (PpsPage *page);

G_END_DECLS
