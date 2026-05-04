# Firebird 5.0 LATERAL JOIN Support

## Overview

Firebird 5.0 (ODS 13.1) introduced support for `LATERAL` joins. A `LATERAL` join allows a subquery in the `FROM` clause to refer to columns of preceding relations in the same `FROM` clause. This is particularly useful for applying a set-returning function or subquery to each row of another table.

FlameRobin's SQL editor tokenizer and parser have been updated to correctly handle `LATERAL` joins, ensuring that features like syntax highlighting, statement splitting, and autocomplete work seamlessly with this modern SQL syntax.

## Syntax

The `LATERAL` keyword is used before a subquery in a `JOIN` or `FROM` clause:

```sql
SELECT ...
FROM table1 t1
JOIN LATERAL (
    SELECT ...
    FROM table2 t2
    WHERE t2.col = t1.col
) ON TRUE;
```

## FlameRobin Support

### 1. Syntax Highlighting
The `LATERAL` keyword is recognized and highlighted in the SQL editor when connected to Firebird 4.0+ (ODS 13.0+) databases. It is treated as a reserved word in these versions.

### 2. Statement Recognition
The `MultiStatement` parser correctly identifies the boundaries of SQL statements containing `LATERAL` joins. It correctly handles the nested parentheses used in `LATERAL` subqueries, ensuring that statements are not prematurely split or incorrectly identified as "incomplete".

### 3. Autocomplete Integration
The autocomplete engine (`IncompleteStatement`) has been verified to handle `LATERAL` joins without errors. While autocomplete inside the `LATERAL` subquery itself is currently limited, the presence of `LATERAL` does not interfere with autocomplete for the rest of the statement.

### 4. Avoiding False Warnings
Previous versions might have produced false warnings or failed to recognize the end of a statement when a complex `LATERAL` join was used. The updated tokenizer ensures that the nesting level of parentheses is tracked correctly where necessary (e.g., in `SelectStatement` analysis), avoiding false "incomplete statement" indications in the UI.

## SQL Examples

### Lateral Join with Correlation
```sql
SELECT t1.id, t2.total
FROM orders t1
CROSS JOIN LATERAL (
    SELECT SUM(item_price) as total
    FROM order_items
    WHERE order_id = t1.id
) t2;
```

### Lateral Join in PSQL
```sql
CREATE OR ALTER PROCEDURE get_order_totals
RETURNS (order_id INT, total NUMERIC(18,2))
AS
BEGIN
    for select o.id, l.total
        from orders o
        join lateral (
            select sum(i.price) as total
            from order_items i
            where i.order_id = o.id
        ) l on true
        into :order_id, :total
    do
    begin
        suspend;
    end
END
```

## Testing and Verification

The `LATERAL` join support is verified by the following tests:

### Multi-Statement Test
The `src/sql/MultiStatementTest.cpp` suite includes tests to ensure that `LATERAL` joins are correctly handled in scripts containing multiple statements, even when the subquery contains complex logic.

### Tokenizer Test
The `src/sql/SqlTokenizerTest.cpp` suite verifies that the `LATERAL` keyword is correctly tokenized and that the `jumpToken(true)` helper correctly skips over entire `LATERAL` subqueries.

### How to Run Tests
To verify the implementation, run the following test targets:
```bash
make sql_tokenizer_test && ./build/sql_tokenizer_test
make multi_statement_test && ./build/multi_statement_test
```
