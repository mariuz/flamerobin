# Firebird 5.0 and 6.0 JSON Support

Firebird 5.0 and 6.0 introduced and enhanced support for SQL-standard JSON functions and aggregates. FlameRobin provides full support for these features in the SQL editor, including syntax highlighting and autocompletion.

## Supported JSON Functions

The following JSON-related keywords are now recognized by FlameRobin when connected to Firebird 5.0 (ODS 13.1) or Firebird 6.0 (ODS 14.0):

- `JSON` (Data type/Keyword)
- `JSON_VALUE`: Extracts a scalar value from a JSON string.
- `JSON_QUERY`: Extracts an object or array from a JSON string.
- `JSON_OBJECT`: Constructs a JSON object.
- `JSON_ARRAY`: Constructs a JSON array.
- `JSON_EXISTS`: Tests whether a JSON path exists in a JSON string.
- `JSON_TABLE`: Query JSON data as a relational table.
- `JSON_ARRAYAGG`: Aggregate function to create a JSON array.
- `JSON_OBJECTAGG`: Aggregate function to create a JSON object.

## Examples

### Using JSON_VALUE and JSON_QUERY
```sql
SELECT 
    JSON_VALUE(INFO, '$.name') as NAME,
    JSON_QUERY(INFO, '$.address') as ADDRESS
FROM CUSTOMERS;
```

### Using JSON_TABLE
```sql
SELECT *
FROM JSON_TABLE('{"items": [{"id": 1}, {"id": 2}]}', '$.items[*]'
    COLUMNS (
        ID INTEGER PATH '$.id'
    )
) AS jt;
```

### Using JSON_OBJECT and JSON_ARRAY
```sql
SELECT JSON_OBJECT(KEY 'id' VALUE ID, KEY 'name' VALUE NAME)
FROM USERS;

SELECT JSON_ARRAY(1, 2, 3, 'a');
```

## FlameRobin Support

### Syntax Highlighting

JSON keywords are recognized as reserved keywords in the FlameRobin SQL editor when connected to supported databases. They will be highlighted according to your selected SQL editor theme.

### Autocompletion

These functions will appear in the autocompletion list (`Ctrl+Space`) when working with a Firebird 5.0+ database, facilitating the discovery and use of JSON capabilities.

## Compatibility

- **Firebird 5.0+ (ODS 13.1+)**: Full support for standard JSON functions.
- **Firebird 6.0+ (ODS 14.0+)**: Continues support and enhancements.
- **Older Versions**: These functions are not supported by the Firebird engine in older versions. FlameRobin will not highlight them as keywords when connected to older servers.

## Implementation Details

- **Tokenizer**: Integrated `kwJSON`, `kwJSON_VALUE`, etc., into the SQL tokenizer.
- **Keyword Sets**: Added JSON keywords to `fb50_keywords`, `fb50_reserved`, `fb60_keywords`, and `fb60_reserved` in `src/sql/firebird_keyword_sets.hpp`.
- **Tests**: Verified correct tokenization and version-aware availability in `src/sql/SqlTokenizerTest.cpp`.
