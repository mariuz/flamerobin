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

#include <wx/datetime.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/platinfo.h>
#include <wx/power.h>
#include <wx/thread.h>
#include <wx/utils.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <numeric>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

#include "core/StringUtils.h"
#include "engine/BenchmarkRunner.h"
#include "engine/db/DatabaseFactory.h"
#include "engine/db/IBlob.h"
#include "engine/db/IDatabase.h"
#include "engine/db/IStatement.h"
#include "engine/db/ITransaction.h"
#include "engine/db/fbcpp/FbCppDatabase.h"

#include <fb-cpp/EventListener.h>

namespace fr
{

namespace
{

using Clock = std::chrono::steady_clock;

// number of step() calls in a run without the OLTP levels and the page
// size comparison, used for the progress display
const int fixedStepCount = 33;
// page sizes of the optional comparison, one step each
const int comparedPageSizes[] = { 4096, 8192, 16384, 32768 };
const int commitsPerPageSize = 50;
// page cache of the storage test that keeps the I/O subsystem busy
const int smallCachePages = 50;
const int idlePauseMs = 500;
const int sparsePauseMs = 200;
const int serverBurstIterations = 50000;
const int clientBurstIterations = 3000000;
const int localFileSize = 4096;
const int largeRoundTrips = 50;
const int blobSize = 64 * 1024;
const int blobChunk = 16 * 1024;
const int versionReads = 20;
// OLTP levels up to settings.oltpMaxClients, and the many users option
const int oltpClientCounts[] = { 1, 2, 4, 8, 16, 32 };
const int manyUserClientCounts[] = { 50, 250 };
const int oltpPointSelects = 5;
// an OLTP client that fails this often in a row has a real error, not
// just update conflicts with the other clients
const int oltpMaxFailuresInRow = 20;
// ID ranges that do not overlap the generated rows (1 .. scanRows)
const int parallelDriveIdBase = 5000000;
const int parallelDriveIdsPerConnection = 1000000;
const int commitIdBase = 10000000;
const int oltpIdBase = 20000000;
// more than a client inserts in oltpSeconds; all levels together stay far
// below the INTEGER limit
const int oltpIdsPerClient = 200000;
// rows per server call; keeps every call short enough for a quick cancel
const int chunkRows = 5000;
// rows per server call of the time limited drive test
const int driveChunkRows = 500;
const int driveLookupChunk = 2000;
// ports of Firebird servers checked from a client, and how long to wait
// for an answer (Windows tries a refused connection again for a second)
const int checkedPortFirst = 3050;
const int checkedPortLast = 3060;
const int portCheckTimeoutMs = 3000;
// the event test posts again after eventRetryMs and gives up after
// eventTimeoutMs; the first notification of a new listener carries no count
const int eventRetryMs = 500;
const int eventTimeoutMs = 5000;
// a round trip this long (and ten times the median) is a retransmission
const double latencyOutlierMs = 150.0;

class BenchmarkCancelled : public std::exception
{
public:
    const char* what() const noexcept override
    {
        return "cancelled";
    }
};

double elapsedMs(Clock::time_point start)
{
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

// the inverse of wx2std()
wxString toWxString(const std::string& s)
{
    return wxString(s.c_str(), *wxConvCurrent);
}

wxString fromUtf8(const std::string& s)
{
    return wxString::FromUTF8(s.c_str()).Trim();
}

std::string toUtf8(const wxString& s)
{
    return std::string(s.utf8_str());
}

ITransactionPtr startTransaction(const IDatabasePtr& db, bool readOnly)
{
    ITransactionPtr tr = db->createTransaction();
    tr->setAccessMode(readOnly ? TransactionAccessMode::Read
        : TransactionAccessMode::Write);
    tr->setIsolationLevel(readOnly ? TransactionIsolationLevel::ReadCommitted
        : TransactionIsolationLevel::Concurrency);
    tr->start();
    return tr;
}

// executes one statement in its own transaction, returns the duration
// including the commit
double executeTimed(const IDatabasePtr& db, const std::string& sql)
{
    auto start = Clock::now();
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(sql);
        st->execute();
        if (st->getColumnCount() > 0)
        {
            while (st->fetch())
                ;
        }
    }
    tr->commit();
    return elapsedMs(start);
}

// full scan of the test table; returns the duration, bytes receives the
// payload size
double timedScan(const IDatabasePtr& db, int64_t* bytes = nullptr)
{
    auto start = Clock::now();
    ITransactionPtr tr = startTransaction(db, true);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::scan);
        st->execute();
        if (st->fetch() && bytes && !st->isNull(1))
            *bytes = int64_t(st->getDouble(1));
    }
    tr->commit();
    return elapsedMs(start);
}

// Runs work(0) .. work(count - 1) on threads of their own and waits for all
// of them, also when a thread cannot be started. work must not throw.
void runParallel(int count, const std::function<void(int)>& work)
{
    std::vector<std::thread> threads;
    threads.reserve(count);
    auto joinAll = [&threads]()
    {
        for (auto& t : threads)
            t.join();
    };
    try
    {
        for (int i = 0; i < count; ++i)
        {
            threads.emplace_back([&work, i]()
            {
                // the DAL logs every statement at debug level; logging is
                // enabled per thread, and these threads must not fill the
                // log buffers while they are measured
                wxLogNull noLog;
                work(i);
            });
        }
    }
    catch (...)
    {
        joinAll();
        throw;
    }
    joinAll();
}

// the MON$DATABASE_NAME of the attached database; empty if unknown
wxString readAttachedFileName(const IDatabasePtr& db)
{
    wxString fileName;
    ITransactionPtr tr = startTransaction(db, true);
    try
    {
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::databaseInfo);
        st->execute();
        if (st->fetch())
            fileName = fromUtf8(st->getString(0));
    }
    catch (const std::exception&)
    {
        // an unknown name never passes the safety check
    }
    tr->commit();
    return fileName;
}

BenchmarkDatabaseInfo readDatabaseInfo(const IDatabasePtr& db)
{
    DatabaseInfoData info{};
    db->getInfo(&info);
    BenchmarkDatabaseInfo r;
    r.odsMajor = info.ods;
    r.odsMinor = info.odsMinor;
    r.pageSize = info.pageSize;
    r.pages = info.pages;
    r.pageBuffers = info.buffers;
    r.forcedWrites = info.forcedWrites;
    r.sweepInterval = info.sweep;
    r.oldestActiveTransaction = info.oldestActiveTransaction;
    r.nextTransaction = info.nextTransaction;
    r.cryptState = info.cryptState;

    ITransactionPtr tr = startTransaction(db, true);
    // MON$ and RDB$LINGER are optional information
    try
    {
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::databaseInfo);
        st->execute();
        if (st->fetch())
        {
            r.fileName = fromUtf8(st->getString(0));
            if (!st->isNull(1))
                r.backupState = st->getInt32(1);
            if (!st->isNull(2))
                r.cachePages = st->getInt32(2);
        }
    }
    catch (const std::exception&)
    {
    }
    try
    {
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::databaseLinger);
        st->execute();
        if (st->fetch() && !st->isNull(0))
            r.lingerSeconds = st->getInt32(0);
    }
    catch (const std::exception&)
    {
    }
    tr->commit();
    return r;
}

// drops the attached database, but only if it is the benchmark database
// with exactly this unique name
void verifyAndDrop(const IDatabasePtr& db, const wxString& uniqueName)
{
    wxString fileName = readAttachedFileName(db);
    if (!isBenchmarkDatabaseFile(fileName, uniqueName))
    {
        try { db->disconnect(); } catch (...) {}
        throw std::runtime_error(toUtf8(wxString::Format(_("Safety check "
            "failed: the connection points to \"%s\" instead of the "
            "benchmark database %s. Nothing was dropped."),
            fileName, uniqueName)));
    }
    db->drop();
}

// the work done by one local CPU burst; the result is used so that the
// compiler cannot remove the loop
double localCpuBurst()
{
    double x = 0.0;
    for (int i = 1; i <= clientBurstIterations; ++i)
    {
        x += std::sqrt(double(i % 1000 + 1));
        x += (i % 4 == 0) ? double(i % 7) : -1.0;
    }
    return x;
}

wxString getLocalOsFamily()
{
    int os = wxPlatformInfo::Get().GetOperatingSystemId();
    if (os & wxOS_WINDOWS)
        return "Windows";
    if (os & wxOS_MAC)
        return "macOS";
    if (os & wxOS_UNIX_LINUX)
        return "Linux";
    if (os & wxOS_UNIX)
        return "Unix";
    return wxEmptyString;
}

