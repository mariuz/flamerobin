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

#include <wx/arrstr.h>
#include <wx/init.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <regex>
#include <string>

#include "engine/Benchmark.h"

using namespace fr;

namespace
{

bool ok = true;

void check(bool condition, const char* name)
{
    if (condition)
        std::cout << "  PASSED: " << name << "\n";
    else
    {
        std::cerr << "  FAILED: " << name << "\n";
        ok = false;
    }
}

const BenchmarkFinding* findFinding(const std::vector<BenchmarkFinding>& findings,
    const wxString& titlePart)
{
    auto it = std::find_if(findings.begin(), findings.end(),
        [&](const BenchmarkFinding& f) { return f.title.Contains(titlePart); });
    return it == findings.end() ? nullptr : &*it;
}

// checks severity, side and audience of the finding whose title contains
// titlePart, and optionally a text in its recommendation
void expect(const BenchmarkMetrics& m, const wxString& titlePart,
    BenchmarkSeverity severity, BenchmarkSide side, BenchmarkAudience audience,
    const char* name, const wxString& recommendationPart = wxEmptyString)
{
    auto findings = analyzeBenchmark(m);
    const BenchmarkFinding* f = findFinding(findings, titlePart);
    bool good = f && f->severity == severity && f->side == side
        && f->audience == audience
        && (recommendationPart.empty() || f->recommendation.Contains(recommendationPart));
    if (!good && f)
    {
        std::cerr << "    got severity " << int(f->severity) << ", side "
            << int(f->side) << ", audience " << int(f->audience) << "\n";
    }
    check(good, name);
}

void expectNone(const BenchmarkMetrics& m, const wxString& titlePart,
    const char* name)
{
    auto findings = analyzeBenchmark(m);
    check(findFinding(findings, titlePart) == nullptr, name);
}

// metrics of a healthy server measured from a client, values as measured
// on a current machine
BenchmarkMetrics healthyMetrics()
{
    BenchmarkMetrics m;
    m.settings = BenchmarkSettings::forIntensity(BenchmarkIntensity::Standard);
    m.runLocation = BenchmarkRunLocation::Client;
    m.connectionKind = BenchmarkConnectionKind::TcpRemote;
    m.connectionHost = "dbserver";
    m.localHostName = "workstation";
    m.clientOsFamily = "Windows";
    m.serverVersion = "5.0.4";
    m.serverMajorVersion = 5;
    m.serverPlatform = "Windows";
    m.clientVersion = "WI-V5.0.4.1812 Firebird 5.0";
    m.benchmarkDirectory = "D:\\db\\";
    m.benchmarkDatabase.pageSize = 8192;
    m.benchmarkDatabase.cachePages = 16384;
    m.benchmarkDatabase.forcedWrites = true;
    m.connectMs = 40.0;
    m.tempDirFileMs = 0.2;
    m.clientBurstIdleMs = 10.0;
    m.clientBurstHotMs = 10.0;
    m.serverBurstIdleMs = 30.0;
    m.serverBurstHotMs = 30.0;
    m.cpuMs = 70.0;
    m.cpuServerMs = 65.0;
    m.parallelCpuMs = 80.0;
    m.commitMs = 1.5;
    m.insertMs = 850.0;
    m.updateMs = 650.0;
    m.deleteMs = 450.0;
    m.garbageCollectMs = 600.0;
    m.smallCachePages = 50;
    m.smallCacheInsertMs = 3750.0;
    m.smallCacheRows = 50000;
    m.smallCacheScanMs = 20.0;
    m.smallCacheLookupMs = 1000.0;
    m.smallCacheLookups = 50000;
    m.smallCacheUpdateMs = 5000.0;
    m.smallCacheUpdates = 50000;
    m.smallCacheDeleteMs = 4000.0;
    m.smallCacheDeletes = 50000;
    m.parallelDriveMs = 2000.0;
    m.parallelDriveRows = 25000;
    m.tempSortMs = 1200.0;
    m.tempSortBytes = 200 * 1024 * 1024;
    m.tempSortRows = 200000;
    m.wideInsertMs = 800.0;
    m.wideInsertRows = 2000;
    m.coldScanMs = 50.0;
    m.warmScanMs = 45.0;
    m.scanBytes = 50 * 1024 * 1024;
    m.serverReadMs = 60.0;
    m.serverSortMs = 130.0;
    m.latencyAvgMs = 0.25;
    m.latencyMedianMs = 0.2;
    m.latencyP95Ms = 0.3;
    m.latencyMaxMs = 1.0;
    m.latencySparseMs = 0.25;
    m.streamMs = 700.0;
    m.streamClientMs = 50.0;
    m.streamRows = 50000;
    m.streamBytes = 52000000;
    m.rowByRowMs = 400.0;
    m.batchMs = 150.0;
    m.prepareMs = 1.0;
    m.blobWriteMs = 300.0;
    m.blobReadMs = 250.0;
    m.blobBytes = 200 * 64 * 1024;
    m.versionReadBeforeMs = 0.5;
    m.versionReadWithOldTxMs = 0.6;
    m.versionReadAfterCommitMs = 0.5;
    m.oltp = { { 1, 5000, 0, 5.0, 1.0, 2.0 }, { 2, 9500, 0, 5.0, 1.1, 2.2 },
               { 4, 18000, 2, 5.0, 1.2, 2.5 }, { 8, 30000, 5, 5.0, 1.4, 3.0 } };
    return m;
}

void testHelpers()
{
    std::cout << "Helpers\n";
    check(classifyBenchmarkConnection("", "") == BenchmarkConnectionKind::Embedded,
        "no protocol is embedded");
    check(classifyBenchmarkConnection("XNET", "") == BenchmarkConnectionKind::LocalXnet,
        "XNET");
    check(classifyBenchmarkConnection("TCPv6", "::1/52811")
        == BenchmarkConnectionKind::TcpLoopback, "IPv6 loopback with port");
    check(classifyBenchmarkConnection("TCPv4", "127.0.0.1/3050")
        == BenchmarkConnectionKind::TcpLoopback, "IPv4 loopback with port");
    check(classifyBenchmarkConnection("TCPv4", "10.1.2.3/50000")
        == BenchmarkConnectionKind::TcpRemote, "remote address");

    check(determineBenchmarkRunLocation(BenchmarkConnectionKind::Embedded,
        "", "srv", false) == BenchmarkRunLocation::Server,
        "embedded runs on the server");
    check(determineBenchmarkRunLocation(BenchmarkConnectionKind::TcpLoopback,
        "localhost", "srv", false) == BenchmarkRunLocation::Server,
        "loopback runs on the server");
    check(determineBenchmarkRunLocation(BenchmarkConnectionKind::TcpRemote,
        "SRV.example.com", "srv", false) == BenchmarkRunLocation::Server,
        "own host name runs on the server");
    check(determineBenchmarkRunLocation(BenchmarkConnectionKind::TcpRemote,
        "dbserver", "workstation", true) == BenchmarkRunLocation::Server,
        "local database file runs on the server");
    check(determineBenchmarkRunLocation(BenchmarkConnectionKind::TcpRemote,
        "dbserver", "workstation", false) == BenchmarkRunLocation::Client,
        "other host runs on a client");

    check(getServerPlatformName("WI-V5.0.4.1812 Firebird 5.0") == "Windows", "Windows");
    check(getServerPlatformName("LI-V4.0.5.3140 Firebird 4.0") == "Linux", "Linux");
    check(getServerPlatformName("LA-V5.0.1.1469 Firebird 5.0") == "Linux",
        "Linux on another CPU architecture");
    check(getServerPlatformName("UI-V3.0.12.33787 Firebird 3.0") == "macOS", "macOS");
    check(getServerPlatformName("XX-V1.0").empty(), "unknown platform");
    check(getServerPlatformName("").empty(), "no version string");

    // isc_info_version answers with a counted list of strings
    const std::string counted = std::string("\x02\x1b") + "LI-V6.3.4.1812 Firebird 5.0"
        + "\x10" + "LI-V6.3.4.1812/X";
    const std::vector<wxString> list = parseBenchmarkVersionList(counted);
    check(list.size() == 2 && list.front() == "LI-V6.3.4.1812 Firebird 5.0",
        "the entries of a counted version list");
    check(getServerPlatformName(list.front()) == "Linux",
        "platform of a counted version list");
    check(parseBenchmarkVersionList("WI-V5.0.4.1812 Firebird 5.0")
        == std::vector<wxString>{ "WI-V5.0.4.1812 Firebird 5.0" }, "plain version string");
    check(getServerPlatformName("\x02\x1bWI-V6.3.4.1812 Firebird 5.0") == "Windows",
        "platform code after leading bytes");

    {   // scope
        std::vector<double> values;
        for (int i = 100; i >= 1; --i)
            values.push_back(i);
        check(getBenchmarkPercentile(values, 0.0) == 1.0
            && getBenchmarkPercentile(values, 1.0) == 100.0,
            "percentile bounds on unsorted values");
        check(getBenchmarkPercentile(values, 0.95) == 95.0, "95th percentile");
        check(getBenchmarkPercentile({}, 0.5) == 0.0
            && getBenchmarkAverage({}) == 0.0, "no values");
        check(getBenchmarkAverage(values) == 50.5, "average");
    }

    check(getBenchmarkDirectoryFromPath("C:\\data\\prod.fdb") == "C:\\data\\",
        "Windows directory");
    check(getBenchmarkDirectoryFromPath("/var/db/prod.fdb") == "/var/db/",
        "Unix directory");
    check(getBenchmarkDirectoryFromPath("employee").empty(), "alias has no directory");
    check(getSupportedPageSizes(3).size() == 3 && getSupportedPageSizes(5).size() == 4,
        "32 KB pages only from Firebird 4");
    check(makeBenchmarkDatabasePath("C:\\data", "FR_BENCHMARK_X.FDB")
        == "C:\\data\\FR_BENCHMARK_X.FDB", "Windows separator is kept");
    check(makeBenchmarkDatabasePath("/var/db/", "FR_BENCHMARK_X.FDB")
        == "/var/db/FR_BENCHMARK_X.FDB", "no duplicate separator");
    check(makeBenchmarkDatabasePath("/var/db", "FR_BENCHMARK_X.FDB")
        == "/var/db/FR_BENCHMARK_X.FDB", "Unix separator is added");

    check(escapeMarkdownTableCell("a|b\nc") == "a\\|b c", "markdown cell escaping");
    check(formatBenchmarkMilliseconds(0.25) == "0.25 ms"
        && formatBenchmarkMilliseconds(250.0) == "250 ms"
        && formatBenchmarkMilliseconds(25000.0) == "25.0 s"
        && formatBenchmarkMilliseconds(0.0018) == "0.0018 ms"
        && formatBenchmarkMilliseconds(0.0) == "0.00 ms", "duration format");
}

void testSafetyCheck()
{
    std::cout << "Drop safety check\n";
    const wxString name = "FR_BENCHMARK_20261003_120000_ABCDEF.FDB";
    check(isBenchmarkDatabaseFile(
        "C:\\DATA\\FR_BENCHMARK_20261003_120000_ABCDEF.FDB", name),
        "matching file (case insensitive)");
    check(isBenchmarkDatabaseFile(
        "/var/db/fr_benchmark_20261003_120000_abcdef.fdb", name),
        "matching file on Unix");
    check(!isBenchmarkDatabaseFile("C:\\DATA\\PROD.FDB", name),
        "a user database never matches");
    check(!isBenchmarkDatabaseFile(
        "C:\\DATA\\XFR_BENCHMARK_20261003_120000_ABCDEF.FDB", name),
        "only the complete file name matches");
    check(!isBenchmarkDatabaseFile("", name), "unknown file name never matches");
    check(!isBenchmarkDatabaseFile("C:\\DATA\\PROD.FDB", "PROD.FDB"),
        "names without the benchmark prefix are refused");
    check(isBenchmarkDatabaseFile("srv:/db/FR_BENCHMARK_20261003_120000_ABCDEF_P8192.FDB",
        "FR_BENCHMARK_20261003_120000_ABCDEF_P8192.FDB"),
        "page size comparison database");

    check(isBenchmarkDatabaseName(name), "generated name has the benchmark form");
    check(!isBenchmarkDatabaseName("FR_BENCHMARK_DATA.FDB"),
        "a user database with the prefix is not a benchmark database");
    check(!isBenchmarkDatabaseName("FR_BENCHMARK_20261003_120000_ABCDEF.GDB"),
        "other extension");
    check(!isBenchmarkDatabaseName("FR_BENCHMARK_2026103_120000_ABCDEF.FDB"),
        "date with wrong length");
    check(!isBenchmarkDatabaseName("FR_BENCHMARK_20261003_120000_ABCDEG.FDB"),
        "random part must be hexadecimal");
    check(!isBenchmarkDatabaseName("FR_BENCHMARK_20261003_120000_ABCDEF_X.FDB"),
        "unknown suffix");
    check(!isBenchmarkDatabaseName("FR_BENCHMARK_20261003_120000_ABCDEF_P.FDB"),
        "page size suffix without size");
    check(!isBenchmarkDatabaseFile("C:\\DATA\\FR_BENCHMARK_DATA.FDB",
        "FR_BENCHMARK_DATA.FDB"), "drop check uses the strict name form");
}

void testPendingDatabases()
{
    std::cout << "Remembered temporary databases\n";
    BenchmarkPendingDatabase pending;
    pending.embedded = true;
    pending.registrationId = "17";
    pending.clientLibrary = "C:\\Firebird 5, x64\\fbclient.dll";
    pending.connectionString = "srv/3050:D:\\Daten, alt|100%\\FR_BENCHMARK_X.FDB";
    wxString entry = pending.toString();
    check(!entry.Contains(","), "entries contain no list separator");
    auto parsed = BenchmarkPendingDatabase::parse(entry);
    check(parsed && parsed->embedded && parsed->registrationId == "17"
        && parsed->clientLibrary == pending.clientLibrary
        && parsed->connectionString == pending.connectionString,
        "commas, separators and percent signs survive");
    check(!BenchmarkPendingDatabase::parse("S|C:\\db\\FR_BENCHMARK_X.FDB"),
        "incomplete entries are rejected");
    check(!BenchmarkPendingDatabase::parse("X|1|lib|srv:db"), "unknown kind");
    check(!BenchmarkPendingDatabase::parse("S|1|lib|"), "missing connection string");
}

void testHealthy()
{
    std::cout << "Healthy system\n";
    auto findings = analyzeBenchmark(healthyMetrics());
    check(std::none_of(findings.begin(), findings.end(),
        [](const BenchmarkFinding& f)
        {
            return f.severity == BenchmarkSeverity::Warning
                || f.severity == BenchmarkSeverity::Problem;
        }), "no warnings for a healthy system");
    check(findFinding(findings, "No bottleneck") != nullptr, "no bottleneck reported");
    check(findFinding(findings, "ran on a client computer") != nullptr,
        "client run is described");
}

void testRules()
{
    using S = BenchmarkSeverity;
    using Side = BenchmarkSide;
    using W = BenchmarkAudience;
    std::cout << "Analyzer rules\n";

    // run location and version
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.runLocation = BenchmarkRunLocation::Server;
        m.connectionKind = BenchmarkConnectionKind::Embedded;
        expect(m, "ran on the database server itself", S::Info, Side::Configuration,
            W::Administrator, "server run is described");
        m.connectionKind = BenchmarkConnectionKind::TcpRemote;
        expect(m, "but over a network connection", S::Info, Side::Configuration,
            W::Administrator, "server run over the network interface");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.serverMajorVersion = 3;
        m.serverVersion = "3.0.12";
        expect(m, "Firebird 3.0.12", S::Info, Side::Configuration,
            W::Administrator, "Firebird 3 upgrade hint", "Firebird 5");
        expectNone(m, "settings could not be read", "no RDB$CONFIG hint on Firebird 3");
        m.serverMajorVersion = 4;
        m.serverVersion = "4.0.5";
        expect(m, "settings could not be read", S::Info, Side::Configuration,
            W::Administrator, "RDB$CONFIG not readable on Firebird 4");
        m.serverConfig["ServerMode"] = "Super";
        expectNone(m, "settings could not be read", "readable configuration");
    }

