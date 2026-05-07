# FlameRobin Roadmap

This document outlines the planned development and major milestones for FlameRobin.

## Priority: Migration to fb-cpp (Completed)

The legacy IBPP library has been replaced by `fb-cpp` as the default engine. This modern C++20 wrapper for the Firebird database API enables full support for Firebird 3.0, 4.0, and 5.0 features.

**GitHub Issue:** [#542](https://github.com/mariuz/flamerobin/issues/542)

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
- [ ] Optional: Remove IBPP library from the source tree.
- [x] Enhance performance by leveraging `fb-cpp`'s modern architecture.

---

For more details on the architectural changes, see [Migration to fb-cpp documentation](docs/migration-to-fb-cpp.md).

---

## Current Focus: Firebird Feature Support

The following phases track FlameRobin UI/metadata support for features introduced in each major Firebird release. ODS version mapping: FB 2.5 → ODS 11.1, FB 3.0 → ODS 12.0, FB 4.0 → ODS 13.0, FB 5.0 → ODS 13.1, FB 6.0 → ODS 14.x.

### Phase 5: Firebird 4.0 Feature Support (ODS 13.x, released June 2021)

- [x] **Long Identifier Names (63 chars)** — Update the UI (tree, DDL editor, all dialogs) to allow identifiers up to 63 characters; remove any hard-coded 31-character limits inherited from Firebird 2.x/3.x.
- [x] **Named Time Zone Display** — Show IANA-named time zones (e.g., Europe/Berlin, America/New_York) in column metadata and property views, not just UTC offsets.
- [x] **`DECFLOAT` and `INT128` DDL Generation** — Ensure `CREATE TABLE` / `CREATE DOMAIN` DDL templates emit correct `DECFLOAT(16)`, `DECFLOAT(34)`, and `INT128` syntax (data type support in the DAL already exists).
- [x] **Scrollable Cursor Display** — Show the `SCROLL` attribute in stored-procedure and trigger DDL when present.
- [x] **Read Committed Read Consistency** — Display the new transaction isolation level in transaction info and monitoring views.
- [x] **Database Encryption Status** — Show whether a database is encrypted in the database properties panel.
- [x] **Built-in Replication Monitoring** — Display `RDB$PUBLICATIONS` / `RDB$PUBLICATION_TABLES` system tables (ODS 13.0) in the metadata tree under a dedicated *Replication* node.
- [x] **SQL Syntax Highlighting for FB4 Keywords** — Activate the FB4 keyword set (already defined in `firebird_keyword_sets.hpp`) based on the connected server version.

### Phase 6: Firebird 5.0 Feature Support (ODS 13.1, released January 2024)

- [x] **Partial (Conditional) Index DDL** — Emit `WHERE <condition>` in generated DDL for partial indexes (`rdb$condition_source` is already read; the DDL output path needs to include it).
- [x] **Parallel Operations UI** — Expose a *parallel worker count* option in the Backup, Restore, and Sweep dialogs (`ParallelWorkers` service parameter).
- [x] **`SKIP LOCKED` Clause Highlighting** — Add `SKIP LOCKED` to SQL keyword/syntax highlighting.
- [x] **`LATERAL` JOIN Support** — Ensure the SQL editor tokenizer/parser handles `LATERAL` joins without producing false "incomplete statement" warnings.
- [x] **`MERGE … WHEN NOT MATCHED BY SOURCE`** — Update the SQL statement recognizer to handle the extended MERGE syntax introduced in FB5.
- [x] **Multiple-Row DML `RETURNING`** — Support displaying multi-row results from `INSERT … RETURNING`, `UPDATE … RETURNING`, and `DELETE … RETURNING` in the results grid.
- [x] **SQL / PSQL Profiler Integration** — Add a UI panel or menu option to run and display output from the Firebird 5 built-in profiler (`RDB$PROFILER` package).
- [x] **Inline ODS Upgrade Option** — Expose the *upgrade ODS without backup/restore* option in the database maintenance dialogs.
- [ ] **Compiled Statement Cache Visibility** — Optionally display cache hit/miss statistics from `MON$COMPILED_STATEMENTS`.

### Phase 7: Firebird 6.0 Feature Support (ODS 14.x, planned ~2026)

- [ ] **SQL Schemas** — Restructure the metadata tree to show schemas as a first-class container; generate schema-qualified DDL (`SCHEMA.OBJECT`); add CREATE / ALTER / DROP SCHEMA management dialogs.
- [x] **JSON Functions** — Add keyword highlighting and code completion for native JSON functions (`JSON_VALUE`, `JSON_QUERY`, `JSON_OBJECT`, `JSON_ARRAY`, etc.).
- [ ] **Tablespaces** — Display tablespace assignments in table and index properties; expose tablespace selection in CREATE TABLE / CREATE INDEX dialogs.
- [x] **`EXPLAIN` Statement** — Show query execution plans from the new `EXPLAIN` statement (distinct from the `PLAN` clause) in the SQL execution frame.

- [x] **Named Arguments for Procedure/Function Calls** — SQL editor support and code completion for named-argument call syntax (`proc(arg1 => val1)`).
- [x] **`GREATEST` / `LEAST` Functions** — Add to SQL keyword completion (SQL:2023).
- [x] **`UNLIST` Function** — Add to SQL keyword completion for string-splitting into result rows.
- [x] **`ANY_VALUE` Aggregate** — Add to SQL keyword completion (SQL:2023).
- [ ] **SQL-Standard `ROW` Data Type** — Support for defining and using `ROW` type variables and columns in PSQL and DDL.
- [ ] **Underscores in Numeric Literals** — Support for `1_000_000` style numeric literals and non-decimal integer literals (SQL:2023) in the SQL editor.
- [ ] **Collation as Data Type Property** — Display and manage collation defined directly on table columns as part of the data type (SQL:2023).
- [ ] **Enhanced SQL Security Management** — UI for `ALTER ... SQL SECURITY {DEFINER | INVOKER}` across all applicable metadata objects.
- [ ] **Optional String Max Lengths** — Ensure `VARCHAR` / `CHAR` column DDL handles implicit-length syntax per SQL:2023.
- [ ] **CSV External Tables** — UI support for defining external tables backed by CSV files when the engine supports it.
- [x] **Enhanced Security: Owner Assignment on CREATE DATABASE** — Expose ownership and initial-user options in the Create Database dialog.

---

## Cross-Cutting Work

These items apply across Firebird versions and support the feature phases above.

- [ ] **Version Detection Improvement** — Replace scattered hard-coded ODS comparisons with named constants (e.g., `ODS_FB4 = 13.0`, `ODS_FB5 = 13.1`, `ODS_FB6 = 14.0`) so version gates are readable and easy to update.
- [ ] **Keyword-Set Selection by Server Version** — Wire the existing per-version keyword sets (`fb25_keywords`, `fb40_keywords`, `fb50_keywords`, `fb60_keywords` in `firebird_keyword_sets.hpp`) to the detected connected server version for accurate syntax highlighting and auto-complete.
- [ ] **System Table Query Audit** — Audit all `MON$*` and `RDB$*` queries to add newly available columns from FB4/5/6 where the ODS version allows.
- [ ] **Documentation / What's New Page** — Update `docs/fr_whatsnew.html` with entries for each Firebird version's newly supported FlameRobin features.
