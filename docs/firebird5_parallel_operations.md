# Firebird 5.0 Parallel Operations support in FlameRobin

Firebird 5.0 introduced support for parallel workers in several maintenance operations, including backup, restore, and sweep. This feature allows these operations to utilize multiple CPU cores, significantly improving performance on multi-core systems.

## Parallel Backup and Restore

In the Backup and Restore dialogs, you can now specify the number of parallel workers to be used. This option is available under "General Options" as "Parallel workers (FB5.0+)".

*   **Parallel Workers**: Specifies the number of worker threads to be used for the operation. A value of 0 (default) uses the default server behavior.
*   **Requirement**: This feature requires Firebird 5.0 or later. For older versions, this setting is ignored.

## Database Maintenance (Sweep and Validation)

A new "Maintenance..." option has been added to the Database tools menu. This dialog allows you to perform manual database maintenance tasks, including:

*   **Sweep (Force garbage collection)**: Forces the server to perform garbage collection on the database.
*   **Validate pages**: Performs a basic validation of database pages.
*   **Full validation**: Performs a comprehensive validation, including record structures.
*   **Mend**: Prepares the database for backup by fixing minor corruptions.
*   **Read-only validation**: Performs validation without attempting to fix any errors.
*   **Ignore checksums**: Ignores page checksum errors during validation.
*   **Kill unavailable shadows**: Removes references to shadow files that are no longer accessible.

### Parallel Sweep and Validation

The Maintenance dialog also includes the "Parallel workers (FB5.0+)" option. When performing a Sweep or Validation on Firebird 5.0+, you can specify the number of threads to be used to speed up the process.

## IBPP Support

The underlying IBPP library has been updated to support the new Service Parameter Block (SPB) items for parallel workers:

*   `isc_spb_bkp_parallel_workers` (for Backup and Restore)
*   `isc_spb_rpr_par_workers` (for Sweep and Repair)

FlameRobin automatically checks the server version and only sends these parameters if the server is Firebird 5.0 or later.
