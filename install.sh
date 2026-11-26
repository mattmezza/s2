#!/bin/sh
set -eu

die() {
    printf 's2 installer: %s\n' "$*" >&2
    exit 1
}

usage() {
    printf '%s\n' 'Usage: sh install.sh [install|upgrade] [vX.Y.Z]' \
        '       sh install.sh uninstall' \
        "Set S2_INSTALL_DIR to an absolute directory to override \$HOME/.local/bin."
}

download() {
    curl -fsSL --retry 3 --proto '=https' --proto-redir '=https' "$@"
}

main() {
    action=${1:-install}
    case "$action" in
        -h|--help) usage; return ;;
        install|upgrade) [ "$#" -le 2 ] || die "$(usage)" ;;
        uninstall) [ "$#" -eq 1 ] || die "$(usage)" ;;
        *) die "$(usage)" ;;
    esac

    bin=${S2_INSTALL_DIR:-"$HOME/.local/bin"}
    case "$bin" in
        /*) ;;
        *) die 'S2_INSTALL_DIR must be an absolute directory.' ;;
    esac
    if [ "$action" = uninstall ]; then
        rm -f -- "$bin/s2"
        printf 'Removed s2 from %s\n' "$bin"
        return
    fi

    case "$(uname -s):$(uname -m)" in
        Linux:x86_64) arch=x86_64 ;;
        Linux:aarch64|Linux:arm64) arch=aarch64 ;;
        *) die 's2 releases support Linux x86_64 and AArch64.' ;;
    esac

    releases=https://github.com/mattmezza/s2/releases
    tag=${2:-}
    if [ -z "$tag" ]; then
        url=$(download -o /dev/null -w '%{url_effective}' "$releases/latest") ||
            die 'Could not find the latest release.'
        tag=${url#"$releases/tag/"}
    fi
    if [ -z "$tag" ] || [ "$(printf '%s\n' "$tag" | LC_ALL=C grep -Ex 'v[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.-]+)?')" != "$tag" ]; then
        die 'Expected a release tag such as v0.1.1.'
    fi

    mkdir -p -- "$bin"
    work=$(mktemp -d "$bin/.s2.XXXXXXXX")
    trap 'rm -rf -- "$work"' EXIT
    trap 'exit 1' HUP INT TERM

    asset=s2-linux-$arch
    base=$releases/download/$tag
    download -o "$work/$asset" "$base/$asset" ||
        die "Could not download $asset from $tag."
    download -o "$work/$asset.sha256" "$base/$asset.sha256"
    checksum=$(cd "$work" && sha256sum "$asset")
    [ "$checksum" = "$(cat "$work/$asset.sha256")" ] || die 'Release checksum mismatch.'
    chmod 755 "$work/$asset"
    [ "$("$work/$asset" -v)" = "s2-${tag#v}" ] || die "Executable failed the $tag version check."
    mv -fT -- "$work/$asset" "$bin/s2"

    printf 'Installed s2 %s at %s/s2\n' "$tag" "$bin"
    case ":${PATH:-}:" in
        *":$bin:"*) ;;
        *) printf 'Add %s to PATH to run s2.\n' "$bin" ;;
    esac
}

main "$@"