wxString makeUniqueName()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned> dist(0, 0xFFFFFF);
    return "FR_BENCHMARK_" + wxDateTime::Now().Format("%Y%m%d_%H%M%S")
        + wxString::Format("_%06X.FDB", dist(gen));
}

} // namespace

BenchmarkRunner::BenchmarkRunner(const BenchmarkConnectionParams& params,
    BenchmarkIntensity intensity)
    : paramsM(params), intensityM(intensity),
      settingsM(BenchmarkSettings::forIntensity(intensity)),
      uniqueNameM(makeUniqueName())
{
    databasePathM = makeBenchmarkDatabasePath(paramsM.serverDirectory,
        uniqueNameM);
    connectionStringM = paramsM.connectionPrefix + wx2std(databasePathM);
}

int BenchmarkRunner::getStepCount() const
{
    return fixedStepCount + int(getOltpLevels().size()) + (paramsM.comparePageSizes
        ? int(std::size(comparedPageSizes)) : 0);
}

std::vector<int> BenchmarkRunner::getOltpLevels() const
{
    std::vector<int> levels;
    for (int clients : oltpClientCounts)
    {
        if (clients <= settingsM.oltpMaxClients)
            levels.push_back(clients);
    }
    if (paramsM.manyUsers)
        levels.insert(levels.end(), std::begin(manyUserClientCounts),
            std::end(manyUserClientCounts));
    return levels;
}

int BenchmarkRunner::getServerPort() const
{
    // "host/port:" or "host:"; without a port the client uses 3050
    std::string prefix = paramsM.connectionPrefix;
    if (!prefix.empty() && prefix.back() == ':')
        prefix.pop_back();
    size_t slash = prefix.rfind('/');
    if (slash == std::string::npos)
        return 3050;
    int port = std::atoi(prefix.c_str() + slash + 1);
    return port > 0 ? port : 3050;
}

std::string BenchmarkRunner::getEmbeddedConfig()
{
    // the engine plugin of Firebird 6, 4/5 and 3; missing ones are skipped
    return "Providers = Engine14, Engine13, Engine12";
}

std::string BenchmarkRunner::getConnectionString() const
{
    return connectionStringM;
}

wxString BenchmarkRunner::getPageSizeName(int pageSize) const
{
    // FR_BENCHMARK_..._ABCDEF.FDB -> FR_BENCHMARK_..._ABCDEF_P8192.FDB
    return uniqueNameM.BeforeLast('.') + wxString::Format("_P%d.FDB", pageSize);
}

std::string BenchmarkRunner::getPageSizeConnectionString(int pageSize) const
{
    return paramsM.connectionPrefix + wx2std(makeBenchmarkDatabasePath(
        paramsM.serverDirectory, getPageSizeName(pageSize)));
}

std::vector<std::string> BenchmarkRunner::getAllConnectionStrings() const
{
    std::vector<std::string> all = { connectionStringM };
    if (paramsM.comparePageSizes)
    {
        for (int size : comparedPageSizes)
            all.push_back(getPageSizeConnectionString(size));
    }
    return all;
}

void BenchmarkRunner::requestCancel()
{
    cancelM = true;
    std::lock_guard<std::mutex> lock(activeMutexM);
    for (auto& db : activeM)
        db->cancelOperation();
}

bool BenchmarkRunner::isCancelRequested() const
{
    return cancelM;
}

void BenchmarkRunner::track(const IDatabasePtr& db)
{
    std::lock_guard<std::mutex> lock(activeMutexM);
    activeM.push_back(db);
    if (cancelM)
        db->cancelOperation();
}

void BenchmarkRunner::untrack(const IDatabasePtr& db)
{
    // must be called before the connection is closed, so that
    // requestCancel() never uses a connection that is being closed
    std::lock_guard<std::mutex> lock(activeMutexM);
    activeM.erase(std::remove(activeM.begin(), activeM.end(), db), activeM.end());
}

void BenchmarkRunner::close(IDatabasePtr& db)
{
    if (!db)
        return;
    untrack(db);
    try { db->disconnect(); } catch (...) {}
    db.reset();
}

void BenchmarkRunner::checkCancel() const
{
    if (cancelM)
        throw BenchmarkCancelled();
}

void BenchmarkRunner::runOptional(const wxString& name, BenchmarkMetrics& m,
    const std::function<void()>& test)
{
    // a failing optional test (e.g. a feature the client library does not
    // support) is reported but does not end the benchmark
    try
    {
        test();
    }
    catch (const BenchmarkCancelled&)
    {
        throw;
    }
    catch (const std::exception& e)
    {
        if (cancelM)
            throw;
        m.skippedTests.push_back({ name, wxString::FromUTF8(e.what()) });
    }
}

void BenchmarkRunner::publish(const wxString& message, bool cleaningUp)
{
    if (!progressM)
        return;
    BenchmarkProgress p;
    p.stepCount = getStepCount();
    p.step = std::min(stepM, p.stepCount);
    p.message = message;
    p.cleaningUp = cleaningUp;
    if (metricsM)
        p.results = buildBenchmarkResults(*metricsM);
    progressM(p);
}

void BenchmarkRunner::step(const wxString& message)
{
    checkCancel();
    ++stepM;
    publish(message);
}

IDatabasePtr BenchmarkRunner::makeDatabase(const BenchmarkConnectionParams& params,
    const std::string& connectionString)
{
    IDatabasePtr db = DatabaseFactory::createDatabase();
    db->setConnectionString(connectionString);
    db->setCredentials(params.username, params.password);
    db->setRole(params.role);
    db->setCharset(params.charset);
    db->setClientLibrary(params.clientLibrary);
    db->setCryptKeyData(params.cryptKeyData);
    if (params.forceEmbedded)
        db->setConfig(getEmbeddedConfig());
    return db;
}

IDatabasePtr BenchmarkRunner::openConnection(const std::string& connectionString,
    bool cancellable)
{
    IDatabasePtr db = makeDatabase(paramsM, connectionString);
    db->connect();
    if (cancellable)
        track(db);
    return db;
}

IDatabasePtr BenchmarkRunner::openConnection(bool cancellable)
{
    return openConnection(connectionStringM, cancellable);
}

void BenchmarkRunner::writeHeaderPageBuffers(const std::string& connectionString,
    int buffers)
{
    // a short connection that only changes the header of a temporary
    // database of this run (connectionString is always one of them); the
    // value applies when the database is opened the next time
    IDatabasePtr db = makeDatabase(paramsM, connectionString);
    db->setHeaderPageBuffers(buffers);
    db->connect();
    db->disconnect();
}

namespace
{

// The engine inside FlameRobin finds its plugins and its firebird.conf in
// the root directory of the client library it was loaded with. A client
// library does not know that directory itself unless it is the one of the
// running program, so the environment variable FIREBIRD names it while the
// library starts; the old value comes back afterwards.
class FirebirdRootGuard
{
public:
    explicit FirebirdRootGuard(const BenchmarkConnectionParams& params)
    {
        const wxString root = getBenchmarkFirebirdRoot(params.clientLibrary);
        if (!params.forceEmbedded || root.empty())
            return;
        hadValueM = wxGetEnv("FIREBIRD", &oldValueM);
        activeM = wxSetEnv("FIREBIRD", root);
    }
    ~FirebirdRootGuard()
    {
        if (!activeM)
            return;
        if (hadValueM)
            wxSetEnv("FIREBIRD", oldValueM);
        else
            wxUnsetEnv("FIREBIRD");
    }
    FirebirdRootGuard(const FirebirdRootGuard&) = delete;
    FirebirdRootGuard& operator=(const FirebirdRootGuard&) = delete;

private:
    bool activeM = false;
    bool hadValueM = false;
    wxString oldValueM;
};

} // namespace

wxString getBenchmarkFirebirdRoot(const std::string& clientLibrary)
{
    // Windows: <root>\fbclient.dll, Linux: <root>/lib/libfbclient.so,
    // macOS: <root>/Libraries/libfbclient.dylib
    wxString directory = toWxString(clientLibrary).BeforeLast('/');
    if (directory.length() < toWxString(clientLibrary).BeforeLast('\\').length())
        directory = toWxString(clientLibrary).BeforeLast('\\');
    const wxString last = directory.AfterLast('/').AfterLast('\\');
    if (last == "lib" || last == "Libraries")
        directory = directory.Left(directory.length() - last.length() - 1);
    return directory;
}

void BenchmarkRunner::dropLeftoverDatabase(const BenchmarkConnectionParams& params,
    const std::string& connectionString)
{
    FirebirdRootGuard root(params);
    // refuse before connecting: no other database is even opened
    wxString name = toWxString(connectionString).AfterLast('/')
        .AfterLast('\\').AfterLast(':');
    if (!isBenchmarkDatabaseName(name))
    {
        throw std::runtime_error(toUtf8(wxString::Format(_("%s is not a "
            "temporary benchmark database. Nothing was dropped."), name)));
    }
    IDatabasePtr db = makeDatabase(params, connectionString);
    db->connect();
    verifyAndDrop(db, name);
}

