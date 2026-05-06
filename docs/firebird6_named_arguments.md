# Firebird 6.0 Named Argument Support

Firebird 6.0 introduced support for named arguments in procedure and function calls, allowing for clearer and more flexible SQL statements. FlameRobin provides support for this new syntax in the SQL editor.

## Overview

Named arguments allow you to pass values to parameters by name rather than by position. This is particularly useful for procedures with many parameters, especially when some have default values.

### Syntax

The `=>` operator is used to assign a value to a named parameter:

```sql
EXECUTE PROCEDURE MY_PROC(
    ARG1 => 'Value 1',
    ARG3 => 'Value 3'
);
```

In the example above, `ARG2` (if it has a default value) can be omitted, and the arguments are passed by name.

## FlameRobin Support

### Tokenization and Syntax Highlighting

The `=>` operator is now recognized by the FlameRobin SQL tokenizer as the `NAMED_ARG_ASSIGN` token. It is correctly handled during statement parsing and splitting.

The keyword `NAMED_ARG_ASSIGN` (which can also be used in some contexts or metadata) is also recognized and highlighted.

### Autocompletion

- **Keywords**: `NAMED_ARG_ASSIGN` appears in the SQL autocompletion list when connected to a Firebird 6.0+ (ODS 14.0+) database.
- **Operator**: The `=>` syntax is fully supported for manual typing and will not break the SQL editor's word detection or other features.

## Examples

### Procedure Call with Named Arguments
```sql
-- Calling a procedure using named arguments
EXECUTE PROCEDURE UPDATE_USER_STATUS(
    USER_ID => 101,
    NEW_STATUS => 'ACTIVE',
    NOTIFY_USER => TRUE
);
```

### Function Call in SELECT
```sql
SELECT * 
FROM GET_SALARY_STATS(
    DEPT_ID => 10,
    INCLUDE_BONUS => FALSE
);
```

## Compatibility

- **Firebird 6.0+ (ODS 14.0+)**: Full syntax support.
- **Older Versions**: Named arguments are not supported by the Firebird engine in versions prior to 6.0. Using the `=>` operator will result in a SQL syntax error from the server.

## Implementation Details

- **Tokenizer**: Updated `SqlTokenizer.cpp` to recognize the `=>` operator and map it to `kwNAMED_ARG_ASSIGN`.
- **Enum**: Added `kwNAMED_ARG_ASSIGN` to the `SqlTokenType` enum in `SqlTokenizer.h`.
- **Mappings**: Added `=>` and `NAMED_ARG_ASSIGN` to `keywordtokens.hpp`.
- **Keyword Sets**: `NAMED_ARG_ASSIGN` is included in the Firebird 6.0 keyword and reserved sets in `firebird_keyword_sets.hpp`.
- **Tests**: Added unit tests in `SqlTokenizerTest.cpp` to verify correct detection of the `=>` operator and the `NAMED_ARG_ASSIGN` keyword.
