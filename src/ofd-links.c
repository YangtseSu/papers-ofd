// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-config.h"

#include "ofd-links.h"

#include "ofd-page.h"
#include "ofd-units.h"

/* rofd preserves the source URI and its Base without resolving them; a reader
 * that hands the string to the desktop must resolve first, or a relative target
 * cannot be opened at all. An absolute URI wins, and a failed resolution keeps
 * the raw string (never inventing a target). */
static gchar *
ofd_link_uri (const rofd_action_t *action)
{
	g_autofree gchar *scheme = NULL;

	if (action->uri == NULL)
		return NULL;

	scheme = g_uri_parse_scheme (action->uri);
	if (scheme == NULL && action->uri_base != NULL && action->uri_base[0] != '\0') {
		g_autofree gchar *resolved = g_uri_resolve_relative (action->uri_base,
		                                                     action->uri,
		                                                     G_URI_FLAGS_NONE, NULL);

		if (resolved != NULL)
			return g_steal_pointer (&resolved);
	}

	return g_strdup (action->uri);
}

PpsLinkDest *
ofd_link_dest_from_rofd (const rofd_destination_t *destination,
                         const rofd_rect_t *origin)
{
	gdouble left, top, right, bottom, zoom;
	gboolean change_left, change_top, change_zoom;
	gint page;

	/* An unresolved GOTO (no HAS_PAGE_INDEX) has no page to jump to; the ABI
	 * forbids reading it as page zero (plan §3). */
	if (destination == NULL || !(destination->flags & ROFD_DESTINATION_HAS_PAGE_INDEX))
		return NULL;

	page = (gint) destination->page_index;

	/* Destination coordinates are absolute physical-page millimetres,
	 * independent of render DPI/scale/rotation; Papers page space starts at the
	 * page's top-left (D3). */
	left = ofd_mm_to_pt (destination->left_mm - origin->x_mm);
	top = ofd_mm_to_pt (destination->top_mm - origin->y_mm);
	right = ofd_mm_to_pt (destination->right_mm - origin->x_mm);
	bottom = ofd_mm_to_pt (destination->bottom_mm - origin->y_mm);

	change_left = (destination->flags & ROFD_DESTINATION_HAS_LEFT) != 0;
	change_top = (destination->flags & ROFD_DESTINATION_HAS_TOP) != 0;
	/* GB/T 33190-2016 table 54: Zoom is the target area's page zoom factor in
	 * [0.1, 64.0], and a zero or absent value keeps the current zoom. */
	zoom = destination->zoom;
	change_zoom = (destination->flags & ROFD_DESTINATION_HAS_ZOOM) != 0 && zoom > 0.0;

	switch (destination->kind) {
	case ROFD_DESTINATION_XYZ:
		return pps_link_dest_new_xyz (page, left, top, zoom,
		                              change_left, change_top, change_zoom);
	case ROFD_DESTINATION_FIT:
		return pps_link_dest_new_fit (page);
	case ROFD_DESTINATION_FIT_H:
		return pps_link_dest_new_fith (page, top, change_top);
	case ROFD_DESTINATION_FIT_V:
		return pps_link_dest_new_fitv (page, left, change_left);
	case ROFD_DESTINATION_FIT_R:
		return pps_link_dest_new_fitr (page, left, bottom, right, top);
	default:
		/* An unrecognised Type still names a page, so jump to it (rofd keeps
		 * the source name in mode_name for diagnostics). */
		g_debug ("destination type \"%s\" is unknown; jumping to the page",
		         destination->mode_name != NULL ? destination->mode_name : "(none)");
		return pps_link_dest_new_page (page);
	}
}

PpsLinkAction *
ofd_link_action_from_rofd (OfdDocument *self,
                           const rofd_action_t *action,
                           const rofd_destination_t *destination)
{
	switch (action->kind) {
	case ROFD_ACTION_GOTO: {
		rofd_rect_t origin = { 0, };
		PpsLinkDest *dest;
		PpsLinkAction *link_action;

		/* Destination coordinates are absolute in the *target* page's space,
		 * so that page's box origin is what re-bases them (D3). */
		if (destination == NULL ||
		    !(destination->flags & ROFD_DESTINATION_HAS_PAGE_INDEX) ||
		    !ofd_document_page_box_mm (self, (gint) destination->page_index, &origin))
			return NULL;

		dest = ofd_link_dest_from_rofd (destination, &origin);
		if (dest == NULL)
			return NULL;

		link_action = pps_link_action_new_dest (dest);
		g_object_unref (dest);

		return link_action;
	}
	case ROFD_ACTION_URI: {
		g_autofree gchar *uri = ofd_link_uri (action);

		if (uri == NULL || uri[0] == '\0')
			return NULL;

		return pps_link_action_new_external_uri (uri);
	}
	default:
		/* Attachment and unknown kinds have no Papers counterpart; the plan
		 * skips them rather than guessing an action (plan §3). */
		return NULL;
	}
}

