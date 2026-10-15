fs = import('fs')

fixture_generator = find_program('generate_fixture.py')

fixtures = custom_target('ofd-test-fixture',
  output: 'geometry-3pages.ofd',
  command: [fixture_generator, 'geometry', '@OUTPUT@'],
  build_by_default: true)

corner_marker_fixture = custom_target('ofd-test-fixture-corner-marker',
  output: 'corner-marker.ofd',
  command: [fixture_generator, 'corner-marker', '@OUTPUT@'],
  build_by_default: true)

origin_offset_fixture = custom_target('ofd-test-fixture-origin-offset',
  output: 'origin-offset.ofd',
  command: [fixture_generator, 'origin-offset', '@OUTPUT@'],
  build_by_default: true)

metadata_fixture = custom_target('ofd-test-fixture-metadata',
  output: 'metadata.ofd',
  command: [fixture_generator, 'metadata', '@OUTPUT@'],
  build_by_default: true)

latency_fixture = custom_target('ofd-test-fixture-latency-300',
  output: 'latency-300pages.ofd',
  command: [fixture_generator, 'latency', '@OUTPUT@', '300'],
  build_by_default: true)

text_fixture = custom_target('ofd-test-fixture-text',
  output: 'text-2pages.ofd',
  command: [fixture_generator, 'text', '@OUTPUT@'],
  build_by_default: true)

scan_fixture = custom_target('ofd-test-fixture-scan',
  output: 'scan-1page.ofd',
  command: [fixture_generator, 'scan', '@OUTPUT@'],
  build_by_default: true)

links_fixture = custom_target('ofd-test-fixture-links',
  output: 'links-2pages.ofd',
  command: [fixture_generator, 'links', '@OUTPUT@'],
  build_by_default: true)

broken_page_fixture = custom_target('ofd-test-fixture-broken-page',
  output: 'broken-page-2pages.ofd',
  command: [fixture_generator, 'broken-page', '@OUTPUT@'],
  build_by_default: true)

origin_text_fixture = custom_target('ofd-test-fixture-origin-text',
  output: 'origin-text.ofd',
  command: [fixture_generator, 'origin-text', '@OUTPUT@'],
  build_by_default: true)

malformed_zip_fixture = custom_target('ofd-test-fixture-malformed-zip',
  output: 'malformed-zip.ofd',
  command: [fixture_generator, 'malformed-zip', '@OUTPUT@'],
  build_by_default: true)

poppler_glib_dep = dependency('poppler-glib')
gtk_dep = dependency('gtk4')
papers_view_dep = dependency('papers-view-4.0', version: ['>= 51', '< 52'])

test_ofd_units = executable('test-ofd-units',
  files('test-ofd-units.c', '../src/ofd-units.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep],
  c_args: ['-DG_LOG_DOMAIN="papers-ofd"'])

test_ofd_document = executable('test-ofd-document',
  files('test-ofd-document.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep])

test_ofd_render = executable('test-ofd-render',
  files('test-ofd-render.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, papers_view_dep, glib_dep, gio_dep, rofd_dep, poppler_glib_dep])

test_ofd_print_file = executable('test-ofd-print-file',
  files('ofd-print-file.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, papers_view_dep, glib_dep, gio_dep, poppler_glib_dep, gtk_dep])

ofd_latency = executable('ofd-latency',
  files('ofd-latency.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep])

test_ofd_metadata = executable('test-ofd-metadata',
  files('test-ofd-metadata.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep])

test_ofd_text = executable('test-ofd-text',
  files('test-ofd-text.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep, rofd_dep])

test_ofd_links = executable('test-ofd-links',
  files('test-ofd-links.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep, rofd_dep])

test_ofd_search = executable('test-ofd-search',
  files('test-ofd-search.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, papers_view_dep, glib_dep, gio_dep, gtk_dep])

test_ofd_errors = executable('test-ofd-errors',
  files('test-ofd-errors.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep])

test_ofd_fd = executable('test-ofd-fd',
  files('test-ofd-fd.c', 'ofd-test-utils.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, papers_view_dep, glib_dep, gio_dep])

test('ofd-units', test_ofd_units, timeout: 30)

test('ofd-document',
  test_ofd_document,
  args: [ofd_module, descriptor_in_build, fixtures, files('../data/papers-ofd.xml')],
  timeout: 60)

test('ofd-discovery-failure',
  test_ofd_document,
  args: ['--expect-discovery-failure',
         files('../data/papers-ofd.xml'),
         fixtures],
  timeout: 60)

test('ofd-render',
  test_ofd_render,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         corner_marker_fixture, origin_offset_fixture, metadata_fixture],
  timeout: 120)

test('ofd-print-file',
  test_ofd_print_file,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         metadata_fixture, 'phase1-print-to-file.pdf'],
  timeout: 120)

test('ofd-metadata',
  test_ofd_metadata,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         metadata_fixture, fixtures],
  timeout: 60)

test('ofd-text',
  test_ofd_text,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         text_fixture, scan_fixture, broken_page_fixture, origin_text_fixture],
  timeout: 120)

test('ofd-links',
  test_ofd_links,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         links_fixture, text_fixture, origin_text_fixture],
  timeout: 120)

test('ofd-search',
  test_ofd_search,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         text_fixture],
  timeout: 120)

test('ofd-errors',
  test_ofd_errors,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         malformed_zip_fixture, text_fixture],
  timeout: 60)

test('ofd-fd',
  test_ofd_fd,
  args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
         text_fixture, metadata_fixture],
  timeout: 120)

# The previewer acceptance needs Sushi, which is where the space bar lives: its
# own selector and renderer are loaded with our plugin and made to load a
# fixture. Runs only where Sushi is installed (and gjs from Sushi's closure). The
# test stages the module, its descriptor and our MIME xml itself, so it needs
# neither an installed package nor PPS_BACKENDS_DIR in the invoking environment.
gjs = find_program('gjs', required: false)
sushi_resource = '/usr/share/sushi/org.gnome.NautilusPreviewer.src.gresource'
if gjs.found() and fs.exists(sushi_resource)
  test('ofd-sushi-plugin',
    gjs,
    args: ['-m', files('test-sushi-plugin.js'),
           ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
           files('../data/papers-ofd-sushi.js'),
           text_fixture, sushi_resource],
    timeout: 120)
else
  message('gjs or Sushi missing: skipping ofd-sushi-plugin')
endif

rofd_src = get_option('rofd_src')
if rofd_src != ''
  oracle_fixture = rofd_src / 'learning' / 'test.ofd'
  oracle_reference = rofd_src / 'tests' / 'fixtures' / 'reference' / 'invoice-phase3.png'
  if fs.exists(oracle_fixture) and fs.exists(oracle_reference)
    test_ofd_oracle = executable('test-ofd-oracle',
      files('test-ofd-oracle.c', 'ofd-test-utils.c'),
      include_directories: ofd_include,
      dependencies: [papers_dep, glib_dep, gio_dep])

    test('ofd-oracle',
      test_ofd_oracle,
      args: [ofd_module, descriptor_in_build, files('../data/papers-ofd.xml'),
             oracle_fixture, oracle_reference],
      timeout: 120)
  else
    warning('rofd_src is set but the oracle fixture or reference is missing; skipping ofd-oracle')
  endif
endif
