# Benchmark report format

Every report of "Performance Benchmark & Diagnosis" (HTML and Markdown) contains the results of its run as a hidden comment, the snapshot. FlameRobin reads it to compare a saved report with a new run ("Compare with an earlier report") or two saved reports with each other ("Compare two reports..."). A report is kept for years and copied into tickets, so the snapshot is a contract between versions of FlameRobin: a later version must read the reports of an earlier one, or refuse them with a clear message, and never compare two values that are not comparable.

The code:

- `src/engine/BenchmarkValues.cpp`: the register of every measurement (`getBenchmarkMeasurementDefs()`).
- `src/engine/BenchmarkCompare.cpp`: writing (`formatBenchmarkSnapshot()`), reading (`parseBenchmarkSnapshot()`) and comparing (`compareBenchmarkSnapshots()`).
- `src/engine/BenchmarkTest.cpp`: the unit tests, with `src/engine/testdata/benchmark/` (`registry.golden` and the saved reports of every format).

## The snapshot

```
<!-- FlameRobin benchmark snapshot (used by FlameRobin to compare reports)
format = 2.0
program = 26.10.0
run.title = HP ZBook Power 15.6 inch G10 Mobile Workstation PC
run.host = IT002038
run.date = 2026-10-05 10:11:22
run.kind = embedded
run.intensity = standard
m.storage.commit = 0.56 | ms | v1 | 200 | commits | 112 | Saving a change (commit) | 0.56 ms
m.storage.insert = 40614.1 | rows/s | v1 | 50000 | rows | 1231.1 | Writing many rows | 40,614 rows/s
m.network.dns =  |  | v1 |  |  |  | Finding the server by its name (DNS) | 1.00 ms
fact.os = Windows 11 Pro 24H2
note.power.saving = cpu | Power saving slows down the server | ... | ...
-->
```

Every line is `key = value`. The lines:

| Key | Value |
|---|---|
| `format` | `<major>.<minor>` of the format, see below |
| `program` | the version of FlameRobin that made the run; only shown |
| `run.title`, `run.host`, `run.date`, `run.kind`, `run.intensity` | the tested system, the computer (missing when private information is hidden), the start (`yyyy-mm-dd hh:mm:ss`, so that the text sorts by time), `embedded`, `local` or `client`, `quick`, `standard` or `deep` (taken from the level of the run, never from the translated name) |
| `m.<id>` | one measurement, see below |
| `fact.<key>` | hardware, software and settings (`getBenchmarkFacts()`), compared as texts |
| `note.<id>` | a hint (`findBenchmarkHints()`): part, title, detail, advice; the id tells the kind of hint, the same in every part it is given for |

The fields of a measurement, separated by `|`:

1. the value in the unit of the register; empty for information that is only a text
2. the unit (`ms`, `rows/s`, `it/ms`, `MB/s`, ...)
3. the version of the test, `v1`, `v2`, ...
4. the amount of work, a whole number; empty if it does not apply
5. the unit of the amount (`rows`, `commits`, `bytes`, ...), untranslated
6. the time of the work in milliseconds
7. the title in the language of the run
8. the value as the report showed it, in the language of the run

Numbers are the same in every language: a point as the decimal separator, no thousands separators, no exponent, at most six significant digits (but every digit before the point). Empty means none. Title and text are only shown; two runs are matched by the id of a measurement, never by a title or a text. Private information is hidden in the texts of the snapshot only, never in its keys, ids, numbers or units.

### Ids

An id consists of letters and digits in parts separated by dots, e.g. `storage.commit`. The first part is the part of the report: `cpu`, `storage`, `memory`, `network`, `users`, `computer`, `application`, and `settings` for the configuration, which is not saved as measurements but compared as facts. A family has one more part for its parameter: `users.tx.8` (8 users at once), `storage.pageSize.16384`, `settings.conf.TempCacheLimit`.

## Reading a report

`parseBenchmarkSnapshot()` returns a status:

| Status | When | The dialog |
|---|---|---|
| `Ok` | format 2.x | compares |
| `NotAReport` | no snapshot (also in a saved comparison, see below) | "The file contains no benchmark results." |
| `TooOld` | format 1 (FlameRobin 26.10.0, marker `FlameRobin benchmark results`) | "The report was made with an older version of FlameRobin (report format 1) and cannot be compared." |
| `TooNew` | a major format above 2 | "The report was made with a newer version of FlameRobin (report format 3.0). Please update FlameRobin to compare it." |
| `Damaged` | no `format` line, an unreadable format or no end of the comment | "The results in the report are incomplete or damaged and cannot be read." |

