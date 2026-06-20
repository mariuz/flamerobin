# FlameRobin Build Instructions

Below are the build instructions for FlameRobin on all supported build environments.

---

## Common to All Builds

### General Requirements

To build FlameRobin, you need the following prerequisites installed on your system:

*   **Firebird SQL**: Client libraries (`fbclient.dll`/`.so`/`.dylib` at minimum) are required for building and running. A full server installation is recommended for testing. [FirebirdSQL Website](https://www.firebirdsql.org/)
*   **C++20 Compiler**: FlameRobin is compiled as C++20 (`CMAKE_CXX_STANDARD=20`). You need a modern compiler such as:
    *   Visual Studio 2022 or newer (Windows)
    *   GCC 11 or newer (Linux)
    *   Clang 13 or newer (Linux)
    *   Apple Clang (macOS)
*   **wxWidgets**: A Unicode build of wxWidgets (3.2+ recommended) with `base`, `core`, `html`, `xml`, `aui`, `stc`, `adv`, `net`, and `webview` components. On Windows and Linux, this is typically resolved automatically via `vcpkg`. On macOS, it is typically installed via Homebrew. [wxWidgets Website](https://www.wxwidgets.org/)
*   **CMake**: Version 3.18 or higher. [CMake Website](https://cmake.org/)
*   **Git**: Required for cloning the repository and managing submodules/dependencies. [Git Website](https://git-scm.com/)

**Optional Tools (Windows):**
*   [GitHub Desktop](https://github.com/) (for interacting with this project)
*   [TortoiseGit](https://tortoisegit.org/) (shell extension for Windows)

### General Notes

*   **C++20 Compatibility**: FlameRobin requires C++20 standard compliance. Please keep code contributions compatible with C++20.
*   **Build Settings Consistency**: FlameRobin must be compiled with the same configuration (Unicode/ANSI, Debug/Release, CRT linkages) as the wxWidgets library it links against. On Linux/GCC, you can compile wxWidgets in Release mode while keeping FlameRobin in Debug mode if you only want to debug FlameRobin.
*   **Development Support**: If you encounter issues during configuration or building, please reach out to the development team at the [FlameRobin GitHub Repository](https://github.com/mariuz/flamerobin).

---

## Core Dependencies (vcpkg Manifest Mode)

FlameRobin uses [vcpkg](https://github.com/microsoft/vcpkg) in **manifest mode** (`vcpkg.json`) to manage and pin third-party library dependencies.

The current list of requirements and overrides defined in the project:

| Dependency | Platform / Scope | Version | Notes |
| :--- | :--- | :--- | :--- |
| **Boost Libraries** | All | `1.90.0` | Includes `boost-dll`, `boost-filesystem`, `boost-multiprecision`, and `boost-test` |
| **firebird** | All | `5.0.4` | Firebird database client and development library |
| **fb-cpp** | All | `0.0.4` | FlameRobin's C++20 Firebird API wrapper (provided as a local vcpkg overlay port in `./ports/fb-cpp`) |
| **icu** | All | `78.1` | International Components for Unicode |
| **gtk3** | Linux, FreeBSD, OpenBSD | *System/vcpkg* | Configured with `wayland` feature support |
| **wxwidgets** | All | *vcpkg/System* | Unicode build with `webview` and `secretstore` features enabled |
| **pcre2** | All | *vcpkg/System* | Perl Compatible Regular Expressions library |
| **lexilla** | Windows | *vcpkg* | Scintilla's lexing component |
| **scintilla** | Windows | *vcpkg* | Scintilla editing control |
| **nlohmann-json** | All | *vcpkg* | JSON for Modern C++ |
| **crashpad** | Windows | *vcpkg* | Google Crashpad for automatic crash minidump capture |

---

## MSW - Visual C++ Build Instructions

The recommended build path for Windows is using **CMake** and **vcpkg** (manifest mode).

### Requirements

*   **Visual Studio 2022** (with *Desktop development with C++* workload)
*   **CMake** (3.18+)
*   **Git**
*   **vcpkg**
*   **Firebird Server** (for runtime testing)

The build expects the repository root to contain `vcpkg.json`, `vcpkg-configuration.json`, and the local overlay port in `ports/fb-cpp` (which applies necessary patches to the `fb-cpp` library during the build).

### Initial Setup

1.  **Clone the Repository**:
    ```cmd
    git clone https://github.com/mariuz/flamerobin.git
    cd flamerobin
    ```

2.  **Provide vcpkg**:
    *   **Option A**: Clone vcpkg directly inside the FlameRobin repository root (CMake will detect it automatically):
        ```cmd
        git clone https://github.com/microsoft/vcpkg vcpkg
        vcpkg\bootstrap-vcpkg.bat
        ```
    *   **Option B**: Use an existing, external vcpkg checkout:
        ```cmd
        git clone https://github.com/microsoft/vcpkg C:\src\vcpkg
        C:\src\vcpkg\bootstrap-vcpkg.bat
        ```

### Configure with CMake

Always use a clean, out-of-source build directory.

*   If you used **Option A** (`.\vcpkg` in repository root):
    ```cmd
    cmake -S . -B build -A x64
    ```
*   If you used **Option B** (external `vcpkg` directory):
    ```cmd
    cmake -S . -B build -A x64 -DCMAKE_TOOLCHAIN_FILE=C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake
    ```

### Build

Compile the release binaries:
```cmd
cmake --build build --config Release
```

### Troubleshooting

If configuration fails with an error similar to:
```
Could not find a package configuration file provided by "fb-cpp"
```
This indicates that the vcpkg toolchain was not correctly activated by CMake. 

1.  Check the configuration output for these debug variables:
    *   `DEBUG: CMAKE_TOOLCHAIN_FILE=...`
    *   `DEBUG: VCPKG_TARGET_TRIPLET=...`
    *   `DEBUG: VCPKG_INSTALLED_DIR=...`
2.  If they are empty, delete the `build` directory.
3.  Reconfigure again, explicitly passing `-DCMAKE_TOOLCHAIN_FILE` pointing to your vcpkg installation.
4.  Always clean the build directory before re-running configuration.

---

## Unix (Debian, Ubuntu, Mint, etc.)

On Debian-based systems, you can install compiler and GUI toolkits via the package manager. Other dependencies will be resolved by vcpkg.

### 1. Install System Tools and Libraries

Install the compiler toolchain, CMake, the wxWidgets development packages, and the WebKitGTK development packages (which are required for the webview component to render SVG icons and user templates correctly):
```bash
sudo apt-get update
sudo apt-get install build-essential git cmake libwxgtk3.2-dev libwebkit2gtk-4.1-dev
```

### 2. Clone and Build

1.  **Clone the repository recursively**:
    ```bash
    git clone --recursive https://github.com/mariuz/flamerobin.git
    cd flamerobin
    ```
2.  **Generate build files & compile**:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
    > [!NOTE]
    > `vcpkg` is enabled by default. If you need to disable vcpkg and build using entirely system/distro packages, use `-DENABLE_VCPKG=OFF`.
    > Note that you will need to manually provide and build all dependencies, including the patched `fb-cpp` package.

### 3. Run FlameRobin

Use the supplied helper script to start the application with its configuration paths set up:
```bash
./run_flamerobin.sh
```

### 4. Install FlameRobin

To install the application system-wide:
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

---

## macOS

### Prerequisites

*   [Homebrew](https://brew.sh/) package manager.
*   **Firebird client library (libfbclient)**:
    *   If using the official Firebird installer (from [FirebirdSQL Website](https://www.firebirdsql.org/)), the installer places the framework at: `/Library/Frameworks/Firebird.framework/`. CMake detects this path automatically.
    *   If using a custom location, you will need to supply the path during the CMake step.

### Build Steps

1.  **Install tools and libraries**:
    ```bash
    brew install cmake wxwidgets
    ```
2.  **Clone and prepare build**:
    ```bash
    git clone https://github.com/mariuz/flamerobin.git
    cd flamerobin
    mkdir build
    cd build
    ```
3.  **Configure with CMake**:
    ```bash
    cmake -DCMAKE_PREFIX_PATH="$(brew --prefix)" ..
    ```
    *If Firebird is installed in a custom location, specify it as follows:*
    ```bash
    cmake -DCMAKE_PREFIX_PATH="/path/to/firebird" ..
    ```
4.  **Build**:
    ```bash
    make
    ```
5.  **Run**:
    ```bash
    open flamerobin.app
    ```

### Install

To install to the `/Applications` folder:
```bash
sudo cmake --install .
```

### macOS Build Notes

*   The compiled app bundle embeds an `rpath` pointing to the locations of dependencies found at build time, so `DYLD_LIBRARY_PATH` overrides are not needed.
*   If you move, update, or reinstall Firebird, rebuild FlameRobin from the CMake configuration step (step 3).
