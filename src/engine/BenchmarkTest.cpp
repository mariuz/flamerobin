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
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/init.h>

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "engine/Benchmark.h"
#include "engine/BenchmarkResults.h"

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

void testLoginAction()
{
    std::cout << "Login for the server of the registration\n";
    // a registration with a saved user name and password
    BenchmarkLoginState saved;
    saved.hasStoredPassword = true;
    saved.hasUsername = true;
    saved.hasPassword = true;
    check(getBenchmarkLoginAction(saved) == BenchmarkLoginAction::UseStored,
        "a saved login is used without asking");

    BenchmarkLoginState s = saved;
    s.hasStoredPassword = false;
    s.hasPassword = false;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::Ask,
        "no saved password: ask");
    s = saved;
    s.hasUsername = false;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::Ask,
        "no saved user name: ask");
    s = saved;
    s.alwaysAsk = true;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::Ask,
        "\"always ask for password\" asks even with a saved password");
    // a secret store without an entry gives no password
    s = saved;
    s.hasPassword = false;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::Ask,
        "a saved password that gives none (secret store): ask");

    s = saved;
    s.encryptedPassword = true;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::UseStored,
        "a decrypted password is used");
    s.hasPassword = false;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::Abort,
        "a cancelled master password prompt does not start and does not ask");
    s.hasStoredPassword = false;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::Ask,
        "encrypted mode without a saved password: ask");

    s = BenchmarkLoginState();
    s.trustedUser = true;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::None,
        "trusted authentication needs no login");
    s = BenchmarkLoginState();
    s.userEntered = true;
    s.alwaysAsk = true;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::None,
        "a login entered in the dialog is not asked for again");
    s = BenchmarkLoginState();
    s.connected = true;
    s.alwaysAsk = true;
    s.encryptedPassword = true;
    s.hasStoredPassword = true;
    check(getBenchmarkLoginAction(s) == BenchmarkLoginAction::UseStored,
        "a connected database uses the login of its connection");
}

void testTargets()
{
    using T = BenchmarkTarget;
    std::cout << "Targets of the dialog\n";
    check(getBenchmarkTargets(true) == std::vector<T>{ T::Registered, T::ThisComputer,
        T::OtherServer }, "with a registration its server comes first");
    check(getBenchmarkTargets(false) == std::vector<T>{ T::ThisComputer, T::OtherServer },
        "without a registration only this computer and another server");
    // the first entry of the list without a registration is this computer,
    // not the server of the registration
    check(getBenchmarkTarget(false, 0) == T::ThisComputer
        && getBenchmarkTarget(false, 1) == T::OtherServer
        && getBenchmarkTarget(true, 0) == T::Registered
        && getBenchmarkTarget(true, 2) == T::OtherServer, "an entry of the list is its target");
    check(getBenchmarkTarget(true, -1) == T::ThisComputer
        && getBenchmarkTarget(false, 2) == T::ThisComputer,
        "no entry, or one beyond the list: this computer");

    const BenchmarkTargetDetails registered = getBenchmarkTargetDetails(T::Registered);
    check(registered.login && registered.loginOptional && !registered.hostAndPort
        && !registered.installation && registered.readDatabase,
        "server of the registration: an optional login, no host");
    const BenchmarkTargetDetails thisComputer = getBenchmarkTargetDetails(T::ThisComputer);
    check(!thisComputer.login && !thisComputer.hostAndPort && thisComputer.installation
        && !thisComputer.readDatabase, "this computer: no host field, no login");
    const BenchmarkTargetDetails other = getBenchmarkTargetDetails(T::OtherServer);
    check(other.login && !other.loginOptional && other.hostAndPort && !other.installation
        && !other.readDatabase, "another server: host, port and login");
}

void testDurationHint()
{
    using I = BenchmarkIntensity;
    std::cout << "Duration before the start\n";
    check(getBenchmarkDurationHint(I::Quick, false, false) == "under a minute"
        && getBenchmarkDurationHint(I::Standard, false, false) == "about 2 minutes"
        && getBenchmarkDurationHint(I::Deep, false, false) == "several minutes",
        "without options only the duration of the intensity");
    check(getBenchmarkDurationHint(I::Standard, true, false)
        == "about 2 minutes, plus a few minutes for all page sizes",
        "comparing all page sizes adds a few minutes");
    check(getBenchmarkDurationHint(I::Quick, false, true)
        == "under a minute, plus about half a minute for 50 and 250 users",
        "50 and 250 users add about half a minute");
    check(getBenchmarkDurationHint(I::Deep, true, true) == "several minutes, plus a few "
        "minutes for all page sizes, plus about half a minute for 50 and 250 users",
        "both options add both");
}

// the progress of a step of a plan as the runner sends it
BenchmarkProgress stageProgress(const std::vector<BenchmarkStagePlan>& plan, int stage,
    int step, bool skipped = false)
{
    BenchmarkProgress p;
    p.stageIndex = stage;
    p.stageCount = int(plan.size());
    p.stage = plan[stage].stage;
    p.stageStep = step;
    p.stageSteps = int(plan[stage].steps.size());
    p.cleaningUp = plan[stage].stage == BenchmarkStage::Finish;
    p.fraction = getBenchmarkProgressFraction(plan, stage, step);
    p.skipped = skipped;
    return p;
}

// a tracker that saw every step up to a stage and a step of it
BenchmarkStageTracker trackUntil(const std::vector<BenchmarkStagePlan>& plan, int stage,
    int step)
{
    BenchmarkStageTracker tracker(plan);
    for (int i = 0; i <= stage; ++i)
    {
        for (int j = 0; j < int(plan[i].steps.size()) && (i < stage || j <= step); ++j)
            tracker.update(stageProgress(plan, i, j));
    }
    return tracker;
}

int stageIndex(const std::vector<BenchmarkStagePlan>& plan, BenchmarkStage stage)
{
    for (int i = 0; i < int(plan.size()); ++i)
    {
        if (plan[i].stage == stage)
            return i;
    }
    return -1;
}

void testStagePlan()
{
    using I = BenchmarkIntensity;
    using S = BenchmarkStage;
    std::cout << "Stages of the progress\n";
    // the steps of the stages that do not depend on the options, in the
    // order of BenchmarkRunner::run(); testStepsHaveStages() counts them
    const std::map<S, int> fixedSteps = { { S::Prepare, 4 }, { S::Connect, 7 }, { S::Cpu, 3 },
        { S::Disk, 7 }, { S::Memory, 6 }, { S::Network, 6 } };
    // the OLTP levels: 1, 2, 4 (Quick), 8 (Standard), 16 and 32 (Deep)
    const std::map<I, int> levels = { { I::Quick, 3 }, { I::Standard, 4 }, { I::Deep, 6 } };
    bool order = true, steps = true, count = true, fractions = true, quiet = true;
    for (I intensity : { I::Quick, I::Standard, I::Deep })
    {
        for (int options = 0; options < 4; ++options)
        {
            const bool pages = options & 1, many = options & 2;
            const std::vector<BenchmarkStagePlan> plan = getBenchmarkStagePlan(intensity,
                pages, many);
            // every stage once, in the order of the run; the page sizes only
            // when they are compared, the cleanup last
            std::vector<S> expected = { S::Prepare, S::Connect, S::Cpu, S::Disk, S::Memory,
                S::Network, S::Users };
            if (pages)
                expected.push_back(S::PageSizes);
            expected.push_back(S::Finish);
            std::vector<S> stages;
            for (const BenchmarkStagePlan& stage : plan)
                stages.push_back(stage.stage);
            order &= stages == expected;
            for (const auto& fixed : fixedSteps)
            {
                const int index = stageIndex(plan, fixed.first);
                steps &= index >= 0 && int(plan[index].steps.size()) == fixed.second;
            }
            const int users = levels.at(intensity) + (many ? 2 : 0);
            steps &= int(plan[stageIndex(plan, S::Users)].steps.size()) == users
                && int(getBenchmarkOltpLevels(BenchmarkSettings::forIntensity(intensity),
                    many).size()) == users
                && plan.back().steps.empty();
            if (pages)
                steps &= plan[stageIndex(plan, S::PageSizes)].steps.size() == 4;
            count &= getBenchmarkStepCount(plan) == 33 + users + (pages ? 4 : 0);
            // the share grows with every step, from above 0 to below 1, and
            // the cleanup comes last
            double last = 0.0;
            for (int i = 0; i < int(plan.size()); ++i)
            {
                for (int j = 0; j < std::max(1, int(plan[i].steps.size())); ++j)
                {
                    const double f = getBenchmarkProgressFraction(plan, i, j);
                    fractions &= f > last && f < 1.0
                        && (plan[i].steps.empty() || plan[i].steps[size_t(j)].weight > 0);
                    last = f;
                }
            }
            for (const BenchmarkStagePlan& stage : plan)
            {
                for (const BenchmarkStepPlan& step : stage.steps)
                {
                    if (stage.stage == S::Users || stage.stage == S::Cpu)
                        quiet &= step.quiet;
                }
            }
        }
    }
    check(order, "every option mix has the stages in the order of the run, page sizes only "
        "when compared, the cleanup last");
    check(steps, "the stages have their steps, the users one per OLTP level, also with 50 "
        "and 250 users, and the page sizes four");
    check(count, "the number of steps is that of the runner: 33 and the levels and page sizes");
    check(fractions, "the share of the run grows with every step and stays below 1 until the "
        "end");
    check(quiet, "the CPU tests and the OLTP levels keep the progress still");

    // the long disk tests take the largest share, as of the time: with one
    // share per step the bar would jump from 20 to 60 percent
    const std::vector<BenchmarkStagePlan> standard = getBenchmarkStagePlan(I::Standard, false,
        false);
    const int disk = stageIndex(standard, S::Disk);
    const double diskShare = getBenchmarkProgressFraction(standard, disk + 1, 0)
        - getBenchmarkProgressFraction(standard, disk, 0);
    check(diskShare > 0.4 && diskShare < 0.75, "the disk stage moves the progress by its "
        "share of the time");
    check(getBenchmarkStageWord(S::Prepare) == "Prepare" && getBenchmarkStageWord(S::Disk)
        == "Disk" && getBenchmarkStageWord(S::PageSizes) == "Pages"
        && getBenchmarkStageName(S::Memory) == "Memory and cache"
        && getBenchmarkStageStateName(BenchmarkStageState::NotRun) == "not run",
        "the stages have short words and names");
}

void testStageStates()
{
    using S = BenchmarkStage;
    using St = BenchmarkStageState;
    using O = BenchmarkStageTracker::Outcome;
    std::cout << "States of the stages\n";
    const std::vector<BenchmarkStagePlan> plan = getBenchmarkStagePlan(
        BenchmarkIntensity::Quick, true, false);
    const int connect = stageIndex(plan, S::Connect), disk = stageIndex(plan, S::Disk);
    const int memory = stageIndex(plan, S::Memory), pages = stageIndex(plan, S::PageSizes);
    const int finish = int(plan.size()) - 1;

    BenchmarkStageTracker tracker(plan);
    check(tracker.getCurrent() == -1 && tracker.getState(0) == St::Waiting
        && tracker.getFraction() == 0.0, "before the start every stage waits");
    // the selected database is not read: one step of the stage is skipped
    tracker.update(stageProgress(plan, 0, 0));
    tracker.update(stageProgress(plan, 0, 1));
    tracker.update(stageProgress(plan, 0, 2, true));
    tracker.update(stageProgress(plan, 0, 3));
    tracker.update(stageProgress(plan, connect, 0));
    check(tracker.getState(0) == St::Done && tracker.getState(connect) == St::Running
        && tracker.getState(connect + 1) == St::Waiting && tracker.getCurrent() == connect
        && tracker.getStageFraction(connect) > 0 && tracker.getStageFraction(connect) < 1,
        "a stage is done when the next one runs, also with a skipped step");

    // every page size is not supported: the stage did nothing
    BenchmarkStageTracker all = trackUntil(plan, pages - 1, 1000);
    for (int j = 0; j < 4; ++j)
        all.update(stageProgress(plan, pages, j, true));
    all.update(stageProgress(plan, finish, 0));
    check(all.getState(pages) == St::Skipped && all.getState(pages - 1) == St::Done
        && all.getState(finish) == St::Running, "a stage whose steps were all skipped is "
        "skipped; the cleanup runs after the last step");
    all.finish(O::Finished);
    check(all.getState(pages) == St::Skipped && all.getState(0) == St::Done
        && all.getState(finish) == St::Done && all.getFraction() == 1.0
        && all.getEndStage() == -1, "a finished run: every stage done or skipped, 100 %");

    // cancelled in the third disk test; the cleanup follows
    BenchmarkStageTracker cancelled = trackUntil(plan, disk, 2);
    const double fraction = cancelled.getFraction();
    cancelled.update(stageProgress(plan, finish, 0));
    check(cancelled.getState(disk) == St::Running && cancelled.getState(finish) == St::Running,
        "the cleanup after a cancel leaves the stage to the end of the run");
    cancelled.finish(O::Cancelled);
    check(cancelled.getState(disk - 1) == St::Done && cancelled.getState(disk) == St::Cancelled
        && cancelled.getState(memory) == St::NotRun && cancelled.getState(pages) == St::NotRun
        && cancelled.getState(finish) == St::Done && cancelled.getEndStage() == disk
        && cancelled.getFraction() == fraction && cancelled.getStageFraction(disk) > 0
        && cancelled.getStageFraction(disk) < 1, "a cancel: the stage cancelled, the later ones "
        "not run, the cleanup done, the share kept");

    // an error in the memory tests
    BenchmarkStageTracker failed = trackUntil(plan, memory, 0);
    failed.update(stageProgress(plan, finish, 0));
    failed.finish(O::Failed);
    check(failed.getState(memory) == St::Failed && failed.getState(memory + 1) == St::NotRun
        && failed.getState(disk) == St::Done && failed.getState(finish) == St::Done
        && failed.getEndStage() == memory, "an error: the stage failed, the later ones not run");

    // the temporary database could not be dropped after the last step
    BenchmarkStageTracker cleanup = trackUntil(plan, pages, 3);
    cleanup.update(stageProgress(plan, finish, 0));
    cleanup.finish(O::Failed, true);
    check(cleanup.getState(pages) == St::Done && cleanup.getState(finish) == St::Failed
        && cleanup.getEndStage() == finish, "an error of the cleanup after the last step is "
        "one of the cleanup");

    // the run ended before its first step, e.g. without a client library
    BenchmarkStageTracker early(plan);
    early.finish(O::Failed);
    check(early.getState(0) == St::Failed && early.getState(1) == St::NotRun
        && early.getEndStage() == 0, "an error before the first step is one of the first stage");
}