Lines and keys that this version does not know are skipped. A measurement with an id that this version does not know is kept with its title and shown in the comparison in its part ("only in this run"). Fields after the eighth are skipped. A line that cannot be read (an invalid id, a number that is not one, a missing version) is skipped like an unknown one; an id counts once.

## Saved comparisons

`BenchmarkComparison::toHtml()` and `toMarkdown()` end with the comment `<!-- FlameRobin benchmark comparison (...) -->` (`getBenchmarkComparisonMarker()`). A comparison holds no snapshot of its own, so `parseBenchmarkSnapshot()` returns `NotAReport` for it; `isBenchmarkComparisonPage()` recognizes the mark. "Open..." shows an HTML comparison as a tab that is not a source of comparisons (`BenchmarkResultStore::addSavedComparison()`, left out of `getComparableIds()`); "Compare..." refuses the file with a message that says it is a comparison. The mark belongs to the format and must stay readable.

## Versions of the format

- The **minor** number rises when lines, keys or fields are added at the end: older versions of the same major format skip them and still compare.
- The **major** number rises when something breaks: a field changes its position or meaning, a kind of line is removed, the grammar changes. A version refuses another major format with a message, if it can recognize the snapshot. FlameRobin 26.10.0 read only a `format` line behind its marker `FlameRobin benchmark results` and would have compared a report of format 2 against nothing; so the snapshot of format 2 begins with another marker, `FlameRobin benchmark snapshot`, which 26.10.0 does not find ("contains no benchmark results"). `parseBenchmarkSnapshot()` looks for the new marker first, then for the old one, so that a report of format 1 is still refused as too old. A later major format that breaks the grammar must again change the marker.

## The register

Every result of `buildBenchmarkResults()` gets the id of its measurement as the first argument, and every id is in the register (the unit test checks both directions). An entry has:

| Field | Meaning |
|---|---|
| `id` | see above |
| `version` | of the test; raised whenever its work changes |
| `section` | the part of the report; the first part of the id |
| `kind` | `Key` (a key value, gives the points), `Measure` (compared) or `Info` (only shown) |
| `unit`, `higherIsBetter` | of the value; a time is lower is better |
| `amountFixed` | every run of an intensity does the same work; false for the tests with a time limit |
| `settings`, `constants`, `sql` | the work of the test: the fields of `BenchmarkSettings`, the values of `BenchmarkWorkload` (and the sizes of `BenchmarkSql`), the statements of `BenchmarkSql` and the objects of its DDL that it uses; see "The signature of the work" |
| `title`, `value` | of a key value: its title and its member of `BenchmarkValues` |
| `family`, `retired` | the id takes a parameter; the measurement is no longer made, the entry stays for older reports |

## Comparing two runs

Every value of two runs gets a state (`BenchmarkCompareState`):

| State | When | Ratio |
|---|---|---|
| `Compared` | both runs have it from the same version of the test | yes, for key values and compared values with a number in both runs |
| `OnlyNewer` | only the newer run has it (new) | no |
| `OnlyOlder` | only the older run has it (no longer measured) | no |
| `TestChanged` | the versions of the test differ | no |
| `WorkChanged` | the work differs where it is fixed (`amountFixed`): for compared values always, for key values only between runs of the same intensity | no |

The direction comes from the register. Tests with a time limit are compared by their rate even when they did different work. Key values of runs of different intensities are compared, roughly, as before (the report says so). Values without a ratio do not count for the points of their part or in total. Ratios are rounded to five significant digits, so that an exact factor stays exact.

## What may be changed

| Change | Allowed | What to do |
|---|---|---|
| A new test | yes | a new entry with version 1 and a new id, then write `registry.golden` again; older reports show it as new |
| Other work of a test: settings of an intensity, a value of `BenchmarkWorkload`, SQL, the code of the runner | yes | raise its version, then write `registry.golden` again; runs of different versions are not compared |
| A new fixed value of the work of a test | yes | put it into `BenchmarkWorkload`, name it in the `constants` of its tests, raise their versions, then write `registry.golden` again |
| A title, an explanation, a hint text | yes, freely | nothing; they are never used to match |
| A test is dropped | yes | keep the entry and mark it `retired`; never remove an id |
| Rename an id | no | a new id, and mark the old one `retired`; both runs then show "new" and "no longer measured" |
| Another unit or direction | no | a new id |
| A key value becomes a compared value or information | yes | write `registry.golden` again |
| A new field at the end of a line, a new kind of line | yes | raise the minor number of the format, add fixtures of it |
| Another position or meaning of a field, a removed kind of line | only with a new major number | older reports are refused then |

