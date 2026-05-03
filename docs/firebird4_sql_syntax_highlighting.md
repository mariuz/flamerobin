# Firebird 4.0 SQL Syntax Highlighting Support

## Overview
Firebird 4.0 introduces several new keywords (e.g., `DECFLOAT`, `INT128`, `PUBLICATION`, `BINARY`, etc.). FlameRobin now dynamically activates the correct keyword set for the SQL editor based on the connected database server version (ODS version).

## Dynamic Keyword Activation
The SQL editor's syntax highlighting and autocomplete features now adapt to the target database:

- **Firebird 4.0 (ODS 13.0)**: Activates keywords like `DECFLOAT`, `INT128`, `PUBLICATION`, `ZONE`, `LATERAL`.
- **Firebird 3.0 (ODS 12.0)**: Uses the FB 3.0 keyword set (e.g., `BOOLEAN`, `SCROLL`, `IDENTITY`).
- **Firebird 2.5 (ODS 11.1)**: Falls back to the legacy keyword set.
- **Firebird 5.0 (ODS 13.1)**: Activates keywords like `SKIP`, `LOCKED` (preliminary support).

## Implementation Details
The implementation is centered around the following components:

### `SqlTokenizer`
The `SqlTokenizer` class maintains keyword sets for different Firebird versions in `src/sql/firebird_keyword_sets.hpp`. It provides static methods to retrieve keywords based on ODS version:
- `SqlTokenizer::getKeywords(kwc, odsMajor, odsMinor)`
- `SqlTokenizer::getKeywordsString(kwc, odsMajor, odsMinor)`

### `SqlEditor`
The `SqlEditor` class (based on `wxStyledTextCtrl` / Scintilla) has been updated with a `setKeywords(odsMajor, odsMinor)` method. This method updates the Scintilla lexer's keyword list (`SetKeyWords(0, ...)`).

### `ExecuteSqlFrame`
When an `ExecuteSqlFrame` is created or connected to a database, it calls `setKeywords()`. This method now retrieves the database's ODS version and propagates it to the `SqlEditor` instance.

## Usage
No user action is required. FlameRobin automatically detects the server version upon connection and configures the SQL editor accordingly. Keywords specific to the connected version will be highlighted in the editor and will appear in the autocomplete suggestions.

## Testing
Unit tests in `src/sql/SqlTokenizerTest.cpp` verify that:
1. The correct keyword set is returned for different ODS versions.
2. New keywords like `DECFLOAT` and `PUBLICATION` are present in FB 4.0+ sets but absent in FB 2.5 sets.
3. FB 5.0 keywords like `SKIP LOCKED` are correctly handled when connected to FB 5.0+.
