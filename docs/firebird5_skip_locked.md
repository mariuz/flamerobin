# Phase 6: Firebird 5.0 Feature Support (ODS 13.1) - SKIP LOCKED Clause Highlighting

## Overview

Firebird 5.0 (ODS 13.1, released January 2024) introduces the `SKIP LOCKED` clause for statements such as `SELECT ... WITH LOCK`, `UPDATE`, and `DELETE`. This feature allows statements to skip rows that are already locked by other transactions, thereby preventing wait conditions or lock conflict errors and improving concurrency.

FlameRobin supports syntax highlighting for the `SKIP LOCKED` clause when connected to a Firebird 5.0 or newer database. This alignment with modern SQL constructs ensures a smooth user experience in high-concurrency environments.

## Feature Importance

In traditional database operations, attempting to read or modify a row that is already locked by another transaction usually results in the current transaction waiting for the lock to be released, or failing with a lock conflict error.

The `SKIP LOCKED` clause is particularly useful for:
- **Queue Processing:** Multiple workers can select and process different rows from a table without blocking each other.
- **Batch Jobs:** Avoiding delays caused by long-running transactions holding locks on specific rows.
- **Performance:** Improving overall system throughput by not waiting on contested resources.

## Implementation Details

The support for `SKIP LOCKED` is integrated into FlameRobin's core SQL infrastructure:

### 1. Token Registration
The words `SKIP` and `LOCKED` are registered as distinct tokens (`kwSKIP` and `kwLOCKED`) in the `SqlTokenizer` component (`src/sql/SqlTokenizer.h`).

### 2. Syntax Highlighting Mapping
The `keywordtokens.hpp` mapping array contains entries for both words, mapping them to their respective internal token identifiers. This ensures that the FlameRobin internal parsing infrastructure correctly identifies these words during SQL analysis.

### 3. Version-Aware Keywords
The `SqlTokenizer` maintains separate keyword sets for different Firebird versions in `src/sql/firebird_keyword_sets.hpp`.
- `LOCKED` is included in the `fb50_keywords` set (used for ODS 13.1+).
- `SKIP` is included in all Firebird keyword sets (from 2.5 to 6.0), as it has been a keyword since Firebird 2.0 (for the `ROWS ... SKIP ...` clause).
- `LOCKED` is correctly absent from older versions (2.5, 3.0, 4.0), ensuring that it doesn't conflict with user-defined identifiers in those versions.

### 4. Dynamic Activation
The SQL editor (`SqlEditor`) dynamically requests the keyword list from the `SqlTokenizer` based on the ODS version of the active connection. This is handled in `ExecuteSqlFrame::setKeywords()`.

## SQL Examples

### Select with Lock
```sql
SELECT * FROM orders 
WHERE status = 'PENDING' 
WITH LOCK SKIP LOCKED;
```

### Update with Skip Locked
```sql
UPDATE tasks 
SET worker_id = 123 
WHERE worker_id IS NULL 
ORDER BY priority DESC 
ROWS 1 
SKIP LOCKED;
```

### Delete with Skip Locked
```sql
DELETE FROM processing_queue 
WHERE attempt_count > 5 
SKIP LOCKED;
```

## Testing and Validation

The implementation is verified through several layers of tests:

### Unit Tests
The `src/sql/SqlTokenizerTest.cpp` test suite includes:
- **Tokenization Tests:** Verifies that `"SKIP LOCKED"` is correctly tokenized as `kwSKIP` followed by `kwLOCKED`, including checks for various case styles (UPPER, lower, MiXeD).
- **Full Statement Tests:** Verifies that complete `SELECT`, `UPDATE`, and `DELETE` statements containing `SKIP LOCKED` are parsed correctly, with tokens found at the expected positions.
- **Version-Specific Tests:** Verifies that the `LOCKED` keyword is present in the Firebird 5.0 keyword set but absent from the Firebird 4.0 and 2.5 sets.

### How to Run Tests
To run the tokenizer tests, build the `sql_tokenizer_test` target using CMake:
```bash
mkdir build && cd build
cmake ..
make sql_tokenizer_test
./sql_tokenizer_test
```
