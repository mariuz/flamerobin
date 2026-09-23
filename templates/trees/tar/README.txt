FlameRobin @VERSION@ for Linux x86_64
======================================

Run it in place
---------------
Extract anywhere and run, no root needed:

    tar xzf @TAR_NAME@.tar.gz
    ./@TAR_NAME@/flamerobin

To add it to your application menu while running it in place:

    ./@TAR_NAME@/install-desktop-entry.sh

Install it
----------
For the current user, into ~/.local/opt/flamerobin, with the "flamerobin"
command in ~/.local/bin and a menu entry:

    ./@TAR_NAME@/install.sh

For all users, into /opt/flamerobin-@VERSION@, with the command in
/usr/local/bin:

    sudo ./@TAR_NAME@/install.sh --system

Or choose the directory with --prefix DIR. Installing again replaces an
earlier install from a tarball; it never overwrites the .deb or files it did
not create. To remove it again:

    ~/.local/opt/flamerobin/uninstall.sh                  (current user)
    sudo /opt/flamerobin-@VERSION@/uninstall.sh           (all users)

System requirements
-------------------
GTK 3, WebKitGTK 4.1 and libsecret must be installed, as for the .deb:

    Debian/Ubuntu: sudo apt install libwebkit2gtk-4.1-0 libsecret-1-0
    Fedora:        sudo dnf install webkit2gtk4.1 libsecret

install.sh warns if any of them is missing.

Firebird
--------
The Firebird 5 client is bundled in lib/, together with the embedded engine
in plugins/firebird/, so a database registered without a server host name
opens in embedded mode with no Firebird server running (ODS 13.0 and 13.1,
i.e. Firebird 4 and 5 databases). firebird.conf in this directory documents
how to add Engine12 for Firebird 3 databases. See
https://github.com/mariuz/flamerobin/blob/master/docs/firebird_embedded.md

Your settings are kept in ~/.flamerobin, shared with any other FlameRobin
installation.
