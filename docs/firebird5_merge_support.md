# Firebird 5.0 MERGE Statement Support in FlameRobin

FlameRobin now supports the extended `MERGE` syntax introduced in Firebird 5.0, including the `WHEN NOT MATCHED BY SOURCE` clause.

## Improvements

### 1. SQL Statement Recognition
The SQL recognizer has been updated to specifically identify `MERGE` statements. Previously, they were often categorized as `actNONE` or incorrectly handled if they deviated from standard DML.
- A new `actMERGE` action has been added to the `SqlAction` enumeration.
- `MERGE` statements are correctly identified as DML (Data Manipulation Language), ensuring they are handled appropriately in the statement history and other DML-specific features.
- The recognizer handles both `MERGE INTO <target>` and the shorthand `MERGE <target>` (where `INTO` is omitted).

### 2. Code Completion and Alias Support
The code completion engine (IntelliSense) has been enhanced to support `MERGE` statements:
- The `MERGE` and `USING` keywords are now recognized as introducing table/view references.
- Table aliases defined in both the `INTO` clause (target table) and the `USING` clause (source table/query) are correctly extracted.
- This allows FlameRobin to provide column suggestions when you type `<alias>.` in any part of the `MERGE` statement, including the new `WHEN NOT MATCHED BY SOURCE` clause.

## Firebird 5.0 Extended MERGE Syntax

Firebird 5.0 expanded the `MERGE` statement to include support for cases where a row exists in the target but not in the source.

### Syntax Overview
```sql
MERGE INTO <target> [ [AS] <alias> ]
USING <source> [ [AS] <alias> ]
ON <condition>
[ WHEN MATCHED [ AND <condition> ] THEN <action> ]
[ WHEN NOT MATCHED [ BY TARGET ] [ AND <condition> ] THEN <action> ]
[ WHEN NOT MATCHED BY SOURCE [ AND <condition> ] THEN <action> ]
```

### New Clause: `WHEN NOT MATCHED BY SOURCE`
This clause allows you to perform an action (usually `DELETE` or `UPDATE`) on the target table when a record exists in the target but has no corresponding record in the source.

**Example:**
```sql
MERGE INTO target t
USING source s ON t.id = s.id
WHEN MATCHED THEN
    UPDATE SET t.val = s.val
WHEN NOT MATCHED BY TARGET THEN
    INSERT (id, val) VALUES (s.id, s.val)
WHEN NOT MATCHED BY SOURCE THEN
    DELETE;
```

In the above example, any records in `target` that are not present in `source` will be deleted, effectively synchronizing the target table with the source.

## Technical Details
- **Affected files:**
    - `src/sql/SqlTokenizer.h`: Added `kwTARGET` to `SqlTokenType`.
    - `src/sql/keywordtokens.hpp`: Added `TARGET` keyword mapping.
    - `src/sql/SqlStatement.h`: Added `actMERGE` to `SqlAction` enum.
    - `src/sql/SqlStatement.cpp`: 
        - Updated constructor to recognize `kwMERGE` as a valid action.
        - Improved "lucky guess" logic to correctly identify target tables for `MERGE` and `UPDATE` even when the table name is a keyword.
        - Updated `isDDL()` to correctly identify `MERGE` as DML.
    - `src/sql/IncompleteStatement.cpp`: Added `kwMERGE` and `kwUSING` to the alias search logic, ensuring all tables in a `MERGE` statement are correctly identified for auto-completion.

