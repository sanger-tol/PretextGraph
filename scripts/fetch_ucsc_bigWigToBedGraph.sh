#!/bin/bash
# Download UCSC's bigWigToBedGraph for this machine.
# The Kent userApps tree is not compiled: it needs MySQL, libpng, and OpenSSL,
# and UCSC does not build this utility from source for Apple Silicon.
# The macOS arm64 binary is linked against Homebrew's liblzma. If that
# library is not installed, this script takes liblzma from conda-forge,
# points the UCSC binary at the copy beside it, and ad-hoc signs both.

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 /path/to/bigWigToBedGraph" >&2
    exit 1
fi

dest=$1
dest_dir=$(cd "$(dirname "$dest")" && pwd)
dest="$dest_dir/$(basename "$dest")"

os=$(uname -s)
arch=$(uname -m)
case "${os}-${arch}" in
    Darwin-arm64) platform=macOSX.arm64 ;;
    Darwin-x86_64) platform=macOSX.x86_64 ;;
    Linux-x86_64) platform=linux.x86_64 ;;
    Linux-aarch64|Linux-arm64) platform=linux.aarch64.v492 ;;
    *)
        echo "No UCSC bigWigToBedGraph download for ${os} ${arch}" >&2
        exit 1
        ;;
esac

url="https://hgdownload.soe.ucsc.edu/admin/exe/${platform}/bigWigToBedGraph"

if [[ ! -x "$dest" ]]; then
    mkdir -p "$dest_dir"
    tmp="${dest}.partial"
    echo "Fetching UCSC bigWigToBedGraph (${platform})"
    curl -fL --retry 3 -o "$tmp" "$url"
    chmod 755 "$tmp"
    mv "$tmp" "$dest"
fi

if [[ "$os" != Darwin ]]; then
    exit 0
fi

lzma_ref=$(otool -L "$dest" | awk '/liblzma/{print $1; exit}')
if [[ -z "${lzma_ref}" || -f "${lzma_ref}" || "${lzma_ref}" == @loader_path/* ]]; then
    exit 0
fi

sign_macho() {
    local file=$1
    xattr -cr "$file" 2>/dev/null || true
    if codesign --force --sign - "$file"; then
        return 0
    fi
    local tmp
    tmp=$(mktemp)
    cp "$file" "$tmp"
    xattr -c "$tmp" 2>/dev/null || true
    codesign --force --sign - "$tmp"
    cp "$tmp" "$file"
    rm -f "$tmp"
}

sidecar="$dest_dir/liblzma.5.dylib"
if [[ ! -f "$sidecar" ]]; then
    found=""
    for candidate in \
        /opt/homebrew/opt/xz/lib/liblzma.5.dylib \
        /usr/local/opt/xz/lib/liblzma.5.dylib
    do
        if [[ -f "$candidate" ]]; then
            found=$candidate
            break
        fi
    done
    if [[ -z "$found" ]] && command -v brew >/dev/null 2>&1; then
        HOMEBREW_NO_AUTO_UPDATE=1 brew install xz
        for candidate in \
            /opt/homebrew/opt/xz/lib/liblzma.5.dylib \
            /usr/local/opt/xz/lib/liblzma.5.dylib
        do
            if [[ -f "$candidate" ]]; then
                found=$candidate
                break
            fi
        done
    fi
    if [[ -z "$found" ]]; then
        if ! command -v conda >/dev/null 2>&1; then
            echo "UCSC bigWigToBedGraph needs liblzma at ${lzma_ref}." >&2
            echo "Install it with: brew install xz" >&2
            exit 1
        fi
        prefix="$dest_dir/ucsc-liblzma"
        # shellcheck disable=SC1090
        source "$(conda info --base)/etc/profile.d/conda.sh"
        conda create -y -p "$prefix" -c conda-forge "liblzma=5.8.3"
        found=$(find "$prefix" -name 'liblzma.5.dylib' -print -quit)
    fi
    cp "$found" "$sidecar"
    install_name_tool -id @loader_path/liblzma.5.dylib "$sidecar"
fi

sign_macho "$sidecar"
install_name_tool -change "$lzma_ref" @loader_path/liblzma.5.dylib "$dest"
sign_macho "$dest"
