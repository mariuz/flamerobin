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

#ifndef FR_BENCHMARKRUNNER_H
#define FR_BENCHMARKRUNNER_H

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "engine/Benchmark.h"
#include "engine/db/DatabaseBackend.h"

namespace fr
{

///
/// Where and how the benchmark connects. The benchmark creates its own
/// temporary database in serverDirectory and drops it at the end; it never
/// writes to any other database.
///
struct BenchmarkConnectionParams
{
    std::string connectionPrefix;   // "host/port:", empty for embedded
    std::string host;               // empty for embedded
    wxString serverDirectory;       // server-side directory for the database
    std::string username;
    std::string password;
    std::string role;
    std::string charset;
    std::string clientLibrary;
    std::string cryptKeyData;
    int pageSize = 8192;            // of the temporary database
    // use the embedded engine even if a local server is running
    bool forceEmbedded = false;
    // repeat the storage and cache tests for every supported page size
    bool comparePageSizes = false;
    // additional OLTP levels with 50 and 250 connections
    bool manyUsers = false;
    wxString targetDescription;     // shown in the report
    // optional: registered database whose header is read (read-only)
    std::string inspectConnectionString;
};

/// the installation directory of a Firebird client library, where the
/// embedded engine finds its plugins and firebird.conf; empty if unknown
wxString getBenchmarkFirebirdRoot(const std::string& clientLibrary);

///
/// Runs the benchmark. run() is meant to be called once, on a worker thread;
/// errors and cancellation are reported in the returned report. The
/// progress callback is called on the worker thread.
///
class BenchmarkRunner
{
public:
    using ProgressCallback = std::function<void(const BenchmarkProgress& progress)>;

    BenchmarkRunner(const BenchmarkConnectionParams& params,
        BenchmarkIntensity intensity);

    BenchmarkReport run(const ProgressCallback& progress);

    /// thread safe; stops the running server operation immediately, the
    /// run then cleans up
    void requestCancel();
    bool isCancelRequested() const;

    /// the temporary database is known before run() creates it, so that
    /// the caller can remember it in case FlameRobin is terminated
    std::string getConnectionString() const;
    /// all temporary databases the run may create (main and page sizes)
    std::vector<std::string> getAllConnectionStrings() const;

    /// number of steps of a complete run (progress display)
    int getStepCount() const;
    static std::string getEmbeddedConfig();

    /// drops a temporary database left behind by an earlier run; refuses
    /// (without connecting) anything that is not named like a benchmark
    /// database, and checks the attached file before dropping
    static void dropLeftoverDatabase(const BenchmarkConnectionParams& params,
        const std::string& connectionString);

private:
    BenchmarkConnectionParams paramsM;
    BenchmarkIntensity intensityM;
    BenchmarkSettings settingsM;
    std::atomic<bool> cancelM{ false };
    ProgressCallback progressM;
    BenchmarkMetrics* metricsM = nullptr;
    int stepM = 0;
    wxString uniqueNameM;
    wxString databasePathM;
    std::string connectionStringM;
    bool databaseCreatedM = false;
    // connections whose running operation is cancelled by requestCancel()
    std::mutex activeMutexM;
    std::vector<IDatabasePtr> activeM;
    // first and last ID inserted by measureParallelDrive(), deleted later
    std::vector<std::pair<int, int>> parallelDriveRangesM;

    static IDatabasePtr makeDatabase(const BenchmarkConnectionParams& params,
        const std::string& connectionString);
    IDatabasePtr openConnection(const std::string& connectionString,
        bool cancellable = true);
    IDatabasePtr openConnection(bool cancellable = true);
    // page buffers in the header of a temporary database (0 = default)
    void writeHeaderPageBuffers(const std::string& connectionString, int buffers);
    void track(const IDatabasePtr& db);
    void untrack(const IDatabasePtr& db);
    // untracks, disconnects (ignoring errors) and resets the connection
    void close(IDatabasePtr& db);
    std::vector<int> getOltpLevels() const;
    // TCP port of the connection string, 3050 if it has none
    int getServerPort() const;
    void step(const wxString& message);
    void publish(const wxString& message, bool cleaningUp = false);
    void checkCancel() const;
    void runOptional(const wxString& name, BenchmarkMetrics& m,
        const std::function<void()>& test);
    wxString getPageSizeName(int pageSize) const;
    std::string getPageSizeConnectionString(int pageSize) const;

    IDatabasePtr createDatabase(BenchmarkMetrics& m);
    bool dropDatabase(IDatabasePtr& db);
    void readEnvironment(const IDatabasePtr& db, BenchmarkMetrics& m);
    void inspectRegisteredDatabase(BenchmarkMetrics& m);
    // hardware and software of this machine
    void readSystem(BenchmarkMetrics& m);
    // client runs: network adapter, name resolution, reachable ports
    void checkNetwork(BenchmarkMetrics& m);
    // POST_EVENT and its notification (RemoteAuxPort)
    void measureEvents(BenchmarkMetrics& m);
    void measureConnect(BenchmarkMetrics& m);
    void measureLocalFiles(BenchmarkMetrics& m);
    void measureClientBursts(BenchmarkMetrics& m);
    void measureLatency(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureCpu(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureParallelCpu(BenchmarkMetrics& m);
    void measureServerBursts(const IDatabasePtr& db, BenchmarkMetrics& m);
    // average time of a single-row transaction minus its two round trips
    double measureCommitLatency(const IDatabasePtr& db, int count,
        double roundTripMs);
    void fillTable(const IDatabasePtr& db, int firstId, int rows, int payloadLength);
    // fills like fillTable() within settings.driveSeconds; returns the time
    // extrapolated to diskRows rows
    double timedFill(const IDatabasePtr& db, int firstId, int* inserted);
    // runs sql with the parameters (first ID, last ID) in chunks over the
    // IDs firstId .. lastId
    double executeChunked(const IDatabasePtr& db, const char* sql, int firstId,
        int lastId);
    // like executeChunked() in small chunks within settings.driveSeconds;
    // returns the time, processed the number of rows
    double timedChunked(const IDatabasePtr& db, const char* sql, int firstId,
        int lastId, int* processed);
    // insert, drive test, update, delete, garbage collection; db is
    // replaced by a new connection
    void measureDisk(IDatabasePtr& db, BenchmarkMetrics& m);
    // the drive test with a minimal page cache: insert, scan, lookups,
    // update, delete, and inserts on parallel connections
    void measureSmallCache(IDatabasePtr& db, BenchmarkMetrics& m, int firstId);
    void measureDriveLookups(const IDatabasePtr& db, BenchmarkMetrics& m,
        int firstId, int rows);
    void measureParallelDrive(BenchmarkMetrics& m);
    void measureTempSort(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureWideInserts(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureServerRead(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureThroughput(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureRowByRow(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measurePrepare(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureBlob(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureVersions(const IDatabasePtr& db, BenchmarkMetrics& m);
    void measureOltp(BenchmarkMetrics& m);
    BenchmarkOltpResult runOltpLevel(int clients, int firstInsertId);
    void comparePageSizes(BenchmarkReport& report);
    // db is the new, empty database; it is closed afterwards
    BenchmarkPageSizeResult measurePageSize(IDatabasePtr& db,
        const std::string& connectionString, int pageSize, double roundTripMs);
    void dropPageSizeDatabase(BenchmarkReport& report,
        const std::string& connectionString, const wxString& name);
    void addEngineStatistics(const IDatabasePtr& db, BenchmarkMetrics& m);
};

} // namespace fr

#endif // FR_BENCHMARKRUNNER_H
