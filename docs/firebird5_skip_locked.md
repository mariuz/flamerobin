# Phase 6: Firebird 5.0 Feature Support (ODS 13.1) - SKIP LOCKED Clause Highlighting

## Overview

Firebird 5.0 (ODS 13.1, released January 2024) introduces the `SKIP LOCKED` clause for statements such as `SELECT ... WITH LOCK`, `UPDATE`, and `DELETE`. This feature allows statements to skip rows that are already locked by other transactions, thereby preventing wait conditions or lock conflict errors and improving concurrency.

FlameRobin now includes syntax highlighting for the `SKIP LOCKED` clause when connected to a Firebird 5.0 or newer database. This aligns with the modern SQL constructs available in Firebird 5.0, making it easier for users to interact with high-concurrency environments.

## Implementation Details

- **Tokens Registration:** The words `SKIP` and `LOCKED` have been formally registered as distinct tokens (`kwSKIP` and `kwLOCKED`) within the `SqlTokenizer` component. They have been placed within the `SqlTokenType` enumeration alongside other standard SQL reserved words.
- **Syntax Highlighting:** The Scintilla-based SQL editor (`SqlEditor`) correctly identifies and highlights `SKIP` and `LOCKED` as keywords under ODS 13.1 (Firebird 5.0) keyword sets. It respects case-insensitivity, processing variations like `skip locked`, `SKIP locked`, and `Skip Locked`.
- **Mapping Update:** The `keywordtokens.hpp` mapping array incorporates mappings from string values `"SKIP"` and `"LOCKED"` to the newly specified token identifiers. This ensures that the FlameRobin internal parsing infrastructure robustly assigns correct token structures when users execute or view queries containing them.

## Usage

No manual intervention or configuration change is required by the user. FlameRobin automatically detects the Firebird server version during connection. When the user writes statements such as:

```sql
SELECT * FROM my_table WITH LOCK SKIP LOCKED;
```

```sql
UPDATE employees SET salary = salary * 1.1 SKIP LOCKED;
```

```sql
DELETE FROM processing_queue SKIP LOCKED;
```

Both `SKIP` and `LOCKED` will receive the configured keyword styling based on the active color scheme.

## Testing

The `SKIP LOCKED` syntax highlighting support is heavily verified through unit tests inside `src/sql/SqlTokenizerTest.cpp`. Specifically, the test suite guarantees:

1. **Tokenization Identity**: The `SqlTokenizer` correctly yields `kwSKIP` and `kwLOCKED` sequential tokens when reading the string `"SKIP LOCKED"`.
2. **Realistic Queries Parsing**: Complete queries testing `SELECT ... WITH LOCK SKIP LOCKED`, `UPDATE ... SKIP LOCKED`, and `DELETE ... SKIP LOCKED` are verified to correctly locate and identify the `SKIP` and `LOCKED` keywords precisely where they reside in the statement.
3. **Version Check**: The function `SqlTokenizer::getKeywordsString` is verified to inject `SKIP` and `LOCKED` explicitly when the target ODS version is 13.1 (Firebird 5.0) or higher. Older versions of Firebird will accurately not enforce strict highlighting if it conflicts, keeping consistent with historical syntax support.