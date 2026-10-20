#!/bin/bash
# Copy the liblzma sidecar, if the UCSC macOS binary needed one.
set -euo pipefail

if [[ -f "${MESON_BUILD_ROOT}/liblzma.5.dylib" ]]; then
    mkdir -p "${MESON_INSTALL_DESTDIR_PREFIX}/bin"
    cp "${MESON_BUILD_ROOT}/liblzma.5.dylib" "${MESON_INSTALL_DESTDIR_PREFIX}/bin/liblzma.5.dylib"
fi