BenchmarkReport BenchmarkRunner::run(const ProgressCallback& progress)
{
    // file tests must not pop up error dialogs from the worker thread, and
    // the debug log of the DAL must not be filled while measuring
    wxLogNull noLog;
    FirebirdRootGuard root(paramsM);
    auto runStart = Clock::now();

    progressM = progress;
    stepM = 0;
    databaseCreatedM = false;

    BenchmarkReport report;
    report.target = paramsM.targetDescription;
    report.benchmarkDatabasePath = databasePathM;
    report.intensityName = getBenchmarkIntensityName(intensityM);
    report.startedAt = wxDateTime::Now().FormatISOCombined(' ');
    BenchmarkMetrics& m = report.metrics;
    metricsM = &m;
    m.settings = settingsM;
    m.connectionHost = wxString::FromUTF8(paramsM.host.c_str());
    m.localHostName = wxGetHostName();
    m.clientPlatform = wxGetOsDescription();
    m.clientOsFamily = getLocalOsFamily();
    m.localCpuCount = wxThread::GetCPUCount();
    wxMemorySize freeMemory = wxGetFreeMemory();
    if (freeMemory > 0)
        m.localFreeMemoryBytes = freeMemory.GetValue();
    wxPowerType power = wxGetPowerType();
    if (power != wxPOWER_UNKNOWN)
        m.localOnBattery = power == wxPOWER_BATTERY;
    m.tempDirectory = wxFileName::GetTempDir();
    m.benchmarkDirectory = paramsM.serverDirectory;
    m.requestedPageSize = paramsM.pageSize;

    IDatabasePtr db;
    try
    {
        step(_("Creating the temporary benchmark database..."));
        db = createDatabase(m);

        step(_("Reading server properties..."));
        readEnvironment(db, m);

        step(paramsM.inspectConnectionString.empty()
            ? _("Skipping the selected database (not requested)...")
            : _("Reading the configuration of the selected database (read-only)..."));
        inspectRegisteredDatabase(m);

        step(_("Reading hardware and software of this machine..."));
        readSystem(m);

        step(m.connectionKind == BenchmarkConnectionKind::TcpRemote
            ? _("Checking the network path and the ports of the server...")
            : _("No network between FlameRobin and the server, skipping the port check..."));
        checkNetwork(m);

        step(_("Measuring connection setup..."));
        measureConnect(m);

        step(_("Testing file operations on this machine..."));
        measureLocalFiles(m);

        step(_("Testing power management of this machine..."));
        measureClientBursts(m);

        step(_("Creating the test tables and procedures..."));
        for (const char* sql : BenchmarkSql::getCreateStatements())
            executeTimed(db, sql);

        step(_("Testing event delivery (RemoteAuxPort)..."));
        measureEvents(m);

        step(_("Measuring round trips..."));
        measureLatency(db, m);

        step(_("Running the CPU test..."));
        measureCpu(db, m);

        step(_("Running the parallel CPU test..."));
        measureParallelCpu(m);

        step(_("Testing power management of the server..."));
        measureServerBursts(db, m);

        step(_("Measuring commit latency..."));
        m.commitMs = measureCommitLatency(db, settingsM.commitCount,
            m.latencyMedianMs.value_or(0.0));

        measureDisk(db, m);  // six steps

        step(_("Preparing data for the cache and network tests..."));
        fillTable(db, 1, settingsM.scanRows, settingsM.scanPayloadLength);
        addEngineStatistics(db, m);

        // a fresh connection for the first, "cold" scan
        step(_("Reconnecting..."));
        close(db);
        db = openConnection();

        step(_("Cache test: first scan..."));
        m.coldScanMs = timedScan(db, &m.scanBytes);
        step(_("Cache test: repeated scan..."));
        m.warmScanMs = timedScan(db);

        step(_("Reading and sorting on the server..."));
        measureServerRead(db, m);

        step(wxString::Format(_("Sorting %d MB, more than the sort memory "
            "(temporary files)..."), settingsM.tempSortMegabytes));
        runOptional(_("Sorting large data (temporary files)"), m,
            [&]() { measureTempSort(db, m); });

        step(_("Streaming a large result set..."));
        measureThroughput(db, m);

        step(wxString::Format(_("Inserting %d rows with %d columns, one round "
            "trip each..."), settingsM.wideRows, BenchmarkSql::wideColumns));
        measureWideInserts(db, m);

        step(_("Comparing row-by-row inserts with a server-side batch..."));
        runOptional(_("Rows one by one compared with in one go"), m,
            [&]() { measureRowByRow(db, m); });

        step(_("Measuring statement preparation..."));
        runOptional(_("Preparing a typical query"), m,
            [&]() { measurePrepare(db, m); });

        step(_("Writing and reading BLOBs..."));
        runOptional(_("Writing / reading large data (BLOBs)"), m, [&]() { measureBlob(db, m); });

        step(_("Measuring record versions and long running transactions..."));
        runOptional(_("Reading while an old transaction is open"), m,
            [&]() { measureVersions(db, m); });

        addEngineStatistics(db, m);
        measureOltp(m);  // one step per level

        if (paramsM.comparePageSizes)
            comparePageSizes(report);  // one step per page size
    }
    catch (const BenchmarkCancelled&)
    {
        report.cancelled = true;
    }
    catch (const std::exception& e)
    {
        // a cancelled server operation ends with an error
        if (cancelM)
            report.cancelled = true;
        else
            report.errorMessage = wxString::FromUTF8(e.what());
    }

    // never skipped, not even after a cancel request
    report.benchmarkDatabaseCreated = databaseCreatedM;
    try
    {
        publish(_("Dropping the temporary benchmark database..."), true);
    }
    catch (...)
    {
        // the progress display must not prevent the cleanup
    }
    try
    {
        report.benchmarkDatabaseDropped = dropDatabase(db);
    }
    catch (const std::exception& e)
    {
        report.errorMessage += (report.errorMessage.empty() ? "" : "\n")
            + wxString::Format(_("The temporary benchmark database %s could "
                "not be dropped: %s"), databasePathM, wxString::FromUTF8(e.what()));
    }

    report.durationSeconds = elapsedMs(runStart) / 1000.0;
    metricsM = nullptr;
    report.evaluate();
    return report;
}

IDatabasePtr BenchmarkRunner::createDatabase(BenchmarkMetrics& m)
{
    // Firebird never overwrites an existing file here, and the name
    // contains a random part, so no existing database can be hit
    IDatabasePtr db = makeDatabase(paramsM, connectionStringM);
    try
    {
        auto start = Clock::now();
        try
        {
            db->create(paramsM.pageSize, 3);
        }
        catch (const std::exception&)
        {
            // Firebird 3 has no 32 KB pages; nothing was created, so the
            // same unique name can be used again
            if (paramsM.pageSize <= 16384)
                throw;
            db = makeDatabase(paramsM, connectionStringM);
            start = Clock::now();
            db->create(16384, 3);
        }
        m.createDatabaseMs = elapsedMs(start);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(toUtf8(wxString::Format(_("Could not create "
            "the temporary benchmark database\n%s\n\nPlease check that the "
            "directory exists on the server, that the Firebird server may "
            "create files there (DatabaseAccess in firebird.conf) and that "
            "the user may create databases (CREATE DATABASE privilege or "
            "administrator).\n\nServer message: %s"),
            toWxString(connectionStringM), wxString::FromUTF8(e.what()))));
    }
    databaseCreatedM = true;
    track(db);
    return db;
}

bool BenchmarkRunner::dropDatabase(IDatabasePtr& db)
{
    close(db);
    if (!databaseCreatedM)
        return false;

    // any connection still open (e.g. after an error inside a test) would
    // make the drop fail
    std::vector<IDatabasePtr> remaining;
    {   // scope
        std::lock_guard<std::mutex> lock(activeMutexM);
        remaining.swap(activeM);
    }
    for (auto& other : remaining)
    {
        try { other->disconnect(); } catch (...) {}
    }
    // a fresh connection without open transactions; not cancellable, the
    // cleanup must not be interrupted
    IDatabasePtr target = openConnection(false);
    verifyAndDrop(target, uniqueNameM);
    databaseCreatedM = false;
    return true;
}

