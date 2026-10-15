// SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

/* Phase-3 previewer acceptance, headless: Sushi -- the space-bar previewer of
 * GNOME Files -- renders documents in-process through Papers' libdocument and
 * libview, and picks a renderer per content type. Its document renderer
 * (js/viewers/pdf.js) claims only the MIME types of org.gnome.Papers.desktop,
 * which out of tree does not include application/ofd, so data/papers-ofd-sushi.js
 * adds the type from Sushi's system plugin directory.
 *
 * This test loads Sushi's own selector and renderer with the plugin, then makes
 * the renderer load a fixture: exactly what the space bar does, with this
 * backend behind it. It needs gjs, the Sushi installation and a display for the
 * live load (exit 77 skips the load part, which Meson reports as skipped).
 *
 * The load runs through Papers' document factory, which takes the backend
 * directory from PPS_BACKENDS_DIR and the type from the MIME database. A package
 * build has neither our module installed nor, on a system without another
 * application/ofd rule, the MIME database entry for the type, so the test stages
 * both into a temporary directory first -- the same staging the C tests do
 * (tests/ofd-test-utils.c). Without it the load fails with "File type Zip
 * archive (application/zip) is not supported": the fixture is a zip, and only
 * our MIME xml makes it an OFD document. */

import Gio from 'gi://Gio';
import GLib from 'gi://GLib';
import Gtk from 'gi://Gtk?version=4.0';
import System from 'system';

const [modulePath, descriptorPath, mimeXmlPath,
       pluginPath, fixturePath, resourcePath] = ARGV;
const SUSHI_SRC = 'resource:///org/gnome/NautilusPreviewer/js/';
const OFD = 'application/ofd';

/* Sushi sets this up in its own main; the document renderer formats the page
 * label with it. */
globalThis._ = s => s;

const fail = message => {
    printerr(`FAIL: ${message}`);
    System.exit(1);
};

if (!GLib.file_test(resourcePath, GLib.FileTest.EXISTS)) {
    print(`no Sushi installation (${resourcePath}); skipping`);
    System.exit(77);
}

/* Both staged before anything can query a content type: GIO builds its MIME
 * database once, on the first lookup, and Papers reads PPS_BACKENDS_DIR when the
 * renderer initializes it below. */
const stagingDir = GLib.dir_make_tmp('papers-ofd-sushi-XXXXXX');

const copyFile = (source, target) => {
    /* gjs raises on both calls instead of returning FALSE, so report them the
     * same way as every other failure here. */
    try {
        const [, contents] = GLib.file_get_contents(source);
        GLib.file_set_contents(target, contents);
    } catch (error) {
        fail(`cannot stage ${source} as ${target}: ${error.message}`);
    }
};

const stageMimeDatabase = () => {
    const dataDir = GLib.build_filenamev([stagingDir, 'xdg']);
    const mimeDir = GLib.build_filenamev([dataDir, 'mime']);
    const packagesDir = GLib.build_filenamev([mimeDir, 'packages']);

    if (GLib.mkdir_with_parents(packagesDir, 0o755) != 0)
        fail(`cannot create ${packagesDir}`);
    copyFile(mimeXmlPath, GLib.build_filenamev([packagesDir, 'papers-ofd.xml']));

    /* Keep the system directories: only application/ofd is ours to add here. Set
     * before update-mime-database so it does not warn about a tree outside the
     * search path. */
    GLib.setenv('XDG_DATA_DIRS', `${dataDir}:/usr/share:/usr/local/share`, true);

    try {
        Gio.Subprocess.new(['update-mime-database', mimeDir],
                           Gio.SubprocessFlags.STDOUT_SILENCE).wait_check(null);
    } catch (error) {
        fail(`update-mime-database ${mimeDir} failed: ${error.message}`);
    }
};