    // connection
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.connectMs = 400.0;
        expect(m, "Connecting to the server is slow", S::Warning, Side::Network,
            W::Administrator, "slow connect is a warning", "DNS");
        m.connectMs = 1500.0;
        expect(m, "Connecting to the server is slow", S::Problem, Side::Network,
            W::Administrator, "very slow connect is a problem");
        expect(m, "Every new connection takes noticeable time", S::Info, Side::Client,
            W::Developer, "developers are told to pool", "pool");
    }

    // power management
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.clientBurstIdleMs = 25.0;
        expect(m, "slows down this computer", S::Problem, Side::Client,
            W::Administrator, "power saving on a Windows client", "powercfg");
        m.clientBurstIdleMs = 15.0;
        expect(m, "slows down this computer", S::Warning, Side::Client,
            W::Administrator, "moderate power saving is a warning");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        expectNone(m, "runs on battery", "unknown power supply is not judged");
        m.localOnBattery = false;
        expectNone(m, "runs on battery", "mains power");
        m.localOnBattery = true;
        expect(m, "runs on battery", S::Warning, Side::Client,
            W::Administrator, "a client on battery", "mains power");
        m.runLocation = BenchmarkRunLocation::Server;
        m.connectionKind = BenchmarkConnectionKind::Embedded;
        expect(m, "runs on battery", S::Warning, Side::Server,
            W::Administrator, "a server on battery", "mains power");
        auto findings = analyzeBenchmark(m);
        const BenchmarkFinding* battery = findFinding(findings, "runs on battery");
        check(battery && battery->detail.Contains("database server included"),
            "on the server the battery affects all values");
        auto results = buildBenchmarkResults(m);
        check(std::any_of(results.begin(), results.end(), [](const BenchmarkResult& r)
            {
                return r.test == "Power supply" && r.value == "battery"
                    && r.category == BenchmarkCategory::LocalMachine;
            }), "the power supply is listed");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.serverPlatform = "Linux";
        m.serverBurstIdleMs = 75.0;
        expect(m, "Power saving slows down the server", S::Problem, Side::Server,
            W::Administrator, "power saving on a Linux server", "governor");
        m.serverBurstHotMs = 5.0;
        m.serverBurstIdleMs = 12.0;
        expectNone(m, "Power saving slows down the server",
            "too short server bursts are not judged");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.latencySparseMs = 1.2;
        expect(m, "after a short pause is slow", S::Problem, Side::Network,
            W::Administrator, "slow sparse queries", "Energy Efficient Ethernet");
        m.latencyMedianMs = 0.08;
        m.latencySparseMs = 0.41;
        expect(m, "after a short pause is slow", S::Warning, Side::Network,
            W::Administrator, "a fraction of a millisecond is only a warning");
        m.latencySparseMs = 0.3;
        expectNone(m, "after a short pause is slow", "tiny differences are ignored");
    }

    // latency
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.latencyMedianMs = 2.0;
        m.latencyAvgMs = 2.0;
        m.latencySparseMs = 2.0;
        m.latencyP95Ms = 2.5;
        expect(m, "Requests over the network take long", S::Warning, Side::Network,
            W::Administrator, "high latency is a warning", "WLAN");
        expect(m, "Small queries mostly wait for the network", S::Info, Side::Network,
            W::Developer, "developers are told to reduce round trips", "Batch API");
        m.latencyMedianMs = 6.0;
        m.latencySparseMs = 6.0;
        m.latencyP95Ms = 7.0;
        expect(m, "Requests over the network take long", S::Problem, Side::Network,
            W::Administrator, "very high latency is a problem");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.runLocation = BenchmarkRunLocation::Server;
        m.connectionKind = BenchmarkConnectionKind::TcpLoopback;
        m.latencyMedianMs = 1.5;
        m.latencySparseMs = 1.5;
        m.latencyP95Ms = 1.6;
        expect(m, "Even the local connection is slow", S::Problem, Side::Server,
            W::Administrator, "slow loopback on a Windows server", "XNET");
        expectNone(m, "Requests over the network take long", "loopback is not network latency");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.latencyP95Ms = 2.0;
        expect(m, "Response times vary a lot", S::Warning, Side::Network,
            W::Administrator, "jitter");
    }

    // throughput
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.streamMs = 20000.0;
        m.serverConfig["TcpRemoteBufferSize"] = "8192";
        m.wireEncrypted = true;
        expect(m, "Large results arrive slowly", S::Problem, Side::Network, W::Administrator,
            "slow transfer from a client", "TcpRemoteBufferSize");
        expect(m, "fetch less data", S::Info, Side::Network, W::Developer,
            "developers are told to fetch less", "ROWS");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.streamClientMs = 500.0;
        expect(m, "slow at processing received data", S::Warning, Side::Client,
            W::Administrator, "slow client processing");
    }

    // CPU
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.cpuServerMs = 300.0;   // 333 iterations/ms
        m.cpuMs = 300.0;
        m.parallelCpuMs = 330.0;
        expect(m, "The server calculates slowly", S::Warning, Side::Server, W::Hardware,
            "slow CPU is a hardware warning", "single core");
        m.cpuServerMs = 600.0;
        expect(m, "The server calculates slowly", S::Problem, Side::Server, W::Hardware,
            "very slow CPU is a problem");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.parallelCpuMs = 280.0;  // 4 connections, no scaling at all
        expect(m, "does not use all its processor cores", S::Problem, Side::Server,
            W::Hardware, "missing scaling without mask is hardware", "vCPU");
        m.serverConfig["CpuAffinityMask"] = "1";
        expect(m, "does not use all its processor cores", S::Problem, Side::Server,
            W::Administrator, "missing scaling with mask is configuration",
            "CpuAffinityMask");
    }

    // storage
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.commitMs = 20.0;
        m.osSyncWriteMs = 0.5;
        expect(m, "Saving changes is slow", S::Problem, Side::Server, W::Administrator,
            "slow commit with fast storage points to a virus scanner", "Defender");
        expect(m, "Each saved change takes noticeable time", S::Info, Side::Server, W::Developer,
            "developers are told to group changes", "transaction");
        m.osSyncWriteMs = 18.0;
        expect(m, "Saving changes is slow", S::Problem, Side::Server, W::Hardware,
            "slow commit with slow storage is hardware", "domain controller");
        m.commitMs = 7.0;
        m.osSyncWriteMs.reset();
        expect(m, "Saving changes is slow", S::Warning, Side::Server, W::Hardware,
            "moderately slow commit is a warning");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.benchmarkDatabase.forcedWrites = false;
        expect(m, "not written safely during the test", S::Info, Side::Configuration,
            W::Administrator, "forced writes off during the test");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.insertMs = 5000.0;   // 10000 rows/s
        expect(m, "Writing many rows is slow", S::Warning, Side::Server, W::Hardware,
            "slow bulk writes", "RAID 10");
        m.insertMs = 20000.0;
        expect(m, "Writing many rows is slow", S::Problem, Side::Server, W::Hardware,
            "very slow bulk writes");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.smallCacheInsertMs = 20000.0;  // 2500 rows/s
        expect(m, "The disk is slow", S::Warning, Side::Server, W::Hardware,
            "slow storage with minimal cache", "RAID");
        m.smallCacheInsertMs = 50000.0;
        expect(m, "The disk is slow", S::Problem, Side::Server, W::Hardware,
            "very slow storage with minimal cache");
        m.smallCachePages = 2048;
        expect(m, "could not use a small cache", S::Info, Side::Server,
            W::Administrator, "minimal cache was not applied");
        expectNone(m, "The disk is slow", "no storage verdict without minimal cache");
    }

    // local files and free space
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.tempDirFileMs = 4.0;
        expect(m, "virus scanner", S::Problem, Side::Client, W::Administrator,
            "slow file operations on the client");
        m.tempDirFileMs = 1.5;
        expect(m, "virus scanner", S::Warning, Side::Client, W::Administrator,
            "moderately slow file operations");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.runLocation = BenchmarkRunLocation::Server;
        m.connectionKind = BenchmarkConnectionKind::Embedded;
        m.databaseDirFileMs = 5.0;
        m.databaseDiskFreeBytes = int64_t(1) * 1024 * 1024 * 1024;
        expect(m, "virus scanner", S::Problem, Side::Server, W::Administrator,
            "slow file operations in the database directory", "firebird.exe");
        expect(m, "Little free disk space", S::Warning, Side::Server,
            W::Hardware, "little free space");
    }

    // cache and sort
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.benchmarkDatabase.cachePages = 2048;   // 16 MB < 50 MB test data
        expect(m, "small page cache", S::Warning, Side::Configuration,
            W::Administrator, "small page cache", "DefaultDbCachePages");
        m.serverConfig["ServerMode"] = "Classic";
        expectNone(m, "small page cache", "no cache size verdict for Classic");
        expect(m, "Server mode Classic", S::Info, Side::Configuration,
            W::Administrator, "Classic is described", "SuperServer");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.serverProcesses = 4;
        expect(m, "Server mode Classic", S::Info, Side::Configuration,
            W::Administrator, "Classic detected by server processes");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.coldScanMs = 2000.0;
        m.warmScanMs = 100.0;
        expect(m, "not in memory is read much more slowly", S::Info, Side::Server,
            W::Hardware, "uncached reads are expensive", "RAM");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.serverSortMs = 800.0;
        m.serverConfig["TempCacheLimit"] = "67108864";
        expect(m, "Sorting is slow", S::Warning, Side::Server, W::Administrator,
            "slow sorting", "67108864");
        expect(m, "Sort only the columns you need", S::Info, Side::Server, W::Developer,
            "developers are told to sort less");
    }

    // inspected database
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        BenchmarkDatabaseInfo selected;
        selected.pageSize = 4096;
        selected.cachePages = 256;
        selected.pages = 100000;
        selected.forcedWrites = false;
        selected.oldestActiveTransaction = 1000;
        selected.nextTransaction = 2000000;
        selected.backupState = 1;
        selected.cryptState = 1;
        m.inspectedDatabase = selected;
        expect(m, "page size of the database is small", S::Warning, Side::Configuration,
            W::Administrator, "small page size", "gbak");
        expect(m, "selected database does not write changes safely", S::Warning,
            Side::Configuration, W::Administrator, "forced writes off");
        expect(m, "has been open for a long time", S::Problem, Side::Configuration,
            W::Developer, "transaction gap", "MON$TRANSACTIONS");
        expect(m, "locked for a backup", S::Problem, Side::Configuration,
            W::Administrator, "nbackup lock");
        expect(m, "database is encrypted", S::Info, Side::Server,
            W::Administrator, "encryption");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.clientVersion = "WI-V3.0.10.33601 Firebird 3.0";
        expect(m, "old client library", S::Warning, Side::Client, W::Developer,
            "old client library");
    }

    // page size
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.requestedPageSize = 32768;
        m.benchmarkDatabase.pageSize = 16384;
        m.serverVersion = "3.0.12";
        expect(m, "Page size adjusted", S::Info, Side::Configuration,
            W::Administrator, "adjusted page size is reported", "Firebird 4");
        m.pageSizes = { { 4096, 2048, 1.0, 1200.0, 80.0, 70.0, 4000.0 },
                        { 8192, 2048, 1.0, 900.0, 50.0, 40.0, 3500.0 },
                        { 16384, 2048, 1.0, 1000.0, 30.0, 45.0, 3800.0 } };
        expect(m, "Page size 8192 performs best", S::Info, Side::Configuration,
            W::Administrator, "page size comparison picks the best size", "gbak");
    }

    // application patterns
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.rowByRowMs = 800.0;   // 5x
        expect(m, "Sending rows one by one", S::Info, Side::Client, W::Developer,
            "row-by-row is expensive", "EXECUTE BLOCK");
        m.rowByRowMs = 3000.0;  // 20x
        expect(m, "Sending rows one by one", S::Warning, Side::Client,
            W::Developer, "row-by-row is very expensive");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.prepareMs = 5.0;
        expect(m, "Preparing queries", S::Info, Side::Server, W::Developer,
            "prepare cost on Firebird 5", "MaxStatementCacheSize");
        m.prepareMs = 15.0;
        m.serverMajorVersion = 4;
        auto findings = analyzeBenchmark(m);
        const BenchmarkFinding* f = findFinding(findings, "Preparing queries");
        check(f && f->severity == S::Warning
            && !f->recommendation.Contains("MaxStatementCacheSize"),
            "no statement cache advice before Firebird 5");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.blobReadMs = 5000.0;  // 2.5 MB/s
        expect(m, "large data (BLOBs) is slow", S::Warning, Side::Network, W::Developer,
            "slow BLOB reads", "grids");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.versionReadWithOldTxMs = 3.0;  // 6x
        expect(m, "Long open transactions slow down", S::Warning,
            Side::Server, W::Developer, "record versions", "READ COMMITTED");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.versionReadAfterCommitMs = 20.0;   // the reader that cleans up
        expect(m, "Long open transactions slow down", S::Info,
            Side::Server, W::Developer, "garbage collection by the next reader");
        auto findings = analyzeBenchmark(m);
        const BenchmarkFinding* f = findFinding(findings, "Long open transactions");
        check(f && f->detail.Contains("garbage collection")
            && !f->detail.Contains("was started before"),
            "only the cleanup is described");
        m.versionReadAfterCommitMs = 150.0;
        expect(m, "Long open transactions slow down", S::Warning,
            Side::Server, W::Developer, "expensive cleanup is a warning");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.oltp.back().transactions = 6000;   // 8 clients barely faster
        m.oltp.back().conflicts = 400;
        expect(m, "do not get more work done", S::Warning, Side::Server, W::Administrator,
            "OLTP throughput does not scale", "LockHashSlots");
        expect(m, "update conflicts", S::Info, Side::Server, W::Developer,
            "update conflicts", "sequences");
    }
}

