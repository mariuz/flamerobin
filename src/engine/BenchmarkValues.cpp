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

#include "engine/Benchmark.h"

// The key values of a run: the most telling value of every test, as a
// rate or a time, by part of the system. A single run is not judged,
// because whether a value is fast or slow depends on the environment; a
// comparison of two runs gives points (see BenchmarkCompare.cpp).

namespace fr
{

namespace
{

// the multi-user value compares runs with the same number of clients, so
// that the intensities stay comparable
const int oltpKeyClients = 8;

enum class Unit
{
    Count,          // per second or per millisecond, higher is faster
    Megabytes,      // MB/s, higher is faster
    Milliseconds    // lower is faster
};

struct Field
{
    double BenchmarkValues::*member;
    const char* key;            // the name in saved reports
    BenchmarkCategory category;
    const char* title;          // translated with _()
    const char* unit;
    Unit kind;
};

const Field fields[] = {
    { &BenchmarkValues::cpuIterationsPerMs, "cpuIterationsPerMs",
        BenchmarkCategory::ServerCpu, wxTRANSLATE("Calculating, one user"), "it/ms",
        Unit::Count },
    { &BenchmarkValues::parallelIterationsPerMs, "parallelIterationsPerMs",
        BenchmarkCategory::ServerCpu, wxTRANSLATE("Calculating, several users at once"),
        "it/ms", Unit::Count },
    { &BenchmarkValues::commitMs, "commitMs",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Saving a change (commit)"), "ms",
        Unit::Milliseconds },
    { &BenchmarkValues::insertRowsPerSecond, "insertRowsPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Writing many rows"), "rows/s",
        Unit::Count },
    { &BenchmarkValues::driveOperationsPerSecond, "driveOperationsPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Disk test with a small cache"),
        "operations/s", Unit::Count },
    { &BenchmarkValues::parallelDriveRowsPerSecond, "parallelDriveRowsPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Disk test, several users at once"),
        "rows/s", Unit::Count },
    { &BenchmarkValues::tempSortMBPerSecond, "tempSortMBPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Sorting large data on disk"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkValues::scanMBPerSecond, "scanMBPerSecond",
        BenchmarkCategory::ServerCache, wxTRANSLATE("Reading data from memory"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkValues::sortMBPerSecond, "sortMBPerSecond",
        BenchmarkCategory::ServerCache, wxTRANSLATE("Sorting in memory"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkValues::roundTripMs, "roundTripMs",
        BenchmarkCategory::Network, wxTRANSLATE("Response time of a request"), "ms",
        Unit::Milliseconds },
    { &BenchmarkValues::transferMBPerSecond, "transferMBPerSecond",
        BenchmarkCategory::Network, wxTRANSLATE("Transferring a large result"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkValues::connectMs, "connectMs",
        BenchmarkCategory::Network, wxTRANSLATE("Connecting"), "ms", Unit::Milliseconds },
    { &BenchmarkValues::wideInsertsPerSecond, "wideInsertsPerSecond",
        BenchmarkCategory::Network, wxTRANSLATE("Small writes, one request each"), "writes/s",
        Unit::Count },
    { &BenchmarkValues::oltpTransactionsPerSecond, "oltpTransactionsPerSecond",
        BenchmarkCategory::MultiUser, wxTRANSLATE("Transactions, up to 8 users"),
        "transactions/s", Unit::Count },
    { &BenchmarkValues::clientCpuBurstMs, "clientCpuBurstMs",
        BenchmarkCategory::LocalMachine, wxTRANSLATE("Short calculation"), "ms",
        Unit::Milliseconds }
};

wxString formatValue(double value, const Field& field)
{
    switch (field.kind)
    {
        case Unit::Milliseconds:
            return formatBenchmarkMilliseconds(value);
        case Unit::Megabytes:
            return wxString::Format("%.1f ", value) + field.unit;
        case Unit::Count:
        default:
            return formatBenchmarkCount(value) + " " + field.unit;
    }
}

} // namespace

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
        if (o.seconds > 0.0 && o.clients <= oltpKeyClients)
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
        for (const Field& f : fields)
        {
            const double value = measured.*f.member;
            if (f.category != section || value <= 0.0)
                continue;
            BenchmarkKeyValue v;
            v.key = f.key;
            v.title = wxGetTranslation(f.title);
            v.valueText = formatValue(value, f);
            auto w = work.find(f.key);
            if (w != work.end())
            {
                v.amountText = w->second.first;
                v.durationMs = w->second.second;
            }
            part.values.push_back(v);
        }
        if (!part.values.empty())
            parts.push_back(part);
    }
    return parts;
}

std::vector<std::pair<wxString, wxString>> formatBenchmarkValues(
    const BenchmarkValues& values)
{
    std::vector<std::pair<wxString, wxString>> list;
    for (const Field& f : fields)
    {
        const double value = values.*f.member;
        list.push_back({ f.key, value > 0.0
            ? wxString::FromCDouble(value, f.kind == Unit::Milliseconds ? 4 : 1)
            : wxString("0") });
    }
    return list;
}

bool parseBenchmarkValue(BenchmarkValues& values, const wxString& key,
    const wxString& text)
{
    for (const Field& f : fields)
    {
        double number = 0.0;
        if (key == f.key && text.ToCDouble(&number) && number >= 0.0)
        {
            values.*f.member = number;
            return true;
        }
    }
    return false;
}

std::vector<BenchmarkComparedValue> compareBenchmarkValues(
    const BenchmarkValues& newer, const BenchmarkValues& older)
{
    std::vector<BenchmarkComparedValue> list;
    for (const Field& f : fields)
    {
        const double value = newer.*f.member;
        const double base = older.*f.member;
        if (value <= 0.0 && base <= 0.0)
            continue;
        BenchmarkComparedValue c;
        c.category = f.category;
        c.key = f.key;
        c.title = wxGetTranslation(f.title);
        c.newerText = value > 0.0 ? formatValue(value, f) : wxString(wxUniChar(0x2013));
        c.olderText = base > 0.0 ? formatValue(base, f) : wxString(wxUniChar(0x2013));
        if (value > 0.0 && base > 0.0)
            c.ratio = f.kind == Unit::Milliseconds ? base / value : value / base;
        list.push_back(c);
    }
    return list;
}

wxString getBenchmarkValueKey(const wxString& title)
{
    for (const Field& f : fields)
    {
        if (title == wxGetTranslation(f.title))
            return f.key;
    }
    return wxEmptyString;
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
