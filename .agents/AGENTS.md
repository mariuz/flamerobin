# FlameRobin Agent Rules

These rules guide agents when interacting with the FlameRobin codebase or running tests/development tasks.

## Model Context Protocol (MCP) Server Usage
FlameRobin has a built-in Model Context Protocol (MCP) server that can be run to allow agents to interact with registered Firebird databases directly.

1. **How to start the MCP server**:
   Run the compiled `flamerobin.exe` with the `--mcp` flag:
   ```bash
   build/Debug/flamerobin.exe --mcp
   ```
2. **Standard Input/Output**:
   The MCP server communicates using JSON-RPC 2.0 messages over standard I/O (stdin/stdout). Log messages are printed to stderr.
3. **Available Tools**:
   - `list_databases`: List registered databases.
   - `get_schema`: Retrieve the schema of a database.
   - `execute_query`: Run standard read-only or DML SQL queries.

For detailed integration guides (e.g. for Claude desktop or other systems), refer to [docs/mcp_howto.md](file:///C:/Work/flamerobin/docs/mcp_howto.md).