void testChecklist()
{
    std::cout << "Configuration checklist\n";
    auto findCheck = [](const std::vector<BenchmarkConfigCheck>& list,
        const wxString& setting) -> const BenchmarkConfigCheck*
    {
        auto it = std::find_if(list.begin(), list.end(),
            [&](const BenchmarkConfigCheck& c) { return c.setting == setting; });
        return it == list.end() ? nullptr : &*it;
    };

    BenchmarkMetrics m = healthyMetrics();
    m.serverConfig["ServerMode"] = "Super";
    m.serverConfig["DefaultDbCachePages"] = "2048";
    m.serverConfig["FileSystemCacheThreshold"] = "1024";
    m.serverConfig["TempCacheLimit"] = "67108864";
    m.serverConfig["CpuAffinityMask"] = "0";
    m.serverConfig["MaxParallelWorkers"] = "1";
    auto list = buildBenchmarkChecklist(m);
    auto cache = findCheck(list, "DefaultDbCachePages");
    check(cache && cache->needsChange && cache->current == "2048",
        "small cache needs a change");
    auto threshold = findCheck(list, "FileSystemCacheThreshold");
    check(threshold && threshold->needsChange, "threshold below cache size");
    auto temp = findCheck(list, "TempCacheLimit");
    check(temp && temp->needsChange, "TempCacheLimit below 1 GB");
    auto mask = findCheck(list, "CpuAffinityMask");
    check(mask && !mask->needsChange, "CpuAffinityMask 0 is fine");
    auto workers = findCheck(list, "MaxParallelWorkers");
    check(workers && workers->needsChange, "parallel workers on Firebird 5");
    check(findCheck(list, "Power plan") != nullptr, "Windows server power plan");
    check(findCheck(list, "vm.swappiness") == nullptr, "no Linux rows for Windows");
    check(findCheck(list, "Network adapter") != nullptr, "client rows on a client run");

    m.serverMajorVersion = 3;
    m.serverPlatform = "Linux";
    m.serverConfig.clear();
    m.serverProcesses = 3;
    list = buildBenchmarkChecklist(m);
    check(findCheck(list, "MaxParallelWorkers") == nullptr,
        "no parallel workers before Firebird 5");
    check(findCheck(list, "vm.swappiness") != nullptr, "Linux server rows");
    auto mode = findCheck(list, "ServerMode");
    check(mode && mode->current == "Classic" && mode->needsChange,
        "Classic detected without RDB$CONFIG");
    check(findCheck(list, "LockHashSlots") != nullptr, "lock table for Classic");
    auto unknown = findCheck(list, "DefaultDbCachePages");
    check(unknown && unknown->current == "?" && !unknown->needsChange,
        "unknown values are not judged");

    // FileSystemCacheThreshold only counts while UseFileSystemCache is not
    // set explicitly
    m = healthyMetrics();
    m.serverConfig["DefaultDbCachePages"] = "100000";
    m.serverConfig["FileSystemCacheThreshold"] = "65536";
    m.serverConfig["UseFileSystemCache"] = "true";
    m.serverConfig["MaxStatementCacheSize"] = "2097152";
    list = buildBenchmarkChecklist(m);
    threshold = findCheck(list, "FileSystemCacheThreshold");
    check(threshold && threshold->needsChange,
        "threshold below the cache bypasses the file cache");
    m.serverConfigExplicit.insert("UseFileSystemCache");
    list = buildBenchmarkChecklist(m);
    threshold = findCheck(list, "FileSystemCacheThreshold");
    check(threshold && !threshold->needsChange,
        "threshold is ignored when UseFileSystemCache is set");
    auto statementCache = findCheck(list, "MaxStatementCacheSize");
    check(statementCache && statementCache->current == "2097152",
        "statement cache size is read on Firebird 5");

    BenchmarkDatabaseInfo selected;
    selected.pageSize = 8192;
    selected.lingerSeconds = 0;
    m.inspectedDatabase = selected;
    list = buildBenchmarkChecklist(m);
    auto linger = findCheck(list, "LINGER");
    check(linger && linger->current == "0", "LINGER of the selected database");
    m.serverConfig["ServerMode"] = "Classic";
    list = buildBenchmarkChecklist(m);
    check(findCheck(list, "LINGER") == nullptr, "no LINGER advice for Classic");
}