void testAnimation()
{
    std::cout << "Animation of the progress\n";
    using A = BenchmarkAnimation;
    BenchmarkAnimationContext normal;
    normal.running = true;
    check(getBenchmarkAnimation(normal) == A::Wave && getBenchmarkAnimationInterval(normal) == 250
        && getBenchmarkTimerInterval(normal) == 250 && getBenchmarkBlinkInterval(normal) == 6000,
        "a normal step moves the edge with 4 pictures a second, the robin blinks every 6 s");
    BenchmarkAnimationContext quiet = normal;
    quiet.quietStep = true;
    check(getBenchmarkAnimation(quiet) == A::Breathe && getBenchmarkAnimationInterval(quiet)
        == 1000 && getBenchmarkTimerInterval(quiet) == 1000 && getBenchmarkBlinkInterval(quiet)
        == 0, "a quiet step only breathes with 1 picture a second and does not blink");
    BenchmarkAnimationContext remote = normal;
    remote.remoteSession = true;
    BenchmarkAnimationContext remoteQuiet = quiet;
    remoteQuiet.remoteSession = true;
    check(getBenchmarkAnimation(remote) == A::Breathe && getBenchmarkAnimationInterval(remote)
        == 1000 && getBenchmarkBlinkInterval(remote) == 0
        && getBenchmarkAnimationInterval(remoteQuiet) == 1000,
        "a remote session gets 1 picture a second, also in normal steps");
    BenchmarkAnimationContext off = normal;
    off.systemAnimations = false;
    BenchmarkAnimationContext offQuiet = quiet;
    offQuiet.systemAnimations = false;
    check(getBenchmarkAnimation(off) == A::Still && getBenchmarkAnimation(offQuiet) == A::Still
        && getBenchmarkTimerInterval(off) == benchmarkElapsedIntervalMs,
        "without the animations of the system the logo is still, the timer shows the time");
    BenchmarkAnimationContext after = normal;
    after.running = false;
    check(getBenchmarkAnimation(after) == A::Still && getBenchmarkTimerInterval(after) == 0,
        "after the run the logo is still and the timer stops");
    // every combination: never faster than 8 pictures a second, the timer
    // runs only during a run, a quiet step never gets more than 1 picture a
    // second and never stands still where the logo may move
    bool limit = true, stops = true, modes = true, quietOnes = true, breathes = true;
    for (int bits = 0; bits < 64; ++bits)
    {
        BenchmarkAnimationContext c;
        c.running = bits & 1;
        c.quietStep = bits & 2;
        c.windowVisible = bits & 4;
        c.remoteSession = bits & 8;
        c.systemAnimations = bits & 16;
        c.userDisabled = bits & 32;
        const A mode = getBenchmarkAnimation(c);
        const int animation = getBenchmarkAnimationInterval(c);
        const int timer = getBenchmarkTimerInterval(c);
        const int blink = getBenchmarkBlinkInterval(c);
        limit &= animation == 0 || animation >= benchmarkMinAnimationIntervalMs;
        limit &= timer == 0 || timer >= benchmarkMinAnimationIntervalMs;
        stops &= (timer == 0) == !c.running;
        // still only where something speaks against every movement
        const bool allowed = c.running && c.windowVisible && c.systemAnimations
            && !c.userDisabled;
        modes &= (mode == A::Still) == !allowed && (animation == 0) == (mode == A::Still);
        modes &= mode != A::Breathe || (c.quietStep || c.remoteSession);
        // without the animation the timer only shows the elapsed time; the
        // blink falls on a picture of the wave
        modes &= animation > 0 || !c.running || timer == benchmarkElapsedIntervalMs;
        modes &= blink == 0 || (mode == A::Wave && blink % animation == 0);
        if (c.quietStep || c.remoteSession)
        {
            quietOnes &= blink == 0
                && (animation == 0 || animation >= benchmarkBreathIntervalMs);
        }
        breathes &= !allowed || animation > 0;
    }
    check(limit && benchmarkMinAnimationIntervalMs >= 125, "the animation never has more "
        "than 8 pictures a second");
    check(stops, "the timer stops when no run is going on");
    check(modes, "still only unseen, without system animations or when turned off, then only "
        "the elapsed time once a second; the breath only in quiet steps and remote sessions");
    check(quietOnes, "quiet steps and remote sessions never get more than 1 picture a second "
        "and no blink");
    check(breathes, "where the logo may move it never stands still, also in quiet steps");
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
        m.remoteAddress = "10.0.0.10";
        m.system.addresses = { { "WLAN", "Wi-Fi", "10.0.0.1" } };
        expectHint(m, "A router or VPN sits between", C::Network,
            "complete addresses are compared, not parts of them");
        m.remoteAddress = "fe80::1";
        m.system.addresses = { { "WLAN", "Wi-Fi", "[FE80::1%7]" } };
        expectNoHint(m, "A router or VPN sits between", "the own IPv6 address");
        check(normalizeBenchmarkAddress("10.0.0.5/51234") == "10.0.0.5"
            && normalizeBenchmarkAddress("::ffff:10.0.0.5") == "10.0.0.5"
            && isBenchmarkIpAddress("192.168.1.20") && isBenchmarkIpAddress("fe80::1")
            && !isBenchmarkIpAddress("256.1.1.1") && !isBenchmarkIpAddress("dbserver")
            && !isBenchmarkIpAddress("all addresses")
            && isBenchmarkPrivateAddress("10.1.2.3") && !isBenchmarkPrivateAddress("127.0.0.1")
            && !isBenchmarkPrivateAddress("0.0.0.0") && !isBenchmarkPrivateAddress("::1"),
            "IP addresses are recognized");
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
    bool all = true;
    for (const BenchmarkMeasurementDef& def : getBenchmarkMeasurementDefs())
    {
        if (def.kind == BenchmarkMeasurementKind::Key
            && getBenchmarkValueAdvice(def.id).empty())
        {
            std::cerr << "    no advice for " << def.id << "\n";
            all = false;
        }
    }
    check(all, "every key value has its typical causes");
    check(getBenchmarkValueAdvice("storage.commit").Contains("disk"),
        "commits wait for the disk");
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
    check(commit.id == "storage.commit" && commit.valueText == "1.50 ms"
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
}

// a run with every measurement and every option, so that buildBenchmarkResults()
// gives every result it knows
BenchmarkMetrics fullMetrics()
{
    BenchmarkMetrics m = healthyMetrics();
    m.serverProcesses = 1;
    m.remoteProtocol = "TCPv4";
    m.protocolVersion = "P19";
    m.wireCompressed = false;
    m.wireEncrypted = true;
    m.authMethod = "Srp256";
    m.securityDatabase = "Default";
    m.idleTimeoutSeconds = 3600;
    m.statementTimeoutMs = 0;
    BenchmarkDatabaseInfo inspected;
    inspected.pageSize = 8192;
    inspected.pages = 1000;
    inspected.lingerSeconds = 60;
    m.inspectedDatabase = inspected;
    m.serverConfig["TempCacheLimit"] = "67108864";
    m.serverConfig["ServerMode"] = "Super";
    m.networkPath = BenchmarkNetworkPath();
    m.networkPath->serverHost = "dbserver";
    m.networkPath->serverAddress = "10.0.0.5";
    m.networkPath->resolveMs = 1.0;
    m.networkPath->adapterName = "eth0";
    m.networkPath->adapterType = "Ethernet";
    m.serverPort = 3050;
    m.serverPorts = { { 3050, BenchmarkPortCheck::State::Open, 0.4 } };
    m.eventMs = 2.0;
    m.localOnBattery = false;
    m.system.tcpKeepAliveSeconds = 7200;
    m.createDatabaseMs = 300.0;
    m.osSyncWriteMs = 0.3;
    m.databaseDirFileMs = 0.25;
    m.databaseDiskFreeBytes = int64_t(100) * 1024 * 1024 * 1024;
    m.largeRoundTripMs = 0.5;
    m.pageSizes = { { 8192, 2048, 1.0, 900.0, 50.0, 40.0, 3500.0 } };
    m.oltp.push_back({ 50, 60000, 50, 5.0, 4.0, 9.0 });
    m.oltp.push_back({ 250, 50000, 400, 5.0, 25.0, 60.0 });
    m.skippedTests.push_back({ "application.versions", "Reading while an old transaction "
        "is open", "unsupported" });
    m.pageReads = 1000;
    m.pageWrites = 500;
    m.pageFetches = 90000;
    m.pageMarks = 4000;
    m.recordPurges = 30;
    return m;
}

void testRegister()
{
    std::cout << "Register of the measurements\n";
    using K = BenchmarkMeasurementKind;
    const std::vector<BenchmarkMeasurementDef>& defs = getBenchmarkMeasurementDefs();
    bool wellFormed = true;
    std::set<wxString> ids;
    std::vector<double BenchmarkValues::*> members;
    for (const BenchmarkMeasurementDef& def : defs)
    {
        const wxString id = def.id;
        const wxString unit = def.unit;
        const bool compared = def.kind != K::Info;
        wxString problem;
        if (!isBenchmarkMeasurementId(id) || id.Freq('.') != 1)
            problem = "not an id of two parts";
        else if (!ids.insert(id).second)
            problem = "twice in the register";
        else if (id.BeforeFirst('.') != getBenchmarkSectionId(def.section))
            problem = "does not start with its section";
        else if (def.version < 1)
            problem = "no version";
        else if ((def.kind == K::Key) != (def.title != nullptr && def.value != nullptr))
            problem = "a key value needs a title and a member, nothing else";
        else if (compared && (unit.empty() || (unit == "ms") == def.higherIsBetter))
            problem = "a time is lower is better, a rate higher is better";
        else if (!compared && !unit.empty())
            problem = "information has no unit";
        else if (def.value
            && std::find(members.begin(), members.end(), def.value) != members.end())
        {
            problem = "a member of BenchmarkValues twice";
        }
        if (def.value)
            members.push_back(def.value);
        if (!problem.empty())
        {
            std::cerr << "    " << id.utf8_str() << ": " << problem.utf8_str() << "\n";
            wellFormed = false;
        }
    }
    check(wellFormed, "every id is unique, starts with its section and has a version");
    check(members.size() * sizeof(double) == sizeof(BenchmarkValues),
        "every key value of BenchmarkValues is in the register");
    check(findBenchmarkMeasurementDef("users.tx.8")
        && findBenchmarkMeasurementDef("users.tx.8") == findBenchmarkMeasurementDef("users.tx.250")
        && !findBenchmarkMeasurementDef("users.tx") && !findBenchmarkMeasurementDef("users.tx.8.1")
        && !findBenchmarkMeasurementDef("cpu.single.2")
        && !findBenchmarkMeasurementDef("Saving a change (commit)")
        && findBenchmarkMeasurementDef("storage.commit")->kind == K::Key,
        "ids and families are found, titles are no ids");
    check(makeBenchmarkMeasurementId("settings.conf", "Temp-Cache Limit")
            == "settings.conf.TempCacheLimit"
        && isBenchmarkMeasurementId("users.tx.8") && !isBenchmarkMeasurementId("users..tx")
        && !isBenchmarkMeasurementId("users.tx.") && !isBenchmarkMeasurementId("commit")
        && !isBenchmarkMeasurementId("storage.commit ms"), "the characters of an id");

    // every result has a registered id, and every registered id a result
    std::set<wxString> found;
    bool registered = true;
    for (const BenchmarkMetrics& m : { fullMetrics(), serverMetrics() })
    {
        for (const BenchmarkResult& r : buildBenchmarkResults(m))
        {
            const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(r.id);
            const bool skipped = r.value == "not measured";
            if (!def || def->retired
                || (!skipped && def->section != getBenchmarkReportSection(r.category)))
            {
                std::cerr << "    not registered in its section: " << r.id.utf8_str() << " ("
                    << r.test.utf8_str() << ")\n";
                registered = false;
            }
            else
                found.insert(def->id);
        }
    }
    check(registered, "every result has the id of a measurement of its section");
    bool allFound = true;
    for (const BenchmarkMeasurementDef& def : defs)
    {
        // the best OLTP level is a key value without a result of its own
        if (!def.retired && !found.count(def.id) && wxString(def.id) != "users.oltp")
        {
            std::cerr << "    no result: " << def.id << "\n";
            allFound = false;
        }
    }
    check(allFound, "every measurement of the register has a result");

    // the hints have ids as well, the same for every part they are given for
    BenchmarkMetrics noisy = fullMetrics();
    noisy.localOnBattery = true;
    noisy.clientBurstIdleMs = 50.0;
    noisy.system.antivirus = { "Windows Defender (on)" };
    noisy.system.virtualization = "Hyper-V";
    noisy.runLocation = BenchmarkRunLocation::Server;
    noisy.serverConfig["ServerMode"] = "Classic";
    std::map<wxString, wxString> titles;
    bool hintIds = true;
    for (const BenchmarkHint& h : findBenchmarkHints(noisy))
    {
        auto title = titles.insert({ h.id, h.title });
        if (!isBenchmarkMeasurementId(h.id) || title.first->second != h.title)
        {
            std::cerr << "    hint " << h.id.utf8_str() << ": " << h.title.utf8_str() << "\n";
            hintIds = false;
        }
    }
    check(hintIds && titles.size() >= 5, "every hint has an id, the same in every part");
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
        [](const BenchmarkResult& r) { return isBenchmarkKeyValueSource(r); }) == 14,
        "every key value but the best of the OLTP levels has the result it comes from");
    {   // scope
        BenchmarkMetrics m = healthyMetrics();
        m.skippedTests.push_back({ "application.blob", "BLOB write / read",
            "Wrong parameters block kind 6" });
        m.skippedTests.push_back({ "storage.tempSort", "Sorting large data", "unsupported" });
        auto results = buildBenchmarkResults(m);
        check(std::any_of(results.begin(), results.end(),
            [](const BenchmarkResult& r)
            {
                return r.id == "application.blob" && r.test == "BLOB write / read"
                    && r.value == "not measured" && r.details.Contains("block kind 6");
            }), "skipped optional tests are listed with the reason");
        check(std::any_of(results.begin(), results.end(),
            [](const BenchmarkResult& r)
            {
                return r.id == "storage.tempSort" && r.value == "not measured"
                    && r.category == BenchmarkCategory::Application
                    && !isBenchmarkKeyValueSource(r);
            }), "a skipped test is no key value");
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
    const wxString visible = md.Left(md.Find("<!-- FlameRobin benchmark snapshot"));
    check(md.Find("## Performance Benchmark & Diagnosis") == 0
        && md.Find("### Key figures") < md.Find("### System")
        && md.Find("### System") < md.Find("### All measured values")
        && md.Find("### All measured values") < md.Find("<!-- FlameRobin benchmark snapshot"),
        "key figures, system, all measured values, then the results for comparisons");
    check(md.Contains("| Part | Key figure | Result | Work |")
        && md.Contains("| DISK | Saving a change (commit) | 1.50 ms | 200 commits in 300 ms |")
        && !md.Contains("### Key values"), "the key figures with the work behind them");
    check(!visible.Contains("Points") && !visible.Contains("What could make")
        && !visible.Contains("Power saving slows"), "a single run is not judged");
    check(md.Contains("\nm.storage.commit = 1.5 | ms | v1 | 200 | commits | 300 | ")
        && md.Contains("\nnote.power.saving = computer | Power saving"),
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

// the rules of a style sheet with their selectors, and whether they are
// inside @media, which the mode of Internet Explorer 7 and 8 ignores
struct CssRule
{
    wxString selector;
    wxString body;
    bool media;
};

std::vector<CssRule> cssRules(const wxString& css)
{
    std::vector<CssRule> rules;
    int depth = 0;
    size_t start = 0;
    for (size_t i = 0; i < css.length(); ++i)
    {
        if (css[i] == '{')
        {
            const size_t next = css.find_first_of("{}", i + 1);
            if (next != wxString::npos && css[next] == '{')
            {
                // @media: the rules follow
                ++depth;
                start = i + 1;
                continue;
            }
            if (next == wxString::npos)
                break;
            wxString selector = css.Mid(start, i - start);
            // comments before the selector
            if (selector.Contains("*/"))
                selector = selector.AfterLast('/');
            selector.Trim().Trim(false);
            rules.push_back({ selector, css.Mid(i + 1, next - i - 1), depth > 0 });
            i = next;
            start = next + 1;
        }
        else if (css[i] == '}')
        {
            depth = std::max(0, depth - 1);
            start = i + 1;
        }
    }
    return rules;
}

// what keeps a page from working in Internet Explorer 11, or from being
// readable in its modes of Internet Explorer 7 and 8; empty if nothing.
// Every declaration it does not understand needs one before it that it
// does, and the page must not depend on a script or a browser switch
wxString findLegacyProblem(const wxString& html)
{
    if (!html.StartsWith("<!doctype html>\n"))
        return "no doctype";
    // X-UA-Compatible only works before every other element but the title
    const int ua = html.Find("<head><meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">");
    if (ua == wxNOT_FOUND || ua > html.Find("<meta charset=\"utf-8\">")
        || html.Find("<meta charset=\"utf-8\">") > html.Find("<style>"))
    {
        return "X-UA-Compatible missing or not first";
    }
    // <nav> and <section> cannot be styled in the mode of Internet Explorer 8
    for (const char* forbidden : { "<script", "@import", "@supports", "-ms-high-contrast",
        "expression(", "<nav", "<section" })
    {
        if (html.Contains(forbidden))
            return wxString("contains ") + forbidden;
    }
    // pictograms: Internet Explorer 11 draws a <use> with xlink:href only,
    // an <svg> without a size is 300 x 150 px and focusable by default
    const int uses = html.Freq('<') == 0 ? 0 : int(wxSplit(html, '<', '\0').size());
    (void)uses;
    size_t count = 0;
    for (size_t pos = html.find("<use "); pos != wxString::npos; pos = html.find("<use ", pos + 1))
    {
        ++count;
        if (html.compare(pos, 18, "<use xlink:href=\"#") != 0)
            return "a <use> without xlink:href";
    }
    for (size_t pos = html.find("<svg class=\"ico"); pos != wxString::npos;
        pos = html.find("<svg class=\"ico", pos + 1))
    {
        const wxString element = html.Mid(pos, html.find('>', pos) - pos);
        if (!element.Contains(" width=\"") || !element.Contains(" height=\"")
            || !element.Contains(" focusable=\"false\""))
        {
            return "a pictogram without its size or with a tab stop: " + element;
        }
    }
    const int sprite = html.Find("<body><svg class=\"sprite\"");
    if (count > 0 && (sprite == wxNOT_FOUND
        || html.find("<svg class=\"sprite\"", sprite + 7) != wxString::npos))
    {
        return "the pictograms are not defined once at the start of the page";
    }

    const int styleStart = html.Find("<style>") + 7;
    const wxString css = html.Mid(styleStart, html.Find("</style>") - styleStart);
    int pairs = 0;
    for (const CssRule& rule : cssRules(css))
    {
        wxString previousProperty, previousValue;
        for (const wxString& declaration : wxSplit(rule.body, ';', '\0'))
        {
            wxString property = declaration.BeforeFirst(':');
            property.Trim().Trim(false);
            wxString value = declaration.AfterFirst(':');
            value.Trim().Trim(false);
            // Internet Explorer takes "font: 13px var(--x)" for the name of
            // a font, the fallback before it would not help; it drops a
            // font shorthand with -apple-system
            if ((property == "font" || property == "font-family") && value.Contains("var("))
                return "a font from a variable: " + declaration;
            if (property == "font" && value.Contains("-apple-system"))
                return "a font shorthand that Internet Explorer drops: " + declaration;
            // the mode of Internet Explorer 7 and 8 makes an element with an
            // unknown display value inline, also after a known one
            if (property == "display" && previousProperty == "display"
                && (value == "flex" || value == "inline-flex")
                && (previousValue == "block" || previousValue == "inline-block"))
            {
                return "display: flex after display: block in " + rule.selector;
            }
            // without :checked (Internet Explorer 8) nothing may stay hidden
            // but what only works with it
            if (property == "display" && value.StartsWith("none") && !rule.media
                && !rule.selector.Contains(":checked") && rule.selector != ".pages"
                && rule.selector != ".filter" && rule.selector != "label.more"
                && rule.selector != ".tip-box"
                && !rule.selector.Contains("::-webkit-details-marker"))
            {
                return "hidden without :checked: " + rule.selector;
            }
            wxString expected;
            if (value.Contains("var(") && !property.StartsWith("--"))
            {
                if (previousProperty != property || previousValue.Contains("var("))
                    return "no fallback for: " + declaration;
                ++pairs;
            }
            else if (property == "display" && value == "grid")
                expected = "display:block";
            else if (property == "display" && value == "inline-grid")
                expected = "display:inline-block";
            else if (property == "overflow-wrap" && value == "anywhere")
                expected = "word-wrap:break-word";
            if (!expected.empty())
            {
                if (previousProperty + ":" + previousValue != expected)
                    return "no fallback for: " + declaration;
                ++pairs;
            }
            previousProperty = property;
            previousValue = value;
        }
    }
    return pairs > 100 ? wxString() : wxString("too few fallbacks");
}

void testCssFallbacks()
{
    std::cout << "Internet Explorer 11\n";
    const wxString css = ":root { --ink: #111; --gap: 4px; }\n"
        ":root[data-theme=\"dark\"] { --ink: #eee; }\n"
        "a { color: var(--ink); margin: var(--gap) 0; }\n"
        "@media (max-width: 640px) { .x { display: grid; gap: 4px; } }\n"
        ".y{overflow-wrap:anywhere}\n"
        ".z { margin-left: 8px; margin-left: var(--gap); }\n"
        ".t { display: inline-grid; color: var(--unknown); }\n";
    const wxString light = addBenchmarkCssFallbacks(css, false);
    check(light.Contains("a { color: #111; color: var(--ink); margin: 4px 0; "
        "margin: var(--gap) 0; }"), "variables get a fixed value before them");
    check(addBenchmarkCssFallbacks(css, true).Contains("color: #eee; color: var(--ink);")
        && addBenchmarkCssFallbacks(css, true).Contains("margin: 4px 0; margin: var(--gap) 0;"),
        "the values of the dark theme, the light ones where it has none");
    check(light.Contains("{ display: block; display: grid; gap: 4px; }")
        && light.Contains(".y{word-wrap: break-word; overflow-wrap:anywhere}")
        && light.Contains("{ display: inline-block; display: inline-grid;"),
        "grid becomes a block, overflow-wrap: anywhere breaks words, also in @media");
    check(light.Contains(".z { margin-left: 8px; margin-left: var(--gap); }"),
        "a pair written for Internet Explorer is kept");
    check(light.Contains("color: var(--unknown); }") && !light.Contains("color: ; "),
        "an unknown variable is left alone");
    check(light.Contains(":root { --ink: #111; --gap: 4px; }")
        && addBenchmarkCssFallbacks("p { margin: 0; }", false) == "p { margin: 0; }",
        "everything else stays as it is");
}

std::vector<BenchmarkMetrics> noticedMetrics();
BenchmarkReport makeReport(const BenchmarkMetrics& m, const wxString& date);

// how often a text occurs
int countOf(const wxString& text, const wxString& part)
{
    int n = 0;
    for (size_t pos = text.find(part); pos != wxString::npos; pos = text.find(part, pos + 1))
        ++n;
    return n;
}

// a page that differs from a correct one only where the check must find it
void testLegacyRules(const wxString& html)
{
    auto broken = [&html](const wxString& from, const wxString& to)
    {
        wxString page = html;
        page.Replace(from, to, false);
        return findLegacyProblem(page);
    };
    check(broken("<use xlink:href=", "<use href=").Contains("xlink:href")
        && broken("<svg class=\"ico\" width=\"20\"", "<svg class=\"ico\"").Contains("size")
        && broken("aria-hidden=\"true\" focusable=\"false\"><use", "aria-hidden=\"true\"><use")
            .Contains("tab stop")
        && broken("<main>", "<main><svg class=\"sprite\"></svg>").Contains("once")
        && broken("<div class=\"overview\">", "<section class=\"overview\">").Contains("section")
        && broken(".so-line { display: block; }", ".so-line { display: block; display: flex; }")
            .Contains("display: flex after display: block")
        && broken(".pages { display: none;", ".pages, .notes { display: none;")
            .Contains("hidden without :checked")
        && broken("<style>", "<style>.x { color: var(--ink); }").Contains("no fallback"),
        "the check finds what Internet Explorer 7, 8 or 11 cannot show");
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
    const wxString page = html.Left(html.Find("<!-- FlameRobin benchmark snapshot"));
    const wxString overview = html.Mid(html.Find("<div class=\"overview\">"),
        html.Find("<div class=\"details\">") - html.Find("<div class=\"overview\">"));
    check(html.StartsWith("<!doctype html>") && html.Contains("</html>"), "complete page");
    check(html.Contains("data-theme=\"dark\""), "theme of FlameRobin");
    check(report.toHtml().Contains("<html lang=\"en\">"),
        "saved page follows the system theme");
    check(html.Contains("Server &lt;b&gt;x&lt;/b&gt; &amp; &quot;y&quot;")
        && !html.Contains("<b>x</b>"), "text is escaped");
    check(html.Find("<div class=\"pages\">") < html.Find("<div class=\"overview\">")
        && html.Find("<div class=\"overview\">") < html.Find("id=\"figures\"")
        && html.Find("id=\"figures\"") < html.Find("id=\"noticed\"")
        && html.Find("id=\"noticed\"") < html.Find("id=\"key-values\"")
        && html.Find("id=\"key-values\"") < html.Find("<div class=\"details\">")
        && html.Find("<div class=\"details\">") < html.Find("id=\"categories\"")
        && html.Find("id=\"categories\"") < html.Find("id=\"system\"")
        && html.Find("id=\"system\"") < html.Find("id=\"values\""),
        "the overview, then the detailed report");
    check(html.Contains("id=\"v1\" checked>")
        && html.Find("\" checked") == html.Find("id=\"v1\" checked") + 6
        && !html.Contains("id=\"v3\""), "the overview is shown first");
    check(!html.Contains("<details open") && !html.Contains("details open"),
        "all details are closed at first");
    check(html.Contains("#v1:checked ~ .details, #v1:checked ~ .cmp-details, #v2:checked ~ "
            ".overview, #v2:checked ~ .cmp-details, #v3:checked ~ .overview, #v3:checked ~ "
            ".details { display: none; }")
        && html.Contains("<div class=\"pages\"><label for=\"v1\">Overview</label><label "
            "for=\"v2\">Detailed report</label></div>"),
        "the views switch without a script");

    // a single run: key figures and notes, without judging them
    check(countOf(overview, "<div class=\"fig ") == 6
        && countOf(overview, "<span class=\"tag-word\">") >= 6
        && overview.Contains("<span class=\"fig-num\">1.50</span><span class=\"fig-unit\">ms"
            "</span></span><span class=\"fig-text\"><span class=\"fig-name\">Saving a change "
            "(commit)</span><span class=\"fig-work\">200 commits in 300 ms</span>")
        && overview.Contains("Key figures <span class=\"quiet\">measured values of this run, "
            "not rated</span>"),
        "six key figures with their work, not rated");
    check(countOf(overview, "<details class=\"note\">") == 1
        && overview.Contains("<span class=\"note-title\">A short CPU burst after an idle "
            "pause takes 2.5x as long</span><span class=\"note-where\">LOCAL</span>")
        && overview.Contains("<b>What can help:</b> "),
        "a note: what was noticed, where, and on a click what can help");
    check(overview.Contains("<details class=\"more\" id=\"key-values\">")
        && overview.Contains("<small>200 commits in 300 ms</small>")
        && overview.Contains("<label class=\"more\" for=\"v2\">"),
        "all key values, the run and the detailed report one click away");
    bool unjudged = true;
    for (const char* judgement : { "Power saving slows", "What could make",
        "<span class=\"dir", "class=\"verdict", "points; the earlier run" })
    {
        if (page.Contains(judgement))
        {
            std::cerr << "    the page contains: " << judgement << "\n";
            unjudged = false;
        }
    }
    check(unjudged, "no points, no changes and no causes in a single run");
    check(html.Contains("note.power.saving = computer | Power saving"),
        "the hints are kept for a comparison");

    // the detailed report: one line per part, opened by a click
    int sections = 0;
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        sections += std::any_of(report.results.begin(), report.results.end(),
            [section](const BenchmarkResult& r)
            { return getBenchmarkReportSection(r.category) == section; });
    }
    check(countOf(html, "<details class=\"area ") == sections
        && countOf(html, "<summary class=\"fig ") == sections
        && !html.Contains("class=\"tile") && !html.Contains("class=\"toggle"),
        "one line per part with values");
    check(html.Contains("<details class=\"area disk\"><summary class=\"fig disk\">")
        && html.Contains("<span class=\"fig-name\">Server disk</span><span class=\"fig-work\">"
            "Saving a change (commit) "),
        "a part shows its first key value in large");
    check(html.Contains("id=\"notes\"") && html.Contains("1 note, by part"),
        "every note in the detailed report");
    check(html.Contains("Server disk") && html.Contains("Settings")
        && html.Contains("Programming patterns"), "every part");
    check(html.Contains("<code>DefaultDbCachePages</code>") && html.Contains("What it does"),
        "the settings with what they do");
    check(html.Contains("id=\"glossary\"") && html.Contains("id=\"values\"")
        && html.Contains("id=\"run\""),
        "background information at the end");
    // the mode of Internet Explorer 8, which knows no :checked, shows
    // every view and part, and no link that does not work
    check(html.Contains(".pages { display: none;")
        && html.Contains(".view-toggle:checked ~ .pages { display: block; }")
        && html.Contains("label.more { display: none; }")
        && html.Contains(".view-toggle:checked ~ .overview label.more { display: flex; }"),
        "without :checked every view is shown");
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
    check(countOf(html, "<svg class=\"sprite\"") == 1
        && html.Contains("<body><svg class=\"sprite\" aria-hidden=\"true\" focusable=\"false\">"
            "<symbol id=\"i-cpu\" viewBox=\"0 0 24 24\">")
        && html.Contains("<use xlink:href=\"#i-write\"/>") && !html.Contains("<use href"),
        "the pictograms once per page, used with xlink:href");
    const wxString legacyDark = findLegacyProblem(html);
    const wxString legacyLight = findLegacyProblem(report.toHtml());
    if (!legacyDark.empty() || !legacyLight.empty())
        std::cerr << "    " << (legacyDark + legacyLight).ToStdString() << "\n";
    check(legacyDark.empty() && legacyLight.empty()
        && html.Contains("html, body { margin: 0; background: #1c1e22; background: var(--bg);")
        && report.toHtml().Contains("background: #ffffff; background: var(--bg);"),
        "a page for Internet Explorer 11 as well, in the theme of FlameRobin");
    check(html.Contains("html, body { margin: 0; background: #1c1e22; background: var(--bg); "
        "color: #e8eaed; color: var(--ink); font-size: 14px; line-height: 1.5; font-family: "
        "-apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, Helvetica, Arial, sans-serif; }"),
        "the font without a variable, so that Internet Explorer finds it");
    testLegacyRules(html);
    // SVG coordinates and CSS widths must not use a decimal comma
    check(!html.Contains("width:0,") && !html.Contains("y=\"0,")
        && !html.Contains("% \""), "locale independent numbers");
}

// reports without key figures or notes: nothing measured, a cancelled or a
// failed run, a run without hints
void testHtmlParts()
{
    std::cout << "HTML report of incomplete runs\n";
    BenchmarkReport cancelled;
    cancelled.cancelled = true;
    cancelled.evaluate();
    const wxString empty = cancelled.toHtml();
    check(!empty.Contains("id=\"figures\"") && !empty.Contains("id=\"noticed\"")
        && !empty.Contains("id=\"key-values\"") && !empty.Contains("class=\"split\"")
        && empty.Contains("<div class=\"label first\">More</div><div class=\"rows\">")
        && empty.Contains("The run was cancelled") && findLegacyProblem(empty).empty(),
        "a run without values: no empty key figures, notes or tables");

    BenchmarkReport failed;
    failed.errorMessage = "connection lost";
    failed.metrics.settings = BenchmarkSettings::forIntensity(BenchmarkIntensity::Standard);
    failed.metrics.cpuMs = 70.0;
    failed.metrics.cpuServerMs = 65.0;
    failed.evaluate();
    const wxString partial = failed.toHtml();
    check(countOf(partial, "<div class=\"fig ") == 1 && partial.Contains("id=\"figures\"")
        && !partial.Contains("id=\"noticed\"") && partial.Contains("connection lost")
        && findLegacyProblem(partial).empty(),
        "a failed run: the key figures it measured");

    const wxString quiet = makeReport(healthyMetrics(), "2026-10-04 12:00:00").toHtml();
    check(countOf(quiet, "<div class=\"fig ") == 6 && !quiet.Contains("id=\"noticed\"")
        && !quiet.Contains("id=\"notes\"")
        && quiet.Contains("<div class=\"side\"><div class=\"label first\">More</div>"),
        "a run without hints has no notes");

    // many hints: three notes, the others in the detailed report
    const BenchmarkReport noticed = makeReport(noticedMetrics()[0], "2026-10-04 12:00:00");
    const wxString html = noticed.toHtml();
    const wxString overview = html.Mid(html.Find("<div class=\"overview\">"),
        html.Find("<div class=\"details\">") - html.Find("<div class=\"overview\">"));
    const size_t kinds = getBenchmarkTopHints(noticed, noticed.hints.size()).size();
    bool everyNote = true;
    for (const BenchmarkHint& h : getBenchmarkTopHints(noticed, kinds))
    {
        everyNote = everyNote
            && html.Contains("<li><b>" + (h.noticed.empty() ? h.title : h.noticed));
    }
    check(countOf(overview, "<details class=\"note\">") == 3
        && overview.Contains(wxString::Format("%d more notes in the detailed report",
            int(kinds) - 3))
        && overview.Contains("A short CPU burst after an idle pause takes 2.5x as long")
        && !overview.Contains("Power saving slows") && everyNote,
        "at most three notes with neutral titles, all of them in the detailed report");
    check(overview.Contains("<span class=\"note-where\">LOCAL, CPU</span>"),
        "a note names every part it was given for once");
}

// the comparison in HTML: the main statement, a line per key figure, what
// stands out and every value of both runs
// a run of a client with many hints
BenchmarkSnapshot olderClientRun()
{
    return makeBenchmarkSnapshot(makeReport(noticedMetrics()[2], "2026-10-04 12:00:00"));
}

// a comparison with every state: faster, slower, about the same, another
// test version, other work, new and no longer measured
BenchmarkComparison everyStateComparison()
{
    BenchmarkSnapshot newer = makeBenchmarkSnapshot(makeReport(healthyMetrics(),
        "2026-10-05 12:00:00"));
    for (BenchmarkMeasurement& x : newer.measurements)
    {
        if (x.id == "cpu.single")
            x.value *= 1.5;
        else if (x.id == "storage.commit")
            x.version = 2;
        else if (x.id == "storage.insert")
            x.amount = 60000;
        else if (x.id == "users.oltp")
            x.value /= 2.0;
    }
    newer.measurements.erase(std::remove_if(newer.measurements.begin(),
        newer.measurements.end(), [](const BenchmarkMeasurement& x)
        { return x.id == "network.roundTrip" || x.id.StartsWith("application."); }),
        newer.measurements.end());
    BenchmarkMeasurement trim;
    trim.id = "storage.trim";
    trim.value = 12;
    trim.unit = "ms";
    trim.version = 1;
    trim.title = "Trimming the disk";
    trim.text = "12.00 ms";
    newer.measurements.push_back(trim);
    return compareBenchmarkSnapshots(newer, olderClientRun());
}

void testComparisonHtml()
{
    std::cout << "HTML comparison\n";
    const wxString separator = wxString("<span class=\"sep\"> ") + wxUniChar(0x00B7)
        + " </span>";
    const BenchmarkSnapshot older = olderClientRun();
    BenchmarkComparison c = everyStateComparison();
    const wxString html = c.toHtml();
    const wxString dark = c.toHtml("dark");
    const wxString overview = html.Mid(html.Find("<div class=\"overview\">"),
        html.Find("<div class=\"cmp-details\">") - html.Find("<div class=\"overview\">"));
    const wxString details = html.Mid(html.Find("<div class=\"cmp-details\">"));

    check(findLegacyProblem(html).empty() && findLegacyProblem(dark).empty(),
        "a comparison for Internet Explorer 11 as well, light and dark");
    check(html.Find("id=\"comparison\"") < html.Find("id=\"areas\"")
        && html.Find("id=\"areas\"") < html.Find("id=\"changes\"")
        && html.Find("id=\"changes\"") < html.Find("id=\"differences\"")
        && html.Find("id=\"differences\"") < html.Find("<div class=\"cmp-details\">")
        && html.Contains("<div class=\"pages\"><label for=\"v1\">Overview</label><label "
            "for=\"v3\">Detailed comparison</label></div>")
        && !html.Contains("id=\"v2\"") && html.Contains("<label class=\"more\" for=\"v3\">"),
        "the main statement first, then the key figures, what stands out and the rest");

    // the main statement counts the lines of the key figures
    const BenchmarkVerdict verdict = getBenchmarkVerdict(c);
    check(verdict.faster == 1 && verdict.slower == 1 && verdict.same == 1
        && verdict.notComparable == 3
        && overview.Contains("<p class=\"verdict\"><span class=\"v-faster\">1 key figure "
            "faster</span>" + separator + "<span class=\"v-slower\">1 slower</span>"
            + separator + "<span class=\"v-same\">1 about the same</span></p>")
        && countOf(overview, "<tr><td class=\"tagcol ") == 6
        && countOf(overview, "<span class=\"dir faster") == 1 + countOf(overview,
            "<span class=\"so-dir\"><span class=\"dir faster")
        && overview.Contains("3 key figures not compared")
        && overview.Contains("<span class=\"tag-word\">APP</span> measured in the earlier "
            "run only"),
        "the counters agree with the lines; what one run did not measure is named");
    check(overview.Contains("<td class=\"n old\" data-label=\"Earlier run\">0.20</td>")
        && overview.Contains("<td class=\"n new\" data-label=\"Later run\">"
            + wxString(wxUniChar(0x2014)) + "<span class=\"unit\">ms</span>")
        && overview.Contains("<span class=\"badge\">no longer measured</span>")
        && countOf(overview, "<span class=\"badge nc\">not comparable</span>") == 2
        && overview.Contains("1,500<span class=\"delta up-text\">+500</span>")
        && overview.Contains("500<span class=\"delta down-text\">-500</span>")
        && overview.Contains("1,000<span class=\"delta same-text\">" + wxString(wxUniChar(
            0x00B1)) + "0</span>"),
        "a line per key figure: both values, the change in words and the points");

    // what stands out: the parts that differ clearly, the largest first
    const std::vector<const BenchmarkComparedPart*> standOut = getBenchmarkStandOut(c, 5);
    bool listed = !standOut.empty() && countOf(overview, "class=\"so\"") == int(standOut.size());
    size_t previous = 0;
    for (const BenchmarkComparedPart* p : standOut)
    {
        const size_t pos = overview.find(">" + getBenchmarkReportSectionName(p->category)
            + "</span>", overview.find("id=\"changes\""));
        listed = listed && pos != wxString::npos && pos > previous;
        previous = pos;
        for (const auto& d : p->differences)
            listed = listed && overview.Contains("<li><b>" + d.label + "</b>: ");
        for (const auto& h : p->hints)
            listed = listed && overview.Contains("<li><b>" + h.title + "</b>");
        for (const auto& a : p->advice)
            listed = listed && overview.Contains("<li><b>" + a.first + "</b>: ");
        if (!getBenchmarkTryAdvice(*p).empty())
            listed = listed && overview.Contains("What to try</span>");
    }
    check(listed && standOut.size() <= 5,
        "what stands out: in the order of the change, with every possible reason");
    const BenchmarkComparedPart* users = c.findPart(BenchmarkCategory::MultiUser);
    check(users && users->ratio < 1.0 && overview.Contains("<span class=\"so-key\">What to "
            "try</span>")
        && overview.Contains("What could make the later run slower"),
        "a slower part: what could make it slower and what to try");

    // every value of both runs, with its state
    check(countOf(details, "<tr class=\"same\">") + countOf(details, "<tr class=\"chg\">")
            == int(c.values.size() + c.stats.size())
        && details.Contains("<span class=\"badge\">new</span>")
        && details.Contains("Trimming the disk")
        && details.Contains("<small>The test changed (version 2, earlier version 1).</small>")
        && details.Contains("<small>Other work: 60,000 rows, earlier 50,000 rows.</small>")
        && details.Contains("<span class=\"badge\">only in the earlier run</span>")
        && details.Contains("id=\"f1\" checked>") && details.Contains("<label for=\"f2\">Only "
            "changes</label>")
        && countOf(details, "<div class=\"why-block\">") == int(std::count_if(c.parts.begin(),
            c.parts.end(), [](const BenchmarkComparedPart& p)
            { return p.differences.size() + p.hints.size() + p.advice.size() > 0; })),
        "every value: new, no longer measured, test changed, other work");
    const BenchmarkComparison same = compareBenchmarkSnapshots(older, older);
    const wxString sameHtml = same.toHtml();
    check(sameHtml.Contains("<p class=\"verdict\"><span class=\"v-same\">6 key figures about "
            "the same</span></p>")
        && !sameHtml.Contains("id=\"changes\"") && !sameHtml.Contains("<tr class=\"chg\">")
        && countOf(sameHtml, " unchanged\">") > 0 && findLegacyProblem(sameHtml).empty(),
        "a run compared with itself: nothing stands out, every part unchanged");

    // a report with an earlier run: three views
    BenchmarkReport report = makeReport(healthyMetrics(), "2026-10-05 12:00:00");
    report.earlier = older;
    const wxString withEarlier = report.toHtml();
    check(withEarlier.Contains("<div class=\"pages\"><label for=\"v1\">Overview</label><label "
            "for=\"v3\">Detailed comparison</label><label for=\"v2\">Detailed report</label>")
        && withEarlier.Contains("id=\"v3\">") && withEarlier.Contains("Compared with the "
            "earlier run ")
        && withEarlier.Find("<div class=\"overview\">") < withEarlier.Find("<div "
            "class=\"cmp-details\">")
        && withEarlier.Find("<div class=\"cmp-details\">") < withEarlier.Find("<div "
            "class=\"details\">")
        && !withEarlier.Contains("id=\"figures\"") && withEarlier.Contains("This run")
        && findLegacyProblem(withEarlier).empty(),
        "a report with an earlier run: overview, detailed comparison, detailed report");
    check(!c.toHtml().Contains("<use href") && countOf(html, "<svg class=\"sprite\"") == 1,
        "the pictograms of a comparison");
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
    m.serverConfig["RemoteBindAddress"] = "172.20.1.9";
    m.serverConfigExplicit.insert("RemoteBindAddress");
    m.versionList = { "WI-V5.0.4.1812 Firebird 5.0",
        "WI-V5.0.4.1812 Firebird 5.0/tcp (DbServer01.kunde.local)/P19:C" };
    m.securityDatabase = "D:\\Kunde\\Sicher\\security5.fdb";
    m.system.fullHostName = "PC-MUELLER.kunde.local";
    m.system.osUser = "KUNDE\\mueller";
    m.system.cpuName = "Intel Core i7-13800H";
    m.system.addresses = { { "Ethernet", "Ethernet", "192.168.10.70" } };
    m.system.firebirdServers = { { "FirebirdServerKundeX", "C:\\Program Files\\KundeX\\fb",
        true } };
    m.system.listeningPorts = { { 3050, "all addresses", "firebird.exe",
        "allowed by \"Kunde X Firebird\"" }, { 3051, "10.77.1.2", "firebird.exe", "" },
        { 3052, "127.0.0.1", "firebird.exe", "" } };
    m.system.otherDatabaseServers = { { "MSSQL$KUNDEDB", "", true } };
    m.networkPath = BenchmarkNetworkPath();
    m.networkPath->serverHost = "dbserver01";
    m.networkPath->serverAddress = "192.168.10.5";
    m.networkPath->serverAddresses = { "192.168.10.5" };
    m.networkPath->localAddress = "192.168.10.77";
    m.networkPath->adapterType = "Ethernet";
    // free texts, in another case than the names
    m.system.firewallNote = "rule Allow dbServer01 is off";
    m.system.mountOptions = "addr=192.168.10.5";
    m.system.osSettings = { { "Shares", "\\\\pc-mueller\\daten" } };
    // a field that anonymized() does not know
    m.system.virtualization = "Hyper-V on Pc-Mueller";
    m.tempDirFileMs = 4.0;
    report.evaluate();
    check(findHint(report.hints, "A router or VPN sits between") != nullptr,
        "the test has a hint that names an address");
    // an earlier run of another machine
    BenchmarkSnapshot earlier = makeBenchmarkSnapshot(report);
    earlier.kind = "local";
    earlier.host = "SRV-ALT";
    earlier.title = "SRV-ALT";
    earlier.facts.push_back({ "system", "SRV-ALT" });
    earlier.facts.push_back({ "conf.RemoteBindAddress", "172.31.5.6" });
    earlier.hints.push_back({ "network.nat", BenchmarkCategory::Network, "A router or VPN "
        "sits between this computer and the server", "The server sees this machine as "
        "172.31.5.7", "", "" });
    report.earlier = earlier;

    const BenchmarkReport shared = report.anonymized();
    const wxString html = shared.toHtml();
    const wxString md = shared.toMarkdown();
    bool clean = true;
    // without regard to case
    const wxString lowerHtml = html.Lower();
    const wxString lowerMd = md.Lower();
    for (const char* secret : { "dbserver01", "pc-mueller", "mueller", "kunde.local", "kunde",
        "192.168.10", "buchhaltung", "firebirdtemp", "appdata", "172.20.1.9", "10.77.1.2",
        "kundedb", "srv-alt", "172.31.5.6", "172.31.5.7", "sicher", "security5.fdb" })
    {
        if (lowerHtml.Contains(secret) || lowerMd.Contains(secret))
        {
            std::cerr << "    still contains: " << secret << "\n";
            clean = false;
        }
    }
    check(clean, "no names, addresses, users, paths, services or rule names");
    // the notes of the overview name what was noticed, not what was measured
    auto escaped = [](wxString text)
    {
        text.Replace("&", "&amp;");
        text.Replace("<", "&lt;");
        text.Replace(">", "&gt;");
        text.Replace("\"", "&quot;");
        text.Replace("'", "&#39;");
        return text;
    };
    bool noDetails = !shared.hints.empty();
    for (const BenchmarkHint& h : shared.hints)
        noDetails = noDetails && (h.detail.empty() || !html.Contains(escaped(h.detail)));
    BenchmarkReport single = shared;
    single.earlier.reset();
    const wxString singleHtml = single.toHtml();
    for (const BenchmarkHint& h : shared.hints)
        noDetails = noDetails && (h.detail.empty() || !singleHtml.Contains(escaped(h.detail)));
    check(noDetails && html.Contains("<li><b>The server sees this computer under another "
        "address</b>") && singleHtml.Contains("<span class=\"note-title\">The server sees "
        "this computer under another address</span>"), "the notes without what was measured");
    check(shared.anonymous && html.Contains("have been removed from this report")
        && md.Contains("have been removed from this report"), "the report says so");
    check(html.Contains("Intel Core i7-13800H") && html.Contains("Firebird 5.0.4")
        && shared.hints.size() == report.hints.size()
        && shared.parts.size() == report.parts.size()
        && shared.results.size() == report.results.size(),
        "hardware, versions, measurements and hints stay");
    check(html.Contains("127.0.0.1"), "the loopback address stays");
    check(shared.benchmarkDatabasePath == "FR_BENCHMARK_20261004_102138_189ABC.FDB",
        "the temporary database keeps only its file name");
    check(!report.anonymous && report.metrics.connectionHost == "dbserver01",
        "the original report is unchanged");
    check(shared.metrics.system.virtualization == "Hyper-V on Pc-Mueller"
        && html.Contains("Hyper-V on " + shared.metrics.localHostName)
        && md.Contains("Hyper-V on " + shared.metrics.localHostName),
        "a name in a field that anonymized() does not know goes from the finished report");

    // names that the report uses as words stay out of the finished report
    BenchmarkReport common = report;
    common.metrics.system.osUser = "WORKGROUP\\Administrator";
    const BenchmarkReport sharedCommon = common.anonymized();
    check(std::none_of(sharedCommon.hiddenTexts.begin(), sharedCommon.hiddenTexts.end(),
        [](const auto& h) { return h.first.CmpNoCase("Administrator") == 0
            || h.first.CmpNoCase("WORKGROUP") == 0; })
        && std::any_of(sharedCommon.hiddenTexts.begin(), sharedCommon.hiddenTexts.end(),
            [](const auto& h) { return h.first == "WORKGROUP\\Administrator"; }),
        "common words such as Administrator are not replaced in the whole report");
}

void testHiddenTexts()
{
    std::cout << "Private texts in a finished report\n";
    const std::vector<std::pair<wxString, wxString>> hidden = { { "server01", "server" },
        { "A&B-PC", "<hidden>" }, { "DEV", "client" } };
    const wxString html = hideBenchmarkTexts("<p class=\"server01\" aria-label=\"Server01 "
        "chart\">SERVER01 and a&amp;b-pc</p><style>.server01 { color: red; }</style>"
        "<!-- host = Server01 -->", hidden, true);
    check(html == "<p class=\"server01\" aria-label=\"server chart\">server and &lt;hidden&gt;"
        "</p><style>.server01 { color: red; }</style><!-- host = server -->",
        "in texts, attribute values and comments, without regard to case, not in classes "
        "and styles");
    check(hideBenchmarkTexts("dev, DEV-PC, devices, Dev_1", hidden, false)
        == "client, client-PC, devices, Dev_1", "short names only as words of their own");
    check(hideBenchmarkTexts("Server01.local", hidden, false) == "server.local"
        && hideBenchmarkTexts("text", {}, true) == "text", "longer names also within words");
}

BenchmarkReport makeReport(const BenchmarkMetrics& m, const wxString& date)
{
    BenchmarkReport report;
    report.target = "Server of TEST";
    report.intensityName = "Standard";
    report.startedAt = date;
    report.metrics = m;
    report.evaluate();
    return report;
}

bool sameMeasurements(const BenchmarkSnapshot& a, const BenchmarkSnapshot& b)
{
    if (a.measurements.size() != b.measurements.size())
        return false;
    for (size_t i = 0; i < a.measurements.size(); ++i)
    {
        const BenchmarkMeasurement& x = a.measurements[i];
        const BenchmarkMeasurement& y = b.measurements[i];
        if (x.id != y.id || x.unit != y.unit || x.version != y.version
            || x.amountUnit != y.amountUnit || x.title != y.title || x.text != y.text
            || std::abs(x.value - y.value) > 1e-6 * x.value
            || std::abs(x.amount - y.amount) > 1e-6 * x.amount
            || std::abs(x.ms - y.ms) > 1e-6 * x.ms)
        {
            std::cerr << "    differs: " << x.id.utf8_str() << " / " << y.id.utf8_str() << "\n";
            return false;
        }
    }
    return true;
}

const BenchmarkComparedValue* findCompared(const BenchmarkComparison& c, const wxString& id)
{
    for (const auto* list : { &c.values, &c.stats })
    {
        for (const BenchmarkComparedValue& v : *list)
        {
            if (v.id == id)
                return &v;
        }
    }
    return nullptr;
}

void testSnapshotFormat()
{
    std::cout << "Saved results (format 2)\n";
    using C = BenchmarkCategory;
    BenchmarkSnapshot s;
    s.program = "26.10.0";
    s.title = "HP ZBook";
    s.host = "workstation";
    s.date = "2026-10-04 12:00:00";
    s.kind = "client";
    s.intensity = "standard";
    BenchmarkMeasurement insert;
    insert.id = "storage.insert";
    insert.value = 1234.5;
    insert.unit = "rows/s";
    insert.version = 1;
    insert.amount = 1234567;
    insert.amountUnit = "rows";
    insert.ms = 0.0125;
    insert.title = "Writing rows";
    insert.text = "1,235 rows/s";
    BenchmarkMeasurement dns;
    dns.id = "network.dns";
    dns.version = 2;
    dns.title = "Finding the server by its name (DNS)";
    dns.text = "1.00 ms";
    BenchmarkMeasurement level = insert;
    level.id = "users.tx.8";
    level.value = 1234567.5;
    level.unit = "tx/s";
    s.measurements = { insert, dns, level };
    s.facts = { { "os", "Windows 11" } };
    s.hints = { { "power.saving", C::ServerCpu, "Power saving", "detail", "advice", "" } };
    const wxString text = formatBenchmarkSnapshot(s);
    check(text.Contains("\nformat = 2.0\nprogram = 26.10.0\nrun.title = HP ZBook\n")
        && text.Contains("\nm.storage.insert = 1234.5 | rows/s | v1 | 1234567 | rows | 0.0125 | "
            "Writing rows | 1,235 rows/s\n")
        && text.Contains("\nm.network.dns =  |  | v2 |  |  |  | Finding the server by its "
            "name (DNS) | 1.00 ms\n")
        && text.Contains("\nm.users.tx.8 = 1234568 | tx/s | v1 | 1234567 | ")
        && text.Contains("\nnote.power.saving = cpu | Power saving | detail | advice\n"),
        "numbers with a point, without thousands separators and exponent, at most six digits");
    const BenchmarkSnapshotResult back = parseBenchmarkSnapshot("# report\n\n" + text);
    BenchmarkSnapshot rounded = s;
    rounded.measurements[2].value = 1234568;
    check(back.ok() && back.format == "2.0" && back.snapshot.program == "26.10.0"
        && back.snapshot.host == "workstation" && back.snapshot.kind == "client"
        && back.snapshot.intensity == "standard" && sameMeasurements(back.snapshot, rounded)
        && back.snapshot.facts == s.facts && back.snapshot.hints.size() == 1
        && back.snapshot.hints[0].id == "power.saving"
        && back.snapshot.hints[0].category == C::ServerCpu,
        "the snapshot is read back as it was written");
    check(formatBenchmarkSnapshot(back.snapshot) == formatBenchmarkSnapshot(rounded),
        "writing it again gives the same text");
    {   // scope
        // the decimal separator of the language must not matter
        const char* const german[] = { "de_DE.UTF-8", "de_DE", "German_Germany.1252" };
        bool set = false;
        for (const char* locale : german)
            set = set || std::setlocale(LC_NUMERIC, locale) != nullptr;
        const wxString localized = formatBenchmarkSnapshot(s);
        std::setlocale(LC_NUMERIC, "C");
        if (!set)
            std::cout << "    (no German locale to test with)\n";
        check(localized == text, "the same numbers in a German locale");
    }
    BenchmarkSnapshot dirty = s;
    dirty.title = "A | B -- C\nD";
    check(formatBenchmarkSnapshot(dirty).Contains("\nrun.title = A / B - - C D\n")
        && formatBenchmarkSnapshot(dirty).Freq('\n') == text.Freq('\n'),
        "a text never ends a line, a field or the comment");

    // versions of the format
    const wxString head = "<!-- FlameRobin benchmark snapshot (used by FlameRobin to "
        "compare reports)\n";
    const wxString oldHead = "<!-- FlameRobin benchmark results (used by FlameRobin to "
        "compare reports)\n";
    const BenchmarkSnapshotResult first = parseBenchmarkSnapshot(oldHead
        + "format = 1\ntitle = old\nvalue.commitMs = 1.5\nstat = storage |  | Saving | "
        "1.50 ms | 200 transactions | 300.000\n-->\n");
    BenchmarkReport localized = makeReport(healthyMetrics(), "2026-10-04 12:00:00");
    localized.intensity = BenchmarkIntensity::Quick;
    localized.intensityName = "Schnell";
    const wxString localizedMd = localized.toMarkdown();
    check(!localizedMd.Contains("FlameRobin benchmark results")
        && !localized.toHtml("dark").Contains("FlameRobin benchmark results")
        && parseBenchmarkSnapshot(localizedMd).snapshot.intensity == "quick",
        "no report contains the marker of format 1, so FlameRobin 26.10.0 finds no results; "
        "the level comes from the enum, not from the translated name");
    check(first.status == BenchmarkSnapshotStatus::TooOld && first.format == "1"
        && getBenchmarkSnapshotProblem(first).Contains("older version")
        && getBenchmarkSnapshotProblem(first).Contains("format 1"),
        "a report of FlameRobin 26.10.0 (format 1) is refused with a message");
    const BenchmarkSnapshotResult future = parseBenchmarkSnapshot(head + "format = 3.0\n"
        "run.title = new\nm.storage.commit = 1.5 | ms | v1\n-->\n");
    check(future.status == BenchmarkSnapshotStatus::TooNew && future.format == "3.0"
        && getBenchmarkSnapshotProblem(future).Contains("newer version")
        && getBenchmarkSnapshotProblem(future).Contains("3.0")
        && future.snapshot.measurements.empty(),
        "a report of a newer major format is refused with a message");
    const BenchmarkSnapshotResult minor = parseBenchmarkSnapshot(head + "format = 2.5\n"
        "run.title = later\nrun.weather = sunny\nnew.kind.of.line = 1\n"
        "m.storage.commit = 1.5 | ms | v1 | 200 | commits | 300 | Saving | 1.50 ms | x | y\n"
        "m.gpu.render = 60 | frames/s | v1 |  |  |  | Rendering | 60 frames/s\n"
        "note.new.kind = cpu | Title | Detail | Advice | Extra\n"
        "a line without equals sign\n-->\n");
    const BenchmarkMeasurement* commit = minor.snapshot.findMeasurement("storage.commit");
    const BenchmarkMeasurement* render = minor.snapshot.findMeasurement("gpu.render");
    check(minor.ok() && minor.snapshot.title == "later" && commit && commit->value == 1.5
        && commit->amount == 200 && commit->amountUnit == "commits" && commit->ms == 300
        && commit->text == "1.50 ms" && render && render->title == "Rendering"
        && minor.snapshot.hints.size() == 1 && minor.snapshot.hints[0].advice == "Advice",
        "a newer minor format: unknown lines and fields are skipped, unknown ids kept");
    check(parseBenchmarkSnapshot(head + "run.title = x\n-->\n").status
            == BenchmarkSnapshotStatus::Damaged
        && parseBenchmarkSnapshot(head + "format = two\n-->\n").status
            == BenchmarkSnapshotStatus::Damaged
        && parseBenchmarkSnapshot(head + "format = 2.0\nrun.title = cut off\n").status
            == BenchmarkSnapshotStatus::Damaged
        && !getBenchmarkSnapshotProblem(parseBenchmarkSnapshot(head + "-->")).empty()
        && parseBenchmarkSnapshot("# notes").status == BenchmarkSnapshotStatus::NotAReport,
        "results without a format or without an end are damaged");
    const BenchmarkSnapshotResult invalid = parseBenchmarkSnapshot(head + "format = 2.0\n"
        "m.storage..insert = 1 | rows/s | v1\nm.storage.insert = abc | rows/s | v1\n"
        "m.storage.update = 1 | rows/s | 1\nm.storage.delete = -5 | rows/s | v1\n"
        "m.storage.gc = 1 | ms | v0\nm.storage.create = 2 | ms | v1\n"
        "m.storage.create = 3 | ms | v1\nnote.x = cpu | a | b | c\n-->\n");
    check(invalid.ok() && invalid.snapshot.measurements.size() == 1
        && invalid.snapshot.measurements[0].value == 2 && invalid.snapshot.hints.empty(),
        "lines that cannot be read are skipped, an id counts once");

    // a real report, also with Windows line ends
    BenchmarkMetrics metrics = healthyMetrics();
    metrics.clientBurstIdleMs = 25.0;
    metrics.skippedTests.push_back({ "application.blob", "Writing / reading large data "
        "(BLOBs)", "not supported" });
    metrics.blobReadMs.reset();
    const BenchmarkReport report = makeReport(metrics, "2026-10-04 12:00:00");
    const wxString md = report.toMarkdown();
    const BenchmarkSnapshotResult fromMd = parseBenchmarkSnapshot(md);
    const BenchmarkSnapshotResult fromHtml = parseBenchmarkSnapshot(report.toHtml("dark"));
    wxString crlf = md;
    crlf.Replace("\n", "\r\n");
    check(fromMd.ok() && fromHtml.ok() && sameMeasurements(fromMd.snapshot, fromHtml.snapshot)
        && sameMeasurements(fromMd.snapshot, parseBenchmarkSnapshot(crlf).snapshot)
        && fromMd.snapshot.measurements.size() > 30,
        "Markdown and HTML carry the same results, also with Windows line ends");
    const BenchmarkSnapshot& saved = fromMd.snapshot;
    const BenchmarkMeasurement* oltp = saved.findMeasurement("users.oltp");
    const BenchmarkMeasurement* blob = saved.findMeasurement("application.blob");
    const BenchmarkMeasurement* update = saved.findMeasurement("storage.update");
    check(oltp && oltp->value == 6000 && oltp->amount == 30000 && oltp->ms == 5000
        && oltp->amountUnit == "transactions" && oltp->title == "Transactions, up to 8 users"
        && update && std::abs(update->value / (50000 * 1000.0 / 650.0) - 1.0) < 1e-5
        && update->amount == 50000 && update->amountUnit == "rows" && update->ms == 650
        && blob && blob->value == 0 && blob->text == "not measured"
        && saved.findMeasurement("users.tx.8") && !saved.findMeasurement("settings.runsOn"),
        "key values, other values with their work, skipped tests; no settings");

    // a computer or user name in a shared report never changes the results
    const std::vector<std::pair<wxString, wxString>> names = { { "STORAGE", "<hidden>" },
        { "rows", "<hidden>" }, { "ms", "<user>" } };
    const BenchmarkSnapshotResult hiddenMd = parseBenchmarkSnapshot(
        hideBenchmarkTexts(md, names, false));
    const BenchmarkSnapshotResult hiddenHtml = parseBenchmarkSnapshot(
        hideBenchmarkTexts(report.toHtml(), names, true));
    bool numbersKept = hiddenMd.ok() && hiddenHtml.ok()
        && hiddenMd.snapshot.measurements.size() == saved.measurements.size();
    for (size_t i = 0; numbersKept && i < saved.measurements.size(); ++i)
    {
        for (const BenchmarkSnapshot* other : { &hiddenMd.snapshot, &hiddenHtml.snapshot })
        {
            const BenchmarkMeasurement& x = saved.measurements[i];
            const BenchmarkMeasurement& y = other->measurements[i];
            numbersKept = numbersKept && x.id == y.id && x.value == y.value
                && x.unit == y.unit && x.amountUnit == y.amountUnit && x.ms == y.ms;
        }
    }
    check(numbersKept && hiddenMd.snapshot.findMeasurement("storage.update")->title
        == "Changing many <hidden>", "names are hidden in the texts, not in ids and units");

    // measurements are matched by their ids, never by their titles
    BenchmarkSnapshot translated = saved;
    for (BenchmarkMeasurement& x : translated.measurements)
    {
        x.title = "Titel " + x.id;
        x.text = "anders";
    }
    const BenchmarkComparison other = compareBenchmarkSnapshots(translated, saved);
    const BenchmarkComparedValue* updated = findCompared(other, "storage.update");
    check(std::abs(other.ratio - 1.0) < 1e-9 && updated
        && std::abs(updated->ratio - 1.0) < 1e-9 && updated->title == "Titel storage.update",
        "the same ids with other titles (another language) are compared");
    BenchmarkSnapshot renamed = saved;
    for (BenchmarkMeasurement& x : renamed.measurements)
    {
        if (x.id == "storage.update")
            x.id = "storage.rewrite";
    }
    const BenchmarkComparison moved = compareBenchmarkSnapshots(renamed, saved);
    const BenchmarkComparedValue* gone = findCompared(moved, "storage.update");
    const BenchmarkComparedValue* added = findCompared(moved, "storage.rewrite");
    check(gone && added && gone->ratio == 0.0 && added->ratio == 0.0
        && gone->newerText == wxString(wxUniChar(0x2013)) && gone->title == added->title
        && added->olderText == wxString(wxUniChar(0x2013))
        && gone->state == BenchmarkCompareState::OnlyOlder
        && added->state == BenchmarkCompareState::OnlyNewer,
        "another id with the same title is another measurement");
}

void testCompareStates()
{
    std::cout << "Comparison by id and test version\n";
    using S = BenchmarkCompareState;
    const BenchmarkSnapshot older = makeBenchmarkSnapshot(makeReport(healthyMetrics(),
        "2026-10-04 12:00:00"));
    const BenchmarkComparison same = compareBenchmarkSnapshots(older, older);
    bool allCompared = !same.values.empty() && !same.stats.empty();
    for (const auto* list : { &same.values, &same.stats })
    {
        for (const BenchmarkComparedValue& v : *list)
        {
            const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(v.id);
            const bool number = def && def->kind != BenchmarkMeasurementKind::Info;
            allCompared = allCompared && v.state == S::Compared && v.newerVersion == 1
                && v.olderVersion == 1 && (number ? v.ratio == 1.0 : v.ratio == 0.0);
        }
    }
    check(allCompared, "a run compared with itself: every value compared, ratios 1");

    auto change = [](BenchmarkSnapshot s, const wxString& id,
        const std::function<void(BenchmarkMeasurement&)>& edit)
    {
        for (BenchmarkMeasurement& x : s.measurements)
        {
            if (x.id == id)
                edit(x);
        }
        return s;
    };
    // a new version of the commit test: not comparable, not in the points
    BenchmarkSnapshot newer = change(older, "storage.commit", [](BenchmarkMeasurement& x)
        {
            x.version = 2;
            x.value /= 4.0;
        });
    newer = change(newer, "storage.update", [](BenchmarkMeasurement& x)
        {
            x.version = 2;
            x.value *= 4.0;
        });
    const BenchmarkComparison changed = compareBenchmarkSnapshots(newer, older);
    const BenchmarkComparedValue* commit = findCompared(changed, "storage.commit");
    const BenchmarkComparedValue* update = findCompared(changed, "storage.update");
    check(commit && commit->state == S::TestChanged && commit->ratio == 0.0
        && commit->newerVersion == 2 && commit->olderVersion == 1
        && update && update->state == S::TestChanged && update->ratio == 0.0
        && std::abs(changed.findPart(BenchmarkCategory::ServerStorage)->ratio - 1.0) < 1e-9
        && std::abs(changed.ratio - 1.0) < 1e-9,
        "another version of a test: not comparable, not in the points");

    // the same intensity with other work: the table of the intensities
    // changed without a new version of the test
    newer = change(older, "storage.insert", [](BenchmarkMeasurement& x)
        {
            x.amount = 60000;
            x.ms *= 1.2;
        });
    newer = change(newer, "storage.delete", [](BenchmarkMeasurement& x) { x.amount = 60000; });
    newer = change(newer, "storage.driveInsert", [](BenchmarkMeasurement& x)
        {
            x.amount *= 2.0;
            x.value *= 2.0;
        });
    newer = change(newer, "users.tx.8", [](BenchmarkMeasurement& x)
        {
            x.amount *= 1.5;
            x.value *= 1.5;
        });
    const BenchmarkComparison work = compareBenchmarkSnapshots(newer, older);
    const BenchmarkComparedValue* insert = findCompared(work, "storage.insert");
    const BenchmarkComparedValue* removed = findCompared(work, "storage.delete");
    const BenchmarkComparedValue* drive = findCompared(work, "storage.driveInsert");
    const BenchmarkComparedValue* level = findCompared(work, "users.tx.8");
    check(insert && insert->state == S::WorkChanged && insert->ratio == 0.0
        && removed && removed->state == S::WorkChanged && removed->ratio == 0.0,
        "other work of a test with fixed work: not comparable");
    // a commit faster than the round trips it was corrected by has no work
    const BenchmarkSnapshot instant = change(older, "storage.commit",
        [](BenchmarkMeasurement& x)
        {
            x.value = 0.01;
            x.amount = 0.0;
            x.amountUnit.clear();
            x.ms = 0.0;
        });
    const BenchmarkComparison faster = compareBenchmarkSnapshots(instant, older);
    const BenchmarkComparedValue* fast = findCompared(faster, "storage.commit");
    check(fast && fast->state == S::Compared && fast->ratio == 150.0,
        "a value without its work is compared by its value");
    check(drive && drive->state == S::Compared && std::abs(drive->ratio - 2.0) < 1e-9
        && level && level->state == S::Compared && std::abs(level->ratio - 1.5) < 1e-9,
        "the tests with a time limit are compared by their rate");
    // another intensity: the key values roughly, the other values not
    BenchmarkSnapshot quick = newer;
    quick.intensity = "quick";
    const BenchmarkComparison intensities = compareBenchmarkSnapshots(quick, older);
    const BenchmarkComparedValue* quickInsert = findCompared(intensities, "storage.insert");
    const BenchmarkComparedValue* quickDelete = findCompared(intensities, "storage.delete");
    check(!intensities.sameKind && quickInsert && quickInsert->state == S::Compared
        && quickInsert->ratio > 0.0 && quickDelete && quickDelete->state == S::WorkChanged,
        "another intensity: key values compared roughly, other values not");

    // new and no longer measured
    newer = older;
    newer.measurements.erase(std::remove_if(newer.measurements.begin(),
        newer.measurements.end(), [](const BenchmarkMeasurement& x)
        { return x.id == "storage.gc" || x.id == "cpu.parallel"; }), newer.measurements.end());
    BenchmarkMeasurement future;
    future.id = "storage.trim";
    future.value = 12;
    future.unit = "ms";
    future.version = 1;
    future.title = "Trimming the disk";
    future.text = "12.00 ms";
    newer.measurements.push_back(future);
    const BenchmarkComparison gone = compareBenchmarkSnapshots(newer, older);
    const BenchmarkComparedValue* gc = findCompared(gone, "storage.gc");
    const BenchmarkComparedValue* parallel = findCompared(gone, "cpu.parallel");
    const BenchmarkComparedValue* trim = findCompared(gone, "storage.trim");
    check(gc && gc->state == S::OnlyOlder && gc->ratio == 0.0
        && parallel && parallel->state == S::OnlyOlder
        && trim && trim->state == S::OnlyNewer && trim->title == "Trimming the disk"
        && trim->category == BenchmarkCategory::ServerStorage && trim->newerText == "12.00 ms",
        "new and no longer measured values are listed, an unknown id with its title");
    const BenchmarkComparedValue* factor = findCompared(same, "memory.coldVsWarm");
    check(factor && factor->state == S::Compared && factor->ratio == 0.0
        && factor->newerText == "1.1x", "information is shown, not compared");

    // anonymized runs give the same ratios and states
    BenchmarkSnapshot hostNewer = newer;
    hostNewer.host = "PC-MUELLER";
    const BenchmarkComparison plain = compareBenchmarkSnapshots(hostNewer, older);
    const BenchmarkComparison shared = anonymizeBenchmarkComparison(plain);
    bool unchanged = plain.values.size() == shared.values.size()
        && plain.stats.size() == shared.stats.size() && plain.ratio == shared.ratio;
    for (size_t i = 0; unchanged && i < plain.stats.size(); ++i)
    {
        unchanged = plain.stats[i].id == shared.stats[i].id
            && plain.stats[i].ratio == shared.stats[i].ratio
            && plain.stats[i].state == shared.stats[i].state;
    }
    check(unchanged, "hiding private information changes no ratio and no state");
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
    std::optional<BenchmarkSnapshot> earlier;
    if (parseBenchmarkSnapshot(md).ok())
        earlier = parseBenchmarkSnapshot(md).snapshot;
    check(earlier && earlier->date == report.startedAt && earlier->kind == "embedded"
        && earlier->intensity == "standard" && earlier->hints.size() == report.hints.size()
        && !earlier->hints.empty(), "a saved report can be read back");
    check(earlier && earlier->findMeasurement("storage.commit")
        && std::abs(earlier->findMeasurement("storage.commit")->value
            - getBenchmarkValues(report.metrics).commitMs) < 0.001, "with its values");
    check(parseBenchmarkSnapshot(report.toHtml()).ok(), "the HTML report as well");
    check(parseBenchmarkSnapshot("# notes\nm.storage.commit = 1\n").status
        == BenchmarkSnapshotStatus::NotAReport, "other files contain no results");
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
        && html.Contains("<span class=\"dir faster\">2.0 times as fast</span>")
        && html.Contains("<th class=\"n\">Earlier run</th><th></th><th class=\"n\">This "
            "run</th>")
        && html.Contains("<p class=\"verdict\">")
        && html.Contains("What could make the earlier run slower")
        && html.Contains("possible reasons</span>"),
        "the HTML report shows the comparison with its reasons");
    check(html.Find("id=\"differences\"") < html.Find("<div class=\"details\">"),
        "the comparison is the overview");
    bool markdown = true;
    for (const char* part : { "### Compared with ", "key figures faster", "Overall ",
        " points; the earlier run has 1,000\n", "| DISK | Saving a change (commit) | ",
        "**What stands out**", "Server disk: 2,000 points (2.0 times as fast)",
        "  - What could make the earlier run slower:\n" })
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
        parseBenchmarkSnapshot(fasterMd).snapshot, *earlier);
    check(findLegacyProblem(html).empty() && findLegacyProblem(files.toHtml("dark")).empty(),
        "comparisons for Internet Explorer 11 as well");
    check(files.toHtml().Contains("Comparison of two benchmark runs")
        && files.toMarkdown().Contains("Server disk: 2,000 points")
        && files.toMarkdown().Contains("Later run")
        && files.toMarkdown().Contains("| Writing many rows | "), "two saved reports");
    std::map<wxString, int> rows;
    for (const auto* list : { &files.values, &files.stats })
    {
        for (const BenchmarkComparedValue& v : *list)
            ++rows[v.id];
    }
    check(std::all_of(rows.begin(), rows.end(), [](const auto& row) { return row.second == 1; })
        && std::any_of(files.stats.begin(), files.stats.end(),
            [](const BenchmarkComparedValue& v)
            { return v.id == "storage.update" && v.title == "Changing many rows"; })
        && std::none_of(files.stats.begin(), files.stats.end(),
            [](const BenchmarkComparedValue& v) { return v.id == "storage.commit"; }),
        "every measurement is compared once, the key values with the key values");

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
    client.hints.push_back({ "network.nat", C::Network, "A router or VPN sits between this "
        "computer and the server", "The server sees this machine as 10.1.2.3",
        "Devices add latency.", "" });
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

    // two saved reports compared: the computer names go everywhere, also
    // where a run names its computer in another case
    BenchmarkSnapshot named = *earlier;
    named.host = "PC-MUELLER";
    named.facts.push_back({ "virtual", "Hyper-V on pc-mueller" });
    BenchmarkComparison twoFiles = compareBenchmarkSnapshots(named, *earlier);
    twoFiles.newerIsCurrentRun = true;
    const BenchmarkComparison sharedFiles = anonymizeBenchmarkComparison(twoFiles);
    check(twoFiles.toMarkdown().Contains("Hyper-V on pc-mueller")
        && twoFiles.toHtml().Contains("Hyper-V on pc-mueller")
        && !sharedFiles.toHtml().Lower().Contains("pc-mueller")
        && !sharedFiles.toMarkdown().Lower().Contains("pc-mueller")
        && sharedFiles.toMarkdown().Contains("Hyper-V on <hidden>")
        && sharedFiles.newerIsCurrentRun && sharedFiles.newer.host.empty(),
        "a shared comparison of two files hides their computer names");
}