void BenchmarkRunner::readEnvironment(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    m.benchmarkDatabase = readDatabaseInfo(db);
    m.versionList = parseBenchmarkVersionList(db->getEngineVersion());
    m.serverImplementation = m.versionList.empty() ? wxString() : m.versionList.front();
    m.serverPlatform = getServerPlatformName(m.serverImplementation);
    m.serverHostName = getBenchmarkServerHostName(m.versionList);

    ITransactionPtr tr = startTransaction(db, true);
    auto query = [&](const char* sql,
        const std::function<void(const IStatementPtr&)>& row)
    {
        // optional information: older servers or missing rights must not
        // stop the benchmark
        try
        {
            IStatementPtr st = db->createStatement(tr);
            st->prepare(sql);
            st->execute();
            while (st->fetch())
                row(st);
        }
        catch (const std::exception&)
        {
        }
    };

    query(BenchmarkSql::engineVersion, [&m](const IStatementPtr& st)
    {
        m.serverVersion = fromUtf8(st->getString(0));
        long major = 0;
        if (m.serverVersion.BeforeFirst('.').ToLong(&major))
            m.serverMajorVersion = int(major);
    });
    query(BenchmarkSql::attachmentInfo, [&m](const IStatementPtr& st)
    {
        if (!st->isNull(0))
            m.remoteProtocol = fromUtf8(st->getString(0));
        if (!st->isNull(1))
            m.remoteAddress = fromUtf8(st->getString(1));
        if (!st->isNull(2))
            m.clientVersion = fromUtf8(st->getString(2));
    });
    auto text = [](const IStatementPtr& st, int column)
    {
        return st->isNull(column) ? wxString() : fromUtf8(st->getString(column));
    };
    query(BenchmarkSql::sessionUser, [&m, &text](const IStatementPtr& st)
    {
        m.firebirdUser = text(st, 0);
        m.firebirdRole = text(st, 1);
    });
    if (m.serverMajorVersion >= 3)
    {
        query(BenchmarkSql::attachmentWireInfo, [&m](const IStatementPtr& st)
        {
            m.wireCompressed = st->getInt32(0) != 0;
            m.wireEncrypted = st->getInt32(1) != 0;
        });
        query(BenchmarkSql::attachmentDetails, [&m, &text](const IStatementPtr& st)
        {
            m.authMethod = text(st, 0);
            m.protocolVersion = text(st, 1);
            m.clientHostSeenByServer = text(st, 2);
        });
        query(BenchmarkSql::securityDatabase, [&m, &text](const IStatementPtr& st)
        {
            m.securityDatabase = text(st, 0);
        });
    }
    if (m.serverMajorVersion >= 4)
    {
        query(BenchmarkSql::attachmentTimeouts, [&m, &text](const IStatementPtr& st)
        {
            m.wireCryptPlugin = text(st, 0);
            if (!st->isNull(1))
                m.idleTimeoutSeconds = st->getInt32(1);
            if (!st->isNull(2))
                m.statementTimeoutMs = st->getInt32(2);
        });
        query(BenchmarkSql::serverConfig, [&m](const IStatementPtr& st)
        {
            wxString name = fromUtf8(st->getString(0));
            m.serverConfig[name] =
                st->isNull(1) ? wxString("<null>") : fromUtf8(st->getString(1));
            if (st->getInt32(2) != 0)
                m.serverConfigExplicit.insert(name);
        });
    }
    tr->commit();

    m.connectionKind = classifyBenchmarkConnection(m.remoteProtocol,
        m.remoteAddress);
    m.databaseFileIsLocal = !m.benchmarkDatabase.fileName.empty()
        && wxFileName::FileExists(m.benchmarkDatabase.fileName);
    m.runLocation = determineBenchmarkRunLocation(m.connectionKind,
        m.connectionHost, m.localHostName, m.databaseFileIsLocal);
}

void BenchmarkRunner::inspectRegisteredDatabase(BenchmarkMetrics& m)
{
    if (paramsM.inspectConnectionString.empty())
        return;
    // read-only: header information, MON$DATABASE and RDB$LINGER
    try
    {
        // the registered connection as it is, without the benchmark target
        BenchmarkConnectionParams params = paramsM;
        params.forceEmbedded = false;
        IDatabasePtr db = makeDatabase(params, paramsM.inspectConnectionString);
        db->connect();
        m.inspectedDatabase = readDatabaseInfo(db);
        db->disconnect();
    }
    catch (const std::exception&)
    {
        // optional; the report simply lacks this part
    }
}

void BenchmarkRunner::readSystem(BenchmarkMetrics& m)
{
    // the drive is only described when the database is on this machine
    std::string storage;
    if (m.databaseFileIsLocal)
        storage = toUtf8(wxFileName(m.benchmarkDatabase.fileName).GetPath());
    m.system = collectBenchmarkSystemInfo(storage);
}

void BenchmarkRunner::checkNetwork(BenchmarkMetrics& m)
{
    m.serverPort = getServerPort();
    if (m.connectionKind != BenchmarkConnectionKind::TcpRemote || paramsM.host.empty())
        return;
    m.networkPath = probeBenchmarkNetworkPath(paramsM.host);
    // the usual ports of Firebird servers, the port of the connection and
    // a fixed RemoteAuxPort; the connections are closed at once
    std::vector<int> ports;
    for (int port = checkedPortFirst; port <= checkedPortLast; ++port)
        ports.push_back(port);
    auto addPort = [&ports](long port)
    {
        if (port > 0 && port < 65536
            && std::find(ports.begin(), ports.end(), int(port)) == ports.end())
        {
            ports.push_back(int(port));
        }
    };
    addPort(m.serverPort);
    auto aux = m.serverConfig.find("RemoteAuxPort");
    long auxPort = 0;
    if (aux != m.serverConfig.end() && aux->second.ToLong(&auxPort))
        addPort(auxPort);
    m.serverPorts = checkBenchmarkPorts(paramsM.host, ports, portCheckTimeoutMs);
}

void BenchmarkRunner::measureEvents(BenchmarkMetrics& m)
{
    if (!prepareBenchmarkEventTest(m))
        return;

    IDatabasePtr db = openConnection();
    try
    {
        auto fbDb = std::dynamic_pointer_cast<FbCppDatabase>(db);
        if (!fbDb)
            throw std::runtime_error("the database layer has no events");
        std::mutex mutex;
        std::condition_variable notified;
        unsigned received = 0;
        {   // scope: the listener is stopped before the connection closes
            fbcpp::EventListener listener(fbDb->getAttachment(),
                { BenchmarkSql::eventName },
                [&](const std::vector<fbcpp::EventCount>& counts)
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    for (const auto& c : counts)
                        received += c.count;
                    notified.notify_all();
                });
            // the first notification of a listener only carries the start
            // counts and is not reported; an event posted before it arrives
            // is lost, so the event is posted again until one arrives
            const auto deadline = Clock::now() + std::chrono::milliseconds(eventTimeoutMs);
            while (!m.eventMs && Clock::now() < deadline)
            {
                checkCancel();
                auto posted = Clock::now();
                executeTimed(db, BenchmarkSql::postEvent);
                std::unique_lock<std::mutex> lock(mutex);
                if (notified.wait_for(lock, std::chrono::milliseconds(eventRetryMs),
                    [&received]() { return received > 0; }))
                {
                    m.eventMs = elapsedMs(posted);
                }
            }
        }
        m.eventResult = m.eventMs ? BenchmarkEventResult::Received
            : BenchmarkEventResult::NotReceived;
        if (!m.eventMs)
        {
            m.eventStatus = wxString::Format(_("not received within %d seconds"),
                eventTimeoutMs / 1000);
        }
    }
    catch (const BenchmarkCancelled&)
    {
        close(db);
        throw;
    }
    catch (const std::exception& e)
    {
        if (cancelM)
        {
            close(db);
            throw;
        }
        m.eventResult = BenchmarkEventResult::Failed;
        m.eventStatus = wxString::Format(_("failed: %s"), wxString::FromUTF8(e.what()));
    }
    close(db);
}

void BenchmarkRunner::measureConnect(BenchmarkMetrics& m)
{
    double total = 0.0;
    for (int i = 0; i < settingsM.connectAttempts; ++i)
    {
        checkCancel();
        auto start = Clock::now();
        IDatabasePtr db = openConnection(false);
        total += elapsedMs(start);
        db->disconnect();
    }
    m.connectMs = total / settingsM.connectAttempts;
}

