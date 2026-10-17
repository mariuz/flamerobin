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

#include <wx/intl.h>

#include <algorithm>
#include <cmath>

#include "engine/Benchmark.h"

// The register of every measurement and the key values of a run. Every
// measurement has an id that identifies it in saved reports, in every
// language and in every later version; a comparison of two runs matches
// their measurements by id only, never by a title. The key values are the
// most telling value of every test, as a rate or a time, by part of the
// system. A single run is not judged, because whether a value is fast or
// slow depends on the environment; a comparison of two runs gives points
// (see BenchmarkCompare.cpp). What may be changed in the register is
// explained in devdocs/Benchmark_report_format.md.

namespace fr
{

namespace
{

using C = BenchmarkCategory;
using K = BenchmarkMeasurementKind;

// whether a larger value is faster
const bool higher = true;
const bool lower = false;
// whether every run of an intensity does the same work
const bool fixedWork = true;
const bool timeLimited = false;

BenchmarkMeasurementDef key(const char* id, int version, C section, const char* unit,
    bool higherIsBetter, bool amountFixed, const char* settings, const char* constants,
    const char* sql, const char* title, double BenchmarkValues::*value)
{
    return { id, version, section, K::Key, unit, higherIsBetter, amountFixed, settings,
        constants, sql, title, value, false, false };
}

BenchmarkMeasurementDef measure(const char* id, int version, C section,
    const char* unit, bool higherIsBetter, bool amountFixed, const char* settings,
    const char* constants, const char* sql)
{
    return { id, version, section, K::Measure, unit, higherIsBetter, amountFixed,
        settings, constants, sql, nullptr, nullptr, false, false };
}

BenchmarkMeasurementDef info(const char* id, int version, C section,
    const char* settings = "", const char* constants = "", const char* sql = "")
{
    return { id, version, section, K::Info, "", false, false, settings, constants, sql,
        nullptr, nullptr, false, false };
}

// a measurement with a parameter after its id, e.g. "users.tx.8"
BenchmarkMeasurementDef family(BenchmarkMeasurementDef def)
{
    def.family = true;
    return def;
}

bool isIdCharacter(wxUniChar c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

} // namespace

const std::vector<BenchmarkMeasurementDef>& getBenchmarkMeasurementDefs()
{
    // The version of a test is raised whenever its work changes: other
    // settings, other values of BenchmarkWorkload, other SQL or other code.
    // The settings, the values and the SQL are listed (settings, constants,
    // sql), so that the unit test of the register notices such a change; a
    // change of the code itself it cannot notice.
    static const std::vector<BenchmarkMeasurementDef> defs = {
        // processor of the server
        key("cpu.single", 1, C::ServerCpu, "it/ms", higher, fixedWork,
            "cpuIterations", "", "cpuTest FR_BENCHMARK_CPU FR_BENCHMARK_GTT",
            wxTRANSLATE("Calculating, one user"), &BenchmarkValues::cpuIterationsPerMs),
        key("cpu.parallel", 1, C::ServerCpu, "it/ms", higher, fixedWork,
            "cpuIterations parallelConnections", "",
            "cpuTest FR_BENCHMARK_CPU FR_BENCHMARK_GTT",
            wxTRANSLATE("Calculating, several users at once"),
            &BenchmarkValues::parallelIterationsPerMs),
        info("cpu.burst", 1, C::ServerCpu, "bursts", "serverBurstIterations idlePauseMs",
            "cpuTest FR_BENCHMARK_CPU FR_BENCHMARK_GTT"),

        // storage of the server
        measure("storage.create", 1, C::ServerStorage, "ms", lower, fixedWork, "", "", ""),
        key("storage.commit", 1, C::ServerStorage, "ms", lower, fixedWork, "commitCount",
            "commitRoundTrips", "commitInsert latency FR_BENCHMARK_TEST",
            wxTRANSLATE("Saving a change (commit)"), &BenchmarkValues::commitMs),
        measure("storage.syncWrite", 1, C::ServerStorage, "ms", lower, fixedWork,
            "commitCount", "localFileSize commitsPerSyncWrite minSyncWrites", ""),
        measure("storage.files", 1, C::ServerStorage, "ms", lower, fixedWork, "localFiles",
            "localFileSize", ""),
        info("storage.freeSpace", 1, C::ServerStorage),
        key("storage.insert", 1, C::ServerStorage, "rows/s", higher, fixedWork,
            "diskRows diskPayloadLength", "chunkRows",
            "fillTable FR_BENCHMARK_TEST FR_BENCHMARK_FILL",
            wxTRANSLATE("Writing many rows"), &BenchmarkValues::insertRowsPerSecond),
        measure("storage.update", 1, C::ServerStorage, "rows/s", higher, fixedWork,
            "diskRows diskPayloadLength", "chunkRows", "bulkUpdate FR_BENCHMARK_TEST"),
        measure("storage.delete", 1, C::ServerStorage, "rows/s", higher, fixedWork,
            "diskRows diskPayloadLength", "chunkRows", "bulkDelete FR_BENCHMARK_TEST"),
        measure("storage.gc", 1, C::ServerStorage, "ms", lower, fixedWork,
            "diskRows diskPayloadLength", "", "bulkDelete garbageCollect FR_BENCHMARK_TEST"),
        key("storage.drive", 1, C::ServerStorage, "operations/s", higher, timeLimited,
            "diskRows diskPayloadLength driveSeconds",
            "smallCachePages driveChunkRows driveLookupChunk",
            "fillTable driveLookups bulkUpdate bulkDelete FR_BENCHMARK_TEST "
            "FR_BENCHMARK_FILL FR_BENCHMARK_LOOKUPS",
            wxTRANSLATE("Disk test with a small cache"),
            &BenchmarkValues::driveOperationsPerSecond),
        measure("storage.driveInsert", 1, C::ServerStorage, "rows/s", higher, timeLimited,
            "diskRows diskPayloadLength driveSeconds", "smallCachePages driveChunkRows",
            "fillTable FR_BENCHMARK_TEST FR_BENCHMARK_FILL"),
        measure("storage.driveScan", 1, C::ServerStorage, "ms", lower, timeLimited,
            "diskRows diskPayloadLength driveSeconds", "smallCachePages driveChunkRows",
            "scan FR_BENCHMARK_TEST"),
        measure("storage.driveLookup", 1, C::ServerStorage, "rows/s", higher, timeLimited,
            "diskRows driveSeconds", "smallCachePages driveLookupChunk",
            "driveLookups FR_BENCHMARK_TEST FR_BENCHMARK_LOOKUPS"),
        measure("storage.driveUpdate", 1, C::ServerStorage, "rows/s", higher, timeLimited,
            "diskRows driveSeconds", "smallCachePages driveChunkRows",
            "bulkUpdate FR_BENCHMARK_TEST"),
        measure("storage.driveDelete", 1, C::ServerStorage, "rows/s", higher, timeLimited,
            "diskRows driveSeconds", "smallCachePages driveChunkRows",
            "bulkDelete FR_BENCHMARK_TEST"),
        key("storage.driveParallel", 1, C::ServerStorage, "rows/s", higher, timeLimited,
            "diskRows diskPayloadLength driveSeconds parallelConnections",
            "smallCachePages driveChunkRows parallelDriveTimeShare",
            "fillTable FR_BENCHMARK_TEST FR_BENCHMARK_FILL",
            wxTRANSLATE("Disk test, several users at once"),
            &BenchmarkValues::parallelDriveRowsPerSecond),
        key("storage.tempSort", 1, C::ServerStorage, "MB/s", higher, fixedWork,
            "tempSortMegabytes scanRows scanPayloadLength commitCount", "tempSortMaxCopies",
            "tempSort FR_BENCHMARK_TEMP_SORT FR_BENCHMARK_TEST",
            wxTRANSLATE("Sorting large data on disk"), &BenchmarkValues::tempSortMBPerSecond),
        family(info("storage.pageSize", 1, C::ServerStorage, "diskRows diskPayloadLength",
            "comparedPageSizes commitsPerPageSize commitRoundTrips smallCachePages "
            "chunkRows driveChunkRows",
            "commitInsert fillTable scan FR_BENCHMARK_TEST FR_BENCHMARK_FILL")),

        // memory of the server
        measure("memory.scanCold", 1, C::ServerCache, "MB/s", higher, fixedWork,
            "scanRows scanPayloadLength commitCount", "",
            "fillTable scan FR_BENCHMARK_TEST FR_BENCHMARK_FILL"),
        key("memory.scan", 1, C::ServerCache, "MB/s", higher, fixedWork,
            "scanRows scanPayloadLength commitCount", "",
            "fillTable scan FR_BENCHMARK_TEST FR_BENCHMARK_FILL",
            wxTRANSLATE("Reading data from memory"), &BenchmarkValues::scanMBPerSecond),
        info("memory.coldVsWarm", 1, C::ServerCache, "scanRows scanPayloadLength", "",
            "scan FR_BENCHMARK_TEST"),
        key("memory.sort", 1, C::ServerCache, "MB/s", higher, fixedWork,
            "scanRows scanPayloadLength commitCount", "",
            "serverRead FR_BENCHMARK_READ FR_BENCHMARK_TEST",
            wxTRANSLATE("Sorting in memory"), &BenchmarkValues::sortMBPerSecond),

        // network and connection
        info("network.dns", 1, C::Network),
        info("network.adapter", 1, C::Network),
        info("network.ports", 1, C::Network, "", "checkedPortFirst checkedPortLast"),
        info("network.tcpConnect", 1, C::Network),
        info("network.events", 1, C::Network, "", "eventRetryMs", "postEvent"),
        key("network.roundTrip", 1, C::Network, "ms", lower, fixedWork, "latencyQueries", "",
            "latency", wxTRANSLATE("Response time of a request"),
            &BenchmarkValues::roundTripMs),
        info("network.roundTripSparse", 1, C::Network, "sparseQueries", "sparsePauseMs",
            "latency"),
        info("network.roundTripLarge", 1, C::Network, "latencyQueries",
            "largeRoundTrips largeRoundTripShare", "largeRoundTrip"),
        key("network.transfer", 1, C::Network, "MB/s", higher, fixedWork,
            "scanRows scanPayloadLength commitCount", "", "throughput FR_BENCHMARK_TEST",
            wxTRANSLATE("Transferring a large result"),
            &BenchmarkValues::transferMBPerSecond),
        info("network.timeSplit", 1, C::Network, "scanRows scanPayloadLength commitCount", "",
            "serverRead throughput FR_BENCHMARK_READ FR_BENCHMARK_TEST"),
        key("network.connect", 1, C::Network, "ms", lower, fixedWork, "connectAttempts", "",
            "", wxTRANSLATE("Connecting"), &BenchmarkValues::connectMs),
        key("network.wideInsert", 1, C::Network, "writes/s", higher, fixedWork, "wideRows",
            "wideColumns", "wideInsert FR_BENCHMARK_WIDE",
            wxTRANSLATE("Small writes, one request each"),
            &BenchmarkValues::wideInsertsPerSecond),

        // several users at once
        key("users.oltp", 1, C::MultiUser, "transactions/s", higher, timeLimited,
            "oltpSeconds oltpMaxClients scanRows",
            "oltpClientCounts oltpKeyClients oltpPointSelects oltpRangeRows",
            "oltpPointSelect oltpRangeSelect oltpUpdate oltpInsert FR_BENCHMARK_POINT "
            "FR_BENCHMARK_RANGE FR_BENCHMARK_ROWS FR_BENCHMARK_TEST",
            wxTRANSLATE("Transactions, up to 8 users"),
            &BenchmarkValues::oltpTransactionsPerSecond),
        family(measure("users.tx", 1, C::MultiUser, "tx/s", higher, timeLimited,
            "oltpSeconds scanRows",
            "oltpClientCounts manyUserClientCounts oltpPointSelects oltpRangeRows",
            "oltpPointSelect oltpRangeSelect oltpUpdate oltpInsert FR_BENCHMARK_POINT "
            "FR_BENCHMARK_RANGE FR_BENCHMARK_ROWS FR_BENCHMARK_TEST")),

        // the computer running FlameRobin
        measure("computer.files", 1, C::LocalMachine, "ms", lower, fixedWork, "localFiles",
            "localFileSize", ""),
        key("computer.burst", 1, C::LocalMachine, "ms", lower, fixedWork, "bursts",
            "clientBurstIterations idlePauseMs", "", wxTRANSLATE("Short calculation"),
            &BenchmarkValues::clientCpuBurstMs),
        info("computer.power", 1, C::LocalMachine),
        info("computer.keepalive", 1, C::LocalMachine),

        // programming patterns
        measure("application.rowByRow", 1, C::Application, "rows/s", higher, fixedWork,
            "rowByRowRows", "",
            "rowInsert fillRows FR_BENCHMARK_ROWS FR_BENCHMARK_FILL_ROWS"),
        measure("application.prepare", 1, C::Application, "ms", lower, fixedWork,
            "prepareCount", "", "prepareProbe FR_BENCHMARK_TEST FR_BENCHMARK_ROWS"),
        info("application.blob", 1, C::Application, "blobCount", "blobSize blobChunk",
            "blobInsert blobRead FR_BENCHMARK_BLOB"),
        info("application.versions", 1, C::Application, "versionRounds", "versionReads",
            "hotFill hotUpdate hotRead FR_BENCHMARK_HOT"),

        // settings and engine statistics, compared as facts
        info("settings.runsOn", 1, C::Environment),
        info("settings.firebird", 1, C::Environment, "", "", "engineVersion"),
        info("settings.architecture", 1, C::Environment, "parallelConnections", "",
            "serverProcesses"),
        info("settings.client", 1, C::Environment, "", "", "attachmentInfo"),
        info("settings.protocol", 1, C::Environment, "", "",
            "attachmentInfo attachmentWireInfo"),
        info("settings.login", 1, C::Environment, "", "",
            "attachmentDetails securityDatabase sessionUser"),
        info("settings.timeouts", 1, C::Environment, "", "", "attachmentTimeouts"),
        info("settings.testDatabase", 1, C::Environment, "", "", "databaseInfo"),
        info("settings.selectedSize", 1, C::Environment, "", "", "databaseInfo"),
        info("settings.selectedCache", 1, C::Environment, "", "", "databaseInfo"),
        info("settings.selectedForcedWrites", 1, C::Environment),
        info("settings.selectedTransactions", 1, C::Environment),
        info("settings.selectedLinger", 1, C::Environment, "", "", "databaseLinger"),
        family(info("settings.conf", 1, C::Environment, "", "", "serverConfig")),
        info("settings.pageIo", 1, C::Environment, "", "", "ioStats"),
        info("settings.pageAccess", 1, C::Environment, "", "", "ioStats"),
        info("settings.recordVersions", 1, C::Environment, "", "", "recordStats")
    };
    return defs;
}

const BenchmarkMeasurementDef* findBenchmarkMeasurementDef(const wxString& id)
{
    for (const BenchmarkMeasurementDef& def : getBenchmarkMeasurementDefs())
    {
        if (!def.family && id == def.id)
            return &def;
        // a member of a family: one more part after the id of the family
        wxString parameter;
        if (def.family && id.StartsWith(wxString(def.id) + ".", &parameter)
            && !parameter.empty()
            && std::all_of(parameter.begin(), parameter.end(), isIdCharacter))
        {
            return &def;
        }
    }
    return nullptr;
}

bool isBenchmarkMeasurementId(const wxString& text)
{
    // letters and digits, at least two parts separated by single dots
    if (text.empty() || text.StartsWith(".") || text.EndsWith(".") || text.Contains("..")
        || !text.Contains("."))
    {
        return false;
    }
    return std::all_of(text.begin(), text.end(),
        [](wxUniChar c) { return c == '.' || isIdCharacter(c); });
}

wxString makeBenchmarkMeasurementId(const char* family, const wxString& parameter)
{
    // a setting of firebird.conf could contain other characters
    wxString part;
    for (wxUniChar c : parameter)
    {
        if (isIdCharacter(c))
            part += c;
    }
    return wxString(family) + "." + (part.empty() ? wxString("x") : part);
}

bool isBenchmarkKeyValueSource(const BenchmarkResult& result)
{
    const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(result.id);
    return def && def->kind == K::Key
        && def->section == getBenchmarkReportSection(result.category);
}

wxString formatBenchmarkValue(double value, const BenchmarkMeasurementDef& def)
{
    const wxString unit = def.unit;
    if (unit == "ms")
        return formatBenchmarkMilliseconds(value);
    if (unit == "MB/s")
        return wxString::Format("%.1f ", value) + unit;
    return formatBenchmarkCount(value) + " " + unit;
}

BenchmarkValues getBenchmarkValues(const BenchmarkMetrics& m)
{
    // 0 for every value that was not measured
    BenchmarkValues v{};
    const BenchmarkSettings& s = m.settings;
    const double megabyte = 1024.0 * 1024.0;
    if (m.cpuMs)
    {
        double serverMs = m.cpuServerMs.value_or(*m.cpuMs);
        if (serverMs > 0.0)
            v.cpuIterationsPerMs = s.cpuIterations / serverMs;
        if (m.parallelCpuMs && *m.parallelCpuMs > 0.0)
        {
            v.parallelIterationsPerMs = double(s.cpuIterations) * s.parallelConnections
                / *m.parallelCpuMs;
        }
    }
    // a commit faster than the round trips it was corrected by counts as
    // 0.01 ms
    if (m.commitMs)
        v.commitMs = std::max(*m.commitMs, 0.01);
    if (m.insertMs && *m.insertMs > 0.0)
        v.insertRowsPerSecond = s.diskRows * 1000.0 / *m.insertMs;
    // the drive tests only count when the minimal cache was really used
    if (m.smallCachePages && *m.smallCachePages <= 1000)
    {
        v.driveOperationsPerSecond = getBenchmarkDriveOpsPerSecond(m);
        if (m.parallelDriveMs && *m.parallelDriveMs > 0.0)
            v.parallelDriveRowsPerSecond = m.parallelDriveRows * 1000.0 / *m.parallelDriveMs;
    }
    if (m.tempSortMs && *m.tempSortMs > 0.0 && m.tempSortBytes > 0)
        v.tempSortMBPerSecond = m.tempSortBytes / megabyte / (*m.tempSortMs / 1000.0);
    if (m.warmScanMs && *m.warmScanMs > 0.0 && m.scanBytes > 0)
        v.scanMBPerSecond = m.scanBytes / megabyte / (*m.warmScanMs / 1000.0);
    if (m.serverSortMs && *m.serverSortMs > 0.0 && m.scanBytes > 0)
        v.sortMBPerSecond = m.scanBytes / megabyte / (*m.serverSortMs / 1000.0);
    // the embedded engine runs inside FlameRobin: its requests are function
    // calls, not network traffic
    if (m.connectionKind != BenchmarkConnectionKind::Embedded)
    {
        if (m.latencyMedianMs)
            v.roundTripMs = std::max(*m.latencyMedianMs, 0.001);
        if (m.streamMs && *m.streamMs > 0.0 && m.streamBytes > 0)
            v.transferMBPerSecond = m.streamBytes / megabyte / (*m.streamMs / 1000.0);
        if (m.connectMs)
            v.connectMs = *m.connectMs;
        if (m.wideInsertMs && *m.wideInsertMs > 0.0)
            v.wideInsertsPerSecond = m.wideInsertRows * 1000.0 / *m.wideInsertMs;
    }
    for (const auto& o : m.oltp)
    {
        if (o.seconds > 0.0 && o.clients <= BenchmarkWorkload::oltpKeyClients)
        {
            v.oltpTransactionsPerSecond = std::max(v.oltpTransactionsPerSecond,
                o.transactions / o.seconds);
        }
    }
    if (m.clientBurstHotMs)
        v.clientCpuBurstMs = *m.clientBurstHotMs;
    return v;
}

std::vector<BenchmarkPart> getBenchmarkParts(const BenchmarkMetrics& m)
{
    const BenchmarkValues measured = getBenchmarkValues(m);
    const BenchmarkMachines machines = getBenchmarkMachines(m);
    auto machineOf = [&machines](BenchmarkCategory c)
    {
        switch (c)
        {
            case BenchmarkCategory::Network:
                return machines.network;
            case BenchmarkCategory::MultiUser:
                return machines.both;
            case BenchmarkCategory::LocalMachine:
                return machines.local;
            default:
                return machines.server;
        }
    };

    const auto work = getBenchmarkWork(m);
    std::vector<BenchmarkPart> parts;
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        BenchmarkPart part{ section, getBenchmarkReportSectionName(section),
            machineOf(section), {} };
        for (const BenchmarkMeasurementDef& def : getBenchmarkMeasurementDefs())
        {
            if (def.kind != K::Key || !def.value || def.section != section)
                continue;
            const double value = measured.*def.value;
            if (value <= 0.0)
                continue;
            BenchmarkKeyValue v;
            v.id = def.id;
            v.title = wxGetTranslation(def.title);
            v.value = value;
            v.valueText = formatBenchmarkValue(value, def);
            auto w = work.find(def.id);
            if (w != work.end())
            {
                v.amountText = w->second.text;
                v.durationMs = w->second.ms;
            }
            part.values.push_back(v);
        }
        if (!part.values.empty())
            parts.push_back(part);
    }
    return parts;
}

namespace
{

// the key figures with their short names, in the order of the report
const std::pair<const char*, const char*> headlines[] = {
    { "cpu.single", wxTRANSLATE("Calculation, one user") },
    { "storage.commit", wxTRANSLATE("Saving a change (commit)") },
    { "storage.insert", wxTRANSLATE("Writing rows") },
    { "memory.scan", wxTRANSLATE("Reading from memory") },
    { "network.roundTrip", wxTRANSLATE("Round trip") },
    { "users.oltp", wxTRANSLATE("Transactions, up to 8 users") }
};

// the number and the short unit of a measurement of a run; empty if the
// run has no number for it
std::pair<wxString, wxString> figureOf(const BenchmarkSnapshot& run,
    const BenchmarkMeasurementDef& def)
{
    const BenchmarkMeasurement* x = run.findMeasurement(def.id);
    if (!x || x->value <= 0.0)
        return {};
    return formatBenchmarkFigure(x->value, def);
}

} // namespace

const std::vector<const char*>& getBenchmarkHeadlineIds()
{
    static const std::vector<const char*> ids = []()
    {
        std::vector<const char*> list;
        for (const auto& h : headlines)
            list.push_back(h.first);
        return list;
    }();
    return ids;
}

std::pair<wxString, wxString> formatBenchmarkFigure(double value,
    const BenchmarkMeasurementDef& def)
{
    const wxString unit = def.unit;
    if (unit == "ms")
    {
        // "0.56 ms", "112 ms", "12.3 s"
        const wxString text = formatBenchmarkMilliseconds(value);
        return { text.BeforeLast(' '), text.AfterLast(' ') };
    }
    wxString number;
    if (value >= 100.0)
        number = formatBenchmarkCount(std::round(value));
    else
        number = wxString::Format(value >= 10.0 ? "%.1f" : "%.2f", value);
    wxString shortUnit = unit;
    if (unit == "transactions/s")
        shortUnit = "tx/s";
    else if (unit == "operations/s")
        shortUnit = "ops/s";
    return { number, shortUnit };
}

std::vector<BenchmarkHeadline> getBenchmarkHeadlineValues(const BenchmarkSnapshot& run)
{
    std::vector<BenchmarkHeadline> list;
    for (const auto& h : headlines)
    {
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(h.first);
        const BenchmarkMeasurement* x = run.findMeasurement(h.first);
        if (!def || !x || x->value <= 0.0)
            continue;
        const auto figure = formatBenchmarkFigure(x->value, *def);
        list.push_back({ h.first, def->section, wxGetTranslation(h.second), figure.first,
            figure.second, formatBenchmarkWork(formatBenchmarkAmount(x->amount,
                x->amountUnit), x->ms) });
    }
    return list;
}

std::vector<BenchmarkHeadlineChange> getBenchmarkHeadlineChanges(
    const BenchmarkComparison& comparison)
{
    std::vector<BenchmarkHeadlineChange> list;
    for (const auto& h : headlines)
    {
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(h.first);
        if (!def)
            continue;
        const auto newer = figureOf(comparison.newer, *def);
        const auto older = figureOf(comparison.older, *def);
        if (newer.first.empty() && older.first.empty())
            continue;
        BenchmarkHeadlineChange change;
        change.id = h.first;
        change.section = def->section;
        change.name = wxGetTranslation(h.second);
        change.unit = newer.first.empty() ? older.second : newer.second;
        change.newerNumber = newer.first;
        change.olderNumber = older.first;
        auto value = std::find_if(comparison.values.begin(), comparison.values.end(),
            [&h](const BenchmarkComparedValue& v) { return v.id == h.first; });
        if (value != comparison.values.end())
        {
            change.ratio = value->ratio;
            change.state = value->state;
        }
        else if (newer.first.empty() || older.first.empty())
        {
            change.state = newer.first.empty() ? BenchmarkCompareState::OnlyOlder
                : BenchmarkCompareState::OnlyNewer;
        }
        list.push_back(change);
    }
    return list;
}

wxString getBenchmarkKindId(const BenchmarkMetrics& m)
{
    if (m.connectionKind == BenchmarkConnectionKind::Embedded)
        return "embedded";
    return m.runLocation == BenchmarkRunLocation::Server ? "local" : "client";
}

wxString getBenchmarkKindName(const wxString& kindId)
{
    if (kindId == "embedded")
        return _("this computer, without a server connection");
    if (kindId == "local")
        return _("on the server, local connection");
    if (kindId == "client")
        return _("from a client over the network");
    return kindId;
}

} // namespace fr
