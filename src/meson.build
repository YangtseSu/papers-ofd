ofd_config = configure_file(
  output: 'ofd-config.h',
  configuration: ofd_conf)

ofd_include = include_directories('.')

ofd_module = shared_module('ofddocument',
  files(
    'ofd-backend.c',
    'ofd-document.c',
    'ofd-links.c',
    'ofd-metadata.c',
    'ofd-outline.c',
    'ofd-page.c',
    'ofd-render.c',
    'ofd-text.c',
    'ofd-units.c',
  ),
  ofd_config,
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep, rofd_dep],
  gnu_symbol_visibility: 'hidden',
  c_args: ['-DG_LOG_DOMAIN="papers-ofd"'],
  link_args: ['-Wl,-z,now', '-Wl,--no-undefined'],
  install: true,
  install_dir: backend_dir)
