descriptor = 'org.gnome.Papers.OfdDocument.papers-backend'

# Keep a copy in the build tree: the documented dev setup copies the module and
# this descriptor from _build into a PPS_BACKENDS_DIR (README "Build and try it").
descriptor_in_build = configure_file(
  input: descriptor,
  output: descriptor,
  copy: true)

install_data(descriptor, install_dir: backend_dir)

install_data('papers-ofd.xml',
  install_dir: get_option('datadir') / 'mime' / 'packages')

install_data('papers-ofd.thumbnailer',
  install_dir: get_option('datadir') / 'thumbnailers')

install_data('org.gnome.Papers.Ofd.desktop',
  install_dir: get_option('datadir') / 'applications')

# Sushi (the space-bar previewer) renders documents through Papers' libraries but
# derives the renderer's content types from Papers' own desktop file, which does
# not list application/ofd out of tree; this plugin adds it (see the file header
# for the mechanism and the removal criterion). Sushi's system plugin directory
# is a build-time constant of Sushi itself, hence the option.
sushi_plugin_dir = get_option('sushi_plugin_dir')
if sushi_plugin_dir != ''
  install_data('papers-ofd-sushi.js', install_dir: sushi_plugin_dir)
endif