const stageBackendDir = () => {
    const dir = GLib.build_filenamev([stagingDir, 'backends']);

    if (GLib.mkdir_with_parents(dir, 0o755) != 0)
        fail(`cannot create ${dir}`);

    /* The descriptor says Module=ofddocument; GModule resolves that to
     * libofddocument.so inside PPS_BACKENDS_DIR. */
    copyFile(modulePath, GLib.build_filenamev([dir, 'libofddocument.so']));
    copyFile(descriptorPath,
             GLib.build_filenamev([dir, GLib.path_get_basename(descriptorPath)]));

    GLib.setenv('PPS_BACKENDS_DIR', dir, true);
};

const removeTree = path => {
    const file = Gio.File.new_for_path(path);
    const type = file.query_file_type(Gio.FileQueryInfoFlags.NOFOLLOW_SYMLINKS, null);

    if (type === Gio.FileType.DIRECTORY) {
        const children = file.enumerate_children('standard::name',
                                                 Gio.FileQueryInfoFlags.NOFOLLOW_SYMLINKS, null);
        for (let info = children.next_file(null); info !== null; info = children.next_file(null))
            removeTree(GLib.build_filenamev([path, info.get_name()]));
        children.close(null);
    }

    if (type !== Gio.FileType.UNKNOWN)
        file.delete(null);
};

stageMimeDatabase();
stageBackendDir();

const resource = Gio.Resource.load(resourcePath);
Gio.resources_register(resource);

const Pdf = await import(`${SUSHI_SRC}viewers/pdf.js`);
const claimedBefore = Pdf.contentTypes.includes(OFD);

/* Sushi imports plugins while the selector is being loaded, so importing the
 * plugin here is the same sequence. */
await import(GLib.filename_to_uri(pluginPath, null));

if (!Pdf.contentTypes.includes(OFD))
    fail('the document renderer does not claim application/ofd after the plugin');
if (claimedBefore)
    print('note: the renderer already claimed application/ofd; the plugin is no longer needed');

const {selectRenderer} = await import(`${SUSHI_SRC}core/rendererSelector.js`);
const {FallbackRenderer} = await import(`${SUSHI_SRC}viewers/fallback.js`);
const rendererClass = selectRenderer(OFD);

if (rendererClass === FallbackRenderer)
    fail('the selector still falls back for application/ofd');
if (rendererClass !== Pdf.Klass)
    fail('the selector does not pick the Papers-backed document renderer');
if (selectRenderer('application/pdf') !== Pdf.Klass)
    fail('application/pdf no longer routes to the document renderer');

print('the selector routes application/ofd to the Papers-backed document renderer');

if (!Gtk.init_check()) {
    print('no display for GTK: skipping the live load');
    System.exit(77);
}

/* The renderer loads the document through PapersView.JobLoad, i.e. through this
 * backend; "ready" means the load job finished with a document. */
const file = Gio.File.new_for_path(fixturePath);
const info = file.query_info('standard::*', Gio.FileQueryInfoFlags.NONE, null);
const renderer = new Pdf.Klass(file, info, {});
const loop = new GLib.MainLoop(null, false);
let pages = -1;

renderer.connect('ready', () => {
    pages = renderer._model.document.get_n_pages();
    loop.quit();
});
renderer.connect('failed', (_, error) => {
    printerr(`FAIL: the renderer failed: ${error.message}`);
    pages = -2;
    loop.quit();
});
GLib.timeout_add_seconds(GLib.PRIORITY_DEFAULT, 30, () => {
    printerr('FAIL: timed out waiting for the renderer');
    pages = -3;
    loop.quit();
    return GLib.SOURCE_REMOVE;
});

loop.run();

if (pages != 2)
    fail(`the renderer reported ${pages} pages instead of the fixture's 2`);

print('Sushi\'s document renderer loaded the OFD fixture: 2 pages');

/* Leaving the staging tree behind is not worth failing the run for. */
try {
    removeTree(stagingDir);
} catch (error) {
    print(`note: could not remove ${stagingDir}: ${error.message}`);
}

System.exit(0);
