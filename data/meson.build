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