void testReport()
{
    std::cout << "Report\n";
    BenchmarkReport report;
    report.target = "Server of TEST (dbserver/3050), directory /var/db/";
    report.benchmarkDatabasePath = "/var/db/FR_BENCHMARK_X.FDB";
    report.benchmarkDatabaseCreated = true;
    report.benchmarkDatabaseDropped = true;
    report.intensityName = "Standard";
    report.metrics = healthyMetrics();
    report.metrics.latencyMedianMs = 6.0;
    report.metrics.latencyAvgMs = 6.0;
    report.metrics.latencySparseMs = 6.0;
    report.metrics.latencyP95Ms = 7.0;
    report.evaluate();

    check(!report.results.empty() && !report.checklist.empty(),
        "results and checklist are built");
    check(std::all_of(report.results.begin(), report.results.end(),
        [](const BenchmarkResult& r)
        {
            return !r.explanation.empty() && !r.machine.empty();
        }), "every result names its machine and explains what and why");
    check(std::any_of(report.results.begin(), report.results.end(),
        [](const BenchmarkResult& r)
        {
            return r.category == BenchmarkCategory::ServerCpu
                && r.machine.Contains("dbserver");
        }), "server CPU results name the server");
    check(std::any_of(report.results.begin(), report.results.end(),
        [](const BenchmarkResult& r)
        {
            return r.category == BenchmarkCategory::LocalMachine
                && r.machine.Contains("workstation");
        }), "local results name the client");
    check(std::count_if(report.results.begin(), report.results.end(),
        [](const BenchmarkResult& r) { return r.test.Contains("users at once"); }) == 4,
        "one result per OLTP level");
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.skippedTests.push_back({ "BLOB write / read", "Wrong parameters block kind 6" });
        auto results = buildBenchmarkResults(m);
        check(std::any_of(results.begin(), results.end(),
            [](const BenchmarkResult& r)
            {
                return r.test == "BLOB write / read" && r.value == "not measured"
                    && r.details.Contains("block kind 6");
            }), "skipped optional tests are listed with the reason");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.runLocation = BenchmarkRunLocation::Server;
        m.connectionKind = BenchmarkConnectionKind::Embedded;
        m.connectionHost.clear();
        auto results = buildBenchmarkResults(m);
        auto machineOf = [&results](BenchmarkCategory category) -> wxString
        {
            auto it = std::find_if(results.begin(), results.end(),
                [category](const BenchmarkResult& r) { return r.category == category; });
            return it == results.end() ? wxString() : it->machine;
        };
        check(machineOf(BenchmarkCategory::ServerCpu).Contains("embedded")
            && machineOf(BenchmarkCategory::ServerCpu)
                == machineOf(BenchmarkCategory::LocalMachine),
            "embedded results name one machine for engine and FlameRobin");
        check(machineOf(BenchmarkCategory::Network).Contains("no network"),
            "embedded round trips are not called network");
    }
    check(report.getProblemSummary() == "1 problem needs attention.",
        "summary counts problems and warnings");

    wxString md = report.toMarkdown();
    check(md.Contains("| **Score** | **") && md.Contains("### Results"),
        "markdown starts with the score and the results");
    check(md.Find("| **Score** |") < md.Find("### Results")
        && md.Find("### Results") < md.Find("### Problems and warnings")
        && md.Find("### Problems and warnings") < md.Find("### Values")
        && md.Find("### Values") < md.Find("### System")
        && md.Find("### System") < md.Find("### All measured values"),
        "score, results, problems, values, system, statistics");
    check(!md.Contains("<details>") && !md.Contains("What to do") && !md.Contains("Glossary")
        && !md.Contains("How the score works"), "the markdown has the results only");
    check(md.Contains("| Part | Value | Result | Work | Reference | Reference time | Points |")
        && md.Contains("200 commits in "), "values with the work and the reference");
    check(md.Contains("<!-- FlameRobin benchmark results") && md.Contains("value.commitMs = "),
        "the markdown contains the results for comparisons");
    {   // scope
        BenchmarkReport partial;
        partial.metrics.settings = BenchmarkSettings::forIntensity(BenchmarkIntensity::Standard);
        partial.metrics.commitMs = 1.0;
        partial.evaluate();
        check(getBenchmarkKeyValues(partial.metrics).size()
            == getBenchmarkKeyValues(report.metrics).size()
            && getBenchmarkKeyValues(partial.metrics).size() == 15,
            "the key values have the same rows in every report");
    }

    {   // scope
        // a virus scanner on the server slows both directories down; the
        // advice is given once
        BenchmarkReport local;
        local.metrics = healthyMetrics();
        local.metrics.runLocation = BenchmarkRunLocation::Server;
        local.metrics.connectionKind = BenchmarkConnectionKind::Embedded;
        local.metrics.connectionHost.clear();
        local.metrics.latencyMedianMs = 0.0018;
        local.metrics.tempDirFileMs = 8.0;
        local.metrics.databaseDirFileMs = 8.0;
        local.evaluate();
        auto top = local.getTopRecommendations();
        check(std::count_if(local.findings.begin(), local.findings.end(),
            [](const BenchmarkFinding& f) { return f.title.Contains("virus scanner"); }) == 2
            && std::count_if(top.begin(), top.end(),
                [](const BenchmarkFinding* f) { return f->title.Contains("virus scanner"); }) == 1,
            "the same recommendation is listed once");
        check(std::all_of(top.begin(), top.end(), [](const BenchmarkFinding* f)
            {
                return f->severity == BenchmarkSeverity::Problem
                    || f->severity == BenchmarkSeverity::Warning;
            }) && top.size() <= 3, "at most 3 problems and warnings");
        check(getBenchmarkCategoryInsight(BenchmarkCategory::Network, local)
            == "Not tested: the test used Firebird directly, without a network connection.",
            "embedded network insight");
        check(getBenchmarkCategoryInsight(BenchmarkCategory::ServerStorage, local)
            .StartsWith("Saving and reading data on the server's disk is "),
            "the storage insight in plain words");
        const BenchmarkFinding* topStorage = getBenchmarkTopFinding(
            BenchmarkCategory::ServerStorage, local);
        check(topStorage && topStorage->severity == BenchmarkSeverity::Problem,
            "the most serious finding of a part");
        check(local.toMarkdown().Contains("0.0018 ms"), "microseconds are not shown as 0.00 ms");
    }
    {   // scope
        BenchmarkReport batch;
        batch.metrics = healthyMetrics();
        batch.metrics.rowByRowMs = 10.8;
        batch.metrics.batchMs = 10.5;
        check(getBenchmarkCategoryInsight(BenchmarkCategory::Application, batch)
            .Contains("about as fast"), "no 1x slower");
        batch.metrics.rowByRowMs = 52.5;
        check(getBenchmarkCategoryInsight(BenchmarkCategory::Application, batch)
            .Contains("5 times slower"), "row-by-row factor");
    }

    report.benchmarkDatabaseDropped = false;
    check(report.toMarkdown().Contains("could not be dropped"), "database not dropped");
    report.cancelled = true;
    check(report.toMarkdown().Contains("cancelled"), "cancelled run is marked");
}

// every name in sql that follows one of the patterns
std::vector<std::string> namesAfter(const std::string& sql, const char* pattern)
{
    std::vector<std::string> names;
    std::regex re(pattern, std::regex::icase);
    for (std::sregex_iterator it(sql.begin(), sql.end(), re), end; it != end; ++it)
        names.push_back((*it)[1].str());
    return names;
}

bool isBenchmarkObject(const std::string& name)
{
    return name.rfind("FR_BENCHMARK_", 0) == 0;
}

