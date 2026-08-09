# FlameRobin Completed Roadmap

This document archives the completed development phases and milestones for FlameRobin.

## Priority: Migration to fb-cpp (Completed)

The legacy IBPP library has been replaced by `fb-cpp` as the default engine. This modern C++20 wrapper for the Firebird database API enables full support for Firebird 3.0, 4.0, and 5.0 features.

### Phase 1: Foundation & Build System (Completed)
- [x] Upgrade codebase to **C++20** standard.
- [x] Integrate **vcpkg** for dependency management.
- [x] Integrate **fb-cpp** as a git submodule.
- [x] Define the **Database Abstraction Layer (DAL)** interfaces (`IDatabase`, `ITransaction`, etc.).
- [x] Implement the **IBPP backend** for the DAL to ensure backward compatibility.
- [x] Implement the initial **fb-cpp backend** for the DAL.
- [x] Standardize the **CMake** build system across Windows, Linux, and macOS.

### Phase 2: Metadata & Core Refactoring (Completed)
- [x] Systematic refactoring of `MetadataItem` classes to use DAL interfaces.
- [x] Implement `DECFLOAT` and `INT128` support in the DAL and UI.
- [x] Improve Timezone handling using modern Firebird API.
- [x] Refactor `Database` and `Server` classes to fully decouple from IBPP.

### Phase 3: SQL Editor & UI Integration (Completed)
- [x] Port the SQL execution engine to the DAL.
- [x] Update DataGrid and other UI components to use DAL result sets.
- [x] Refactor asynchronous service operations (Backup/Restore) to use `IService` DAL interface.

### Phase 4: Finalization (Completed)
- [x] Full validation of all FlameRobin features using the `fb-cpp` backend.
- [x] Set `fb-cpp` as the default database engine.
- [x] Optional: Remove IBPP library from the source tree.
- [x] Enhance performance by leveraging `fb-cpp`'s modern architecture.

---

## Firebird Feature Support (Completed)

The following phases track FlameRobin UI/metadata support for features introduced in each major Firebird release.

### Phase 5: Firebird 4.0 Feature Support (Completed)
- [x] **Long Identifier Names (63 chars)** — Update the UI to allow identifiers up to 63 characters; remove hard-coded limits.
- [x] **Named Time Zone Display** — Show IANA-named time zones in column metadata and property views.
- [x] **`DECFLOAT` and `INT128` DDL Generation** — Ensure `CREATE TABLE` / `CREATE DOMAIN` templates emit correct DECFLOAT/INT128 syntax.
- [x] **Scrollable Cursor Display** — Show `SCROLL` attribute in stored-procedure and trigger DDL.
- [x] **Read Committed Read Consistency** — Display transaction isolation level in transaction info.
- [x] **Database Encryption Status** — Show whether database is encrypted in properties.
- [x] **Built-in Replication Monitoring** — Display `RDB$PUBLICATIONS` in metadata tree.
- [x] **SQL Syntax Highlighting for FB4 Keywords** — Activate the FB4 keyword set.

### Phase 6: Firebird 5.0 Feature Support (Completed)
- [x] **Partial (Conditional) Index DDL** — Emit `WHERE <condition>` in generated DDL.
- [x] **Parallel Operations UI** — Parallel worker count in Backup/Restore/Sweep.
- [x] **`SKIP LOCKED` Clause Highlighting** — Add keyword highlighting.
- [x] **`LATERAL` JOIN Support** — Support tokenizer/parser parsing.
- [x] **`MERGE … WHEN NOT MATCHED BY SOURCE`** — Support statement recognition.
- [x] **Multiple-Row DML `RETURNING`** — Support displaying results in grid.
- [x] **SQL / PSQL Profiler Integration** — UI panel to run/display output.
- [x] **Inline ODS Upgrade Option** — Upgrade ODS without backup/restore.
- [x] **Compiled Statement Cache Visibility** — Display cache hit/miss stats.

