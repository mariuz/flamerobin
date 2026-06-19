# How to Debug FlameRobin Crashes using Crashpad

Google Crashpad is integrated into FlameRobin on Windows to automatically capture process state and write minidump crash reports when a fatal crash or exception is encountered.

This guide explains how to locate, configure, and debug these minidumps.

---

## 1. Where are the Minidumps Saved?

Crashpad writes minidump files (with `.dmp` extension) locally. They are stored inside the FlameRobin user local data directory:

* **Windows**: `%LOCALAPPDATA%\FlameRobin\crashes\pending` or `%LOCALAPPDATA%\FlameRobin\crashes\completed`
  *(Typically: `C:\Users\<Username>\AppData\Local\FlameRobin\crashes`)*

Inside this directory, Crashpad maintains a structured database containing metadata and the raw `.dmp` files.

---

## 2. Triggering a Dump Programmatically

Whenever FlameRobin catches a fatal error via its `parachute()` handler (for unhandled C++ exceptions or through `std::set_terminate`), it programmatically captures the current CPU context and writes a minidump:

```cpp
#ifdef FR_USE_CRASHPAD
    CONTEXT context;
    RtlCaptureContext(&context);
    crashpad::CrashpadClient::DumpWithoutCrash(context);
#endif
```

For hard crashes (such as Access Violations / Null Pointer Dereferences / Segfaults), the Crashpad Out-of-Process handler automatically intercepts the exception, writes a minidump, and terminates the process.

---

## 3. How to Debug a Minidump (.dmp)

To debug a captured minidump and view the exact call stack of the crash:

### Using Visual Studio (Recommended on Windows)
1. **Open the Dump**: Launch Visual Studio and open the `.dmp` file directly (`File -> Open -> File...`).
2. **Configure Symbols**:
   * To get a meaningful call stack, Visual Studio needs the debug symbols (`.pdb` files) generated during the build.
   * Go to `Tools -> Options -> Debugging -> Symbols`.
   * Add the path to the directory containing your compiled `flamerobin.pdb` (e.g., `<flamerobin-source-dir>\build\Debug`).
3. **Start Debugging**:
   * In the **Minidump Summary** screen in Visual Studio, look at the **Actions** pane on the right.
   * Click **Debug with Native Only**.
4. **View the Call Stack**:
   * Visual Studio will load the dump, resolve the call stack using the PDBs, and point you directly to the file and line of code that caused the crash.

### Using WinDbg
1. Open WinDbg.
2. Select `File -> Open Crash Dump...` and choose the `.dmp` file.
3. Configure the symbol path:
   ```cmd
   .sympath+ C:\Path\To\flamerobin\build\Debug
   .reload
   ```
4. Run the analyze command to inspect the crash:
   ```cmd
   !analyze -v
   ```
   This will output a detailed analysis of the exception, registry state, and call stack.

---

## 4. Building with Crashpad Support

Crashpad integration is enabled by default when compiling FlameRobin on Windows with `vcpkg`.

If you need to verify or configure the build:
1. Ensure `crashpad` is present in the `vcpkg.json` dependencies.
2. Run CMake configure:
   ```bash
   cmake -B build -S .
   ```
3. During CMake configuration:
   * The compiler definition `FR_USE_CRASHPAD` is automatically added.
   * A post-build command copies `crashpad_handler.exe` to your build output directory (e.g., `build\Debug\`).
4. Run build:
   ```bash
   cmake --build build --config Debug
   ```
