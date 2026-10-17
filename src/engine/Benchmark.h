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
#include <functional>
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
    struct SkippedTest
    {
        wxString id;        // of the measurement, see the register
        wxString name;
        wxString reason;
    };
    std::vector<SkippedTest> skippedTests;

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
    // identifies the measurement in saved reports, in every language and
    // version; see the register in BenchmarkValues.cpp
    wxString id;
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
    // the amount as a number and its unit, untranslated ("rows", "bytes"),
    // as saved in the snapshot; 0 and empty where it does not apply
    double amountValue = 0.0;
    wxString amountUnit;
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
    // the kind of hint, e.g. "power.saving"; the same for every part it is
    // given for, never translated
    wxString id;
    BenchmarkCategory category;     // the part it can slow down
    wxString title;     // the possible cause in plain words
    wxString detail;    // what was measured or read
    wxString advice;    // what can be done about it
    // what was noticed, as a state or a ratio without a cause or a judgement,
    // for the report of a single run; empty where the title says it already.
    // Not saved: a saved report has the title only
    wxString noticed;
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
    wxString id;            // see the register in BenchmarkValues.cpp
    wxString title;
    double value = 0.0;     // in the unit of the register
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

/// how a measurement is compared between two runs
enum class BenchmarkMeasurementKind
{
    Key,        // a key value: compared, and the points come from it
    Measure,    // compared by its value
    Info        // only shown, never compared
};

///
/// An entry of the register of measurements (BenchmarkValues.cpp). The id
/// identifies a measurement in saved reports, in every language and in every
/// later version of FlameRobin; devdocs/Benchmark_report_format.md explains
/// what may be changed.
///
struct BenchmarkMeasurementDef
{
    // the part of the system (see getBenchmarkSectionId()) and a name, e.g.
    // "storage.commit"; a family ends before its parameter: "users.tx" for
    // "users.tx.8"
    const char* id;
    // raised whenever the work of the test changes, so that runs with
    // different work are not compared
    int version;
    BenchmarkCategory section;
    BenchmarkMeasurementKind kind;
    const char* unit;           // of the value; empty for information
    bool higherIsBetter;
    // every run of an intensity does the same work; false for the tests
    // with a time limit
    bool amountFixed;
    // the work of the test, checked by the unit test of the register: the
    // fields of BenchmarkSettings, the values of BenchmarkWorkload (and the
    // sizes of BenchmarkSql), and the statements of BenchmarkSql and
    // objects of its DDL that it uses, separated by spaces
    const char* settings;
    const char* constants;
    const char* sql;
    // of a key value: its title, translated with _(), and its member
    const char* title;
    double BenchmarkValues::*value;
    bool family;
    // no longer measured; kept so that older reports can still be compared
    bool retired;
};

/// every measurement, in the order of the report
const std::vector<BenchmarkMeasurementDef>& getBenchmarkMeasurementDefs();
/// the entry of an id, also of a member of a family ("users.tx.8"); nullptr
/// for an id that this version does not know
const BenchmarkMeasurementDef* findBenchmarkMeasurementDef(const wxString& id);
/// whether a text can be an id: letters and digits, parts separated by dots
bool isBenchmarkMeasurementId(const wxString& text);
/// the id of a member of a family: "users.tx" and "8" give "users.tx.8"
wxString makeBenchmarkMeasurementId(const char* family, const wxString& parameter);
/// a value in the unit of a measurement: "1,389 it/ms", "0.44 ms"
wxString formatBenchmarkValue(double value, const BenchmarkMeasurementDef& def);
/// whether a result is the one a key value is calculated from; a skipped
/// test is not, it is listed with the programming patterns
bool isBenchmarkKeyValueSource(const BenchmarkResult& result);

///
/// One measurement of a run as it is saved: the numbers in the same form in
/// every language, the texts only to show it.
///
struct BenchmarkMeasurement
{
    wxString id;            // see the register
    double value = 0.0;     // in unit; 0: no number, only the text
    wxString unit;
    int version = 0;        // of the test that measured it
    // the work behind the value: this amount of amountUnit ("rows",
    // "bytes", untranslated) in ms; 0 and empty where it does not apply
    double amount = 0.0;
    wxString amountUnit;
    double ms = 0.0;
    // in the language of the run, never used to find a measurement
    wxString title;
    wxString text;          // the value as the report showed it
};

