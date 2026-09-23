#!/bin/sh
# Installs FlameRobin from this extracted tree.
#
#   ./install.sh                 for the current user, no root needed:
#                                ~/.local/opt/flamerobin, command in ~/.local/bin
#   sudo ./install.sh --system   for all users:
#                                /opt/flamerobin-<version>, command in /usr/local/bin
#   ./install.sh --prefix DIR    into DIR (with --user or --system for where the
#                                command, menu entry and icons go)
#
# Installing again over an earlier tarball install replaces it.  Everything
# installed is recorded, and <prefix>/uninstall.sh removes exactly that.

set -eu

HERE=$(dirname "$(readlink -f "$0")")
VERSION=$(cat "${HERE}/VERSION" 2>/dev/null || echo unknown)
MANIFEST_NAME=.flamerobin-install-manifest

usage()
{
    sed -n '2,13s/^# \{0,1\}//p' "$0"
    exit "${1:-0}"
}

die()
{
    echo "install.sh: $*" >&2
    exit 1
}

MODE=
PREFIX=
while [ $# -gt 0 ]; do
    case "$1" in
        --user) MODE=user ;;
        --system) MODE=system ;;
        --prefix) [ $# -ge 2 ] || die "--prefix needs a directory"; PREFIX=$2; shift ;;
        --prefix=*) PREFIX=${1#--prefix=} ;;
        -h|--help) usage 0 ;;
        *) echo "install.sh: unknown option $1" >&2; usage 1 ;;
    esac
    shift
done

if [ -z "${MODE}" ]; then
    if [ "$(id -u)" -eq 0 ]; then MODE=system; else MODE=user; fi
fi

if [ "${MODE}" = system ]; then
    [ "$(id -u)" -eq 0 ] || die "--system needs root, run: sudo \"$0\" --system"
    # /opt/flamerobin belongs to the .deb, so the tarball gets its own directory.
    : "${PREFIX:=/opt/flamerobin-${VERSION}}"
    BINDIR=/usr/local/bin
    DATADIR=/usr/local/share
else
    [ -n "${HOME:-}" ] || die "HOME is not set"
    : "${PREFIX:=${HOME}/.local/opt/flamerobin}"
    BINDIR=${HOME}/.local/bin
    DATADIR=${XDG_DATA_HOME:-${HOME}/.local/share}
fi