// the Markdown report shows what the HTML report shows
void testMarkdown()
{
    std::cout << "Markdown and HTML\n";
    // a single run: the key figures and the notes
    const BenchmarkReport report = makeReport(noticedMetrics()[0], "2026-10-04 12:00:00");
    const wxString html = report.toHtml();
    const wxString md = report.toMarkdown();
    bool same = true;
    for (const BenchmarkHeadline& f : getBenchmarkHeadlineValues(makeBenchmarkSnapshot(report)))
    {
        same = same && md.Contains("| " + getBenchmarkReportSectionWord(f.section) + " | "
            + f.name + " | " + f.number + " " + f.unit + " | " + f.work + " |")
            && html.Contains("<span class=\"fig-num\">" + f.number + "</span><span "
                "class=\"fig-unit\">" + f.unit + "</span>")
            && html.Contains("<span class=\"fig-work\">" + f.work + "</span>");
    }
    for (const BenchmarkHint& h : getBenchmarkTopHints(report, 3))
    {
        const wxString title = h.noticed.empty() ? h.title : h.noticed;
        same = same && md.Contains("** " + title + "\n")
            && html.Contains("<span class=\"note-title\">" + title + "</span>")
            && md.Contains("  - What can help: " + h.advice + "\n");
    }
    const int more = int(getBenchmarkTopHints(report, 100).size()) - 3;
    check(same && more > 0 && md.Contains(wxString::Format("%d more notes are in the HTML "
        "report.", more)) && md.Find("### Key figures") < md.Find("### Noticed")
        && md.Find("### Noticed") < md.Find("### System"),
        "a single run: the same key figures and notes");
    const BenchmarkReport shared = report.anonymized();
    const wxString sharedMd = shared.toMarkdown();
    bool hidden = true;
    for (const BenchmarkHint& h : getBenchmarkTopHints(shared, 3))
        hidden = hidden && sharedMd.Contains(h.noticed.empty() ? h.title : h.noticed)
            && !sharedMd.Contains("  - " + h.detail);
    check(hidden, "a shared report: the notes without what was measured");

    // a comparison: the main statement, the key figures, what stands out
    // and every value
    const BenchmarkComparison c = everyStateComparison();
    const wxString cHtml = c.toHtml();
    const wxString cMd = c.toMarkdown();
    bool compared = true;
    for (const auto& count : describeBenchmarkVerdict(getBenchmarkVerdict(c)))
        compared = compared && cMd.Contains(count.second) && cHtml.Contains(count.second);
    for (const wxString& note : getBenchmarkVerdictNotes(c))
        compared = compared && cMd.Contains(note);
    for (const BenchmarkHeadlineChange& h : getBenchmarkHeadlineChanges(c))
    {
        compared = compared && cMd.Contains(" | " + h.name + " | ")
            && cHtml.Contains("<td class=\"area\">" + h.name + "</td>");
    }
    for (const auto* list : { &c.values, &c.stats })
    {
        for (const BenchmarkComparedValue& v : *list)
        {
            const bool found = cMd.Contains("\n| " + escapeMarkdownTableCell(v.title))
                && cHtml.Contains("<td>" + v.title);
            if (!found)
                std::cerr << "    not in both: " << v.id << "\n";
            compared = compared && found;
        }
    }
    for (const BenchmarkComparedPart* p : getBenchmarkStandOut(c, 5))
    {
        compared = compared && cMd.Contains("- **" + getBenchmarkReportSectionWord(p->category)
            + "** ") && (getBenchmarkLikelyReason(*p).empty()
                || cMd.Contains("  - Likely: " + getBenchmarkLikelyReason(*p) + "\n"))
            && cMd.Contains("  - What to try: " + getBenchmarkTryAdvice(*p) + "\n");
    }
    check(compared, "a comparison: the same statement, key figures, reasons and values");
    check(cMd.Contains(" | not comparable (test changed) | ")
        && cMd.Contains(" | not comparable (other work) | ")
        && cMd.Contains("| Trimming the disk | ") && cMd.Contains(" | new | ")
        && cMd.Contains(" | no longer measured | ")
        && cMd.Contains("(The test changed (version 2, earlier version 1).)")
        && cMd.Contains(" (only in the earlier run)\n")
        && cMd.Contains("| Points |\n|---|---|---:|---:|---|---:|\n"),
        "the states in words, also in Markdown");
}

