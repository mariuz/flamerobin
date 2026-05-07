# Firebird 6.0 Feature: Enhanced SQL Security Management

Firebird 6.0 (building on features introduced in Firebird 4.0) provides granular control over the security context in which SQL objects (Tables, Views, Procedures, Functions, Packages, and Triggers) are executed.

## SQL SECURITY {DEFINER | INVOKER}

-   **DEFINER**: The object executes with the privileges of the user who defined (created or last altered) it.
-   **INVOKER**: The object executes with the privileges of the user who is currently calling it.

## FlameRobin Support

FlameRobin now provides a dedicated UI for managing SQL Security across all supported metadata objects.

### Changing SQL Security

To change the SQL Security property of an object:

1.  Right-click the object in the metadata tree.
2.  Select **Set SQL Security...** from the context menu.
3.  Choose one of the following options:
    -   **DEFINER**: Explicitly set to DEFINER.
    -   **INVOKER**: Explicitly set to INVOKER.
    -   **DEFAULT (Use Database Default)**: Remove the explicit setting (using `DROP SQL SECURITY`), which makes the object inherit the database-level default.

### Supported Objects

The **Set SQL Security...** option is available for:

-   **Databases**: `ALTER DATABASE SET SQL SECURITY {DEFINER | INVOKER}`
-   **Tables**: `ALTER TABLE name SET SQL SECURITY ...` or `DROP SQL SECURITY`
-   **Views**: `ALTER VIEW name SET SQL SECURITY ...` or `DROP SQL SECURITY`
-   **Procedures**: `ALTER PROCEDURE name SET SQL SECURITY ...` or `DROP SQL SECURITY`
-   **Functions**: `ALTER FUNCTION name SET SQL SECURITY ...` or `DROP SQL SECURITY`
-   **Packages**: `ALTER PACKAGE name SET SQL SECURITY ...` or `DROP SQL SECURITY`
-   **Triggers**: `ALTER TRIGGER name SET SQL SECURITY ...` or `DROP SQL SECURITY`

### Availability

This feature is available when FlameRobin is connected to a Firebird server supporting ODS 13.0 (Firebird 4.0) or higher. For Firebird 6.0 (ODS 14.0), the support is more comprehensive and robust.

## Benefits

-   **Enhanced Security**: Fine-grained control over permissions, allowing complex logic to run with elevated privileges (DEFINER) without granting those privileges directly to the end user.
-   **SQL Standard Compliance**: Aligns Firebird more closely with the SQL standard regarding security contexts.
