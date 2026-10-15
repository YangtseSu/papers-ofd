#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Yangtse Su <yangtsesu@gmail.com>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Locate the out-of-tree dependencies of papers-ofd on this machine.
#
#   . scripts/dev-env.sh    # export PAPERS_SRC, ROFD_SRC, ROFD_INCLUDEDIR, ROFD_LIBDIR
#   scripts/dev-env.sh      # print the resolution table; non-zero exit when the ABI is unresolved
#   scripts/dev-env.sh -h   # this help
#
# The ABI comes from the installed librofd-ffi package or from a built checkout;
# neither checkout is a build requirement. $ROFD_SRC is needed to carry or
# re-verify the rofd changes we ship (patches/README.md) and to run rofd's own
# test suite; $PAPERS_SRC only for grepping and for phase 4 — the plan cites
# Papers files upstream at
# https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/. Resolution order per
# checkout:
#
#   1. $PAPERS_SRC / $ROFD_SRC from the environment
#   2. PAPERS_SRC= / ROFD_SRC= in the untracked ./local.conf (shell syntax;
#      relative values are resolved against the repository root)
#   3. sibling defaults ../papers and ../rofd
#
# Identity is validated, not assumed: a directory that exists but lacks the
# expected marker is reported as a wrong repository (../rofd may be
# office-rs/rofd, a different OFD project with the same name) instead of being used.
#
# The ABI pair (header + library) comes from, in order: a built $ROFD_SRC, then
# pkg-config (rofd-ffi.pc, installed by our AUR package), then /usr/include +
# /usr/lib. Setting $ROFD_INCLUDEDIR / $ROFD_LIBDIR overrides either side.

_papers_ofd_self=${BASH_SOURCE[0]:-$0}
_papers_ofd_root=$(cd -- "$(dirname -- "$_papers_ofd_self")/.." && pwd)
_papers_ofd_sourced=0
[ "$_papers_ofd_self" != "$0" ] && _papers_ofd_sourced=1

_papers_ofd_usage() {
    awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' \
        "$_papers_ofd_self"
}

_papers_ofd_warn() {
    printf 'dev-env: %s\n' "$*" >&2
}

