# Partial (Conditional) Indexes in Firebird 5.0

Firebird 5.0 introduced support for partial (conditional) indexes. A partial index is an index that only includes a subset of rows from a table, based on a boolean condition.

## Syntax

The syntax for creating a partial index is:

```sql
CREATE [UNIQUE] [ASCENDING | DESCENDING] INDEX index_name
ON table_name
{ (column_name [, column_name ...]) | COMPUTED BY (expression) }
WHERE <search_condition>;
```

## FlameRobin Support

FlameRobin supports partial indexes in the following ways:

### Metadata Extraction (DDL)

When extracting the DDL for a table or a standalone index, FlameRobin correctly includes the `WHERE` clause if the index is a partial index.

Example:
```sql
CREATE INDEX IDX_T_A ON T (A, B) WHERE A IS NOT NULL;
CREATE INDEX IDX_T_EXPR ON T COMPUTED BY UPPER(A) WHERE A IS NOT NULL;
```

### GUI Representation

In the table properties page, under the "Indexes" tab, partial indexes are displayed with their condition. The "Fields/Expression" column will show both the indexed fields and the `WHERE` clause.

### Metadata Loading

FlameRobin's metadata loader has been extended to read the `RDB$CONDITION_SOURCE` column from the `RDB$INDICES` system table for databases with ODS 13.1 (Firebird 5.0) or higher.

## Implementation Details

- The `Index` metadata class stores the condition source in the `conditionM` member.
- The `Index::getFieldsAsString()` method has been updated to include the `WHERE` clause.
- The `buildIndexBodySql` helper function in `src/metadata/IndexDDL.cpp` centralizes the rendering of index definitions, ensuring consistency between standalone index DDL and table-level DDL.
- Extensive unit tests in `src/metadata/IndexDDLTest.cpp` verify the correctness of the generated DDL for various combinations of regular/expression indexes and conditions.