void testScore()
{
    std::cout << "Score\n";
    BenchmarkMetrics m = healthyMetrics();
    BenchmarkScore score = computeBenchmarkScore(m);
    check(score.categories.size() == 6, "six score categories");
    check(score.total > 0.0, "total score");
    auto category = [&score](const wxString& name) -> const BenchmarkScoreCategory*
    {
        for (const auto& c : score.categories)
        {
            if (c.name == name)
                return &c;
        }
        return nullptr;
    };
    const BenchmarkScoreCategory* storage = category("Server disk");
    check(storage && storage->items.size() == 5,
        "storage: commit, insert, drive test, parallel drive test, temporary files");
    const BenchmarkScoreCategory* net = category("Network");
    check(net && net->items.size() == 4,
        "network: round trip, transfer, connect, wide inserts");

    // twice as fast gives twice the points
    BenchmarkMetrics fast = m;
    fast.commitMs = *m.commitMs / 2.0;
    fast.insertMs = *m.insertMs / 2.0;
    fast.smallCacheInsertMs = *m.smallCacheInsertMs / 2.0;
    fast.smallCacheLookupMs = *m.smallCacheLookupMs / 2.0;
    fast.smallCacheUpdateMs = *m.smallCacheUpdateMs / 2.0;
    fast.smallCacheDeleteMs = *m.smallCacheDeleteMs / 2.0;
    fast.parallelDriveMs = *m.parallelDriveMs / 2.0;
    fast.tempSortMs = *m.tempSortMs / 2.0;
    const BenchmarkScoreCategory* fastStorage = nullptr;
    BenchmarkScore fastScore = computeBenchmarkScore(fast);
    for (const auto& c : fastScore.categories)
    {
        if (c.name == "Server disk")
            fastStorage = &c;
    }
    check(storage && fastStorage && std::abs(fastStorage->points - 2.0 * storage->points) < 1.0,
        "twice as fast gives twice the points");
    check(fastScore.total > score.total, "a faster system gets a higher total");
    check(score.basis == "from a client over the network",
        "client runs use the client references");
    const BenchmarkScoreCategory* local = category("This computer");
    check(local && local->items.size() == 1, "the file test is a diagnosis, not scored");

    // the geometric mean limits extreme values
    BenchmarkMetrics extreme = m;
    extreme.latencyMedianMs = 0.000001;
    const BenchmarkScoreCategory* network = nullptr;
    BenchmarkScore extremeScore = computeBenchmarkScore(extreme);
    for (const auto& c : extremeScore.categories)
    {
        if (c.name == "Network")
            network = &c;
    }
    check(network && network->items.front().points <= 5000.0, "points are capped");

    BenchmarkMetrics embedded = m;
    embedded.connectionKind = BenchmarkConnectionKind::Embedded;
    embedded.runLocation = BenchmarkRunLocation::Server;
    BenchmarkScore embeddedScore = computeBenchmarkScore(embedded);
    check(std::none_of(embeddedScore.categories.begin(), embeddedScore.categories.end(),
        [](const BenchmarkScoreCategory& c) { return c.name == "Network"; }),
        "an embedded run has no network score");
    check(embeddedScore.basis == "this computer, without a server connection",
        "embedded references");
    BenchmarkMetrics local2 = m;
    local2.runLocation = BenchmarkRunLocation::Server;
    local2.connectionKind = BenchmarkConnectionKind::TcpLoopback;
    check(computeBenchmarkScore(local2).basis == "on the server, local connection",
        "local references");

    BenchmarkMetrics notApplied = m;
    notApplied.smallCachePages = 2048;
    BenchmarkScore notAppliedScore = computeBenchmarkScore(notApplied);
    for (const auto& c : notAppliedScore.categories)
    {
        if (c.name == "Server disk")
            check(c.items.size() == 3, "drive tests without minimal cache are not scored");
    }
    check(computeBenchmarkScore(BenchmarkMetrics()).total == 0.0, "nothing measured, no score");
    check(rateBenchmarkPoints(1000.0) == BenchmarkSeverity::Ok
        && rateBenchmarkPoints(900.0) == BenchmarkSeverity::Ok
        && rateBenchmarkPoints(899.0) == BenchmarkSeverity::Warning
        && rateBenchmarkPoints(499.0) == BenchmarkSeverity::Problem, "point ratings");

    check(BenchmarkLimits::commitMs.rate(4.9) == BenchmarkSeverity::Ok
        && BenchmarkLimits::commitMs.rate(5.0) == BenchmarkSeverity::Warning
        && BenchmarkLimits::insertRowsPerSecond.rate(4999.0) == BenchmarkSeverity::Problem,
        "limit boundaries");
}

void testHtml()
{
    std::cout << "HTML report\n";
    BenchmarkReport report;
    report.target = "Server <b>x</b> & \"y\"";
    report.benchmarkDatabasePath = "/var/db/FR_BENCHMARK_X.FDB";
    report.benchmarkDatabaseCreated = true;
    report.benchmarkDatabaseDropped = true;
    report.metrics = healthyMetrics();
    report.metrics.pageSizes = { { 8192, 2048, 1.0, 900.0, 50.0, 40.0, 3500.0 },
                                 { 16384, 2048, 1.0, 1000.0, 30.0, 45.0, 3800.0 } };
    report.evaluate();
    wxString html = report.toHtml("dark");
    check(html.StartsWith("<!doctype html>") && html.Contains("</html>"), "complete page");
    check(html.Contains("data-theme=\"dark\""), "theme of FlameRobin");
    check(report.toHtml().Contains("<html lang=\"en\">"),
        "saved page follows the system theme");
    check(html.Contains("Server &lt;b&gt;x&lt;/b&gt; &amp; &quot;y&quot;")
        && !html.Contains("<b>x</b>"), "text is escaped");
    check(html.Contains("id=\"score\"") && html.Contains("id=\"categories\""),
        "score and categories");
    check(html.Find("<nav class=\"pages\">") < html.Find("<div class=\"overview\">")
        && html.Find("<div class=\"overview\">") < html.Find("id=\"score\"")
        && html.Find("id=\"score\"") < html.Find("id=\"parts\"")
        && html.Find("id=\"parts\"") < html.Find("<div class=\"details\">")
        && html.Find("<div class=\"details\">") < html.Find("id=\"system\"")
        && html.Find("id=\"system\"") < html.Find("id=\"categories\"")
        && html.Find("id=\"categories\"") < html.Find("What to do first"),
        "score, overview, then the detailed report");
    check(html.Contains("id=\"v1\" checked>") && html.Freq('"') > 0
        && html.Find("\" checked") == html.Find("id=\"v1\" checked") + 6,
        "the overview is shown first");
    check(!html.Contains("<details open") && !html.Contains("details open"),
        "all details and panels are closed at first");
    check(html.Contains("#v1:checked ~ .details, #v2:checked ~ .overview { display: none; }")
        && html.Contains("<label for=\"v1\">Overview</label> | <label for=\"v2\">Detailed "
            "report</label>"),
        "the views switch without a script");
    check(html.Contains("<details class=\"part\"><summary class=\"row\">")
        && html.Contains("<th class=\"n\">Reference system</th>")
        && html.Contains("the same in "), "the parts show their values against the reference");
    // one check box, one tile and one panel per section; a CSS rule opens
    // the panel of a checked box, so the page needs no script
    const int sections = int(getBenchmarkReportSections().size());
    int toggles = 0, tiles = 0, panels = 0;
    for (int i = 0; i < sections; ++i)
    {
        toggles += html.Contains(wxString::Format("<input type=\"checkbox\" class=\"toggle\" "
            "id=\"c%d\">", i));
        tiles += html.Contains(wxString::Format("for=\"c%d\"><span class=\"name\">", i));
        panels += html.Contains(wxString::Format("<div class=\"panel p%d\">", i))
            && html.Contains(wxString::Format("#c%d:checked~.tiles .p%d{display:block}", i, i));
    }
    check(toggles == sections && tiles == sections && panels == sections,
        "one tile, one panel and one toggle per section");
    check(html.Contains(".cats > .toggle { position: fixed;"),
        "the toggles do not scroll the page when a tile is clicked");
    check(html.Contains("Server disk") && html.Contains("Settings")
        && html.Contains("Programming patterns"), "every section");
    check(html.Contains("How the score works") && html.Contains("Run details")
        && html.Contains("id=\"glossary\"") && html.Contains("id=\"values\"")
        && html.Contains("id=\"calibration\""), "background information at the end");
    // tooltips for units and tests
    check(html.Contains("<span class=\"tip-box\" role=\"tooltip\">Transactions per second"),
        "units are explained in tooltips");
    check(html.Contains("Pure CPU work of one connection inside Firebird.</span></span>"),
        "tests are explained in tooltips");
    check(html.Contains("<dt>OLTP</dt>") && html.Contains("<dt>Calculation, one "
        "connection</dt>"), "the glossary explains terms and tests");
    check(html.Contains(formatBenchmarkPoints(report.score.total)), "total score shown");
    check(!html.Contains("http://") && !html.Contains("https://")
        && !html.Contains("<script"), "no external resources and no scripts");
    // SVG coordinates and CSS widths must not use a decimal comma
    check(!html.Contains("width:0,") && !html.Contains("y=\"0,")
        && !html.Contains("% \"") , "locale independent numbers");
}

void testFindingSections()
{
    std::cout << "Findings belong to sections\n";
    BenchmarkMetrics m = healthyMetrics();
    m.commitMs = 20.0;
    m.latencyMedianMs = 2.0;
    m.cpuServerMs = 600.0;
    m.versionReadWithOldTxMs = 3.0;
    m.oltp.back().transactions = 6000;
    m.tempDirFileMs = 4.0;
    auto findings = analyzeBenchmark(m);
    auto sectionOf = [&findings](const wxString& title)
    {
        const BenchmarkFinding* f = findFinding(findings, title);
        return f ? getBenchmarkReportSection(f->category) : BenchmarkCategory::Engine;
    };
    using C = BenchmarkCategory;
    check(sectionOf("Saving changes is slow") == C::ServerStorage, "commits: storage");
    check(sectionOf("Requests over the network take long") == C::Network, "latency: network");
    check(sectionOf("The server calculates slowly") == C::ServerCpu, "CPU: server CPU");
    check(sectionOf("Long open transactions") == C::Application, "versions: application");
    check(sectionOf("More users do not get more work done") == C::MultiUser, "OLTP: multi-user");
    check(sectionOf("virus scanner") == C::LocalMachine, "files: this machine");
    check(sectionOf("ran on a client computer") == C::Environment, "run location: configuration");
    check(getBenchmarkReportSection(C::Connection) == C::Network
        && getBenchmarkReportSection(C::Engine) == C::Environment, "merged sections");
}

