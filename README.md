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

Key Features & Capabilities
---------------------------
FlameRobin is a lightweight, cross-platform database administration tool for the **Firebird DBMS** (supporting Firebird 2.5, 3.0, 4.0, 5.0, and 6.0+).

### 🚀 Developer Productivity & IDE Enhancements
- **SQL Schemas & Metadata Management**: Native support for Firebird 6.0+ SQL Schemas (`CREATE`, `ALTER`, `DROP SCHEMA`) and full schema qualification refactoring.
- **Visual JSON & Document Field Editor**: Interactive visual JSON tree viewer, pretty-printer, syntax validator, and Firebird 6 native JSON expression builder (`JSON_VALUE`, `JSON_QUERY`, `JSON_EXISTS`).
- **Temporal Tables & Point-In-Time Querying**: First-class GUI display for system-versioned temporal tables (`PERIOD FOR SYSTEM_TIME`) and automated historical query generator (`FOR SYSTEM_TIME AS OF` / `BETWEEN`).
- **"Script as ..." Context Menu Actions**: One-click DDL/DML code generators (`Script as SELECT FIRST 100`, `INSERT`, `UPDATE`, `DELETE`, `MERGE`, `CREATE`, `ALTER`, `DROP`).
- **Multi-Format Result Exports**: Export data grid query results to **CSV**, **JSON**, **TSV**, **Excel (XLSX)**, **HTML**, and **Markdown tables**.
- **Query Plan Visualizer**: Interactive execution plan viewer rendering Firebird `PLAN` and `EXPLAIN` statement tree structures with index usage vs. full-table scan warnings.
- **SQL:2023 Standard Compliance**: Full editor and metadata support for `DECFLOAT`, `INT128`, `ROW` data types, numeric literals with underscores (e.g. `1_000_000`), non-decimal literals (`0x`, `0b`, `0o`), and column-level `COLLATE` definitions.

