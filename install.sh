#!/usr/bin/env bash
# Installs FlameRobin on Linux x86_64 from the release tarball on GitHub.
#
#   curl -fsSL https://raw.githubusercontent.com/mariuz/flamerobin/master/install.sh | bash
#
# The latest release is installed for the current user, into
# ~/.local/opt/flamerobin, with the flamerobin command in ~/.local/bin and an
# application menu entry.  No root access is needed.
#
# Arguments, passed after "bash -s":
#
#   VERSION         a release such as 26.9.7, or "latest" (the default)
#   --system        install for all users into /opt/flamerobin-VERSION,
#                   with the command in /usr/local/bin; run with sudo:
#                   curl -fsSL .../install.sh | sudo bash -s -- --system
#   --prefix DIR    install into DIR
#
#   curl -fsSL https://raw.githubusercontent.com/mariuz/flamerobin/master/install.sh | bash -s 26.9.7
#
# Running it again installs the requested version over the previous one.
# To uninstall: ~/.local/opt/flamerobin/uninstall.sh

set -euo pipefail

REPO=mariuz/flamerobin
MIN_GLIBC=2.39

# Everything is inside main, so that a download cut short by the network
# never runs half a script.
main()
{
    local version=latest
    local install_args=()
    while [ $# -gt 0 ]; do
        case "$1" in
            --system|--user) install_args+=("$1") ;;
            --prefix) [ $# -ge 2 ] || die "--prefix needs a directory"; install_args+=("$1" "$2"); shift ;;
            --prefix=*) install_args+=("$1") ;;
            -h|--help) usage; exit 0 ;;
            latest|v[0-9]*|[0-9]*) version=${1#v} ;;
            *) die "unknown argument: $1" ;;
        esac
        shift
    done

    check_platform
    local fetch
    fetch=$(downloader)

    if [ "${version}" = latest ]; then
        info "Looking up the latest FlameRobin release"
        version=$(latest_version "${fetch}")
        [ -n "${version}" ] || die "could not determine the latest release of ${REPO}; pass a version, e.g. bash -s 26.9.7"
    fi

    local name="flamerobin-${version}-linux-x64"
    # FLAMEROBIN_RELEASES_URL points at a mirror, or a local copy for testing.
    local base="${FLAMEROBIN_RELEASES_URL:-https://github.com/${REPO}/releases/download}/v${version}"
    local tmp
    tmp=$(mktemp -d "${TMPDIR:-/tmp}/flamerobin-install.XXXXXX")
    # shellcheck disable=SC2064
    trap "rm -rf '${tmp}'" EXIT

    info "Downloading FlameRobin ${version}"
    if ! download "${fetch}" "${base}/${name}.tar.gz" "${tmp}/${name}.tar.gz"; then
        die "release ${version} has no Linux tarball (${base}/${name}.tar.gz).
The tarball is published from FlameRobin 26.9.7 on; for older releases use the
.deb, snap or Flatpak from https://github.com/${REPO}/releases/tag/v${version}"
    fi

    if download "${fetch}" "${base}/${name}.tar.gz.sha256" "${tmp}/${name}.tar.gz.sha256" 2>/dev/null; then
        verify_checksum "${tmp}" "${name}.tar.gz"
    else
        warn "no checksum published for ${name}.tar.gz; skipping verification"
    fi

    local top
    top=$(tar -tzf "${tmp}/${name}.tar.gz" | head -n 1 | cut -d/ -f1) || true
    [ -n "${top}" ] || die "${name}.tar.gz is empty or damaged"
    tar -C "${tmp}" -xzf "${tmp}/${name}.tar.gz"
    [ -x "${tmp}/${top}/install.sh" ] || die "${name}.tar.gz does not contain install.sh"

    # The tarball's own installer does the rest, and writes the uninstaller.
    bash "${tmp}/${top}/install.sh" ${install_args[@]+"${install_args[@]}"}
}

usage()
{
    cat <<EOF
Usage: install.sh [VERSION|latest] [--system] [--prefix DIR]

  curl -fsSL https://raw.githubusercontent.com/${REPO}/master/install.sh | bash
  curl -fsSL https://raw.githubusercontent.com/${REPO}/master/install.sh | bash -s 26.9.7
  curl -fsSL https://raw.githubusercontent.com/${REPO}/master/install.sh | sudo bash -s -- --system

See https://github.com/${REPO}/blob/master/docs/install_linux.md
EOF
}

info() { printf '%s\n' "$*"; }
warn() { printf 'Warning: %s\n' "$*" >&2; }
die()  { printf 'Error: %s\n' "$*" >&2; exit 1; }

check_platform()
{
    [ "$(uname -s)" = Linux ] || die "this installer is for Linux; see https://github.com/${REPO}/releases for other systems"
    case "$(uname -m)" in
        x86_64|amd64) ;;
        *) die "FlameRobin release binaries are built for x86_64 only (this is $(uname -m)); build from source: https://github.com/${REPO}/blob/master/BUILD.md" ;;
    esac

    local glibc
    glibc=$(getconf GNU_LIBC_VERSION 2>/dev/null | awk '{ print $2 }') || true
    [ -n "${glibc}" ] || die "glibc not found; FlameRobin release binaries need glibc ${MIN_GLIBC} or later (musl-based distributions such as Alpine are not supported)"
    if [ "$(printf '%s\n%s\n' "${MIN_GLIBC}" "${glibc}" | sort -V | head -n 1)" != "${MIN_GLIBC}" ]; then
        die "glibc ${glibc} is too old; FlameRobin needs glibc ${MIN_GLIBC} or later (Ubuntu 24.04, Debian 13, Fedora 40 or newer)"
    fi

    local tool
    for tool in tar gzip mktemp; do
        command -v "${tool}" > /dev/null 2>&1 || die "${tool} is required"
    done
}

downloader()
{
    if command -v curl > /dev/null 2>&1; then
        echo curl
    elif command -v wget > /dev/null 2>&1; then
        echo wget
    else
        die "curl or wget is required"
    fi
}

# download TOOL URL FILE - fails on HTTP errors such as 404
download()
{
    case "$1" in
        curl) curl -fsSL --retry 3 -o "$3" "$2" ;;
        wget) wget -q -O "$3" "$2" ;;
    esac
}

latest_version()
{
    local json
    case "$1" in
        curl) json=$(curl -fsSL "https://api.github.com/repos/${REPO}/releases/latest") || return 0 ;;
        wget) json=$(wget -q -O - "https://api.github.com/repos/${REPO}/releases/latest") || return 0 ;;
    esac
    printf '%s\n' "${json}" | sed -n 's/.*"tag_name": *"v\{0,1\}\([^"]*\)".*/\1/p' | head -n 1
}

verify_checksum()
{
    local dir=$1 file=$2
    if ! command -v sha256sum > /dev/null 2>&1; then
        warn "sha256sum not found; skipping checksum verification"
        return 0
    fi
    local expected actual
    expected=$(awk '{ print $1; exit }' "${dir}/${file}.sha256")
    actual=$(sha256sum "${dir}/${file}" | awk '{ print $1 }')
    [ -n "${expected}" ] && [ "${expected}" = "${actual}" ] \
        || die "checksum mismatch for ${file}: expected ${expected:-nothing}, got ${actual}"
    info "Checksum verified"
}

main "$@"
