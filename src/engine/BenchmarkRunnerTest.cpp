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

#include <wx/init.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include "engine/BenchmarkRunner.h"
#include "engine/db/DatabaseFactory.h"
#include "engine/db/IDatabase.h"
#include "engine/db/TestUtils.h"

// Runs the benchmark against the server given in IBPP_TEST_SERVER: once to
// the end, cancelled between two steps, cancelled from another thread during
// a server operation and with the page size comparison. Checks that the
// temporary databases are gone in every case and that the leftover cleanup
// refuses other databases.

namespace
{

bool databaseExists(const std::string& connectionString)
{
    try
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase();
        db->setConnectionString(connectionString);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();
        db->disconnect();
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool createDatabase(const std::string& connectionString)
{
    try
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase();
        db->setConnectionString(connectionString);
        db->setCredentials("SYSDBA", "masterkey");
        db->create(8192, 3);
        db->disconnect();
        return true;
    }
    catch (const std::exception& e)
    {
        fr_test::printException(e, "create a database");
        return false;
    }
}

void dropDatabase(const std::string& connectionString)
{
    try
    {
        fr::IDatabasePtr db = fr::DatabaseFactory::createDatabase();
        db->setConnectionString(connectionString);
        db->setCredentials("SYSDBA", "masterkey");
        db->connect();
        db->drop();
    }
    catch (...) {}
}

fr::BenchmarkConnectionParams makeParams(const std::string& server)
{
    // the directory of the regular test databases
    std::string probe = fr_test::getTestDbPath("benchmark_runner_test");
    fr::BenchmarkConnectionParams params;
    params.connectionPrefix = server + ":";
    params.host = server.substr(0, server.find('/'));
    params.serverDirectory = fr::getBenchmarkDirectoryFromPath(
        wxString::FromUTF8(probe.c_str()));
    params.username = "SYSDBA";
    params.password = "masterkey";
    params.pageSize = 16384;
    params.targetDescription = "benchmark_runner_test";
    return params;
}

} // namespace