The unit test catches what it can: `registry.golden` holds every entry with its unit, direction, version, kind, work and the signature of its work (see below). When the signature changes but the version does not, `benchmark_test` fails with "the work of the test changed, but its version was not raised". It also fails when an id disappears, when a unit or direction changes, and when a setting, a value of the work, a statement or an object of the DDL belongs to no measurement.

### The signature of the work

The signature of a test is made of:

- the values of the fields of `BenchmarkSettings` it uses, for the three intensities (rows, iterations, queries, seconds, clients);
- the values of `BenchmarkWorkload` (in `Benchmark.h`) it uses, the fixed part of its work that is the same in every intensity: amounts and sizes (`localFileSize`, `blobSize`, `blobChunk`, `smallCachePages`), pauses (`idlePauseMs`, `sparsePauseMs`), the size of a burst (`serverBurstIterations`, `clientBurstIterations`), repetitions (`versionReads`, `oltpPointSelects`, `largeRoundTrips`, `minSyncWrites`), rows per server call (`chunkRows`, `driveChunkRows`, `driveLookupChunk`), shares of a time or an amount (`parallelDriveTimeShare`, `largeRoundTripShare`, `commitsPerSyncWrite`), limits that decide the work (`tempSortMaxCopies`), the numbers of users (`oltpClientCounts`, `manyUserClientCounts`, `oltpKeyClients`), the page sizes of the comparison, the checked ports, the retry of the event test, and the sizes `oltpRangeRows` and `wideColumns` of `BenchmarkSql`;
- an FNV-1a hash of the SQL it uses: its statements and the DDL of its objects.

Not part of it are values that change no result: the progress display (the stages and the weights of their steps in `getBenchmarkStagePlan()`), timeouts and limits that only end a test or a run (`portCheckTimeoutMs`, `eventTimeoutMs`, `oltpMaxFailuresInRow`), the ID ranges that keep the rows of the tests apart (`parallelDriveIdBase` and the like), how often a loop checks for a cancel request, and thresholds that only classify a value for its details or a hint (`latencyOutlierMs`). These stay in `BenchmarkRunner.cpp`; the unit test reads that file and fails for a numeric constant that is neither in `BenchmarkWorkload` nor in its list of such values (`notWorkload` in `BenchmarkTest.cpp`, with the reason). So a new constant always needs a decision: it changes a result, then it belongs to `BenchmarkWorkload` and the `constants` of its tests, or it does not, then it goes into that list.

The code itself is not part of the signature: a new formula, another statistic (the median instead of the mean), another order of the steps. Raise the version of the tests yourself when it changes.

All tests have version 1. The values of `BenchmarkWorkload` were added to the signature before any report of format 2 was published, so no version had to be raised; from now on every change of them needs one.

## Updating the test data

`registry.golden`, after the version was raised where needed:

```
benchmark_test --print-golden > src/engine/testdata/benchmark/registry.golden
```

The saved reports (fixtures) in `src/engine/testdata/benchmark/`:

| File | Content |
|---|---|
| `format1-v26.10.0.md` | a report of FlameRobin 26.10.0, refused as too old |
| `format2-embedded.md`, `format2-server.md`, `format2-client.html`, `format2-anonymized.md` | reports of format 2.0: an embedded run with a skipped test, a run on the server, a run from a client with hints (HTML), the same run with hidden private information |
| `format2.9-unknown.md` | a later minor format with unknown lines, fields and measurements |
| `format9-future.md` | a newer major format, refused as too new |

Every later version must read and compare them; never change them. (The reports of format 2 were written again once, with the new marker, before format 2 was published.) When the format gets a new minor or major number, add the reports of the new format next to them (`benchmark_test --write-fixtures <directory>` writes them, then rename them to their format) and keep the old ones with the expectations of the new version.

The tests read the directory given as argument; CTest passes `src/engine/testdata/benchmark`. Without it, the tests of the files fail with a hint. The files are read with Windows line ends as well (a Git checkout may convert them).

## Known limits

- Facts are texts in the language of the run ("on", "an"); two reports in different languages show such facts as different.
- The work of the multi-user key value names its best level in the report ("30,000 transactions of 8 users"); the snapshot keeps only the transactions.
