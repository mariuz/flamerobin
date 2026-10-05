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
#include <optional>
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

const BenchmarkHint* findHint(const std::vector<BenchmarkHint>& hints,
    const wxString& titlePart, std::optional<BenchmarkCategory> category = std::nullopt)
{
    auto it = std::find_if(hints.begin(), hints.end(), [&](const BenchmarkHint& h)
        {
            return h.title.Contains(titlePart) && (!category || h.category == *category);
        });
    return it == hints.end() ? nullptr : &*it;
}

// checks that the run has a hint for the part whose title contains
// titlePart, and optionally a text in its advice
void expectHint(const BenchmarkMetrics& m, const wxString& titlePart,
    BenchmarkCategory category, const char* name, const wxString& advicePart = wxEmptyString)
{
    const auto hints = findBenchmarkHints(m);
    const BenchmarkHint* h = findHint(hints, titlePart, category);
    check(h && (advicePart.empty() || h->advice.Contains(advicePart)), name);
}

void expectNoHint(const BenchmarkMetrics& m, const wxString& titlePart, const char* name)
{
    check(findHint(findBenchmarkHints(m), titlePart) == nullptr, name);
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

// a run on the server itself with the embedded engine
BenchmarkMetrics serverMetrics()
{
    BenchmarkMetrics m = healthyMetrics();
    m.runLocation = BenchmarkRunLocation::Server;
    m.connectionKind = BenchmarkConnectionKind::Embedded;
    m.connectionHost.clear();
    m.latencyMedianMs = 0.0018;
    m.latencyAvgMs = 0.002;
    m.latencyP95Ms = 0.003;
    m.latencySparseMs = 0.002;
    return m;
}

void testHints()
{
    using C = BenchmarkCategory;
    std::cout << "Hints: what could slow down a part\n";
    check(findBenchmarkHints(healthyMetrics()).empty(), "nothing is noticed on a healthy system");
    check(findBenchmarkHints(BenchmarkMetrics()).empty(), "nothing is noticed without values");

    // power
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.clientBurstIdleMs = 25.0;
        expectHint(m, "Power saving slows down this computer", C::LocalMachine,
            "power saving of a Windows client", "powercfg");
        expectNoHint(m, "slows down the server", "a client run does not blame the server");
        m.system.powerPlan = "High performance";
        expectHint(m, "Power saving slows down this computer", C::LocalMachine,
            "the power plan is already set", "already set to performance");
        m.clientBurstIdleMs = 10.0;
        expectNoHint(m, "Power saving", "no power saving with the same bursts");
        expectNoHint(m, "power plan", "a power plan set to performance");
        m.system.powerPlan = "Balanced, mode Best power efficiency";
        expectHint(m, "The power plan saves energy", C::LocalMachine,
            "a power plan that saves energy", "High performance");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        expectNoHint(m, "runs on battery", "unknown power supply");
        m.localOnBattery = false;
        expectNoHint(m, "runs on battery", "mains power");
        m.localOnBattery = true;
        expectHint(m, "runs on battery", C::LocalMachine, "a client on battery", "mains power");
        check(!findHint(findBenchmarkHints(m), "runs on battery", C::ServerCpu),
            "the battery of a client is not the server's");
        m = serverMetrics();
        m.localOnBattery = true;
        const auto hints = findBenchmarkHints(m);
        const BenchmarkHint* battery = findHint(hints, "runs on battery", C::ServerCpu);
        check(battery && battery->detail.Contains("database server included"),
            "on the server the battery slows down the server as well");
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
        expectHint(m, "Power saving slows down the server", C::ServerCpu,
            "power saving of a Linux server", "governor");
        m.serverBurstHotMs = 5.0;
        m.serverBurstIdleMs = 12.0;
        expectNoHint(m, "slows down the server", "too short bursts are not compared");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.latencySparseMs = 1.2;
        expectHint(m, "after a short pause is slow", C::Network, "slow sparse queries",
            "Energy Efficient Ethernet");
        m.latencyMedianMs = 0.08;
        m.latencySparseMs = 0.3;
        expectNoHint(m, "after a short pause", "tiny differences are ignored");
    }

    // virus scanners: named when active, nothing without an active one
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.system.antivirus = { "Windows Defender (off)" };
        expectNoHint(m, "virus scanner", "no active scanner");
        m.system.antivirus = { "Windows Defender (off)", "Trend Micro Security Agent (on)" };
        const auto hints = findBenchmarkHints(m);
        const BenchmarkHint* h = findHint(hints, "virus scanner", C::LocalMachine);
        check(h && h->detail.Contains("Trend Micro Security Agent")
            && !h->detail.Contains("Windows Defender"), "the active scanner is named");
        check(!findHint(hints, "virus scanner", C::ServerStorage),
            "the scanners of a client say nothing about the server");
        m = serverMetrics();
        m.system.antivirus = { "Windows Defender (on)" };
        m.databaseDirFileMs = 4.0;
        expectHint(m, "may check the database files", C::ServerStorage,
            "a scanner on the database server", "administrator rights");
        m.system.storagePathExcluded = false;
        expectHint(m, "may check the database files", C::ServerStorage,
            "a database directory that is not excluded", "not excluded");
        m.system.storagePathExcluded = true;
        expectNoHint(m, "may check the database files", "an existing exclusion is respected");
    }

    // processor
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.parallelCpuMs = 280.0;   // 4 connections, no scaling at all
        expectHint(m, "does not use all its processor cores", C::ServerCpu,
            "missing scaling", "vCPU");
        m.serverConfig["CpuAffinityMask"] = "1";
        expectHint(m, "does not use all its processor cores", C::ServerCpu,
            "missing scaling with a mask", "CpuAffinityMask");
        m = serverMetrics();
        m.system.virtualization = "VMware";
        m.system.firebirdServers = { { "A", "", true }, { "B", "", true } };
        m.system.otherDatabaseServers = { { "MSSQLSERVER", "", true } };
        const auto hints = findBenchmarkHints(m);
        check(findHint(hints, "virtual machine (VMware)", C::ServerCpu)
            && findHint(hints, "virtual machine (VMware)", C::ServerStorage),
            "a virtual machine shares CPU and storage");
        check(findHint(hints, "2 Firebird servers run", C::ServerCpu)
            && findHint(hints, "Other database servers", C::ServerCpu), "other servers");
        BenchmarkMetrics client = healthyMetrics();
        client.system.virtualization = "VMware";
        expectNoHint(client, "virtual machine", "a virtual client is not the server");
    }

    // storage
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.commitMs = 20.0;
        m.osSyncWriteMs = 0.5;
        expectHint(m, "The disk is fast, saving a change takes longer", C::ServerStorage,
            "a commit much slower than the disk", "Defender");
        m.osSyncWriteMs = 18.0;
        expectNoHint(m, "The disk is fast", "a slow disk explains a slow commit");
        m.tempSortMs = 20000.0;
        const auto hints = findBenchmarkHints(m);
        const BenchmarkHint* sort = findHint(hints, "Sorting large data", C::ServerStorage);
        check(sort && sort->advice.StartsWith("Tip"), "large sorts much slower than in memory");
    }
    {   // scope
        BenchmarkMetrics m = serverMetrics();
        m.system.storagePath = "D:\\db";
        m.system.storageHealthWarning = true;
        m.system.storageHealth = { { "Media and data integrity errors", "12", true } };
        m.system.storageMedia = "HDD";
        m.system.writeCacheEnabled = false;
        m.databaseDiskFreeBytes = int64_t(1) * 1024 * 1024 * 1024;
        const auto hints = findBenchmarkHints(m);
        const BenchmarkHint* health = findHint(hints, "reports problems", C::ServerStorage);
        check(health && health->detail.Contains("integrity errors: 12"), "drive health");
        check(findHint(hints, "on a hard disk", C::ServerStorage) != nullptr, "a hard disk");
        check(findHint(hints, "write cache of the disk is off", C::ServerStorage) != nullptr,
            "the write cache is off");
        check(findHint(hints, "Little free disk space", C::ServerStorage) != nullptr,
            "little free space");
        m.system.writeCachePowerProtected = true;
        expectNoHint(m, "write cache", "a protected write cache");
    }

    // memory
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.benchmarkDatabase.cachePages = 2048;   // 16 MB < 50 MB test data
        expectHint(m, "small page cache", C::ServerCache, "a small page cache",
            "DefaultDbCachePages");
        m.serverConfig["ServerMode"] = "Classic";
        expectNoHint(m, "small page cache", "Classic has a cache per connection");
        expectHint(m, "Server mode Classic", C::ServerCache, "Classic: memory", "SuperServer");
        expectHint(m, "Server mode Classic", C::MultiUser, "Classic: many users");
        BenchmarkMetrics processes = healthyMetrics();
        processes.serverProcesses = 4;
        expectHint(processes, "Server mode Classic", C::ServerCache,
            "Classic detected by its processes");
    }
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.coldScanMs = 2000.0;
        m.warmScanMs = 100.0;
        expectHint(m, "not in memory is read much more slowly", C::ServerCache,
            "data that is not cached", "RAM");
        m.serverSortMs = 800.0;
        m.serverConfig["TempCacheLimit"] = "67108864";
        expectHint(m, "Sorting takes much longer than reading", C::ServerCache,
            "sorting compared with reading", "67108864");
    }

    // network
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.latencyP95Ms = 2.0;
        expectHint(m, "Response times vary a lot", C::Network, "jitter");
        m.streamClientMs = 500.0;
        expectHint(m, "FlameRobin needs most of the time", C::LocalMachine,
            "this computer processes the data slowly");
        m.clientVersion = "WI-V3.0.10.33601 Firebird 3.0";
        expectHint(m, "old client library", C::Network, "an old client library");
        m.latencyOutliers = 10;
        expectHint(m, "Network packets get lost", C::Network, "lost packets");
        m.networkPath = BenchmarkNetworkPath();
        m.networkPath->adapterType = "Wi-Fi";
        m.networkPath->adapterName = "WLAN";
        m.networkPath->linkSpeedBitsPerSecond = 1000000000;
        m.streamMs = 10000.0;   // 5 MB/s on a gigabit link
        m.serverConfig["TcpRemoteBufferSize"] = "8192";
        expectHint(m, "over Wi-Fi", C::Network, "Wi-Fi is named");
        expectHint(m, "only part of the network speed", C::Network,
            "the transfer compared with the link speed", "TcpRemoteBufferSize (currently 8192)");
        m.remoteAddress = "10.0.0.5/51234";
        m.system.addresses = { { "WLAN", "Wi-Fi", "192.168.178.58" } };
        expectHint(m, "A router or VPN sits between", C::Network, "network address translation");
        m.system.addresses = { { "WLAN", "Wi-Fi", "10.0.0.5" } };
        expectNoHint(m, "A router or VPN sits between", "the own address");
        m.serverPorts = { { 3050, BenchmarkPortCheck::State::Open, 2.0 } };
        m.serverPort = 3050;
        m.connectMs = 400.0;
        expectHint(m, "Logging in takes most of the connection time", C::Network,
            "the login takes the time", "AuthServer");
        BenchmarkMetrics embedded = serverMetrics();
        embedded.clientVersion = "WI-V3.0.10.33601 Firebird 3.0";
        expectNoHint(embedded, "old client library", "the embedded engine has no network");
    }

    // several users
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.oltp.back().transactions = 6000;   // 8 clients barely faster
        m.oltp.back().conflicts = 400;
        expectHint(m, "do not get more work done", C::MultiUser, "no scaling", "LockHashSlots");
        expectHint(m, "update conflicts", C::MultiUser, "update conflicts", "sequences");
        BenchmarkMetrics many = healthyMetrics();
        many.oltp.push_back({ 250, 4000, 0, 5.0, 300.0, 600.0 });
        expectHint(many, "slows down with many users", C::MultiUser, "many users");
    }
}