void testKeyFigures()
{
    std::cout << "Key figures and notes of a single run\n";
    using C = BenchmarkCategory;
    const std::vector<const char*>& ids = getBenchmarkHeadlineIds();
    bool registered = ids.size() == 6;
    BenchmarkCategory previous = C::ServerCpu;
    for (const char* id : ids)
    {
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(id);
        registered = registered && def && def->kind == BenchmarkMeasurementKind::Key
            && !def->family && !def->retired && def->value
            && std::count(ids.begin(), ids.end(), id) == 1;
        // in the order of the sections of the report
        const auto sections = getBenchmarkReportSections();
        registered = registered && def && std::find(sections.begin(), sections.end(),
            def->section) >= std::find(sections.begin(), sections.end(), previous);
        if (def)
            previous = def->section;
    }
    check(registered, "every key figure is a key value of the register, once, in the "
        "order of the report");

    const BenchmarkSnapshot client = makeBenchmarkSnapshot(makeReport(healthyMetrics(),
        "2026-10-04 12:00:00"));
    const std::vector<BenchmarkHeadline> six = getBenchmarkHeadlineValues(client);
    const std::vector<BenchmarkHeadline> five = getBenchmarkHeadlineValues(
        makeBenchmarkSnapshot(makeReport(serverMetrics(), "2026-10-04 12:00:00")));
    check(six.size() == 6 && five.size() == 5
        && std::none_of(five.begin(), five.end(),
            [](const BenchmarkHeadline& h) { return h.section == C::Network; }),
        "six key figures over a network, five for the embedded engine");
    auto figure = [&six](const char* id) -> const BenchmarkHeadline*
    {
        auto it = std::find_if(six.begin(), six.end(),
            [id](const BenchmarkHeadline& h) { return h.id == id; });
        return it == six.end() ? nullptr : &*it;
    };
    const BenchmarkHeadline* commit = figure("storage.commit");
    const BenchmarkHeadline* users = figure("users.oltp");
    const BenchmarkHeadline* insert = figure("storage.insert");
    check(commit && commit->number == "1.50" && commit->unit == "ms"
        && commit->name == "Saving a change (commit)"
        && commit->work == "200 commits in 300 ms"
        && insert && insert->number == "58,824" && insert->unit == "rows/s"
        && insert->name == "Writing rows"
        && users && users->unit == "tx/s" && users->number == "6,000"
        && users->work.StartsWith("30,000 transactions in "),
        "a key figure: a number, a short unit, a short name and the work behind it");
    const BenchmarkMeasurementDef* scan = findBenchmarkMeasurementDef("memory.scan");
    const BenchmarkMeasurementDef* drive = findBenchmarkMeasurementDef("storage.drive");
    check(scan && formatBenchmarkFigure(3086.94, *scan) == std::make_pair(wxString("3,087"),
            wxString("MB/s"))
        && formatBenchmarkFigure(12.345, *scan).first == "12.3"
        && formatBenchmarkFigure(1.5, *scan).first == "1.50"
        && drive && formatBenchmarkFigure(9418.2, *drive).second == "ops/s",
        "large numbers without decimals, small ones with");
    check(getBenchmarkHeadlineValues(BenchmarkSnapshot()).empty(),
        "no key figures without values");

    // short words for the sections, the same in every report
    std::set<wxString> words;
    for (BenchmarkCategory section : getBenchmarkReportSections())
        words.insert(getBenchmarkReportSectionWord(section));
    check(words.size() == getBenchmarkReportSections().size() && !words.count(wxString())
        && getBenchmarkReportSectionWord(C::ServerStorage) == "DISK"
        && getBenchmarkReportSectionWord(C::Connection) == "NET"
        && getBenchmarkReportSectionWord(C::Engine) == "CONFIG",
        "every section has its own short word");

    // the notes: every kind once, at most three in the overview
    BenchmarkMetrics noticed = serverMetrics();
    noticed.clientBurstIdleMs = 25.0;
    noticed.system.antivirus = { "Windows Defender (on)" };
    noticed.system.storagePathExcluded = false;
    noticed.system.virtualization = "VMware";
    noticed.benchmarkDatabase.cachePages = 2048;
    BenchmarkReport report = makeReport(noticed, "2026-10-04 12:00:00");
    const std::vector<BenchmarkHint> top = getBenchmarkTopHints(report, 3);
    const std::vector<BenchmarkHint> all = getBenchmarkTopHints(report, 100);
    std::set<wxString> kinds;
    for (const BenchmarkHint& h : report.hints)
        kinds.insert(h.id);
    check(report.hints.size() > all.size() && all.size() == kinds.size() && top.size() == 3
        && top[0].id == report.hints[0].id && top[0].id == "power.saving"
        && top[1].id == all[1].id && top[2].id == all[2].id,
        "every kind of note once, in the order found, at most three");
}

