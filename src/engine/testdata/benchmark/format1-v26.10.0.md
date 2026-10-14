## Performance Benchmark & Diagnosis: Server dbserver

| | |
|---|---|
| Computer | workstation |
| Date | 2026-10-04 12:00:00 |
| Test | from a client over the network, Standard, 123 s |

### Key values

| Part | Value | Result | Work |
|---|---|---:|---|
| Server processor | Calculating, one user | 1,538 it/ms | 100,000 iterations in 65.00 ms |
| Server processor | Calculating, several users at once | 5,000 it/ms | 400,000 iterations in 80.00 ms |
| Server disk | Saving a change (commit) | 1.50 ms | 200 commits in 300 ms |
| Server disk | Writing many rows | 58,824 rows/s | 50,000 rows in 850 ms |
| Server disk | Disk test with a small cache | 14,545 operations/s | 200,000 operations in 13.8 s |
| Server disk | Disk test, several users at once | 12,500 rows/s | 25,000 rows in 2000 ms |
| Server disk | Sorting large data on disk | 166.7 MB/s | 200.0 MB in 1200 ms |
| Server memory | Reading data from memory | 1111.1 MB/s | 50.0 MB in 45.00 ms |
| Server memory | Sorting in memory | 384.6 MB/s | 50.0 MB in 130 ms |
| Network | Response time of a request | 0.20 ms | 2,500 requests in 625 ms |
| Network | Transferring a large result | 70.8 MB/s | 49.6 MB in 700 ms |
| Network | Connecting | 40.00 ms | 3 connections in 120 ms |
| Network | Small writes, one request each | 2,500 writes/s | 2,000 rows in 800 ms |
| Several users at once | Transactions, up to 8 users | 6,000 transactions/s | 30,000 transactions of 8 users in 5000 ms |
| This computer | Short calculation | 10.00 ms | 8 calculations in 80.00 ms |

### System

| | |
|---|---|
| System | Server dbserver |
| Firebird | Firebird 5.0.4 on Windows |
| Connection | TCP/IP, port 3050 |
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
| Server disk | Creating the test database | 300 ms |  |  |  |  |
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
| Network | Server ports reachable from here | 3050 |  |  |  |  |
| Network | Network part of a connection (TCP) | 0.40 ms |  |  |  |  |
| Network | Connecting with login | 40.00 ms | 3 connections | 120 ms | 40.00 ms | 25 connections/s |
| Network | Response time of a request (round trip) | 0.20 ms | 2,500 queries | 625 ms | 250 µs | 4,000 queries/s |
| Network | Response time after short pauses | 0.25 ms | 30 queries |  |  |  |
| Network | Response time with a larger answer (8 KB) | 0.50 ms |  |  |  |  |
| Network | Transferring a large result | 70.8 MB/s | 50,000 rows | 700 ms | 14 µs | 71,429 rows/s |
| Network | Time split: server / transfer / FlameRobin | 60.00 ms / 590 ms / 50.00 ms |  |  |  |  |
| Network | Small writes, one request each (100 columns) | 2,500 writes/s | 2,000 rows | 800 ms | 400 µs | 2,500 writes/s |
| Several users at once | Transactions with 1 users at once | 1,000 tx/s | 5,000 transactions | 5000 ms | 1.00 ms | 1,000 tx/s |
| Several users at once | Transactions with 2 users at once | 1,900 tx/s | 9,500 transactions | 5000 ms | 1.10 ms | 1,900 tx/s |
| Several users at once | Transactions with 4 users at once | 3,600 tx/s | 18,000 transactions | 5000 ms | 1.20 ms | 3,600 tx/s |
| Several users at once | Transactions with 8 users at once | 6,000 tx/s | 30,000 transactions | 5000 ms | 1.40 ms | 6,000 tx/s |
| This computer | Creating and deleting small files | 0.20 ms | 200 files | 40.00 ms | 200 µs | 5,000 files/s |
| This computer | Short calculation after a pause / without a pause | 25.00 ms / 10.00 ms | 16 bursts |  |  |  |
| Programming patterns | Rows one by one compared with in one go | 400 ms / 150 ms | 2,000 rows | 400 ms | 200 µs | 5,000 rows/s |
| Programming patterns | Preparing a typical query | 1.00 ms | 200 prepares | 200 ms | 1.00 ms | 1,000 prepares/s |
| Programming patterns | Writing / reading large data (BLOBs) | 41.7 MB/s / 50.0 MB/s | 200 BLOBs, 12.5 MB | 300 ms / 250 ms |  |  |
| Programming patterns | Reading while an old transaction is open | 0.50 ms -> 0.60 ms | 50 updates |  |  |  |

