// SPDX-License-Identifier: GPL-2.0-or-later

#include "ofd-metadata.h"

#include <string.h>

/* rofd distinguishes a missing field (NULL) from an explicitly empty one ("" in
 * the document); neither is worth showing, so both map to "unset". */
static gchar *
ofd_metadata_dup (const char *value)
{
	if (value == NULL || value[0] == '\0')
		return NULL;

	return g_strdup (value);
}

static void
ofd_metadata_set_string (PpsDocumentInfo *info,
                         gchar **field,
                         guint bit,
                         const char *value)
{
	gchar *copy = ofd_metadata_dup (value);

	if (copy == NULL)
		return;

	*field = copy;
	info->fields_mask |= bit;
}

/* OFD dates are producer-written strings (rofd.h: "Dates retain their producer
 * representation"): real documents carry both the date-only form the invoice
 * fixture uses and plain ISO 8601 datetimes. g_date_time_new_from_iso8601 needs
 * a timezone unless the string has one, so naive values are read as UTC, which
 * also makes the information dialog print the value as written. */
static GDateTime *
ofd_metadata_parse_datetime (const gchar *value)
{
	g_autoptr (GTimeZone) utc = NULL;

	if (value == NULL || value[0] == '\0')
		return NULL;

	utc = g_time_zone_new_utc ();

	if (g_ascii_isdigit (value[0])) {
		GDateTime *datetime = g_date_time_new_from_iso8601 (value, utc);

		if (datetime != NULL)
			return datetime;

		/* Date-only: the invoice fixture (and the GB/T 33190 examples) use it. */
		if (strlen (value) == 10 && value[4] == '-' && value[7] == '-') {
			g_autofree gchar *midnight = g_strdup_printf ("%sT00:00:00", value);

			return g_date_time_new_from_iso8601 (midnight, utc);
		}
	}

	return NULL;
}

static void
ofd_metadata_set_keywords (PpsDocumentInfo *info, const rofd_metadata_t *metadata)
{
	g_autoptr (GString) joined = NULL;
	rofd_error_t *rerror = NULL;
	size_t count = 0;
	size_t i;

	if (rofd_metadata_get_keyword_count (metadata, &count, &rerror) != ROFD_STATUS_OK) {
		g_debug ("keyword count unavailable: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "unknown error");
		if (rerror != NULL)
			rofd_error_free (rerror);
		return;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	joined = g_string_new (NULL);
	for (i = 0; i < count; i++) {
		const char *keyword = NULL;

		if (rofd_metadata_get_keyword (metadata, i, &keyword, &rerror) != ROFD_STATUS_OK)
			break;
		if (keyword == NULL || keyword[0] == '\0')
			continue;

		if (joined->len > 0)
			g_string_append (joined, ", ");
		g_string_append (joined, keyword);
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	if (joined->len > 0) {
		info->keywords = g_string_free (g_steal_pointer (&joined), FALSE);
		info->fields_mask |= PPS_DOCUMENT_INFO_KEYWORDS;
	}
}

PpsDocumentInfo *
ofd_metadata_new (rofd_document_t *document,
                  gint n_pages,
                  const PpsDocumentPageSize *page_sizes)
{
	PpsDocumentInfo *info = pps_document_info_new ();
	rofd_metadata_t *metadata = NULL;
	rofd_error_t *rerror = NULL;

	/* A negative count means "not loaded"; today it cannot reach here (get_info
	 * runs after a load), but the bit must not promise a count that does not
	 * exist (R16). */
	if (n_pages >= 0) {
		info->n_pages = n_pages;
		info->fields_mask |= PPS_DOCUMENT_INFO_N_PAGES;
	}

	if (page_sizes != NULL && n_pages > 0) {
		info->page_sizes = g_memdup2 (page_sizes,
		                              sizeof (PpsDocumentPageSize) * (gsize) n_pages);
		info->fields_mask |= PPS_DOCUMENT_INFO_PAPER_SIZE;
	}

	if (document == NULL ||
	    rofd_document_get_metadata (document, &metadata, &rerror) != ROFD_STATUS_OK) {
		g_debug ("metadata unavailable: %s",
		         rerror != NULL ? rofd_error_get_message (rerror) : "no document");
		if (rerror != NULL)
			rofd_error_free (rerror);
		return info;
	}
	if (rerror != NULL)
		rofd_error_free (rerror);

	ofd_metadata_set_string (info, &info->title, PPS_DOCUMENT_INFO_TITLE,
	                         rofd_metadata_get_title (metadata));
	ofd_metadata_set_string (info, &info->author, PPS_DOCUMENT_INFO_AUTHOR,
	                         rofd_metadata_get_author (metadata));
	ofd_metadata_set_string (info, &info->subject, PPS_DOCUMENT_INFO_SUBJECT,
	                         rofd_metadata_get_subject (metadata));

	/* The ABI has no producer field, so CreatorVersion is folded into the
	 * creator string ("nuonuo 1.0.0"); the properties dialog labels it
	 * "Source file created with". */
	{
		const char *creator = rofd_metadata_get_creator (metadata);
		const char *version = rofd_metadata_get_creator_version (metadata);

		if (creator != NULL && creator[0] != '\0') {
			g_autofree gchar *combined =
				version != NULL && version[0] != '\0'
					? g_strdup_printf ("%s %s", creator, version)
					: g_strdup (creator);

			ofd_metadata_set_string (info, &info->creator,
			                         PPS_DOCUMENT_INFO_CREATOR, combined);
		}
	}

	{
		GDateTime *created = ofd_metadata_parse_datetime (
			rofd_metadata_get_creation_date (metadata));
		GDateTime *modified = ofd_metadata_parse_datetime (
			rofd_metadata_get_modification_date (metadata));

		if (created != NULL)
			pps_document_info_take_created_datetime (info, created);
		if (modified != NULL)
			pps_document_info_take_modified_datetime (info, modified);
	}

	ofd_metadata_set_keywords (info, metadata);

	rofd_metadata_free (metadata);

	return info;
}
