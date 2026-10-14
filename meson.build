project('papers-ofd', 'c',
  version: '0.1.0',
  license: 'GPL-2.0-or-later',
  meson_version: '>= 1.0',
  default_options: [
    'c_std=c11',
    'warning_level=2',
    'buildtype=release',
  ])

# Papers marks its headers unstable for out-of-tree consumers; accepting that
# contract is what makes the public headers usable here (plan §3).
add_project_arguments(
  '-DI_KNOW_THE_PAPERS_LIBS_ARE_UNSTABLE_AND_HAVE_TALKED_WITH_THE_AUTHORS',
  # POSIX/GNU interfaces (symlink, /proc/self/fd for the fd loading path)
  '-D_GNU_SOURCE',
  language: 'c')

cc = meson.get_compiler('c')

papers_dep = dependency('papers-document-4.0', version: ['>= 51', '< 52'])
glib_dep = dependency('glib-2.0', version: '>= 2.75')
gio_dep = dependency('gio-2.0')
cairo_dep = dependency('cairo')

backend_dir = get_option('papers_backend_dir')
if backend_dir == ''
  backend_dir = papers_dep.get_variable(pkgconfig: 'backenddir',
                                         default_value: '/usr/lib/papers/6/backends')
endif

# The ABI comes from rofd-ffi.pc when it is installed (our AUR package ships it);
# otherwise the caller points at a build tree. Only the .pc carries a version, so
# the minimum documented in README.md (0.4.2, A19) can be enforced there and is
# merely warned about for explicit directories.
rofd_dep = dependency('rofd-ffi', version: ['>= 0.4.2'], required: false)
if not rofd_dep.found()
  rofd_libdir = get_option('rofd_libdir')
  rofd_includedir = get_option('rofd_includedir')
  if rofd_libdir == '' or rofd_includedir == ''
    error('rofd-ffi.pc not found: install librofd-ffi, or pass ' +
          '-Drofd_libdir=<dir> -Drofd_includedir=<dir> (see scripts/dev-env.sh)')
  endif
  rofd_dep = declare_dependency(
    include_directories: include_directories(rofd_includedir, is_system: true),
    dependencies: cc.find_library('rofd_ffi', dirs: [rofd_libdir], required: true))
  message('rofd ABI taken from -Drofd_libdir/-Drofd_includedir; ' +
          'rofd >= 0.4.2 is required but cannot be checked without rofd-ffi.pc')
endif

ofd_conf = configuration_data()
# rofd.h includes <cairo.h>, so even the probe needs cairo's include path; the
# installed .pc declares the same through Requires: cairo.
if get_option('rofd_page_size') == 'disabled'
  message('rofd_page_size=disabled: always fetching pages to size them')
elif cc.has_function('rofd_document_get_page_size_mm',
                     prefix: '#include <rofd.h>',
                     dependencies: [rofd_dep, cairo_dep])
  ofd_conf.set('HAVE_ROFD_PAGE_SIZE', 1)
  message('rofd_document_get_page_size_mm present: metadata-only page sizes')
else
  message('rofd_document_get_page_size_mm absent: falling back to per-page fetch')
endif

subdir('src')
subdir('data')
if get_option('tests')
  subdir('tests')
endif