case "${PREFIX}" in
    /*) ;;
    *) PREFIX=$(pwd)/${PREFIX} ;;
esac
PREFIX=${PREFIX%/}

[ -x "${HERE}/bin/flamerobin" ] || die "run install.sh from the extracted FlameRobin tree"
[ "$(readlink -f "${PREFIX}" 2>/dev/null || echo "${PREFIX}")" != "${HERE}" ] \
    || die "this tree is already at ${PREFIX}; nothing to install"

# Never clobber something that is not an earlier tarball install.
if [ -e "${PREFIX}" ] && [ ! -f "${PREFIX}/${MANIFEST_NAME}" ]; then
    if [ -d "${PREFIX}" ] && [ -z "$(ls -A "${PREFIX}")" ]; then
        :
    else
        die "${PREFIX} exists and was not installed by this script; choose another --prefix"
    fi
fi

for f in "${BINDIR}/flamerobin" "${DATADIR}/applications/flamerobin.desktop"; do
    if [ -e "${f}" ] || [ -L "${f}" ]; then
        if [ -f "${PREFIX}/${MANIFEST_NAME}" ] && grep -qxF "${f}" "${PREFIX}/${MANIFEST_NAME}"; then
            continue
        fi
        die "${f} already exists (another FlameRobin installation?); remove it first"
    fi
done

if [ -f "${PREFIX}/uninstall.sh" ]; then
    echo "Replacing the FlameRobin installation in ${PREFIX}"
    sh "${PREFIX}/uninstall.sh" --quiet
fi

echo "Installing FlameRobin ${VERSION} into ${PREFIX}"
mkdir -p "${PREFIX}"
MANIFEST="${PREFIX}/${MANIFEST_NAME}"
: > "${MANIFEST}"

# The tree itself, without the scripts that only make sense before installing.
( cd "${HERE}" && tar cf - --exclude=./install.sh --exclude=./install-desktop-entry.sh . ) \
    | ( cd "${PREFIX}" && tar xf - --no-same-owner )
echo "${PREFIX}" >> "${MANIFEST}"

mkdir -p "${BINDIR}"
ln -s "${PREFIX}/flamerobin" "${BINDIR}/flamerobin"
echo "${BINDIR}/flamerobin" >> "${MANIFEST}"

mkdir -p "${DATADIR}/applications"
cat > "${DATADIR}/applications/flamerobin.desktop" <<EOF
[Desktop Entry]
Name=FlameRobin
GenericName=Database administration tool
Comment=Administration Tool for Firebird DBMS
Exec="${PREFIX}/flamerobin" %F
Icon=flamerobin
Type=Application
Terminal=false
Categories=Development;Database;GTK;
Keywords=flamerobin;firebird;
EOF
echo "${DATADIR}/applications/flamerobin.desktop" >> "${MANIFEST}"

for icon in "${PREFIX}"/share/icons/hicolor/*/apps/flamerobin.*; do
    [ -e "${icon}" ] || continue
    rel=${icon#"${PREFIX}"/share/icons/}
    target="${DATADIR}/icons/${rel}"
    # Leave icons another installation (e.g. the .deb) put there alone.
    [ -e "${target}" ] && continue
    mkdir -p "$(dirname "${target}")"
    cp "${icon}" "${target}"
    echo "${target}" >> "${MANIFEST}"
done

cat > "${PREFIX}/uninstall.sh" <<'EOF'
#!/bin/sh
# Removes the FlameRobin installation this script belongs to: exactly the
# files and directories listed in the manifest next to it.
set -eu
HERE=$(dirname "$(readlink -f "$0")")
MANIFEST="${HERE}/.flamerobin-install-manifest"
[ -f "${MANIFEST}" ] || { echo "uninstall.sh: ${MANIFEST} not found" >&2; exit 1; }
QUIET=
[ "${1:-}" = --quiet ] && QUIET=1
# Files first, the installation directory (first line) last.
tail -n +2 "${MANIFEST}" | while IFS= read -r f; do
    [ -n "${f}" ] || continue
    rm -f "${f}"
    [ -n "${QUIET}" ] || echo "Removed ${f}"
done
sed -n 's#/applications/flamerobin\.desktop$##p' "${MANIFEST}" | while IFS= read -r d; do
    update-desktop-database "${d}/applications" >/dev/null 2>&1 || true
    gtk-update-icon-cache -q -t "${d}/icons/hicolor" >/dev/null 2>&1 || true
done
PREFIX=$(head -n 1 "${MANIFEST}")
case "${PREFIX}" in
    ""|/) echo "uninstall.sh: refusing to remove '${PREFIX}'" >&2; exit 1 ;;
esac
rm -rf "${PREFIX}"
[ -n "${QUIET}" ] || echo "Removed ${PREFIX}"
EOF
chmod 755 "${PREFIX}/uninstall.sh"

update-desktop-database "${DATADIR}/applications" >/dev/null 2>&1 || true
gtk-update-icon-cache -q -t "${DATADIR}/icons/hicolor" >/dev/null 2>&1 || true

# FlameRobin needs GTK 3, WebKitGTK 4.1 and libsecret from the system.
MISSING=$(LD_LIBRARY_PATH="${PREFIX}/lib" ldd "${PREFIX}/bin/flamerobin" 2>/dev/null \
    | awk '/not found/ { print $1 }' | sort -u)

echo
echo "FlameRobin ${VERSION} is installed."
echo "  Start it from the application menu, or run: flamerobin"
case ":${PATH}:" in
    *":${BINDIR}:"*) ;;
    *) echo "  ${BINDIR} is not in your PATH; add it, or run ${PREFIX}/flamerobin" ;;
esac
echo "  To uninstall: ${PREFIX}/uninstall.sh"

if [ -n "${MISSING}" ]; then
    echo
    echo "Warning: these system libraries are missing, and FlameRobin will not start"
    echo "until they are installed:"
    for lib in ${MISSING}; do echo "  ${lib}"; done
    echo "  Debian/Ubuntu: sudo apt install libwebkit2gtk-4.1-0 libsecret-1-0"
    echo "  Fedora:        sudo dnf install webkit2gtk4.1 libsecret"
fi
