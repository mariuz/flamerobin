## Performance Benchmark & Diagnosis: workstation

| | |
|---|---|
| Computer | workstation |
| Date | 2026-10-14 09:00:00 |
| Test | this computer, without a server connection, Standard, 131 s |

### Key figures

| Part | Key figure | Result | Work |
|---|---|---:|---|
| CPU | Calculation, one user | 1,538 it/ms | 100,000 iterations in 65.00 ms |
| DISK | Saving a change (commit) | 1.50 ms | 200 commits in 300 ms |
| DISK | Writing rows | 58,824 rows/s | 50,000 rows in 850 ms |
| RAM | Reading from memory | 1,111 MB/s | 50.0 MB in 45.00 ms |
| USERS | Transactions, up to 8 users | 6,000 tx/s | 30,000 transactions in 5000 ms |

### Noticed

States and ratios within this run, not rated.

- **LOCAL, CPU** Power plan: Balanced
  - Current setting: Balanced.
  - What can help: Windows: select the power plan "High performance" (powercfg /setactive SCHEME_MIN). Set the BIOS/UEFI power profile to maximum performance, and on a virtual machine also the power policy of its host.
- **LOCAL** A virus scanner is active on this computer
  - Active virus scanner: Windows Defender. Creating, writing and deleting a small file in the temporary directory takes 0.20 ms.
  - What can help: Exclude the database files (*.fdb, nbackup delta files), the Firebird temporary directory and the Firebird server process from on-access virus scanning. On Windows add the exclusions in Microsoft Defender (or the installed security product) for the folders, the file types and firebird.exe, and exclude them from "Controlled folder access" / ransomware protection.
- **DISK** The database directory is not excluded from the virus scanner
  - Active virus scanner: Windows Defender.
  - What can help: Exclude the database files (*.fdb, nbackup delta files), the Firebird temporary directory and the Firebird server process from on-access virus scanning. On Windows add the exclusions in Microsoft Defender (or the installed security product) for the folders, the file types and firebird.exe, and exclude them from "Controlled folder access" / ransomware protection. The database directory is not excluded from Microsoft Defender yet.

### System

| | |
|---|---|
| System | workstation |
| Firebird | Firebird 5.0.4 on Windows |
| Connection | embedded engine in FlameRobin |
| Power plan | Balanced |
| Virus scanner | Windows Defender (on) |
| Page size | 8 KB |
| Page cache | 16384 pages = 128 MB |
| Forced writes | on |

### All measured values

