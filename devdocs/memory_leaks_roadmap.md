# C++ Memory Leak Detection — Roadmap & Integration Guide

This document covers the **practical integration** of Microsoft's three primary memory diagnostics tools into FlameRobin, plus Google-recommended best practices and a phased roadmap.

---

## 🗺️ Phased Roadmap

| Phase | Goal | Tools |
|-------|------|-------|
| **1 – Static Analysis** | Catch bugs before running | Clang-Tidy, MSVC `/analyze`, MSVC Code Analysis |
| **2 – Compile-Time Instrumentation** | Detect leaks & corruption at runtime | **ASan** (`/fsanitize=address` / `-fsanitize=address,leak`) |
| **3 – Debug-Heap Reporting** | File+line attribution of CRT allocations | **CRT Debug Heap** (`crtdbg.h`, `_CrtDumpMemoryLeaks`) |
| **4 – Interactive Profiling** | Visualise live heap deltas in the IDE | **VS Diagnostic Tools** (Memory Usage profiler) |
| **5 – CI / CD Gates** | Prevent regressions on every PR | ASan + LSan in CI, exit-code policy |

---

## Tool 1 — AddressSanitizer in MSVC (`/fsanitize=address`)

> **Microsoft Docs:** https://learn.microsoft.com/cpp/sanitizers/asan

ASan instruments every heap allocation and access at compile time.  It detects:
* **Heap-buffer overflows / underflows**
* **Use-after-free** and **use-after-scope**
* **Double-free / invalid-free**
* **Memory leaks** (via LeakSanitizer, bundled on Linux/macOS)

### How it is wired into FlameRobin

The `CMakeLists.txt` at the repository root exposes an opt-in CMake option:

```cmake
option(ENABLE_ASAN "Enable AddressSanitizer (ASan) for memory error detection" OFF)
```

When turned on it:
1. Strips `/RTC1` (incompatible with ASan on MSVC) from the Debug flags.
2. Adds `/fsanitize=address` to every compilation unit.
3. Adds `/INCREMENTAL:NO` to the linker (required by ASan on MSVC).
4. On GCC/Clang adds `-fsanitize=address,leak -fno-omit-frame-pointer`.

### Usage

**Windows (MSVC, Visual Studio 2019+)**
```powershell
# Configure
cmake -B build -DENABLE_ASAN=ON

# Build Debug
cmake --build build --config Debug

# Run — violations are printed to stderr and the VS Output window
build\Debug\flamerobin.exe
```

**Linux / macOS (GCC or Clang)**
```bash
cmake -B build -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/flamerobin
```

### Reading ASan output

A heap-buffer-overflow looks like:
```
==12345==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000050
READ of size 4 at 0x602000000050 thread T0
    #0 0x401234 in DataGridTable::getRowCount src/gui/DataGrid.cpp:128
    #1 0x402abc in ExecuteSqlFrame::execute src/gui/ExecuteSqlFrame.cpp:3006
```

### Caveats on MSVC
* ASan requires the **Debug** or **RelWithDebInfo** configuration.
* Do **not** mix ASan-instrumented and non-instrumented DLLs in the same process.
* The MSVC runtime must be the **DLL** variant (`/MD` / `/MDd`) — the static (`/MT`) variant is not supported by MSVC ASan.
* Disable any security mitigations (e.g. `/guard:cf`) that interfere with ASan's shadow memory; CMake handles this automatically when the option is ON.

---

## Tool 2 — CRT Debug Heap (`crtdbg.h` / `_CrtDumpMemoryLeaks`)

> **Microsoft Docs:** https://learn.microsoft.com/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library

The CRT debug heap replaces `malloc`/`free` with instrumented versions that track every allocation together with its **source file and line number**.  On program exit, `_CrtDumpMemoryLeaks()` writes a report to the VS **Output** window and/or `stderr`.

### How it is wired into FlameRobin

A second CMake option activates this feature only in MSVC Debug builds:

```cmake
option(ENABLE_CRT_LEAK_CHECK "Enable CRT Debug Heap leak reporting on exit (MSVC Debug builds only)" OFF)
```

When ON, the preprocessor symbol `FR_CRT_LEAK_CHECK` is defined for Debug builds.  `src/main.cpp` then:

1. **Includes the required headers** (must happen before any other CRT header):
   ```cpp
   #if defined(FR_CRT_LEAK_CHECK) && defined(_MSC_VER) && defined(_DEBUG)
       #define _CRTDBG_MAP_ALLOC
       #include <cstdlib>
       #include <crtdbg.h>
       #define FR_CRT_LEAK_CHECK_ACTIVE 1
   #endif
   ```

2. **Arms the heap flags** in `Application::OnInit()`:
   ```cpp
   #ifdef FR_CRT_LEAK_CHECK_ACTIVE
       _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
       _CrtSetReportMode(_CRT_WARN,  _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
       _CrtSetReportFile(_CRT_WARN,  _CRTDBG_FILE_STDERR);
       // ... (error and assert modes set similarly)
   #endif
   ```

