# Windows Portable Releases in FlameRobin

Starting with version 26.6.8, FlameRobin automatically builds and packages **Portable Release ZIP files** for Windows (both 32-bit and 64-bit architectures) via GitHub Actions.

These portable packages allow you to run FlameRobin on any Windows machine without requiring an installation process or pre-installing a Firebird server/client instance.

---

## 1. What's Inside the Portable ZIP

Each portable ZIP file contains a complete, self-contained environment:

- **FlameRobin Executable**: `flamerobin.exe` (built in Release mode).
- **FlameRobin Resources**: Complete subdirectories for configuration templates, documentation, styles, and templates:
  - `docs/`
  - `conf-defs/`
  - `code-templates/`
  - `html-templates/`
  - `sys-templates/`
  - `xml-styles/`
- **Firebird Client Library**: `fbclient.dll` and its helper `ib_util.dll` (matching the Firebird version specified in `vcpkg.json`, currently `5.0.4`).
- **Unicode Support (ICU)**: Required ICU libraries (e.g., `icu*.dll` and `icu*.dat`) needed by the Firebird client to handle international character sets.
- **Authentication Plugins**:
  - `plugins/srp.dll` (Secure Remote Password authentication plugin).
  - `plugins/legacy_auth.dll` and `plugins/legacy_usermanager.dll` (for legacy authentication support).
- **International Character Sets**:
  - `intl/fbintl.dll` and `intl/fbintl.conf` (for local character set translations).
- **Default Configuration**: A pre-configured `firebird.conf` file.

---

## 2. Advantages of the Portable Release

1. **Zero Installation**: Extract the ZIP to any folder (even a USB flash drive or a temporary directory) and run `flamerobin.exe` directly.
2. **Built-in Firebird Client**: You don't need to manually install Firebird or copy `fbclient.dll` to your system directories. The bundled `fbclient.dll` in the application directory is automatically loaded by FlameRobin.
3. **Out-of-the-box Embedded Support**: Because a default `firebird.conf` and all necessary authentication/unicode plugins are bundled next to `fbclient.dll`, you can connect to local `.fdb` databases in **Embedded Mode** immediately without setting up a remote server (see [Using Firebird Embedded](firebird_embedded.md) guide).

---

## 3. How it is Built

The portable releases are automatically compiled and packaged by GitHub Actions workflows:
- **PR/Branch builds** ([windows-build.yml](file:///.github/workflows/windows-build.yml)): Uploads the portable ZIP as a standard workflow run artifact (separate from the installer artifact).
- **Release builds** ([release.yml](file:///.github/workflows/release.yml)): Automatically compiles and publishes the portable ZIP files as release assets alongside the Inno Setup installers on the GitHub Release page (e.g. `flamerobin-portable-26.6.8-x64.zip` and `flamerobin-portable-26.6.8-x86.zip`).
