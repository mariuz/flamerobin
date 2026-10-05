/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef FR_BENCHMARK_H
#define FR_BENCHMARK_H

#include <wx/string.h>

#include "engine/BenchmarkSystemInfo.h"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace fr
{

enum class BenchmarkIntensity
{
    Quick,
    Standard,
    Deep
};

///
/// Workload sizes used by the benchmark for one intensity level.
///
struct BenchmarkSettings
{
    int connectAttempts;
    int localFiles;
    int cpuIterations;
    int parallelConnections;
    int commitCount;
    int diskRows;
    int diskPayloadLength;
    int scanRows;
    int scanPayloadLength;
    int latencyQueries;
    int sparseQueries;
    int bursts;
    int oltpSeconds;        // per concurrency level
    int rowByRowRows;
    int prepareCount;
    int blobCount;
    int versionRounds;
    // time limit of each part of the drive test with a minimal page cache,
    // which can take very long on slow storage
    int driveSeconds;
    // data sorted by the temporary file test, more than the default sort
    // memory of SuperServer (TempCacheLimit = 64 MB)
    int tempSortMegabytes;
    int wideRows;           // inserts of the net test, one round trip each
    int oltpMaxClients;     // OLTP levels 1, 2, 4, ... up to this

    static BenchmarkSettings forIntensity(BenchmarkIntensity intensity);
};

enum class BenchmarkConnectionKind
{
    Unknown,
    Embedded,
    LocalXnet,
    TcpLoopback,
    TcpRemote
};

/// Where FlameRobin runs relative to the Firebird server
enum class BenchmarkRunLocation
{
    Unknown,
    Server,
    Client
};

/// Outcome of the event test (POST_EVENT, delivered through RemoteAuxPort)
enum class BenchmarkEventResult
{
    NotRun,
    Received,
    NotReceived,            // no notification within the time limit
    PortFiltered,           // RemoteAuxPort does not answer from this machine
    Failed,
    // not tested, because the test could wait for the TCP timeout
    NotTestedRandomPort,    // RemoteAuxPort is 0
    NotTestedUnknownPort    // RemoteAuxPort cannot be read
};

///
/// Header information of a database (isc_info_* and MON$DATABASE).
///
struct BenchmarkDatabaseInfo
{
    wxString fileName;
    int odsMajor = 0;
    int odsMinor = 0;
    int pageSize = 0;
    int64_t pages = 0;
    // page buffers set in the header (gfix -buffers); 0 = server default
    int pageBuffers = 0;
    // pages in the page cache in use (MON$PAGE_BUFFERS)
    int cachePages = 0;
    bool forcedWrites = false;
    int sweepInterval = 0;
    int64_t oldestActiveTransaction = 0;
    int64_t nextTransaction = 0;
    int cryptState = 0;
    std::optional<int> backupState;
    std::optional<int> lingerSeconds;   // RDB$LINGER, Firebird 3+
};

///
/// Result of the mixed OLTP workload for one number of parallel clients.
///
struct BenchmarkOltpResult
{
    int clients = 0;
    int64_t transactions = 0;
    int64_t conflicts = 0;
    double seconds = 0.0;
    double avgMs = 0.0;
    double p95Ms = 0.0;
};

///
/// Storage and cache results of one page size (page size comparison).
///
struct BenchmarkPageSizeResult
{
    int pageSize = 0;
    int cachePages = 0;
    double commitMs = 0.0;          // per transaction
    double insertMs = 0.0;
    double coldScanMs = 0.0;
    double warmScanMs = 0.0;
    double smallCacheInsertMs = 0.0;  // extrapolated to all rows
};

///
/// Raw values collected by BenchmarkRunner. Every measurement is optional
/// because a run can be cancelled, fail, or hit a server that does not
/// provide a certain value. All durations are in milliseconds.
///
struct BenchmarkMetrics
{
    BenchmarkSettings settings{};

    // where the benchmark runs
    BenchmarkRunLocation runLocation = BenchmarkRunLocation::Unknown;
    wxString connectionHost;        // empty for embedded connections
    wxString localHostName;
    wxString clientPlatform;
    wxString clientOsFamily;        // "Windows", "Linux", "macOS", ...
    int localCpuCount = 0;
    std::optional<int64_t> localFreeMemoryBytes;
    // power supply of this machine; empty where the OS does not report it
    std::optional<bool> localOnBattery;
    wxString tempDirectory;
    bool databaseFileIsLocal = false;
    // hardware and software of the machine running FlameRobin; with
    // runLocation Server this is the database server
    BenchmarkSystemInfo system;

    // client runs over TCP/IP: name resolution, network adapter, and which
    // server ports this machine can reach
    std::optional<BenchmarkNetworkPath> networkPath;
    int serverPort = 0;
    std::vector<BenchmarkPortCheck> serverPorts;
    // events (POST_EVENT) arrive through RemoteAuxPort on remote connections
    std::optional<double> eventMs;      // from posting to the notification
    BenchmarkEventResult eventResult = BenchmarkEventResult::NotRun;
    wxString eventStatus;               // why events were not tested or not received

    // server and connection
    wxString serverVersion;         // e.g. "5.0.4"
    int serverMajorVersion = 0;
    wxString serverImplementation;  // e.g. "WI-V5.0.4.1812 Firebird 5.0"
    wxString serverPlatform;        // derived from the implementation string
    BenchmarkConnectionKind connectionKind = BenchmarkConnectionKind::Unknown;
    wxString remoteProtocol;
    wxString remoteAddress;
    wxString clientVersion;
    std::optional<bool> wireCompressed;
    std::optional<bool> wireEncrypted;
    wxString wireCryptPlugin;           // Firebird 4+
    wxString authMethod;                // e.g. "Srp256", "Win_Sspi"
    wxString protocolVersion;           // e.g. "P17"
    wxString clientHostSeenByServer;    // MON$REMOTE_HOST
    wxString securityDatabase;          // "Default", "Self" or another file
    wxString firebirdUser;              // CURRENT_USER of the benchmark
    wxString firebirdRole;
    // the server's own host name from the version of the remote layer
    wxString serverHostName;
    std::vector<wxString> versionList;  // every entry of isc_info_version
    // timeouts of the connection (Firebird 4+, 0: none): Firebird itself
    // closes idle connections or cancels long statements
    std::optional<int> idleTimeoutSeconds;
    std::optional<int> statementTimeoutMs;
    std::map<wxString, wxString> serverConfig; // RDB$CONFIG, Firebird 4+
    // the settings of serverConfig that are set in firebird.conf instead of
    // using their default (RDB$CONFIG_IS_SET)
    std::set<wxString> serverConfigExplicit;
    // number of server processes serving the parallel connections:
    // more than one means Classic server
    std::optional<int> serverProcesses;

    // the temporary database created for the benchmark
    wxString benchmarkDirectory;    // server-side directory
    int requestedPageSize = 0;
    BenchmarkDatabaseInfo benchmarkDatabase;
    // the registered database, only when the user asked for it (read-only)
    std::optional<BenchmarkDatabaseInfo> inspectedDatabase;

    // measurements
    std::optional<double> connectMs;
    std::optional<double> createDatabaseMs;
    std::optional<double> tempDirFileMs;       // per file
    std::optional<double> databaseDirFileMs;   // per file, server only
    std::optional<double> osSyncWriteMs;       // per write + flush, server only
    std::optional<int64_t> databaseDiskFreeBytes; // server only
    std::optional<double> cpuMs;
    std::optional<double> cpuServerMs;
    std::optional<double> parallelCpuMs;
    // power management: short bursts after idle pauses vs. back to back
    std::optional<double> clientBurstIdleMs;
    std::optional<double> clientBurstHotMs;
    std::optional<double> serverBurstIdleMs;
    std::optional<double> serverBurstHotMs;
    std::optional<double> commitMs;            // per transaction
    std::optional<double> insertMs;
    std::optional<double> updateMs;
    std::optional<double> deleteMs;
    std::optional<double> garbageCollectMs;
    // the drive test: page I/O with a minimal page cache
    std::optional<int> smallCachePages;
    // time for diskRows rows, extrapolated from the rows inserted within
    // settings.driveSeconds
    std::optional<double> smallCacheInsertMs;
    std::optional<int> smallCacheRows;
    std::optional<double> smallCacheScanMs;
    // the inserted rows read by primary key, updated and deleted, each part
    // within settings.driveSeconds; the times are for the counted rows
    std::optional<double> smallCacheLookupMs;
    int smallCacheLookups = 0;
    std::optional<double> smallCacheUpdateMs;
    int smallCacheUpdates = 0;
    std::optional<double> smallCacheDeleteMs;
    int smallCacheDeletes = 0;
    // inserts with the minimal cache on parallel connections
    std::optional<double> parallelDriveMs;
    int64_t parallelDriveRows = 0;
    // sort larger than the sort memory: temporary files
    std::optional<double> tempSortMs;
    int64_t tempSortBytes = 0;
    int64_t tempSortRows = 0;
    std::optional<double> coldScanMs;
    std::optional<double> warmScanMs;
    int64_t scanBytes = 0;
    std::optional<double> serverReadMs;
    std::optional<double> serverSortMs;
    std::optional<double> latencyAvgMs;
    std::optional<double> latencyMedianMs;
    std::optional<double> latencyP95Ms;
    std::optional<double> latencyMaxMs;
    std::optional<double> latencySparseMs;     // with idle pauses in between
    // round trips that took over 150 ms and ten times the median: lost
    // packets are only sent again after such a timeout
    int latencyOutliers = 0;
    std::optional<double> largeRoundTripMs;    // 8 KB answer, client only
    std::optional<double> streamMs;
    std::optional<double> streamClientMs;      // time spent in FlameRobin itself
    int64_t streamRows = 0;
    int64_t streamBytes = 0;
    // inserts with 100 parameters each, one round trip per row (the net
    // test)
    std::optional<double> wideInsertMs;
    int wideInsertRows = 0;

    // application patterns
    std::vector<BenchmarkOltpResult> oltp;
    std::optional<double> rowByRowMs;          // one INSERT per round trip
    std::optional<double> batchMs;             // same rows in one server call
    std::optional<double> prepareMs;           // per prepare
    std::optional<double> blobWriteMs;
    std::optional<double> blobReadMs;
    int64_t blobBytes = 0;
    std::optional<double> versionReadBeforeMs; // read of frequently updated rows
    std::optional<double> versionReadWithOldTxMs;
    // the first read after the old transaction ended; it removes the old
    // record versions (cooperative garbage collection)
    std::optional<double> versionReadAfterCommitMs;
    // storage and cache tests repeated for every supported page size
    std::vector<BenchmarkPageSizeResult> pageSizes;
    // optional tests that could not run, with the reason
    std::vector<std::pair<wxString, wxString>> skippedTests;

    // engine statistics (MON$IO_STATS / MON$RECORD_STATS)
    std::optional<int64_t> pageReads;
    std::optional<int64_t> pageWrites;
    std::optional<int64_t> pageFetches;
    std::optional<int64_t> pageMarks;
    std::optional<int64_t> recordBackouts;
    std::optional<int64_t> recordPurges;
    std::optional<int64_t> recordExpunges;
};

enum class BenchmarkCategory
{
    Environment,
    Connection,
    LocalMachine,
    ServerCpu,
    ServerStorage,
    ServerCache,
    Network,
    MultiUser,
    Application,
    Engine
};

struct BenchmarkResult
{
    BenchmarkCategory category;
    wxString machine;   // which machine (or link) the value belongs to
    wxString test;
    wxString value;
    wxString details;
    wxString explanation;   // what is measured, how, and why it matters
    wxString hint;          // one sentence for the tooltip
    // the columns of the statistics table, empty where they do not apply
    wxString amount;        // e.g. "50,000 rows"
    wxString duration;      // time of the whole test
    wxString average;       // per operation
    wxString rate;          // per second
    double durationMs = 0.0;    // the duration as a number; 0: none
    // the key value this result is the source of; empty if none
    wxString valueKey;
};

///
/// Something about a run that can explain why a part of the system is
/// slower than in another run, e.g. power saving, a virus scanner or a
/// small page cache. A single run is not judged: whether a value is fast
/// or slow depends on the environment. When two runs are compared, the
/// hints of the slower run are shown for the parts that differ clearly.
/// See BenchmarkAnalyzer.cpp.
///
struct BenchmarkHint
{
    BenchmarkCategory category;     // the part it can slow down
    wxString title;     // the possible cause in plain words
    wxString detail;    // what was measured or read
    wxString advice;    // what can be done about it
};

///
/// One setting that influences the results, with what it does; the report
/// lists them without judging them.
///
struct BenchmarkSetting
{
    wxString scope;         // "firebird.conf" or "database"
    wxString name;
    wxString value;         // "?" when it could not be read
    wxString meaning;
};

///
/// A key value of a run: the most telling value of a test, as a rate
/// (work per second) or a time, with the work behind it. The report shows
/// the key values per part of the system; a comparison of two runs gives
/// points from them.
///
struct BenchmarkKeyValue
{
    wxString key;           // see BenchmarkValues.cpp
    wxString title;
    wxString valueText;     // with unit
    // the work behind the value, e.g. "200 commits" in 88 ms
    wxString amountText;
    double durationMs = 0.0;
};

/// The key values of one part of the system
struct BenchmarkPart
{
    BenchmarkCategory category;
    wxString name;
    wxString machine;
    std::vector<BenchmarkKeyValue> values;
};

///
/// The key values of a run in fixed units; 0 where not measured. See
/// BenchmarkValues.cpp.
///
struct BenchmarkValues
{
    double cpuIterationsPerMs;          // one core
    double parallelIterationsPerMs;     // all parallel connections together
    double commitMs;
    double insertRowsPerSecond;
    double driveOperationsPerSecond;    // insert, read, update, delete
    double parallelDriveRowsPerSecond;
    double tempSortMBPerSecond;
    double scanMBPerSecond;
    double sortMBPerSecond;
    double roundTripMs;
    double transferMBPerSecond;
    double connectMs;
    double wideInsertsPerSecond;
    double oltpTransactionsPerSecond;   // best level with up to 8 clients
    double clientCpuBurstMs;
};

///
/// The results of one run in a compact form. Every report (HTML and
/// Markdown) contains it as a hidden comment, so that a saved report can be
/// compared with a new run or with another saved report later.
///
struct BenchmarkSnapshot
{
    wxString title;         // the tested system, e.g. the model of the server
    wxString host;          // the computer that ran the test; empty if hidden
    wxString date;          // start of the run, "yyyy-mm-dd hh:mm:ss"
    wxString kind;          // embedded, local or client: see getBenchmarkKindId()
    wxString intensity;     // quick, standard or deep
    BenchmarkValues values{};   // 0: not measured
    // the work behind every key value by its key: amount and milliseconds
    std::map<wxString, std::pair<wxString, double>> work;
    // every measured value of the statistics
    struct Stat
    {
        BenchmarkCategory section;
        wxString key;       // the key value it is the source of
        wxString test;
        wxString value;
        wxString amount;
        double ms = 0.0;
    };
    std::vector<Stat> stats;
    // hardware, software and settings by key (see getBenchmarkFacts())
    std::vector<std::pair<wxString, wxString>> facts;
    std::vector<BenchmarkHint> hints;
};

///
/// The names of the machines of one run, used in every result, so that it
/// is always clear whose CPU, RAM, storage or network a value belongs to.
///
struct BenchmarkMachines
{
    wxString local;     // the machine running FlameRobin
    wxString server;    // the machine running Firebird
    wxString network;   // the connection between them
    wxString both;      // values that depend on both machines
};

///
/// Snapshot sent to the GUI while the benchmark runs.
///
struct BenchmarkProgress
{
    int step = 0;
    int stepCount = 0;
    wxString message;
    std::vector<BenchmarkResult> results;
    bool cleaningUp = false;
};

///
/// Collected results of one benchmark run. A report can be partial when the
/// run was cancelled or failed; errorMessage is set in the latter case.
///
struct BenchmarkReport
{
    wxString target;                // server and directory that were tested
    wxString benchmarkDatabasePath; // temporary file, dropped at the end
    bool benchmarkDatabaseCreated = false;
    bool benchmarkDatabaseDropped = false;
    // temporary databases that were created but could not be dropped
    std::vector<wxString> undroppedDatabases;
    wxString intensityName;
    wxString startedAt;
    double durationSeconds = 0.0;
    BenchmarkMetrics metrics;
    std::vector<BenchmarkResult> results;
    std::vector<BenchmarkPart> parts;
    std::vector<BenchmarkHint> hints;
    std::vector<BenchmarkSetting> settings;
    bool cancelled = false;
    wxString errorMessage;
    // names, addresses, user names and paths are removed (anonymized())
    bool anonymous = false;
    // a saved run the user compares this run with
    std::optional<BenchmarkSnapshot> earlier;

    /// fills results, parts, hints and settings from metrics
    void evaluate();
    /// a copy to share in public: computer and host names, domains, IP
    /// addresses, user names, paths, service and firewall rule names are
    /// replaced; hardware, versions, settings and measurements stay
    BenchmarkReport anonymized() const;
    /// the results only, compact, to compare and to keep
    wxString toMarkdown() const;
    /// self-contained HTML page (no external resources) with an overview
    /// for everybody and the detailed report; theme is "dark", "light" or
    /// empty to follow the system setting
    wxString toHtml(const wxString& theme = wxEmptyString) const;
};

///
/// A temporary database that may still exist after a run; the GUI remembers
/// it in the configuration until it is dropped. Stored without any
/// credentials as "<E|S>|<registration id>|<client library>|<connection
/// string>" (E = embedded). '%', ',' and '|' are escaped inside the fields,
/// because the configuration separates list entries with commas.
///
struct BenchmarkPendingDatabase
{
    bool embedded = false;
    // the database registration the run was started from: only its
    // credentials may be sent to the server of the temporary database
    wxString registrationId;
    wxString clientLibrary;
    wxString connectionString;

    wxString toString() const;
    static std::optional<BenchmarkPendingDatabase> parse(const wxString& entry);
};

/// One fact of the environment, e.g. the CPU or a firebird.conf setting;
/// the key identifies it in saved reports, the label is translated
struct BenchmarkFact
{
    wxString key;
    wxString label;
    wxString value;
};

/// Name/value rows of the system description, under a title
struct BenchmarkInfoGroup
{
    wxString title;
    std::vector<std::pair<wxString, wxString>> rows;
};

struct BenchmarkGlossaryEntry
{
    wxString term;
    wxString explanation;
};

// the description of the tested system (BenchmarkSystemReport.cpp)
/// the tested system: model of this machine for a server run, the server
/// for a client run
wxString getBenchmarkSystemTitle(const BenchmarkReport& report);
/// CPU, RAM, storage, OS and Firebird in one line
wxString getBenchmarkSystemSpecs(const BenchmarkReport& report);
/// the most important facts per machine and the network path
std::vector<BenchmarkInfoGroup> getBenchmarkSystemOverview(const BenchmarkReport& report);
/// everything that is known about hardware, software and the connection
std::vector<BenchmarkInfoGroup> getBenchmarkSystemDetails(const BenchmarkReport& report);
/// the hardware, software and settings that decide the results, in a
/// fixed order; compared between runs
std::vector<BenchmarkFact> getBenchmarkFacts(const BenchmarkReport& report);
/// the label of a fact key, also for keys this version does not know
wxString getBenchmarkFactLabel(const wxString& key);
/// abbreviations, units and technical terms of the report
std::vector<BenchmarkGlossaryEntry> getBenchmarkGlossary();
/// explanation of the unit in a value such as "1,389 it/ms"; empty if none
wxString getBenchmarkUnitHint(const wxString& value);

BenchmarkMachines getBenchmarkMachines(const BenchmarkMetrics& m);
/// the key values of this run, 0 where not measured
BenchmarkValues getBenchmarkValues(const BenchmarkMetrics& m);
/// the measured key values by part of the system, in the order of the
/// report; without the network for the embedded engine
std::vector<BenchmarkPart> getBenchmarkParts(const BenchmarkMetrics& m);

/// One value of two runs; ratio > 1: the newer run is faster
struct BenchmarkComparedValue
{
    BenchmarkCategory category;
    wxString key;           // of a key value; empty for other measurements
    wxString title;
    wxString newerText;
    wxString olderText;
    double ratio = 0.0;     // 0: not measured in one of the runs
    // the work and its duration, e.g. "200 commits in 88 ms"
    wxString newerWork;
    wxString olderWork;
};

/// A fact of the environment that differs between two runs
struct BenchmarkComparedFact
{
    wxString key;           // see getBenchmarkFacts()
    wxString label;
    wxString newerText;
    wxString olderText;
};

/// One part of the system in two runs, and where it differs clearly, what
/// could make the slower run slower
struct BenchmarkComparedPart
{
    BenchmarkCategory category;
    // the geometric mean of its key values; > 1: the newer run is faster,
    // 0: no key value was measured in both runs
    double ratio = 0.0;
    // the facts of this part that differ between the runs
    std::vector<BenchmarkComparedFact> differences;
    // what was noticed on the slower run
    std::vector<BenchmarkHint> hints;
    // typical causes of the key values that are clearly slower
    std::vector<std::pair<wxString, wxString>> advice;  // title, advice
};

///
/// Two runs side by side. A single run is not judged; in a comparison the
/// earlier run gets 1000 points for every part and in total, the later run
/// proportionally more when it is faster and fewer when it is slower. The
/// parts and the total are geometric means, so that no single value
/// dominates.
///
struct BenchmarkComparison
{
    BenchmarkSnapshot newer;
    BenchmarkSnapshot older;
    // all parts; > 1: the newer run is faster, 0: no common key values
    double ratio = 0.0;
    std::vector<BenchmarkComparedPart> parts;
    std::vector<BenchmarkComparedValue> values;
    // the other measured values; ratio from the durations of the same work
    std::vector<BenchmarkComparedValue> stats;
    std::vector<BenchmarkComparedFact> differences;
    // both runs were made the same way and with the same intensity
    bool sameKind = true;
    // the newer run is the one of the report, not a saved one
    bool newerIsCurrentRun = false;

    static constexpr double earlierPoints = 1000.0;

    const BenchmarkComparedPart* findPart(BenchmarkCategory category) const;
    /// the values that changed clearly, the largest change first
    std::vector<const BenchmarkComparedValue*> getLargestChanges(size_t maximum) const;
    wxString toMarkdown() const;
    wxString toHtml(const wxString& theme = wxEmptyString) const;
};

BenchmarkSnapshot makeBenchmarkSnapshot(const BenchmarkReport& report);
/// the hidden comment with the snapshot that every report contains
wxString formatBenchmarkSnapshot(const BenchmarkSnapshot& snapshot);
/// the snapshot of a saved report (Markdown or HTML); nothing if the text
/// contains none
std::optional<BenchmarkSnapshot> parseBenchmarkSnapshot(const wxString& text);
BenchmarkComparison compareBenchmarkSnapshots(const BenchmarkSnapshot& newer,
    const BenchmarkSnapshot& older);
/// "This run is 35 % faster overall than the earlier run."
wxString describeBenchmarkComparison(const BenchmarkComparison& comparison);
/// "What could make the earlier run slower": the title of the reasons of a
/// part that differs clearly
wxString getBenchmarkReasonsTitle(const BenchmarkComparison& comparison,
    const BenchmarkComparedPart& part);
/// the comparison as Markdown; without the table of the two runs when it
/// is part of a report
wxString formatBenchmarkComparisonMarkdown(const BenchmarkComparison& comparison,
    bool standalone);
/// every key value of two runs; ratio > 1: newer is faster
std::vector<BenchmarkComparedValue> compareBenchmarkValues(
    const BenchmarkValues& newer, const BenchmarkValues& older);
/// the key of a key value by its title; empty if unknown
wxString getBenchmarkValueKey(const wxString& title);
/// the work behind a key value: amount and milliseconds
std::map<wxString, std::pair<wxString, double>> getBenchmarkWork(const BenchmarkMetrics& m);
/// "200 commits in 88 ms"
wxString formatBenchmarkWork(const wxString& amount, double ms);
/// 0: within the normal variation of a measurement (less than 15 %),
/// 1: a clear change, 2: a large change (40 % or more)
int getBenchmarkChangeLevel(double ratio);
/// "about the same", "18 % faster", "2.3 times as fast", "35 % slower"
wxString formatBenchmarkChange(double ratio);
/// the key values with their keys, as saved in a snapshot
std::vector<std::pair<wxString, wxString>> formatBenchmarkValues(
    const BenchmarkValues& values);
/// sets one value of a snapshot; false for an unknown key
bool parseBenchmarkValue(BenchmarkValues& values, const wxString& key,
    const wxString& text);
/// a copy to share in public: without the computer name, server names and
/// paths
BenchmarkSnapshot anonymizeBenchmarkSnapshot(const BenchmarkSnapshot& snapshot);
/// "embedded", "local" or "client": how the run was made; runs of
/// different kinds measure different things
wxString getBenchmarkKindId(const BenchmarkMetrics& m);
wxString getBenchmarkKindName(const wxString& kindId);
/// whether a fact (see getBenchmarkFacts()) can influence a part of the
/// system, e.g. the power plan the processor; the facts of the machine
/// running FlameRobin belong to the server only on a run on the server
bool isBenchmarkFactOfPart(const wxString& key, BenchmarkCategory part, bool serverRun);
/// what a section measures, in one plain sentence
wxString getBenchmarkSectionDescription(BenchmarkCategory section,
    const BenchmarkMetrics& m);
wxString formatBenchmarkPoints(double points);
std::vector<BenchmarkResult> buildBenchmarkResults(const BenchmarkMetrics& m);
/// what could slow down each part of the system in this run
std::vector<BenchmarkHint> findBenchmarkHints(const BenchmarkMetrics& m);
/// the typical causes when a key value is slower than in another run
wxString getBenchmarkValueAdvice(const wxString& key);
/// the settings of Firebird and of the inspected database that influence
/// the results
std::vector<BenchmarkSetting> getBenchmarkSettings(const BenchmarkMetrics& m);

wxString getBenchmarkIntensityName(BenchmarkIntensity intensity);
wxString getBenchmarkCategoryName(BenchmarkCategory category);
/// the report shows the results in these sections, in this order
std::vector<BenchmarkCategory> getBenchmarkReportSections();
/// the section a category is shown in (connection: network, engine
/// statistics: configuration)
BenchmarkCategory getBenchmarkReportSection(BenchmarkCategory category);
wxString getBenchmarkReportSectionName(BenchmarkCategory section);
wxString getBenchmarkConnectionKindName(BenchmarkConnectionKind kind);
wxString getBenchmarkRunLocationName(BenchmarkRunLocation location);
BenchmarkConnectionKind classifyBenchmarkConnection(const wxString& protocol,
    const wxString& address);
BenchmarkRunLocation determineBenchmarkRunLocation(BenchmarkConnectionKind kind,
    const wxString& connectionHost, const wxString& localHostName,
    bool databaseFileIsLocal);
/// whether the event test can run without waiting for the TCP timeout of
/// the operating system; if not, sets eventResult and eventStatus. Uses
/// connectionKind, serverMajorVersion, serverConfig and serverPorts.
bool prepareBenchmarkEventTest(BenchmarkMetrics& m);
/// every entry of an isc_info_version answer, which is a counted list of
/// strings ("\x02\x1bLI-V6.3.4.1812 Firebird 5.0..."); a plain string is
/// returned as the only entry
std::vector<wxString> parseBenchmarkVersionList(const std::string& infoVersion);
/// the host name the server reports for itself in the version of the
/// remote layer ("WI-V5.0.4.1812 Firebird 5.0/tcp (dbserver)/P19:C")
wxString getBenchmarkServerHostName(const std::vector<wxString>& versionList);
wxString getServerPlatformName(const wxString& implementation);
wxString formatBenchmarkMilliseconds(double milliseconds);
/// with thousands separators: 44,944
wxString formatBenchmarkCount(double value);
/// the time of one operation; below 1 ms in microseconds
wxString formatBenchmarkAverage(double milliseconds);
/// e.g. "1 Gbit/s", "100 Mbit/s"
wxString formatBenchmarkBitRate(double bitsPerSecond);
/// an IP address without brackets, port, zone or IPv4-mapped prefix, in
/// lower case: "[FE80::1%3]" -> "fe80::1", "10.0.0.5/51234" -> "10.0.0.5"
wxString normalizeBenchmarkAddress(const wxString& text);
/// whether a text is an IPv4 or IPv6 address (see normalizeBenchmarkAddress())
bool isBenchmarkIpAddress(const wxString& text);
/// an IP address that identifies a machine: not the loopback address and not
/// the address that stands for all addresses
bool isBenchmarkPrivateAddress(const wxString& text);
/// a firebird.conf setting whose value is an address of a machine
bool isBenchmarkAddressSetting(const wxString& name, const wxString& value);
/// all operations of the drive test (insert, read, update, delete with the
/// minimal page cache) per second; 0 if not measured
double getBenchmarkDriveOpsPerSecond(const BenchmarkMetrics& m);
wxString escapeMarkdownTableCell(const wxString& text);

/// directory part of a server-side database path; empty for aliases
wxString getBenchmarkDirectoryFromPath(const wxString& databasePath);
/// full path of the temporary benchmark database in a server-side directory
wxString makeBenchmarkDatabasePath(const wxString& directory,
    const wxString& uniqueName);
/// true if the file name has the exact form of a benchmark database:
/// FR_BENCHMARK_<yyyymmdd>_<hhmmss>_<6 hex digits>[_P<page size>].FDB
bool isBenchmarkDatabaseName(const wxString& fileName);
/// true if the attached file name is the benchmark database with that
/// unique name; the safety check before every drop
bool isBenchmarkDatabaseFile(const wxString& attachedFileName,
    const wxString& uniqueName);
/// page sizes a Firebird version supports (32768 needs Firebird 4)
std::vector<int> getSupportedPageSizes(int serverMajorVersion);
/// percentile (p from 0 to 1: the value at the rounded position p * (n - 1)
/// of the sorted values) and average; both 0 for no values
double getBenchmarkPercentile(std::vector<double> values, double p);
double getBenchmarkAverage(const std::vector<double>& values);

///
/// All SQL executed by the benchmark inside its own temporary database.
/// Statements use "?" placeholders for workload sizes.
///
namespace BenchmarkSql
{
    // DDL in creation order; each statement is committed separately
    std::vector<const char*> getCreateStatements();
    // the part of the DDL needed by the storage and cache tests (test
    // table, its indexes and the row generator); used per page size
    std::vector<const char*> getStorageCreateStatements();
    // FR_BENCHMARK_RANGE counts this many neighbouring IDs
    const int oltpRangeRows = 100;
    // columns of FR_BENCHMARK_WIDE, the table of the net test
    const int wideColumns = 100;
    const char* getWideTableStatement();
    // INSERT with one parameter per column
    const char* getWideInsertStatement();

    extern const char* const engineVersion;
    extern const char* const attachmentInfo;
    extern const char* const attachmentWireInfo;
    extern const char* const sessionUser;
    extern const char* const attachmentDetails;
    extern const char* const attachmentTimeouts;
    extern const char* const securityDatabase;
    extern const char* const databaseInfo;
    extern const char* const databaseLinger;
    extern const char* const serverConfig;
    extern const char* const postEvent;
    extern const char* const eventName;
    extern const char* const driveLookups;
    extern const char* const tempSort;
    extern const char* const cpuTest;
    extern const char* const commitInsert;
    extern const char* const fillTable;
    extern const char* const bulkUpdate;
    extern const char* const bulkDelete;
    extern const char* const garbageCollect;
    extern const char* const scan;
    extern const char* const serverRead;
    extern const char* const latency;
    extern const char* const largeRoundTrip;
    extern const char* const throughput;
    extern const char* const ioStats;
    extern const char* const recordStats;
    extern const char* const rowInsert;
    extern const char* const fillRows;
    extern const char* const prepareProbe;
    extern const char* const oltpPointSelect;
    extern const char* const oltpRangeSelect;
    extern const char* const oltpUpdate;
    extern const char* const oltpInsert;
    extern const char* const blobInsert;
    extern const char* const blobRead;
    extern const char* const hotFill;
    extern const char* const hotUpdate;
    extern const char* const hotRead;
    extern const char* const serverProcesses;
} // namespace BenchmarkSql

} // namespace fr

#endif // FR_BENCHMARK_H
