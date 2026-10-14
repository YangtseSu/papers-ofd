fixture_generator = find_program('generate_fixture.py')

fixtures = custom_target('ofd-test-fixture',
  output: 'geometry-3pages.ofd',
  command: [fixture_generator, '@OUTPUT@'],
  build_by_default: true)

test_ofd_units = executable('test-ofd-units',
  files('test-ofd-units.c', '../src/ofd-units.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep],
  c_args: ['-DG_LOG_DOMAIN="papers-ofd"'])

test_ofd_document = executable('test-ofd-document',
  files('test-ofd-document.c'),
  include_directories: ofd_include,
  dependencies: [papers_dep, glib_dep, gio_dep])

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
