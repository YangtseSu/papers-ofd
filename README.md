# papers-ofd

OFD (GB/T 33190-2016) backend for [GNOME Papers](https://github.com/GNOME/papers).

Out-of-tree: installs a single shared module plus one desktop file into the
Papers backends directory. No Papers fork required.

## Status

Planning done; implementation not started. See [docs/plan.md](docs/plan.md).

## How it works

`libofddocument.so` implements `PpsDocument` on top of rofd's stable C ABI
([rofd-ffi](https://github.com/linuxdeepin/rofd)); Papers discovers it at
runtime by scanning its backends directory for `*.papers-backend` desktop files.

## License

GPL-2.0-or-later (see `COPYING`), matching Papers; links rofd (LGPL-2.1+).