// metrics on which every check of BenchmarkAnalyzer.cpp finds something
std::vector<BenchmarkMetrics> noticedMetrics()
{
    BenchmarkMetrics server = serverMetrics();
    server.clientBurstIdleMs = 25.0;
    server.localOnBattery = true;
    server.serverBurstIdleMs = 75.0;
    server.system.antivirus = { "Windows Defender (on)" };
    server.databaseDirFileMs = 4.0;
    server.system.storagePathExcluded = false;
    server.parallelCpuMs = 280.0;
    server.system.virtualization = "VMware";
    server.system.firebirdServers = { { "A", "", true }, { "B", "", true } };
    server.system.otherDatabaseServers = { { "MSSQLSERVER", "", true } };
    server.commitMs = 20.0;
    server.osSyncWriteMs = 0.5;
    server.tempSortMs = 20000.0;
    server.system.storagePath = "D:\\db";
    server.system.storageHealthWarning = true;
    server.system.storageHealth = { { "Media and data integrity errors", "12", true } };
    server.system.storageMedia = "HDD";
    server.system.writeCacheEnabled = false;
    server.databaseDiskFreeBytes = int64_t(1) * 1024 * 1024 * 1024;
    server.benchmarkDatabase.cachePages = 2048;
    server.coldScanMs = 2000.0;
    server.warmScanMs = 100.0;
    server.serverSortMs = 800.0;
    server.system.memoryTotalBytes = int64_t(32) * 1024 * 1024 * 1024;
    server.system.memoryAvailableBytes = int64_t(1) * 1024 * 1024 * 1024;
    server.oltp.back().transactions = 6000;
    server.oltp.back().conflicts = 1000;
    server.oltp.push_back({ 250, 4000, 0, 5.0, 300.0, 600.0 });
    BenchmarkMetrics plan = healthyMetrics();
    plan.system.powerPlan = "Balanced";
    plan.serverConfig["ServerMode"] = "Classic";
    plan.serverBurstIdleMs = 75.0;
    BenchmarkMetrics client = healthyMetrics();
    client.latencySparseMs = 1.2;
    client.latencyP95Ms = 2.0;
    client.streamClientMs = 500.0;
    client.clientVersion = "WI-V3.0.10.33601 Firebird 3.0";
    client.latencyOutliers = 10;
    client.networkPath = BenchmarkNetworkPath();
    client.networkPath->adapterType = "Wi-Fi";
    client.networkPath->linkSpeedBitsPerSecond = 1000000000;
    client.remoteAddress = "10.0.0.5/51234";
    client.system.addresses = { { "WLAN", "Wi-Fi", "192.168.178.58" } };
    client.serverPorts = { { 3050, BenchmarkPortCheck::State::Open, 2.0 } };
    client.serverPort = 3050;
    client.connectMs = 400.0;
    BenchmarkMetrics vpn = client;
    vpn.networkPath->adapterType = "VPN";
    vpn.streamMs = 10000.0;
    return { server, plan, client, vpn };
}

// a key value from the work saved with it: the work per time, or the time
// of one operation
double valueOfWork(const BenchmarkMeasurement& x)
{
    if (x.unit == "ms")
        return x.ms / x.amount;
    if (x.unit == "it/ms")
        return x.amount / x.ms;
    if (x.unit == "MB/s")
        return x.amount / (1024.0 * 1024.0) / (x.ms / 1000.0);
    return x.amount * 1000.0 / x.ms;
}

void testKeyValueWork()
{
    std::cout << "Key values and the work behind them\n";
    // the saved results of runs with every key value, read back
    bool matching = true;
    int checked = 0;
    for (const BenchmarkMetrics& m : { healthyMetrics(), fullMetrics(), serverMetrics() })
    {
        const BenchmarkSnapshotResult saved = parseBenchmarkSnapshot(makeReport(m,
            "2026-10-04 12:00:00").toMarkdown());
        for (const BenchmarkMeasurement& x : saved.snapshot.measurements)
        {
            const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(x.id);
            if (!def || def->kind != BenchmarkMeasurementKind::Key || x.amount <= 0.0
                || x.ms <= 0.0)
            {
                continue;
            }
            ++checked;
            if (std::abs(valueOfWork(x) - x.value) > 1e-4 * x.value)
            {
                std::cerr << "    the work does not give the value: " << x.id << "\n";
                matching = false;
            }
        }
    }
    check(matching && checked >= 35, "every key value is the work saved with it per time");
    // the round trip is the median: a few slow round trips change the mean
    BenchmarkMetrics lost = healthyMetrics();
    lost.latencyAvgMs = 2.0;
    const BenchmarkSnapshot run = makeBenchmarkSnapshot(makeReport(lost, "2026-10-04 12:00:00"));
    const BenchmarkMeasurement* roundTrip = run.findMeasurement("network.roundTrip");
    const BenchmarkSnapshot embedded = makeBenchmarkSnapshot(makeReport(serverMetrics(),
        "2026-10-04 12:00:00"));
    check(roundTrip && roundTrip->value == 0.2 && roundTrip->amount == 2500
        && std::abs(roundTrip->ms - 500.0) < 1e-9
        && formatBenchmarkWork(formatBenchmarkAmount(roundTrip->amount, roundTrip->amountUnit),
            roundTrip->ms) == "2,500 requests in 500 ms"
        && !embedded.findMeasurement("network.roundTrip"),
        "the work of the round trip is the median time of the requests");
}

void testNeutralNotes()
{
    std::cout << "Notes of a single run without a judgement\n";
    std::map<wxString, wxString> titles;
    for (const BenchmarkMetrics& m : noticedMetrics())
    {
        for (const BenchmarkHint& h : findBenchmarkHints(m))
            titles[h.id] = h.noticed.empty() ? h.title : h.noticed;
    }
    // every check of the analyzer: power and pauses (5), virus scanners (2),
    // processor and other servers (4), storage (6), memory (5), network (9),
    // users (3)
    bool neutral = titles.size() == 34;
    if (!neutral)
        std::cerr << "    " << titles.size() << " kinds of notes\n";
    for (const auto& t : titles)
    {
        const wxString lower = t.second.Lower();
        for (const char* judgement : { "slow", "little", "much", "lost", "old ", "problem",
            "do not", "too ", "most", "saves energy", "vary a lot", "may " })
        {
            if (lower.Contains(judgement))
            {
                std::cerr << "    " << t.first << ": " << t.second << "\n";
                neutral = false;
            }
        }
    }
    check(neutral, "every note names a state or a ratio, no cause and no judgement");

    // the neutral titles are not saved: the saved results stay as they were
    BenchmarkReport report = makeReport(noticedMetrics()[0], "2026-10-04 12:00:00");
    const wxString saved = formatBenchmarkSnapshot(makeBenchmarkSnapshot(report));
    BenchmarkReport withoutTitles = report;
    for (BenchmarkHint& h : withoutTitles.hints)
        h.noticed.clear();
    const bool hidden = saved == formatBenchmarkSnapshot(makeBenchmarkSnapshot(withoutTitles))
        && std::any_of(report.hints.begin(), report.hints.end(),
            [](const BenchmarkHint& h) { return !h.noticed.empty(); });
    check(hidden && saved.Contains("note.power.saving = cpu | Power saving slows down the "
        "server |"), "a saved report keeps the titles of the hints");
}

void testVerdict()
{
    std::cout << "Main statement of a comparison\n";
    using C = BenchmarkCategory;
    using S = BenchmarkCompareState;
    const BenchmarkSnapshot older = makeBenchmarkSnapshot(makeReport(healthyMetrics(),
        "2026-10-04 12:00:00"));
    auto edit = [](BenchmarkSnapshot s, const std::function<bool(BenchmarkMeasurement&)>& keep)
    {
        s.measurements.erase(std::remove_if(s.measurements.begin(), s.measurements.end(),
            [&keep](BenchmarkMeasurement& x) { return !keep(x); }), s.measurements.end());
        return s;
    };
    // one key figure for every state
    const BenchmarkSnapshot newer = edit(older, [](BenchmarkMeasurement& x)
        {
            if (x.id == "cpu.single")
                x.value *= 1.5;
            else if (x.id == "storage.commit")
                x.version = 2;
            else if (x.id == "storage.insert")
                x.amount = 60000;
            else if (x.id == "users.oltp")
                x.value /= 2.0;
            return x.id != "network.roundTrip";
        });
    const BenchmarkComparison c = compareBenchmarkSnapshots(newer, older);
    const std::vector<BenchmarkHeadlineChange> changes = getBenchmarkHeadlineChanges(c);
    auto change = [&changes](const char* id) -> const BenchmarkHeadlineChange*
    {
        auto it = std::find_if(changes.begin(), changes.end(),
            [id](const BenchmarkHeadlineChange& h) { return h.id == id; });
        return it == changes.end() ? nullptr : &*it;
    };
    const BenchmarkHeadlineChange* cpu = change("cpu.single");
    const BenchmarkHeadlineChange* commit = change("storage.commit");
    const BenchmarkHeadlineChange* insert = change("storage.insert");
    const BenchmarkHeadlineChange* scan = change("memory.scan");
    const BenchmarkHeadlineChange* roundTrip = change("network.roundTrip");
    const BenchmarkHeadlineChange* users = change("users.oltp");
    check(changes.size() == 6 && cpu && cpu->state == S::Compared
        && std::abs(cpu->ratio - 1.5) < 1e-9 && commit && commit->state == S::TestChanged
        && insert && insert->state == S::WorkChanged && scan && scan->ratio == 1.0
        && roundTrip && roundTrip->state == S::OnlyOlder && roundTrip->newerNumber.empty()
        && roundTrip->olderNumber == "0.20" && roundTrip->unit == "ms"
        && users && std::abs(users->ratio - 0.5) < 1e-9 && users->newerNumber == "3,000"
        && users->olderNumber == "6,000", "the key figures of both runs with their states");
    const BenchmarkVerdict verdict = getBenchmarkVerdict(c);
    check(verdict.faster == 1 && verdict.slower == 1 && verdict.same == 1
        && verdict.notComparable == 3, "faster, slower, about the same and not comparable");
    const BenchmarkComparison reverse = compareBenchmarkSnapshots(older, newer);
    const std::vector<BenchmarkHeadlineChange> reverseChanges = getBenchmarkHeadlineChanges(
        reverse);
    check(std::any_of(reverseChanges.begin(), reverseChanges.end(),
        [](const BenchmarkHeadlineChange& h)
        { return h.id == "network.roundTrip" && h.state == S::OnlyNewer
            && h.olderNumber.empty() && h.newerNumber == "0.20"; }),
        "a key figure of the newer run only is new");

    // the states of the parts
    const BenchmarkSnapshot noNetwork = edit(older, [](BenchmarkMeasurement& x)
        { return !x.id.StartsWith("network."); });
    const BenchmarkSnapshot changedMemory = edit(older, [](BenchmarkMeasurement& x)
        {
            if (x.id.StartsWith("memory."))
                x.version = 2;
            return true;
        });
    const BenchmarkComparison lost = compareBenchmarkSnapshots(noNetwork, older);
    const BenchmarkComparison gained = compareBenchmarkSnapshots(older, noNetwork);
    const BenchmarkComparison memory = compareBenchmarkSnapshots(changedMemory, older);
    check(getBenchmarkPartState(lost, C::Network) == S::OnlyOlder
        && getBenchmarkPartState(gained, C::Network) == S::OnlyNewer
        && getBenchmarkPartState(memory, C::ServerCache) == S::TestChanged
        && getBenchmarkPartState(c, C::ServerCpu) == S::Compared
        && getBenchmarkPartState(c, C::Application) == S::Compared
        && c.findPart(C::Application) && c.findPart(C::Application)->ratio == 0.0,
        "a part in one run only, a part that cannot be compared, a part without points");
    const std::vector<std::pair<int, wxString>> words = describeBenchmarkVerdict(
        getBenchmarkVerdict(c));
    BenchmarkVerdict slowerOnly;
    slowerOnly.slower = 2;
    slowerOnly.same = 1;
    const std::vector<wxString> lostNotes = getBenchmarkVerdictNotes(lost);
    const std::vector<wxString> memoryNotes = getBenchmarkVerdictNotes(memory);
    check(words.size() == 3 && words[0] == std::make_pair(1, wxString("1 key figure faster"))
        && words[1] == std::make_pair(-1, wxString("1 slower"))
        && words[2] == std::make_pair(0, wxString("1 about the same"))
        && describeBenchmarkVerdict(slowerOnly).front().second == "2 key figures slower"
        && describeBenchmarkVerdict(BenchmarkVerdict()).empty()
        && getBenchmarkVerdictNotes(c) == std::vector<wxString>{ "3 key figures not compared" }
        && lostNotes == std::vector<wxString>{ "NET measured in the earlier run only",
            "1 key figure not compared" }
        && getBenchmarkVerdictNotes(gained).front() == "NET measured in the later run only"
        && std::count(memoryNotes.begin(), memoryNotes.end(), "RAM not comparable") == 1,
        "the main statement and what it leaves out, in words");

    // what stands out: the parts that differ clearly, the largest first
    const std::vector<const BenchmarkComparedPart*> standOut = getBenchmarkStandOut(c, 5);
    check(standOut.size() == 2 && standOut[0]->category == C::MultiUser
        && standOut[1]->category == C::ServerCpu && getBenchmarkStandOut(c, 1).size() == 1,
        "the parts that differ clearly, the largest change first");

    // the states in words, and why a value is not comparable
    const BenchmarkComparedValue* commitValue = findCompared(c, "storage.commit");
    const BenchmarkComparedValue* insertValue = findCompared(c, "storage.insert");
    const BenchmarkComparedValue* cpuValue = findCompared(c, "cpu.single");
    check(getBenchmarkCompareStateName(S::OnlyNewer) == "new"
        && getBenchmarkCompareStateName(S::OnlyOlder) == "no longer measured"
        && getBenchmarkCompareStateName(S::WorkChanged) == "not comparable"
        && getBenchmarkCompareStateName(S::Compared).empty()
        && commitValue && describeBenchmarkCompareState(c, *commitValue)
            == "The test changed (version 2, earlier version 1)."
        && insertValue && describeBenchmarkCompareState(c, *insertValue)
            == "Other work: 60,000 rows, earlier 50,000 rows."
        && cpuValue && describeBenchmarkCompareState(c, *cpuValue).empty(),
        "the states in words");
    check(cpuValue && formatBenchmarkComparedWork(c, *cpuValue)
            == "100,000 iterations: 65.00 ms " + wxString(wxUniChar(0x2192)) + " 65.00 ms"
        && insertValue && formatBenchmarkComparedWork(c, *insertValue)
            == "50,000 rows in 850 ms " + wxString(wxUniChar(0x2192)) + " 60,000 rows in 850 ms",
        "the work of both runs");
}