| Part | Test | Result | Amount | Time | Average | Rate |
|---|---|---:|---:|---:|---:|---:|
| Server processor | Calculation, one connection | 65.00 ms | 100,000 iterations | 65.00 ms | 0.7 µs | 1,538,462 it/s |
| Server processor | Calculation, 4 connections at once | 80.00 ms | 400,000 iterations | 80.00 ms | 0.2 µs | 5,000,000 it/s |
| Server processor | Short calculation after a pause / without a pause | 30.00 ms / 30.00 ms | 16 bursts |  |  |  |
| Server disk | Saving a small change (commit) | 1.50 ms | 200 transactions | 300 ms | 1.50 ms | 667 tx/s |
| Server disk | Writing rows (table with 5 indexes) | 58,824 rows/s | 50,000 rows | 850 ms | 17 µs | 58,824 rows/s |
| Server disk | Changing many rows | 76,923 rows/s | 50,000 rows | 650 ms | 13 µs | 76,923 rows/s |
| Server disk | Deleting many rows | 111,111 rows/s | 50,000 rows | 450 ms | 9.0 µs | 111,111 rows/s |
| Server disk | Cleaning up deleted rows (garbage collection) | 600 ms |  | 600 ms |  |  |
| Server disk | Disk test with a small cache | 14,545 operations/s |  |  |  |  |
| Server disk | Disk test: writing | 13,333 rows/s | 50,000 rows | 3750 ms | 75 µs | 13,333 rows/s |
| Server disk | Disk test: reading everything | 20.00 ms |  | 20.00 ms |  |  |
| Server disk | Disk test: reading single rows | 50,000 rows/s | 50,000 lookups | 1000 ms | 20 µs | 50,000 rows/s |
| Server disk | Disk test: changing | 10,000 rows/s | 50,000 rows | 5000 ms | 100 µs | 10,000 rows/s |
| Server disk | Disk test: deleting | 12,500 rows/s | 50,000 rows | 4000 ms | 80 µs | 12,500 rows/s |
| Server disk | Disk test, 4 connections at once | 12,500 rows/s | 25,000 rows | 2000 ms | 80 µs | 12,500 rows/s |
| Server disk | Sorting large data (temporary files) | 166.7 MB/s | 200,000 rows, 200.0 MB | 1200 ms | 6.0 µs | 166,667 rows/s |
| Server memory | Reading all rows, first time | 1000.0 MB/s | 50.0 MB | 50.00 ms |  | 1000.0 MB/s |
| Server memory | Reading all rows again (from memory) | 1111.1 MB/s | 50.0 MB | 45.00 ms |  | 1111.1 MB/s |
| Server memory | First time compared with again | 1.1x |  |  |  |  |
| Server memory | Sorting in memory | 384.6 MB/s | 50.0 MB | 130 ms |  | 384.6 MB/s |
| Network | Connecting with login | 40.00 ms | 3 connections | 120 ms | 40.00 ms | 25 connections/s |
| Network | Response time of a request (round trip) | 0.0018 ms | 2,500 queries | 5.00 ms | 2.0 µs | 500,000 queries/s |
| Network | Response time after short pauses | 0.0020 ms | 30 queries |  |  |  |
| Network | Transferring a large result | 70.8 MB/s | 50,000 rows | 700 ms | 14 µs | 71,429 rows/s |
| Network | Time split: server / transfer / FlameRobin | 60.00 ms / 590 ms / 50.00 ms |  |  |  |  |
| Network | Small writes, one request each (100 columns) | 2,500 writes/s | 2,000 rows | 800 ms | 400 µs | 2,500 writes/s |
| Several users at once | Transactions with 1 users at once | 1,000 tx/s | 5,000 transactions | 5000 ms | 1.00 ms | 1,000 tx/s |
| Several users at once | Transactions with 2 users at once | 1,900 tx/s | 9,500 transactions | 5000 ms | 1.10 ms | 1,900 tx/s |
| Several users at once | Transactions with 4 users at once | 3,600 tx/s | 18,000 transactions | 5000 ms | 1.20 ms | 3,600 tx/s |
| Several users at once | Transactions with 8 users at once | 6,000 tx/s | 30,000 transactions | 5000 ms | 1.40 ms | 6,000 tx/s |
| This computer | Creating and deleting small files | 0.20 ms | 200 files | 40.00 ms | 200 µs | 5,000 files/s |
| This computer | Short calculation after a pause / without a pause | 10.00 ms / 10.00 ms | 16 bursts |  |  |  |
| Programming patterns | Rows one by one compared with in one go | 400 ms / 150 ms | 2,000 rows | 400 ms | 200 µs | 5,000 rows/s |
| Programming patterns | Preparing a typical query | 1.00 ms | 200 prepares | 200 ms | 1.00 ms | 1,000 prepares/s |
| Programming patterns | Reading while an old transaction is open | 0.50 ms -> 0.60 ms | 50 updates |  |  |  |
| Programming patterns | Writing / reading large data (BLOBs) | not measured |  |  |  |  |

Not measured:

- Writing / reading large data (BLOBs): Wrong parameters block kind 6

