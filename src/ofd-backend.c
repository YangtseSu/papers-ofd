// SPDX-License-Identifier: GPL-2.0-or-later

/* Backend entry point: Papers looks this symbol up in the module named by the
 * `Module=` key of our .papers-backend descriptor (plan §3). */

#include <papers-document.h>

#include <rofd.h>

#include "ofd-document.h"

PPS_PUBLIC
GType pps_backend_query_type (void)
{
	/* The build-time constant and the loaded library must agree; a mismatch is
	 * a packaging error, and the detail makes it debuggable in the journal. */
	if (rofd_abi_version () != ROFD_ABI_VERSION)
		g_warning ("papers-ofd built for rofd ABI v%u but the library reports v%u",
		           (guint) ROFD_ABI_VERSION, (guint) rofd_abi_version ());

	return OFD_TYPE_DOCUMENT;
}
