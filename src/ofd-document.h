// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>

G_BEGIN_DECLS

#define OFD_TYPE_DOCUMENT (ofd_document_get_type ())

G_DECLARE_FINAL_TYPE (OfdDocument, ofd_document, OFD, DOCUMENT, PpsDocument)

G_END_DECLS
