// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-config.h"

#include "ofd-outline.h"

#include "ofd-links.h"

/* The action a node activates with: the first GOTO with a reachable page or the
 * first URI, in the node's declaration order. A node without one still gets a
 * PpsLink (without an action), because the sidebar dereferences the link of the
 * selected row unconditionally. */
static PpsLinkAction *
ofd_outline_node_action (OfdDocument *self,
                         const rofd_outline_t *outline,
                         size_t node_index,
                         const rofd_outline_node_t *node)
{
	rofd_error_t *rerror = NULL;
	size_t i;

	for (i = 0; i < node->action_count; i++) {
		rofd_action_t action = { 0, };
		rofd_destination_t destination = { 0, };
		PpsLinkAction *link_action;

		action.struct_size = sizeof (action);
		if (rofd_outline_get_action (outline, node_index, i, &action, &rerror) != ROFD_STATUS_OK) {
			g_debug ("outline node %zu action %zu failed: %s", node_index, i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			break;
		}
		if (rerror != NULL) {
			rofd_error_free (rerror);
			rerror = NULL;
		}

		if (action.kind == ROFD_ACTION_GOTO) {
			destination.struct_size = sizeof (destination);
			if (rofd_outline_get_action_destination (outline, node_index, i, &destination,
			                                         &rerror) != ROFD_STATUS_OK) {
				g_debug ("outline node %zu destination failed: %s", node_index,
				         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
				if (rerror != NULL) {
					rofd_error_free (rerror);
					rerror = NULL;
				}
				continue;
			}
			if (rerror != NULL) {
				rofd_error_free (rerror);
				rerror = NULL;
			}
		}

		link_action = ofd_link_action_from_rofd (self, &action,
		                                         action.kind == ROFD_ACTION_GOTO ? &destination : NULL);
		if (link_action != NULL)
			return link_action;
	}

	if (rerror != NULL)
		rofd_error_free (rerror);

	return NULL;
}

static void
ofd_outline_clear_error (rofd_error_t **rerror)
{
	if (*rerror != NULL) {
		rofd_error_free (*rerror);
		*rerror = NULL;
	}
}

static gboolean
ofd_outline_node_count (OfdDocument *self, size_t *count)
{
	rofd_document_t *document = ofd_document_borrow_handle (self);
	rofd_outline_t *outline = NULL;
	rofd_error_t *rerror = NULL;
	gboolean ok = FALSE;

	*count = 0;

	if (document == NULL)
		return FALSE;

	if (rofd_document_get_outline (document, &outline, &rerror) != ROFD_STATUS_OK) {
		g_debug ("outline query failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}
	ofd_outline_clear_error (&rerror);

	if (rofd_outline_get_count (outline, count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("outline count failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		*count = 0;
		goto out;
	}

	ok = TRUE;

out:
	ofd_outline_clear_error (&rerror);
	if (outline != NULL)
		rofd_outline_free (outline);

	return ok;
}

gboolean
ofd_outline_has_document_links (OfdDocument *self)
{
	size_t count = 0;

	return ofd_outline_node_count (self, &count) && count > 0;
}

/* The ABI hands out a preorder snapshot with parent/first_child/next_sibling
 * relations, so the tree is rebuilt from the relations rather than from
 * subtree contiguity: one children store per node that has children, and each
 * node appended to the store of its parent (plan §5 phase 2). */
GListModel *
ofd_outline_get_links_model (OfdDocument *self)
{
	rofd_document_t *document = ofd_document_borrow_handle (self);
	rofd_outline_t *outline = NULL;
	rofd_error_t *rerror = NULL;
	GListStore *root = NULL;
	GListStore **children_stores = NULL;
	size_t count = 0;
	size_t i;
	GListModel *model = NULL;

	if (document == NULL)
		return NULL;

	if (rofd_document_get_outline (document, &outline, &rerror) != ROFD_STATUS_OK) {
		g_debug ("outline query failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}
	ofd_outline_clear_error (&rerror);

	if (rofd_outline_get_count (outline, &count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("outline count failed: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		goto out;
	}
	ofd_outline_clear_error (&rerror);

	/* No outline: the sidebar must not be offered at all. */
	if (count == 0)
		goto out;

	root = g_list_store_new (PPS_TYPE_OUTLINES);
	children_stores = g_new0 (GListStore *, count);

	for (i = 0; i < count; i++) {
		rofd_outline_node_t node = { 0, };
		PpsLinkAction *link_action;
		PpsOutlines *outlines;
		PpsLink *link;
		g_autofree gchar *markup = NULL;
		GListStore *parent_store;

		node.struct_size = sizeof (node);
		if (rofd_outline_get_node (outline, i, &node, &rerror) != ROFD_STATUS_OK) {
			g_debug ("outline node %zu failed: %s", i,
			         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
			goto out;
		}
		ofd_outline_clear_error (&rerror);

		if (node.parent == ROFD_NO_INDEX) {
			parent_store = root;
		} else {
			/* Preorder guarantees that a parent was visited already; a
			 * violation means malformed relations, and the node is kept
			 * visible as a root instead of being dropped. */
			if (node.parent >= i || children_stores[node.parent] == NULL)
				g_debug ("outline node %zu has unusable parent %zu; showing it at "
				         "the top level", i, node.parent);
			parent_store = node.parent < i ? children_stores[node.parent] : NULL;
			if (parent_store == NULL)
				parent_store = root;
		}

		link_action = ofd_outline_node_action (self, outline, i, &node);
		link = pps_link_new (node.title, link_action);
		if (link_action != NULL)
			g_object_unref (link_action);

		markup = node.title != NULL ? g_markup_escape_text (node.title, -1) : NULL;

		/* pps_outlines_new is declared but not exported either (A21); the type is
		 * a plain final GObject. */
		outlines = g_object_new (PPS_TYPE_OUTLINES, NULL);
		pps_outlines_set_markup (outlines, markup != NULL ? markup : "");
		pps_outlines_set_expand (outlines, node.expanded != 0);
		/* pps_outlines_set_link is declared but not defined (A21); the "link"
		 * property takes its own reference, so g_object_set is the usable path. */
		g_object_set (outlines, "link", link, NULL);
		g_object_unref (link);

		if (node.first_child != ROFD_NO_INDEX) {
			children_stores[i] = g_list_store_new (PPS_TYPE_OUTLINES);
			pps_outlines_set_children (outlines, G_LIST_MODEL (children_stores[i]));
		}

		g_list_store_append (parent_store, outlines);
		g_object_unref (outlines);
	}

	model = G_LIST_MODEL (g_steal_pointer (&root));

out:
	ofd_outline_clear_error (&rerror);
	if (root != NULL)
		g_object_unref (root);
	if (children_stores != NULL) {
		for (i = 0; i < count; i++) {
			if (children_stores[i] != NULL)
				g_object_unref (children_stores[i]);
		}
		g_free (children_stores);
	}
	if (outline != NULL)
		rofd_outline_free (outline);

	return model;
}
