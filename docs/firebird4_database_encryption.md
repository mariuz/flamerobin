# Firebird 4.0 Database Encryption Status Support

## Overview
Firebird 4.0 (ODS 13) introduces support for database-level encryption. This allows sensitive data to be protected at rest. Encryption is managed via encryption plugins and key holder plugins.

FlameRobin now provides visibility into the database encryption status directly from the Database Properties panel.

## Encryption States
The encryption status displayed in FlameRobin can be one of the following:
- **Not encrypted**: The database is currently not encrypted.
- **Encrypted**: The database is fully encrypted.
- **Encryption in progress**: The database is currently being encrypted.
- **Decryption in progress**: The database is currently being decrypted.

These states correspond to the `MON$CRYPT_STATE` column in the `MON$DATABASE` system monitoring table, or the `fb_info_crypt_state` database information item.

## Technical Implementation
- **DAL**: The `DatabaseInfoData` structure has been updated to include a `cryptState` field.
- **Backends**: Both `IBPP` and `fb-cpp` backends have been updated to retrieve the `fb_info_crypt_state` information from the Firebird engine.
- **Metadata**: The `DatabaseInfo` class now exposes `getCryptState()`.
- **UI**: The `DATABASE.html` template has been updated to include an "Encryption Status" row in the database info table, expanded via the `{%dbinfo:crypt_state%}` command.

## Usage
1. Open FlameRobin and connect to a Firebird 4.0+ database.
2. Right-click the database in the tree and select **Properties**.
3. In the **Database info** section, look for the **Encryption Status** field.