/* Every ABI call publishes an owned error handle; dropping it after each call
 * keeps a later success from leaking the earlier failure. */
static void
ofd_links_clear_error (rofd_error_t **rerror)
{
	if (*rerror != NULL) {
		rofd_error_free (*rerror);
		*rerror = NULL;
	}
}

PpsMappingList *
ofd_links_get_links (OfdDocument *self, PpsPage *page)
{
	OfdPageRef page_ref;
	rofd_link_list_t *links = NULL;
	rofd_error_t *rerror = NULL;
	GList *mappings = NULL;
	size_t count = 0;
	size_t i;

	if (!ofd_page_ref_acquire (self, page, &page_ref))
		return NULL;

	if (rofd_page_get_links (page_ref.rofd_page, &links, &rerror) != ROFD_STATUS_OK) {
		g_debug ("page links failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}
	ofd_links_clear_error (&rerror);

	if (rofd_link_list_get_count (links, &count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("page link count failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}
	ofd_links_clear_error (&rerror);

	for (i = 0; i < count; i++) {
		rofd_action_t action = { 0, };
		rofd_destination_t destination = { 0, };
		PpsLinkAction *link_action;
		PpsLink *link;
		size_t action_count = 0;
		size_t region_count = 0;
		size_t j;
		gboolean found = FALSE;

		if (rofd_link_list_get_action_count (links, i, &action_count, &rerror) != ROFD_STATUS_OK) {
			g_debug ("link %zu action count failed: %s", i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			ofd_links_clear_error (&rerror);
			break;
		}
		ofd_links_clear_error (&rerror);

		for (j = 0; j < action_count && !found; j++) {
			action.struct_size = sizeof (action);
			if (rofd_link_list_get_action (links, i, j, &action, &rerror) != ROFD_STATUS_OK) {
				g_debug ("link %zu action %zu failed: %s", i, j,
				         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
				ofd_links_clear_error (&rerror);
				break;
			}
			ofd_links_clear_error (&rerror);

			/* Mouse hit testing uses CLICK actions; a document-open or
			 * page-open action is not what a click on the page means. */
			if (action.event != ROFD_ACTION_EVENT_CLICK)
				continue;

			if (action.kind == ROFD_ACTION_GOTO) {
				destination.struct_size = sizeof (destination);
				if (rofd_link_list_get_action_destination (links, i, j, &destination,
				                                           &rerror) != ROFD_STATUS_OK) {
					g_debug ("link %zu destination failed: %s", i,
					         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
					ofd_links_clear_error (&rerror);
					continue;
				}
				ofd_links_clear_error (&rerror);
			}

			/* rofd publishes one entry per action (probed: every entry has
			 * action_count 1), so this is the entry's only chance to be a link;
			 * an unresolved GOTO is dropped below rather than read as page zero
			 * (plan §3). */
			found = TRUE;
		}

		if (!found)
			continue;

		link_action = ofd_link_action_from_rofd (self, &action,
		                                         action.kind == ROFD_ACTION_GOTO ? &destination : NULL);
		if (link_action == NULL)
			continue;

		if (rofd_link_list_get_region_count (links, i, &region_count, &rerror) != ROFD_STATUS_OK) {
			g_debug ("link %zu region count failed: %s", i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			ofd_links_clear_error (&rerror);
			g_object_unref (link_action);
			break;
		}
		ofd_links_clear_error (&rerror);

		link = pps_link_new (NULL, link_action);
		g_object_unref (link_action);

		for (j = 0; j < region_count; j++) {
			rofd_rect_t region_mm = { 0, };
			PpsMapping *mapping;

			if (rofd_link_list_get_region (links, i, j, &region_mm, &rerror) != ROFD_STATUS_OK) {
				g_debug ("link %zu region %zu failed: %s", i, j,
				         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
				ofd_links_clear_error (&rerror);
				break;
			}
			ofd_links_clear_error (&rerror);

			mapping = g_new0 (PpsMapping, 1);
			ofd_rect_from_mm (&region_mm, &page_ref.box_mm, &mapping->area);
			/* One reference per mapping: the list's destroy notify drops it. */
			mapping->data = g_object_ref (link);
			mappings = g_list_prepend (mappings, mapping);
		}

		g_object_unref (link);
	}

out:
	ofd_links_clear_error (&rerror);
	if (links != NULL)
		rofd_link_list_free (links);
	ofd_page_ref_release (&page_ref);

	return pps_mapping_list_new (page->index, g_list_reverse (mappings),
	                             (GDestroyNotify) g_object_unref);
}