void testEventTest()
{
    std::cout << "Event test\n";
    // a remote event test only runs with a fixed RemoteAuxPort that answers
    BenchmarkMetrics events = healthyMetrics();
    events.serverConfig["RemoteAuxPort"] = "0";
    check(!prepareBenchmarkEventTest(events)
        && events.eventResult == BenchmarkEventResult::NotTestedRandomPort,
        "no event test with a random RemoteAuxPort");
    BenchmarkMetrics unknownPort = healthyMetrics();
    unknownPort.serverConfig.clear();
    unknownPort.serverMajorVersion = 3;
    check(!prepareBenchmarkEventTest(unknownPort)
        && unknownPort.eventResult == BenchmarkEventResult::NotTestedUnknownPort
        && unknownPort.eventStatus.Contains("Firebird 3")
        && !unknownPort.eventStatus.Contains("administrators"),
        "Firebird 3 cannot report RemoteAuxPort");
    unknownPort.serverMajorVersion = 4;
    check(!prepareBenchmarkEventTest(unknownPort)
        && unknownPort.eventStatus.Contains("only administrators"),
        "RemoteAuxPort of Firebird 4 needs an administrator");
    events.serverConfig["RemoteAuxPort"] = "3051";
    events.serverPorts = { { 3051, BenchmarkPortCheck::State::Filtered, 0.0 } };
    check(!prepareBenchmarkEventTest(events)
        && events.eventResult == BenchmarkEventResult::PortFiltered
        && events.eventStatus.Contains("3051"), "a filtered RemoteAuxPort");
    events.serverPorts = { { 3051, BenchmarkPortCheck::State::Open, 1.0 } };
    events.eventResult = BenchmarkEventResult::NotRun;
    events.eventStatus.clear();
    check(prepareBenchmarkEventTest(events) && events.eventStatus.empty(),
        "events are tested with an open RemoteAuxPort");
    BenchmarkMetrics localEvents = healthyMetrics();
    localEvents.connectionKind = BenchmarkConnectionKind::Embedded;
    localEvents.serverConfig.clear();
    check(prepareBenchmarkEventTest(localEvents), "local events are always tested");
}

