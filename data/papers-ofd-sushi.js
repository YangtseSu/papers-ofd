// SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

/* Sushi is the space-bar previewer of GNOME Files, and it renders documents
 * in-process through Papers' own libdocument/libview. Which renderer it uses is
 * decided by content type: src/core/rendererSelector.js matches the file's type
 * against every renderer's `contentTypes`, and src/viewers/pdf.js derives its
 * own list from the MimeType line of org.gnome.Papers.desktop. Papers lists
 * application/ofd only when the backend lives in Papers' own tree (plan §5
 * phase 4), so out of tree the type finds no renderer and the preview falls back
 * to the file-information panel.
 *
 * Sushi loads plugins from its system plugin directory before the selector
 * builds its map (built-in → system → user, later wins), so adding the type to
 * the document renderer's list here is enough: the renderer then loads OFD files
 * exactly like PDFs, through PapersView.JobLoad, with this backend behind it.
 *
 * Removal criterion: delete this file (and its install rule) as soon as
 * application/ofd is part of Papers' own desktop MimeType list, i.e. after the
 * phase-4 move into libdocument/backend/ofd/. If Sushi ever changes how pdf.js
 * derives its content types, this plugin loads with an error and Sushi keeps
 * working with the fallback renderer — the preview of other types is unaffected.
 */

import * as DocumentRenderer from 'resource:///org/gnome/NautilusPreviewer/js/viewers/pdf.js';

const OFD_CONTENT_TYPE = 'application/ofd';

/* pdf.js:46 gates its load on this list, so the type has to be in it as well as
 * in the list the selector sees. */
if (!DocumentRenderer.contentTypes.includes(OFD_CONTENT_TYPE))
    DocumentRenderer.contentTypes.push(OFD_CONTENT_TYPE);

export const contentTypes = [OFD_CONTENT_TYPE];
export const Klass = DocumentRenderer.Klass;