void testSql()
{
    std::cout << "SQL\n";
    namespace S = BenchmarkSql;
    auto creates = S::getCreateStatements();
    check(creates.size() == 19, "nineteen DDL statements");
    const std::string wideInsert = S::getWideInsertStatement();
    check(std::string(creates.back()).find(" C99 DOUBLE PRECISION)") != std::string::npos
        && std::count(wideInsert.begin(), wideInsert.end(), '?') == S::wideColumns,
        "the net test table has 100 columns and the insert 100 parameters");
    auto storage = S::getStorageCreateStatements();
    check(storage.size() == 7 && std::equal(storage.begin(), storage.end(),
        creates.begin()), "storage DDL is the first part of the DDL");

    const char* const statements[] = { S::engineVersion, S::attachmentInfo,
        S::attachmentWireInfo, S::databaseInfo, S::databaseLinger,
        S::serverConfig, S::cpuTest, S::commitInsert, S::fillTable,
        S::bulkUpdate, S::bulkDelete, S::garbageCollect, S::scan,
        S::serverRead, S::latency, S::largeRoundTrip, S::throughput,
        S::ioStats, S::recordStats, S::rowInsert, S::fillRows,
        S::prepareProbe, S::oltpPointSelect, S::oltpRangeSelect,
        S::oltpUpdate, S::oltpInsert, S::blobInsert, S::blobRead, S::hotFill,
        S::hotUpdate, S::hotRead, S::serverProcesses, S::attachmentDetails,
        S::attachmentTimeouts, S::securityDatabase, S::postEvent, S::driveLookups,
        S::tempSort, S::getWideInsertStatement() };
    std::vector<std::string> all(std::begin(statements), std::end(statements));
    all.insert(all.end(), creates.begin(), creates.end());

    // the most important property of the benchmark: it creates, changes and
    // calls nothing but its own objects
    bool ownObjectsOnly = true;
    bool noDestructiveStatement = true;
    for (const std::string& sql : all)
    {
        const char* const writePatterns[] = {
            R"(CREATE\s+(?:GLOBAL\s+TEMPORARY\s+)?TABLE\s+(\w+))",
            R"(CREATE\s+(?:DESCENDING\s+)?INDEX\s+(\w+))",
            R"(INDEX\s+\w+\s+ON\s+(\w+))",
            R"(CREATE\s+PROCEDURE\s+(\w+))",
            R"(INSERT\s+INTO\s+(\w+))",
            R"(UPDATE\s+(\w+)\s+SET)",
            R"(DELETE\s+FROM\s+(\w+))",
            R"(EXECUTE\s+PROCEDURE\s+(\w+))",
            // selectable procedures with parameters (not SUBSTRING(x FROM y))
            R"(FROM\s+(\w+)\s*\(\s*[?:])" };
        for (const char* pattern : writePatterns)
        {
            for (const auto& name : namesAfter(sql, pattern))
            {
                if (!isBenchmarkObject(name))
                {
                    std::cerr << "    not a benchmark object: " << name << "\n";
                    ownObjectsOnly = false;
                }
            }
        }
        std::regex destructive(R"(\b(DROP|ALTER|RECREATE|GRANT|REVOKE)\b)",
            std::regex::icase);
        if (std::regex_search(sql, destructive))
            noDestructiveStatement = false;
    }
    check(ownObjectsOnly, "SQL only creates, changes and calls FR_BENCHMARK_* objects");
    auto foreign = namesAfter("UPDATE CUSTOMERS SET A = 1", R"(UPDATE\s+(\w+)\s+SET)");
    check(foreign.size() == 1 && !isBenchmarkObject(foreign[0]),
        "the SQL check detects other objects");
    check(noDestructiveStatement, "SQL contains no DROP, ALTER, GRANT or REVOKE");

    // the runner draws the first ID so that the range stays inside the
    // filled rows; this needs the same number of rows as the procedure
    auto range = std::find_if(all.begin(), all.end(), [](const std::string& sql)
        { return sql.find("CREATE PROCEDURE FR_BENCHMARK_RANGE") != std::string::npos; });
    check(range != all.end() && range->find("BETWEEN :FIRST_ID AND :FIRST_ID + "
            + std::to_string(S::oltpRangeRows - 1)) != std::string::npos,
        "the OLTP range procedure counts oltpRangeRows neighbouring IDs");
}


// a SMBIOS structure: header, formatted area and strings
void addSmbios(std::vector<uint8_t>& table, uint8_t type,
    const std::vector<uint8_t>& formatted, const std::vector<std::string>& strings)
{
    table.push_back(type);
    table.push_back(uint8_t(4 + formatted.size()));
    table.push_back(0x00);
    table.push_back(0x01);
    table.insert(table.end(), formatted.begin(), formatted.end());
    for (const auto& s : strings)
    {
        table.insert(table.end(), s.begin(), s.end());
        table.push_back(0);
    }
    if (strings.empty())
        table.push_back(0);
    table.push_back(0);
}

void testSystemInfoParsers()
{
    std::cout << "System information parsers\n";
    // type 1: manufacturer and product are the strings 1 and 2
    std::vector<uint8_t> table;
    std::vector<uint8_t> system(0x19 - 4, 0);
    system[0] = 1;  // offset 0x04
    system[1] = 2;  // offset 0x05
    addSmbios(table, 1, system, { "HP", "ZBook Power G10" });
    // type 4: populated socket, 5200 MHz maximum, 2500 MHz at boot
    std::vector<uint8_t> cpu(0x2A - 4, 0);
    cpu[0x10 - 4] = 1;
    cpu[0x14 - 4] = 0x50; cpu[0x15 - 4] = 0x14;   // 5200
    cpu[0x16 - 4] = 0xC4; cpu[0x17 - 4] = 0x09;   // 2500
    cpu[0x18 - 4] = 0x41;
    addSmbios(table, 4, cpu, { "Intel Core i7" });
    // type 16: system memory with 2 slots
    std::vector<uint8_t> array(0x0F - 4, 0);
    array[0x05 - 4] = 0x03;
    array[0x0D - 4] = 2;
    addSmbios(table, 16, array, {});
    // type 17: 16 GB DDR5 SODIMM, 5600 MT/s running at 5200
    std::vector<uint8_t> module(0x28 - 4, 0);
    module[0x0C - 4] = 0x00; module[0x0D - 4] = 0x40;  // 16384 MB
    module[0x0E - 4] = 0x0D;
    module[0x10 - 4] = 1;
    module[0x12 - 4] = 0x22;
    module[0x15 - 4] = 0xE0; module[0x16 - 4] = 0x15;  // 5600
    module[0x17 - 4] = 2;
    module[0x1A - 4] = 3;
    module[0x20 - 4] = 0x50; module[0x21 - 4] = 0x14;  // 5200
    addSmbios(table, 17, module, { "DIMM 1", "Samsung", "M425R2GA3BB0" });
    // an empty slot is not a module
    std::vector<uint8_t> empty(0x28 - 4, 0);
    addSmbios(table, 17, empty, {});
    addSmbios(table, 127, {}, {});
    BenchmarkSystemInfo info;
    parseBenchmarkSmbios(table.data(), table.size(), info);
    check(info.manufacturer == "HP" && info.model == "ZBook Power G10", "SMBIOS machine");
    check(info.cpuSockets == 1 && info.cpuMaxMHz == 5200 && info.cpuBaseMHz == 2500
        && info.cpuName == "Intel Core i7", "SMBIOS processor");
    check(info.memorySlots == 2 && info.memoryModules.size() == 1, "SMBIOS slots and modules");
    if (info.memoryModules.size() == 1)
    {
        const BenchmarkMemoryModule& mm = info.memoryModules.front();
        check(mm.sizeBytes == 16LL * 1024 * 1024 * 1024 && mm.type == "DDR5"
            && mm.formFactor == "SODIMM" && mm.speedMTs == 5600
            && mm.configuredSpeedMTs == 5200 && mm.locator == "DIMM 1"
            && mm.manufacturer == "Samsung" && mm.partNumber == "M425R2GA3BB0",
            "SMBIOS memory module");
    }
    BenchmarkSystemInfo placeholders;
    std::vector<uint8_t> oem;
    addSmbios(oem, 1, system, { "To Be Filled By O.E.M.", "System Product Name" });
    parseBenchmarkSmbios(oem.data(), oem.size(), placeholders);
    check(placeholders.manufacturer.empty() && placeholders.model.empty(),
        "firmware placeholders are ignored");
    BenchmarkSystemInfo truncated;
    parseBenchmarkSmbios(table.data(), 7, truncated);
    check(truncated.model.empty(), "a truncated table is not read beyond its end");

    // NVMe health log: 39 degrees, 2 % used, 100 % spare, 58 unsafe shutdowns
    std::vector<uint8_t> log(512, 0);
    log[1] = 0x38; log[2] = 0x01;   // 312 K
    log[3] = 100;
    log[4] = 10;
    log[5] = 2;
    log[48] = 0x10; log[49] = 0x27; // 10000 units written = 5.12 GB
    log[128] = 0x4F; log[129] = 0x0A;  // 2639 hours
    log[144] = 58;
    bool warning = true;
    auto health = parseBenchmarkNvmeHealth(log.data(), log.size(), &warning);
    auto valueOf = [](const std::vector<BenchmarkHealthValue>& values, const std::string& name)
    {
        for (const auto& v : values)
        {
            if (v.name == name)
                return v.value;
        }
        return std::string();
    };
    check(!warning && valueOf(health, "Temperature").rfind("39 ", 0) == 0
        && valueOf(health, "Wear (percentage used)") == "2 %"
        && valueOf(health, "Data written") == "5.1 GB"
        && valueOf(health, "Power-on hours") == "2,639"
        && valueOf(health, "Unsafe shutdowns") == "58", "NVMe health values");
    log[0] = 0x04;  // reliability degraded
    parseBenchmarkNvmeHealth(log.data(), log.size(), &warning);
    check(warning, "an NVMe critical warning is reported");

    // ATA SMART: 3 reallocated sectors, 35 degrees
    std::vector<uint8_t> smart(512, 0);
    smart[2] = 5; smart[5] = 100; smart[7] = 3;
    smart[14] = 194; smart[17] = 65; smart[19] = 35;
    auto attributes = parseBenchmarkAtaSmart(smart.data(), smart.size(), &warning);
    check(warning && valueOf(attributes, "Reallocated sectors") == "3"
        && valueOf(attributes, "Temperature").rfind("35 ", 0) == 0, "ATA SMART attributes");

    check(getBenchmarkPowerPlanName("{8C5E7FDA-E8BF-4A96-9A85-A6E23A8C635C}") == "High performance"
        && getBenchmarkPowerPlanName("ded574b5-45a0-4f42-8737-46345c09c238") == "Best performance"
        && getBenchmarkPowerPlanName("12345678-0000-0000-0000-000000000000").empty(),
        "power plan names");
    check(getBenchmarkHypervisor("VMware, Inc.", "VMware7,1") == "VMware"
        && getBenchmarkHypervisor("Microsoft Corporation", "Virtual Machine") == "Hyper-V"
        && getBenchmarkHypervisor("Microsoft Corporation", "Surface Laptop").empty()
        && getBenchmarkHypervisor("HP", "ZBook").empty(), "hypervisors");
    check(getBenchmarkMeminfoBytes("MemTotal:       16188592 kB\nSwapTotal: 0 kB\n",
        "MemTotal") == 16188592LL * 1024
        && getBenchmarkMeminfoBytes("MemTotal: 1 kB\n", "MemAvailable") == -1, "meminfo");
    check(getBenchmarkOsReleaseValue("NAME=\"Ubuntu\"\nPRETTY_NAME=\"Ubuntu 24.04.5 LTS\"\n",
        "PRETTY_NAME") == "Ubuntu 24.04.5 LTS", "os-release");
    std::string fileSystem, options;
    const std::string mountinfo =
        "22 1 8:1 / / rw,relatime - ext4 /dev/sda1 rw\n"
        "30 22 8:2 / /var/lib/firebird rw,noatime - xfs /dev/sdb1 rw,attr2\n"
        "31 22 0:40 / /var/lib/firebird2 rw - overlay overlay rw,lowerdir=/a:/b,upperdir=/c\n";
    check(findBenchmarkMount(mountinfo, "/var/lib/firebird/data", &fileSystem, &options)
        && fileSystem == "xfs" && options == "rw,noatime,attr2", "the longest mount point wins");
    check(findBenchmarkMount(mountinfo, "/var/lib/firebird2/x", &fileSystem, &options)
        && fileSystem == "overlay" && options == "rw", "overlay layer directories are left out");
    check(findBenchmarkMount(mountinfo, "/var/lib/firebirdX", &fileSystem, nullptr)
        && fileSystem == "ext4", "a mount point matches whole directories only");
    check(getBenchmarkSelectedChoice("always [madvise] never") == "madvise"
        && getBenchmarkSelectedChoice("none") == "none", "sysfs choices");

    std::string name;
    check(matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|Protocol=6|"
        "LPort=3050|Name=Firebird|", 3050, "", &name) == 1 && name == "Firebird",
        "a port rule allows the port");
    check(matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|Protocol=6|"
        "LPort=3050-3060|Name=Range|", 3055, "", nullptr) == 1, "port ranges");
    check(matchBenchmarkFirewallRule("v2.31|Action=Block|Active=TRUE|Dir=In|"
        "App=C:\\fb\\firebird.exe|Name=Block|", 3050, "c:\\FB\\Firebird.exe", nullptr) == -1,
        "a program rule blocks");
    check(matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|"
        "App=%ProgramFiles%\\Firebird\\firebird.exe|", 3050,
        "C:\\Program Files\\Firebird\\firebird.exe",
        nullptr) == 1, "program paths with variables");
    check(matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|LPort=3051|", 3050,
        "", nullptr) == 0
        && matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=FALSE|Dir=In|", 3050, "",
            nullptr) == 0
        && matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=Out|", 3050, "",
            nullptr) == 0
        && matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|Protocol=17|", 3050,
            "", nullptr) == 0
        && matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|PFN=App.Package|",
            3050, "", nullptr) == 0, "rules for other ports, directions, protocols and apps");
    check(matchBenchmarkFirewallRule("v2.31|Action=Allow|Active=TRUE|Dir=In|"
        "Name=@FirewallAPI.dll,-28502|", 3050, "", &name) == 1
        && name == "a built-in Windows rule", "resource names are not shown");

    check(classifyBenchmarkAdapter("", "wg0", "") == "VPN"
        && classifyBenchmarkAdapter("Ethernet", "Ethernet 2",
            "PANGP Virtual Ethernet Adapter") == "VPN"
        && classifyBenchmarkAdapter("Ethernet", "vEthernet (WSL)",
            "Hyper-V Virtual Ethernet Adapter") == "virtual"
        && classifyBenchmarkAdapter("Ethernet", "Ethernet", "Intel I219-LM") == "Ethernet",
        "adapter types");

    const std::string version = std::string("\x02\x1b") + "WI-V5.0.4.1812 Firebird 5.0"
        + "\x2c" + "WI-V5.0.4.1812 Firebird 5.0/tcp (DBSRV)/P19:C";
    std::vector<wxString> list = parseBenchmarkVersionList(version);
    check(list.size() == 2 && list[0] == "WI-V5.0.4.1812 Firebird 5.0"
        && getBenchmarkServerHostName(list) == "DBSRV", "the server host name of the remote layer");
    check(getBenchmarkServerHostName({ "WI-V5.0.4.1812 Firebird 5.0" }).empty(),
        "no host name for embedded connections");
}

