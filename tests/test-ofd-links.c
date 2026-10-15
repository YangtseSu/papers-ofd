// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-2 acceptance, headless (plan §5): the page link snapshot as Papers'
 * PpsMappingList (source order, regions, GOTO and URI actions) and the
 * PpsOutlines model the sidebar navigates, both compared against a direct rofd
 * query of the same fixture.
 *
 * The sidebar turns an outline row into a page through
 * pps_document_links_get_link_page, and the view turns a click into a link
 * through pps_mapping_list_get, so both are exercised the way the shell uses
 * them. */

#include "ofd-test-utils.h"

#include <papers-document.h>
#include <rofd.h>

#include <math.h>

#define MM(v) ((v) * 72.0 / 25.4)
#define EPSILON 0.01

static rofd_document_t *
oracle_open (const gchar *fixture_path)
{
	rofd_document_t *document = NULL;
	rofd_error_t *rerror = NULL;

	g_assert_cmpint (rofd_document_open (fixture_path, NULL, &document, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_nonnull (document);

	return document;
}

static rofd_page_t *
oracle_page (rofd_document_t *document, gint index)
{
	rofd_page_t *page = NULL;
	rofd_error_t *rerror = NULL;

	g_assert_cmpint (rofd_document_get_page (document, (size_t) index, &page, &rerror),
	                 ==, ROFD_STATUS_OK);

	return page;
}

/* The fixture's outline, as the ABI reports it: six preorder nodes in four
 * top-level entries, the first one collapsed, the third one a URI, the fourth
 * resolved through a named bookmark, and the fifth a grouping node without an
 * action of its own, which inherits the first reachable GOTO of its subtree. */
static void
test_outline_model (PpsDocument *document)
{
	static const struct {
		const gchar *title;
		gboolean expanded;
		gint page;
		PpsLinkDestType dest_type;
	} nodes[] = {
		{ "第一章 概览", FALSE, 0, PPS_LINK_DEST_TYPE_FIT },
		{ "1.1 详情", TRUE, 1, PPS_LINK_DEST_TYPE_FITH },
		{ "网站", TRUE, -1, PPS_LINK_DEST_TYPE_UNKNOWN },
		{ "书签目标", TRUE, 1, PPS_LINK_DEST_TYPE_XYZ },
		{ "附属内容", TRUE, 1, PPS_LINK_DEST_TYPE_FITV },
		{ "4.1 附录", TRUE, 1, PPS_LINK_DEST_TYPE_FITV },
	};
	g_autoptr (GListModel) model = NULL;
	PpsOutlines *root;
	PpsOutlines *child;
	PpsOutlines *grouped;
	PpsOutlines *grouped_child;
	GListModel *children; /* borrowed from root, like the sidebar uses it */
	guint i;

	g_assert_true (pps_document_links_has_document_links (PPS_DOCUMENT_LINKS (document)));

	model = pps_document_links_get_links_model (PPS_DOCUMENT_LINKS (document));
	g_assert_nonnull (model);

	/* Four top-level entries; the first and the last carry a child. */
	g_assert_cmpint (g_list_model_get_n_items (model), ==, 4);
	for (i = 0; i < 4; i++) {
		g_autoptr (PpsOutlines) outlines = g_list_model_get_item (model, i);
		GListModel *node_children; /* borrowed */

		g_assert_true (PPS_IS_OUTLINES (outlines));
		node_children = pps_outlines_get_children (outlines);
		if (i == 0 || i == 3) {
			g_assert_nonnull (node_children);
			g_assert_cmpint (g_list_model_get_n_items (node_children), ==, 1);
		} else {
			g_assert_null (node_children);
		}
	}

	root = g_list_model_get_item (model, 0);
	children = pps_outlines_get_children (root);
	child = g_list_model_get_item (children, 0);
	grouped = g_list_model_get_item (model, 3);
	grouped_child = g_list_model_get_item (pps_outlines_get_children (grouped), 0);

	/* Preorder flattening of the tree the fixture declares. */
	{
		g_autoptr (PpsOutlines) second = g_list_model_get_item (model, 1);
		g_autoptr (PpsOutlines) third = g_list_model_get_item (model, 2);
		PpsOutlines *flat[6] = { root, child, second, third, grouped, grouped_child };

		for (i = 0; i < G_N_ELEMENTS (flat); i++) {
			g_autofree gchar *markup = NULL;
			g_autofree gchar *escaped = NULL;
			PpsLink *link;
			PpsLinkAction *action;
			PpsLinkDest *dest;

			g_object_get (flat[i], "markup", &markup, NULL);
			escaped = g_markup_escape_text (nodes[i].title, -1);
			g_assert_cmpstr (markup, ==, escaped);
			g_assert_cmpint (pps_outlines_get_expand (flat[i]), ==, nodes[i].expanded);

			/* Every row carries a link: the sidebar dereferences it. */
			link = pps_outlines_get_link (flat[i]);
			g_assert_nonnull (link);
			action = pps_link_get_action (link);
			g_assert_nonnull (action);

			if (i == 2) {
				g_assert_cmpint (pps_link_action_get_action_type (action),
				                 ==, PPS_LINK_ACTION_TYPE_EXTERNAL_URI);
				g_assert_cmpstr (pps_link_action_get_uri (action), ==,
				                 "https://example.test/outline");
				/* No destination: the row is not a page target. */
				g_assert_cmpint (pps_document_links_get_link_page (PPS_DOCUMENT_LINKS (document),
				                                                   link),
				                 ==, -1);
				continue;
			}

			g_assert_cmpint (pps_link_action_get_action_type (action),
			                 ==, PPS_LINK_ACTION_TYPE_GOTO_DEST);
			dest = pps_link_action_get_dest (action);
			g_assert_nonnull (dest);
			g_assert_cmpint (pps_link_dest_get_dest_type (dest), ==, nodes[i].dest_type);
			g_assert_cmpint (pps_link_dest_get_page (dest), ==, nodes[i].page);

			/* link_page is what the sidebar maps a row to a page with. */
			g_assert_cmpint (pps_document_links_get_link_page (PPS_DOCUMENT_LINKS (document),
			                                                   link),
			                 ==, nodes[i].page);
		}
	}

	/* The FitH and XYZ destinations keep the ABI's coordinates (mm to points
	 * from the page's top-left) and their presence bits. The link and its
	 * destination are borrowed from the outlines (transfer none). */
	{
		PpsLink *link = pps_outlines_get_link (child);
		PpsLinkDest *dest = pps_link_action_get_dest (pps_link_get_action (link));
		gboolean change_top = FALSE;
		gdouble top = pps_link_dest_get_top (dest, &change_top);

		g_assert_true (change_top);
		g_assert_cmpfloat_with_epsilon (top, MM (25.0), EPSILON);
	}

	{
		g_autoptr (PpsOutlines) bookmark = g_list_model_get_item (model, 2);
		PpsLink *link = pps_outlines_get_link (bookmark);
		PpsLinkDest *dest = pps_link_action_get_dest (pps_link_get_action (link));
		gboolean change = FALSE;
		gdouble value;

		value = pps_link_dest_get_left (dest, &change);
		g_assert_true (change);
		g_assert_cmpfloat_with_epsilon (value, MM (10.0), EPSILON);

		value = pps_link_dest_get_top (dest, &change);
		g_assert_true (change);
		g_assert_cmpfloat_with_epsilon (value, MM (30.0), EPSILON);

		value = pps_link_dest_get_zoom (dest, &change);
		g_assert_true (change);
		g_assert_cmpfloat_with_epsilon (value, 1.5, EPSILON);
	}

	/* The inherited destination is the child's own FitV, coordinates included. */
	{
		PpsLink *link = pps_outlines_get_link (grouped);
		PpsLinkDest *dest = pps_link_action_get_dest (pps_link_get_action (link));
		gboolean change_left = FALSE;
		gdouble left = pps_link_dest_get_left (dest, &change_left);

		g_assert_true (change_left);
		g_assert_cmpfloat_with_epsilon (left, MM (15.0), EPSILON);
	}

	g_object_unref (grouped_child);
	g_object_unref (grouped);
	g_object_unref (child);
	g_object_unref (root);
}

/* Every page link, region for region and action for action, against the ABI's
 * own snapshot; the mapping the view would activate sits on the object box. */
static void
test_page_links (PpsDocument *document, rofd_document_t *oracle)
{
	PpsPage *page = pps_document_get_page (document, 0);
	rofd_page_t *oracle_page0 = oracle_page (oracle, 0);
	rofd_link_list_t *oracle_links = NULL;
	rofd_error_t *rerror = NULL;
	PpsMappingList *links;
	size_t oracle_count = 0;
	guint i;
	guint ours;

	g_assert_nonnull (page);

	links = pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), page);
	g_assert_nonnull (links);
	g_assert_cmpint (pps_mapping_list_get_page (links), ==, 0);

	g_assert_cmpint (rofd_page_get_links (oracle_page0, &oracle_links, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (rofd_link_list_get_count (oracle_links, &oracle_count, &rerror),
	                 ==, ROFD_STATUS_OK);

	/* rofd publishes one link entry per action: the page-root URI, the two
	 * page-level GOTO entries, the two object actions that resolve, the
	 * unresolved GOTO and the sibling URI action that object carries, and the
	 * object action with two hit regions. The unresolved entry has no page to
	 * jump to, so it is dropped instead of becoming a link to page zero
	 * (plan §3); every other region becomes one mapping. */
	g_assert_cmpint (oracle_count, ==, 8);
	g_assert_cmpint (pps_mapping_list_length (links), ==, 8);

	for (i = 0, ours = 0; i < oracle_count; i++) {
		PpsMapping *region_mappings[8];
		rofd_action_t oracle_action = { 0, };
		rofd_destination_t oracle_destination = { 0, };
		size_t region_count = 0;
		size_t r;

		g_assert_cmpint (rofd_link_list_get_region_count (oracle_links, i, &region_count,
		                                                  &rerror),
		                 ==, ROFD_STATUS_OK);
		g_assert_cmpint ((gint) region_count, >, 0);
		g_assert_cmpint ((gint) region_count, <=, (gint) G_N_ELEMENTS (region_mappings));

		oracle_action.struct_size = sizeof (oracle_action);
		g_assert_cmpint (rofd_link_list_get_action (oracle_links, i, 0, &oracle_action,
		                                           &rerror),
		                 ==, ROFD_STATUS_OK);
		g_assert_cmpint (oracle_action.event, ==, ROFD_ACTION_EVENT_CLICK);

		if (oracle_action.kind == ROFD_ACTION_GOTO) {
			oracle_destination.struct_size = sizeof (oracle_destination);
			g_assert_cmpint (rofd_link_list_get_action_destination (oracle_links, i, 0,
			                                                        &oracle_destination,
			                                                        &rerror),
			                 ==, ROFD_STATUS_OK);

			if (!(oracle_destination.flags & ROFD_DESTINATION_HAS_PAGE_INDEX))
				continue; /* no reachable page: nothing for the module to publish */
		}

		for (r = 0; r < region_count; r++) {
			PpsMapping *mapping;
			PpsLink *link;
			PpsLinkAction *action;
			rofd_rect_t region_mm = { 0, };

			g_assert_cmpint (rofd_link_list_get_region (oracle_links, i, r, &region_mm,
			                                            &rerror),
			                 ==, ROFD_STATUS_OK);

			mapping = pps_mapping_list_nth (links, ours++);
			g_assert_nonnull (mapping);
			region_mappings[r] = mapping;

			/* D3: absolute millimetres become page points from the top-left. */
			g_assert_cmpfloat_with_epsilon (mapping->area.x1, MM (region_mm.x_mm), EPSILON);
			g_assert_cmpfloat_with_epsilon (mapping->area.y1, MM (region_mm.y_mm), EPSILON);
			g_assert_cmpfloat_with_epsilon (mapping->area.x2,
			                                MM (region_mm.x_mm + region_mm.width_mm), EPSILON);
			g_assert_cmpfloat_with_epsilon (mapping->area.y2,
			                                MM (region_mm.y_mm + region_mm.height_mm), EPSILON);

			link = PPS_LINK (pps_mapping_get_data (mapping));
			g_assert_nonnull (link);
			action = pps_link_get_action (link);
			g_assert_nonnull (action);

			if (oracle_action.kind == ROFD_ACTION_URI) {
				g_autofree gchar *expected = g_strdup (oracle_action.uri);
				g_autofree gchar *scheme = g_uri_parse_scheme (oracle_action.uri);

				g_assert_cmpint (pps_link_action_get_action_type (action),
				                 ==, PPS_LINK_ACTION_TYPE_EXTERNAL_URI);

				/* rofd preserves a relative URI and its Base; the module resolves
				 * it against the base so the shell can hand it to the desktop. */
				if (oracle_action.uri_base != NULL && scheme == NULL) {
					g_free (expected);
					expected = g_uri_resolve_relative (oracle_action.uri_base, oracle_action.uri,
					                                   G_URI_FLAGS_NONE, NULL);
				}

				g_assert_nonnull (expected);
				g_assert_cmpstr (pps_link_action_get_uri (action), ==, expected);
			} else {
				PpsLinkDest *dest;
				gdouble zoom;
				gboolean change_zoom = FALSE;
				const guint box_bits = ROFD_DESTINATION_HAS_LEFT |
				                       ROFD_DESTINATION_HAS_TOP |
				                       ROFD_DESTINATION_HAS_RIGHT |
				                       ROFD_DESTINATION_HAS_BOTTOM;

				g_assert_cmpint (pps_link_action_get_action_type (action),
				                 ==, PPS_LINK_ACTION_TYPE_GOTO_DEST);

				dest = pps_link_action_get_dest (action);
				g_assert_nonnull (dest);
				g_assert_cmpint (pps_link_dest_get_page (dest), ==,
				                 (gint) oracle_destination.page_index);

				if (oracle_destination.kind == ROFD_DESTINATION_FIT_R) {
					if ((oracle_destination.flags & box_bits) == box_bits &&
					    oracle_destination.right_mm > oracle_destination.left_mm &&
					    oracle_destination.bottom_mm > oracle_destination.top_mm) {
						g_assert_cmpint (pps_link_dest_get_dest_type (dest),
						                 ==, PPS_LINK_DEST_TYPE_FITR);
						g_assert_cmpfloat_with_epsilon (pps_link_dest_get_left (dest, NULL),
						                                MM (oracle_destination.left_mm), EPSILON);
						g_assert_cmpfloat_with_epsilon (pps_link_dest_get_top (dest, NULL),
						                                MM (oracle_destination.top_mm), EPSILON);
						g_assert_cmpfloat_with_epsilon (pps_link_dest_get_right (dest),
						                                MM (oracle_destination.right_mm), EPSILON);
						g_assert_cmpfloat_with_epsilon (pps_link_dest_get_bottom (dest),
						                                MM (oracle_destination.bottom_mm), EPSILON);
					} else {
						/* R18: the ABI zeroes an absent coordinate and clears its
						 * HAS_* bit, and a zero-size box would make the view's
						 * fit-page zoom divide by zero, so an incomplete FitR
						 * becomes a plain page fit. */
						g_assert_cmpint (pps_link_dest_get_dest_type (dest),
						                 ==, PPS_LINK_DEST_TYPE_FIT);
					}
				}

				zoom = pps_link_dest_get_zoom (dest, &change_zoom);
				g_assert_cmpint (change_zoom, ==,
				                 (oracle_destination.flags & ROFD_DESTINATION_HAS_ZOOM) != 0);
				g_assert_cmpfloat_with_epsilon (zoom, oracle_destination.zoom, EPSILON);
			}
		}

		/* R17: the view resolves the mapping behind a link by pointer identity
		 * (pps_mapping_list_find), so every region needs a link object of its
		 * own; a shared one would anchor the tooltip and the link popover on
		 * the first region. The PDF backend builds one link per mapping too. */
		for (r = 0; r < region_count; r++) {
			gconstpointer data = pps_mapping_get_data (region_mappings[r]);

			g_assert_true (pps_mapping_list_find (links, data) == region_mappings[r]);
			if (r > 0)
				g_assert_true (data != pps_mapping_get_data (region_mappings[0]));
		}
	}
	g_assert_cmpint ((gint) ours, ==, pps_mapping_list_length (links));

	/* Clicking inside the GOTO box must find the smaller mapping, not the
	 * full-page URI action behind it. */
	{
		PpsPoint point = { .x = MM (70.0), .y = MM (110.0) };
		PpsLink *link = PPS_LINK (pps_mapping_list_get_data (links, &point));

		g_assert_nonnull (link);
		g_assert_cmpint (pps_link_action_get_action_type (pps_link_get_action (link)),
		                 ==, PPS_LINK_ACTION_TYPE_GOTO_DEST);
	}

	pps_mapping_list_unref (links);
	rofd_link_list_free (oracle_links);
	rofd_page_free (oracle_page0);
	g_object_unref (page);
}

/* The second page's whole-page GOTO back to the first one, with the ABI's
 * explicit coordinates. */
static void
test_page_links_second_page (PpsDocument *document)
{
	PpsPage *page = pps_document_get_page (document, 1);
	PpsMappingList *links;
	PpsMapping *mapping;
	PpsLink *link;
	PpsLinkAction *action;
	PpsLinkDest *dest;
	gboolean change = FALSE;
	gdouble value;

	g_assert_nonnull (page);

	links = pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), page);
	g_assert_nonnull (links);
	g_assert_cmpint (pps_mapping_list_get_page (links), ==, 1);
	g_assert_cmpint (pps_mapping_list_length (links), ==, 1);

	mapping = pps_mapping_list_nth (links, 0);
	g_assert_nonnull (mapping);
	g_assert_cmpfloat_with_epsilon (mapping->area.x1, 0.0, EPSILON);
	g_assert_cmpfloat_with_epsilon (mapping->area.y1, 0.0, EPSILON);
	g_assert_cmpfloat_with_epsilon (mapping->area.x2, MM (210.0), EPSILON);
	g_assert_cmpfloat_with_epsilon (mapping->area.y2, MM (297.0), EPSILON);

	link = PPS_LINK (pps_mapping_get_data (mapping));
	g_assert_nonnull (link);
	action = pps_link_get_action (link);
	g_assert_nonnull (action);
	g_assert_cmpint (pps_link_action_get_action_type (action), ==, PPS_LINK_ACTION_TYPE_GOTO_DEST);

	dest = pps_link_action_get_dest (action);
	g_assert_nonnull (dest);
	g_assert_cmpint (pps_link_dest_get_dest_type (dest), ==, PPS_LINK_DEST_TYPE_XYZ);
	g_assert_cmpint (pps_link_dest_get_page (dest), ==, 0);

	value = pps_link_dest_get_left (dest, &change);
	g_assert_true (change);
	g_assert_cmpfloat_with_epsilon (value, MM (10.0), EPSILON);

	value = pps_link_dest_get_top (dest, &change);
	g_assert_true (change);
	g_assert_cmpfloat_with_epsilon (value, MM (20.0), EPSILON);

	value = pps_link_dest_get_zoom (dest, &change);
	g_assert_true (change);
	g_assert_cmpfloat_with_epsilon (value, 2.0, EPSILON);

	pps_mapping_list_unref (links);
	g_object_unref (page);
}