void testReasonLines()
{
    std::cout << "Likely reason and what to try\n";
    using C = BenchmarkCategory;
    const wxString arrow = wxString(" ") + wxUniChar(0x2192) + " ";
    BenchmarkComparedPart power;
    power.category = C::ServerCpu;
    power.ratio = 1.4;
    power.differences.push_back({ "power", "Power plan", "High performance", "Balanced" });
    power.hints.push_back({ "power.saving", C::ServerCpu, "Power saving slows down the "
        "server", "detail", "Windows: select the power plan. And more.", "" });
    power.advice.push_back({ "Calculating, one user", "A slower processor. Check it." });
    check(getBenchmarkLikelyReason(power) == "Power plan: Balanced" + arrow + "High performance"
        && getBenchmarkTryAdvice(power).Contains("powercfg"),
        "a fact that differs comes first, with its advice");
    BenchmarkComparedPart other = power;
    other.differences.front().key = "os";
    check(getBenchmarkTryAdvice(other) == "Windows: select the power plan.",
        "a fact without advice: the first sentence of the advice of a hint");
    BenchmarkComparedPart hint = power;
    hint.differences.clear();
    check(getBenchmarkLikelyReason(hint) == "Power saving slows down the server"
        && getBenchmarkTryAdvice(hint) == "Windows: select the power plan.",
        "otherwise the hint of the slower run");
    BenchmarkComparedPart typical = hint;
    typical.hints.clear();
    check(getBenchmarkLikelyReason(typical).empty()
        && getBenchmarkTryAdvice(typical).StartsWith("Repeat the slower run"),
        "only typical causes: no likely reason, but what to try");
    check(getBenchmarkLikelyReason(BenchmarkComparedPart()).empty()
        && getBenchmarkTryAdvice(BenchmarkComparedPart()).empty(), "nothing without reasons");
    check(getBenchmarkShortAdvice("Do this. Then that.") == "Do this."
        && getBenchmarkShortAdvice("Just this.") == "Just this."
        && getBenchmarkShortAdvice("Current setting: Balanced. Windows: select the power "
            "plan. Also this.") == "Windows: select the power plan.",
        "the first sentence that advises something");
    bool facts = true;
    for (const char* key : { "power", "access", "scanner", "pagecache",
        "conf.DefaultDbCachePages", "firebird", "conf.ServerMode", "cores", "ram", "storage",
        "virtual", "forcedwrites", "wire", "conf.WireCrypt", "conf.TempCacheLimit" })
    {
        facts = facts && !getBenchmarkFactAdvice(key).empty();
    }
    check(facts && getBenchmarkFactAdvice("scanner").Contains("on-access virus scanning")
        && getBenchmarkFactAdvice("system").empty(), "advice for the facts that matter");
}

// ---------------------------------------------- registry.golden and fixtures

// The register is checked against src/engine/testdata/benchmark/registry.golden:
// one line per measurement with its unit, direction, version and the
// signature of its work (the settings of the three intensities it uses, the
// values of BenchmarkWorkload it uses and a hash of its SQL). When the work
// of a test changes but its version does not, the test fails; see
// devdocs/Benchmark_report_format.md.

const std::pair<const char*, int BenchmarkSettings::*> settingsFields[] = {
    { "connectAttempts", &BenchmarkSettings::connectAttempts },
    { "localFiles", &BenchmarkSettings::localFiles },
    { "cpuIterations", &BenchmarkSettings::cpuIterations },
    { "parallelConnections", &BenchmarkSettings::parallelConnections },
    { "commitCount", &BenchmarkSettings::commitCount },
    { "diskRows", &BenchmarkSettings::diskRows },
    { "diskPayloadLength", &BenchmarkSettings::diskPayloadLength },
    { "scanRows", &BenchmarkSettings::scanRows },
    { "scanPayloadLength", &BenchmarkSettings::scanPayloadLength },
    { "latencyQueries", &BenchmarkSettings::latencyQueries },
    { "sparseQueries", &BenchmarkSettings::sparseQueries },
    { "bursts", &BenchmarkSettings::bursts },
    { "oltpSeconds", &BenchmarkSettings::oltpSeconds },
    { "rowByRowRows", &BenchmarkSettings::rowByRowRows },
    { "prepareCount", &BenchmarkSettings::prepareCount },
    { "blobCount", &BenchmarkSettings::blobCount },
    { "versionRounds", &BenchmarkSettings::versionRounds },
    { "driveSeconds", &BenchmarkSettings::driveSeconds },
    { "tempSortMegabytes", &BenchmarkSettings::tempSortMegabytes },
    { "wideRows", &BenchmarkSettings::wideRows },
    { "oltpMaxClients", &BenchmarkSettings::oltpMaxClients }
};

// every statement of BenchmarkSql by its name in the register
std::map<std::string, std::string> namedStatements()
{
    namespace S = BenchmarkSql;
    return { { "engineVersion", S::engineVersion }, { "attachmentInfo", S::attachmentInfo },
        { "attachmentWireInfo", S::attachmentWireInfo }, { "sessionUser", S::sessionUser },
        { "attachmentDetails", S::attachmentDetails },
        { "attachmentTimeouts", S::attachmentTimeouts },
        { "securityDatabase", S::securityDatabase }, { "databaseInfo", S::databaseInfo },
        { "databaseLinger", S::databaseLinger }, { "serverConfig", S::serverConfig },
        { "postEvent", S::postEvent }, { "driveLookups", S::driveLookups },
        { "tempSort", S::tempSort }, { "cpuTest", S::cpuTest },
        { "commitInsert", S::commitInsert }, { "fillTable", S::fillTable },
        { "bulkUpdate", S::bulkUpdate }, { "bulkDelete", S::bulkDelete },
        { "garbageCollect", S::garbageCollect }, { "scan", S::scan },
        { "serverRead", S::serverRead }, { "latency", S::latency },
        { "largeRoundTrip", S::largeRoundTrip }, { "throughput", S::throughput },
        { "ioStats", S::ioStats }, { "recordStats", S::recordStats },
        { "rowInsert", S::rowInsert }, { "fillRows", S::fillRows },
        { "prepareProbe", S::prepareProbe }, { "oltpPointSelect", S::oltpPointSelect },
        { "oltpRangeSelect", S::oltpRangeSelect }, { "oltpUpdate", S::oltpUpdate },
        { "oltpInsert", S::oltpInsert }, { "blobInsert", S::blobInsert },
        { "blobRead", S::blobRead }, { "hotFill", S::hotFill }, { "hotUpdate", S::hotUpdate },
        { "hotRead", S::hotRead }, { "serverProcesses", S::serverProcesses },
        { "wideInsert", S::getWideInsertStatement() } };
}

wxString constantText(int value)
{
    return wxString::Format("%d", value);
}

template <size_t N>
wxString constantText(const int (&values)[N])
{
    wxString text;
    for (int value : values)
        text += (text.empty() ? "" : ",") + constantText(value);
    return text;
}

// every value of BenchmarkWorkload and the sizes of BenchmarkSql by their
// names in the register
std::map<std::string, wxString> workloadConstants()
{
    namespace W = BenchmarkWorkload;
    return { { "comparedPageSizes", constantText(W::comparedPageSizes) },
        { "commitsPerPageSize", constantText(W::commitsPerPageSize) },
        { "commitRoundTrips", constantText(W::commitRoundTrips) },
        { "smallCachePages", constantText(W::smallCachePages) },
        { "idlePauseMs", constantText(W::idlePauseMs) },
        { "sparsePauseMs", constantText(W::sparsePauseMs) },
        { "serverBurstIterations", constantText(W::serverBurstIterations) },
        { "clientBurstIterations", constantText(W::clientBurstIterations) },
        { "localFileSize", constantText(W::localFileSize) },
        { "commitsPerSyncWrite", constantText(W::commitsPerSyncWrite) },
        { "minSyncWrites", constantText(W::minSyncWrites) },
        { "largeRoundTrips", constantText(W::largeRoundTrips) },
        { "largeRoundTripShare", constantText(W::largeRoundTripShare) },
        { "blobSize", constantText(W::blobSize) },
        { "blobChunk", constantText(W::blobChunk) },
        { "versionReads", constantText(W::versionReads) },
        { "oltpClientCounts", constantText(W::oltpClientCounts) },
        { "manyUserClientCounts", constantText(W::manyUserClientCounts) },
        { "oltpPointSelects", constantText(W::oltpPointSelects) },
        { "oltpKeyClients", constantText(W::oltpKeyClients) },
        { "chunkRows", constantText(W::chunkRows) },
        { "driveChunkRows", constantText(W::driveChunkRows) },
        { "driveLookupChunk", constantText(W::driveLookupChunk) },
        { "parallelDriveTimeShare", constantText(W::parallelDriveTimeShare) },
        { "tempSortMaxCopies", constantText(W::tempSortMaxCopies) },
        { "checkedPortFirst", constantText(W::checkedPortFirst) },
        { "checkedPortLast", constantText(W::checkedPortLast) },
        { "eventRetryMs", constantText(W::eventRetryMs) },
        { "oltpRangeRows", constantText(BenchmarkSql::oltpRangeRows) },
        { "wideColumns", constantText(BenchmarkSql::wideColumns) } };
}

// the constants of BenchmarkRunner.cpp that change no result, and why; a
// constant that changes one belongs to BenchmarkWorkload and the register
const std::pair<const char*, const char*> notWorkload[] = {
    { "oltpMaxFailuresInRow", "ends a run with an error, the work is the same" },
    { "parallelDriveIdBase", "keeps the IDs of the tests apart" },
    { "parallelDriveIdsPerConnection", "keeps the IDs of the tests apart" },
    { "commitIdBase", "keeps the IDs of the tests apart" },
    { "oltpIdBase", "keeps the IDs of the tests apart" },
    { "oltpIdsPerClient", "keeps the IDs of the tests apart" },
    { "portCheckTimeoutMs", "a timeout" },
    { "eventTimeoutMs", "a timeout" },
    { "latencyOutlierMs", "counts slow round trips for a detail and a hint, the value is "
        "the median" } };

// everything the signature of the work of a test is made of; the tests
// change a copy of it
struct Workload
{
    BenchmarkSettings intensities[3];
    std::map<std::string, wxString> constants;
    std::map<std::string, std::string> statements;
};

// the object a DDL statement creates; an index belongs to its table
std::string ddlObject(const std::string& sql)
{
    std::smatch match;
    static const std::regex index(R"(INDEX\s+\w+\s+ON\s+(\w+))", std::regex::icase);
    static const std::regex object(
        R"(CREATE\s+(?:GLOBAL\s+TEMPORARY\s+)?(?:TABLE|PROCEDURE)\s+(\w+))", std::regex::icase);
    if (std::regex_search(sql, match, index) || std::regex_search(sql, match, object))
        return match[1].str();
    return std::string();
}

std::vector<std::string> words(const char* text)
{
    std::vector<std::string> list;
    for (const wxString& w : wxSplit(wxString(text), ' ', '\0'))
    {
        if (!w.empty())
            list.push_back(std::string(w.utf8_str()));
    }
    return list;
}

