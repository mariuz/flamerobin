#!/bin/sh
# Adds FlameRobin to the application menu of the current user, pointing at
# this tree, for running FlameRobin in place without installing it.  Run it
# again after moving the tree.  To install instead, use install.sh.
HERE=$(dirname "$(readlink -f "$0")")
APPS="${XDG_DATA_HOME:-${HOME}/.local/share}/applications"
mkdir -p "${APPS}"
cat > "${APPS}/flamerobin.desktop" <<EOF
[Desktop Entry]
Name=FlameRobin
GenericName=Database administration tool
Comment=Administration Tool for Firebird DBMS
Exec="${HERE}/flamerobin" %F
Icon=${HERE}/share/icons/hicolor/256x256/apps/flamerobin.png
Type=Application
Terminal=false
Categories=Development;Database;GTK;
Keywords=flamerobin;firebird;
EOF
update-desktop-database "${APPS}" >/dev/null 2>&1 || true
echo "Installed ${APPS}/flamerobin.desktop"