///
/// The results of one run in a compact form. Every report (HTML and
/// Markdown) contains it as a hidden comment, so that a saved report can be
/// compared with a new run or with another saved report later; see
/// devdocs/Benchmark_report_format.md.
///
struct BenchmarkSnapshot
{
    wxString program;       // the version of FlameRobin that made the run
    wxString title;         // the tested system, e.g. the model of the server
    wxString host;          // the computer that ran the test; empty if hidden
    wxString date;          // start of the run, "yyyy-mm-dd hh:mm:ss"
    wxString kind;          // embedded, local or client: see getBenchmarkKindId()
    wxString intensity;     // quick, standard or deep
    // every measurement of the run in the order of the report, also those
    // that this version does not know
    std::vector<BenchmarkMeasurement> measurements;
    // hardware, software and settings by key (see getBenchmarkFacts())
    std::vector<std::pair<wxString, wxString>> facts;
    std::vector<BenchmarkHint> hints;

    /// the measurement with this id; nullptr if the run has none
    const BenchmarkMeasurement* findMeasurement(const wxString& id) const;
};

/// whether a saved report can be read
enum class BenchmarkSnapshotStatus
{
    Ok,
    NotAReport,     // the text contains no results
    TooOld,         // made by an older version of FlameRobin (format 1)
    TooNew,         // made by a newer version with another major format
    Damaged         // the results are incomplete or cannot be read
};

/// the results of a saved report and whether they could be read
struct BenchmarkSnapshotResult
{
    BenchmarkSnapshotStatus status = BenchmarkSnapshotStatus::NotAReport;
    wxString format;        // as written in the report, e.g. "1" or "2.0"
    BenchmarkSnapshot snapshot;     // only with Ok

    bool ok() const { return status == BenchmarkSnapshotStatus::Ok; }
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
/// The stages of a run, in the order in which they run. Every step of the
/// runner belongs to one; the progress shows them.
///
enum class BenchmarkStage
{
    Prepare,    // the temporary database, the server, this computer
    Connect,    // network path, connections, test tables, round trips
    Cpu,
    Disk,       // the commit latency and the disk tests
    Memory,     // cache and sorting
    Network,    // result sets, wide rows and the programming patterns
    Users,      // one step per OLTP level
    PageSizes,  // one step per page size, only with "Compare all page sizes"
    Finish      // the temporary databases are dropped; no step of its own
};

/// one step of a stage in the plan of a run
struct BenchmarkStepPlan
{
    // its share of the run, about its seconds in a Standard run (measured),
    // so that the progress moves evenly and does not stand for a minute
    double weight;
    // a short measurement that other work on the computer would disturb;
    // the logo of the progress only breathes while it runs
    bool quiet;
};

struct BenchmarkStagePlan
{
    BenchmarkStage stage;
    std::vector<BenchmarkStepPlan> steps;

    double getWeight() const;
};

/// the state of a stage in the progress
enum class BenchmarkStageState
{
    Waiting,
    Running,
    Done,
    Skipped,    // every step of the stage had nothing to do in this run
    Failed,     // the run ended with an error in this stage
    Cancelled,  // the run was cancelled in this stage
    NotRun      // a later stage after a cancel or an error
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
    // the stage of the step in the plan of the run (Finish while cleaning up)
    BenchmarkStage stage = BenchmarkStage::Prepare;
    int stageIndex = 0;
    int stageCount = 0;
    // the step within the stage, from 0
    int stageStep = 0;
    int stageSteps = 0;
    // the share of the run, by the weights of the steps (0 to 1)
    double fraction = 0.0;
    // the step has nothing to do in this run, e.g. the selected database
    // that was not to be read
    bool skipped = false;
    // see BenchmarkStepPlan::quiet
    bool quiet = false;
};

///
/// Follows the progress of a run and knows the state of every stage, also
/// after a cancel or an error.
///
class BenchmarkStageTracker
{
public:
    enum class Outcome { Finished, Cancelled, Failed };

    explicit BenchmarkStageTracker(const std::vector<BenchmarkStagePlan>& plan
        = std::vector<BenchmarkStagePlan>());