void testSettings()
{
    std::cout << "Settings\n";
    auto find = [](const std::vector<BenchmarkSetting>& list,
        const wxString& name) -> const BenchmarkSetting*
    {
        auto it = std::find_if(list.begin(), list.end(),
            [&](const BenchmarkSetting& s) { return s.name == name; });
        return it == list.end() ? nullptr : &*it;
    };

    BenchmarkMetrics m = healthyMetrics();
    m.serverConfig["DefaultDbCachePages"] = "2048";
    m.serverConfig["MaxStatementCacheSize"] = "2097152";
    auto list = getBenchmarkSettings(m);
    const BenchmarkSetting* cache = find(list, "DefaultDbCachePages");
    check(cache && cache->value == "2048" && cache->scope == "firebird.conf"
        && !cache->meaning.empty(), "a setting with its value and what it does");
    const BenchmarkSetting* mode = find(list, "ServerMode");
    check(mode && mode->value == "?", "unknown values are shown as ?");
    check(find(list, "MaxStatementCacheSize") && find(list, "MaxStatementCacheSize")->value
        == "2097152", "the statement cache of Firebird 5");
    check(std::all_of(list.begin(), list.end(), [](const BenchmarkSetting& s)
        { return !s.meaning.empty(); }), "every setting is explained");
    m.serverMajorVersion = 4;
    check(find(getBenchmarkSettings(m), "MaxParallelWorkers") == nullptr,
        "no parallel workers before Firebird 5");
    check(find(list, "Forced writes") == nullptr, "no database settings without a database");

    BenchmarkDatabaseInfo selected;
    selected.pageSize = 8192;
    selected.forcedWrites = false;
    selected.lingerSeconds = 0;
    m.inspectedDatabase = selected;
    list = getBenchmarkSettings(m);
    check(find(list, "Forced writes") && find(list, "Forced writes")->value == "off"
        && find(list, "Page size")->value == "8192", "the settings of the selected database");
    check(find(list, "LINGER") && find(list, "LINGER")->value == "0", "LINGER");
}

