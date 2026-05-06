# Firebird 6.0 EXPLAIN Statement Support

FlameRobin now supports the Firebird 6.0 `EXPLAIN` statement, which provides a modern, standardized way to retrieve and analyze query execution plans.

## Overview

The `EXPLAIN` statement in Firebird 6.0 replaces or augments the legacy method of retrieving plans via API info items. It is implemented as a standard SQL statement that returns a result set containing the execution plan.

Key advantages of the `EXPLAIN` statement:
- **Unlimited Size**: Unlike legacy plans (limited to 64KB), the `EXPLAIN` statement can handle plans of any complexity.
- **Structured Formats**: Supports `TREE` (hierarchical) and `PLAIN` (legacy) formats.
- **Execution Statistics**: With the `ANALYZE` option, it can provide actual execution statistics alongside estimates.

## Usage in FlameRobin

A new "Explain Statement" tool has been added to the SQL Execution Frame (SQL Editor).

1. Open a SQL Editor and enter your query.
2. Click the **Explain statement** button in the toolbar (or select **Statement -> Explain statement (FB 6.0+)** from the menu).
3. FlameRobin will execute the query prefixed with `EXPLAIN (FORMAT TREE)`.
4. The resulting plan will be displayed in the data grid.

### Compatibility

- **Firebird 6.0+ (ODS 14.0+)**: Full support for the `EXPLAIN` statement.
- **Older Versions**: When using older Firebird versions, FlameRobin will display a message indicating that the statement is not supported. Use the traditional "Show Execution Plan" feature instead.

## Features

### FORMAT TREE
By default, FlameRobin uses `FORMAT TREE` to provide a hierarchical view of the plan. This makes it easier to understand complex join orders and index usage.

### ANALYZE (Manual Usage)
You can also manually type `EXPLAIN (ANALYZE) SELECT ...` in the SQL editor and click **Execute**. FlameRobin will display the actual execution statistics in the result grid.

## Implementation Details

The feature is implemented in `ExecuteSqlFrame` by intercepting the current SQL and prepending the `EXPLAIN` clause. This allows leveraging the existing robust query execution and result display mechanisms in FlameRobin.
