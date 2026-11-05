# FlameRobin Roadmap

This document outlines the active roadmap and pending tasks for FlameRobin development.

## 📊 Status Summary
- **Core Feature Roadmap (Phases 1-10):** **100% Completed** (Archived in [ROADMAP_DONE.md](ROADMAP_DONE.md))
- **Security & Distribution Integrity Roadmap:** **0% Completed** (Outstanding / Active TODOs listed below, including Windows code signing, Flatpak/Flathub, and a signed apt repository)
- **Performance & Speed Optimization Roadmap (Phase 11):** **0% Completed**
- **Community UX & Developer Workflow Roadmap (Phase 12):** **0% Completed**

---

## 🔒 Security & Release Integrity Roadmap (Active TODOs)

This section tracks security-focused work inspired by [discussion #591](https://github.com/mariuz/flamerobin/discussions/591) about supply-chain and distribution risks.

### 1) Threat Model and Risk Assessment
- [ ] Define and publish a lightweight threat model for FlameRobin releases and distribution.
- [ ] Document priority risks (release asset tampering, compromised dependencies, CI/CD compromise, website compromise).
- [ ] Review and update the threat model on each major release cycle.

### 2) Release Integrity and Provenance
- [ ] Publish SHA-256 checksums for all release artifacts.
- [ ] Add detached signatures for release artifacts and document signature verification steps.
- [ ] Ensure release artifacts are published only from protected/reviewed workflows.
- [ ] Evaluate GitHub artifact attestations / provenance for release builds.

### 3) Dependency and Build Supply Chain
- [ ] Keep dependency versions pinned where possible (including GitHub Actions).
- [ ] Establish a routine dependency vulnerability review process (vcpkg and other third-party components).
- [ ] Generate and publish an SBOM for release artifacts.
- [ ] Investigate reproducible-build practices for key release targets.

### 4) CI/CD and Repository Hardening
- [ ] Enforce branch protection and required status checks for release-related branches.
- [ ] Require CODEOWNERS or maintainer review for workflow and release pipeline changes.
- [ ] Minimize GitHub token/workflow permissions to least privilege.
- [ ] Enable and monitor GitHub security features (Dependabot alerts, code scanning, secret scanning where applicable).

### 5) Website and Distribution Channel Security
- [ ] Document the trusted official distribution channels (GitHub Releases, flamerobin.org).
- [ ] Add a user-facing checklist for verifying downloads before installation.
- [ ] Review hosting and publication controls for flamerobin.org and linked assets.

### 6) Vulnerability Handling and Incident Response
- [ ] Add a `SECURITY.md` vulnerability disclosure policy if not already present.
- [ ] Define a response playbook for compromised artifacts or credentials.
- [ ] Define communication steps for security advisories and emergency rebuilds/revocations.

### 7) User Documentation
- [ ] Document the current update model (manual download/install, no in-app auto-updater).
- [ ] Add a "How to verify a release" section to project documentation.
- [ ] Add a short security FAQ based on recurring community questions.

### 8) Signed apt Repository for Debian and Ubuntu
Today the `.deb` is installed from a file downloaded from GitHub Releases (see [docs/install_linux.md](docs/install_linux.md)), so users have to download and install every new version by hand. [Claude's Debian page](https://code.claude.com/docs/en/desktop-linux) centres on a signed apt repository instead, so updates arrive with the system's regular `apt update && apt upgrade`. FlameRobin could have that too, hosted on GitHub Pages and filled by the release workflow.
- [ ] **Maintainer:** create a dedicated GPG signing key for the repository, publish its fingerprint, and store the private key and passphrase as GitHub Actions secrets.
- [ ] Publish the repository on GitHub Pages (a `gh-pages` branch or Pages deployment): `dists/stable/…` with a signed `InRelease`/`Release.gpg`, the `pool/` of `.deb` files, and the public key as `key.asc`.
- [ ] Extend `release.yml` to add each new `.deb` to the pool, regenerate the indexes (`dpkg-scanpackages` / `apt-ftparchive`) and sign them with the key from the secrets.
- [ ] Optionally let the `.deb` register the repository on install, as Claude Desktop's does, so a `.deb` installed from a downloaded file also receives updates.
- [ ] Document it in `docs/install_linux.md`: add the key, verify its fingerprint, register the repository with `signed-by=`, `apt install flamerobin`; update with `apt upgrade`; uninstall including the repository entry and key.

### 9) Windows Code Signing & Linux Flatpak / AppImage Distribution
- [ ] **Windows Authenticode Code Signing ([#746](https://github.com/mariuz/flamerobin/issues/746)):** Integrate automated code signing for `flamerobin.exe` and the Inno Setup installer (via SignPath Foundation for open-source projects or Firebird Foundation certificate) to prevent Windows SmartScreen "Unknown Publisher" warnings and antivirus false positives.
- [ ] **Flathub / Flatpak & AppImage Packaging ([#438](https://github.com/mariuz/flamerobin/issues/438)):** Publish an official Flatpak manifest (with bundled `libfbclient`) on Flathub so users on immutable Linux distributions (Fedora Silverblue, openSUSE MicroOS, SteamOS) and modern desktop environments can install and update FlameRobin seamlessly.
- [ ] **CI AddressSanitizer & LeakSanitizer Gate:** Enforce `ENABLE_ASAN=ON` with LeakSanitizer on CTest suites in GitHub Actions (per [devdocs/memory_leaks_roadmap.md](devdocs/memory_leaks_roadmap.md)).

---

## ⚡ Phase 11: Performance & Speed Optimization Roadmap

### 1) Metadata Loading & Connection Startup Speed
- [ ] **Batch Metadata Prefetching (Eliminate N+1 Queries):** Replace per-object catalog queries (columns, parameters, constraints, descriptions) with batched/bulk `RDB$` queries per collection or schema in `MetadataLoader` to drastically reduce round-trip latency over remote/WAN connections.
- [ ] **Asynchronous Connection & Tree Population:** Move initial database connection handshakes and metadata tree population to background worker threads so slow or high-latency connections never block the main wxWidgets UI thread.
- [ ] **Prepared Metadata Statement Caching:** Keep frequently executed system catalog queries prepared across metadata refreshes in `MetadataLoader`.

### 2) DataGrid & Large Result Set Speed
- [ ] **Lazy Cell Formatting & Zero-Copy Grid Buffer:** Format cell values into `wxString` lazily on paint (only for visible rows in `DataGridRows` / `DataGridRowBuffer`) to reduce memory allocations and UTF-8 conversion overhead on wide or large result sets.
- [ ] **Streaming Result Set Exports:** Stream large exports (CSV, JSON, Markdown, TSV, HTML) directly from the database statement cursor without materializing the entire dataset in the GUI grid buffer first.
- [ ] **Background Prefetch Windowing:** Prefetch upcoming row chunks asynchronously while scrolling through large result sets.
- [ ] **Lazy BLOB Previewing:** Fetch only BLOB sub-type headers and the first *N* preview bytes for inline DataGrid cells, deferring full payload loading until `EditBlobDialog` is opened.

### 3) SQL Editor & Autocomplete Responsiveness
- [ ] **Incremental & Scoped SQL Tokenization:** Tokenize only the active statement or visible editor range during typing, brace matching, and autocomplete in `ExecuteSqlFrame` so multi-megabyte SQL scripts remain responsive with near-zero typing latency.
- [ ] **Non-Blocking Query Cancellation:** Provide an immediate cancel action and elapsed timer in the SQL editor using Firebird's `fb_cancel_operation(fb_cancel_raise)` without freezing the UI thread.

---

## 🛠️ Phase 12: Community UX & Developer Workflow Roadmap

### 1) Open Community Feature Requests
- [ ] **Live Tree Filter Bar in Main Window ([#239](https://github.com/mariuz/flamerobin/issues/239)):** Add a quick-filter search box (with a clear `X` button) at the top of `MainFrame` to filter across servers, databases, and connected metadata items in real time by hiding non-matching nodes.
- [ ] **Quick Table Row Count Action ([#748](https://github.com/mariuz/flamerobin/issues/748)):** Add a **"Count records"** context menu item on tables (between *Browse data* and *Script as...*) that executes `SELECT COUNT(*) FROM <table_name>`, displays the count in the tree node label (e.g. `MyTable (12) = 235433 rows`), and invalidates on refresh or disconnect.
- [ ] **`*` Wildcard Field Entry & Shortcuts in Code Generation Dialog ([#749](https://github.com/mariuz/flamerobin/issues/749)):** Add `*` as the first selectable entry in the column selection list (emitting `alias.*` and auto-deselecting individual columns), add `F1`/`F2` keyboard shortcuts for toggling column checkboxes, and fix `[TAB]` focus wrapping from `[Cancel]` back to the Table alias input.

### 2) Core IDE & Architecture Enhancements
- [ ] **Dependency-Ordered Full DDL Extraction:** Build a topological dependency graph for views, computed columns, domains, packages, and procedures in `CreateDDLVisitor` so extracted database DDL scripts execute cleanly in a single pass without forward-reference errors.
- [ ] **Editable Multi-Table / JOIN Result Sets in DataGrid:** Track underlying relation and primary key provenance per column in `DataGrid` / `DataGridRows` so users can inline-edit or delete rows directly from queries containing `JOIN`s.
- [ ] **Per-Connection Client Library (`fbclient`) Selector UI:** Enable the per-connection client library text control and file browser in `DatabaseRegistrationDialog` so users can select a specific `fbclient.dll` / `libfbclient.so` / `.dylib` (e.g., Firebird 3.0, 4.0, 5.0, or 6.0 Embedded) per registered database in the GUI.
- [ ] **Package Member Autocomplete:** Complete `PACKAGE.PROCEDURE` and `PACKAGE.FUNCTION` member resolution in the SQL editor autocomplete engine.
