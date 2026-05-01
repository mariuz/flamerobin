# FlameRobin Roadmap

This document outlines the planned development and major milestones for FlameRobin.

## Current Priority: Migration to fb-cpp (In Progress)

The primary goal is to replace the legacy and unmaintained IBPP library with `fb-cpp`, a modern C++20 wrapper for the Firebird database API. This will enable support for Firebird 3.0, 4.0, and 5.0 features.

**GitHub Issue:** [#542](https://github.com/mariuz/flamerobin/issues/542)

### Phase 1: Foundation & Build System (Completed)
- [x] Upgrade codebase to **C++20** standard.
- [x] Integrate **vcpkg** for dependency management.
- [x] Integrate **fb-cpp** as a git submodule.
- [x] Define the **Database Abstraction Layer (DAL)** interfaces (`IDatabase`, `ITransaction`, etc.).
- [x] Implement the **IBPP backend** for the DAL to ensure backward compatibility.
- [x] Implement the initial **fb-cpp backend** for the DAL.
- [x] Standardize the **CMake** build system across Windows, Linux, and macOS.

### Phase 2: Metadata & Core Refactoring (In Progress)
- [x] Systematic refactoring of `MetadataItem` classes to use DAL interfaces.
- [x] Implement `DECFLOAT` and `INT128` support in the DAL and UI.
- [ ] Improve Timezone handling using modern Firebird API.
- [ ] Refactor `Database` and `Server` classes to fully decouple from IBPP.

### Phase 3: SQL Editor & UI Integration
- [ ] Port the SQL execution engine to the DAL.
- [ ] Update DataGrid and other UI components to use DAL result sets.
- [ ] Refactor asynchronous service operations (Backup/Restore) to use `IService` DAL interface.

### Phase 4: Finalization
- [ ] Full validation of all FlameRobin features using the `fb-cpp` backend.
- [ ] Optional: Remove IBPP library from the source tree.
- [ ] Enhance performance by leveraging `fb-cpp`'s modern architecture.

---

For more details on the architectural changes, see [Migration to fb-cpp documentation](docs/migration-to-fb-cpp.md).
