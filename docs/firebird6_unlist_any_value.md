# Firebird 6.0 ANY_VALUE and UNLIST Support

Firebird 6.0 introduces the `ANY_VALUE` aggregate function and the `UNLIST` built-in table-valued function, both of which are now fully supported by FlameRobin's SQL editor.

## ANY_VALUE Aggregate Function

The `ANY_VALUE` function returns an arbitrary value from a group of rows. This function is particularly useful in queries with a `GROUP BY` clause when you need to include a column that is not part of the grouping criteria and its specific value within the group is not important.

### Syntax

```sql
ANY_VALUE (expression)
```

- **Behavior**: Unlike `MIN()` or `MAX()`, which require comparing all values in the group, `ANY_VALUE()` can be more efficient as it conceptually returns the first value it encounters for that group.
- **SQL Standard**: This function was introduced in the SQL:2023 standard and is now implemented in Firebird 6.0.

### Example

```sql
-- Select department ID, an arbitrary department name, and total salary
SELECT 
    DEPARTMENT_ID, 
    ANY_VALUE(DEPARTMENT_NAME) as DEPT_NAME, 
    SUM(SALARY)
FROM EMPLOYEES
GROUP BY DEPARTMENT_ID;
```

## UNLIST Function

The `UNLIST` function is a built-in table-valued function that splits a string into multiple rows based on a delimiter. It effectively performs the inverse operation of the `LIST()` or `LISTAGG()` aggregate functions.

### Syntax

```sql
SELECT ... FROM UNLIST (string_expression [, delimiter])
```

- **Arguments**:
    - `string_expression`: The string to be split into rows.
    - `delimiter`: (Optional) A string or character used as a separator. If omitted, it defaults to a comma (`,`).
- **Return Value**: Returns a result set (table) where each row contains one segment of the split string.

### Examples

#### Basic Usage
```sql
-- Returns three rows: 'Red', 'Green', 'Blue'
SELECT * FROM UNLIST('Red,Green,Blue');
```

#### Custom Delimiters
```sql
-- Splitting a pipe-separated string
SELECT * FROM UNLIST('101|202|303', '|');
```

#### Integration with Other Tables
```sql
-- Joining with a table based on a comma-separated list of IDs
SELECT e.NAME 
FROM EMPLOYEES e
JOIN UNLIST('1,5,10') u ON e.ID = CAST(u.VALUE AS INTEGER);
```

## FlameRobin Support

### Syntax Highlighting

Both `ANY_VALUE` and `UNLIST` are recognized as **reserved keywords** in the FlameRobin SQL editor when connected to a Firebird 6.0 (ODS 14.0) database. They will be highlighted as keywords, distinguishing them from user-defined identifiers.

### Autocompletion and Calltips

- **Autocompletion**: These functions appear in the SQL editor's autocompletion list (Ctrl+Space) only when working with a Firebird 6.0+ database.
- **Calltips**: Future versions of FlameRobin will include parameter hints (calltips) for these functions to further assist with SQL development.

## Compatibility and Version Awareness

FlameRobin employs **version-aware syntax highlighting**. This means:
- When connected to a **Firebird 6.0+** database, these keywords are active and highlighted.
- When connected to **older versions** (Firebird 5.0, 4.0, etc.), these words are treated as regular identifiers. This prevents false highlighting of columns or variables that might happen to be named `UNLIST` or `ANY_VALUE` in legacy schemas where they are not reserved by the engine.

## Implementation Details

- **Tokenizer**: Integrated `kwANY_VALUE` and `kwUNLIST` into the `SqlTokenizer` with proper ODS version gating.
- **Keyword Sets**: Added both keywords to `fb60_keywords` and `fb60_reserved` in `src/sql/firebird_keyword_sets.hpp`.
- **Version Normalization**: Refactored ODS-to-Version mapping logic to correctly distinguish between ODS 13.x (FB 4/5) and ODS 14.x (FB 6).
- **Tests**: Added comprehensive regression tests in `src/sql/SqlTokenizerTest.cpp` covering tokenization, reserved status verification, and version-aware availability.