### Phase 7: Firebird 6.0 Feature Support (Completed)
- [x] **SQL Schemas** — Restructure metadata tree, dialogs, qualification.
- [x] **JSON Functions** — Highlighting and autocomplete.
- [x] **Tablespaces** — Create/display tablespaces.
- [x] **`EXPLAIN` Statement** — Native execution plans.
- [x] **Named Arguments for Procedure/Function Calls** — Editor support.
- [x] **`GREATEST` / `LEAST` Functions** — Autocomplete.
- [x] **`UNLIST` Function** — Autocomplete.
- [x] **`ANY_VALUE` Aggregate** — Autocomplete.
- [x] **SQL-Standard `ROW` Data Type** — DDL/PSQL support.
- [x] **Underscores in Numeric Literals** — Editor token support.
- [x] **Collation as Data Type Property** — Display/manage column collations.
- [x] **Enhanced SQL Security Management** — UI for definer/invoker settings.
- [x] **Optional String Max Lengths** — Implicit-length parsing.
- [x] **CSV External Tables** — CSV-backed tables support.
- [x] **Enhanced Security: Owner Assignment on CREATE DATABASE** — Owner option in DB dialog.

### Phase 8: IDE & SQL Experience Enhancements (Completed)
- [x] **"Script as ..." Context Actions** — Right-click script generation.
- [x] **Multi-Format Result Set Export** — Export to JSON, Excel, Markdown, TSV.
- [x] **Query Result Filtering & Search** — Filter/sort in results grid.
- [x] **Environment Color Coding & Connection Profiles** — Environment profile tags.
- [x] **Quick Connection Switcher in Editor** — Swapper in Execution Frame.
- [x] **Visual / Tree Query Execution Plan** — Interactive plan visualization tree.
- [x] **Database Schema Comparison & Migration Generator** — Compare schema and generate migration DDL.
- [x] **Enhanced Model Context Protocol (MCP) Tools** — Extended tool set.

### Phase 9: PostgreSQL-Inspired Developer Tools & Monitoring (Completed)
- [x] **Live Session & Transaction Lock Monitor** — Monitoring dashboard with cancel/disconnect functions.
- [x] **PSQL Routine Parameter Helper** — Template generator/autocomplete.
- [x] **Database Maintenance & Health Dashboard** — Health index/maintenance options.
- [x] **Query Execution Statistics & Buffer Metrics** — Read/write statistics in SQL frame.
- [x] **Wire Encryption & Security Protocol Status** — Active encryption plugin reporting.
- [x] **Interactive Parameterized Routine Executor** — Validation/execution dialog.
- [x] **Expanded MCP Monitoring & Session Tools** — Connection/session endpoints.

### Phase 10: Next-Gen Firebird Engine Features & Advanced GUI Roadmap (Completed)
- [x] **Interactive Schema DDL Generator** — Visualization for Schema objects.
- [x] **JSON & Document Field Editor** — JSON formatting tree viewer/editor.
- [x] **Vector & AI Embedding Support** — AI vector inspector & `fbvector` installer.
- [x] **Temporal Tables & System-Time Versioning** — Period-based tables.
- [x] **Connection Pool & Memory Diagnostics** — Graph and diagnostics tool.
- [x] **Granular System Privilege Security Matrix** — Management matrix dialog.
- [x] **Automated Database Backup Schedule & Cloud Sync Manager** — Backup options UI.

---

### Cross-Cutting Work (Completed)
- [x] **Version Detection Improvement** — Named constants for ODS version gating.
- [x] **Keyword-Set Selection by Server Version** — Keyword sets mapped to connected DB.
- [x] **System Table Query Audit** — Audit system tables queries for FB4/5/6 fields.
- [x] **SVG Icon System & HiDPI Support** — Crisp vector rendering system.
- [x] **Documentation / What's New Page** — User-facing updates docs.
- [x] **Memory Leak Detection Roadmap** — Memory leak guides and practices.