/* A page whose PhysicalBox origin is "10 20": link regions and destination
 * coordinates are absolute physical-page millimetres in the ABI, so both must
 * come back re-based on that origin (plan §4, D3). The link box sits at
 * 10 mm / 100 mm absolute, 0 mm / 80 mm from the box corner, and its
 * destination names the same page at 15 mm / 26 mm. */
static void
test_origin_normalised_links (const gchar *module_path,
                              const gchar *descriptor_path,
                              const gchar *mime_xml_path,
                              const gchar *fixture_path)
{
	PpsDocument *document = ofd_test_load_fixture (module_path, descriptor_path,
	                                               mime_xml_path, fixture_path);
	PpsPage *page = pps_document_get_page (document, 0);
	rofd_document_t *oracle = oracle_open (fixture_path);
	rofd_page_t *oracle_page0 = oracle_page (oracle, 0);
	rofd_link_list_t *oracle_links = NULL;
	rofd_error_t *rerror = NULL;
	rofd_rect_t oracle_region = { 0, };
	PpsMappingList *links;
	PpsMapping *mapping;
	PpsLink *link;
	PpsLinkAction *action;
	PpsLinkDest *dest;
	gboolean change = FALSE;
	size_t oracle_count = 0;

	g_assert_nonnull (page);

	links = pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), page);
	g_assert_nonnull (links);
	g_assert_cmpint (pps_mapping_list_length (links), ==, 1);

	mapping = pps_mapping_list_nth (links, 0);
	g_assert_nonnull (mapping);

	/* The oracle keeps the absolute millimetres; the module must subtract the
	 * box origin before converting to points. */
	g_assert_cmpint (rofd_page_get_links (oracle_page0, &oracle_links, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (rofd_link_list_get_count (oracle_links, &oracle_count, &rerror),
	                 ==, ROFD_STATUS_OK);
	g_assert_cmpint (oracle_count, ==, 1);
	g_assert_cmpint (rofd_link_list_get_region (oracle_links, 0, 0, &oracle_region,
	                                            &rerror),
	                 ==, ROFD_STATUS_OK);

	g_assert_cmpfloat_with_epsilon (oracle_region.x_mm, 10.0, 0.5);
	g_assert_cmpfloat_with_epsilon (mapping->area.x1, MM (10.0 - 10.0), EPSILON);
	g_assert_cmpfloat_with_epsilon (mapping->area.y1, MM (100.0 - 20.0), EPSILON);
	g_assert_cmpfloat_with_epsilon (mapping->area.x2, MM (70.0 - 10.0), EPSILON);
	g_assert_cmpfloat_with_epsilon (mapping->area.y2, MM (120.0 - 20.0), EPSILON);

	link = PPS_LINK (pps_mapping_get_data (mapping));
	g_assert_nonnull (link);
	action = pps_link_get_action (link);
	g_assert_nonnull (action);
	g_assert_cmpint (pps_link_action_get_action_type (action), ==, PPS_LINK_ACTION_TYPE_GOTO_DEST);

	dest = pps_link_action_get_dest (action);
	g_assert_nonnull (dest);
	g_assert_cmpint (pps_link_dest_get_dest_type (dest), ==, PPS_LINK_DEST_TYPE_XYZ);
	g_assert_cmpint (pps_link_dest_get_page (dest), ==, 0);

	g_assert_cmpfloat_with_epsilon (pps_link_dest_get_left (dest, &change),
	                                MM (15.0 - 10.0), EPSILON);
	g_assert_true (change);
	g_assert_cmpfloat_with_epsilon (pps_link_dest_get_top (dest, &change),
	                                MM (26.0 - 20.0), EPSILON);
	g_assert_true (change);

	/* The ABI reports the explicit Zoom=0; the standard says a zero zoom keeps
	 * the current one, so the destination must not ask for a zoom change. */
	g_assert_cmpfloat_with_epsilon (pps_link_dest_get_zoom (dest, &change), 0.0, EPSILON);
	g_assert_false (change);

	pps_mapping_list_unref (links);
	rofd_link_list_free (oracle_links);
	rofd_page_free (oracle_page0);
	rofd_document_free (oracle);
	g_object_unref (page);
	g_object_unref (document);
}

/* The two vfuncs without an ABI counterpart are stubs, and out-of-range pages
 * produce no mappings at all. */
static void
test_links_stubs_and_bounds (PpsDocument *document, PpsDocument *no_outline)
{
	PpsPage *out_of_range = pps_page_new (99);
	PpsMappingList *links;

	g_assert_null (pps_document_links_find_link_dest (PPS_DOCUMENT_LINKS (document), "target"));
	g_assert_cmpint (pps_document_links_find_link_page (PPS_DOCUMENT_LINKS (document), "target"),
	                 ==, -1);

	g_assert_null (pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), NULL));
	g_assert_null (pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), out_of_range));

	links = pps_document_links_get_links (PPS_DOCUMENT_LINKS (document), out_of_range);
	g_assert_null (links);

	/* A document without an outline offers no sidebar and no model. */
	g_assert_false (pps_document_links_has_document_links (PPS_DOCUMENT_LINKS (no_outline)));
	g_assert_null (pps_document_links_get_links_model (PPS_DOCUMENT_LINKS (no_outline)));

	g_object_unref (out_of_range);
}