int main(int argc, char** argv)
{
    wxInitializer initializer(argc, argv);
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxWidgets\n";
        return 1;
    }

    const char* envServer = std::getenv("IBPP_TEST_SERVER");
    const std::string server = envServer ? envServer : "";
    if (server.empty())
    {
        std::cout << "IBPP_TEST_SERVER is not set, skipping BenchmarkRunnerTest.\n";
        return 0;
    }

    bool ok = true;
    std::cout << "Complete Quick run\n";
    {   // scope
        fr::BenchmarkConnectionParams params = makeParams(server);
        fr::BenchmarkRunner runner(params, fr::BenchmarkIntensity::Quick);
        int lastStep = 0;
        bool monotonic = true;
        fr::BenchmarkReport report = runner.run(
            [&](const fr::BenchmarkProgress& p)
            {
                if (p.step < lastStep)
                    monotonic = false;
                lastStep = p.step;
                std::cout << "    [" << p.step << "/" << p.stepCount << "] "
                    << p.message.utf8_str() << "\n";
            });
        const fr::BenchmarkMetrics& m = report.metrics;
        if (!report.errorMessage.empty())
            std::cerr << "    error: " << report.errorMessage.utf8_str() << "\n";

        ok &= fr_test::check(report.errorMessage.empty(), "run without error");
        ok &= fr_test::check(!report.cancelled, "not cancelled");
        ok &= fr_test::check(monotonic, "progress is monotonic");
        ok &= fr_test::check(lastStep == runner.getStepCount(), "all steps reported");
        ok &= fr_test::check(report.benchmarkDatabaseCreated
            && report.benchmarkDatabaseDropped,
            "temporary database reported as created and dropped");
        ok &= fr_test::check(!databaseExists(params.connectionPrefix
            + std::string(report.benchmarkDatabasePath.utf8_str())),
            "temporary database no longer exists");
        ok &= fr_test::check(m.benchmarkDatabase.pageSize == 16384,
            "requested page size used");
        ok &= fr_test::check(m.serverMajorVersion >= 3, "server version read");
        ok &= fr_test::check(m.connectMs && m.cpuMs && m.cpuServerMs
            && m.parallelCpuMs && m.serverBurstIdleMs && m.commitMs
            && m.insertMs && m.updateMs && m.deleteMs && m.garbageCollectMs
            && m.smallCacheInsertMs && m.coldScanMs && m.warmScanMs
            && m.serverReadMs && m.serverSortMs && m.latencyMedianMs
            && m.latencySparseMs && m.streamMs, "all measurements present");
        ok &= fr_test::check(m.smallCachePages && *m.smallCachePages <= 1000,
            "the minimal page cache is really used");
        ok &= fr_test::check(m.smallCacheRows && *m.smallCacheRows > 0
            && *m.smallCacheRows <= m.settings.diskRows,
            "the drive test inserted rows within its time limit");
        ok &= fr_test::check(m.smallCacheLookupMs && m.smallCacheLookups > 0
            && m.smallCacheUpdateMs && m.smallCacheUpdates > 0
            && m.smallCacheDeleteMs && m.smallCacheDeletes > 0
            && m.smallCacheDeletes <= *m.smallCacheRows,
            "the drive test read, updated and deleted the inserted rows");
        ok &= fr_test::check(m.parallelDriveMs && m.parallelDriveRows > 0,
            "the drive test ran on parallel connections");
        ok &= fr_test::check(m.tempSortMs && m.tempSortBytes
            >= int64_t(m.settings.tempSortMegabytes) * 1024 * 1024 * 9 / 10,
            "the temporary file test sorted the requested amount of data");
        ok &= fr_test::check(m.wideInsertMs && m.wideInsertRows == m.settings.wideRows,
            "the net test inserted all wide rows");
        ok &= fr_test::check(m.system.cpuThreads > 0 && m.system.memoryTotalBytes > 0
            && !m.system.osName.empty(), "hardware and software of this machine");
        ok &= fr_test::check(m.networkPath && !m.networkPath->serverAddress.empty()
            && std::any_of(m.serverPorts.begin(), m.serverPorts.end(),
                [&m](const fr::BenchmarkPortCheck& c)
                {
                    return c.port == m.serverPort
                        && c.state == fr::BenchmarkPortCheck::State::Open;
                }), "network path and the open port of the server");
        // Events are only tested with a fixed RemoteAuxPort; the Firebird
        // container of the CI keeps the default 0, and Firebird 3 cannot
        // report it, so the runner skips the test there. Set
        // FR_TEST_REMOTE_AUX_PORT=1 when the test server has a fixed
        // RemoteAuxPort that this machine can reach, so that the event
        // delivery must be measured.
        const char* auxPortEnv = std::getenv("FR_TEST_REMOTE_AUX_PORT");
        const bool eventsRequired = auxPortEnv && std::string(auxPortEnv) == "1";
        const bool eventsSkipped =
            m.eventResult == fr::BenchmarkEventResult::NotTestedRandomPort
            || m.eventResult == fr::BenchmarkEventResult::NotTestedUnknownPort;
        ok &= fr_test::check(m.eventMs.has_value()
            || (eventsSkipped && !eventsRequired && !m.eventStatus.empty()),
            eventsRequired ? "events are delivered"
                : "events are delivered or not testable on this server");
        if (!m.eventMs)
            std::cout << "    events: " << m.eventStatus.utf8_str() << "\n";
        ok &= fr_test::check(report.score.total > 0.0
            && report.score.categories.size() == 6, "score with six categories");
        ok &= fr_test::check(report.toHtml().Contains("id=\"score\""), "HTML report");
        // Quick runs 1, 2 and 4 clients
        ok &= fr_test::check(m.oltp.size() == 3 && std::all_of(m.oltp.begin(),
            m.oltp.end(), [](const fr::BenchmarkOltpResult& o)
            {
                return o.transactions > 0;
            }), "every OLTP level ran transactions");
        fr::BenchmarkConnectionParams many = params;
        many.manyUsers = true;
        ok &= fr_test::check(fr::BenchmarkRunner(many, fr::BenchmarkIntensity::Quick)
            .getStepCount() == runner.getStepCount() + 2,
            "the many users option adds the levels 50 and 250");
        // optional tests are either measured or listed with the reason
        // (BLOBs need a Firebird 4+ client library)
        ok &= fr_test::check(m.rowByRowMs && m.prepareMs && m.versionReadWithOldTxMs
            && (m.blobReadMs || m.skippedTests.size() == 1),
            "optional tests measured");
        for (const auto& skipped : m.skippedTests)
        {
            std::cout << "    skipped: " << skipped.first.utf8_str() << ": "
                << skipped.second.utf8_str() << "\n";
        }
        ok &= fr_test::check(m.streamRows == m.settings.scanRows
            + m.settings.commitCount, "all rows streamed");
        // generated rows plus the rows of the commit test, whose payload
        // 'commit latency test' has 19 characters
        ok &= fr_test::check(m.scanBytes == int64_t(m.settings.scanRows)
            * m.settings.scanPayloadLength + int64_t(m.settings.commitCount)
            * 19, "scan saw all payload bytes");
        ok &= fr_test::check(!report.findings.empty(), "findings present");
        ok &= fr_test::check(report.toMarkdown().Contains("<!-- FlameRobin benchmark results"),
            "markdown report with the results for comparisons");
        ok &= fr_test::check(fr::parseBenchmarkSnapshot(report.toHtml()).has_value(),
            "a saved report can be compared later");
    }

    std::cout << "Cancelled run\n";
    {   // scope
        fr::BenchmarkConnectionParams params = makeParams(server);
        fr::BenchmarkRunner runner(params, fr::BenchmarkIntensity::Quick);
        bool cleanupReported = false;
        fr::BenchmarkReport report = runner.run(
            [&](const fr::BenchmarkProgress& p)
            {
                // cancel once the test tables exist (step 9 creates them)
                if (p.step >= 10)
                    runner.requestCancel();
                if (p.cleaningUp)
                    cleanupReported = true;
            });
        ok &= fr_test::check(report.cancelled, "run is cancelled");
        ok &= fr_test::check(!report.metrics.insertMs, "stopped before the disk test");
        ok &= fr_test::check(cleanupReported, "cleanup is reported");
        ok &= fr_test::check(report.benchmarkDatabaseDropped,
            "temporary database dropped after cancel");
        ok &= fr_test::check(!databaseExists(params.connectionPrefix
            + std::string(report.benchmarkDatabasePath.utf8_str())),
            "temporary database no longer exists after cancel");
    }

    std::cout << "Cancel from another thread during a server operation\n";
    {   // scope
        fr::BenchmarkConnectionParams params = makeParams(server);
        fr::BenchmarkRunner runner(params, fr::BenchmarkIntensity::Deep);
        std::thread canceller;
        std::chrono::steady_clock::time_point cancelTime;
        fr::BenchmarkReport report = runner.run(
            [&](const fr::BenchmarkProgress& p)
            {
                // the insert step of the disk test; cancel while it runs
                if (p.message.Contains("inserting") && !canceller.joinable())
                {
                    canceller = std::thread([&]()
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(300));
                        cancelTime = std::chrono::steady_clock::now();
                        runner.requestCancel();
                    });
                }
            });
        auto finished = std::chrono::steady_clock::now();
        if (canceller.joinable())
            canceller.join();
        double seconds = std::chrono::duration<double>(finished - cancelTime).count();
        std::cout << "    run ended " << seconds << " s after the cancel request\n";
        ok &= fr_test::check(report.cancelled && report.errorMessage.empty(),
            "run is cancelled without error");
        ok &= fr_test::check(seconds < 10.0, "cancel takes effect quickly");
        ok &= fr_test::check(report.benchmarkDatabaseDropped,
            "temporary database dropped after cancel");
        ok &= fr_test::check(!databaseExists(runner.getConnectionString()),
            "temporary database no longer exists");
    }

    std::cout << "Page size comparison\n";
    {   // scope
        fr::BenchmarkConnectionParams params = makeParams(server);
        params.comparePageSizes = true;
        fr::BenchmarkRunner runner(params, fr::BenchmarkIntensity::Quick);
        fr::BenchmarkReport report = runner.run(fr::BenchmarkRunner::ProgressCallback());
        const fr::BenchmarkMetrics& m = report.metrics;
        if (!report.errorMessage.empty())
            std::cerr << "    error: " << report.errorMessage.utf8_str() << "\n";
        ok &= fr_test::check(report.errorMessage.empty(), "comparison without error");
        ok &= fr_test::check(m.pageSizes.size()
            == fr::getSupportedPageSizes(m.serverMajorVersion).size(),
            "every supported page size measured");
        ok &= fr_test::check(std::all_of(m.pageSizes.begin(), m.pageSizes.end(),
            [](const fr::BenchmarkPageSizeResult& p)
            {
                return p.insertMs > 0.0 && p.warmScanMs > 0.0
                    && p.smallCacheInsertMs > 0.0;
            }), "all page size measurements present");
        ok &= fr_test::check(report.undroppedDatabases.empty(),
            "no comparison database left behind");
        bool anyExists = false;
        for (const auto& connectionString : runner.getAllConnectionStrings())
            anyExists = anyExists || databaseExists(connectionString);
        ok &= fr_test::check(!anyExists, "no temporary database exists afterwards");
    }

    std::cout << "Leftover cleanup never drops other databases\n";
    {   // scope
        fr::BenchmarkConnectionParams params = makeParams(server);
        // a user database, and one whose name only starts like a benchmark
        // database
        const std::string directory = std::string(params.serverDirectory.utf8_str());
        const std::string others[] = {
            server + ":" + fr_test::getTestDbPath("benchmark_not_a_benchmark_db"),
            server + ":" + directory + "FR_BENCHMARK_USERDATA.FDB" };
        for (const std::string& other : others)
        {
            ok &= createDatabase(other);
            bool refused = false;
            try
            {
                fr::BenchmarkRunner::dropLeftoverDatabase(params, other);
            }
            catch (const std::exception&)
            {
                refused = true;
            }
            ok &= fr_test::check(refused, "dropping a non-benchmark database is refused");
            ok &= fr_test::check(databaseExists(other), "the other database still exists");
            dropDatabase(other);
        }
    }

    std::cout << (ok ? "ALL BENCHMARK RUNNER TESTS PASSED\n"
                     : "SOME BENCHMARK RUNNER TESTS FAILED\n");
    return ok ? 0 : 1;
}