_papers_ofd_abs() {
    case "$1" in
        /*) realpath -m -- "$1" ;;
        *)  realpath -m -- "$_papers_ofd_root/$1" ;;
    esac
}

_papers_ofd_origin() {
    [ -d "$1/.git" ] || return 0
    git -C "$1" remote get-url origin 2>/dev/null || true
}

_papers_ofd_row() {
    printf '  %-16s %-46s %s\n' "$1" "$2" "$3"
}

case "${1:-}" in
    -h|--help)
        _papers_ofd_usage
        exit 0
        ;;
esac

# 1. local configuration (untracked, see .gitignore)
if [ -r "$_papers_ofd_root/local.conf" ]; then
    # shellcheck source=/dev/null
    . "$_papers_ofd_root/local.conf"
fi

# 2. checkout locations
PAPERS_SRC=$(_papers_ofd_abs "${PAPERS_SRC:-../papers}")
ROFD_SRC=$(_papers_ofd_abs "${ROFD_SRC:-../rofd}")

if [ -f "$PAPERS_SRC/libdocument/pps-document.h" ]; then
    PAPERS_SRC_STATUS=ok
elif [ ! -d "$PAPERS_SRC" ]; then
    PAPERS_SRC_STATUS=missing
elif [ -e "$PAPERS_SRC/libdocument" ] || [ -e "$PAPERS_SRC/shell" ]; then
    PAPERS_SRC_STATUS=wrong-repository
else
    PAPERS_SRC_STATUS=incomplete
fi

if [ -f "$ROFD_SRC/crates/rofd-ffi/include/rofd.h" ]; then
    ROFD_SRC_STATUS=ok
elif [ ! -d "$ROFD_SRC" ]; then
    ROFD_SRC_STATUS=missing
elif [ -e "$ROFD_SRC/crates" ]; then
    ROFD_SRC_STATUS=wrong-repository
else
    ROFD_SRC_STATUS=incomplete
fi

# 3. ABI header + library: explicit override > built checkout > pkg-config > /usr guess
_papers_ofd_inc_override=${ROFD_INCLUDEDIR:-}
_papers_ofd_lib_override=${ROFD_LIBDIR:-}

if [ "$ROFD_SRC_STATUS" = ok ] && [ -e "$ROFD_SRC/target/release/librofd_ffi.so" ]; then
    ROFD_ABI_SOURCE=checkout
    ROFD_INCLUDEDIR="$ROFD_SRC/crates/rofd-ffi/include"
    ROFD_LIBDIR="$ROFD_SRC/target/release"
elif pkg-config --exists rofd-ffi 2>/dev/null; then
    ROFD_ABI_SOURCE=pkg-config
    ROFD_INCLUDEDIR=$(pkg-config --variable=includedir rofd-ffi)
    ROFD_LIBDIR=$(pkg-config --variable=libdir rofd-ffi)
    [ -n "$ROFD_INCLUDEDIR" ] || ROFD_INCLUDEDIR=/usr/include
    [ -n "$ROFD_LIBDIR" ] || ROFD_LIBDIR=/usr/lib
else
    ROFD_ABI_SOURCE=installed
    ROFD_INCLUDEDIR=/usr/include
    _papers_ofd_lib=$(cc -print-file-name=librofd_ffi.so.0 2>/dev/null || true)
    case "$_papers_ofd_lib" in
        /*) ROFD_LIBDIR=$(realpath -m -- "$(dirname -- "$_papers_ofd_lib")") ;;
        *)  ROFD_LIBDIR=/usr/lib ;;
    esac
fi

# an explicitly set variable wins over the tier above, independently per side
if [ -n "$_papers_ofd_inc_override" ] || [ -n "$_papers_ofd_lib_override" ]; then
    ROFD_ABI_SOURCE=override
    [ -n "$_papers_ofd_inc_override" ] &&
        ROFD_INCLUDEDIR=$(_papers_ofd_abs "$_papers_ofd_inc_override")
    [ -n "$_papers_ofd_lib_override" ] &&
        ROFD_LIBDIR=$(_papers_ofd_abs "$_papers_ofd_lib_override")
fi

ROFD_HEADER="$ROFD_INCLUDEDIR/rofd.h"
ROFD_LIBRARY="$ROFD_LIBDIR/librofd_ffi.so.0"

ROFD_ABI_VERSION=$(sed -nE 's/^#define[[:space:]]+ROFD_ABI_VERSION[[:space:]]+([0-9]+).*/\1/p' \
    "$ROFD_HEADER" 2>/dev/null | head -n 1)

if nm -D --defined-only -- "$ROFD_LIBRARY" 2>/dev/null | grep -q 'rofd_document_get_page_size_mm$'; then
    ROFD_PAGE_SIZE=present
else
    ROFD_PAGE_SIZE=absent
fi

# 4. status summary
_papers_ofd_header_status=missing
[ -n "$ROFD_ABI_VERSION" ] && _papers_ofd_header_status="ok (rofd.h, ROFD_ABI_VERSION $ROFD_ABI_VERSION)"
[ -f "$ROFD_HEADER" ] && [ -z "$ROFD_ABI_VERSION" ] && _papers_ofd_header_status="ok (rofd.h, no ROFD_ABI_VERSION)"

_papers_ofd_lib_status=missing
if [ -e "$ROFD_LIBRARY" ]; then
    _papers_ofd_lib_status="ok (librofd_ffi.so.0, page-size fast path $ROFD_PAGE_SIZE)"
fi

_papers_ofd_src_status() {
    case "$1" in
        ok) printf 'ok (%s)' "$2" ;;
        missing) printf 'missing' ;;
        incomplete) printf 'incomplete checkout (no %s)' "$2" ;;
        wrong-repository)
            _papers_ofd_note=$(_papers_ofd_origin "$3")
            if [ -n "$_papers_ofd_note" ]; then
                printf 'wrong repository (origin %s)' "$_papers_ofd_note"
            else
                printf 'wrong repository (%s)' "$2"
            fi
            ;;
    esac
}

if [ "$_papers_ofd_sourced" = 1 ]; then
    _papers_ofd_problems=
    _papers_ofd_add_problem() {
        _papers_ofd_problems="${_papers_ofd_problems:+$_papers_ofd_problems; }$1"
    }
    [ "$PAPERS_SRC_STATUS" = ok ] ||
        _papers_ofd_add_problem "PAPERS_SRC ${PAPERS_SRC_STATUS//wrong-repository/wrong repository}"
    [ "$ROFD_SRC_STATUS" = ok ] ||
        _papers_ofd_add_problem "ROFD_SRC ${ROFD_SRC_STATUS//wrong-repository/wrong repository}"
    [ -f "$ROFD_HEADER" ] || _papers_ofd_add_problem "no rofd.h in $ROFD_INCLUDEDIR"
    [ -e "$ROFD_LIBRARY" ] || _papers_ofd_add_problem "no librofd_ffi.so.0 in $ROFD_LIBDIR"
    if [ -n "$_papers_ofd_problems" ]; then
        _papers_ofd_warn "$_papers_ofd_problems — run scripts/dev-env.sh for details"
    fi