void BenchmarkRunner::measureLocalFiles(BenchmarkMetrics& m)
{
    const std::vector<char> data(localFileSize, 'x');
    const wxString prefix = wxString::Format("frbench_%lu_",
        (unsigned long)wxGetProcessId());

    // returns the average time per file, nothing if the directory is not
    // writable; only files created here are deleted
    auto fileTest = [&](const wxString& dir) -> std::optional<double>
    {
        auto start = Clock::now();
        for (int i = 0; i < settingsM.localFiles; ++i)
        {
            checkCancel();
            wxFileName fn(dir, prefix + wxString::Format("%d.tmp", i));
            wxFile f;
            if (!f.Create(fn.GetFullPath(), false))
                return std::nullopt;
            f.Write(data.data(), data.size());
            f.Close();
            wxRemoveFile(fn.GetFullPath());
        }
        return elapsedMs(start) / settingsM.localFiles;
    };

    m.tempDirFileMs = fileTest(m.tempDirectory);

    // the tests in the database directory only make sense on the server
    if (!m.databaseFileIsLocal)
        return;
    wxString dir = wxFileName(m.benchmarkDatabase.fileName).GetPath();
    m.databaseDirFileMs = fileTest(dir);

    wxLongLong totalSpace, freeSpace;
    if (wxGetDiskSpace(dir, &totalSpace, &freeSpace))
        m.databaseDiskFreeBytes = freeSpace.GetValue();

    // synchronous writes of the operating system, for comparison with
    // the commit latency of Firebird
    wxFileName fn(dir, prefix + "sync.tmp");
    wxFile f;
    if (!f.Create(fn.GetFullPath(), false))
        return;
    const int writes = std::max(20, settingsM.commitCount / 4);
    auto start = Clock::now();
    bool ok = true;
    for (int i = 0; i < writes && ok; ++i)
    {
        f.Seek(0);
        ok = f.Write(data.data(), data.size()) == data.size() && f.Flush();
    }
    double duration = elapsedMs(start);
    f.Close();
    wxRemoveFile(fn.GetFullPath());
    if (ok)
        m.osSyncWriteMs = duration / writes;
}

void BenchmarkRunner::measureClientBursts(BenchmarkMetrics& m)
{
    volatile double sink = 0.0;
    std::vector<double> idle, hot;
    for (int i = 0; i < settingsM.bursts; ++i)
    {
        checkCancel();
        std::this_thread::sleep_for(std::chrono::milliseconds(idlePauseMs));
        auto start = Clock::now();
        sink = sink + localCpuBurst();
        idle.push_back(elapsedMs(start));
    }
    for (int i = 0; i < settingsM.bursts; ++i)
    {
        auto start = Clock::now();
        sink = sink + localCpuBurst();
        hot.push_back(elapsedMs(start));
    }
    m.clientBurstIdleMs = getBenchmarkPercentile(idle, 0.5);
    m.clientBurstHotMs = getBenchmarkPercentile(hot, 0.5);
}

void BenchmarkRunner::measureLatency(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    // executes a prepared statement and fetches its row, returns the time
    auto roundTrip = [](const IStatementPtr& st) -> double
    {
        auto start = Clock::now();
        st->execute();
        st->fetch();
        return elapsedMs(start);
    };

    ITransactionPtr tr = startTransaction(db, true);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::latency);

        std::vector<double> times;
        times.reserve(settingsM.latencyQueries);
        for (int i = 0; i < settingsM.latencyQueries; ++i)
        {
            if (i % 100 == 0)
                checkCancel();
            times.push_back(roundTrip(st));
        }
        m.latencyAvgMs = getBenchmarkAverage(times);
        m.latencyMedianMs = getBenchmarkPercentile(times, 0.5);
        m.latencyP95Ms = getBenchmarkPercentile(times, 0.95);
        m.latencyMaxMs = *std::max_element(times.begin(), times.end());
        // TCP sends a lost packet again only after at least 200 ms
        const double outlier = std::max(latencyOutlierMs, 10.0 * *m.latencyMedianMs);
        m.latencyOutliers = int(std::count_if(times.begin(), times.end(),
            [outlier](double t) { return t > outlier; }));
        publish(_("Measuring round trips after idle pauses..."));

        // single queries after short idle pauses, like an interactive user
        std::vector<double> sparse;
        for (int i = 0; i < settingsM.sparseQueries; ++i)
        {
            checkCancel();
            std::this_thread::sleep_for(std::chrono::milliseconds(sparsePauseMs));
            sparse.push_back(roundTrip(st));
        }
        m.latencySparseMs = getBenchmarkPercentile(sparse, 0.5);

        // larger answers only tell something about a real network
        if (m.runLocation == BenchmarkRunLocation::Client)
        {
            IStatementPtr large = db->createStatement(tr);
            large->prepare(BenchmarkSql::largeRoundTrip);
            const int count = std::max(largeRoundTrips, settingsM.latencyQueries / 10);
            std::vector<double> largeTimes;
            for (int i = 0; i < count; ++i)
            {
                if (i % 100 == 0)
                    checkCancel();
                largeTimes.push_back(roundTrip(large));
            }
            m.largeRoundTripMs = getBenchmarkPercentile(largeTimes, 0.5);
        }
    }
    tr->commit();
}

void BenchmarkRunner::measureCpu(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::cpuTest);
        st->setInt32(0, settingsM.cpuIterations);
        // only the execution is timed, exactly like in the parallel test
        auto start = Clock::now();
        st->execute();
        if (st->fetch())
            m.cpuServerMs = st->getInt32(1);
        m.cpuMs = elapsedMs(start);
    }
    tr->commit();
}

void BenchmarkRunner::measureParallelCpu(BenchmarkMetrics& m)
{
    const int n = settingsM.parallelConnections;
    std::vector<IDatabasePtr> connections;
    // Transactions and statements are created and destroyed on this thread;
    // the worker threads only execute. fb-cpp releases interfaces a second
    // time after commit() or free(), which is harmless when it happens one
    // after another but corrupts memory when threads do it concurrently.
    std::vector<ITransactionPtr> transactions;
    std::vector<IStatementPtr> statements;
    // open connections would prevent dropping the database at the end
    auto finish = [&]()
    {
        statements.clear();
        for (auto& tr : transactions)
        {
            try { tr->commit(); } catch (...) {}
        }
        transactions.clear();
        for (auto& db : connections)
            close(db);
    };
    try
    {
        for (int i = 0; i < n; ++i)
        {
            checkCancel();
            connections.push_back(openConnection());
        }
        try
        {
            ITransactionPtr tr = startTransaction(connections.front(), true);
            {   // scope
                IStatementPtr st = connections.front()->createStatement(tr);
                st->prepare(BenchmarkSql::serverProcesses);
                st->execute();
                if (st->fetch())
                    m.serverProcesses = int(st->getInt64(0));
            }
            tr->commit();
        }
        catch (const std::exception&)
        {
            // optional information
        }
        for (const auto& db : connections)
        {
            transactions.push_back(startTransaction(db, false));
            statements.push_back(db->createStatement(transactions.back()));
            statements.back()->prepare(BenchmarkSql::cpuTest);
            statements.back()->setInt32(0, settingsM.cpuIterations);
        }
    }
    catch (...)
    {
        finish();
        throw;
    }

    std::vector<std::string> errors(n);
    auto start = Clock::now();
    runParallel(n, [&statements, &errors](int i)
    {
        try
        {
            // an executable procedure: no cursor is opened
            statements[i]->execute();
            statements[i]->fetch();
        }
        catch (const std::exception& e)
        {
            errors[i] = e.what();
        }
    });
    double duration = elapsedMs(start);

    finish();
    for (const auto& e : errors)
    {
        if (!e.empty())
            throw std::runtime_error(e);
    }
    m.parallelCpuMs = duration;
}

void BenchmarkRunner::measureServerBursts(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    // the procedure measures its own run time, so preparing a new statement
    // per burst does not distort the result; a full commit empties the GTT
    auto burst = [&]() -> double
    {
        ITransactionPtr tr = startTransaction(db, false);
        double ms = 0.0;
        {   // scope
            IStatementPtr st = db->createStatement(tr);
            st->prepare(BenchmarkSql::cpuTest);
            st->setInt32(0, serverBurstIterations);
            st->execute();
            if (st->fetch())
                ms = st->getInt32(1);
        }
        tr->commit();
        return ms;
    };

    std::vector<double> idle, hot;
    for (int i = 0; i < settingsM.bursts; ++i)
    {
        checkCancel();
        std::this_thread::sleep_for(std::chrono::milliseconds(idlePauseMs));
        idle.push_back(burst());
    }
    for (int i = 0; i < settingsM.bursts; ++i)
        hot.push_back(burst());
    m.serverBurstIdleMs = getBenchmarkPercentile(idle, 0.5);
    m.serverBurstHotMs = getBenchmarkPercentile(hot, 0.5);
}