    void update(const BenchmarkProgress& progress);
    /// the end of the run; cleanupFailed: the temporary database could not
    /// be dropped
    void finish(Outcome outcome, bool cleanupFailed = false);

    const std::vector<BenchmarkStagePlan>& getPlan() const { return planM; }
    BenchmarkStageState getState(size_t index) const;
    /// the stage that runs (or ran last), -1 before the first step
    int getCurrent() const { return currentM; }
    /// the stage the run ended in after a cancel or an error, else -1
    int getEndStage() const { return endM; }
    /// the share of the run (0 to 1)
    double getFraction() const { return fractionM; }
    /// the share of the steps of a stage that are done (0 to 1)
    double getStageFraction(size_t index) const;

private:
    std::vector<BenchmarkStagePlan> planM;
    std::vector<BenchmarkStageState> statesM;
    // whether every step of a stage so far was skipped
    std::vector<bool> skippedM;
    int currentM = -1;
    int stepM = 0;
    // the last stage before the cleanup, which an error belongs to
    int workM = -1;
    int endM = -1;
    double fractionM = 0.0;
};

///
/// What the animation of the progress depends on. It is small (4 pictures
/// a second), but it must never take time from a measurement, and not
/// send pictures over the network of a remote session.
///
struct BenchmarkAnimationContext
{
    bool running = false;           // a run is going on
    bool quietStep = false;         // see BenchmarkStepPlan::quiet
    bool windowVisible = true;      // the progress can be seen
    bool remoteSession = false;     // e.g. Remote Desktop
    bool systemAnimations = true;   // the system shows animations
    bool userDisabled = false;      // FLAMEROBIN_NO_ANIMATION is set
};

/// how the logo of the progress moves
enum class BenchmarkAnimation
{
    Still,      // one picture: no run, nobody looks, animations are off
    Breathe,    // the filling rises and sinks a little, 1 picture a second
    Wave        // the edge of the filling moves as a wave, the robin blinks
};

/// the pictures of the wave: 4 a second, never more than 8
const int benchmarkAnimationIntervalMs = 250;
const int benchmarkMinAnimationIntervalMs = 125;
/// the pictures of the breath: 1 a second
const int benchmarkBreathIntervalMs = 1000;
/// the robin closes its eye once in this time, only with the wave
const int benchmarkBlinkIntervalMs = 6000;
/// without the animation the timer only shows the elapsed time
const int benchmarkElapsedIntervalMs = 1000;

/// the wave in normal steps; the breath in quiet steps and in a remote
/// session; still without a run, while nobody looks, without the animations
/// of the system and with FLAMEROBIN_NO_ANIMATION
BenchmarkAnimation getBenchmarkAnimation(const BenchmarkAnimationContext& context);
/// the interval of the pictures in ms, 0 for no animation
int getBenchmarkAnimationInterval(const BenchmarkAnimationContext& context);
/// the interval of the blink in ms, 0 for none
int getBenchmarkBlinkInterval(const BenchmarkAnimationContext& context);
/// the interval of the one timer of the progress: the animation, or once a
/// second for the elapsed time; 0 when no run is going on (it stops)
int getBenchmarkTimerInterval(const BenchmarkAnimationContext& context);

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
    // the level is stored in the snapshot; the name is translated, for display
    BenchmarkIntensity intensity = BenchmarkIntensity::Standard;
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
    // the private texts that anonymized() replaced, the longest first, and
    // what replaces them: toMarkdown() and toHtml() replace them in their
    // whole text as well, see hideBenchmarkTexts()
    std::vector<std::pair<wxString, wxString>> hiddenTexts;
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

/// What the benchmark does about the login of the selected registration
/// when a test on its server starts
enum class BenchmarkLoginAction
{
    None,           // trusted authentication, or the user entered a login
    UseStored,      // the login of the connection or of the registration
    Ask,            // ask for user name and password, only for this run
    Abort           // the master password was not entered: do not start
};

/// The state of the registration that decides the login; the password is
/// the decrypted one, which needs the master password in the encrypted mode
struct BenchmarkLoginState
{
    bool userEntered = false;       // a user name in the benchmark dialog
    bool trustedUser = false;
    bool connected = false;
    bool alwaysAsk = false;
    bool encryptedPassword = false;
    bool hasStoredPassword = false; // saved, maybe encrypted
    bool hasUsername = false;
    bool hasPassword = false;       // after decryption
};

BenchmarkLoginAction getBenchmarkLoginAction(const BenchmarkLoginState& state);

/// Where the benchmark runs, as chosen in the dialog
enum class BenchmarkTarget
{
    Registered,     // the server of the selected database
    ThisComputer,   // embedded: no server connection, no login
    OtherServer     // a server entered in the dialog
};

/// the targets the dialog lists, in this order; the server of the selected
/// database only with a registration
std::vector<BenchmarkTarget> getBenchmarkTargets(bool hasRegistration);
/// the target of an entry of that list; this computer for no entry
BenchmarkTarget getBenchmarkTarget(bool hasRegistration, int selection);

/// What the dialog asks for on the page of a target
struct BenchmarkTargetDetails
{
    bool hostAndPort = false;
    bool login = false;             // user name and password
    // without a user name the login of the registration is used
    bool loginOptional = false;
    bool installation = false;      // a Firebird installation of this computer
    bool readDatabase = false;      // can read the settings of the selected database
};

BenchmarkTargetDetails getBenchmarkTargetDetails(BenchmarkTarget target);

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

///
/// A key figure: one of the few key values that the overview of a report
/// shows in large, with a short name. A single run is not judged, so a key
/// figure is a measured value only.
///
struct BenchmarkHeadline
{
    wxString id;            // see the register
    BenchmarkCategory section;
    wxString name;          // short, e.g. "Writing rows"
    wxString number;        // "40,613"
    wxString unit;          // short, e.g. "rows/s"
    wxString work;          // "50,000 rows in 1231 ms"
};
/// the ids of the key figures in the order of the report: processor, saving
/// a change, writing rows, reading from memory, the network and several
/// users; every one is a key value of the register
const std::vector<const char*>& getBenchmarkHeadlineIds();
/// the key figures that a run measured; the embedded engine has no network
std::vector<BenchmarkHeadline> getBenchmarkHeadlineValues(const BenchmarkSnapshot& run);
/// a value in large: its number and its short unit, e.g. "40,613" and
/// "rows/s", "0.56" and "ms"
std::pair<wxString, wxString> formatBenchmarkFigure(double value,
    const BenchmarkMeasurementDef& def);
/// the notes of a run: every kind of hint once, for the first part it was
/// given for, in the order they were found; at most maximum
std::vector<BenchmarkHint> getBenchmarkTopHints(const BenchmarkReport& report,
    size_t maximum);

/// whether a measurement of two runs could be compared
enum class BenchmarkCompareState
{
    // both runs have it from the same test; it has a ratio when both have
    // a number that is compared (not for information)
    Compared,
    OnlyNewer,      // new: only the newer run has it
    OnlyOlder,      // no longer measured: only the older run has it
    TestChanged,    // not comparable: the test changed (another version)
    WorkChanged     // not comparable: the test did other work, e.g. more rows
};

/// One value of two runs; ratio > 1: the newer run is faster
struct BenchmarkComparedValue
{
    BenchmarkCategory category;
    wxString id;            // of the measurement, see the register
    wxString title;
    wxString newerText;
    wxString olderText;
    double ratio = 0.0;     // 0: not compared, see state
    BenchmarkCompareState state = BenchmarkCompareState::Compared;
    // the versions of the test; 0 in the run that does not have it
    int newerVersion = 0;
    int olderVersion = 0;
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
    // the key values in the order of the register
    std::vector<BenchmarkComparedValue> values;
    // the other measurements in the order of the newer run, then those that
    // only the older run has
    std::vector<BenchmarkComparedValue> stats;
    std::vector<BenchmarkComparedFact> differences;
    // both runs were made the same way and with the same intensity
    bool sameKind = true;
    // the newer run is the one of the report, not a saved one
    bool newerIsCurrentRun = false;
    // the computer names of anonymizeBenchmarkComparison(), replaced in the
    // whole text of toMarkdown() and toHtml()
    std::vector<std::pair<wxString, wxString>> hiddenTexts;

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
/// the snapshot of a saved report (Markdown or HTML), or why it cannot be
/// read: lines and fields that this version does not know are skipped
BenchmarkSnapshotResult parseBenchmarkSnapshot(const wxString& text);
/// why the results of a saved report cannot be compared, in one or two
/// sentences; empty if they can
wxString getBenchmarkSnapshotProblem(const BenchmarkSnapshotResult& result);
/// where the snapshot comment of a finished report starts ("<!--"); npos if
/// the text has none
size_t findBenchmarkSnapshot(const wxString& text);
/// the comment that marks a saved comparison of two runs (HTML or
/// Markdown). A comparison holds no results of its own: it cannot be compared
/// again, but FlameRobin can show it
wxString getBenchmarkComparisonMarker();
bool isBenchmarkComparisonPage(const wxString& text);
/// applies hide to the private texts of a snapshot comment line by line:
/// the keys, ids, numbers and units stay as they are, so that a private
/// name cannot change them
wxString hideBenchmarkSnapshotTexts(const wxString& comment,
    const std::function<wxString(const wxString&)>& hide);
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

/// A key figure in two runs, see getBenchmarkHeadlineValues()
struct BenchmarkHeadlineChange
{
    wxString id;
    BenchmarkCategory section;
    wxString name;
    wxString unit;
    wxString olderNumber;   // empty if the run has none
    wxString newerNumber;
    double ratio = 0.0;     // > 1: the newer run is faster, 0: not compared
    BenchmarkCompareState state = BenchmarkCompareState::Compared;
};
/// every key figure that one of the runs measured
std::vector<BenchmarkHeadlineChange> getBenchmarkHeadlineChanges(
    const BenchmarkComparison& comparison);
/// the key figures of a comparison counted by their change: the main
/// statement of a comparison
struct BenchmarkVerdict
{
    int faster = 0;
    int slower = 0;
    int same = 0;           // within the normal variation
    int notComparable = 0;  // in one run only, or the test or its work changed
};
BenchmarkVerdict getBenchmarkVerdict(const BenchmarkComparison& comparison);
/// the main statement in words, e.g. "4 key figures faster", "1 slower",
/// "1 about the same", without the counts that are 0; with the direction of
/// each: 1 faster, -1 slower, 0 about the same
std::vector<std::pair<int, wxString>> describeBenchmarkVerdict(const BenchmarkVerdict& verdict);
/// what the main statement leaves out: the parts that one run did not
/// measure ("NET measured in this run only") and the key figures that were
/// not compared
std::vector<wxString> getBenchmarkVerdictNotes(const BenchmarkComparison& comparison);
/// whether a part was measured in both runs: OnlyNewer or OnlyOlder when
/// all its values are in one run only, TestChanged when none of them could
/// be compared, otherwise Compared
BenchmarkCompareState getBenchmarkPartState(const BenchmarkComparison& comparison,
    BenchmarkCategory section);
/// the parts that differ clearly, the largest change first; at most maximum
std::vector<const BenchmarkComparedPart*> getBenchmarkStandOut(
    const BenchmarkComparison& comparison, size_t maximum);
/// the most likely reason why a part differs: the first fact that differs,
/// otherwise the first hint of the slower run, otherwise the first typical
/// cause; empty if there is none
wxString getBenchmarkLikelyReason(const BenchmarkComparedPart& part);
/// what to try about a part that differs, also when it got faster: the
/// advice about the first fact that differs, otherwise the first sentence
/// of the advice of the first hint or typical cause
wxString getBenchmarkTryAdvice(const BenchmarkComparedPart& part);
/// the first sentence of an advice, for one line
wxString getBenchmarkShortAdvice(const wxString& advice);
/// "new", "no longer measured" or "not comparable"; empty for a value that
/// was compared
wxString getBenchmarkCompareStateName(BenchmarkCompareState state);
/// why a value of two runs was not compared, in one sentence; empty if it
/// was
wxString describeBenchmarkCompareState(const BenchmarkComparison& comparison,
    const BenchmarkComparedValue& value);
/// the work behind a value in both runs: "200 commits: 820 ms -> 240 ms",
/// or both works where the amounts differ
wxString formatBenchmarkComparedWork(const BenchmarkComparison& comparison,
    const BenchmarkComparedValue& value);
/// the work behind a key value: this amount of unit in ms, and how the
/// report shows the amount
struct BenchmarkWork
{
    double amount = 0.0;
    wxString unit;          // untranslated: "rows", "bytes", ...
    double ms = 0.0;
    wxString text;          // e.g. "30,000 transactions of 8 users"
};
/// the work behind every key value by its id
std::map<wxString, BenchmarkWork> getBenchmarkWork(const BenchmarkMetrics& m);
/// "50,000 rows", "49.6 MB": an amount with its untranslated unit, in the
/// language of the user; empty without a unit
wxString formatBenchmarkAmount(double amount, const wxString& unit);
/// "200 commits in 88 ms"
wxString formatBenchmarkWork(const wxString& amount, double ms);
/// 0: within the normal variation of a measurement (less than 15 %),
/// 1: a clear change, 2: a large change (40 % or more)
int getBenchmarkChangeLevel(double ratio);
/// "about the same", "18 % faster", "2.3 times as fast", "35 % slower"
wxString formatBenchmarkChange(double ratio);
/// a copy to share in public: without the computer name, server names and
/// paths
BenchmarkSnapshot anonymizeBenchmarkSnapshot(const BenchmarkSnapshot& snapshot);
/// a comparison to share in public: both runs anonymized, and their computer
/// names replaced in the whole text
BenchmarkComparison anonymizeBenchmarkComparison(const BenchmarkComparison& comparison);
/// replaces private texts in a finished report (Markdown or HTML) without
/// regard to case, in the given order; a text shorter than 6 characters only
/// as a word of its own. In HTML the texts, comments and the values of
/// attributes such as aria-label, not the style sheet, the elements and the
/// values of class, id and similar attributes.
wxString hideBenchmarkTexts(const wxString& text,
    const std::vector<std::pair<wxString, wxString>>& hidden, bool html);
/// "embedded", "local" or "client": how the run was made; runs of
/// different kinds measure different things
wxString getBenchmarkKindId(const BenchmarkMetrics& m);
wxString getBenchmarkKindName(const wxString& kindId);
/// the style sheet of the HTML report for Internet Explorer 11 as well:
/// before every declaration with var(), grid or overflow-wrap: anywhere one
/// that it understands, with the values of the light or the dark theme;
/// the other browsers take the later declaration, as before
wxString addBenchmarkCssFallbacks(const wxString& css, bool dark);
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
wxString getBenchmarkValueAdvice(const wxString& id);
/// what to do when a fact (see getBenchmarkFacts()) differs between two
/// runs, in one or two sentences; empty for a fact without advice
wxString getBenchmarkFactAdvice(const wxString& key);
/// the settings of Firebird and of the inspected database that influence
/// the results
std::vector<BenchmarkSetting> getBenchmarkSettings(const BenchmarkMetrics& m);

wxString getBenchmarkIntensityName(BenchmarkIntensity intensity);
/// how long a run of an intensity takes, e.g. "about 2 minutes"
wxString getBenchmarkIntensityDuration(BenchmarkIntensity intensity);
/// the same with what the options add: "about 2 minutes, plus about half a
/// minute for 50 and 250 users"
wxString getBenchmarkDurationHint(BenchmarkIntensity intensity, bool comparePageSizes,
    bool manyUsers);
/// the parallel clients of the OLTP levels of a run
std::vector<int> getBenchmarkOltpLevels(const BenchmarkSettings& settings, bool manyUsers);
/// the stages of a run with these options, in their order, each with its
/// steps; the page sizes only when they are compared, Finish always last
std::vector<BenchmarkStagePlan> getBenchmarkStagePlan(BenchmarkIntensity intensity,
    bool comparePageSizes, bool manyUsers);
/// the number of steps of a plan
int getBenchmarkStepCount(const std::vector<BenchmarkStagePlan>& plan);
/// the share of the run while a step runs (0 to 1): the steps before it and
/// half of it, by their weights; Finish ends at 1
double getBenchmarkProgressFraction(const std::vector<BenchmarkStagePlan>& plan,
    int stageIndex, int stageStep);
/// a short word for a stage, e.g. "Prepare", "Disk"
wxString getBenchmarkStageWord(BenchmarkStage stage);
/// what a stage does, e.g. "Memory and cache"
wxString getBenchmarkStageName(BenchmarkStage stage);
/// the state of a stage in words, e.g. "done", for screen readers
wxString getBenchmarkStageStateName(BenchmarkStageState state);
wxString getBenchmarkCategoryName(BenchmarkCategory category);
/// the report shows the results in these sections, in this order
std::vector<BenchmarkCategory> getBenchmarkReportSections();
/// the section a category is shown in (connection: network, engine
/// statistics: configuration)
BenchmarkCategory getBenchmarkReportSection(BenchmarkCategory category);
wxString getBenchmarkReportSectionName(BenchmarkCategory section);
/// a short word for a section, e.g. "CPU", "DISK", "RAM", "NET"
wxString getBenchmarkReportSectionWord(BenchmarkCategory section);
/// "cpu", "storage", "memory", "network", "users", "computer",
/// "application" or "settings": the first part of the ids of a section,
/// the same in every language
wxString getBenchmarkSectionId(BenchmarkCategory section);
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
/// The fixed part of the work of the tests, the same in every intensity
/// (BenchmarkRunner.cpp). Every value belongs to the tests whose results it
/// changes: the register (BenchmarkValues.cpp) names it with them, so that
/// a change shows in registry.golden and needs a new version of the tests;
/// see devdocs/Benchmark_report_format.md.
///
namespace BenchmarkWorkload
{
    // page sizes of the optional comparison, and the commits measured in
    // each of their databases
    const int comparedPageSizes[] = { 4096, 8192, 16384, 32768 };
    const int commitsPerPageSize = 50;
    // every commit of the commit test needs this many round trips, which
    // are subtracted from its time (the execute and the commit)
    const int commitRoundTrips = 2;
    // page cache of the storage test that keeps the I/O subsystem busy
    const int smallCachePages = 50;
    // the pause before every burst of the power saving tests, and the pause
    // before every query of the round trips after idle pauses
    const int idlePauseMs = 500;
    const int sparsePauseMs = 200;
    // the work of one burst: iterations of the CPU procedure on the server,
    // iterations of the loop in FlameRobin
    const int serverBurstIterations = 50000;
    const int clientBurstIterations = 3000000;
    // the bytes of a file of the file tests and of a write of the
    // synchronous write test, which writes commitCount / commitsPerSyncWrite
    // times, at least minSyncWrites times
    const int localFileSize = 4096;
    const int commitsPerSyncWrite = 4;
    const int minSyncWrites = 20;
    // round trips with a larger answer: latencyQueries / largeRoundTripShare,
    // at least largeRoundTrips
    const int largeRoundTrips = 50;
    const int largeRoundTripShare = 10;
    // the size of a BLOB and of every part written and read
    const int blobSize = 64 * 1024;
    const int blobChunk = 16 * 1024;
    // reads of the frequently updated rows before and while an old
    // transaction is open
    const int versionReads = 20;
    // OLTP levels up to settings.oltpMaxClients, and the many users option
    const int oltpClientCounts[] = { 1, 2, 4, 8, 16, 32 };
    const int manyUserClientCounts[] = { 50, 250 };
    // primary key lookups of every OLTP transaction
    const int oltpPointSelects = 5;
    // the multi-user key value is the best level with up to this many users,
    // so that the intensities stay comparable
    const int oltpKeyClients = 8;
    // rows per server call of the inserts, updates and deletes; keeps every
    // call short enough for a quick cancel
    const int chunkRows = 5000;
    // rows and lookups per server call of the time limited drive test
    const int driveChunkRows = 500;
    const int driveLookupChunk = 2000;
    // the parallel drive test runs this share of driveSeconds
    const int parallelDriveTimeShare = 2;
    // the temporary file test sorts at most this many copies of the rows
    const int tempSortMaxCopies = 64;
    // ports of Firebird servers checked from a client
    const int checkedPortFirst = 3050;
    const int checkedPortLast = 3060;
    // the event test posts again after this time without a notification;
    // the first notification of a new listener carries no count
    const int eventRetryMs = 500;
} // namespace BenchmarkWorkload

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