else
    printf 'papers-ofd dependencies (root: %s)\n\n' "$_papers_ofd_root"
    _papers_ofd_row PAPERS_SRC "$PAPERS_SRC" "$(_papers_ofd_src_status "$PAPERS_SRC_STATUS" libdocument/pps-document.h "$PAPERS_SRC")"
    _papers_ofd_row ROFD_SRC "$ROFD_SRC" "$(_papers_ofd_src_status "$ROFD_SRC_STATUS" crates/rofd-ffi/include/rofd.h "$ROFD_SRC")"
    _papers_ofd_row ROFD_INCLUDEDIR "$ROFD_INCLUDEDIR" "$_papers_ofd_header_status"
    _papers_ofd_row ROFD_LIBDIR "$ROFD_LIBDIR" "$_papers_ofd_lib_status"
    case "$ROFD_ABI_SOURCE" in
        override)  _papers_ofd_abi_note="environment override" ;;
        checkout)  _papers_ofd_abi_note="checkout $ROFD_SRC" ;;
        pkg-config) _papers_ofd_abi_note="rofd-ffi.pc $(pkg-config --modversion rofd-ffi 2>/dev/null)" ;;
        *)         _papers_ofd_abi_note="installed librofd-ffi package (no .pc)" ;;
    esac
    _papers_ofd_row ABI_SOURCE "$ROFD_ABI_SOURCE" "$_papers_ofd_abi_note"

    if [ "$PAPERS_SRC_STATUS" != ok ]; then
        if [ "$PAPERS_SRC_STATUS" = wrong-repository ]; then
            printf '\n$PAPERS_SRC exists but is not a GNOME Papers checkout.\n'
        fi
        printf '\nPapers sources are optional: the module builds against the installed\n'
        printf 'papers-document-4.0 headers, and the plan cites Papers files at\n'
        printf 'https://gitlab.gnome.org/GNOME/papers/-/blob/51.1/<path>. A checkout is\n'
        printf 'only a grepping convenience and a phase-4 prerequisite. To get one:\n'
        printf '  git clone https://gitlab.gnome.org/GNOME/papers %s\n' "$PAPERS_SRC"
    fi
    case "$ROFD_SRC_STATUS" in
        wrong-repository)
            printf '\n$ROFD_SRC is not the rofd ABI repository: no\n'
            printf 'crates/rofd-ffi/include/rofd.h. Point $ROFD_SRC at the ABI checkout, or\n'
            printf 'leave it unset to use the installed package. Expected checkout, e.g.\n'
            printf '  git clone https://github.com/YangtseSu/rofd %s\n' "$ROFD_SRC"
            ;;
        incomplete)
            printf '\n$ROFD_SRC has no crates/rofd-ffi/include/rofd.h (unfinished clone or\n'
            printf 'empty directory), so the ABI comes from the installed librofd-ffi\n'
            printf 'package. Expected checkout:\n'
            printf '  git clone https://github.com/YangtseSu/rofd %s\n' "$ROFD_SRC"
            ;;
        missing)
            printf '\nrofd sources are needed to re-verify the rofd citations in docs/plan.md\n'
            printf 'and to maintain the carried rofd changes. Point $ROFD_SRC (or ROFD_SRC= in local.conf) at\n'
            printf 'a checkout of https://github.com/YangtseSu/rofd (the fork; the carried\n'
            printf 'changes live on its release/* branches — patches/README.md), or leave it unset.'
            ;;
    esac
    if [ "$ROFD_SRC_STATUS" = ok ] && [ "$ROFD_ABI_SOURCE" = installed ]; then
        printf '\n$ROFD_SRC is present but not built; using the installed librofd-ffi\n'
        printf 'package for the ABI. To build the checkout instead:\n'
        printf '  (cd %s && cargo build --release --offline -p rofd-ffi)\n' "$ROFD_SRC"
    fi
    if [ "$ROFD_SRC_STATUS" = ok ] && [ "$ROFD_ABI_SOURCE" = checkout ] &&
       [ ! -e "$ROFD_LIBRARY" ]; then
        printf '\nMissing SONAME symlink; papers would fail with "Failed to load backend":\n'
        printf '  ln -sf librofd_ffi.so %s/librofd_ffi.so.0\n' "$ROFD_LIBDIR"
    fi
    printf '\nMeson setup:\n  meson setup _build -Drofd_libdir=%s -Drofd_includedir=%s\n' \
        "$ROFD_LIBDIR" "$ROFD_INCLUDEDIR"
fi

export PAPERS_SRC ROFD_SRC ROFD_INCLUDEDIR ROFD_LIBDIR
export PAPERS_SRC_STATUS ROFD_SRC_STATUS ROFD_ABI_SOURCE

if [ "$_papers_ofd_sourced" = 0 ]; then
    if [ -f "$ROFD_HEADER" ] && [ -e "$ROFD_LIBRARY" ]; then
        exit 0
    fi
    exit 1
fi