double BenchmarkRunner::measureCommitLatency(const IDatabasePtr& db, int count,
    double roundTripMs)
{
    // COMMIT RETAINING writes the changed pages exactly like COMMIT, but
    // the prepared statement can be reused
    ITransactionPtr tr = startTransaction(db, false);
    double perCommit = 0.0;
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::commitInsert);
        auto start = Clock::now();
        for (int i = 0; i < count; ++i)
        {
            if (i % 20 == 0)
                checkCancel();
            st->setInt32(0, commitIdBase + i);
            st->execute();
            tr->commitRetain();
        }
        perCommit = elapsedMs(start) / count;
    }
    tr->commit();
    // every iteration needs two round trips (execute and commit)
    return std::max(0.0, perCommit - 2.0 * roundTripMs);
}

void BenchmarkRunner::fillTable(const IDatabasePtr& db, int firstId, int rows,
    int payloadLength)
{
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::fillTable);
        for (int done = 0; done < rows; done += chunkRows)
        {
            checkCancel();
            st->setInt32(0, firstId + done);
            st->setInt32(1, std::min(chunkRows, rows - done));
            st->setInt32(2, payloadLength);
            st->execute();
        }
    }
    tr->commit();
}

double BenchmarkRunner::timedFill(const IDatabasePtr& db, int firstId, int* inserted)
{
    // up to diskRows rows within the time limit, in one transaction like
    // fillTable(); the time is extrapolated to diskRows rows
    const int rows = settingsM.diskRows;
    const double limitMs = settingsM.driveSeconds * 1000.0;
    auto start = Clock::now();
    int done = 0;
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::fillTable);
        while (done < rows && (done == 0 || elapsedMs(start) < limitMs))
        {
            checkCancel();
            const int count = std::min(driveChunkRows, rows - done);
            st->setInt32(0, firstId + done);
            st->setInt32(1, count);
            st->setInt32(2, settingsM.diskPayloadLength);
            st->execute();
            done += count;
        }
    }
    tr->commit();
    if (inserted)
        *inserted = done;
    return elapsedMs(start) * rows / done;
}

double BenchmarkRunner::executeChunked(const IDatabasePtr& db, const char* sql,
    int firstId, int lastId)
{
    // one transaction, one server call per ID range
    auto start = Clock::now();
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(sql);
        for (int first = firstId; first <= lastId; first += chunkRows)
        {
            checkCancel();
            st->setInt32(0, first);
            st->setInt32(1, std::min(first + chunkRows - 1, lastId));
            st->execute();
        }
    }
    tr->commit();
    return elapsedMs(start);
}

double BenchmarkRunner::timedChunked(const IDatabasePtr& db, const char* sql,
    int firstId, int lastId, int* processed)
{
    const double limitMs = settingsM.driveSeconds * 1000.0;
    auto start = Clock::now();
    int done = 0;
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(sql);
        for (int first = firstId; first <= lastId
            && (done == 0 || elapsedMs(start) < limitMs); first += driveChunkRows)
        {
            checkCancel();
            const int last = std::min(first + driveChunkRows - 1, lastId);
            st->setInt32(0, first);
            st->setInt32(1, last);
            st->execute();
            done += last - first + 1;
        }
    }
    tr->commit();
    *processed = done;
    return elapsedMs(start);
}

void BenchmarkRunner::measureDisk(IDatabasePtr& db, BenchmarkMetrics& m)
{
    const int rows = settingsM.diskRows;
    step(_("Disk test: inserting rows..."));
    auto start = Clock::now();
    fillTable(db, 1, rows, settingsM.diskPayloadLength);
    m.insertMs = elapsedMs(start);

    // Before anything is deleted, so that both inserts get new pages and
    // are comparable; afterwards free pages would be reused.
    step(_("Disk test with a small cache: writing, reading, changing, deleting..."));
    measureSmallCache(db, m, rows + 1);  // and one step

    step(_("Disk test: updating rows..."));
    m.updateMs = executeChunked(db, BenchmarkSql::bulkUpdate, 1, rows);

    step(_("Disk test: deleting rows..."));
    // the commit latency rows are outside the ID ranges
    m.deleteMs = executeChunked(db, BenchmarkSql::bulkDelete, 1, rows);
    // what the time limited drive tests left behind (not measured)
    executeChunked(db, BenchmarkSql::bulkDelete, rows + 1, 2 * rows);
    for (const auto& range : parallelDriveRangesM)
        executeChunked(db, BenchmarkSql::bulkDelete, range.first, range.second);
    parallelDriveRangesM.clear();

    step(_("Disk test: garbage collection..."));
    m.garbageCollectMs = executeTimed(db, BenchmarkSql::garbageCollect);
}

void BenchmarkRunner::measureSmallCache(IDatabasePtr& db, BenchmarkMetrics& m,
    int firstId)
{
    // SuperServer ignores cache sizes requested by a connection; only the
    // page buffers in the database header count, and they are read when
    // the database is opened. Only the benchmark's own connections use the
    // temporary database, so closing them closes the database.
    close(db);
    writeHeaderPageBuffers(connectionStringM, smallCachePages);
    db = openConnection();
    m.smallCachePages = readDatabaseInfo(db).cachePages;

    int rows = 0;
    m.smallCacheInsertMs = timedFill(db, firstId, &rows);
    m.smallCacheRows = rows;
    m.smallCacheScanMs = timedScan(db);

    // the same rows, read, changed and deleted; every part within the time
    // limit
    publish(_("Disk test: reading single rows..."));
    measureDriveLookups(db, m, firstId, rows);
    publish(_("Disk test: changing rows..."));
    m.smallCacheUpdateMs = timedChunked(db, BenchmarkSql::bulkUpdate, firstId,
        firstId + rows - 1, &m.smallCacheUpdates);
    publish(_("Disk test: deleting rows..."));
    m.smallCacheDeleteMs = timedChunked(db, BenchmarkSql::bulkDelete, firstId,
        firstId + rows - 1, &m.smallCacheDeletes);

    step(wxString::Format(_("Disk test with %d connections at once..."),
        settingsM.parallelConnections));
    measureParallelDrive(m);

    // back to the server default for the following tests
    close(db);
    writeHeaderPageBuffers(connectionStringM, 0);
    db = openConnection();
}

void BenchmarkRunner::measureDriveLookups(const IDatabasePtr& db, BenchmarkMetrics& m,
    int firstId, int rows)
{
    const double limitMs = settingsM.driveSeconds * 1000.0;
    auto start = Clock::now();
    int done = 0;
    ITransactionPtr tr = startTransaction(db, true);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::driveLookups);
        while (done < rows && (done == 0 || elapsedMs(start) < limitMs))
        {
            checkCancel();
            const int count = std::min(driveLookupChunk, rows - done);
            st->setInt32(0, firstId);
            st->setInt32(1, rows);
            st->setInt32(2, done);
            st->setInt32(3, count);
            st->execute();
            st->fetch();
            done += count;
        }
    }
    tr->commit();
    m.smallCacheLookupMs = elapsedMs(start);
    m.smallCacheLookups = done;
}

void BenchmarkRunner::measureParallelDrive(BenchmarkMetrics& m)
{
    // inserts into the database with the minimal cache on parallel
    // connections; like measureParallelCpu(), all objects are created and
    // freed on this thread and the workers only execute
    const int n = settingsM.parallelConnections;
    const int rowsPerConnection = std::max(driveChunkRows, settingsM.diskRows / n);
    const double limitMs = settingsM.driveSeconds * 1000.0 / 2.0;
    std::vector<IDatabasePtr> connections;
    std::vector<ITransactionPtr> transactions;
    std::vector<IStatementPtr> statements;
    auto finish = [&]()
    {
        statements.clear();
        for (auto& tr : transactions)
        {
            try { tr->commit(); } catch (...) {}
        }
        transactions.clear();
        for (auto& db : connections)
            close(db);
    };
    try
    {
        for (int i = 0; i < n; ++i)
        {
            checkCancel();
            connections.push_back(openConnection());
            transactions.push_back(startTransaction(connections.back(), false));
            statements.push_back(connections.back()->createStatement(transactions.back()));
            statements.back()->prepare(BenchmarkSql::fillTable);
        }
    }
    catch (...)
    {
        finish();
        throw;
    }

    std::vector<int> inserted(n, 0);
    std::vector<std::string> errors(n);
    auto start = Clock::now();
    runParallel(n, [&](int i)
    {
        try
        {
            const int firstId = parallelDriveIdBase + i * parallelDriveIdsPerConnection;
            while (inserted[i] < rowsPerConnection && !cancelM
                && (inserted[i] == 0 || elapsedMs(start) < limitMs))
            {
                const int count = std::min(driveChunkRows, rowsPerConnection - inserted[i]);
                statements[i]->setInt32(0, firstId + inserted[i]);
                statements[i]->setInt32(1, count);
                statements[i]->setInt32(2, settingsM.diskPayloadLength);
                statements[i]->execute();
                inserted[i] += count;
            }
        }
        catch (const std::exception& e)
        {
            errors[i] = e.what();
        }
    });
    double duration = elapsedMs(start);
    finish();
    for (int i = 0; i < n; ++i)
    {
        if (inserted[i] > 0)
        {
            const int firstId = parallelDriveIdBase + i * parallelDriveIdsPerConnection;
            parallelDriveRangesM.push_back({ firstId, firstId + inserted[i] - 1 });
        }
    }
    checkCancel();
    for (const auto& e : errors)
    {
        if (!e.empty())
            throw std::runtime_error(e);
    }
    m.parallelDriveMs = duration;
    m.parallelDriveRows = std::accumulate(inserted.begin(), inserted.end(), int64_t(0));
}

