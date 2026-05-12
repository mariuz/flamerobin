FlameRobin
---------------------------
FlameRobin is a software package for administration of Firebird DBMS. It is
developed by:

* Milan Babuskov
* Nando Dessena
* Michael Hieke
* Gregory Sapunkov
* Bart Bakker
* Marius Popa


The following people also made a significant non-coding contributions:

* Alex Stanciu
* Barbara Del Vecchio

Build Status
---------------------------
| Platform | Status |
|----------|--------|
| Windows  | [![Build Flamerobin for Windows](https://github.com/mariuz/flamerobin/actions/workflows/windows-build.yml/badge.svg)](https://github.com/mariuz/flamerobin/actions/workflows/windows-build.yml) |
| Debian/Linux | [![Build FlameRobin for Debian](https://github.com/mariuz/flamerobin/actions/workflows/linux-build.yml/badge.svg)](https://github.com/mariuz/flamerobin/actions/workflows/linux-build.yml) |
| macOS    | [![Build FlameRobin for macOS](https://github.com/mariuz/flamerobin/actions/workflows/macos-build.yml/badge.svg)](https://github.com/mariuz/flamerobin/actions/workflows/macos-build.yml) |
| Fedora Flatpak | [![Build FlameRobin Flatpak (Fedora)](https://github.com/mariuz/flamerobin/actions/workflows/fedora-flatpak-build.yml/badge.svg)](https://github.com/mariuz/flamerobin/actions/workflows/fedora-flatpak-build.yml) |

Notice:
Now you can download latest Windows snapshot builds directly from the [Build Flamerobin for Windows](https://github.com/mariuz/flamerobin/actions/workflows/windows-build.yml) Action

Building
---------------------------
For detailed build instructions for all supported platforms (Windows, Linux, macOS), see the
[BUILD.txt](https://github.com/mariuz/flamerobin/blob/master/BUILD.txt) file.

**Notice:** FlameRobin is currently undergoing a major migration to the `fb-cpp` database library and has been upgraded to **C++20**. Please ensure contributions are compatible with the C++20 standard.

Roadmap & Modernization
---------------------------
*   [Project Roadmap](ROADMAP.md) - Track the progress of the migration to `fb-cpp`.
*   [Database Abstraction Layer (DAL) & fb-cpp Migration](docs/migration-to-fb-cpp.md) - Detailed architectural documentation.

macOS distribution builds
---------------------------
For producing a signed, notarized `FlameRobin.app` distributable to other
Macs (build + bundle dylibs + sign + notarize + staple + zip in one step),
see [dist/macos/README.md](dist/macos/README.md). One-shot release:

```sh
dist/macos/release.sh
```

Wiki
---------------------------
Additional documentation and guides are available on the [FlameRobin Wiki](https://github.com/mariuz/flamerobin/wiki).

Documentation
---------------------------
* [What's New](https://github.com/mariuz/flamerobin/blob/master/docs/fr_whatsnew.html)
* [License](https://github.com/mariuz/flamerobin/blob/master/docs/fr_license.html)

Developer Documentation
---------------------------
Developer-focused documentation can be found in the [devdocs](https://github.com/mariuz/flamerobin/tree/master/devdocs) folder:

* [Creating Linux Packages](https://github.com/mariuz/flamerobin/blob/master/devdocs/Creating_Linux_Packages.txt)
* [Building with Dev-C++](https://github.com/mariuz/flamerobin/blob/master/devdocs/Dev-Cpp_How_to_build.txt)
* [Building with MSVC.NET](https://github.com/mariuz/flamerobin/blob/master/devdocs/MSVC.NET_How_to_build.txt)
* [Files and Paths](https://github.com/mariuz/flamerobin/blob/master/devdocs/Files_and_paths.txt)

License
---------------------------
FlameRobin code is licensed under the MIT license.
A copy of the license can be found in the [LICENSE](https://github.com/mariuz/flamerobin/blob/master/LICENSE) file

Part of code covering IBPP library is licensed under IBPP license.
A copy of IBPP license can be found in src/ibpp folder.


Internal fb-cpp Library
---------------------------
FlameRobin uses a patched version of the [fb-cpp](https://github.com/asfernandes/fb-cpp) library. These patches are necessary to support delayed transaction starts and transaction-less statement preparation, which are critical for FlameRobin's Data Access Layer (DAL).

The library is built from an isolated source tree located in `src/engine/db/fbcpp/patches/fb-cpp/`. This ensures that all internal headers consistently see our modifications and avoids build conflicts with the clean upstream submodule in `src/fb-cpp/`.

### Synchronizing with Upstream

If you need to update the internal `fb-cpp` source from the submodule (e.g., to pull in new upstream features), use the provided synchronization script:

```bash
./utils/sync-fbcpp.sh
```

This script will:
1. Sync the source files from `src/fb-cpp/src/fb-cpp/` to the internal patches directory.
2. Automatically re-apply all FlameRobin-specific patches using `perl` regex.

After running the script, verify that the project still compiles and then commit the changes.

### Manual Patching (Gemini CLI Prompt)

If you need to re-apply the patches manually or the synchronization script fails due to upstream changes, you can use the following prompt with Gemini CLI:

> "Copy the clean `fb-cpp` source from `src/fb-cpp/src/fb-cpp/` to `src/engine/db/fbcpp/patches/fb-cpp/` and apply the FlameRobin patches: 1) Add `Transaction(Client&)` and `start()` to `Transaction`, 2) decouple `Statement::prepare` from transaction, 3) implement `closeCursor()` in `Statement`, and 4) fix aggregate initialization in `BackupManager.h`."
