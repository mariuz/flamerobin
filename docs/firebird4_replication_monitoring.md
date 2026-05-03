# Firebird 4.0 Built-in Replication Monitoring Support

## Overview
Firebird 4.0 (ODS 13) introduces built-in replication features. Replication is managed through publications, which define the set of tables whose changes should be replicated. 

FlameRobin now supports viewing and monitoring these publications through a dedicated **Replication** node in the metadata tree.

## Metadata Tree
A new top-level node named **Replication** is visible for Firebird 4.0+ databases. Under this node, you can find the **Publications** collection.

### Publications
The **Publications** node lists all publications defined in the database.
- **RDB$DEFAULT**: The system-defined publication. By default, it is defined for `ALL TABLES`.

## Publication Properties
When you select a publication in the tree, FlameRobin displays its properties:
- **Name**: The name of the publication.
- **Defined for**: Indicates whether the publication is for `All user-defined tables` or for a `Specific set of tables`.
- **Tables**: If defined for specific tables, a list of included tables is shown.

## DDL Extraction
FlameRobin can extract the DDL for publications. 
- Example for `ALL TABLES`:
  ```sql
  CREATE PUBLICATION RDB$DEFAULT FOR ALL TABLES;
  ```
- Example for specific tables:
  ```sql
  CREATE PUBLICATION MY_PUB FOR TABLE (TABLE1, TABLE2);
  ```

## Implementation Details
The support is implemented by querying the following system tables:
- `RDB$PUBLICATIONS`: Stores publication metadata.
- `RDB$PUBLICATION_TABLES`: Stores the list of tables included in each publication.

FlameRobin uses a heuristic to determine if a publication is defined for `ALL TABLES`: if there are no entries in `RDB$PUBLICATION_TABLES` for a given publication, it is treated as `FOR ALL TABLES` (matching Firebird's behavior where a publication must be one or the other).

## Usage
1. Connect to a Firebird 4.0+ database.
2. Locate the **Replication** node in the tree.
3. Expand **Publications** to see available publications.
4. Click on a publication to view its properties and DDL.