void BenchmarkRunner::measureServerRead(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    ITransactionPtr tr = startTransaction(db, true);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::serverRead);
        for (int sorted = 0; sorted <= 1; ++sorted)
        {
            checkCancel();
            st->setInt32(0, sorted);
            st->execute();
            if (st->fetch())
            {
                if (sorted)
                    m.serverSortMs = st->getInt32(2);
                else
                    m.serverReadMs = st->getInt32(2);
            }
        }
    }
    tr->commit();
}

void BenchmarkRunner::measureTempSort(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    if (m.scanBytes <= 0)
        return;
    // copies of the test rows until the data exceeds the sort memory
    const double target = settingsM.tempSortMegabytes * 1024.0 * 1024.0;
    const int copies = std::clamp(int(std::ceil(target / double(m.scanBytes))), 1, 64);
    ITransactionPtr tr = startTransaction(db, true);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::tempSort);
        st->setInt32(0, copies);
        st->execute();
        if (st->fetch())
        {
            m.tempSortRows = st->getInt32(0);
            m.tempSortBytes = int64_t(st->getDouble(1));
            m.tempSortMs = st->getInt32(2);
        }
    }
    tr->commit();
}

void BenchmarkRunner::measureWideInserts(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    // every row is one round trip with 100 parameters, like many small
    // application inserts; the values are prepared before the timing
    const int rows = settingsM.wideRows;
    std::vector<std::string> texts;
    for (int c = 0; c < BenchmarkSql::wideColumns; ++c)
        texts.push_back("value " + std::to_string(c));
    ITransactionPtr tr = startTransaction(db, false);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::getWideInsertStatement());
        auto start = Clock::now();
        for (int i = 0; i < rows; ++i)
        {
            if (i % 100 == 0)
                checkCancel();
            st->setInt32(0, i + 1);
            for (int c = 1; c < BenchmarkSql::wideColumns; ++c)
            {
                if (c <= 33)
                    st->setInt32(c, i + c);
                else if (c <= 66)
                    st->setString(c, texts[c]);
                else
                    st->setDouble(c, i * 0.5 + c);
            }
            st->execute();
        }
        m.wideInsertMs = elapsedMs(start);
        m.wideInsertRows = rows;
    }
    tr->commit();
}

void BenchmarkRunner::measureThroughput(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    ITransactionPtr tr = startTransaction(db, true);
    {   // scope
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::throughput);

        double clientMs = 0.0;
        int64_t rows = 0;
        int64_t bytes = 0;
        auto start = Clock::now();
        st->execute();
        while (st->fetch())
        {
            // the conversion of the row is the work FlameRobin does itself
            auto convertStart = Clock::now();
            st->getInt32(0);
            bytes += 4 + int64_t(st->getString(1).size())
                + int64_t(st->getString(2).size());
            clientMs += elapsedMs(convertStart);
            if (++rows % 1000 == 0)
                checkCancel();
        }
        m.streamMs = elapsedMs(start);
        m.streamClientMs = clientMs;
        m.streamRows = rows;
        m.streamBytes = bytes;
    }
    tr->commit();
}

void BenchmarkRunner::measureRowByRow(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    const int rows = settingsM.rowByRowRows;
    {   // scope
        auto start = Clock::now();
        ITransactionPtr tr = startTransaction(db, false);
        {   // scope
            IStatementPtr st = db->createStatement(tr);
            st->prepare(BenchmarkSql::rowInsert);
            for (int i = 0; i < rows; ++i)
            {
                if (i % 100 == 0)
                    checkCancel();
                st->setInt32(0, 1 + i);
                st->execute();
            }
        }
        tr->commit();
        m.rowByRowMs = elapsedMs(start);
    }
    {   // scope
        auto start = Clock::now();
        ITransactionPtr tr = startTransaction(db, false);
        {   // scope
            IStatementPtr st = db->createStatement(tr);
            st->prepare(BenchmarkSql::fillRows);
            st->setInt32(0, 1 + rows);
            st->setInt32(1, rows);
            st->execute();
        }
        tr->commit();
        m.batchMs = elapsedMs(start);
    }
}

void BenchmarkRunner::measurePrepare(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    ITransactionPtr tr = startTransaction(db, true);
    auto start = Clock::now();
    for (int i = 0; i < settingsM.prepareCount; ++i)
    {
        if (i % 20 == 0)
            checkCancel();
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::prepareProbe);
    }
    m.prepareMs = elapsedMs(start) / settingsM.prepareCount;
    tr->commit();
}

void BenchmarkRunner::measureBlob(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    std::vector<char> data(blobSize);
    for (int i = 0; i < blobSize; ++i)
        data[i] = char(i * 7 + i / 251);

    {   // scope
        auto start = Clock::now();
        ITransactionPtr tr = startTransaction(db, false);
        {   // scope
            IStatementPtr st = db->createStatement(tr);
            st->prepare(BenchmarkSql::blobInsert);
            for (int i = 0; i < settingsM.blobCount; ++i)
            {
                if (i % 10 == 0)
                    checkCancel();
                IBlobPtr blob = db->createBlob(tr);
                blob->create();
                for (int pos = 0; pos < blobSize; pos += blobChunk)
                    blob->write(data.data() + pos, blobChunk);
                blob->close();
                st->setInt32(0, i + 1);
                st->setBlob(1, blob);
                st->execute();
            }
        }
        tr->commit();
        m.blobWriteMs = elapsedMs(start);
        m.blobBytes = int64_t(settingsM.blobCount) * blobSize;
    }
    {   // scope
        std::vector<char> buffer(blobChunk);
        auto start = Clock::now();
        ITransactionPtr tr = startTransaction(db, true);
        {   // scope
            IStatementPtr st = db->createStatement(tr);
            st->prepare(BenchmarkSql::blobRead);
            st->execute();
            int count = 0;
            while (st->fetch())
            {
                if (++count % 10 == 0)
                    checkCancel();
                IBlobPtr blob = st->getBlob(0);
                if (!blob)
                    continue;
                blob->open();
                while (blob->read(buffer.data(), blobChunk) > 0)
                    ;
                blob->close();
            }
        }
        tr->commit();
        m.blobReadMs = elapsedMs(start);
    }
}

void BenchmarkRunner::measureVersions(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    executeTimed(db, BenchmarkSql::hotFill);

    // reads the frequently updated rows in the given transaction, returns
    // the average time per read
    auto read = [&](const ITransactionPtr& tr, int count) -> double
    {
        IStatementPtr st = db->createStatement(tr);
        st->prepare(BenchmarkSql::hotRead);
        auto start = Clock::now();
        for (int i = 0; i < count; ++i)
        {
            st->execute();
            st->fetch();
        }
        return elapsedMs(start) / count;
    };

    // a long running snapshot transaction, like a report that stays open
    ITransactionPtr oldTr = db->createTransaction();
    oldTr->setAccessMode(TransactionAccessMode::Read);
    oldTr->setIsolationLevel(TransactionIsolationLevel::Concurrency);
    oldTr->start();
    m.versionReadBeforeMs = read(oldTr, versionReads);

    // every committed update creates a new record version that cannot be
    // garbage collected while the old transaction is open
    for (int i = 0; i < settingsM.versionRounds; ++i)
    {
        checkCancel();
        executeTimed(db, BenchmarkSql::hotUpdate);
    }
    m.versionReadWithOldTxMs = read(oldTr, versionReads);
    oldTr->commit();

    // the first read after the old transaction ended removes the old
    // versions (cooperative garbage collection); only this read is timed
    ITransactionPtr newTr = startTransaction(db, true);
    m.versionReadAfterCommitMs = read(newTr, 1);
    newTr->commit();
}

