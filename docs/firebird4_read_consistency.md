# Firebird 4.0 Read Committed Read Consistency Support

## Overview
Firebird 4.0 introduced a new transaction isolation level: **Read Committed Read Consistency**. 
This mode provides a consistent view of the data for each statement in a transaction, similar to `SNAPSHOT` isolation, but for `READ COMMITTED` transactions. It avoids many update conflicts and provides better concurrency.

FlameRobin now supports this isolation level in the following areas:
- **SQL Editor**: You can now select "Read consistency isolation mode" from the "Transaction settings" menu.
- **Transaction Info**: When a transaction is started in the SQL editor, the status bar displays the current isolation level, including "Read Consistency".
- **Monitoring**: The Database Properties page now displays a list of active transactions for the current connection, showing their isolation level, read-only status, and wait mode.

## Technical Details

### Transaction Isolation Level Mapping
The internal `fr::TransactionIsolationLevel` enum has been updated to include `ReadConsistency`:
- `Consistency` (0)
- `Concurrency` (1)
- `ReadDirty` (2) - `Read Committed Record Version`
- `ReadCommitted` (3) - `Read Committed No Record Version`
- `ReadConsistency` (4) - `Read Committed Read Consistency` (Firebird 4.0+)

### Implementation
The support is implemented in both `IBPP` and the new `fb-cpp` based engine:
- `FbCppTransaction` correctly maps the isolation level to the Firebird 4.0 TPB tag `isc_tpb_read_consistency`.
- `IbppTransaction` also supports the new tag.
- `DatabaseInfo` now loads active transaction information from `MON$TRANSACTIONS`.

## Usage
1. Open a SQL editor for a Firebird 4.0 database.
2. Go to `Statement -> Transaction settings`.
3. Select `Read consistency isolation mode`.
4. Execute your statements. The status bar will show `Transaction started (Read Committed Read Consistency)`.
5. To see all active transactions, right-click the database in the tree and select `Properties`. Scroll down to the `Active Transactions` section.
