# Firebird 6.0 GREATEST and LEAST Functions

Firebird 6.0 (and Firebird 5.0) introduced support for the `GREATEST` and `LEAST` functions, which are part of the SQL:2023 standard. FlameRobin provides full support for these functions in the SQL editor, including syntax highlighting and autocompletion.

## Overview

The `GREATEST` and `LEAST` functions take two or more arguments and return the maximum or minimum value among them, respectively.

### Syntax

```sql
GREATEST (value1, value2, [, ...])
LEAST (value1, value2, [, ...])
```

- **Arguments**: Must be of comparable types. Firebird will perform implicit type conversion if necessary, following the same rules as `COALESCE` or `CASE` expressions.
- **Null Handling**: These functions are **null-sensitive**. If any argument is `NULL`, the entire function returns `NULL`. This follows the standard SQL behavior for these functions (unlike some other database systems where `NULL`s might be ignored).

## FlameRobin Support

### Syntax Highlighting

The `GREATEST` and `LEAST` keywords are recognized by the FlameRobin SQL tokenizer and are highlighted as reserved keywords when connected to Firebird 5.0 or 6.0 databases.

### Autocompletion

When typing in the SQL editor, `GREATEST` and `LEAST` will appear in the autocompletion list (triggered by `Ctrl+Space` or automatically after a few characters) if the connected database supports them (ODS 13.1 for FB 5.0 or ODS 14.0 for FB 6.0).

## Examples

### Using GREATEST to find the latest date
```sql
SELECT 
    ID, 
    GREATEST(CREATED_AT, UPDATED_AT, LAST_LOGIN) as LAST_ACTIVITY
FROM USERS;
```

### Using LEAST for price comparison
```sql
SELECT 
    PRODUCT_NAME, 
    LEAST(BASE_PRICE, DISCOUNTED_PRICE, COMPETITOR_PRICE) as BEST_PRICE
FROM PRODUCTS;
```

## Compatibility

- **Firebird 5.0+ (ODS 13.1+)**: Full support.
- **Firebird 6.0+ (ODS 14.0+)**: Full support.
- **Older Versions**: These functions are not available in older Firebird versions. Using them will result in a server-side SQL error. FlameRobin will not highlight them as keywords when connected to older servers.

## Implementation Details

- **Tokenizer**: Added `kwGREATEST` and `kwLEAST` tokens to `SqlTokenizer.h`.
- **Keyword Sets**: Added `GREATEST` and `LEAST` to `fb50_keywords`, `fb50_reserved`, `fb60_keywords`, and `fb60_reserved` in `src/sql/firebird_keyword_sets.hpp`.
- **Tests**: Verified correct tokenization and version-aware availability in `src/sql/SqlTokenizerTest.cpp`.
