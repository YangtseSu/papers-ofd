// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ofd-document.h"

G_BEGIN_DECLS

/* PpsDocumentText, PpsSelection and PpsDocumentFind, implemented on OfdDocument
 * (plan §3) and registered in ofd-document.c. Every vfunc of the three
 * interfaces is present: Papers dispatches PpsSelection::get_selected_text and
 * PpsDocumentFind::find_text without a NULL check (A9). */

gchar *ofd_text_get_text (PpsDocumentText *document_text,
                          PpsPage *page);
gboolean ofd_text_get_text_layout (PpsDocumentText *document_text,
                                   PpsPage *page,
                                   PpsRectangle **areas,
                                   guint *n_areas);
gchar *ofd_text_get_text_in_area (PpsDocumentText *document_text,
                                  PpsPage *page,
                                  PpsRectangle *area);
cairo_region_t *ofd_text_get_text_mapping (PpsDocumentText *document_text,
                                           PpsPage *page);
PangoAttrList *ofd_text_get_text_attrs (PpsDocumentText *document_text,
                                        PpsPage *page);

void ofd_selection_render_selection (PpsSelection *selection,
                                     PpsRenderContext *rc,
                                     cairo_surface_t **surface,
                                     PpsRectangle *points,
                                     PpsRectangle *old_points,
                                     PpsSelectionStyle style,
                                     GdkRGBA *text,
                                     GdkRGBA *base);
gchar *ofd_selection_get_selected_text (PpsSelection *selection,
                                        PpsPage *page,
                                        PpsSelectionStyle style,
                                        PpsRectangle *points);
cairo_region_t *ofd_selection_get_selection_region (PpsSelection *selection,
                                                    PpsRenderContext *rc,
                                                    PpsSelectionStyle style,
                                                    PpsRectangle *points);

PpsFindOptions ofd_find_get_supported_options (PpsDocumentFind *document_find);
GList *ofd_find_text (PpsDocumentFind *document_find,
                      PpsPage *page,
                      const gchar *text,
                      PpsFindOptions options);

G_END_DECLS