3. **Dumps leaks** in `Application::OnExit()`:
   ```cpp
   #ifdef FR_CRT_LEAK_CHECK_ACTIVE
       if (_CrtDumpMemoryLeaks())
           wxLogDebug("[FlameRobin] CRT leak check: memory leaks detected");
       else
           wxLogDebug("[FlameRobin] CRT leak check: no memory leaks detected");
   #endif
   ```

### Usage

```powershell
cmake -B build -DENABLE_CRT_LEAK_CHECK=ON
cmake --build build --config Debug
build\Debug\flamerobin.exe
```

### Reading the leak report

Each leaked block prints in the VS **Output** window:
```
{c:\path\to\source\file.cpp(42)} normal block at 0x00A83C80, 48 bytes long.
 Data: <  example data  > 00 01 02 03 04 05 06 07 ...
```

> **Note:** `_CRTDBG_MAP_ALLOC` **must** be defined before the first CRT `#include` for source-file/line attribution.  FlameRobin's `main.cpp` does this correctly because it is the application entry point.

### Targeting a specific allocation

If you see the same block leaking repeatedly you can break-on-alloc:
```cpp
// In a Debug session, find the allocation number in the Output, e.g. {42}
_CrtSetBreakAlloc(42);  // debugger will break at that malloc call
```

---

## Tool 3 — Visual Studio Diagnostic Tools (Memory Usage Profiler)

> **Microsoft Docs:** https://learn.microsoft.com/visualstudio/profiling/memory-usage

The **Memory Usage** tool in Visual Studio provides an interactive, snapshot-based heap profiler — no recompilation required.

### Opening the tool

1. Open `build/flamerobin.slnx` in Visual Studio.
2. Set configuration to **Debug**.
3. Go to **Debug → Performance Profiler** (`Alt+F2`).
4. Check **Memory Usage** → click **Start**.

### Workflow for FlameRobin leak analysis

| Step | Action | What to look for |
|------|--------|-----------------|
| Baseline | Click **Take snapshot** before the suspect operation | Heap size at rest |
| Trigger | Execute the SQL / open the database / close dialogs | — |
| After | Click **Take snapshot** again | Delta vs baseline |
| Compare | Click the **▲ / diff** link between snapshots | Object types that grew |
| Drill down | Click an object type | Allocation call stacks |

### What the diff view shows

* **Count Diff** — how many more live objects of that type exist after the operation.
* **Size Diff** — extra bytes retained.
* **Call stack** — exactly which code path allocated the surviving objects.

### Tips for FlameRobin

* Take a snapshot **after** the main window is fully loaded to get a clean baseline.
* Open and close the SQL editor several times — growing `DataGridTable` or `Statement` counts indicate retained objects.
* Closing a database connection should collapse related metadata objects; leftover `MetadataItem` or `Database` entries in the diff are prime leak suspects.

---

## Google-Recommended Additions

### LeakSanitizer (LSan) — standalone on Linux
```bash
# Run without recompiling (LD_PRELOAD approach)
LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libasan.so.5 \
ASAN_OPTIONS=detect_leaks=1 \
./build/flamerobin
```

### Valgrind Memcheck (Linux/macOS)
```bash
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --log-file=valgrind.log \
         ./build/flamerobin
```

---

## CI/CD Gating

Add to your GitHub Actions workflow:

```yaml
- name: Configure with ASan
  run: cmake -B build -DENABLE_ASAN=ON -DCMAKE_BUILD_TYPE=Debug

- name: Build
  run: cmake --build build

- name: Run tests (leak-check enforced)
  env:
    ASAN_OPTIONS: "detect_leaks=1:halt_on_error=1"
    LSAN_OPTIONS: "suppressions=.lsan-suppressions.txt"
  run: ctest --test-dir build -C Debug --output-on-failure
```

Any leak causes the process to exit non-zero, which fails the CI job.

---

## C++ Best Practices (Quick Reference)

| Rule | Example |
|------|---------|
| Prefer `std::unique_ptr` for sole ownership | `auto p = std::make_unique<Widget>();` |
| Use `std::shared_ptr` only when shared | `auto p = std::make_shared<Database>();` |
| Break cycles with `std::weak_ptr` | `std::weak_ptr<MetadataItem> parent_;` |
| Never `delete` a raw pointer you don't own | Pass `T*` as *observer*, not *owner* |
| Use standard containers | `std::vector`, `std::string` — not manual arrays |
| wxWidgets: let parent windows own children | `new wxButton(this, ...)` — `this` deletes it |
| wxWidgets: call `Destroy()` on non-modal dialogs | Avoids event-loop use-after-free |

---

*Last updated: 2026-06-28. See also [ROADMAP.md](../ROADMAP.md) and [SECURITY_ROADMAP.md](../SECURITY_ROADMAP.md).*