### 🧠 Vector & AI Embedding Support (Firebird AI Extensions)
FlameRobin provides native data grid rendering, inspection, and similarity query helpers for high-dimensional vector embeddings (`VECTOR`):
- **Vector Data Viewer & Inspector** (`VectorDataEditorDialog`): Inspect dimensions, L2 norms, individual float elements, and perform interactive L2 normalization.
- **Similarity Search Query Generator**: Automatically generate similarity queries using Firebird vector functions (`COSINE_DISTANCE`, `L2_DISTANCE`, `INNER_PRODUCT`, `L1_DISTANCE`).
- **`fbvector` Installation Wizard** (`VectorInstallerWizard`): Built-in automated wizard to verify Firebird engine vector capabilities and auto-deploy/register the [`fbvector`](https://github.com/mariuz/fbvector) UDF extension package directly to connected Firebird databases.

### 📊 Connection Pool & Memory Diagnostics Dashboard
- **Live Session & Transaction Monitor**: Real-time monitoring window (`SessionMonitorFrame`) displaying active database attachments (`MON$ATTACHMENTS`), executing statements (`MON$STATEMENTS`), and running transactions (`MON$TRANSACTIONS`) with **Cancel Query** and **Disconnect Attachment** capabilities.
- **Memory Pools & Diagnostics**: Visual diagnostic graphs and tabular views for Firebird server memory pools (`MON$POOLS`) and memory usage statistics (`MON$MEMORY_USAGE` allocations, used bytes, and max limits per stat group).
- **Connection Pool Monitoring**: Track active vs. idle attachments, connection limits, and request wait queues across database sessions.
- **Compiled Statement Cache**: Inspect statement compilation metrics (`MON$COMPILED_STATEMENTS`), cache hits, misses, and hit ratio percentages.

### 🤖 Model Context Protocol (MCP) Server Integration
FlameRobin features an embedded **Model Context Protocol (MCP) Server** enabling direct integration with AI assistants (such as Claude Desktop, Gemini CLI, Cursor, and custom agentic coding assistants):
- **How to start the MCP server**:
  ```bash
  flamerobin --mcp
  ```
- **Supported MCP Tools**:
  - `list_databases`: List registered Firebird databases and connection details.
  - `get_schema`: Retrieve complete metadata schemas for tables, views, procedures, and indices.
  - `execute_query`: Run standard read-only or DML SQL statements.
  - `get_metadata_ddl`: Retrieve full DDL script representations of any metadata object.
  - `get_performance_stats`: Fetch session metrics, transaction counts, and page buffer allocations.
  - `get_memory_diagnostics`: Inspect Firebird memory pools (`MON$POOLS`), memory usage stats (`MON$MEMORY_USAGE`), and connection pool states.
  - `list_active_sessions`: Inspect connected users, running queries, and attachment metadata via `MON$` system tables.
  - `cancel_statement`: Send cancellation signals to long-running SQL statement IDs.
  - `recalculate_index_stats`: Trigger index selectivity recalculations (`SET STATISTICS INDEX`).
  - `explain_query`: Retrieve execution plans for query optimization.
  - `compare_schemas`: Automated schema diffing and DDL migration script generation.

For full MCP integration details, see [docs/mcp_howto.md](docs/mcp_howto.md) and [.agents/AGENTS.md](.agents/AGENTS.md).

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
[BUILD.md](https://github.com/mariuz/flamerobin/blob/master/BUILD.md) file.

**Notice:** FlameRobin is built using modern **C++20** and uses the modern `fb-cpp` database library abstraction layer.

Roadmap & Architecture
---------------------------
*   [Project Roadmap](ROADMAP.md) - Milestone tracking across Firebird 4.0, 5.0, 6.0, and IDE features.
*   [Security Roadmap](SECURITY_ROADMAP.md) - Security hardening and supply-chain safety improvements.
*   [Database Abstraction Layer (DAL) & fb-cpp Architecture](docs/migration-to-fb-cpp.md) - Detailed DAL design documentation.
*   [MCP How-To Guide](docs/mcp_howto.md) - Model Context Protocol integration instructions.

macOS distribution builds
---------------------------
For producing a signed, notarized `FlameRobin.app` distributable to other
Macs (build + bundle dylibs + sign + notarize + staple + zip in one step),
see [dist/macos/README.md](dist/macos/README.md). One-shot release:

```sh
dist/macos/release.sh
```

Wiki & Documentation
---------------------------
* [What's New](https://github.com/mariuz/flamerobin/blob/master/docs/fr_whatsnew.html)
* [FlameRobin Wiki](https://github.com/mariuz/flamerobin/wiki)
* [Using Firebird Embedded](docs/firebird_embedded.md)
* [Windows Portable Releases](docs/portable_release.md)
* [License](https://github.com/mariuz/flamerobin/blob/master/docs/fr_license.html)

Developer Documentation
---------------------------
Developer-focused documentation can be found in the [devdocs](https://github.com/mariuz/flamerobin/tree/master/devdocs) folder:

* [Creating Linux Packages](https://github.com/mariuz/flamerobin/blob/master/devdocs/Creating_Linux_Packages.txt)
* [Memory Leak Detection Roadmap](https://github.com/mariuz/flamerobin/blob/master/devdocs/memory_leaks_roadmap.md)
* [Files and Paths](https://github.com/mariuz/flamerobin/blob/master/devdocs/Files_and_paths.txt)

License
---------------------------
FlameRobin code is licensed under the MIT license.
A copy of the license can be found in the [LICENSE](https://github.com/mariuz/flamerobin/blob/master/LICENSE) file.

Part of code covering IBPP library is licensed under IBPP license.
A copy of IBPP license can be found in src/ibpp folder.

Internal fb-cpp Library
---------------------------
FlameRobin uses a patched version of the [fb-cpp](https://github.com/asfernandes/fb-cpp) library. These patches are necessary to support delayed transaction starts and transaction-less statement preparation, which are critical for FlameRobin's Data Access Layer (DAL).

The library is built using a custom vcpkg port located in `ports/fb-cpp/`. The patches are applied automatically during the vcpkg build process from `ports/fb-cpp/fb-cpp-flamerobin.patch`. This ensures that the upstream submodule in `src/fb-cpp/` remains clean.

### Updating Patches

If you need to update the patches (e.g., after updating the submodule):
1. Use the synchronization script to generate a new patch:
   ```bash
   ./utils/sync-fbcpp.sh
   ```
2. The script will now automatically update `ports/fb-cpp/fb-cpp-flamerobin.patch`.
3. Verify that the project still compiles and then commit the changes.