const BenchmarkFinding* findingIn(const BenchmarkMetrics& m, const wxString& title,
    std::vector<BenchmarkFinding>& storage)
{
    storage = analyzeBenchmark(m);
    return findFinding(storage, title);
}

void testStateAwareAdvice()
{
    using S = BenchmarkSeverity;
    std::cout << "Advice that depends on the current settings\n";
    std::vector<BenchmarkFinding> findings;

    // power saving: no advice for a setting that is already made
    BenchmarkMetrics power = healthyMetrics();
    power.clientBurstIdleMs = 25.0;
    power.system.powerPlan = "High performance";
    const BenchmarkFinding* f = findingIn(power, "slows down this computer", findings);
    check(f && f->recommendation.Contains("already set to performance")
        && !f->recommendation.Contains("powercfg"), "the power plan is already set");
    power.system.powerPlan = "Balanced, mode Best power efficiency";
    f = findingIn(power, "slows down this computer", findings);
    check(f && f->recommendation.Contains("Current setting: Balanced")
        && f->recommendation.Contains("High performance"), "the current plan is named");
    power.system.powerPlan.clear();
    f = findingIn(power, "slows down this computer", findings);
    check(f && f->recommendation.StartsWith("Tip, FlameRobin cannot check"),
        "unknown settings are only a tip");

    // virus scanners: named when active, not blamed when there is none
    BenchmarkMetrics files = healthyMetrics();
    files.tempDirFileMs = 4.0;
    files.system.antivirus = { "Windows Defender (off)" };
    f = findingIn(files, "Creating and deleting files is slow", findings);
    check(f && f->title == "Creating and deleting files is slow"
        && f->detail.Contains("No active virus scanner"),
        "no active scanner, no scanner advice");
    files.system.antivirus = { "Windows Defender (off)", "Trend Micro Security Agent (on)" };
    f = findingIn(files, "slows down file access", findings);
    check(f && f->title.Contains("virus scanner")
        && f->detail.Contains("Trend Micro Security Agent")
        && !f->detail.Contains("Windows Defender"), "the active scanner is named");
    BenchmarkMetrics excluded = healthyMetrics();
    excluded.runLocation = BenchmarkRunLocation::Server;
    excluded.connectionKind = BenchmarkConnectionKind::Embedded;
    excluded.databaseDirFileMs = 4.0;
    excluded.system.antivirus = { "Windows Defender (on)" };
    excluded.system.storagePathExcluded = true;
    f = nullptr;
    findings = analyzeBenchmark(excluded);
    for (const auto& finding : findings)
    {
        if (finding.detail.Contains("database directory"))
            f = &finding;
    }
    check(f && f->recommendation.Contains("already excluded"),
        "an existing exclusion is respected");

    // events, timeouts and keepalive; a remote event test only runs with a
    // fixed RemoteAuxPort that answers, and the advice follows the result
    // of the test, not its (translated) status text
    BenchmarkMetrics events = healthyMetrics();
    events.serverConfig["RemoteAuxPort"] = "0";
    check(!prepareBenchmarkEventTest(events)
        && events.eventResult == BenchmarkEventResult::NotTestedRandomPort,
        "no event test with a random RemoteAuxPort");
    events.eventStatus = "(translated text)";
    check(findingIn(events, "Events use a random port", findings)
        && findFinding(findings, "Events use a random port")->severity == S::Warning,
        "a random RemoteAuxPort");
    BenchmarkMetrics unknownPort = healthyMetrics();
    unknownPort.serverConfig.clear();
    unknownPort.serverMajorVersion = 3;
    check(!prepareBenchmarkEventTest(unknownPort)
        && unknownPort.eventResult == BenchmarkEventResult::NotTestedUnknownPort
        && unknownPort.eventStatus.Contains("Firebird 3")
        && !unknownPort.eventStatus.Contains("administrators"),
        "Firebird 3 cannot report RemoteAuxPort");
    f = findingIn(unknownPort, "Event delivery was not tested", findings);
    check(f && f->severity == S::Info && f->detail.Contains("Firebird 3"),
        "an unknown RemoteAuxPort is a tip");
    unknownPort.serverMajorVersion = 4;
    check(!prepareBenchmarkEventTest(unknownPort)
        && unknownPort.eventStatus.Contains("only administrators"),
        "RemoteAuxPort of Firebird 4 needs an administrator");
    events.serverConfig["RemoteAuxPort"] = "3051";
    events.serverPorts = { { 3051, BenchmarkPortCheck::State::Filtered, 0.0 } };
    events.eventStatus.clear();
    check(!prepareBenchmarkEventTest(events)
        && events.eventResult == BenchmarkEventResult::PortFiltered
        && events.eventStatus.Contains("3051"), "a filtered RemoteAuxPort");
    f = findingIn(events, "Events do not reach", findings);
    check(f && f->severity == S::Problem, "events that cannot arrive");
    events.serverPorts = { { 3051, BenchmarkPortCheck::State::Open, 1.0 } };
    events.eventResult = BenchmarkEventResult::NotRun;
    events.eventStatus.clear();
    check(prepareBenchmarkEventTest(events) && events.eventStatus.empty(),
        "events are tested with an open RemoteAuxPort");
    BenchmarkMetrics localEvents = healthyMetrics();
    localEvents.connectionKind = BenchmarkConnectionKind::Embedded;
    localEvents.serverConfig.clear();
    check(prepareBenchmarkEventTest(localEvents), "local events are always tested");
    events.eventResult = BenchmarkEventResult::NotReceived;
    events.eventStatus = "not received within 5 seconds";
    f = findingIn(events, "Events do not reach", findings);
    check(f && f->severity == S::Problem, "events that do not arrive");
    events.eventResult = BenchmarkEventResult::Received;
    events.eventStatus.clear();
    events.eventMs = 2.0;
    check(!findingIn(events, "Events", findings), "delivered events are fine");

    BenchmarkMetrics idle = healthyMetrics();
    idle.idleTimeoutSeconds = 1800;
    f = findingIn(idle, "Firebird closes unused connections", findings);
    check(f && f->detail.Contains("30 minutes"), "Firebird's idle timeout");
    idle.idleTimeoutSeconds = 0;
    check(!findingIn(idle, "closes unused connections", findings), "no idle timeout");

    BenchmarkMetrics keepAlive = healthyMetrics();
    keepAlive.system.tcpKeepAliveSeconds = 7200;
    f = findingIn(keepAlive, "cut off by firewalls", findings);
    check(f && f->severity == S::Info && f->detail.Contains("120 minutes")
        && f->recommendation.Contains("KeepAliveTime"), "a long keepalive time is a tip");
    keepAlive.system.tcpKeepAliveSeconds = 300;
    check(!findingIn(keepAlive, "cut off by firewalls", findings), "a short keepalive time");

    // network path
    BenchmarkMetrics net = healthyMetrics();
    net.latencyOutliers = 10;
    f = findingIn(net, "Network packets get lost", findings);
    check(f && f->severity == S::Warning, "lost packets");
    net.latencyOutliers = 0;
    net.wideInsertMs = 2000.0;
    net.wideInsertRows = 500;
    check(findingIn(net, "Few small writes per second", findings), "the net test limit");
    net.networkPath = BenchmarkNetworkPath();
    net.networkPath->adapterType = "Wi-Fi";
    net.networkPath->adapterName = "WLAN";
    check(findingIn(net, "over Wi-Fi", findings), "Wi-Fi is named");
    net.remoteAddress = "10.0.0.5/51234";
    net.system.addresses = { { "WLAN", "Wi-Fi", "192.168.178.58" } };
    check(findingIn(net, "A router or VPN sits between", findings), "network address translation");
    net.system.addresses = { { "WLAN", "Wi-Fi", "10.0.0.5" } };
    check(!findingIn(net, "A router or VPN sits between", findings), "the own address");

    // sorts, many users, the drive and other servers
    BenchmarkMetrics sort = healthyMetrics();
    sort.tempSortMs = 20000.0;
    f = findingIn(sort, "Sorting large data is slow", findings);
    check(f && f->recommendation.StartsWith("Tip"), "slow temporary sorts");
    BenchmarkMetrics many = healthyMetrics();
    many.oltp.push_back({ 250, 4000, 0, 5.0, 300.0, 600.0 });
    check(findingIn(many, "slows down with many users", findings), "many users");
    BenchmarkMetrics drive = healthyMetrics();
    drive.runLocation = BenchmarkRunLocation::Server;
    drive.connectionKind = BenchmarkConnectionKind::Embedded;
    drive.system.storagePath = "D:\\db";
    drive.system.storageHealthWarning = true;
    drive.system.storageHealth = { { "Media and data integrity errors", "12", true } };
    drive.system.storageMedia = "HDD";
    drive.system.firebirdServers = { { "A", "", true }, { "B", "", true } };
    findings = analyzeBenchmark(drive);
    check(findFinding(findings, "reports problems")
        && findFinding(findings, "reports problems")->severity == S::Problem
        && findFinding(findings, "reports problems")->detail.Contains("integrity errors: 12"),
        "drive health");
    check(findFinding(findings, "on a hard disk") != nullptr, "a hard disk");
    check(findFinding(findings, "2 Firebird servers run") != nullptr, "several Firebird servers");
}


