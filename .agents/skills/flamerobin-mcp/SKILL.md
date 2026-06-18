---
name: flamerobin-mcp
description: Model Context Protocol (MCP) server for FlameRobin to interact with registered Firebird databases.
---

# flamerobin-mcp Skill

This skill allows agents to invoke and interact with FlameRobin's built-in MCP server.

## Usage Instructions

When you need to list databases, explore database schemas, or query data from a Firebird database managed by FlameRobin:

1. **Verify if FlameRobin is built**:
   Ensure `build/Debug/flamerobin.exe` exists. If not, build the project:
   ```powershell
   & "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" build/flamerobin.vcxproj /p:Configuration=Debug /p:Platform=x64
   ```

2. **Launch the MCP server in a background task**:
   Use the `run_command` tool to launch the MCP server as a background process:
   ```powershell
   build/Debug/flamerobin.exe --mcp
   ```

3. **Protocol Interactions**:
   Send JSON-RPC 2.0 messages over standard input and receive responses over standard output.
   For example, to list all registered databases, send:
   ```json
   {"jsonrpc": "2.0", "method": "tools/call", "id": 1, "params": {"name": "list_databases", "arguments": {}}}
   ```