uint32_t fnv1a(const std::string& text)
{
    uint32_t hash = 2166136261u;
    for (unsigned char c : text)
    {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

// the line of registry.golden for an entry; problems gets the names of the
// settings, constants and statements that are not known
wxString goldenLine(const BenchmarkMeasurementDef& def, const Workload& work,
    std::vector<wxString>& problems)
{
    using K = BenchmarkMeasurementKind;
    const bool info = def.kind == K::Info;
    const BenchmarkSettings (&intensities)[3] = work.intensities;
    wxString settings;
    for (const std::string& name : words(def.settings))
    {
        auto field = std::find_if(std::begin(settingsFields), std::end(settingsFields),
            [&name](const auto& f) { return name == f.first; });
        if (field == std::end(settingsFields))
        {
            problems.push_back(wxString(def.id) + ": unknown setting " + name);
            continue;
        }
        settings += (settings.empty() ? "" : " ") + wxString::Format("%s=%d/%d/%d",
            name.c_str(), intensities[0].*field->second, intensities[1].*field->second,
            intensities[2].*field->second);
    }
    wxString constants;
    for (const std::string& name : words(def.constants))
    {
        auto constant = work.constants.find(name);
        if (constant == work.constants.end())
            problems.push_back(wxString(def.id) + ": unknown constant " + name);
        else
            constants += (constants.empty() ? "" : " ") + name + "=" + constant->second;
    }
    std::string sql;
    for (const std::string& name : words(def.sql))
    {
        std::string text;
        auto statement = work.statements.find(name);
        if (statement != work.statements.end())
            text = statement->second;
        for (const char* create : BenchmarkSql::getCreateStatements())
        {
            if (ddlObject(create) == name)
                text += std::string(create) + "\n";
        }
        if (text.empty())
            problems.push_back(wxString(def.id) + ": unknown statement " + name);
        sql += name + ":" + text + "\n";
    }
    const wxString kind = def.kind == K::Key ? "key" : def.kind == K::Measure ? "measure" : "info";
    return wxString(def.id) + (def.family ? ".*" : "") + " | " + getBenchmarkSectionId(def.section)
        + " | " + (info ? wxString("-") : wxString(def.unit)) + " | "
        + (info ? "-" : def.higherIsBetter ? "higher" : "lower")
        + wxString::Format(" | v%d | ", def.version) + kind + (def.retired ? " retired" : "")
        + " | " + (info ? "-" : def.amountFixed ? "fixed" : "time-limited") + " | "
        + (settings.empty() ? wxString("-") : settings) + " | "
        + (constants.empty() ? wxString("-") : constants) + " | "
        + (sql.empty() ? wxString("-") : wxString::Format("sql %08x", unsigned(fnv1a(sql))));
}

std::vector<wxString> goldenLines(const std::vector<BenchmarkMeasurementDef>& defs,
    const Workload& work, std::vector<wxString>& problems)
{
    std::vector<wxString> lines;
    for (const BenchmarkMeasurementDef& def : defs)
        lines.push_back(goldenLine(def, work, problems));
    return lines;
}

Workload currentWorkload()
{
    return { { BenchmarkSettings::forIntensity(BenchmarkIntensity::Quick),
        BenchmarkSettings::forIntensity(BenchmarkIntensity::Standard),
        BenchmarkSettings::forIntensity(BenchmarkIntensity::Deep) },
        workloadConstants(), namedStatements() };
}

wxString goldenText()
{
    std::vector<wxString> problems;
    wxString text = "# The register of the benchmark measurements (src/engine/BenchmarkValues.cpp):\n"
        "# id | part | unit | direction | version | kind | work | settings quick/standard/deep "
        "| constants | hash of the SQL\n"
        "# Checked by benchmark_test. Never remove or rename an id, never change its unit or\n"
        "# direction; when the work of a test changes, raise its version. Then write this file\n"
        "# again with: benchmark_test --print-golden > registry.golden\n"
        "# See devdocs/Benchmark_report_format.md.\n";
    for (const wxString& line : goldenLines(getBenchmarkMeasurementDefs(), currentWorkload(),
        problems))
    {
        text += line + "\n";
    }
    return text;
}

// what differs between registry.golden and the register, with what to do
std::vector<wxString> compareGolden(const wxString& golden, const std::vector<wxString>& lines)
{
    auto fields = [](const wxString& line)
    {
        wxArrayString parts = wxSplit(line, '|', '\0');
        for (auto& part : parts)
            part.Trim().Trim(false);
        return parts;
    };
    std::map<wxString, wxArrayString> current;
    for (const wxString& line : lines)
        current[fields(line)[0]] = fields(line);
    std::vector<wxString> problems;
    std::set<wxString> known;
    for (wxString line : wxSplit(golden, '\n', '\0'))
    {
        line.Replace("\r", "");
        if (line.empty() || line.StartsWith("#"))
            continue;
        const wxArrayString old = fields(line);
        if (old.size() != 10)
        {
            problems.push_back("registry.golden: a line that cannot be read: " + line);
            continue;
        }
        const wxString& id = old[0];
        known.insert(id);
        auto now = current.find(id);
        if (now == current.end())
        {
            problems.push_back(id + ": removed from the register; never remove an id, mark "
                "it retired");
            continue;
        }
        const wxArrayString& f = now->second;
        long oldVersion = 0, newVersion = 0;
        old[4].Mid(1).ToLong(&oldVersion);
        f[4].Mid(1).ToLong(&newVersion);
        if (old[2] != f[2] || old[3] != f[3])
            problems.push_back(id + ": unit or direction changed; use a new id instead");
        else if (newVersion < oldVersion)
            problems.push_back(id + ": the version of the test went down");
        else if (newVersion == oldVersion && (old[7] != f[7] || old[8] != f[8]
            || old[9] != f[9]))
        {
            problems.push_back(id + ": the work of the test changed, but its version was not "
                "raised; raise the version in BenchmarkValues.cpp, then write registry.golden "
                "again");
        }
        else if (wxJoin(old, '|', '\0') != wxJoin(f, '|', '\0'))
            problems.push_back(id + ": changed; write registry.golden again");
    }
    for (const auto& line : current)
    {
        if (!known.count(line.first))
            problems.push_back(line.first + ": new; write registry.golden again");
    }
    return problems;
}

wxString readFile(const wxString& path)
{
    wxFFile file(path, "r");
    wxString text;
    if (!file.IsOpened() || !file.ReadAll(&text, wxConvUTF8))
        return wxEmptyString;
    return text;
}

bool hasProblem(const std::vector<wxString>& problems, const wxString& start,
    const wxString& part)
{
    return std::any_of(problems.begin(), problems.end(), [&](const wxString& p)
        { return p.StartsWith(start) && p.Contains(part); });
}

void testGolden(const wxString& dir)
{
    std::cout << "Register against registry.golden\n";
    const Workload work = currentWorkload();
    std::vector<wxString> unknown;
    const std::vector<wxString> lines = goldenLines(getBenchmarkMeasurementDefs(), work, unknown);
    for (const wxString& p : unknown)
        std::cerr << "    " << p.utf8_str() << "\n";
    check(unknown.empty(), "the register names only known settings, constants and statements");

    // every setting, constant and statement belongs to a measurement, so
    // that a change of it reaches registry.golden
    std::set<std::string> used;
    for (const BenchmarkMeasurementDef& def : getBenchmarkMeasurementDefs())
    {
        for (const char* list : { def.settings, def.constants, def.sql })
        {
            for (const std::string& name : words(list))
                used.insert(name);
        }
    }
    bool covered = sizeof(BenchmarkSettings) == sizeof(settingsFields) / sizeof(settingsFields[0])
        * sizeof(int);
    for (const auto& field : settingsFields)
        covered = covered && used.count(field.first);
    for (const auto& constant : work.constants)
        covered = covered && used.count(constant.first);
    for (const auto& statement : work.statements)
        covered = covered && used.count(statement.first);
    for (const char* create : BenchmarkSql::getCreateStatements())
        covered = covered && used.count(ddlObject(create));
    check(covered, "every setting, constant, statement and object of the DDL belongs to a "
        "measurement");

    if (dir.empty())
    {
        check(false, "the directory of the test data is given as argument "
            "(src/engine/testdata/benchmark)");
        return;
    }
    const wxString golden = readFile(dir + "/registry.golden");
    const std::vector<wxString> problems = compareGolden(golden, lines);
    for (const wxString& p : problems)
        std::cerr << "    " << p.utf8_str() << "\n";
    check(!golden.empty() && problems.empty(), "the register is the one of registry.golden "
        "(benchmark_test --print-golden writes it)");

    // what the check finds: other work without a new version, ...
    Workload rows = work;
    rows.intensities[1].diskRows += 1;
    std::vector<wxString> ignored;
    const std::vector<wxString> settings = compareGolden(golden,
        goldenLines(getBenchmarkMeasurementDefs(), rows, ignored));
    Workload statements = work;
    statements.statements["cpuTest"] += " ";
    const std::vector<wxString> sql = compareGolden(golden,
        goldenLines(getBenchmarkMeasurementDefs(), statements, ignored));
    check(hasProblem(settings, "storage.insert:", "version was not raised")
        && hasProblem(settings, "storage.update:", "version was not raised")
        && !hasProblem(settings, "cpu.single:", "")
        && hasProblem(sql, "cpu.single:", "version was not raised")
        && hasProblem(sql, "cpu.burst:", "version was not raised")
        && !hasProblem(sql, "storage.insert:", ""),
        "other settings or SQL of a test without a new version are found");
    // ... a new version, a removed id, a new unit and a new measurement
    std::vector<BenchmarkMeasurementDef> defs = getBenchmarkMeasurementDefs();
    for (BenchmarkMeasurementDef& def : defs)
    {
        if (wxString(def.id) == "storage.insert")
            def.version = 2;
        if (wxString(def.id) == "storage.update")
            def.unit = "rows/min";
    }
    defs.erase(std::remove_if(defs.begin(), defs.end(), [](const BenchmarkMeasurementDef& d)
        { return wxString(d.id) == "storage.gc"; }), defs.end());
    BenchmarkMeasurementDef added = defs.front();
    added.id = "cpu.vector";
    defs.push_back(added);
    const std::vector<wxString> edited = compareGolden(golden, goldenLines(defs, rows, ignored));
    check(hasProblem(edited, "storage.insert:", "write registry.golden again")
        && !hasProblem(edited, "storage.insert:", "version was not raised")
        && hasProblem(edited, "storage.update:", "unit or direction")
        && hasProblem(edited, "storage.gc:", "never remove an id")
        && hasProblem(edited, "cpu.vector:", "new"),
        "a new version, a removed id, another unit and a new id are found");
    wxString crlf = golden;
    crlf.Replace("\n", "\r\n");
    check(compareGolden(crlf, lines).empty(), "registry.golden with Windows line ends");
}

// the names of the numeric constants that a source text declares with a
// number: values of the work, not ones calculated from the settings
std::set<std::string> declaredConstants(const wxString& text)
{
    static const std::regex constant(R"((?:^|\n)[ \t]*(?:static[ \t]+)?(?:constexpr|const)[ \t]+)"
        R"((?:unsigned[ \t]+)?(?:int|long|short|double|float|size_t|int64_t)[ \t]+(\w+))"
        R"([ \t]*(?:\[\])?[ \t]*=[ \t]*[-{0-9])");
    const std::string source(text.utf8_str());
    std::set<std::string> names;
    for (std::sregex_iterator it(source.begin(), source.end(), constant), end; it != end; ++it)
        names.insert((*it)[1].str());
    return names;
}

// the text of a namespace of a header, up to its closing comment
wxString namespaceText(const wxString& text, const wxString& name)
{
    const size_t start = text.find("namespace " + name + "\n");
    const size_t end = text.find("} // namespace " + name, start);
    if (start == wxString::npos || end == wxString::npos)
        return wxEmptyString;
    return text.substr(start, end - start);
}

// MSVC limits a string literal to 16380 bytes: the style sheet of the report
// is split into several
void testStyleSheetSource(const wxString& dir)
{
    std::cout << "Style sheet of the report\n";
    if (dir.empty())
        return;
    const wxString source = readFile(dir + "/../../BenchmarkHtml.cpp");
    int literals = 0;
    bool small = !source.empty();
    for (size_t start = source.find("R\"CSS("); start != wxString::npos;
        start = source.find("R\"CSS(", start + 1))
    {
        const size_t end = source.find(")CSS\"", start);
        ++literals;
        const size_t bytes = std::string(source.Mid(start + 6,
            end - start - 6).utf8_str()).size();
        if (end == wxString::npos || bytes > 16000)
        {
            std::cerr << "    literal " << literals << ": " << bytes << " bytes\n";
            small = false;
        }
    }
    check(small && literals >= 2, "every literal of the style sheet stays below the limit of "
        "MSVC");
}

void testWorkloadConstants(const wxString& dir)
{
    std::cout << "Constants of the work of the tests\n";
    const Workload work = currentWorkload();
    std::vector<wxString> ignored;
    const std::vector<wxString> lines = goldenLines(getBenchmarkMeasurementDefs(), work, ignored);
    wxString current;
    for (const wxString& line : lines)
        current += line + "\n";

    // another pause of the power saving tests changes their work, nothing else
    Workload pause = work;
    pause.constants["idlePauseMs"] = "600";
    const std::vector<wxString> paused = compareGolden(current,
        goldenLines(getBenchmarkMeasurementDefs(), pause, ignored));
    // a constant that changes no result is no part of any signature, e.g.
    // the time after which a round trip counts as a retransmission
    Workload outlier = work;
    outlier.constants["latencyOutlierMs"] = "300";
    const std::vector<wxString> display = goldenLines(getBenchmarkMeasurementDefs(), outlier,
        ignored);
    check(paused.size() == 2
        && hasProblem(paused, "cpu.burst:", "version was not raised")
        && hasProblem(paused, "computer.burst:", "version was not raised")
        && display == lines,
        "another value of a constant of the work changes the signature of its tests only");
    BenchmarkMeasurementDef wrong = getBenchmarkMeasurementDefs().front();
    wrong.constants = "latencyOutlierMs";
    std::vector<wxString> notWork;
    goldenLine(wrong, work, notWork);
    check(notWork.size() == 1 && notWork[0].Contains("unknown constant latencyOutlierMs"),
        "a test cannot name a constant that changes no result");

    // every numeric constant of the runner and of BenchmarkWorkload is
    // known: in the register, or as one that changes no result
    if (dir.empty())
        return;
    const wxString engine = dir + "/../..";
    wxString header = readFile(engine + "/Benchmark.h");
    header.Replace("\r", "");
    std::set<std::string> listed;
    for (const char* name : { "BenchmarkWorkload", "BenchmarkSql" })
    {
        for (const std::string& constant : declaredConstants(namespaceText(header, name)))
            listed.insert(constant);
    }
    bool registered = listed.size() > 20;
    for (const std::string& name : listed)
    {
        if (!work.constants.count(name))
        {
            std::cerr << "    not in the list of the test: " << name << "\n";
            registered = false;
        }
    }
    for (const auto& constant : work.constants)
        registered = registered && listed.count(constant.first);
    check(registered, "every constant of BenchmarkWorkload is in the signature of its tests");
    const std::set<std::string> runner = declaredConstants(readFile(engine
        + "/BenchmarkRunner.cpp"));
    bool known = runner.size() >= std::size(notWorkload);
    for (const std::string& name : runner)
    {
        if (std::none_of(std::begin(notWorkload), std::end(notWorkload),
            [&name](const auto& n) { return name == n.first; }))
        {
            std::cerr << "    a constant of BenchmarkRunner.cpp that belongs to "
                "BenchmarkWorkload and its tests, or to notWorkload: " << name << "\n";
            known = false;
        }
    }
    for (const auto& n : notWorkload)
        known = known && runner.count(n.first);
    check(known, "every constant of BenchmarkRunner.cpp changes no result or is in "
        "BenchmarkWorkload");
}

// the reports of the fixtures; a fixed date, so that they can be written again
BenchmarkReport fixtureReport(const BenchmarkMetrics& m, const char* date)
{
    BenchmarkReport report = makeReport(m, date);
    report.benchmarkDatabasePath = "FR_BENCHMARK_20261014_090000_ABCDEF.FDB";
    report.benchmarkDatabaseCreated = true;
    report.benchmarkDatabaseDropped = true;
    report.durationSeconds = 131.0;
    return report;
}

BenchmarkReport clientFixture()
{
    BenchmarkMetrics m = fullMetrics();
    m.skippedTests.clear();
    m.clientBurstIdleMs = 25.0;
    m.system.powerPlan = "Balanced";
    m.system.antivirus = { "Windows Defender (on)" };
    return fixtureReport(m, "2026-10-14 11:00:00");
}

// the files of the fixtures of this format, see devdocs/Benchmark_report_format.md
std::vector<std::pair<wxString, wxString>> fixtureFiles()
{
    BenchmarkMetrics embedded = serverMetrics();
    embedded.system.powerPlan = "Balanced";
    embedded.system.antivirus = { "Windows Defender (on)" };
    embedded.system.storagePathExcluded = false;
    embedded.skippedTests.push_back({ "application.blob", "Writing / reading large data "
        "(BLOBs)", "Wrong parameters block kind 6" });
    embedded.blobReadMs.reset();
    BenchmarkMetrics server = healthyMetrics();
    server.runLocation = BenchmarkRunLocation::Server;
    server.connectionKind = BenchmarkConnectionKind::LocalXnet;
    server.connectionHost.clear();
    server.latencyMedianMs = 0.02;
    server.latencyAvgMs = 0.025;
    BenchmarkReport client = clientFixture();
    client.metrics.localHostName = "PC-MUELLER";
    client.metrics.connectionHost = "dbserver01";
    const wxString clientMd = clientFixture().toMarkdown();
    // a later minor format: lines, fields and a measurement this version
    // does not know
    wxString unknown = clientMd;
    unknown.Replace("\nformat = 2.0\n", "\nformat = 2.9\n");
    unknown.Replace("\nrun.intensity = standard\n", "\nrun.intensity = standard\n"
        "run.operator = night shift\nfuture.section.value = 1\n");
    unknown.Replace(" | Saving a change (commit) | 1.50 ms\n",
        " | Saving a change (commit) | 1.50 ms | a later field\n");
    unknown.Replace("\nfact.system = ", "\nm.cpu.vector = 812 | it/ms | v1 | 100000 | "
        "iterations | 123 | Vector calculation | 812 it/ms\nm.gpu.render = 60 | frames/s | v1 "
        "|  |  |  | Rendering | 60 frames/s\nfact.system = ");
    wxString future = clientMd;
    future.Replace("\nformat = 2.0\n", "\nformat = 9.0\n");
    return { { "format2-embedded.md", fixtureReport(embedded, "2026-10-14 09:00:00").toMarkdown() },
        { "format2-server.md", fixtureReport(server, "2026-10-14 10:00:00").toMarkdown() },
        { "format2-client.html", clientFixture().toHtml() },
        { "format2-anonymized.md", client.anonymized().toMarkdown() },
        { "format2.9-unknown.md", unknown }, { "format9-future.md", future } };
}

BenchmarkSnapshotResult readFixture(const wxString& dir, const char* name)
{
    return parseBenchmarkSnapshot(readFile(dir + "/" + name));
}

// every step of the runner names its stage, and the plan has a weight for
// each of them: a new step without a place in the plan fails here
void testStepsHaveStages(const wxString& dir)
{
    std::cout << "Every step of the runner has a stage\n";
    if (dir.empty())
        return;
    const std::string source(readFile(dir + "/../../BenchmarkRunner.cpp").utf8_str());
    // the calls of step(), without the comments
    std::string code;
    for (size_t pos = 0; pos < source.size();)
    {
        const size_t comment = source.find("//", pos);
        code += source.substr(pos, comment == std::string::npos ? std::string::npos
            : comment - pos);
        if (comment == std::string::npos)
            break;
        pos = source.find('\n', comment);
    }
    std::map<std::string, int> calls;
    bool named = true;
    int count = 0;
    static const std::regex stage(R"(^\s*BenchmarkStage::(\w+))");
    for (size_t pos = code.find("step("); pos != std::string::npos;
        pos = code.find("step(", pos + 1))
    {
        const char before = pos > 0 ? code[pos - 1] : ' ';
        // the definition BenchmarkRunner::step( and words such as stageStep(
        if (std::isalnum(static_cast<unsigned char>(before)) || before == '_' || before == ':'
            || before == '.')
        {
            continue;
        }
        ++count;
        std::smatch match;
        const std::string rest = code.substr(pos + 5, 80);
        if (std::regex_search(rest, match, stage))
            ++calls[match[1].str()];
        else
        {
            std::cerr << "    a step without a stage: " << rest.substr(0, 50) << "\n";
            named = false;
        }
    }
    check(named && count > 30, "every call of step() names a stage");
    // the steps of a stage that does not depend on the options are called
    // once each; the OLTP levels in a loop, the page sizes in a loop with a
    // call for a page size the server does not support
    const std::vector<BenchmarkStagePlan> plan = getBenchmarkStagePlan(
        BenchmarkIntensity::Standard, true, false);
    const std::map<BenchmarkStage, std::string> names = { { BenchmarkStage::Prepare, "Prepare" },
        { BenchmarkStage::Connect, "Connect" }, { BenchmarkStage::Cpu, "Cpu" },
        { BenchmarkStage::Disk, "Disk" }, { BenchmarkStage::Memory, "Memory" },
        { BenchmarkStage::Network, "Network" }, { BenchmarkStage::Users, "Users" },
        { BenchmarkStage::PageSizes, "PageSizes" }, { BenchmarkStage::Finish, "Finish" } };
    bool planned = calls.size() == 8;
    for (const BenchmarkStagePlan& s : plan)
    {
        const std::string name = names.at(s.stage);
        const int expected = s.stage == BenchmarkStage::Users ? 1
            : s.stage == BenchmarkStage::PageSizes ? 2 : int(s.steps.size());
        const int found = calls.count(name) ? calls[name] : 0;
        if (found != expected)
        {
            std::cerr << "    stage " << name << ": " << found << " calls of step(), the plan "
                "(getBenchmarkStagePlan()) has " << expected << "\n";
            planned = false;
        }
    }
    check(planned, "the plan of the stages has every step of the runner");
}

void testFixtures(const wxString& dir)
{
    std::cout << "Saved reports of this and other versions (fixtures)\n";
    using S = BenchmarkCompareState;
    if (dir.empty())
    {
        check(false, "the directory of the test data is given as argument");
        return;
    }
    // every report of this format can be read and compared, also by later
    // versions
    std::map<wxString, BenchmarkSnapshot> reports;
    bool readable = true;
    for (const char* name : { "format2-embedded.md", "format2-server.md",
        "format2-client.html", "format2-anonymized.md" })
    {
        const BenchmarkSnapshotResult r = readFixture(dir, name);
        if (!r.ok() || r.snapshot.measurements.size() < 30)
        {
            std::cerr << "    cannot read " << name << "\n";
            readable = false;
        }
        reports[name] = r.snapshot;
    }
    check(readable, "the reports of format 2 can be read");
    check(isBenchmarkReportHtmlPlain(readFile(dir + "/format2-client.html")),
        "a saved HTML report of format 2 can be shown as it was saved");
    bool same = readable;
    for (const auto& report : reports)
    {
        const BenchmarkComparison c = compareBenchmarkSnapshots(report.second, report.second);
        same = same && std::abs(c.ratio - 1.0) < 1e-12;
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const BenchmarkComparedValue& v : *list)
                same = same && v.state == S::Compared && (v.ratio == 0.0 || v.ratio == 1.0);
        }
    }
    check(same, "every report compared with itself: all values compared, ratios 1");
    const BenchmarkComparison kinds = compareBenchmarkSnapshots(reports["format2-server.md"],
        reports["format2-embedded.md"]);
    const BenchmarkComparison shared = compareBenchmarkSnapshots(
        reports["format2-anonymized.md"], reports["format2-client.html"]);
    bool anonymous = std::abs(shared.ratio - 1.0) < 1e-12
        && reports["format2-anonymized.md"].host.empty();
    for (const BenchmarkComparedValue& v : shared.stats)
        anonymous = anonymous && v.state == S::Compared && (v.ratio == 0.0 || v.ratio == 1.0);
    check(kinds.ratio > 0.0 && !kinds.sameKind && kinds.findPart(BenchmarkCategory::ServerStorage)
        && anonymous, "reports of different runs and an anonymized report are compared");

    // older and newer formats
    const BenchmarkSnapshotResult first = readFixture(dir, "format1-v26.10.0.md");
    const BenchmarkSnapshotResult future = readFixture(dir, "format9-future.md");
    const BenchmarkSnapshotResult later = readFixture(dir, "format2.9-unknown.md");
    check(first.status == BenchmarkSnapshotStatus::TooOld && first.format == "1"
        && future.status == BenchmarkSnapshotStatus::TooNew && future.format == "9.0",
        "a report of FlameRobin 26.10.0 and one of a newer major format are refused");
    const BenchmarkComparison minor = compareBenchmarkSnapshots(later.snapshot,
        reports["format2-client.html"]);
    const BenchmarkComparedValue* vector = findCompared(minor, "cpu.vector");
    const BenchmarkComparedValue* commit = findCompared(minor, "storage.commit");
    check(later.ok() && later.format == "2.9" && vector && vector->state == S::OnlyNewer
        && vector->title == "Vector calculation" && vector->ratio == 0.0
        && vector->category == BenchmarkCategory::ServerCpu
        && later.snapshot.findMeasurement("gpu.render") && !findCompared(minor, "gpu.render")
        && commit && commit->state == S::Compared && commit->ratio == 1.0
        && std::abs(minor.ratio - 1.0) < 1e-12,
        "a report of a later minor format: unknown lines skipped, unknown values listed "
        "in their part");

    // two saved reports, like "Compare..."
    BenchmarkMetrics faster = clientFixture().metrics;
    faster.commitMs = 0.75;
    faster.insertMs = 425.0;
    const wxString files[] = { wxFileName::CreateTempFileName("frbench"),
        wxFileName::CreateTempFileName("frbench") };
    {   // scope
        wxFFile older(files[0], "w");
        wxFFile newer(files[1], "w");
        older.Write(clientFixture().toHtml(), wxConvUTF8);
        newer.Write(fixtureReport(faster, "2026-10-15 08:00:00").toMarkdown(), wxConvUTF8);
    }
    const BenchmarkSnapshotResult a = parseBenchmarkSnapshot(readFile(files[0]));
    const BenchmarkSnapshotResult b = parseBenchmarkSnapshot(readFile(files[1]));
    wxRemoveFile(files[0]);
    wxRemoveFile(files[1]);
    const BenchmarkComparison two = compareBenchmarkSnapshots(b.snapshot, a.snapshot);
    const BenchmarkComparedValue* saved = findCompared(two, "storage.commit");
    check(a.ok() && b.ok() && a.snapshot.date < b.snapshot.date && saved
        && std::abs(saved->ratio - 2.0) < 1e-9 && two.ratio > 1.0
        && two.toMarkdown().Contains("| DISK | Saving a change (commit) | "),
        "two saved files are compared");
}

