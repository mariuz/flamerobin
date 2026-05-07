# Firebird 6.0 Feature: Enhanced Security (Owner and Initial User)

Firebird 6.0 (ODS 14.0) introduces enhanced security features for database creation, allowing the assignment of an explicit owner and an initial user at the time the database is created.

## Overview

Previously, the user who created the database was automatically its owner. In Firebird 6.0, you can explicitly specify:

1.  **OWNER**: The user or role that will be the owner of the database.
2.  **INITIAL USER**: The first user (besides the owner and SYSDBA) who will have access to the database.

These options are part of the `CREATE DATABASE` statement and are also supported via the Database Parameter Block (DPB).

## FlameRobin Support

FlameRobin provides full support for these features in the **Create New Database** dialog.

### Create New Database Dialog

When creating a new database for a server that supports ODS 14.0 (Firebird 6.0+), the following new fields are available:

-   **Owner**: Specify the username or role to be assigned as the database owner.
-   **Initial User**: Specify an additional user to be granted immediate access.

### Technical Implementation

FlameRobin implements these features using the Firebird 4.0+ Object-Oriented API via the `fb-cpp` backend.

#### Database Parameter Block (DPB) Tags

The following DPB tags are used:

-   `isc_dpb_owner` (102): Used to specify the database owner.
-   `isc_dpb_initial_user` (103): Used to specify the initial user.

#### Code Snippet (DPB Construction)

```cpp
if (!owner.empty())
    dpbBuilder->insertString(&statusWrapper, isc_dpb_owner, owner.c_str());
if (!initialUser.empty())
    dpbBuilder->insertString(&statusWrapper, isc_dpb_initial_user, initialUser.c_str());
```

## SQL Syntax Support

FlameRobin's SQL tokenizer and statement parser have been updated to recognize the `OWNER` and `INITIAL USER` keywords in the `CREATE DATABASE` statement context.

Example SQL:
```sql
CREATE DATABASE 'localhost:test.fdb'
  USER 'SYSDBA' PASSWORD 'masterkey'
  PAGE_SIZE 8192
  DEFAULT CHARACTER SET UTF8
  OWNER 'DB_ADMIN'
  INITIAL USER 'APP_USER';
```

## Benefits

-   **Separation of Duties**: Allows a system administrator to create a database on behalf of another user or role.
-   **Provisioning Automation**: Simplifies the process of setting up new databases with pre-defined ownership and access controls.