void testAnonymized()
{
    std::cout << "Report without private information\n";
    BenchmarkReport report;
    report.target = "Server of CUSTOMERDB (dbserver01/3050), directory D:\\Kunde\\db\\";
    report.benchmarkDatabasePath = "D:\\Kunde\\db\\FR_BENCHMARK_20261004_102138_189ABC.FDB";
    report.benchmarkDatabaseCreated = true;
    report.benchmarkDatabaseDropped = true;
    report.intensityName = "Standard";
    report.metrics = healthyMetrics();
    BenchmarkMetrics& m = report.metrics;
    m.connectionHost = "dbserver01";
    m.serverHostName = "DBSERVER01";
    m.localHostName = "PC-MUELLER";
    m.remoteAddress = "192.168.10.77/51022";
    m.clientHostSeenByServer = "PC-MUELLER";
    m.benchmarkDirectory = "D:\\Kunde\\db\\";
    m.tempDirectory = "C:\\Users\\mueller\\AppData\\Local\\Temp";
    m.firebirdUser = "MUELLER";
    m.firebirdRole = "BUCHHALTUNG";
    m.serverConfig["TempDirectories"] = "E:\\FirebirdTemp";
    m.serverConfigExplicit.insert("TempDirectories");
    m.versionList = { "WI-V5.0.4.1812 Firebird 5.0",
        "WI-V5.0.4.1812 Firebird 5.0/tcp (DBSERVER01)/P19:C" };
    m.system.fullHostName = "PC-MUELLER.kunde.local";
    m.system.osUser = "KUNDE\\mueller";
    m.system.cpuName = "Intel Core i7-13800H";
    m.system.addresses = { { "Ethernet", "Ethernet", "192.168.10.77" } };
    m.system.firebirdServers = { { "FirebirdServerKundeX", "C:\\Program Files\\KundeX\\fb",
        true } };
    m.system.listeningPorts = { { 3050, "all addresses", "firebird.exe",
        "allowed by \"Kunde X Firebird\"" } };
    m.networkPath = BenchmarkNetworkPath();
    m.networkPath->serverHost = "dbserver01";
    m.networkPath->serverAddress = "192.168.10.5";
    m.networkPath->serverAddresses = { "192.168.10.5" };
    m.networkPath->localAddress = "192.168.10.77";
    m.networkPath->adapterType = "Ethernet";
    m.tempDirFileMs = 4.0;
    report.evaluate();

    const BenchmarkReport shared = report.anonymized();
    const wxString html = shared.toHtml();
    const wxString md = shared.toMarkdown();
    bool clean = true;
    for (const char* secret : { "dbserver01", "DBSERVER01", "PC-MUELLER", "mueller",
        "MUELLER", "kunde.local", "KUNDE", "Kunde", "192.168.10", "BUCHHALTUNG",
        "FirebirdTemp", "AppData" })
    {
        if (html.Contains(secret) || md.Contains(secret))
        {
            std::cerr << "    still contains: " << secret << "\n";
            clean = false;
        }
    }
    check(clean, "no names, addresses, users, paths, services or rule names");
    check(shared.anonymous && html.Contains("have been removed from this report")
        && md.Contains("have been removed from this report"), "the report says so");
    check(html.Contains("Intel Core i7-13800H") && html.Contains("Firebird 5.0.4")
        && html.Contains(formatBenchmarkPoints(report.score.total))
        && shared.findings.size() == report.findings.size()
        && shared.results.size() == report.results.size(),
        "hardware, versions, measurements and findings stay");
    check(shared.benchmarkDatabasePath == "FR_BENCHMARK_20261004_102138_189ABC.FDB",
        "the temporary database keeps only its file name");
    check(!report.anonymous && report.metrics.connectionHost == "dbserver01",
        "the original report is unchanged");
}


void testComparison()
{
    std::cout << "Comparison of two runs\n";
    BenchmarkReport report;
    report.intensityName = "Standard";
    report.startedAt = "2026-10-04 12:00:00";
    report.metrics = healthyMetrics();
    report.metrics.system.powerPlan = "Balanced";
    report.evaluate();
    const wxString md = report.toMarkdown();
    std::optional<BenchmarkSnapshot> earlier = parseBenchmarkSnapshot(md);
    check(earlier && earlier->date == report.startedAt && earlier->kind == "client"
        && earlier->intensity == "standard" && std::abs(earlier->score - report.score.total) < 0.1
        && earlier->categories.size() == report.score.categories.size(),
        "a saved report can be read back");
    check(earlier && std::abs(earlier->values.commitMs
        - getBenchmarkMeasuredValues(report.metrics).commitMs) < 0.001, "with its values");
    check(parseBenchmarkSnapshot(report.toHtml()).has_value(), "the HTML report as well");
    check(!parseBenchmarkSnapshot("# notes\nscore = 1\n"), "other files contain no results");
    if (!earlier)
        return;

    BenchmarkComparison same = compareBenchmarkSnapshots(*earlier, *earlier);
    check(same.differences.empty() && same.getLargestChanges(5).empty()
        && getBenchmarkChangeLevel(same.scoreRatio) == 0 && same.sameKind,
        "the same run has no changes");

    // a faster disk after a change of the power plan
    BenchmarkReport faster = report;
    faster.startedAt = "2026-10-05 09:30:00";
    faster.metrics.commitMs = *report.metrics.commitMs / 2.0;
    faster.metrics.system.powerPlan = "High performance";
    faster.earlier = earlier;
    faster.evaluate();
    BenchmarkComparison c = compareBenchmarkSnapshots(makeBenchmarkSnapshot(faster), *earlier);
    auto largest = c.getLargestChanges(5);
    check(largest.size() == 1 && largest[0]->title == "Saving a change (commit)"
        && std::abs(largest[0]->ratio - 2.0) < 0.01, "the largest change");
    check(std::any_of(c.differences.begin(), c.differences.end(),
        [](const BenchmarkComparedFact& f)
        { return f.label == "Power plan" && f.newerText.Contains("High performance")
            && f.olderText.Contains("Balanced"); }), "what is different");
    check(formatBenchmarkChange(2.0) == "2.0 times as fast"
        && formatBenchmarkChange(1.25) == "25 % faster"
        && formatBenchmarkChange(0.7) == "30 % slower"
        && formatBenchmarkChange(1.05) == "about the same"
        && formatBenchmarkChange(0.25) == "4.0 times slower", "changes in words");
    check(getBenchmarkChangeLevel(1.1) == 0 && getBenchmarkChangeLevel(1.2) == 1
        && getBenchmarkChangeLevel(0.6) == 2, "small changes are normal variation");
    check(std::abs(faster.score.total - computeBenchmarkScore(faster.metrics).total) < 0.001,
        "a comparison does not change the score");

    const wxString html = faster.toHtml();
    const wxString fasterMd = faster.toMarkdown();
    check(html.Contains("Compared with the earlier run")
        && html.Contains("<span class=\"chg up2\">2.0 times as fast</span>")
        && html.Contains("earlier 1.50 ms, 2.0 times as fast")
        && html.Contains("<th class=\"n\">Earlier run</th><th class=\"n\">This run</th>")
        && html.Contains("<span class=\"marker earlier\""),
        "the HTML report shows the comparison");
    check(html.Find("id=\"differences\"") < html.Find("<div class=\"details\">"),
        "the comparison is the overview");
    check(fasterMd.Contains("### Compared with ") && fasterMd.Contains("**2.0 times as fast**")
        && fasterMd.Find("### Compared with ") < fasterMd.Find("### Results"),
        "the Markdown report as well");

    // two saved reports
    BenchmarkComparison files = compareBenchmarkSnapshots(
        *parseBenchmarkSnapshot(fasterMd), *earlier);
    check(files.toHtml().Contains("Comparison of two benchmark runs")
        && files.toMarkdown().Contains("| **Score** |")
        && files.toMarkdown().Contains("Later run")
        && files.toMarkdown().Contains("| Writing many rows | "), "two saved reports");
    check(std::any_of(files.stats.begin(), files.stats.end(),
        [](const BenchmarkComparedValue& v) { return v.title == "Changing many rows"; })
        && std::none_of(files.stats.begin(), files.stats.end(),
            [](const BenchmarkComparedValue& v)
            { return v.title == "Saving a small change (commit)"; }),
        "every other measurement is compared once");

    // problems that are gone and new ones
    BenchmarkSnapshot withProblem = *earlier;
    withProblem.findings.push_back({ BenchmarkSeverity::Problem, BenchmarkCategory::ServerStorage,
        "Something is slow" });
    BenchmarkComparison solved = compareBenchmarkSnapshots(*earlier, withProblem);
    check(solved.solved.size() == 1 && solved.added.empty(), "solved problems");

    BenchmarkSnapshot client = *earlier;
    client.title = "Server dbserver01";
    client.host = "PC-MUELLER";
    client.facts.push_back({ "conf.TempDirectories", "E:\\Temp" });
    const BenchmarkSnapshot hidden = anonymizeBenchmarkSnapshot(client);
    check(hidden.host.empty() && hidden.title == "Firebird server"
        && std::none_of(hidden.facts.begin(), hidden.facts.end(),
            [](const auto& f) { return f.second.Contains("E:\\Temp"); }),
        "a shared comparison names no computer, server or path");
    BenchmarkReport sharedReport = faster;
    sharedReport.earlier = client;
    const wxString sharedHtml = sharedReport.anonymized().toHtml();
    check(!sharedHtml.Contains("PC-MUELLER") && !sharedHtml.Contains("E:\\Temp"),
        "the shared report hides the earlier run as well");
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

    testHelpers();
    testSafetyCheck();
    testPendingDatabases();
    testHealthy();
    testRules();
    testChecklist();
    testReport();
    testScore();
    testHtml();
    testFindingSections();
    testSql();
    testSystemInfoParsers();
    testStateAwareAdvice();
    testAnonymized();
    testComparison();

    std::cout << (ok ? "ALL BENCHMARK TESTS PASSED\n" : "SOME BENCHMARK TESTS FAILED\n");
    return ok ? 0 : 1;
}
