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

// The benchmark score and the reference system behind its 1000 points.
//
// How to calibrate the references on a new reference system:
// 1. Run the Standard intensity at least three times per kind of run
//    (embedded, local connection on the server, client over the network),
//    on mains power, with nothing else running.
// 2. Copy the block "Calibration values" from every report (Copy as
//    Markdown contains it as well).
// 3. Enter the median of every value in the table of that kind of run
//    below and describe the system in getBenchmarkReferenceSystem().

namespace fr
{

namespace
{

// PROVISIONAL: the reference system will change. Measured on the system of
// getBenchmarkReferenceSystem() (median of 6 Standard runs on mains power);
// the drive test, sort with temporary files and wide inserts are single
// measurements, the values of a client over the network are estimates.
const BenchmarkReferenceValues embeddedReferences = {
    1350.0,     // cpuIterationsPerMs
    4550.0,     // parallelIterationsPerMs
    0.41,       // commitMs
    48000.0,    // insertRowsPerSecond
    12000.0,    // driveOperationsPerSecond
    14000.0,    // parallelDriveRowsPerSecond
    150.0,      // tempSortMBPerSecond
    940.0,      // scanMBPerSecond
    335.0,      // sortMBPerSecond
    0.0,        // roundTripMs: no network
    0.0,        // transferMBPerSecond
    0.0,        // connectMs
    0.0,        // wideInsertsPerSecond
    7000.0,     // oltpTransactionsPerSecond
    4.6         // clientCpuBurstMs
};

const BenchmarkReferenceValues localReferences = {
    1350.0, 4550.0, 0.41, 48000.0, 12000.0, 14000.0, 150.0, 940.0, 335.0,
    0.045,      // roundTripMs
    200.0,      // transferMBPerSecond
    32.0,       // connectMs
    8000.0,     // wideInsertsPerSecond
    4150.0,     // oltpTransactionsPerSecond
    4.6
};

// estimates: the local values with the 0.25 ms round trips of a gigabit
// LAN; an OLTP transaction needs 9 round trips, a wide insert one
const BenchmarkReferenceValues clientReferences = {
    1350.0, 4550.0, 0.41, 48000.0, 12000.0, 14000.0, 150.0, 940.0, 335.0,
    0.25,       // roundTripMs
    100.0,      // transferMBPerSecond
    60.0,       // connectMs
    2500.0,     // wideInsertsPerSecond
    2000.0,     // oltpTransactionsPerSecond
    4.6
};

// a single value can neither dominate the score nor zero it
const double minimumPoints = 1.0;
const double maximumPoints = 5000.0;
// the OLTP value compares runs with the same number of clients
const int oltpScoredClients = 8;

enum class Unit
{
    Count,          // per second or per millisecond, higher is better
    Megabytes,      // MB/s, higher is better
    Milliseconds    // lower is better
};

struct Field
{
    double BenchmarkReferenceValues::*member;
    const char* key;            // name in the calibration block
    BenchmarkCategory category;
    const char* title;          // translated with _()
    const char* unit;
    Unit kind;
};

const Field fields[] = {
    { &BenchmarkReferenceValues::cpuIterationsPerMs, "cpuIterationsPerMs",
        BenchmarkCategory::ServerCpu, wxTRANSLATE("Calculating, one user"), "it/ms",
        Unit::Count },
    { &BenchmarkReferenceValues::parallelIterationsPerMs, "parallelIterationsPerMs",
        BenchmarkCategory::ServerCpu, wxTRANSLATE("Calculating, several users at once"),
        "it/ms", Unit::Count },
    { &BenchmarkReferenceValues::commitMs, "commitMs",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Saving a change (commit)"), "ms",
        Unit::Milliseconds },
    { &BenchmarkReferenceValues::insertRowsPerSecond, "insertRowsPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Writing many rows"), "rows/s",
        Unit::Count },
    { &BenchmarkReferenceValues::driveOperationsPerSecond, "driveOperationsPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Disk test with a small cache"),
        "operations/s", Unit::Count },
    { &BenchmarkReferenceValues::parallelDriveRowsPerSecond, "parallelDriveRowsPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Disk test, several users at once"),
        "rows/s", Unit::Count },
    { &BenchmarkReferenceValues::tempSortMBPerSecond, "tempSortMBPerSecond",
        BenchmarkCategory::ServerStorage, wxTRANSLATE("Sorting large data on disk"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkReferenceValues::scanMBPerSecond, "scanMBPerSecond",
        BenchmarkCategory::ServerCache, wxTRANSLATE("Reading data from memory"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkReferenceValues::sortMBPerSecond, "sortMBPerSecond",
        BenchmarkCategory::ServerCache, wxTRANSLATE("Sorting in memory"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkReferenceValues::roundTripMs, "roundTripMs",
        BenchmarkCategory::Network, wxTRANSLATE("Response time of a request"), "ms",
        Unit::Milliseconds },
    { &BenchmarkReferenceValues::transferMBPerSecond, "transferMBPerSecond",
        BenchmarkCategory::Network, wxTRANSLATE("Transferring a large result"), "MB/s",
        Unit::Megabytes },
    { &BenchmarkReferenceValues::connectMs, "connectMs",
        BenchmarkCategory::Network, wxTRANSLATE("Connecting"), "ms", Unit::Milliseconds },
    { &BenchmarkReferenceValues::wideInsertsPerSecond, "wideInsertsPerSecond",
        BenchmarkCategory::Network, wxTRANSLATE("Small writes, one request each"), "writes/s",
        Unit::Count },
    { &BenchmarkReferenceValues::oltpTransactionsPerSecond, "oltpTransactionsPerSecond",
        BenchmarkCategory::MultiUser, wxTRANSLATE("Transactions, up to 8 users"),
        "transactions/s", Unit::Count },
    { &BenchmarkReferenceValues::clientCpuBurstMs, "clientCpuBurstMs",
        BenchmarkCategory::LocalMachine, wxTRANSLATE("Short calculation"), "ms",
        Unit::Milliseconds }
};

double geometricMean(const std::vector<double>& values)
{
    if (values.empty())
        return 0.0;
    double logSum = 0.0;
    for (double v : values)
        logSum += std::log(v);
    return std::exp(logSum / double(values.size()));
}

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

wxString getBenchmarkReferenceSystem()
{
    return _("Reference system (1000 points), provisional: a 2023 mobile "
        "workstation with an Intel Core i7-13800H, 32 GB RAM and a Samsung 980 "
        "PRO NVMe SSD, Windows 11 and Firebird 5.0.4 with its default "
        "configuration, measured on mains power with the Standard intensity, "
        "embedded and over a local connection. The values of a client over the "
        "network are estimates for a gigabit LAN. Quick and Deep runs use other "
        "amounts of data and are only roughly comparable.");
}

const BenchmarkReferenceValues& getBenchmarkReferenceValues(const BenchmarkMetrics& m,
    wxString* basis)
{
    // values that depend on where FlameRobin runs have their own references
    const wxString kind = getBenchmarkKindId(m);
    if (basis)
        *basis = getBenchmarkKindName(kind);
    if (kind == "embedded")
        return embeddedReferences;
    return kind == "local" ? localReferences : clientReferences;
}

BenchmarkReferenceValues getBenchmarkMeasuredValues(const BenchmarkMetrics& m)
{
    // 0 for every value that was not measured
    BenchmarkReferenceValues v{};
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
    if (m.latencyMedianMs)
        v.roundTripMs = std::max(*m.latencyMedianMs, 0.001);
    if (m.streamMs && *m.streamMs > 0.0 && m.streamBytes > 0)
        v.transferMBPerSecond = m.streamBytes / megabyte / (*m.streamMs / 1000.0);
    if (m.connectMs)
        v.connectMs = *m.connectMs;
    if (m.wideInsertMs && *m.wideInsertMs > 0.0)
        v.wideInsertsPerSecond = m.wideInsertRows * 1000.0 / *m.wideInsertMs;
    for (const auto& o : m.oltp)
    {
        if (o.seconds > 0.0 && o.clients <= oltpScoredClients)
        {
            v.oltpTransactionsPerSecond = std::max(v.oltpTransactionsPerSecond,
                o.transactions / o.seconds);
        }
    }
    if (m.clientBurstHotMs)
        v.clientCpuBurstMs = *m.clientBurstHotMs;
    return v;
}

BenchmarkScore computeBenchmarkScore(const BenchmarkMetrics& m)
{
    BenchmarkScore score;
    const BenchmarkReferenceValues& ref = getBenchmarkReferenceValues(m, &score.basis);
    const BenchmarkReferenceValues measured = getBenchmarkMeasuredValues(m);
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
    for (const Field& f : fields)
    {
        const double value = measured.*f.member;
        const double reference = ref.*f.member;
        if (value <= 0.0 || reference <= 0.0)
            continue;
        auto category = std::find_if(score.categories.begin(), score.categories.end(),
            [&f](const BenchmarkScoreCategory& c) { return c.category == f.category; });
        if (category == score.categories.end())
        {
            score.categories.push_back({ f.category, getBenchmarkCategoryName(f.category),
                machineOf(f.category), {}, 0.0 });
            category = score.categories.end() - 1;
        }
        double points = BenchmarkScore::referencePoints * (f.kind == Unit::Milliseconds
            ? reference / value : value / reference);
        BenchmarkScoreItem item;
        item.key = f.key;
        item.title = wxGetTranslation(f.title);
        item.valueText = formatValue(value, f);
        item.referenceText = formatValue(reference, f);
        item.points = std::clamp(points, minimumPoints, maximumPoints);
        // the reference system does the same work in proportion to its speed
        auto w = work.find(f.key);
        if (w != work.end())
        {
            item.amountText = w->second.first;
            item.durationMs = w->second.second;
            item.referenceMs = w->second.second * points / BenchmarkScore::referencePoints;
        }
        category->items.push_back(item);
    }

    // the fixed order of the report, independent of the order of the fields
    const std::vector<BenchmarkCategory> order = getBenchmarkReportSections();
    std::stable_sort(score.categories.begin(), score.categories.end(),
        [&order](const BenchmarkScoreCategory& a, const BenchmarkScoreCategory& b)
        {
            return std::find(order.begin(), order.end(), a.category)
                < std::find(order.begin(), order.end(), b.category);
        });
    std::vector<double> totals;
    for (auto& c : score.categories)
    {
        std::vector<double> points;
        for (const auto& i : c.items)
            points.push_back(i.points);
        c.points = geometricMean(points);
        totals.push_back(c.points);
    }
    score.total = geometricMean(totals);
    return score;
}

std::vector<std::pair<wxString, wxString>> formatBenchmarkValues(
    const BenchmarkReferenceValues& values)
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

bool parseBenchmarkValue(BenchmarkReferenceValues& values, const wxString& key,
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
    const BenchmarkReferenceValues& newer, const BenchmarkReferenceValues& older)
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

std::vector<BenchmarkKeyValue> getBenchmarkKeyValues(const BenchmarkMetrics& m)
{
    const BenchmarkReferenceValues measured = getBenchmarkMeasuredValues(m);
    std::vector<BenchmarkKeyValue> values;
    for (const Field& f : fields)
    {
        const double value = measured.*f.member;
        values.push_back({ f.category, wxGetTranslation(f.title), f.unit,
            value > 0.0 ? formatValue(value, f) : wxString(wxUniChar(0x2013)) });
    }
    return values;
}

std::vector<std::pair<wxString, wxString>> getBenchmarkCalibrationValues(
    const BenchmarkMetrics& m)
{
    return formatBenchmarkValues(getBenchmarkMeasuredValues(m));
}

} // namespace fr
