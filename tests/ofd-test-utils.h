// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <papers-document.h>

G_BEGIN_DECLS

/* Test staging shared by the phase-1 test programs.
 *
 * application/ofd is not registered on a machine that has neither our package
 * nor another OFD package installed, and OFD files are zip archives, so
 * detection needs our MIME definition: it is installed into a scratch
 * XDG_DATA_DIRS tree with update-mime-database, exactly as plan §7 prescribes
 * for validating the shipped file. The backend directory holds the built module
 * and its descriptor, and PPS_BACKENDS_DIR replaces the system set. */

/* Installs mime_xml_path into a temporary XDG tree and prepends it to
 * XDG_DATA_DIRS; calls update-mime-database. Runs once per process and returns
 * the staging directory (never NULL; aborts the test on failure). */
gchar *ofd_test_prepare_mime_database (const gchar *mime_xml_path);

/* Temporary directory with libofddocument.so -> module_path and a copy of the
 * descriptor, i.e. what PPS_BACKENDS_DIR must point at. The caller owns the
 * returned path. */
gchar *ofd_test_stage_backend_dir (const gchar *module_path,
                                   const gchar *descriptor_path);

/* Loads fixture_path through Papers' own factory: stages the MIME database and
 * the backend directory (once per process), initializes Papers and runs
 * pps_document_load. Aborts the test when the module cannot load the fixture. */
PpsDocument *ofd_test_load_fixture (const gchar *module_path,
                                    const gchar *descriptor_path,
                                    const gchar *mime_xml_path,
                                    const gchar *fixture_path);

/* The staging half of ofd_test_load_fixture, for callers that time the load
 * themselves: installs the MIME database, points PPS_BACKENDS_DIR at the staged
 * backend directory and calls pps_init. Runs once per process. */
void ofd_test_stage_all (const gchar *module_path,
                         const gchar *descriptor_path,
                         const gchar *mime_xml_path);

/* Removes the staging trees; call at the end of main. */
void ofd_test_cleanup (void);

G_END_DECLS