void testValueAdvice()
{
    std::cout << "Typical causes of slower values\n";
    const BenchmarkValues values = getBenchmarkValues(healthyMetrics());
    bool all = true;
    for (const auto& v : formatBenchmarkValues(values))
    {
        if (getBenchmarkValueAdvice(v.first).empty())
        {
            std::cerr << "    no advice for " << v.first.utf8_str() << "\n";
            all = false;
        }
    }
    check(all, "every key value has its typical causes");
    check(getBenchmarkValueAdvice("commitMs").Contains("disk"), "commits wait for the disk");
    check(getBenchmarkValueAdvice("unknownKey").empty(), "unknown keys have no advice");
}

void testParts()
{
    std::cout << "Key values by part\n";
    using C = BenchmarkCategory;
    const BenchmarkMetrics m = healthyMetrics();
    const std::vector<BenchmarkPart> parts = getBenchmarkParts(m);
    auto part = [&parts](C category) -> const BenchmarkPart*
    {
        auto it = std::find_if(parts.begin(), parts.end(),
            [category](const BenchmarkPart& p) { return p.category == category; });
        return it == parts.end() ? nullptr : &*it;
    };
    check(parts.size() == 6, "six parts with key values on a client");
    check(parts.front().category == C::ServerCpu && parts.back().category == C::LocalMachine,
        "in the order of the report");
    check(part(C::ServerStorage) && part(C::ServerStorage)->values.size() == 5,
        "storage: commit, insert, drive test, parallel drive test, temporary files");
    check(part(C::Network) && part(C::Network)->values.size() == 4,
        "network: round trip, transfer, connect, small writes");
    check(part(C::LocalMachine) && part(C::LocalMachine)->values.size() == 1,
        "this computer: the short calculation");
    const BenchmarkKeyValue& commit = part(C::ServerStorage)->values.front();
    check(commit.key == "commitMs" && commit.valueText == "1.50 ms"
        && formatBenchmarkWork(commit.amountText, commit.durationMs).StartsWith("200 commits in "),
        "a key value with the work behind it");
    check(part(C::ServerCpu)->machine.Contains("dbserver")
        && part(C::LocalMachine)->machine.Contains("workstation"), "the machine of a part");

    BenchmarkMetrics embedded = serverMetrics();
    const std::vector<BenchmarkPart> embeddedParts = getBenchmarkParts(embedded);
    check(std::none_of(embeddedParts.begin(), embeddedParts.end(),
        [](const BenchmarkPart& p) { return p.category == BenchmarkCategory::Network; })
        && getBenchmarkValues(embedded).roundTripMs == 0.0,
        "the embedded engine has no network values");

    BenchmarkMetrics notApplied = m;
    notApplied.smallCachePages = 2048;
    for (const auto& p : getBenchmarkParts(notApplied))
    {
        if (p.category == C::ServerStorage)
            check(p.values.size() == 3, "no drive test values without the minimal cache");
    }
    check(getBenchmarkParts(BenchmarkMetrics()).empty(), "nothing measured, no parts");

    BenchmarkValues parsed{};
    check(parseBenchmarkValue(parsed, "commitMs", "1.5") && parsed.commitMs == 1.5
        && !parseBenchmarkValue(parsed, "nonsense", "1"), "key values are read back by key");
    check(getBenchmarkValueKey("Saving a change (commit)") == "commitMs",
        "the key of a key value");
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
    report.metrics.clientBurstIdleMs = 25.0;
    report.evaluate();

    check(!report.results.empty() && report.parts.size() == 6 && !report.settings.empty()
        && !report.hints.empty(), "results, parts, hints and settings are built");
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
    check(std::count_if(report.results.begin(), report.results.end(),
        [](const BenchmarkResult& r) { return !r.valueKey.empty(); }) == 14,
        "every key value but the best of the OLTP levels has the result it comes from");
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
        auto results = buildBenchmarkResults(serverMetrics());
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

    // the Markdown report has the values only, without judging them
    const wxString md = report.toMarkdown();
    const wxString visible = md.Left(md.Find("<!-- FlameRobin benchmark results"));
    check(md.Find("## Performance Benchmark & Diagnosis") == 0
        && md.Find("### Key values") < md.Find("### System")
        && md.Find("### System") < md.Find("### All measured values")
        && md.Find("### All measured values") < md.Find("<!-- FlameRobin benchmark results"),
        "key values, system, all measured values, then the results for comparisons");
    check(md.Contains("| Part | Value | Result | Work |") && md.Contains("200 commits in "),
        "the key values with the work behind them");
    check(!visible.Contains("Points") && !visible.Contains("What could make")
        && !visible.Contains("Power saving slows"), "a single run is not judged");
    check(md.Contains("value.commitMs = ") && md.Contains("hint = computer | Power saving"),
        "the snapshot keeps the values and the hints for a comparison");
    {   // scope
        BenchmarkReport local;
        local.metrics = serverMetrics();
        local.evaluate();
        check(local.toMarkdown().Contains("0.0018 ms"), "microseconds are not shown as 0.00 ms");
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

void testHtml()
{
    std::cout << "HTML report\n";
    BenchmarkReport report;
    report.target = "Server <b>x</b> & \"y\"";
    report.benchmarkDatabasePath = "/var/db/FR_BENCHMARK_X.FDB";
    report.benchmarkDatabaseCreated = true;
    report.benchmarkDatabaseDropped = true;
    report.metrics = healthyMetrics();
    report.metrics.clientBurstIdleMs = 25.0;
    report.metrics.pageSizes = { { 8192, 2048, 1.0, 900.0, 50.0, 40.0, 3500.0 },
                                 { 16384, 2048, 1.0, 1000.0, 30.0, 45.0, 3800.0 } };
    report.evaluate();
    wxString html = report.toHtml("dark");
    const wxString page = html.Left(html.Find("<!-- FlameRobin benchmark results"));
    check(html.StartsWith("<!doctype html>") && html.Contains("</html>"), "complete page");
    check(html.Contains("data-theme=\"dark\""), "theme of FlameRobin");
    check(report.toHtml().Contains("<html lang=\"en\">"),
        "saved page follows the system theme");
    check(html.Contains("Server &lt;b&gt;x&lt;/b&gt; &amp; &quot;y&quot;")
        && !html.Contains("<b>x</b>"), "text is escaped");
    check(html.Find("<nav class=\"pages\">") < html.Find("<div class=\"overview\">")
        && html.Find("<div class=\"overview\">") < html.Find("id=\"intro\"")
        && html.Find("id=\"intro\"") < html.Find("id=\"key-values\"")
        && html.Find("id=\"key-values\"") < html.Find("id=\"parts\"")
        && html.Find("id=\"parts\"") < html.Find("<div class=\"details\">")
        && html.Find("<div class=\"details\">") < html.Find("id=\"system\"")
        && html.Find("id=\"system\"") < html.Find("id=\"categories\"")
        && html.Find("id=\"categories\"") < html.Find("id=\"values\""),
        "the overview, then the detailed report");
    check(html.Contains("id=\"v1\" checked>")
        && html.Find("\" checked") == html.Find("id=\"v1\" checked") + 6,
        "the overview is shown first");
    check(!html.Contains("<details open") && !html.Contains("details open"),
        "all details and panels are closed at first");
    check(html.Contains("#v1:checked ~ .details, #v2:checked ~ .overview { display: none; }")
        && html.Contains("<label for=\"v1\">Overview</label> | <label for=\"v2\">Detailed "
            "report</label>"),
        "the views switch without a script");

    // a single run shows its values without judging them
    check(page.Contains("a single run is not judged")
        && page.Contains("<summary class=\"row plain\">")
        && page.Contains("<th>Test</th><th class=\"n\">Result</th>")
        && page.Contains("200 commits in ")
        && page.Contains("<tr class=\"group\"><td colspan=\"3\">Server disk"),
        "the key values at a glance and the parts with all their values");
    bool unjudged = true;
    for (const char* judgement : { "Power saving slows", "What could make",
        "<span class=\"chg", "<span class=\"points-num" })
    {
        if (page.Contains(judgement))
        {
            std::cerr << "    the page contains: " << judgement << "\n";
            unjudged = false;
        }
    }
    check(unjudged, "no points, no changes and no reasons in a single run");
    check(html.Contains("hint = computer | Power saving"),
        "the hints are kept for a comparison");

    // one check box, one tile and one panel per part with values
    int toggles = 0, tiles = 0, panels = 0;
    const int sections = int(getBenchmarkReportSections().size());
    for (int i = 0; i < sections; ++i)
    {
        toggles += html.Contains(wxString::Format("<input type=\"checkbox\" class=\"toggle\" "
            "id=\"c%d\">", i));
        tiles += html.Contains(wxString::Format("<label class=\"tile t%d\" for=\"c%d\">", i, i));
        panels += html.Contains(wxString::Format("<div class=\"panel p%d\">", i))
            && html.Contains(wxString::Format("#c%d:checked~.tiles .p%d{display:block}", i, i));
    }
    check(toggles == sections && tiles == sections && panels == sections,
        "one tile, one panel and one toggle per part");
    check(html.Contains(".cats > .toggle { position: fixed;"),
        "the toggles do not scroll the page when a tile is clicked");
    check(html.Contains("Server disk") && html.Contains("Settings")
        && html.Contains("Programming patterns"), "every part");
    check(html.Contains("<code>DefaultDbCachePages</code>") && html.Contains("What it does"),
        "the settings with what they do");
    check(html.Contains("id=\"glossary\"") && html.Contains("id=\"values\"")
        && html.Contains("id=\"run\""),
        "background information at the end");
    // tooltips for units and tests
    check(html.Contains("<span class=\"tip-box\" role=\"tooltip\">Transactions per second"),
        "units are explained in tooltips");
    check(html.Contains("Pure CPU work of one connection inside Firebird.</span></span>"),
        "tests are explained in tooltips");
    check(html.Contains("<dt>OLTP</dt>") && html.Contains("<dt>Key value</dt>")
        && html.Contains("<dt>Points</dt>") && html.Contains("<dt>Calculation, one "
        "connection</dt>"), "the glossary explains terms and tests");
    check(!html.Contains("http://") && !html.Contains("https://")
        && !html.Contains("<script"), "no external resources and no scripts");
    // SVG coordinates and CSS widths must not use a decimal comma
    check(!html.Contains("width:0,") && !html.Contains("y=\"0,")
        && !html.Contains("% \""), "locale independent numbers");
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
    m.system.addresses = { { "Ethernet", "Ethernet", "192.168.10.70" } };
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
    check(findHint(report.hints, "A router or VPN sits between") != nullptr,
        "the test has a hint that names an address");

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
        && shared.hints.size() == report.hints.size()
        && shared.parts.size() == report.parts.size()
        && shared.results.size() == report.results.size(),
        "hardware, versions, measurements and hints stay");
    check(shared.benchmarkDatabasePath == "FR_BENCHMARK_20261004_102138_189ABC.FDB",
        "the temporary database keeps only its file name");
    check(!report.anonymous && report.metrics.connectionHost == "dbserver01",
        "the original report is unchanged");
}

void testComparison()
{
    std::cout << "Comparison of two runs\n";
    using C = BenchmarkCategory;
    // a server with a slow disk and an active virus scanner
    BenchmarkReport report;
    report.intensityName = "Standard";
    report.startedAt = "2026-10-04 12:00:00";
    report.metrics = serverMetrics();
    report.metrics.system.powerPlan = "Balanced";
    report.metrics.system.antivirus = { "Windows Defender (on)" };
    report.metrics.system.storagePathExcluded = false;
    report.evaluate();
    const wxString md = report.toMarkdown();
    std::optional<BenchmarkSnapshot> earlier = parseBenchmarkSnapshot(md);
    check(earlier && earlier->date == report.startedAt && earlier->kind == "embedded"
        && earlier->intensity == "standard" && earlier->hints.size() == report.hints.size()
        && !earlier->hints.empty(), "a saved report can be read back");
    check(earlier && std::abs(earlier->values.commitMs
        - getBenchmarkValues(report.metrics).commitMs) < 0.001, "with its values");
    check(parseBenchmarkSnapshot(report.toHtml()).has_value(), "the HTML report as well");
    check(!parseBenchmarkSnapshot("# notes\nvalue.commitMs = 1\n"),
        "other files contain no results");
    if (!earlier)
        return;

    BenchmarkComparison same = compareBenchmarkSnapshots(*earlier, *earlier);
    check(same.differences.empty() && same.getLargestChanges(5).empty()
        && std::abs(same.ratio - 1.0) < 0.001 && same.sameKind,
        "the same run has no changes");
    check(std::all_of(same.parts.begin(), same.parts.end(), [](const BenchmarkComparedPart& p)
        { return p.hints.empty() && p.advice.empty() && p.differences.empty(); }),
        "no reasons without a difference");

    // the disk got twice as fast after the virus scanner was switched off
    BenchmarkReport faster = report;
    faster.startedAt = "2026-10-05 09:30:00";
    BenchmarkMetrics& f = faster.metrics;
    f.commitMs = *report.metrics.commitMs / 2.0;
    f.insertMs = *report.metrics.insertMs / 2.0;
    f.smallCacheInsertMs = *report.metrics.smallCacheInsertMs / 2.0;
    f.smallCacheLookupMs = *report.metrics.smallCacheLookupMs / 2.0;
    f.smallCacheUpdateMs = *report.metrics.smallCacheUpdateMs / 2.0;
    f.smallCacheDeleteMs = *report.metrics.smallCacheDeleteMs / 2.0;
    f.parallelDriveMs = *report.metrics.parallelDriveMs / 2.0;
    f.tempSortMs = *report.metrics.tempSortMs / 2.0;
    f.system.antivirus = { "Windows Defender (off)" };
    faster.earlier = earlier;
    faster.evaluate();
    BenchmarkComparison c = compareBenchmarkSnapshots(makeBenchmarkSnapshot(faster), *earlier);
    const BenchmarkComparedPart* storage = c.findPart(C::ServerStorage);
    check(storage && std::abs(storage->ratio - 2.0) < 0.01
        && formatBenchmarkPoints(BenchmarkComparison::earlierPoints * storage->ratio) == "2,000",
        "twice as fast gives 2000 points against the 1000 of the earlier run");
    check(c.findPart(C::ServerCpu) && std::abs(c.findPart(C::ServerCpu)->ratio - 1.0) < 0.001
        && c.ratio > 1.0 && c.ratio < storage->ratio, "the total is the mean of the parts");
    check(storage && std::any_of(storage->differences.begin(), storage->differences.end(),
        [](const BenchmarkComparedFact& d) { return d.key == "scanner"; })
        && std::any_of(storage->hints.begin(), storage->hints.end(),
            [](const BenchmarkHint& h) { return h.title.Contains("virus scanner"); })
        && std::any_of(storage->advice.begin(), storage->advice.end(),
            [](const auto& a) { return a.first == "Saving a change (commit)"; }),
        "the reasons of the slower earlier run: a difference, a hint and typical causes");
    check(storage && getBenchmarkReasonsTitle(c, *storage)
        == "What could make the earlier run slower", "the earlier run was slower");
    auto largest = c.getLargestChanges(5);
    check(largest.size() == 5 && std::abs(largest[0]->ratio - 2.0) < 0.01, "the largest changes");
    check(std::any_of(c.differences.begin(), c.differences.end(),
        [](const BenchmarkComparedFact& d)
        { return d.label == "Virus scanner" && d.newerText.Contains("off")
            && d.olderText.Contains("(on)"); }), "what is different");
    check(formatBenchmarkChange(2.0) == "2.0 times as fast"
        && formatBenchmarkChange(1.25) == "25 % faster"
        && formatBenchmarkChange(0.7) == "30 % slower"
        && formatBenchmarkChange(1.05) == "about the same"
        && formatBenchmarkChange(0.25) == "4.0 times slower", "changes in words");
    check(getBenchmarkChangeLevel(1.1) == 0 && getBenchmarkChangeLevel(1.2) == 1
        && getBenchmarkChangeLevel(0.6) == 2, "small changes are normal variation");

    // the newer run slower: the reasons are those of the newer run
    BenchmarkComparison slower = compareBenchmarkSnapshots(*earlier,
        makeBenchmarkSnapshot(faster));
    const BenchmarkComparedPart* slowerStorage = slower.findPart(C::ServerStorage);
    check(slowerStorage && slowerStorage->ratio < 1.0
        && getBenchmarkReasonsTitle(slower, *slowerStorage)
            == "What could make the later run slower"
        && !slowerStorage->hints.empty(), "the later run was slower");
    slower.newerIsCurrentRun = true;
    check(getBenchmarkReasonsTitle(slower, *slowerStorage) == "What could make this run slower",
        "this run was slower");

    const wxString html = faster.toHtml();
    const wxString fasterMd = faster.toMarkdown();
    check(html.Contains("Compared with the earlier run")
        && html.Contains("<span class=\"chg up2\">2.0 times as fast</span>")
        && html.Contains("<th class=\"n\">Earlier run</th><th class=\"n\">This run</th>")
        && html.Contains("<span class=\"points-num\">")
        && html.Contains("What could make the earlier run slower")
        && html.Contains("possible reasons</span>"),
        "the HTML report shows the comparison with its reasons");
    check(html.Find("id=\"differences\"") < html.Find("<div class=\"details\">"),
        "the comparison is the overview");
    bool markdown = true;
    for (const char* part : { "### Compared with ", "| **All parts** | **1,000** |",
        "| Server disk | 1,000 | 2,000 | **",
        "**Server disk: What could make the earlier run slower**" })
    {
        if (!fasterMd.Contains(part))
        {
            std::cerr << "    the Markdown lacks: " << part << "\n";
            markdown = false;
        }
    }
    check(markdown && !fasterMd.Contains("### Key values"), "the Markdown report as well");

    // two saved reports
    BenchmarkComparison files = compareBenchmarkSnapshots(
        *parseBenchmarkSnapshot(fasterMd), *earlier);
    check(files.toHtml().Contains("Comparison of two benchmark runs")
        && files.toMarkdown().Contains("| **All parts** |")
        && files.toMarkdown().Contains("Later run")
        && files.toMarkdown().Contains("| Writing many rows | "), "two saved reports");
    check(std::any_of(files.stats.begin(), files.stats.end(),
        [](const BenchmarkComparedValue& v) { return v.title == "Changing many rows"; })
        && std::none_of(files.stats.begin(), files.stats.end(),
            [](const BenchmarkComparedValue& v)
            { return v.title == "Saving a small change (commit)"; }),
        "every other measurement is compared once");

    // which facts can explain which part
    check(isBenchmarkFactOfPart("power", C::LocalMachine, false)
        && !isBenchmarkFactOfPart("power", C::ServerCpu, false)
        && isBenchmarkFactOfPart("power", C::ServerCpu, true)
        && isBenchmarkFactOfPart("conf.DefaultDbCachePages", C::ServerCache, false)
        && isBenchmarkFactOfPart("network", C::Network, false)
        && !isBenchmarkFactOfPart("network", C::ServerStorage, false)
        && !isBenchmarkFactOfPart("unknown", C::ServerCpu, true),
        "the facts of the machine belong to the server only on the server");

    // sharing
    BenchmarkSnapshot client = *earlier;
    client.kind = "client";
    client.title = "Server dbserver01";
    client.host = "PC-MUELLER";
    client.facts.push_back({ "conf.TempDirectories", "E:\\Temp" });
    client.hints.push_back({ C::Network, "A router or VPN sits between this computer and the "
        "server", "The server sees this machine as 10.1.2.3", "Devices add latency." });
    const BenchmarkSnapshot hidden = anonymizeBenchmarkSnapshot(client);
    check(hidden.host.empty() && hidden.title == "Firebird server"
        && std::none_of(hidden.facts.begin(), hidden.facts.end(),
            [](const auto& fact) { return fact.second.Contains("E:\\Temp"); })
        && std::all_of(hidden.hints.begin(), hidden.hints.end(),
            [](const BenchmarkHint& h) { return h.detail.empty(); }),
        "a shared comparison names no computer, server, path or address");
    BenchmarkReport sharedReport = faster;
    sharedReport.earlier = client;
    const wxString sharedHtml = sharedReport.anonymized().toHtml();
    check(!sharedHtml.Contains("PC-MUELLER") && !sharedHtml.Contains("E:\\Temp")
        && !sharedHtml.Contains("10.1.2.3"), "the shared report hides the earlier run as well");
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
    testHints();
    testEventTest();
    testSettings();
    testValueAdvice();
    testParts();
    testReport();
    testHtml();
    testSql();
    testSystemInfoParsers();
    testAnonymized();
    testComparison();

    std::cout << (ok ? "ALL BENCHMARK TESTS PASSED\n" : "SOME BENCHMARK TESTS FAILED\n");
    return ok ? 0 : 1;
}
