# Using Firebird Embedded in FlameRobin

FlameRobin supports connecting directly to a local Firebird database using the Firebird Embedded engine (via `fbclient.dll` on Windows or `libfbclient.so` on Linux/macOS). This bypasses the need for a running Firebird server process.

---

## 1. GUI Configuration (FlameRobin)

To use the embedded engine in FlameRobin, you must configure the registered server block with an empty hostname and port. This tells the internal connection-string builder to pass the local file path directly to the client library without any network protocol prefix.

### Step 1: Register or Edit the Server
1. In the metadata tree view, right-click the root node and choose **Register new server** (or right-click an existing server and select **Server registration info**).
2. Configure the settings:
   - **Name (Display Name)**: Give the server a descriptive name (e.g., `Firebird Embedded` or `Local Embedded`).
   - **Host (Server)**: Leave this field **completely empty**.
   - **Port**: Leave this field **completely empty**.
3. Click **Save**.

### Step 2: Register the Database
1. Right-click the newly registered server and choose **Register existing database**.
2. Configure the database connection settings:
   - **Display name**: Choose any descriptive name.
   - **Database path**: Enter the **absolute local file path** to your database file (e.g. `C:\Data\mydb.fdb` on Windows, or `/var/db/mydb.fdb` on Linux/macOS). 
     > [!IMPORTANT]
     > Do **not** use prefixes like `localhost:` or `127.0.0.1:`. If a prefix is present, the client library will attempt a network TCP/IP connection instead.
   - **Username / Password**: Typically, use `SYSDBA` and `masterkey` (though in true embedded mode, the engine usually grants access based on OS file permissions and ignores password verification).
3. Click **Save** and double-click the database to connect.

---

## 2. Command-Line Usage

### Firebird Client Tools (e.g. `isql`)
To connect from the command line using standard Firebird command-line utilities (such as `isql`), pass the absolute database path directly as the parameter with no network prefix:
```bash
isql C:\Data\mydb.fdb -user SYSDBA -password masterkey
```

### FlameRobin Command-Line Limitations
If you pass a database file path directly to the FlameRobin executable from the command line:
```bash
flamerobin.exe C:\Data\mydb.fdb
```
FlameRobin automatically registers these command-line databases under a special virtual server called **"Unregistered local databases"**. By default, this virtual server has its hostname hardcoded to `"localhost"` in the codebase ([root.cpp](file:///C:/Work/flamerobin/src/metadata/root.cpp#L244)). 

As a result, FlameRobin will construct a connection string with the `localhost:` prefix (e.g., `localhost:C:\Data\mydb.fdb`), causing `fbclient.dll` to attempt a network TCP/IP connection. To connect to an embedded database, you must configure it through the GUI as described in Section 1.

---

## 3. Requirements & Troubleshooting

- **Direct DLL Placement**: Ensure that `fbclient.dll` (which acts as/or loads the embedded engine) is present in the same directory as the `flamerobin.exe` binary.
- **Exclusive Access vs Shared Lock**: By default, the Firebird Embedded engine opens database files directly on disk and demands an exclusive file lock. If a Firebird SuperServer service or another process has already opened the database, the embedded connection will fail.
  If you are using Firebird 3.0 or later and need multiple applications (or multiple instances of FlameRobin) to access the same local `.fdb` database file simultaneously without a centralized server, you can tell the embedded engine to use shared lock files.
  To do this, create or edit the `firebird.conf` file in the same directory as `fbclient.dll` (FlameRobin portable releases include this pre-configured by default), and uncomment/add the `ServerMode` parameter set to either `SuperClassic` or `Classic`:
  ```text
  ServerMode = SuperClassic
  ```
- **Plugins Directory**: For Firebird 3.0+ and 5.0+, make sure that authentication plugins (e.g. `srp` or `legacy_auth` in `plugins/` directory) and Unicode/ICU libraries are distributed alongside the client DLL.
