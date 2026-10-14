// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ofd-document.h"

G_BEGIN_DECLS

/* PpsDocumentLinks, page half (plan §3): one PpsMapping per region of every
 * CLICK action of the page, in rofd's source order. */
PpsMappingList *ofd_links_get_links (OfdDocument *self, PpsPage *page);

/* PpsLinkDest from a rofd GOTO destination: the ABI's absolute physical-page
 * millimetres become page points relative to the box origin (D3), and the
 * presence bits become Papers' change flags. NULL when the destination names no
 * reachable page. */
PpsLinkDest *ofd_link_dest_from_rofd (const rofd_destination_t *destination,
                                      const rofd_rect_t *origin);

/* PpsLinkAction from a rofd action plus its already queried destination (NULL
 * for kinds that carry none). A GOTO destination is re-based on the box origin
 * of the page it targets, which is why the document is needed. NULL when the ABI
 * kind has no Papers counterpart, which is what skips the link. */
PpsLinkAction *ofd_link_action_from_rofd (OfdDocument *self,
                                          const rofd_action_t *action,
                                          const rofd_destination_t *destination);

G_END_DECLS