// the results of the benchmark dialog: tests, saved files and comparisons
void testResults()
{
    std::cout << "Results of the dialog\n";
    using K = BenchmarkResultKind;
    auto run = [](const wxString& date)
    {
        BenchmarkReport report = makeReport(healthyMetrics(), date);
        report.metrics.localHostName = "PC-MUELLER";
        return report;
    };

    // numbers grow and are never given again
    BenchmarkResultStore store;
    for (int i = 0; i < 10; ++i)
        store.addRun(run(wxString::Format("2026-10-14 09:%02d:00", 10 + i)), std::nullopt);
    for (int id : { 2, 4, 6, 8 })
        store.close(id);
    const int eleventh = store.addRun(run("2026-10-14 10:00:00"), std::nullopt);
    std::vector<int> order;
    for (const BenchmarkResultEntry& e : store.getEntries())
        order.push_back(e.id);
    check(eleventh == 11 && order == std::vector<int>({ 1, 3, 5, 7, 9, 10, 11 }),
        "ten tests, four closed: the next one is number 11, in the order of the tabs");

    // the selection after closing
    check(store.select(5) && store.close(5) == 7 && store.getSelected() == 7,
        "closing the selected result selects its right neighbour");
    check(store.select(11) && store.close(11) == 10, "the last one selects its left neighbour");
    check(store.select(10) && store.close(3) == 10 && store.getSelected() == 10,
        "closing another result keeps the selection");
    check(!store.select(3) && store.getSelected() == 10 && store.select(0)
        && store.getSelected() == 0, "a closed result cannot be selected, 0 selects none");
    BenchmarkResultStore single;
    const int only = single.addRun(run("2026-10-14 09:00:00"), std::nullopt);
    check(single.select(only) && single.close(only) == 0 && single.getEntries().empty(),
        "closing the only result selects none");
    store.select(9);
    check(store.closeOthers(9) == 9 && store.getEntries().size() == 1
        && store.getEntries()[0].id == 9, "close other results");
    check(store.closeAll() == 0 && store.getEntries().empty() && store.getSelected() == 0
        && store.addRun(run("2026-10-14 11:00:00"), std::nullopt) == 12,
        "close all results; the numbers go on");

    // only tests that measured something
    BenchmarkReport failed = makeReport(BenchmarkMetrics(), "2026-10-14 12:00:00");
    failed.metrics.settings = BenchmarkSettings::forIntensity(BenchmarkIntensity::Standard);
    failed.errorMessage = "I/O error during \"open\" operation";
    failed.evaluate();
    BenchmarkReport cancelled = run("2026-10-14 12:05:00");
    cancelled.cancelled = true;
    BenchmarkResultStore kept;
    const int cancelledId = kept.addRun(cancelled, std::nullopt);
    check(!hasBenchmarkData(failed) && !hasBenchmarkData(BenchmarkReport())
        && kept.addRun(failed, std::nullopt) == 0 && cancelledId == 1
        && kept.find(cancelledId)->incomplete, "a test without a value is not kept, a "
        "cancelled one is kept as incomplete");

    // the labels, the descriptions and the names of the files
    BenchmarkResultStore results;
    const int first = results.addRun(run("2026-10-14 09:55:12"), std::nullopt);
    BenchmarkReport quick = run("2026-10-14 10:31:00");
    quick.intensity = BenchmarkIntensity::Quick;
    quick.intensityName = getBenchmarkIntensityName(BenchmarkIntensity::Quick);
    const int second = results.addRun(quick, std::nullopt);
    BenchmarkReport other = clientFixture();
    other.metrics.localHostName = "SRV-ALT";
    const BenchmarkSnapshot otherSnapshot = makeBenchmarkSnapshot(other);
    const int file = results.addSavedReport(otherSnapshot, other.toHtml(),
        "/home/user/reports/orders-before-the-upgrade-of-the-server.html");
    const BenchmarkResultEntry* a = results.find(first);
    const BenchmarkResultEntry* f = results.find(file);
    check(getBenchmarkResultLabel(*a) == "* 1  09:55"
        && getBenchmarkResultLabel(*f) == "orders-before-the-u..."
        && getBenchmarkResultLabel(*f).length() == 22, "tab labels: unsaved test with "
        "its time, a file name cut to 22 characters");

    // a saved comparison carries a mark; opened, it is shown but never compared
    const BenchmarkComparison savedPage = everyStateComparison();
    const wxString comparisonHtml = savedPage.toHtml();
    check(isBenchmarkComparisonPage(comparisonHtml)
        && isBenchmarkComparisonPage(savedPage.toMarkdown())
        && parseBenchmarkSnapshot(comparisonHtml).status == BenchmarkSnapshotStatus::NotAReport
        && parseBenchmarkSnapshot(savedPage.toMarkdown()).status == BenchmarkSnapshotStatus::NotAReport
        && isBenchmarkReportHtmlPlain(comparisonHtml)
        && !isBenchmarkComparisonPage(other.toHtml()),
        "a saved comparison is marked, has no results and passes the check of the view");
    BenchmarkResultStore pages;
    const int file2 = pages.addSavedReport(otherSnapshot, other.toHtml(), "/home/user/r.html");
    const int shown = pages.addSavedComparison(comparisonHtml, "/home/user/versus.html");
    const std::vector<int> comparable = pages.getComparableIds();
    check(shown != 0 && pages.find(shown)->comparisonPage
        && std::find(comparable.begin(), comparable.end(), shown) == comparable.end()
        && std::find(comparable.begin(), comparable.end(), file2) != comparable.end()
        && getBenchmarkResultLabel(*pages.find(shown)) == "versus.html",
        "an opened comparison is a tab, but not a source of comparisons");
    check(describeBenchmarkResult(*a).StartsWith("1  Run, 2026-10-14 09:55, Standard, client, ")
        && describeBenchmarkResult(*a).EndsWith(" (not saved)")
        && describeBenchmarkResult(*f).StartsWith("File orders-before-the-upgrade-of-the-"
            "server.html, 2026-10-14 11:00, Standard, ")
        && getBenchmarkResultHeader(*a) == "Result 1 - not saved"
        && getBenchmarkResultHeader(*f).EndsWith(" - shown as saved"),
        "descriptions and the header of a test and of a file");
    check(getBenchmarkResultFileName(*a) == "flamerobin-benchmark-1-20261014-0955.html",
        "a test is saved with its number and time");
    check(getBenchmarkResultSummary(*results.find(second)) == "10:31, Quick, client",
        "a test in a few words, for the question before it is closed");

    // saved and unsaved
    check(results.getUnsavedIds() == std::vector<int>({ first, second }) && f->saved,
        "tests are not saved, an opened file is");
    results.markSaved(first, "/tmp/first.html");
    check(results.getUnsavedIds() == std::vector<int>({ second })
        && results.find(first)->saved && results.find(first)->path == "/tmp/first.html"
        && getBenchmarkResultLabel(*results.find(first)) == "1  09:55"
        && getBenchmarkResultHeader(*results.find(first)) == "Result 1 - saved",
        "a saved test loses its star");
    check(getBenchmarkCloseQuestion(results, BenchmarkCloseScope::One, first).empty()
        && getBenchmarkCloseQuestion(results, BenchmarkCloseScope::One, second)
            == std::vector<int>({ second })
        && getBenchmarkCloseQuestion(results, BenchmarkCloseScope::Others, second).empty()
        && getBenchmarkCloseQuestion(results, BenchmarkCloseScope::Others, file)
            == std::vector<int>({ second })
        && getBenchmarkCloseQuestion(results, BenchmarkCloseScope::All, file)
            == std::vector<int>({ second })
        && formatBenchmarkResultIds({ 3, 5, 6 }) == "3, 5, 6",
        "a question only for tests that are not saved");
    {
        BenchmarkResultStore discarded = results;
        discarded.discardUnsaved();
        check(discarded.getUnsavedIds().empty() && !discarded.find(second)
            && discarded.find(first) && discarded.find(file)
            && discarded.getEntries().size() == results.getEntries().size() - 1,
            "discarding drops only the tests that are not saved");
    }

    // comparisons hold copies
    const BenchmarkPair pair = orderBenchmarkPair(*results.find(second), *results.find(first));
    const int compared = results.addComparison(*pair.older, *pair.newer);
    const BenchmarkResultEntry* c = results.find(compared);
    check(pair.older->id == first && c && c->kind == K::Comparison
        && getBenchmarkResultLabel(*c) == "1 vs 2" && c->comparison->older.date
            == "2026-10-14 09:55:12" && c->comparison->newer.date == "2026-10-14 10:31:00"
        && getBenchmarkResultHeader(*c) == "Comparison 1 vs 2 - result 1 is the reference"
        && getBenchmarkResultFileName(*c) == "flamerobin-benchmark-comparison-1-vs-2.html",
        "a comparison of two tests, the earlier one is the reference");
    check(results.addComparison(*results.find(first), *results.find(second)) == compared
        && results.getEntries().size() == 4, "the same comparison again selects it");
    check(results.getComparableIds() == std::vector<int>({ first, second, file })
        && results.getUnsavedIds() == std::vector<int>({ second }),
        "comparisons are no source of comparisons and need no saving");
    const double ratio = c->comparison->ratio;
    const wxString markdown = c->comparison->toMarkdown();
    results.close(first);
    results.close(second);
    c = results.find(compared);
    check(c && c->comparison->ratio == ratio && c->comparison->toMarkdown() == markdown
        && c->comparison->older.measurements.size() > 30,
        "a comparison stays the same when both results are closed");

    // the order and the checks of a pair
    BenchmarkResultEntry early, late, undated;
    early.id = 3;
    early.snapshot.date = "2026-10-14 09:55:00";
    early.snapshot.kind = "client";
    early.snapshot.intensity = "quick";
    late.id = 5;
    late.snapshot.date = "2026-10-14 10:31:00";
    late.snapshot.kind = "embedded";
    late.snapshot.intensity = "standard";
    undated.id = 7;
    check(orderBenchmarkPair(late, early).older == &early
        && orderBenchmarkPair(early, late).older == &early
        && orderBenchmarkPair(late, undated).older == &late
        && orderBenchmarkPair(undated, late).older == &undated
        && orderBenchmarkPair(early, early).older == &early,
        "the earlier date is the older run; without a date the order of the choice");
    check(describeBenchmarkPairOrder(early, late) == "Earlier: result 3 (09:55)   Later: "
        "result 5 (10:31)", "the order in words");
    BenchmarkResultEntry fileA, fileB;
    fileA.kind = fileB.kind = K::SavedReport;
    fileA.path = fileB.path = "/tmp/orders.md";
    fileA.id = 4;
    using P = BenchmarkPairCheck;
    const std::vector<P> different = checkBenchmarkPair(early, late);
    late.incomplete = true;
    check(checkBenchmarkPair(early, early) == std::vector<P>({ P::SameResult })
        && checkBenchmarkPair(fileA, fileB) == std::vector<P>({ P::SameResult })
        && different == std::vector<P>({ P::DifferentKind, P::DifferentIntensity })
        && checkBenchmarkPair(early, late) == std::vector<P>({ P::DifferentKind,
            P::DifferentIntensity, P::Incomplete })
        && checkBenchmarkPair(*results.find(file), *results.find(file)).size() == 1,
        "the checks of a pair: the same result, another kind or intensity, incomplete");
    check(describeBenchmarkPairCheck(P::DifferentKind, early, late) == "Result 3 was made "
        "over the network (client) and result 5 with the embedded engine: they measure "
        "different things and are only roughly comparable. The comparison says so as well."
        && describeBenchmarkPairCheck(P::DifferentIntensity, early, late) == "Result 3 is "
        "a Quick run and result 5 a Standard run: the comparison is only roughly valid.",
        "what the checks say");

    // at most 30 results and comparisons
    BenchmarkResultStore full;
    for (size_t i = 0; i < BenchmarkResultStore::maximumEntries; ++i)
        full.addRun(run("2026-10-14 09:00:00"), std::nullopt);
    check(full.isFull() && full.addRun(run("2026-10-14 10:00:00"), std::nullopt) == 0
        && full.addSavedReport(otherSnapshot, wxEmptyString, "/tmp/a.html") == 0
        && full.addComparison(full.getEntries()[0], full.getEntries()[1]) == 0
        && full.close(30) == 0 && !full.isFull()
        && full.addRun(run("2026-10-14 10:00:00"), std::nullopt) == 31,
        "at most 30 results; a closed one makes room");

    // the measured report stays, the hidden one has no names
    BenchmarkResultStore privacy;
    BenchmarkReport mine = run("2026-10-14 09:00:00");
    BenchmarkReport theirs = run("2026-10-14 10:00:00");
    theirs.metrics.localHostName = "SRV-OTHER";
    theirs.metrics.system.virtualization = "Hyper-V on SRV-OTHER";
    const int mineId = privacy.addRun(mine, otherSnapshot);
    const int theirsId = privacy.addRun(theirs, std::nullopt);
    const BenchmarkResultEntry* m = privacy.find(mineId);
    check(!m->report->anonymous && m->report->metrics.localHostName == "PC-MUELLER"
        && !m->report->earlier && m->baseline && m->baseline->host == "SRV-ALT",
        "the test is kept as measured, the earlier report as a copy of its own");
    const int both = privacy.addComparison(*m, *privacy.find(theirsId));
    const BenchmarkComparison hidden = anonymizeBenchmarkComparison(
        *privacy.find(both)->comparison);
    const wxString hiddenText = (hidden.toHtml() + hidden.toMarkdown()
        + hidden.toHtml("dark")).Lower();
    check(!hiddenText.Contains("pc-mueller") && !hiddenText.Contains("srv-other")
        && privacy.find(both)->comparison->toMarkdown().Lower().Contains("pc-mueller"),
        "a comparison of two computers without private information names neither");

    // two saved files compare like the two tests
    const wxString files[] = { wxFileName::CreateTempFileName("frbench"),
        wxFileName::CreateTempFileName("frbench") };
    {   // scope
        wxFFile older(files[0], "w");
        wxFFile newer(files[1], "w");
        older.Write(mine.toHtml(), wxConvUTF8);
        newer.Write(theirs.toMarkdown(), wxConvUTF8);
    }
    const BenchmarkSnapshotResult olderFile = parseBenchmarkSnapshot(readFile(files[0]));
    const BenchmarkSnapshotResult newerFile = parseBenchmarkSnapshot(readFile(files[1]));
    wxRemoveFile(files[0]);
    wxRemoveFile(files[1]);
    BenchmarkResultEntry olderSource, newerSource;
    olderSource.kind = newerSource.kind = K::SavedReport;
    olderSource.snapshot = olderFile.snapshot;
    newerSource.snapshot = newerFile.snapshot;
    olderSource.path = "/tmp/older.html";
    newerSource.path = "/tmp/newer.md";
    const int fromFiles = privacy.addComparison(olderSource, newerSource);
    const BenchmarkComparison& runs = *privacy.find(both)->comparison;
    const BenchmarkComparison& saved = *privacy.find(fromFiles)->comparison;
    check(olderFile.ok() && newerFile.ok() && fromFiles != both
        && std::abs(saved.ratio - runs.ratio) < 1e-9 && saved.values.size() == runs.values.size()
        && getBenchmarkResultLabel(*privacy.find(fromFiles)) == "older.html vs newer.md"
        && getBenchmarkResultFileName(*privacy.find(fromFiles))
            == "flamerobin-benchmark-comparison-older-vs-newer.html",
        "two saved files compare like the two tests");
}

// a saved page is shown in the dialog only if it is a plain report
void testPlainReport()
{
    std::cout << "Saved pages shown in the dialog\n";
    BenchmarkReport report = clientFixture();
    BenchmarkReport compared = report;
    compared.earlier = makeBenchmarkSnapshot(makeReport(healthyMetrics(),
        "2026-10-13 09:00:00"));
    const BenchmarkComparison comparison = compareBenchmarkSnapshots(
        makeBenchmarkSnapshot(report), *compared.earlier);
    bool plain = true;
    int pages = 0;
    for (const wxString& page : { report.toHtml(), report.toHtml("light"),
        report.toHtml("dark"), report.anonymized().toHtml(), compared.toHtml(),
        compared.anonymized().toHtml("dark"), comparison.toHtml(), comparison.toHtml("dark"),
        anonymizeBenchmarkComparison(comparison).toHtml("light"),
        makeReport(serverMetrics(), "2026-10-14 10:00:00").toHtml() })
    {
        ++pages;
        if (!isBenchmarkReportHtmlPlain(page))
        {
            std::cerr << "    page " << pages << " is not plain\n";
            plain = false;
        }
    }
    check(plain, "every page FlameRobin writes is a plain report: report, comparison, "
        "light, dark, without private information, with an earlier run");

    const wxString page = report.toHtml();
    const size_t body = page.find("<body>") + 6;
    auto with = [&page, body](const wxString& part)
    {
        return page.substr(0, body) + part + page.substr(body);
    };
    bool refused = true;
    const char* const additions[] = { "<script>alert(1)</script>", "<SCRIPT SRC=x.js></SCRIPT>",
        "<div onclick=\"alert(1)\">x</div>", "<details OnToggle=alert(1)>",
        "<svg><use xlink:href=\"javascript:alert(1)\"/></svg>",
        "<svg><use href=\"data:image/svg+xml;base64,AAAA#x\"/></svg>",
        "<svg><use xlink:href=\"http://example.com/i.svg#a\"/></svg>",
        "<iframe></iframe>", "<object data=\"x\"></object>", "<embed>", "<img src=x>",
        "<link rel=stylesheet>", "<form>", "<base>", "<a href=\"https://example.com\">x</a>",
        "<meta http-equiv=\"refresh\" content=\"0;url=https://example.com\">",
        "<!--[if IE]><script>alert(1)</script><![endif]-->", "<![CDATA[x]]>",
        "<style>body { background: url(https://example.com/x.png) }</style>",
        "<style>@import 'x.css';</style>", "<div style=\"width: expr/**/ession(alert(1))\">",
        "<div style=\"behavior: url(x.htc)\">", "<style>b { color: \\72 ed }</style>",
        "<input type=\"image\" src=\"x\">", "<input type=text>",
        // where a browser reads a page differently than a simple check
        "<!--><script>alert(1)</script>-->", "<!--->\n<script>alert(1)</script>-->",
        "<!-- x --!><script>alert(1)</script> -->",
        "<svg><title><script>alert(1)</script></title></svg>",
        "<svg><style><img src=x onerror=alert(1)></style></svg>",
        "<svg><style><script>alert(1)</script></style></svg>",
        "<svg><svg/><title><script>alert(1)</script></title></svg>",
        "<meta http-equiv=\"refresh\" http-equiv=\"x-ua-compatible\" "
            "content=\"0;url=https://example.com\">",
        "<input type=\"image\" type=\"radio\">",
        "<div style=\"background:u&#114;l(https://example.com/x)\">",
        "<div style=\"width:e&#120;pression(alert(1))\">",
        "<svg><use href=\"&#x6a;avascript:x\"/></svg>",
        "<title></title ><script>alert(1)</script>",
        "<style></style ><script>alert(1)</script>" };
    for (const char* addition : additions)
    {
        if (isBenchmarkReportHtmlPlain(with(addition)))
        {
            std::cerr << "    accepted: " << addition << "\n";
            refused = false;
        }
    }
    check(refused, "a script, a handler, an address, a frame, an object, a form, an image, "
        "a refresh, a conditional comment, a style that loads or runs something are refused");
    check(isBenchmarkReportHtmlPlain(with("<p>1 &lt; 2 and 3 < 4</p><!-- note -->"))
        && isBenchmarkReportHtmlPlain(with("<svg><use xlink:href=\"#i-cpu\"/></svg>"))
        && isBenchmarkReportHtmlPlain(with("<svg><svg/></svg><svg/><style>b { }</style>"))
        && isBenchmarkReportHtmlPlain("<!doctype html><html lang=\"en\"><body>"
            "<p>text</p></body></html>"), "text, comments and links within the page are fine");

    // the start of the page up to its <html> tag; the style sheet names
    // the themes as well
    auto start = [](const wxString& html)
    {
        return html.Left(html.find('>', html.find("<html")) + 1);
    };
    const wxString light = report.toHtml("light");
    const wxString none = report.toHtml();
    const wxString dark = applyBenchmarkReportTheme(none, "dark");
    check(start(none) == "<!doctype html>\n<html lang=\"en\">"
        && start(dark) == "<!doctype html>\n<html data-theme=\"dark\" lang=\"en\">"
        && dark.length() == none.length() + 18, "the theme is set where the page has none");
    check(start(light) == "<!doctype html>\n<html lang=\"en\" data-theme=\"light\">"
        && start(applyBenchmarkReportTheme(light, "dark"))
            == "<!doctype html>\n<html data-theme=\"dark\" lang=\"en\">"
        && applyBenchmarkReportTheme(light, wxEmptyString) == none,
        "a theme is replaced, or removed for the theme of the system");
    check(applyBenchmarkReportTheme("<HTML Data-Theme='light' data-theme=dark lang=en><body>",
        "light") == "<HTML data-theme=\"light\" lang=en><body>"
        && applyBenchmarkReportTheme("<p>no page</p>", "dark") == "<p>no page</p>",
        "twice in any case, and a text without <html>");

    // the policy comes before anything of the page, after its doctype
    const wxString policy = addBenchmarkReportPolicy(none);
    const size_t meta = policy.find("<meta http-equiv=\"Content-Security-Policy\"");
    check(meta == 15 && policy.StartsWith("<!doctype html><meta")
        && policy.Contains("default-src 'none'; script-src 'unsafe-eval';")
        && policy.substr(policy.find('>', meta) + 1) == none.substr(15),
        "a saved page gets the policy right after its doctype");
    check(addBenchmarkReportPolicy("text<p>x</p>").StartsWith("<meta http-equiv=\"Content")
        && addBenchmarkReportPolicy("\n <!DOCTYPE html><html>").StartsWith(
            "\n <!DOCTYPE html><meta http-equiv=")
        && addBenchmarkReportPolicy("<!-- c --><head>").StartsWith("<meta"),
        "the policy is in front of a page without a doctype or <head>, also before text");
}

void writeFixtures(const wxString& dir)
{
    for (const auto& file : fixtureFiles())
    {
        wxFFile out(dir + "/" + file.first, "w");
        out.Write(file.second, wxConvUTF8);
        std::cout << dir.utf8_str() << "/" << file.first.utf8_str() << "\n";
    }
}

} // namespace

// benchmark_test <directory of the test data>: all tests
// benchmark_test --print-golden: the register for registry.golden
// benchmark_test --write-fixtures <directory>: the reports of this format
int main(int argc, char** argv)
{
    const std::string first = argc > 1 ? argv[1] : "";
    const std::string second = argc > 2 ? argv[2] : "";
    wxInitializer initializer(argc, argv);
    if (!initializer.IsOk())
    {
        std::cerr << "Failed to initialize wxWidgets\n";
        return 1;
    }
    if (first == "--print-golden")
    {
        std::cout << goldenText().utf8_str();
        return 0;
    }
    if (first == "--write-fixtures" && !second.empty())
    {
        writeFixtures(wxString::FromUTF8(second.c_str()));
        return 0;
    }
    const wxString dataDirectory = wxString::FromUTF8(first.c_str());

    testHelpers();
    testSafetyCheck();
    testPendingDatabases();
    testLoginAction();
    testTargets();
    testDurationHint();
    testStagePlan();
    testStageStates();
    testAnimation();
    testHints();
    testEventTest();
    testSettings();
    testValueAdvice();
    testParts();
    testRegister();
    testReport();
    testCssFallbacks();
    testHtml();
    testSql();
    testSystemInfoParsers();
    testAnonymized();
    testHiddenTexts();
    testSnapshotFormat();
    testCompareStates();
    testComparison();
    testHtmlParts();
    testComparisonHtml();
    testMarkdown();
    testKeyFigures();
    testKeyValueWork();
    testNeutralNotes();
    testVerdict();
    testReasonLines();
    testResults();
    testPlainReport();
    testGolden(dataDirectory);
    testWorkloadConstants(dataDirectory);
    testStyleSheetSource(dataDirectory);
    testStepsHaveStages(dataDirectory);
    testFixtures(dataDirectory);

    std::cout << (ok ? "ALL BENCHMARK TESTS PASSED\n" : "SOME BENCHMARK TESTS FAILED\n");
    return ok ? 0 : 1;
}
