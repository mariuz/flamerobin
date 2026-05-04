# Multiple-Row DML RETURNING Support in Firebird 5.0

Firebird 5.0 (ODS 13.1) introduced a powerful enhancement to DML statements: the ability for `INSERT`, `UPDATE`, `DELETE`, and `MERGE` with a `RETURNING` clause to return multiple rows. Previously, `RETURNING` was limited to singleton results (exactly one row).

## Overview

In Firebird 5.0, if a DML statement with `RETURNING` affects multiple rows, it can be treated similarly to a `SELECT` statement. This is particularly useful for:

-   Retrieving generated IDs for a batch insert.
-   Auditing updated or deleted values in a single operation.
-   Processing results of a complex `MERGE` operation.

### Examples

#### Multiple-Row INSERT
```sql
INSERT INTO t1 (val) 
SELECT val FROM t2 
RETURNING id, val;
```

#### Multiple-Row UPDATE
```sql
UPDATE t1 
SET val = val * 1.1 
WHERE category = 'A' 
RETURNING id, old.val, new.val;
```

#### Multiple-Row DELETE
```sql
DELETE FROM t1 
WHERE expiration_date < CURRENT_DATE 
RETURNING id, val;
```

#### Multiple-Row MERGE
```sql
MERGE INTO t1 t
USING t2 s ON t.id = s.id
WHEN MATCHED THEN UPDATE SET val = s.val
WHEN NOT MATCHED THEN INSERT (val) VALUES (s.val)
RETURNING t.id, t.val;
```

## FlameRobin Support

FlameRobin now fully supports displaying multi-row results from these statements in the SQL editor's result grid.

### Key Features
-   **Automatic Detection**: FlameRobin automatically detects if a DML statement has a `RETURNING` clause.
-   **Result Grid Integration**: Multi-row results are displayed in the same results grid used for `SELECT` statements.
-   **Affected Rows Logging**: Even when results are shown in the grid, the number of rows affected by the DML operation is still logged in the message area.

### Implementation Details
The support is implemented in the `fb-cpp` backend by ensuring that any statement with output columns (regardless of its type) is treated as having a potential result set. The UI logic in `ExecuteSqlFrame` was updated to stay in the results grid view (`vmGrid`) when output columns are present, while still providing DML-specific feedback.

## Requirements
-   **Firebird 5.0 or later**: The database engine must support multi-row RETURNING.
-   **Modern ODS (13.1+)**: The database must be using ODS 13.1 or higher.
-   **fb-cpp Backend**: FlameRobin must be configured to use the `fb-cpp` database engine (default in current versions).