<!-- FlameRobin benchmark snapshot (used by FlameRobin to compare reports)
format = 2.0
program = 26.10.0
run.title = workstation
run.host = workstation
run.date = 2026-10-14 09:00:00
run.kind = embedded
run.intensity = standard
m.computer.files = 0.2 | ms | v1 | 200 | files | 40 | Creating and deleting small files | 0.20 ms
m.computer.burst = 10 | ms | v1 | 8 | calculations | 80 | Short calculation | 10.00 ms
m.cpu.single = 1538.46 | it/ms | v1 | 100000 | iterations | 65 | Calculating, one user | 1,538 it/ms
m.cpu.parallel = 5000 | it/ms | v1 | 400000 | iterations | 80 | Calculating, several users at once | 5,000 it/ms
m.cpu.burst =  |  | v1 |  |  |  | Short calculation after a pause / without a pause | 30.00 ms / 30.00 ms
m.storage.commit = 1.5 | ms | v1 | 200 | commits | 300 | Saving a change (commit) | 1.50 ms
m.storage.insert = 58823.5 | rows/s | v1 | 50000 | rows | 850 | Writing many rows | 58,824 rows/s
m.storage.update = 76923.1 | rows/s | v1 | 50000 | rows | 650 | Changing many rows | 76,923 rows/s
m.storage.delete = 111111 | rows/s | v1 | 50000 | rows | 450 | Deleting many rows | 111,111 rows/s
m.storage.gc = 600 | ms | v1 |  |  | 600 | Cleaning up deleted rows (garbage collection) | 600 ms
m.storage.drive = 14545.5 | operations/s | v1 | 200000 | operations | 13750 | Disk test with a small cache | 14,545 operations/s
m.storage.driveInsert = 13333.3 | rows/s | v1 | 50000 | rows | 3750 | Disk test: writing | 13,333 rows/s
m.storage.driveScan = 20 | ms | v1 |  |  | 20 | Disk test: reading everything | 20.00 ms
m.storage.driveLookup = 50000 | rows/s | v1 | 50000 | lookups | 1000 | Disk test: reading single rows | 50,000 rows/s
m.storage.driveUpdate = 10000 | rows/s | v1 | 50000 | rows | 5000 | Disk test: changing | 10,000 rows/s
m.storage.driveDelete = 12500 | rows/s | v1 | 50000 | rows | 4000 | Disk test: deleting | 12,500 rows/s
m.storage.driveParallel = 12500 | rows/s | v1 | 25000 | rows | 2000 | Disk test, several users at once | 12,500 rows/s
m.storage.tempSort = 166.667 | MB/s | v1 | 209715200 | bytes | 1200 | Sorting large data on disk | 166.7 MB/s
m.memory.scanCold = 1000 | MB/s | v1 | 52428800 | bytes | 50 | Reading all rows, first time | 1000.0 MB/s
m.memory.scan = 1111.11 | MB/s | v1 | 52428800 | bytes | 45 | Reading data from memory | 1111.1 MB/s
m.memory.coldVsWarm =  |  | v1 |  |  |  | First time compared with again | 1.1x
m.memory.sort = 384.615 | MB/s | v1 | 52428800 | bytes | 130 | Sorting in memory | 384.6 MB/s
m.network.roundTripSparse =  |  | v1 |  |  |  | Response time after short pauses | 0.0020 ms
m.network.timeSplit =  |  | v1 |  |  |  | Time split: server / transfer / FlameRobin | 60.00 ms / 590 ms / 50.00 ms
m.application.rowByRow = 5000 | rows/s | v1 | 2000 | rows | 400 | Rows one by one compared with in one go | 400 ms / 150 ms
m.application.prepare = 1 | ms | v1 | 200 | prepares | 200 | Preparing a typical query | 1.00 ms
m.application.versions =  |  | v1 |  |  |  | Reading while an old transaction is open | 0.50 ms -> 0.60 ms
m.users.tx.1 = 1000 | tx/s | v1 | 5000 | transactions | 5000 | Transactions with 1 users at once | 1,000 tx/s
m.users.tx.2 = 1900 | tx/s | v1 | 9500 | transactions | 5000 | Transactions with 2 users at once | 1,900 tx/s
m.users.tx.4 = 3600 | tx/s | v1 | 18000 | transactions | 5000 | Transactions with 4 users at once | 3,600 tx/s
m.users.tx.8 = 6000 | tx/s | v1 | 30000 | transactions | 5000 | Transactions with 8 users at once | 6,000 tx/s
m.application.blob =  |  | v1 |  |  |  | Writing / reading large data (BLOBs) | not measured
m.users.oltp = 6000 | transactions/s | v1 | 30000 | transactions | 5000 | Transactions, up to 8 users | 6,000 transactions/s
fact.system = workstation
fact.firebird = Firebird 5.0.4 on Windows
fact.access = embedded engine in FlameRobin
fact.power = Balanced
fact.scanner = Windows Defender (on)
fact.pagesize = 8 KB
fact.pagecache = 16384 pages = 128 MB
fact.forcedwrites = on
note.power.plan = computer | The power plan saves energy | Current setting: Balanced. | Windows: select the power plan "High performance" (powercfg /setactive SCHEME_MIN). Set the BIOS/UEFI power profile to maximum performance, and on a virtual machine also the power policy of its host.
note.power.plan = cpu | The power plan saves energy | Current setting: Balanced. | Windows: select the power plan "High performance" (powercfg /setactive SCHEME_MIN). Set the BIOS/UEFI power profile to maximum performance, and on a virtual machine also the power policy of its host.
note.scanner.files = computer | A virus scanner checks the files of this computer | Active virus scanner: Windows Defender. Creating, writing and deleting a small file in the temporary directory takes 0.20 ms. | Exclude the database files (*.fdb, nbackup delta files), the Firebird temporary directory and the Firebird server process from on-access virus scanning. On Windows add the exclusions in Microsoft Defender (or the installed security product) for the folders, the file types and firebird.exe, and exclude them from "Controlled folder access" / ransomware protection.
note.scanner.database = storage | A virus scanner may check the database files | Active virus scanner: Windows Defender. | Exclude the database files (*.fdb, nbackup delta files), the Firebird temporary directory and the Firebird server process from on-access virus scanning. On Windows add the exclusions in Microsoft Defender (or the installed security product) for the folders, the file types and firebird.exe, and exclude them from "Controlled folder access" / ransomware protection. The database directory is not excluded from Microsoft Defender yet.
-->
