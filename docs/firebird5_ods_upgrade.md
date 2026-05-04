# Phase 6: Firebird 5.0 Feature Support (ODS 13.1) - Inline ODS Upgrade

Firebird 5.0 (ODS 13.1) introduced a new feature that allows upgrading the On-Disk Structure (ODS) of a database to the latest version supported by the engine without requiring a full backup and restore cycle. This is historically done using the `gfix -upgrade` command, and is now integrated into FlameRobin's Database Maintenance interface.

## Inline ODS Upgrade in FlameRobin

FlameRobin provides a simple checkbox in the Database Maintenance dialog to trigger an inline ODS upgrade.

### How to use the Inline ODS Upgrade

1.  **Connect to Database**: Ensure you are connected to the database you wish to upgrade.
2.  **Open Maintenance Dialog**: Right-click on the database in the metadata tree and select **Maintenance...** (or go to **Database -> Maintenance...**).
3.  **Select Upgrade ODS**: Check the **Upgrade ODS (FB 5.0+)** option.
4.  **Start Maintenance**: Click the **Start** button. FlameRobin will connect to the Firebird Services Manager and execute the upgrade command.

### Important Considerations

-   **Firebird 5.0+ Only**: This feature is only supported when connected to a Firebird 5.0 or later server. On older servers, the "Upgrade ODS" checkbox will be disabled.
-   **Exclusive Access**: Like most maintenance operations, an ODS upgrade usually requires exclusive access to the database. Ensure no other users are connected before starting the process.
-   **One-Way Process**: Upgrading the ODS is a one-way process. Once a database is upgraded to a newer ODS, it cannot be opened by older versions of the Firebird engine. **Always make a backup before performing an ODS upgrade.**
-   **No Backup/Restore Needed**: The primary advantage of this feature is speed. It avoids the time-consuming process of backing up a large database and restoring it just to upgrade the ODS version.

## Technical Details

When the "Upgrade ODS" option is selected, FlameRobin sends the `isc_spb_rpr_upgrade_db` flag to the Firebird Services Manager via the `isc_action_svc_repair` action.

The internal implementation in `IbppService::maintain` maps the `MaintenanceFlags::UpgradeODS` to the corresponding IBPP and Firebird SPB constants:

```cpp
// src/engine/db/ibpp/IbppService.cpp
if ((int)config.flags & (int)MaintenanceFlags::UpgradeODS) 
    flags |= IBPP::rpUpgrade; // Maps to isc_spb_rpr_upgrade_db (0x1000)
```

## Requirements
-   **Firebird 5.0 or later**: The database server must be Firebird 5.0+.
-   **Modern Client Library**: The client library (fbclient) used by FlameRobin must also be from Firebird 5.0 or later to recognize the new SPB constant.