int
main (int argc, char **argv)
{
	g_autofree gchar *module_path = NULL;
	g_autofree gchar *descriptor_path = NULL;
	g_autofree gchar *mime_xml_path = NULL;
	g_autofree gchar *links_fixture = NULL;
	g_autofree gchar *text_fixture = NULL;
	g_autofree gchar *origin_fixture = NULL;
	PpsDocument *document;
	PpsDocument *text_document;
	rofd_document_t *oracle;

	if (argc != 7) {
		g_printerr ("usage: %s <module> <descriptor> <mime-xml> <links.ofd> <text.ofd> "
		            "<origin-text.ofd>\n", argv[0]);
		return 2;
	}

	module_path = g_canonicalize_filename (argv[1], NULL);
	descriptor_path = g_canonicalize_filename (argv[2], NULL);
	mime_xml_path = g_canonicalize_filename (argv[3], NULL);
	links_fixture = g_canonicalize_filename (argv[4], NULL);
	text_fixture = g_canonicalize_filename (argv[5], NULL);
	origin_fixture = g_canonicalize_filename (argv[6], NULL);

	document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                  links_fixture);
	text_document = ofd_test_load_fixture (module_path, descriptor_path, mime_xml_path,
	                                       text_fixture);
	oracle = oracle_open (links_fixture);

	test_outline_model (document);
	test_page_links (document, oracle);
	test_page_links_second_page (document);
	test_links_stubs_and_bounds (document, text_document);
	test_origin_normalised_links (module_path, descriptor_path, mime_xml_path,
	                              origin_fixture);

	rofd_document_free (oracle);
	g_object_unref (text_document);
	g_object_unref (document);
	ofd_test_cleanup ();

	g_print ("phase-2 link and outline checks passed\n");

	return 0;
}
