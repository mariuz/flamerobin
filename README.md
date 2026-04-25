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

Compile-time C++ standard notice: FlameRobin is currently built as C++14
(`CMAKE_CXX_STANDARD=14`). Please keep contributions compatible with C++14.

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

Some icons are licensed under LGPL license.
A copy of LGPL license can be found in res folder.
