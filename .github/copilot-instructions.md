# FlameRobin — Copilot Instructions

FlameRobin is a C++20/wxWidgets GUI administration tool for Firebird DBMS.

## Build

**Linux (Debian/Ubuntu)**
```sh
mkdir build && cd build
cmake ..
make -j$(nproc)
# Run from build dir (sets up paths to resource subdirs):
./run_flamerobin.sh
```

**Windows (CMake + vcpkg)**
```cmd
cmake -S . -B build -A x64
cmake --build build --config Release
```

**macOS**
```sh
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix)" ..
make -j$(nproc)
open flamerobin.app
```

vcpkg is used for dependencies and is auto-detected if cloned into the repo root as `./vcpkg`. The `fb-cpp` dependency is supplied via a custom overlay port in `ports/fb-cpp/`.

When adding new `.cpp` files, re-run CMake to register them (`cmake ..` or `cmake -S . -B build`).

## Tests

Run the full test suite from the build directory:
```sh
ctest --output-on-failure
# or with verbose Boost.Test output:
ctest --output-on-failure -V
BOOST_TEST_LOG_LEVEL=all ctest -R <test_name>
```

Run a single named test:
```sh
ctest -R index_ddl_test
# or run the executable directly:
./index_ddl_test
```

Test files are named `*Test.cpp` and are co-located with the source they test (e.g., `src/metadata/IndexDDLTest.cpp`). Tests use Boost.Test. Prefer comprehensive test cases that verify all related properties in one test rather than many small single-assertion tests.

## Architecture

```
src/
├── engine/db/          # Database Abstraction Layer (DAL)
│   ├── IDatabase.h     # Core interfaces (IDatabase, ITransaction, IStatement, IService, IBlob)
│   ├── DatabaseBackend.h  # Shared types, enums (ColumnType, StatementType, BackupConfig, etc.)
│   ├── DatabaseFactory.{cpp,h}
│   ├── fbcpp/          # Modern fb-cpp backend (DEFAULT)
│   └── ibpp/           # Legacy IBPP backend (planned for removal)
├── metadata/           # Object model: Database, Table, View, Procedure, etc.
├── gui/                # wxWidgets frames and dialogs
├── core/               # Subject/Observer, template processing, utilities
├── sql/                # SQL tokenizer, statement splitter, keyword sets
├── ibpp/               # IBPP library source (legacy)
└── fb-cpp/             # fb-cpp submodule (keep clean — patches in ports/fb-cpp/)
```

### DAL (Database Abstraction Layer)
All database access goes through pure-virtual interfaces in `src/engine/db/` under the `fr::` namespace. `DatabaseFactory` instantiates either the `FbCpp` or `IBPP` backend. `fb-cpp` is the current default. Smart pointer typedefs (`IDatabasePtr`, `ITransactionPtr`, etc.) are defined in `DatabaseBackend.h`.

### Metadata Object Model
`MetadataItem` is the base class for all database objects. The tree hierarchy (`Root → Server → Database → Tables → Table → Column`, etc.) uses a `NodeType` enum (`ntTable`, `ntView`, etc.) defined in `metadataitem.h`. Changes propagate via the **Subject/Observer** pattern (`src/core/Subject.h`, `Observer.h`).

### Visitor Pattern
DDL generation, HTML property rendering, and context menus are all implemented as Visitors inheriting from `MetadataItemVisitor` (GoF Visitor). To add behavior for a new object type, add `visitXxx()` to the visitor interface and implement it in each concrete visitor.

### GUI → Metadata → DAL Flow
GUI frames call metadata methods → metadata classes call `DatabaseFactory`-created DAL interfaces → DAL implementations call either fb-cpp or IBPP → Firebird client library (`libfbclient`).

### Template System
Metadata property views are rendered as HTML via a template processor (`src/core/TemplateProcessor.h`). Templates live in `html-templates/`. `MetadataItemPropertiesFrame` displays the rendered HTML.

## Code Style

- **PascalCase** for class names; **camelCase** for methods and variables.
- No leading or trailing underscores in variable names.
- Braces on `if`/`for`/`while` only when condition + body exceeds two lines; otherwise omit braces.
- Format C++ files with `clang-format`.
- Doxygen `///` comments; use blank `///` lines before and after doc blocks.
- RAII scopes should be commented: `{  // scope`.
- Lowercase numeric literal suffixes: `1u`, `3u` (not `1U`, `3U`).
- Do not use string literals in `assert()` (no `assert(ptr && "msg")`).
- Only define destructor as `= default` when necessary (virtual or translation-unit control).
- All DAL code lives in the `fr::` namespace.
- Every source file starts with the MIT license header. When creating new files, copy the header and update the Copyright year and author name.

## Key Files

| File | Purpose |
|------|---------|
| `src/frversion.h` | Version constants (`FR_VERSION_MAJOR/MINOR/RLS`) |
| `src/engine/db/DatabaseBackend.h` | All DAL shared types and enums |
| `src/metadata/MetadataClasses.h` | Forward declarations for all metadata types |
| `src/sql/firebird_keyword_sets.hpp` | Per-version Firebird keyword sets |
| `ports/fb-cpp/fb-cpp-flamerobin.patch` | Patches applied to fb-cpp submodule via vcpkg |
| `utils/sync-fbcpp.sh` | Regenerates the fb-cpp patch after submodule changes |

## fb-cpp Submodule

`src/fb-cpp/` is a git submodule that must stay unmodified. All FlameRobin-specific changes to fb-cpp are maintained as a patch in `ports/fb-cpp/fb-cpp-flamerobin.patch` and applied automatically by vcpkg. To update patches after changing the submodule, run `./utils/sync-fbcpp.sh`.
