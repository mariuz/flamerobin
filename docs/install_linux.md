# Installing FlameRobin on Linux

FlameRobin publishes a `.deb` package for Debian and Ubuntu, and an install
script for every other Linux distribution. Both install the same build: the
FlameRobin application with its own Firebird 5 client and the Firebird embedded
engine, so a local database opens with no Firebird server running (see
[Using Firebird Embedded](firebird_embedded.md)).

| you use | install with |
| --- | --- |
| Ubuntu 24.04 or later, Debian 13 or later | [the `.deb` package](#debian-and-ubuntu) |
| Fedora, Arch, openSUSE, or another distribution | [the install script](#other-distributions) |
| a sandboxed package | the snap or Flatpak from the [releases page](https://github.com/mariuz/flamerobin/releases); these connect through a Firebird server and have no embedded engine |

---

## Debian and Ubuntu

### Requirements

* A Debian-based distribution: Ubuntu 24.04 or later, or Debian 13 or later
* x86_64 (`amd64`)

Other Debian-based distributions built on these releases, such as Linux Mint 22
or Pop!_OS 24.04, may work but aren't tested. Ubuntu 22.04 and Debian 12 are too
old: FlameRobin needs glibc 2.39 and the C++ library of GCC 13 or later.

### Install

FlameRobin doesn't have an apt repository yet, so you install the `.deb` from
the [releases page](https://github.com/mariuz/flamerobin/releases) and repeat
the same steps to update. Open a terminal and run the commands in each step.

1. **Download the package.** This command looks up the newest release and
   downloads its `.deb` to the current directory:

   ```bash
   curl -fLO "$(curl -fsSL https://api.github.com/repos/mariuz/flamerobin/releases/latest | grep -o 'https://[^"]*/flamerobin-[0-9.]*\.deb' | head -n 1)"
   ```

   If `curl` reports `command not found`, install it first with
   `sudo apt install curl`. You can also download the `.deb` from the releases
   page in your browser.

2. **Install it.** From the directory that contains the downloaded file:

   ```bash
   sudo apt install ./flamerobin-*.deb
   ```

   apt installs the libraries FlameRobin needs from your distribution: GTK 3,
   WebKitGTK 4.1 and libsecret. You can also open the downloaded file with your
   software installer, such as GNOME Software.

3. **Launch it.** Start **FlameRobin** from your application launcher, or run
   `flamerobin` from a terminal.

FlameRobin is installed in `/opt/flamerobin`. The package adds the `flamerobin`
command in `/usr/local/bin` (or `/usr/bin` when `/usr/local/bin` isn't in your
`PATH`), a menu entry and icons.

### Update

FlameRobin doesn't update itself. To update, download and install the new
`.deb` with the same two commands as in [Install](#install); apt replaces the
installed version and keeps your settings.

To see which version you have:

```bash
dpkg -s flamerobin | grep '^Version'
```

### Uninstall

```bash
sudo apt remove flamerobin
```

This removes `/opt/flamerobin`, the `flamerobin` command, the menu entry and
the icons. Your settings and registered databases stay in `~/.flamerobin`;
delete that directory to remove them too.

Packages before 26.9.7 leave the command, menu entry and icons behind when
removed. To clean them up:

```bash
sudo rm -f /usr/local/bin/flamerobin /usr/local/share/applications/flamerobin.desktop \
    /usr/share/pixmaps/flamerobin.png
sudo find /usr/share/icons/hicolor /usr/local/share/icons/hicolor -name 'flamerobin.*' -delete
```

### Troubleshoot

#### Unsupported file ./flamerobin-*.deb given on commandline

The pattern didn't match a `.deb` file in the current directory. Confirm the
download completed, then run `sudo apt install ./flamerobin-*.deb` again from
the directory that contains the file.

#### Unmet dependencies

If apt stops with `The following packages have unmet dependencies`, read which
dependency it names:

* `libc6 (>= 2.39)` or `libstdc++6 (>= 13)`: your distribution is older than
  the package supports. Ubuntu 22.04 ships glibc 2.35 and Debian 12 ships 2.36.
  Upgrade to Ubuntu 24.04 or later, or Debian 13 or later, or use the snap or
  Flatpak.
* `libwebkit2gtk-4.1-0` is `not installable`: your package lists are out of
  date or your distribution is too old. Run `sudo apt update` and install
  again.

#### Unable to complete network request to host "localhost"

A database registered without a server host name opens in embedded mode. If
this error appears, the Firebird engine is missing or can't open that
database; see [Troubleshooting in Using Firebird Embedded](firebird_embedded.md#7-troubleshooting).

---

## Other distributions

### Requirements

* A Linux distribution with glibc 2.39 or later: for example Fedora 40 or later,
  Ubuntu 24.04 or later, Debian 13 or later, openSUSE Tumbleweed or Arch Linux
* x86_64
* `curl` or `wget`, `tar` and `gzip` to install
* GTK 3, WebKitGTK 4.1 and libsecret from your distribution to run

musl-based distributions such as Alpine aren't supported.

### Install

Run the install script. It installs the latest release for the current user,
without root access:

```bash
curl -fsSL https://raw.githubusercontent.com/mariuz/flamerobin/master/install.sh | bash
```

Then start **FlameRobin** from your application launcher, or run `flamerobin`
from a terminal. If the script reports that `~/.local/bin` isn't in your
`PATH`, add it, or start FlameRobin with `~/.local/opt/flamerobin/flamerobin`.

The script:

1. checks that the system is x86_64 Linux with glibc 2.39 or later;
2. downloads `flamerobin-VERSION-linux-x64.tar.gz` from the
   [releases page](https://github.com/mariuz/flamerobin/releases) and verifies
   it against the published `.sha256` checksum;
3. installs it into `~/.local/opt/flamerobin`, links the `flamerobin` command
   into `~/.local/bin`, and adds a menu entry and icons under `~/.local/share`;
4. writes `~/.local/opt/flamerobin/uninstall.sh`, and warns if GTK, WebKitGTK
   or libsecret are missing.

It never overwrites a FlameRobin it didn't install, such as the `.deb`.

To install the libraries FlameRobin needs:

| distribution | command |
| --- | --- |
| Fedora | `sudo dnf install webkit2gtk4.1 libsecret` |
| openSUSE | `sudo zypper install libwebkit2gtk-4_1-0 libsecret-1-0` |
| Arch Linux | `sudo pacman -S webkit2gtk-4.1 libsecret` |
| Debian, Ubuntu | `sudo apt install libwebkit2gtk-4.1-0 libsecret-1-0` |

### Install a specific version

Pass a version number after `bash -s`:

```bash
curl -fsSL https://raw.githubusercontent.com/mariuz/flamerobin/master/install.sh | bash -s 26.9.7
```

The Linux tarball is published from FlameRobin 26.9.7 on.

### Install for all users

Run the script with `sudo` and `--system`. FlameRobin is installed into
`/opt/flamerobin-VERSION`, with the `flamerobin` command in `/usr/local/bin`
and the menu entry and icons under `/usr/local/share`:

```bash
curl -fsSL https://raw.githubusercontent.com/mariuz/flamerobin/master/install.sh | sudo bash -s -- --system
```

To choose the directory, add `--prefix DIR`:

```bash
curl -fsSL https://raw.githubusercontent.com/mariuz/flamerobin/master/install.sh | bash -s -- --prefix ~/apps/flamerobin
```

### Install without the script

Download `flamerobin-VERSION-linux-x64.tar.gz` from the
[releases page](https://github.com/mariuz/flamerobin/releases), then either run
it in place:

```bash
tar xzf flamerobin-*-linux-x64.tar.gz
./flamerobin-*-linux-x64/flamerobin
```

or install it with the script it contains, which is what the install script
runs:

```bash
./flamerobin-*-linux-x64/install.sh             # for the current user
sudo ./flamerobin-*-linux-x64/install.sh --system
```

To check the download first, save the matching `.sha256` file next to it and
run `sha256sum -c flamerobin-*-linux-x64.tar.gz.sha256`.

### Update

FlameRobin doesn't update itself. Run the install command again: it replaces
the installed version with the latest release, or with the version you pass.
Your settings are kept.

### Uninstall

Run the uninstaller the install wrote. For an install for the current user:

```bash
~/.local/opt/flamerobin/uninstall.sh
```

For an install for all users, replacing `VERSION`:

```bash
sudo /opt/flamerobin-VERSION/uninstall.sh
```

It removes exactly what was installed. Your settings and registered databases
stay in `~/.flamerobin`; delete that directory to remove them too.

### Troubleshoot

#### glibc is too old

FlameRobin's release binaries need glibc 2.39 or later. On an older
distribution, use the snap or Flatpak, or
[build FlameRobin from source](../BUILD.md).

#### release VERSION has no Linux tarball

The tarball is published from FlameRobin 26.9.7 on. For an older release, use
the `.deb`, snap or Flatpak.

#### checksum mismatch

The download was damaged or changed on the way. Run the install command again.
If it keeps failing, check whether a proxy on your network rewrites downloads
from `github.com` or `objects.githubusercontent.com`.

#### already exists (another FlameRobin installation?)

The script found a `flamerobin` command or menu entry it didn't create, most
often from the `.deb`. Remove the other installation first, for example with
`sudo apt remove flamerobin`, or keep using it.

#### FlameRobin doesn't start after installing

The installer lists the system libraries that are missing. Install them with
the command for your distribution from [Install](#install-1), then start
FlameRobin again.
