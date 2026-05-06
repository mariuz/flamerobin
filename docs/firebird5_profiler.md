# SQL / PSQL Profiler Integration in Firebird 5.0

Firebird 5.0 (ODS 13.1) introduced a built-in profiler package `RDB$PROFILER` that allows measuring the performance of SQL and PSQL code (stored procedures, triggers, etc.) with nanosecond precision.

## FlameRobin Integration

FlameRobin provides a seamless way to use the Firebird 5.0 profiler directly from the SQL Editor.

### How to use the Profiler

1.  **Open SQL Editor**: Open a new or existing SQL editor window.
2.  **Enable Profiler**: 
    -   Go to the **Statement** menu and check **Display SQL/PSQL Profiler**.
    -   OR click the **Profiler** button (history icon) on the toolbar.
3.  **Execute SQL**: Write your SQL or PSQL code and execute it as usual (press F4 or click Execute).
4.  **View Results**: After execution, a new **Profiler** tab will appear in the bottom notebook (alongside Statistics and Data).

### Profiler Results

The Profiler tab displays two grids providing deep insights into your query performance:

#### 1. PSQL Statistics
This grid shows line-by-line execution statistics for PSQL modules (procedures, triggers, packages, anonymous blocks).
-   **REQUEST_NAME**: The name of the PSQL module being executed.
-   **LINE_NUM**: The line number in the source code.
-   **COLUMN_NUM**: The column number (if available).
-   **COUNTER**: The number of times this specific line was executed.
-   **TOTAL_TIME (ms)**: The cumulative time spent executing this line, in milliseconds.
-   **MAX_TIME (ms)**: The longest time a single execution of this line took.

**Tip**: Sort by `TOTAL_TIME` to quickly identify bottlenecks in your stored procedures.

#### 2. Record Source Statistics
This grid shows performance data for each record source (table scan, index seek, join, sort) used in the query execution.
-   **SOURCE_NAME**: The name of the record source or the type of operation (e.g., "Table Scan", "Index Seek").
-   **COUNTER**: The number of fetches or operations performed on this source.
-   **TOTAL_TIME (ms)**: The total time spent on this record source.

**Tip**: High counters in "Table Scan" operations often suggest that adding an index could improve performance.

## Technical Details

When the profiler is enabled, FlameRobin performs the following steps automatically for each execution:

1.  **Start Session**: Calls `SELECT RDB$PROFILER.START_SESSION('FlameRobin', NULL, 'Default_Profiler') FROM RDB$DATABASE` before executing your SQL. This returns a unique Session ID.
2.  **Execute SQL**: Runs your statement(s) as part of the profiled session.
3.  **Finish Session**: Calls `EXECUTE PROCEDURE RDB$PROFILER.FINISH_SESSION(TRUE)` to flush collected data from memory to the profiling global temporary tables.
4.  **Fetch Data**: Queries `PLG$PROF_PSQL_STATS` and `PLG$PROF_RECORD_SOURCE_STATS` for the generated session ID and populates the UI grids.

## Performance Considerations

-   **Overhead**: Profiling adds a small amount of overhead to query execution. It is recommended to use it for performance tuning and disable it for normal operations.
-   **Precision**: Firebird 5.0 provides nanosecond precision for profiling data. FlameRobin displays these values in milliseconds for better readability.
-   **Granularity**: Unlike standard execution plans, the profiler shows *where* the time was actually spent during execution, accounting for actual data distributions and system state.

## Troubleshooting

-   **Profiler Tab missing**: Ensure you are connected to a Firebird 5.0 (or later) database with ODS 13.1 or higher. The profiler is not available on older versions.
-   **No data in grids**: Verify that the `Default_Profiler` plugin is enabled in your `firebird.conf`. If it's disabled, the `RDB$PROFILER` package might exist but won't collect any data.
-   **Permissions**: The user must have sufficient privileges to execute procedures in the `RDB$PROFILER` package and to read from the `PLG$PROF_*` tables.

## Requirements
-   **Firebird 5.0 or later**: The database engine must support the `RDB$PROFILER` package.
-   **Modern ODS (13.1+)**: The database must be using ODS 13.1 or higher.
-   **Profiler Plugin**: The `Default_Profiler` plugin must be enabled in `firebird.conf` (enabled by default).
