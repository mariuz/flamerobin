# How to Use FlameRobin Model Context Protocol (MCP) Server

FlameRobin now includes a built-in Model Context Protocol (MCP) server written in C++. This allows AI assistants (like Claude, Gemini, Antigravity, and others) to safely connect to and interact with your registered Firebird databases to query schemas, inspect metadata, and execute queries.

---

## 1. Quick Start

### Build FlameRobin
Ensure that you have compiled FlameRobin in Debug or Release mode:
```powershell
# Build using Visual Studio MSBuild (Windows)
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" build/flamerobin.vcxproj /p:Configuration=Debug /p:Platform=x64
```

### Launch the MCP Server
Run FlameRobin from the terminal with the `--mcp` flag:
```bash
build/Debug/flamerobin.exe --mcp
```

The server runs on standard input/output (stdio) and outputs debugging logs to standard error (`stderr`).

---

## 2. Integration with AI Clients

### Claude Desktop
To integrate FlameRobin MCP with Claude Desktop, add it to your `claude_desktop_config.json` file:

- **Windows**:
  ```json
  {
    "mcpServers": {
      "flamerobin": {
        "command": "C:\\Work\\flamerobin\\build\\Debug\\flamerobin.exe",
        "args": ["--mcp"]
      }
    }
  }
  ```
- **macOS / Linux**:
  ```json
  {
    "mcpServers": {
      "flamerobin": {
        "command": "/path/to/flamerobin/build/flamerobin",
        "args": ["--mcp"]
      }
    }
  }
  ```

---

## 3. Supported MCP Tools

The server exposes the following JSON-RPC tools:

### `list_databases`
Lists all registered databases in FlameRobin's `fr_databases.conf` configuration file.
- **Arguments**: None
- **Response**: List of databases, including names, hostnames, usernames, roles, and dialects.

### `get_schema`
Fetches all user tables, views, and columns/datatypes from the specified database.
- **Arguments**:
  - `database_name` (string, required): The registered name of the database.
  - `password` (string, optional): Connection password if not saved.
- **Response**: Database schema JSON object.

### `execute_query`
Executes an arbitrary SQL statement against the specified database and returns the results.
- **Arguments**:
  - `database_name` (string, required): The registered name of the database.
  - `sql` (string, required): The SQL query (e.g. `SELECT`, `INSERT`, `UPDATE`).
  - `password` (string, optional): Connection password if not saved.
- **Response**: List of JSON rows, column count, and affected rows count.

---

## 4. Troubleshooting

- **Check stderr logs**:
  The MCP server writes status logs (e.g. `[McpServer] Starting FlameRobin MCP server...`) to standard error (`stderr`). If your client is failing to connect, inspect your client's logs to see the output of stderr.
- **Authentication**:
  If a database requires a password and it is not saved in FlameRobin's credentials vault, you must supply the `password` argument when calling `get_schema` or `execute_query`.