void BenchmarkRunner::measureOltp(BenchmarkMetrics& m)
{
    // every client inserts into its own ID range
    int firstInsertId = oltpIdBase;
    for (int clients : getOltpLevels())
    {
        step(wxString::Format(_("OLTP workload, parallel clients: %d..."),
            clients));
        m.oltp.push_back(runOltpLevel(clients, firstInsertId));
        firstInsertId += clients * oltpIdsPerClient;
    }
}

BenchmarkOltpResult BenchmarkRunner::runOltpLevel(int clients, int firstInsertId)
{
    // Connections, transactions and statements are created and destroyed on
    // this thread (see measureParallelCpu); the clients only execute, and
    // the lookups are executable procedures, so no cursor is used.
    struct Client
    {
        IDatabasePtr db;
        ITransactionPtr tr;
        IStatementPtr point;
        IStatementPtr range;
        IStatementPtr update;
        IStatementPtr insert;
        std::vector<double> latencies;
        int64_t conflicts = 0;
        std::string error;
    };
    std::vector<Client> all(clients);
    auto finish = [&]()
    {
        for (auto& c : all)
        {
            c.point.reset();
            c.range.reset();
            c.update.reset();
            c.insert.reset();
            if (c.tr)
            {
                try { c.tr->commit(); } catch (...) {}
                c.tr.reset();
            }
            close(c.db);
        }
    };
    try
    {
        for (auto& c : all)
        {
            checkCancel();
            c.db = openConnection();
            c.tr = c.db->createTransaction();
            c.tr->setIsolationLevel(TransactionIsolationLevel::ReadCommitted);
            c.tr->start();
            auto prepare = [&c](const char* sql)
            {
                IStatementPtr st = c.db->createStatement(c.tr);
                st->prepare(sql);
                return st;
            };
            c.point = prepare(BenchmarkSql::oltpPointSelect);
            c.range = prepare(BenchmarkSql::oltpRangeSelect);
            c.update = prepare(BenchmarkSql::oltpUpdate);
            c.insert = prepare(BenchmarkSql::oltpInsert);
        }
    }
    catch (...)
    {
        finish();
        throw;
    }

    const auto duration = std::chrono::seconds(settingsM.oltpSeconds);
    const int scanRows = settingsM.scanRows;
    auto start = Clock::now();
    runParallel(clients, [&](int index)
    {
        Client& c = all[index];
        try
        {
            std::mt19937 gen(unsigned(firstInsertId + index));
            std::uniform_int_distribution<int> id(1, scanRows);
            // the range stays inside the filled IDs 1 .. scanRows
            std::uniform_int_distribution<int> range(1,
                std::max(1, scanRows - BenchmarkSql::oltpRangeRows + 1));
            int nextId = firstInsertId + index * oltpIdsPerClient;
            int failuresInRow = 0;
            while (Clock::now() - start < duration && !cancelM)
            {
                auto txStart = Clock::now();
                try
                {
                    for (int i = 0; i < oltpPointSelects; ++i)
                    {
                        c.point->setInt32(0, id(gen));
                        c.point->execute();
                        c.point->fetch();
                    }
                    c.range->setInt32(0, range(gen));
                    c.range->execute();
                    c.range->fetch();
                    c.update->setInt32(0, id(gen));
                    c.update->execute();
                    c.insert->setInt32(0, nextId++);
                    c.insert->execute();
                    c.tr->commitRetain();
                    c.latencies.push_back(elapsedMs(txStart));
                    failuresInRow = 0;
                }
                catch (const std::exception& e)
                {
                    if (cancelM)
                        break;
                    // update conflicts and deadlocks with the other clients
                    // belong to the workload; an error that repeats does not
                    if (++failuresInRow >= oltpMaxFailuresInRow)
                    {
                        c.error = e.what();
                        break;
                    }
                    ++c.conflicts;
                    c.tr->rollbackRetain();
                }
            }
        }
        catch (const std::exception& e)
        {
            c.error = e.what();
        }
    });
    double seconds = elapsedMs(start) / 1000.0;
    finish();
    checkCancel();

    BenchmarkOltpResult result;
    result.clients = clients;
    result.seconds = seconds;
    std::vector<double> latencies;
    for (const auto& c : all)
    {
        if (!c.error.empty())
            throw std::runtime_error(c.error);
        latencies.insert(latencies.end(), c.latencies.begin(), c.latencies.end());
        result.conflicts += c.conflicts;
    }
    result.transactions = int64_t(latencies.size());
    result.avgMs = getBenchmarkAverage(latencies);
    result.p95Ms = getBenchmarkPercentile(std::move(latencies), 0.95);
    return result;
}

void BenchmarkRunner::comparePageSizes(BenchmarkReport& report)
{
    BenchmarkMetrics& m = report.metrics;
    const std::vector<int> supported = getSupportedPageSizes(m.serverMajorVersion);
    for (int size : comparedPageSizes)
    {
        if (std::find(supported.begin(), supported.end(), size) == supported.end())
        {
            step(wxString::Format(_("Page size comparison: %d bytes is not "
                "supported by Firebird %s, skipped..."), size, m.serverVersion));
            continue;
        }
        step(wxString::Format(_("Page size comparison: %d bytes..."), size));

        const wxString name = getPageSizeName(size);
        const std::string connectionString = getPageSizeConnectionString(size);
        IDatabasePtr db = makeDatabase(paramsM, connectionString);
        db->create(size, 3);
        track(db);
        try
        {
            m.pageSizes.push_back(measurePageSize(db, connectionString, size,
                m.latencyMedianMs.value_or(0.0)));
        }
        catch (const std::exception&)
        {
            close(db);
            dropPageSizeDatabase(report, connectionString, name);
            throw;
        }
        dropPageSizeDatabase(report, connectionString, name);
    }
}

BenchmarkPageSizeResult BenchmarkRunner::measurePageSize(IDatabasePtr& db,
    const std::string& connectionString, int pageSize, double roundTripMs)
{
    // the storage and cache tests of the main run, in a database of its own
    BenchmarkPageSizeResult r;
    r.pageSize = pageSize;
    const int rows = settingsM.diskRows;
    for (const char* sql : BenchmarkSql::getStorageCreateStatements())
        executeTimed(db, sql);
    r.cachePages = readDatabaseInfo(db).cachePages;
    r.commitMs = measureCommitLatency(db, commitsPerPageSize, roundTripMs);

    auto start = Clock::now();
    fillTable(db, 1, rows, settingsM.diskPayloadLength);
    r.insertMs = elapsedMs(start);

    // scans right after a new connection and repeated
    close(db);
    db = openConnection(connectionString);
    r.coldScanMs = timedScan(db);
    r.warmScanMs = timedScan(db);

    // storage I/O with a minimal cache, as in the main test: new rows into
    // new pages, exactly like the first insert
    close(db);
    writeHeaderPageBuffers(connectionString, smallCachePages);
    db = openConnection(connectionString);
    r.smallCacheInsertMs = timedFill(db, rows + 1, nullptr);
    close(db);
    return r;
}

void BenchmarkRunner::dropPageSizeDatabase(BenchmarkReport& report,
    const std::string& connectionString, const wxString& name)
{
    try
    {
        IDatabasePtr db = makeDatabase(paramsM, connectionString);
        db->connect();
        verifyAndDrop(db, name);
    }
    catch (const std::exception& e)
    {
        wxString path = toWxString(connectionString);
        report.undroppedDatabases.push_back(path);
        report.errorMessage += (report.errorMessage.empty() ? "" : "\n")
            + wxString::Format(_("The temporary benchmark database %s could "
                "not be dropped: %s"), path, wxString::FromUTF8(e.what()));
    }
}

void BenchmarkRunner::addEngineStatistics(const IDatabasePtr& db, BenchmarkMetrics& m)
{
    // optional: the counters of the current connection are added up over
    // the benchmark's connections
    auto add = [](std::optional<int64_t>& target, int64_t value)
    {
        target = target.value_or(0) + value;
    };
    try
    {
        ITransactionPtr tr = startTransaction(db, true);
        {   // scope
            IStatementPtr st = db->createStatement(tr);
            st->prepare(BenchmarkSql::ioStats);
            st->execute();
            if (st->fetch())
            {
                add(m.pageReads, st->getInt64(0));
                add(m.pageWrites, st->getInt64(1));
                add(m.pageFetches, st->getInt64(2));
                add(m.pageMarks, st->getInt64(3));
            }
            IStatementPtr rs = db->createStatement(tr);
            rs->prepare(BenchmarkSql::recordStats);
            rs->execute();
            if (rs->fetch())
            {
                add(m.recordBackouts, rs->getInt64(0));
                add(m.recordPurges, rs->getInt64(1));
                add(m.recordExpunges, rs->getInt64(2));
            }
        }
        tr->commit();
    }
    catch (const std::exception&)
    {
    }
}

} // namespace fr