<!-- FlameRobin benchmark results (used by FlameRobin to compare reports)
format = 1
title = Server dbserver
host = workstation
date = 2026-10-04 12:00:00
kind = client
intensity = standard
value.cpuIterationsPerMs = 1538.5
value.parallelIterationsPerMs = 5000.0
value.commitMs = 1.5000
value.insertRowsPerSecond = 58823.5
value.driveOperationsPerSecond = 14545.5
value.parallelDriveRowsPerSecond = 12500.0
value.tempSortMBPerSecond = 166.7
value.scanMBPerSecond = 1111.1
value.sortMBPerSecond = 384.6
value.roundTripMs = 0.2000
value.transferMBPerSecond = 70.8
value.connectMs = 40.0000
value.wideInsertsPerSecond = 2500.0
value.oltpTransactionsPerSecond = 6000.0
value.clientCpuBurstMs = 10.0000
work.clientCpuBurstMs = 8 calculations | 80.000
work.commitMs = 200 commits | 300.000
work.connectMs = 3 connections | 120.000
work.cpuIterationsPerMs = 100,000 iterations | 65.000
work.driveOperationsPerSecond = 200,000 operations | 13750.000
work.insertRowsPerSecond = 50,000 rows | 850.000
work.oltpTransactionsPerSecond = 30,000 transactions of 8 users | 5000.000
work.parallelDriveRowsPerSecond = 25,000 rows | 2000.000
work.parallelIterationsPerMs = 400,000 iterations | 80.000
work.roundTripMs = 2,500 requests | 625.000
work.scanMBPerSecond = 50.0 MB | 45.000
work.sortMBPerSecond = 50.0 MB | 130.000
work.tempSortMBPerSecond = 200.0 MB | 1200.000
work.transferMBPerSecond = 49.6 MB | 700.000
work.wideInsertsPerSecond = 2,000 rows | 800.000
stat = network |  | Server ports reachable from here | 3050 |  | 0.000
stat = network |  | Network part of a connection (TCP) | 0.40 ms |  | 0.000
stat = network | connectMs | Connecting with login | 40.00 ms | 3 connections | 120.000
stat = computer |  | Creating and deleting small files | 0.20 ms | 200 files | 40.000
stat = computer | clientCpuBurstMs | Short calculation after a pause / without a pause | 25.00 ms / 10.00 ms | 16 bursts | 0.000
stat = cpu | cpuIterationsPerMs | Calculation, one connection | 65.00 ms | 100,000 iterations | 65.000
stat = cpu | parallelIterationsPerMs | Calculation, 4 connections at once | 80.00 ms | 400,000 iterations | 80.000
stat = cpu |  | Short calculation after a pause / without a pause | 30.00 ms / 30.00 ms | 16 bursts | 0.000
stat = storage |  | Creating the test database | 300 ms |  | 300.000
stat = storage | commitMs | Saving a small change (commit) | 1.50 ms | 200 transactions | 300.000
stat = storage | insertRowsPerSecond | Writing rows (table with 5 indexes) | 58,824 rows/s | 50,000 rows | 850.000
stat = storage |  | Changing many rows | 76,923 rows/s | 50,000 rows | 650.000
stat = storage |  | Deleting many rows | 111,111 rows/s | 50,000 rows | 450.000
stat = storage |  | Cleaning up deleted rows (garbage collection) | 600 ms |  | 600.000
stat = storage | driveOperationsPerSecond | Disk test with a small cache | 14,545 operations/s |  | 0.000
stat = storage |  | Disk test: writing | 13,333 rows/s | 50,000 rows | 3750.000
stat = storage |  | Disk test: reading everything | 20.00 ms |  | 20.000
stat = storage |  | Disk test: reading single rows | 50,000 rows/s | 50,000 lookups | 1000.000
stat = storage |  | Disk test: changing | 10,000 rows/s | 50,000 rows | 5000.000
stat = storage |  | Disk test: deleting | 12,500 rows/s | 50,000 rows | 4000.000
stat = storage | parallelDriveRowsPerSecond | Disk test, 4 connections at once | 12,500 rows/s | 25,000 rows | 2000.000
stat = storage | tempSortMBPerSecond | Sorting large data (temporary files) | 166.7 MB/s | 200,000 rows, 200.0 MB | 1200.000
stat = memory |  | Reading all rows, first time | 1000.0 MB/s | 50.0 MB | 50.000
stat = memory | scanMBPerSecond | Reading all rows again (from memory) | 1111.1 MB/s | 50.0 MB | 45.000
stat = memory |  | First time compared with again | 1.1x |  | 0.000
stat = memory | sortMBPerSecond | Sorting in memory | 384.6 MB/s | 50.0 MB | 130.000
stat = network | roundTripMs | Response time of a request (round trip) | 0.20 ms | 2,500 queries | 625.000
stat = network |  | Response time after short pauses | 0.25 ms | 30 queries | 0.000
stat = network |  | Response time with a larger answer (8 KB) | 0.50 ms |  | 0.000
stat = network | transferMBPerSecond | Transferring a large result | 70.8 MB/s | 50,000 rows | 700.000
stat = network |  | Time split: server / transfer / FlameRobin | 60.00 ms / 590 ms / 50.00 ms |  | 0.000
stat = network | wideInsertsPerSecond | Small writes, one request each (100 columns) | 2,500 writes/s | 2,000 rows | 800.000
stat = application |  | Rows one by one compared with in one go | 400 ms / 150 ms | 2,000 rows | 400.000
stat = application |  | Preparing a typical query | 1.00 ms | 200 prepares | 200.000
stat = application |  | Writing / reading large data (BLOBs) | 41.7 MB/s / 50.0 MB/s | 200 BLOBs, 12.5 MB | 0.000
stat = application |  | Reading while an old transaction is open | 0.50 ms -> 0.60 ms | 50 updates | 0.000
stat = users |  | Transactions with 1 users at once | 1,000 tx/s | 5,000 transactions | 5000.000
stat = users |  | Transactions with 2 users at once | 1,900 tx/s | 9,500 transactions | 5000.000
stat = users |  | Transactions with 4 users at once | 3,600 tx/s | 18,000 transactions | 5000.000
stat = users |  | Transactions with 8 users at once | 6,000 tx/s | 30,000 transactions | 5000.000
fact.system = Server dbserver
fact.firebird = Firebird 5.0.4 on Windows
fact.access = TCP/IP, port 3050
fact.power = Balanced
fact.scanner = Windows Defender (on)
fact.pagesize = 8 KB
fact.pagecache = 16384 pages = 128 MB
fact.forcedwrites = on
hint = computer | Power saving slows down this computer | A short CPU burst after an idle pause takes 2.5x as long as the same burst back to back; the CPU clocks down when idle. | Current setting: Balanced. Windows: select the power plan "High performance" (powercfg /setactive SCHEME_MIN). Set the BIOS/UEFI power profile to maximum performance, and on a virtual machine also the power policy of its host.
hint = computer | A virus scanner checks the files of this computer | Active virus scanner: Windows Defender. Creating, writing and deleting a small file in the temporary directory takes 0.20 ms. | Exclude the application directories and temporary files of the database applications from on-access scanning on the client.
-->
