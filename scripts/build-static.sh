#!/bin/sh
# Run as root in Alpine 3.23 with the source and an output directory mounted.
set -eu

output=$1
source_dir=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
jobs=${JOBS:-4}

apk add --no-cache build-base binutils ca-certificates cmake curl gperf meson pkgconf python3 xz \
    cairo-dev cairo-static fontconfig-dev fribidi-dev fribidi-static \
    freetype-static glib-static graphite2-static harfbuzz-dev harfbuzz-static libffi-dev \
    libx11-static libxcb-static libxext-static pixman-static \
    brotli-static bzip2-static expat-static gettext-static libpng-static \
    pcre2-static util-linux-static zlib-static

mkdir "$work/deps" "$work/s2"
cd "$work/deps"
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

fetch_source() {
    file=${1##*/}
    curl -fsSL --retry 3 -o "$file" "$1"
    printf '%s  %s\n' "$2" "$file" | sha256sum -c -
    tar -xf "$file"
}

autotools_static() (
    cd "$1"
    shift
    ./configure --prefix=/usr/local --enable-static --disable-shared "$@"
    make -j "$jobs" install
)

meson_static() (
    cd "$1"
    shift
    meson setup build --prefix=/usr/local --libdir=lib --buildtype=release \
        --default-library=static --wrap-mode=nofallback "$@"
    meson compile -C build -j "$jobs"
    meson install -C build --no-rebuild
)

# Build static archives for libraries Alpine supplies only as shared libraries.
xorg=https://xorg.freedesktop.org/archive/individual/lib
fetch_source "$xorg/libXau-1.0.12.tar.xz" \
    74d0e4dfa3d39ad8939e99bda37f5967aba528211076828464d2777d477fc0fb
fetch_source "$xorg/libXrender-0.9.12.tar.xz" \
    b832128da48b39c8d608224481743403ad1691bf4e554e4be9c174df171d1b97
fetch_source "$xorg/libXft-2.3.9.tar.xz" \
    60a25b78945ed6932635b3bb1899a517d31df7456e69867ffba27f89ff3976f5
fetch_source https://download.gnome.org/sources/pango/1.56/pango-1.56.4.tar.xz \
    17065e2fcc5f5a5bdbffc884c956bfc7c451a96e8c4fb2f8ad837c6413cb5a01
fetch_source https://distfiles.alpinelinux.org/distfiles/v3.23/libeconf-0.8.3.tar.gz \
    c8855f08d64c06b57a5028a14eec6ddde5e35124f9718031d592b44d8e091f83
fetch_source https://distfiles.alpinelinux.org/distfiles/edge/fontconfig-2.18.4.tar.xz \
    95013637eff5f81d4871dd16cd098c273264c16d0f9f0b27cc43b81b2794bee3

autotools_static libXau-1.0.12
autotools_static libXrender-0.9.12
autotools_static libXft-2.3.9
meson_static pango-1.56.4 -Dcairo=enabled -Dintrospection=disabled \
    -Ddocumentation=false -Dman-pages=false -Dxft=disabled -Dlibthai=disabled -Dsysprof=disabled
# GLib's static GIO needs libeconf through libblkid.
meson_static libeconf-0.8.3
# Read current font rules on rolling distributions.
autotools_static fontconfig-2.18.4 --sysconfdir=/etc --localstatedir=/var --disable-docs --disable-nls

# Use a fresh configure/build tree, including when invoked from a local checkout.
cd "$source_dir"
cp -R ./*.c ./*.h Makefile config.mk cui.lock.json assets scripts "$work/s2"
rm -f "$work/s2/config.h"
cd "$work/s2"
libs=$(pkg-config --static --libs xft libpng pangocairo cairo-xlib fontconfig)
make -j "$jobs" s2 LDFLAGS="-static -flto=auto -Wl,-Map,$work/link.map" \
    LDLIBS="-Wl,--start-group $libs -lm -lstdc++ -leconf -Wl,--end-group"

asset=s2-linux-$(uname -m)
strip -o "$output/$asset" s2
readelf -lW "$output/$asset" > "$work/program-headers"
readelf -dW "$output/$asset" > "$work/dynamic-section"
if grep -Eq '[[:space:]]INTERP[[:space:]]' "$work/program-headers" ||
    grep -Fq '(NEEDED)' "$work/dynamic-section"; then
    echo 'Release executable has dynamic dependencies.' >&2
    exit 1
fi

relink=$work/$asset-relink
python3 scripts/release-sources.py "$work/link.map" "$relink"
mkdir "$relink/objects" "$relink/notices"
cp ./*.o "$relink/objects/"
cp "$work/deps/"*.tar.* "$relink/sources/"
cp "$source_dir/LICENSE" assets/fonts/OFL.txt "$relink/notices/"
cp vendor/cui/LICENSE "$relink/notices/LICENSE-CUI"
cp vendor/cui/third_party/clay/LICENSE "$relink/notices/LICENSE-Clay"
cp scripts/relink.txt "$relink/README.txt"
cc -dumpfullversion > "$relink/gcc-version"
(
    cd "$relink"
    cc -static -flto=auto -o "$work/relinked-s2" objects/*.o \
        -Wl,--start-group libraries/*.a -Wl,--end-group
)
test "$("$work/relinked-s2" -v)" = "$(./s2 -v)"
tar -czf "$output/$asset-relink.tar.gz" -C "$work" "$asset-relink"
(cd "$output" && sha256sum "$asset" > "$asset.sha256")
