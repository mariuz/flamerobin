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
#include <wx/intl.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <tuple>

#include "engine/Benchmark.h"

namespace fr
{

BenchmarkSettings BenchmarkSettings::forIntensity(BenchmarkIntensity intensity)
{
    // connectAttempts, localFiles, cpuIterations, parallelConnections,
    // commitCount, diskRows, diskPayloadLength, scanRows, scanPayloadLength,
    // latencyQueries, sparseQueries, bursts, oltpSeconds, rowByRowRows,
    // prepareCount, blobCount, versionRounds, driveSeconds,
    // tempSortMegabytes, wideRows, oltpMaxClients
    switch (intensity)
    {
        case BenchmarkIntensity::Quick:
            return { 3, 100, 50000, 4, 100, 20000, 200, 10000, 1000, 1000, 15, 5,
                3, 500, 50, 50, 20, 10, 96, 500, 4 };
        case BenchmarkIntensity::Deep:
            return { 5, 500, 500000, 4, 500, 100000, 200, 100000, 1000, 5000, 50, 10,
                10, 5000, 500, 500, 100, 40, 384, 5000, 32 };
        case BenchmarkIntensity::Standard:
        default:
            return { 3, 200, 100000, 4, 200, 50000, 200, 50000, 1000, 2500, 30, 8,
                5, 2000, 200, 200, 50, 20, 192, 2000, 8 };
    }
}

void BenchmarkReport::evaluate()
{
    results = buildBenchmarkResults(metrics);
    parts = getBenchmarkParts(metrics);
    hints = findBenchmarkHints(metrics);
    settings = getBenchmarkSettings(metrics);
}

namespace
{

wxString formatRate(double count, double milliseconds, const wxString& unit)
{
    if (milliseconds <= 0.0)
        return wxEmptyString;
    return formatBenchmarkCount(count * 1000.0 / milliseconds) + " " + unit;
}

wxString formatMegabytes(double bytes)
{
    const double megabytes = bytes / (1024.0 * 1024.0);
    if (megabytes >= 10240.0)
        return wxString::Format(_("%.0f GB"), megabytes / 1024.0);
    if (megabytes >= 1024.0)
        return wxString::Format(_("%.1f GB"), megabytes / 1024.0);
    return wxString::Format(_("%.1f MB"), megabytes);
}

wxString formatMegabytesPerSecond(double bytes, double milliseconds)
{
    if (milliseconds <= 0.0)
        return wxEmptyString;
    return wxString::Format(_("%.1f MB/s"),
        bytes / (1024.0 * 1024.0) / (milliseconds / 1000.0));
}

wxString formatFactor(double factor)
{
    return wxString::Format("%.1fx", factor);
}

} // namespace

wxString formatBenchmarkCount(double value)
{
    // thousands separators; small values keep a decimal
    if (std::abs(value) < 10.0 && value != std::floor(value))
        return wxString::Format("%.1f", value);
    wxString digits = wxString::Format("%.0f", value);
    const bool negative = digits.StartsWith("-");
    if (negative)
        digits.Remove(0, 1);
    wxString grouped;
    const int n = int(digits.length());
    for (int i = 0; i < n; ++i)
    {
        grouped += digits[i];
        if ((n - i - 1) % 3 == 0 && i < n - 1)
            grouped += ',';
    }
    return negative ? "-" + grouped : grouped;
}

wxString formatBenchmarkAverage(double milliseconds)
{
    // operations of a few microseconds are easier to read in microseconds
    if (milliseconds > 0.0 && milliseconds < 1.0)
    {
        const double micro = milliseconds * 1000.0;
        return wxString::Format(micro < 10.0 ? "%.1f " : "%.0f ", micro)
            + wxString::FromUTF8("\xC2\xB5s");
    }
    return formatBenchmarkMilliseconds(milliseconds);
}

wxString formatBenchmarkBitRate(double bitsPerSecond)
{
    if (bitsPerSecond >= 1e9)
    {
        const double g = bitsPerSecond / 1e9;
        return wxString::Format(g == std::floor(g) ? _("%.0f Gbit/s") : _("%.1f Gbit/s"), g);
    }
    return wxString::Format(_("%.0f Mbit/s"), bitsPerSecond / 1e6);
}

double getBenchmarkDriveOpsPerSecond(const BenchmarkMetrics& m)
{
    // all operations of the drive test in their actual time
    if (!m.smallCacheInsertMs || !m.smallCacheRows || m.settings.diskRows <= 0)
        return 0.0;
    double operations = *m.smallCacheRows;
    double ms = *m.smallCacheInsertMs * *m.smallCacheRows / m.settings.diskRows;
    auto add = [&](const std::optional<double>& partMs, int count)
    {
        if (partMs && count > 0)
        {
            operations += count;
            ms += *partMs;
        }
    };
    add(m.smallCacheLookupMs, m.smallCacheLookups);
    add(m.smallCacheUpdateMs, m.smallCacheUpdates);
    add(m.smallCacheDeleteMs, m.smallCacheDeletes);
    return ms > 0.0 ? operations * 1000.0 / ms : 0.0;
}

wxString formatBenchmarkWork(const wxString& amount, double ms)
{
    if (amount.empty() || ms <= 0.0)
        return wxEmptyString;
    return wxString::Format(_("%s in %s"), amount, formatBenchmarkMilliseconds(ms));
}

std::map<wxString, std::pair<wxString, double>> getBenchmarkWork(const BenchmarkMetrics& m)
{
    // the keys of BenchmarkValues.cpp; every value is the amount of work
    // divided by its time, so the time shows how long the test took
    const BenchmarkSettings& s = m.settings;
    std::map<wxString, std::pair<wxString, double>> work;
    auto add = [&work](const char* key, const wxString& amount, double ms)
    {
        if (ms > 0.0)
            work[key] = { amount, ms };
    };
    auto count = [](double n, const wxString& unit)
    {
        return formatBenchmarkCount(n) + " " + unit;
    };
    if (m.cpuMs)
    {
        add("cpuIterationsPerMs", count(s.cpuIterations, _("iterations")),
            m.cpuServerMs.value_or(*m.cpuMs));
    }
    if (m.parallelCpuMs)
    {
        add("parallelIterationsPerMs", count(double(s.cpuIterations) * s.parallelConnections,
            _("iterations")), *m.parallelCpuMs);
    }
    if (m.commitMs)
        add("commitMs", count(s.commitCount, _("commits")), *m.commitMs * s.commitCount);
    if (m.insertMs)
        add("insertRowsPerSecond", count(s.diskRows, _("rows")), *m.insertMs);
    const double driveOps = getBenchmarkDriveOpsPerSecond(m);
    if (driveOps > 0.0 && m.smallCacheRows)
    {
        const double operations = double(*m.smallCacheRows) + m.smallCacheLookups
            + m.smallCacheUpdates + m.smallCacheDeletes;
        add("driveOperationsPerSecond", count(operations, _("operations")),
            operations * 1000.0 / driveOps);
    }
    if (m.parallelDriveMs && m.parallelDriveRows > 0)
    {
        add("parallelDriveRowsPerSecond", count(double(m.parallelDriveRows), _("rows")),
            *m.parallelDriveMs);
    }
    if (m.tempSortMs && m.tempSortBytes > 0)
        add("tempSortMBPerSecond", formatMegabytes(double(m.tempSortBytes)), *m.tempSortMs);
    if (m.warmScanMs && m.scanBytes > 0)
        add("scanMBPerSecond", formatMegabytes(double(m.scanBytes)), *m.warmScanMs);
    if (m.serverSortMs && m.scanBytes > 0)
        add("sortMBPerSecond", formatMegabytes(double(m.scanBytes)), *m.serverSortMs);
    if (m.latencyAvgMs)
    {
        add("roundTripMs", count(s.latencyQueries, _("requests")),
            *m.latencyAvgMs * s.latencyQueries);
    }
    if (m.streamMs && m.streamBytes > 0)
        add("transferMBPerSecond", formatMegabytes(double(m.streamBytes)), *m.streamMs);
    if (m.connectMs)
    {
        add("connectMs", count(s.connectAttempts, _("connections")),
            *m.connectMs * s.connectAttempts);
    }
    if (m.wideInsertMs && m.wideInsertRows > 0)
        add("wideInsertsPerSecond", count(m.wideInsertRows, _("rows")), *m.wideInsertMs);
    const BenchmarkOltpResult* best = nullptr;
    for (const auto& o : m.oltp)
    {
        if (o.seconds > 0.0 && o.clients <= 8 && (!best
            || o.transactions / o.seconds > best->transactions / best->seconds))
        {
            best = &o;
        }
    }
    if (best)
    {
        add("oltpTransactionsPerSecond", count(double(best->transactions),
            wxString::Format(_("transactions of %d users"), best->clients)),
            best->seconds * 1000.0);
    }
    if (m.clientBurstHotMs)
    {
        add("clientCpuBurstMs", count(s.bursts, _("calculations")),
            *m.clientBurstHotMs * s.bursts);
    }
    return work;
}

BenchmarkMachines getBenchmarkMachines(const BenchmarkMetrics& m)
{
    BenchmarkMachines r;
    if (m.connectionKind == BenchmarkConnectionKind::Embedded)
    {
        // FlameRobin and the engine share one process on one machine
        r.local = r.server = wxString::Format(_("this machine %s (embedded engine)"),
            m.localHostName);
        r.network = wxString::Format(_("no network (embedded engine on %s)"),
            m.localHostName);
    }
    else if (m.runLocation == BenchmarkRunLocation::Server)
    {
        r.local = r.server = wxString::Format(_("server %s (this machine)"),
            m.localHostName);
        r.network = wxString::Format(_("local connection on %s"), m.localHostName);
    }
    else
    {
        wxString host = m.connectionHost.empty() ? _("server") : m.connectionHost;
        r.local = wxString::Format(_("client %s (this machine)"), m.localHostName);
        r.server = m.connectionHost.empty() ? host
            : wxString::Format(_("server %s"), m.connectionHost);
        r.network = wxString::Format(_("network %s <-> %s"), m.localHostName, host);
    }
    r.both = r.local == r.server ? r.local
        : wxString::Format(_("%s and %s"), r.local, r.server);
    return r;
}

wxString formatBenchmarkPoints(double points)
{
    return formatBenchmarkCount(std::round(points));
}

wxString getBenchmarkSectionDescription(BenchmarkCategory section,
    const BenchmarkMetrics& m)
{
    switch (section)
    {
        case BenchmarkCategory::ServerCpu:
            return _("How fast the server calculates, for example for queries, "
                "stored procedures and sorting.");
        case BenchmarkCategory::ServerStorage:
            return _("How fast the server saves data on its disk and reads it back.");
        case BenchmarkCategory::ServerCache:
            return _("How fast the server works with data that it keeps in memory.");
        case BenchmarkCategory::Network:
            if (m.connectionKind == BenchmarkConnectionKind::Embedded
                || m.runLocation == BenchmarkRunLocation::Server)
            {
                return _("The connection between FlameRobin and Firebird on the same "
                    "computer.");
            }
            return _("How fast requests and data travel between this computer and the "
                "server.");
        case BenchmarkCategory::MultiUser:
            return _("How much work the server manages when several users work at the "
                "same time.");
        case BenchmarkCategory::LocalMachine:
            return _("The computer FlameRobin runs on: its speed, power saving and "
                "virus scanner.");
        case BenchmarkCategory::Application:
            return _("How expensive typical ways of programming are; of interest to "
                "software developers.");
        case BenchmarkCategory::Environment:
            return _("Settings of Firebird, of the test database and of the operating "
                "system.");
        default:
            return wxEmptyString;
    }
}

std::vector<BenchmarkResult> buildBenchmarkResults(const BenchmarkMetrics& m)
{
    std::vector<BenchmarkResult> r;
    const BenchmarkSettings& s = m.settings;
    using C = BenchmarkCategory;
    // adds a result; the caller may fill the statistics columns of the
    // returned entry right away (the reference is only valid until the
    // next add)
    auto add = [&r](C c, const wxString& machine, const wxString& test,
        const wxString& value, const wxString& details, const wxString& hint,
        const wxString& explanation) -> BenchmarkResult&
    {
        BenchmarkResult result;
        result.category = c;
        result.machine = machine;
        result.test = test;
        result.value = value;
        result.details = details;
        result.hint = hint;
        result.explanation = explanation;
        r.push_back(std::move(result));
        return r.back();
    };
    // amount, duration, average and rate of count operations in ms
    auto stats = [](BenchmarkResult& result, const wxString& amount, double count,
        double ms, const wxString& unit)
    {
        result.amount = amount;
        if (ms > 0.0)
        {
            result.duration = formatBenchmarkMilliseconds(ms);
            result.durationMs = ms;
        }
        if (count > 0.0 && ms > 0.0)
        {
            result.average = formatBenchmarkAverage(ms / count);
            result.rate = formatBenchmarkCount(count * 1000.0 / ms) + " " + unit;
        }
    };
    auto count = [](double n, const wxString& unit)
    {
        return formatBenchmarkCount(n) + " " + unit;
    };

    const BenchmarkMachines machines = getBenchmarkMachines(m);
    const wxString& local = machines.local;
    const wxString& server = machines.server;
    const wxString& network = machines.network;
    const wxString& both = machines.both;

    // configuration and environment
    add(C::Environment, local, _("Benchmark runs on"),
        getBenchmarkRunLocationName(m.runLocation),
        wxString::Format(_("connection: %s"),
            getBenchmarkConnectionKindName(m.connectionKind)),
        _("Where FlameRobin runs: on the database server or on a client."),
        _("Derived from the connection protocol, the host name and whether "
          "the database file is visible locally. A run on the server shows "
          "the server alone; a run on a client adds the network and the "
          "client, so comparing both separates server problems from network "
          "and client problems."));
    if (!m.serverVersion.empty())
    {
        add(C::Environment, server, _("Firebird version / server OS"),
            m.serverVersion + (m.serverPlatform.empty() ? wxString()
                : " / " + m.serverPlatform),
            m.serverImplementation,
            _("Version and operating system as reported by the server."),
            _("Read with RDB$GET_CONTEXT and the version information of the "
              "connection. Tests and recommendations depend on the version "
              "(RDB$CONFIG from 4.0, parallel workers and the statement cache "
              "from 5.0). Firebird does not report CPU, RAM or drives of the "
              "server machine; a run on the server shows them."));
    }
    if (m.serverProcesses)
    {
        add(C::Environment, server, _("Server architecture"),
            *m.serverProcesses > 1 ? _("Classic") : _("SuperServer or SuperClassic"),
            wxString::Format(_("%d server processes for %d connections"),
                *m.serverProcesses, s.parallelConnections),
            _("Whether all connections share one server process."),
            _("Counts the server processes (MON$SERVER_PID) while the parallel "
              "test connections are open. SuperServer shares one page cache "
              "between all connections, Classic has one process and cache per "
              "connection; this changes the right cache settings. Works on "
              "Firebird 3 as well, where RDB$CONFIG is missing."));
    }
    if (!m.clientVersion.empty())
    {
        add(C::Environment, local, _("Firebird client library"), m.clientVersion,
            wxEmptyString, _("The fbclient library FlameRobin uses."),
            _("Version of the client library (MON$CLIENT_VERSION). An older "
              "client cannot use newer protocol features of the server, such "
              "as wire compression or batches."));
    }
    if (!m.remoteProtocol.empty())
    {
        wxString details = m.protocolVersion;
        if (m.wireCompressed || m.wireEncrypted)
        {
            details += (details.empty() ? "" : ", ") + wxString::Format(
                _("compression %s, encryption %s"),
                m.wireCompressed.value_or(false) ? _("on") : _("off"),
                m.wireEncrypted.value_or(false)
                    ? (m.wireCryptPlugin.empty() ? _("on") : m.wireCryptPlugin)
                    : _("off"));
        }
        add(C::Environment, network, _("Network protocol"), m.remoteProtocol, details,
            _("Network protocol, its version, compression and encryption."),
            _("From MON$ATTACHMENTS of the benchmark connection. Compression "
              "helps on slow links and costs CPU on fast ones; encryption "
              "costs CPU on both sides."));
    }
    if (!m.authMethod.empty())
    {
        add(C::Environment, server, _("Login"), m.authMethod,
            m.securityDatabase.empty() ? wxString()
                : wxString::Format(_("security database: %s"), m.securityDatabase),
            _("How the benchmark connection logged in."),
            _("The authentication plugin (MON$AUTH_METHOD) and the security "
              "database of the logins. SRP with a high cost and Windows "
              "authentication with slow domain controllers make every new "
              "connection slower."));
    }
    if (m.idleTimeoutSeconds || m.statementTimeoutMs)
    {
        add(C::Environment, server, _("Connection timeouts"),
            wxString::Format(_("idle %s, statements %s"),
                m.idleTimeoutSeconds.value_or(0) > 0
                    ? wxString::Format(_("%d min"), *m.idleTimeoutSeconds / 60)
                    : _("none"),
                m.statementTimeoutMs.value_or(0) > 0
                    ? formatBenchmarkMilliseconds(*m.statementTimeoutMs)
                    : _("none")),
            _("from ConnectionIdleTimeout and StatementTimeout"),
            _("Whether Firebird itself closes idle connections."),
            _("The timeouts of the benchmark connection (Firebird 4+). An idle "
              "timeout makes Firebird close connections that are unused for "
              "that time; applications then report lost connections, exactly "
              "like a firewall that drops idle connections."));
    }
    const BenchmarkDatabaseInfo& bdb = m.benchmarkDatabase;
    if (bdb.pageSize > 0)
    {
        add(C::Environment, server, _("Test database"),
            wxString::Format(_("page size %d"), bdb.pageSize),
            wxString::Format(_("in %s; ODS %d.%d, page cache %d pages (%s), "
                "forced writes %s"), m.benchmarkDirectory,
                bdb.odsMajor, bdb.odsMinor, bdb.cachePages,
                formatMegabytes(double(bdb.cachePages) * bdb.pageSize),
                bdb.forcedWrites ? _("ON") : _("OFF")),
            _("The database all tests run in; it is dropped afterwards."),
            _("Created for this run with a unique name and dropped at the end. "
              "Its page size and the page cache influence the storage and "
              "cache results."));
    }
    if (m.inspectedDatabase)
    {
        const BenchmarkDatabaseInfo& idb = *m.inspectedDatabase;
        const wxString hint = _("Read-only information of the selected database.");
        const wxString why = _("Read from the header and MON$DATABASE of the "
            "selected database, only when asked for; nothing is changed. Used to "
            "judge its configuration.");
        add(C::Environment, server, _("Selected database: size"),
            formatMegabytes(double(idb.pages) * idb.pageSize),
            wxString::Format(_("ODS %d.%d, page size %d"),
                idb.odsMajor, idb.odsMinor, idb.pageSize), hint, why);
        add(C::Environment, server, _("Selected database: page cache"),
            wxString::Format(_("%d pages"), idb.cachePages),
            wxString::Format(_("%s; page buffers in the header: %d (0 = "
                "DefaultDbCachePages)"),
                formatMegabytes(double(idb.cachePages) * idb.pageSize),
                idb.pageBuffers), hint, why);
        add(C::Environment, server, _("Selected database: forced writes"),
            idb.forcedWrites ? _("ON") : _("OFF"), wxEmptyString, hint, why);
        add(C::Environment, server, _("Selected database: transaction gap"),
            wxString::Format("%lld",
                (long long)(idb.nextTransaction - idb.oldestActiveTransaction)),
            wxString::Format(_("next - oldest active; sweep interval %d"),
                idb.sweepInterval), hint,
            _("A large gap means a long running transaction that blocks "
              "garbage collection."));
        if (idb.lingerSeconds)
        {
            add(C::Environment, server, _("Selected database: LINGER"),
                wxString::Format(_("%d s"), *idb.lingerSeconds), wxEmptyString, hint,
                _("How long SuperServer keeps the database and its page cache "
                  "open after the last connection closed."));
        }
    }
    for (const auto& kv : m.serverConfig)
    {
        add(C::Environment, server, "firebird.conf: " + kv.first, kv.second,
            m.serverConfigExplicit.count(kv.first) ? _("set in firebird.conf")
                                                   : _("default value"),
            _("Effective server setting."),
            _("Read from RDB$CONFIG (Firebird 4+, administrators only): the "
              "value the server uses and whether firebird.conf sets it."));
    }

    // network and connection
    if (m.networkPath)
    {
        const BenchmarkNetworkPath& p = *m.networkPath;
        if (p.resolveMs >= 0.0)
        {
            add(C::Connection, local, _("Finding the server by its name (DNS)"),
                formatBenchmarkMilliseconds(p.resolveMs),
                wxString::Format(_("%s -> %s"), wxString::FromUTF8(p.serverHost.c_str()),
                    p.serverAddress.empty() ? _("not resolved")
                        : wxString::FromUTF8(p.serverAddress.c_str())),
                _("Time to find the IP address of the server."),
                _("Resolves the server name like a new connection does (DNS, "
                  "hosts file). The operating system caches the answer, so the "
                  "value is often small; a slow first lookup delays every new "
                  "connection of an application that starts."));
        }
        if (!p.adapterName.empty())
        {
            wxString type = wxString::FromUTF8(p.adapterType.c_str());
            wxString details = wxString::FromUTF8(p.adapterDescription.c_str());
            if (p.linkSpeedBitsPerSecond > 0)
            {
                details += (details.empty() ? "" : ", ") + wxString::Format(
                    _("link %s"), formatBenchmarkBitRate(double(p.linkSpeedBitsPerSecond)));
            }
            if (p.mtu > 0)
                details += wxString::Format(_(", MTU %d"), p.mtu);
            if (!p.wifiSignal.empty())
                details += _(", signal ") + wxString::FromUTF8(p.wifiSignal.c_str());
            if (p.sameSubnet)
            {
                details += *p.sameSubnet ? _(", server in the same subnet")
                                         : _(", server reached through a router");
            }
            add(C::Connection, local, _("Network adapter to the server"),
                type.empty() ? wxString::FromUTF8(p.adapterName.c_str())
                    : type + " (" + wxString::FromUTF8(p.adapterName.c_str()) + ")",
                details,
                _("The network connection this machine uses to reach the server."),
                _("Found with the route the operating system chooses for the "
                  "server address (no packets are sent). Wi-Fi and VPN add "
                  "latency and fluctuations; the link speed limits the transfer "
                  "of large result sets; a router between client and server adds "
                  "latency and often a firewall."));
        }
    }
    if (!m.serverPorts.empty())
    {
        wxString open, closed, filtered;
        double handshakeMs = -1.0;
        for (const auto& c : m.serverPorts)
        {
            wxString& list = c.state == BenchmarkPortCheck::State::Open ? open
                : c.state == BenchmarkPortCheck::State::Closed ? closed : filtered;
            list += (list.empty() ? "" : ", ") + wxString::Format("%d", c.port);
            if (c.port == m.serverPort && c.state == BenchmarkPortCheck::State::Open)
                handshakeMs = c.ms;
        }
        wxString details;
        if (!closed.empty())
            details += wxString::Format(_("closed (no server): %s"), closed);
        if (!filtered.empty())
        {
            details += (details.empty() ? "" : "; ")
                + wxString::Format(_("no answer (firewall): %s"), filtered);
        }
        add(C::Connection, local, _("Server ports reachable from here"),
            open.empty() ? _("none") : open, details,
            _("Which Firebird ports of the server this machine can reach."),
            _("Opens a TCP connection to the ports 3050 to 3060, the port of "
              "the connection and a fixed RemoteAuxPort at the same time and "
              "closes it at once. Open: a server listens. Closed: the server "
              "answers but nothing listens. No answer: a firewall drops the "
              "packets. Finds other Firebird instances and blocked ports in a "
              "few seconds."));
        if (handshakeMs >= 0.0)
        {
            add(C::Connection, network, _("Network part of a connection (TCP)"),
                formatBenchmarkMilliseconds(handshakeMs),
                wxString::Format(_("to port %d, without login"), m.serverPort),
                _("The network part of a new connection."),
                _("The pure TCP handshake to the Firebird port. Compared with the "
                  "complete connect time it shows how much of a connection is "
                  "network and how much is login, encryption and the server "
                  "itself."));
        }
    }
    if (m.connectMs)
    {
        add(C::Connection, network, _("Connecting with login"),
            formatBenchmarkMilliseconds(*m.connectMs),
            wxString::Format(_("average of %d connections"), s.connectAttempts),
            _("Time to open a complete Firebird connection with login."),
            _("Opens and closes complete connections. Includes name "
              "resolution, authentication (SRP, Win_Sspi, ...), wire "
              "encryption and the attachment on the server. Matters for "
              "applications that connect per request."));
        stats(r.back(), count(s.connectAttempts, _("connections")), s.connectAttempts,
            *m.connectMs * s.connectAttempts, _("connections/s"));
    }
    if (m.eventMs || !m.eventStatus.empty())
    {
        add(C::Connection, network, _("Events (notifications, POST_EVENT)"),
            m.eventMs ? formatBenchmarkMilliseconds(*m.eventMs) : _("not received"),
            m.eventMs ? _("from posting until FlameRobin was notified") : m.eventStatus,
            _("Whether database events reach this machine."),
            _("Registers for an event, posts it with POST_EVENT and waits for "
              "the notification. Remote clients receive events through a "
              "second connection to the RemoteAuxPort of the server; if a "
              "firewall blocks that port, applications that wait for events "
              "hang or never react. A random RemoteAuxPort (0) is usually "
              "blocked by firewalls."));
    }

    // this machine
    if (m.tempDirFileMs)
    {
        add(C::LocalMachine, local, _("Creating and deleting small files"),
            formatBenchmarkMilliseconds(*m.tempDirFileMs),
            wxString::Format(_("per file, in %s"), m.tempDirectory),
            _("File handling speed; on-access virus scanners slow it down."),
            _("Creates, writes (4 KB) and deletes small files in the temporary "
              "directory. On-access virus scanners inspect every new file, "
              "which makes this slow; the same scanners slow down applications "
              "and Firebird files."));
        stats(r.back(), count(s.localFiles, _("files")), s.localFiles,
            *m.tempDirFileMs * s.localFiles, _("files/s"));
    }
    if (m.clientBurstIdleMs && m.clientBurstHotMs)
    {
        add(C::LocalMachine, local, _("Short calculation after a pause / without a pause"),
            formatBenchmarkMilliseconds(*m.clientBurstIdleMs) + " / "
                + formatBenchmarkMilliseconds(*m.clientBurstHotMs),
            _("a large difference points to power saving"),
            _("Short CPU work after a pause compared with back to back."),
            _("Runs the same short computation after idle pauses and back to "
              "back. With power saving the CPU clocks down while idle and the "
              "first burst is slower - interactive applications feel exactly "
              "this delay."));
        r.back().amount = count(2.0 * s.bursts, _("bursts"));
    }
    if (m.localOnBattery)
    {
        add(C::LocalMachine, local, _("Power supply"),
            *m.localOnBattery ? _("battery") : _("mains"),
            wxString::FromUTF8(m.system.powerPlan.c_str()),
            _("Mains or battery; notebooks are slower on battery."),
            _("Reported by the operating system, together with the power plan. "
              "Notebooks lower the CPU clock and often the storage speed on "
              "battery, so runs on battery are not comparable with runs on "
              "mains power."));
    }
    if (m.system.tcpKeepAliveSeconds)
    {
        add(C::LocalMachine, local, _("TCP keepalive"),
            wxString::Format(_("after %d min idle"), *m.system.tcpKeepAliveSeconds / 60),
            wxEmptyString,
            _("When the operating system checks idle connections."),
            _("The idle time after which TCP sends keepalive packets on an "
              "unused connection (Windows KeepAliveTime, Linux "
              "net.ipv4.tcp_keepalive_time). Firewalls and NAT routers often "
              "drop connections that are idle for 5 to 60 minutes; keepalive "
              "packets sent before that keep them open."));
    }

    // server CPU
    if (m.cpuMs)
    {
        const double ms = m.cpuServerMs.value_or(*m.cpuMs);
        add(C::ServerCpu, server, _("Calculation, one connection"),
            formatBenchmarkMilliseconds(ms),
            wxString::Format(_("%s iterations"), formatBenchmarkCount(s.cpuIterations)),
            _("Pure CPU work of one connection inside Firebird."),
            _("A PSQL loop with MOD, SQRT, SUBSTRING and CASE that writes to a "
              "global temporary table, so there is no database I/O. The time "
              "is measured inside the server and shows the single core speed "
              "available to Firebird."));
        stats(r.back(), count(s.cpuIterations, _("iterations")), s.cpuIterations, ms,
            _("it/s"));
    }
    if (m.parallelCpuMs && m.cpuMs)
    {
        double scaling = *m.cpuMs * s.parallelConnections / *m.parallelCpuMs;
        add(C::ServerCpu, server,
            wxString::Format(_("Calculation, %d connections at once"),
                s.parallelConnections),
            formatBenchmarkMilliseconds(*m.parallelCpuMs),
            wxString::Format(_("scaling %s (ideal %dx)"), formatFactor(scaling),
                s.parallelConnections),
            _("The same CPU work on several connections at once."),
            _("Runs the procedure on several connections at the same time. "
              "Shows whether Firebird can use several CPU cores "
              "(CpuAffinityMask, number of vCPUs, server mode) and whether "
              "other load on the server takes cores away."));
        stats(r.back(), count(double(s.cpuIterations) * s.parallelConnections,
            _("iterations")), double(s.cpuIterations) * s.parallelConnections,
            *m.parallelCpuMs, _("it/s"));
    }
    if (m.serverBurstIdleMs && m.serverBurstHotMs)
    {
        add(C::ServerCpu, server, _("Short calculation after a pause / without a pause"),
            formatBenchmarkMilliseconds(*m.serverBurstIdleMs) + " / "
                + formatBenchmarkMilliseconds(*m.serverBurstHotMs),
            _("measured inside the server"),
            _("Power saving of the server CPU."),
            _("Like the burst test of this machine, but inside the server: "
              "detects power saving of the server or of its virtualization "
              "host."));
        r.back().amount = count(2.0 * s.bursts, _("bursts"));
    }

    // server storage
    const wxString storage = wxString::Format(_("storage of %s"), m.benchmarkDirectory);
    if (m.createDatabaseMs)
    {
        add(C::ServerStorage, server, _("Creating the test database"),
            formatBenchmarkMilliseconds(*m.createDatabaseMs), storage,
            _("Time to create an empty database."),
            _("Creating the database writes and flushes its first pages; "
              "mostly file system and storage work on the server, slowed down "
              "by virus scanners that inspect new files."));
        r.back().durationMs = *m.createDatabaseMs;
    }
    if (m.commitMs)
    {
        add(C::ServerStorage, server, _("Saving a small change (commit)"),
            formatBenchmarkMilliseconds(*m.commitMs),
            _("network round trips subtracted"),
            _("How long the storage needs to make a change permanent."),
            _("Inserts one row and commits, many times. With forced writes "
              "every commit waits until the storage confirms that the pages "
              "are written. The most important storage value for typical "
              "applications with many small transactions."));
        stats(r.back(), count(s.commitCount, _("transactions")), s.commitCount,
            *m.commitMs * s.commitCount, _("tx/s"));
    }
    if (m.osSyncWriteMs)
    {
        add(C::ServerStorage, server, _("Writing a file directly, without Firebird"),
            formatBenchmarkMilliseconds(*m.osSyncWriteMs), storage,
            _("A synchronous write outside Firebird, for comparison."),
            _("Writes 4 KB to a file in the database directory and flushes it, "
              "outside Firebird. If this is fast but the commit is slow, the "
              "cause is not the storage (e.g. a virus scanner watching the "
              "database file)."));
        r.back().durationMs = *m.osSyncWriteMs;
        r.back().average = formatBenchmarkAverage(*m.osSyncWriteMs);
    }
    if (m.databaseDirFileMs)
    {
        add(C::ServerStorage, server, _("Creating and deleting small files"),
            formatBenchmarkMilliseconds(*m.databaseDirFileMs),
            wxString::Format(_("per file, in %s"), m.benchmarkDirectory),
            _("File handling in the database directory."),
            _("The file test in the database directory: shows whether this "
              "directory is excluded from on-access virus scanning."));
        stats(r.back(), count(s.localFiles, _("files")), s.localFiles,
            *m.databaseDirFileMs * s.localFiles, _("files/s"));
    }
    if (m.databaseDiskFreeBytes)
    {
        add(C::ServerStorage, server, _("Free disk space"),
            formatMegabytes(double(*m.databaseDiskFreeBytes)), storage,
            _("Free space on the drive of the database."),
            _("Database growth, sort files and backups need free space."));
    }
    if (m.insertMs)
    {
        add(C::ServerStorage, server, _("Writing rows (table with 5 indexes)"),
            formatRate(s.diskRows, *m.insertMs, _("rows/s")), _("normal page cache"),
            _("Bulk insert into a table with 5 indexes."),
            _("Rows are generated inside the server (no network) into a "
              "table with 5 indexes; measures page writes and index "
              "maintenance."));
        stats(r.back(), count(s.diskRows, _("rows")), s.diskRows, *m.insertMs,
            _("rows/s"));
    }
    if (m.updateMs)
    {
        add(C::ServerStorage, server, _("Changing many rows"),
            formatRate(s.diskRows, *m.updateMs, _("rows/s")), _("normal page cache"),
            _("Changes every row and all its indexed columns."),
            _("Updates every row and changes all indexed columns: Firebird "
              "writes a new record version, keeps the old one as back "
              "version and adds new entries to all 5 indexes."));
        stats(r.back(), count(s.diskRows, _("rows")), s.diskRows, *m.updateMs,
            _("rows/s"));
    }
    if (m.deleteMs)
    {
        add(C::ServerStorage, server, _("Deleting many rows"),
            formatRate(s.diskRows, *m.deleteMs, _("rows/s")), _("normal page cache"),
            _("Deletes every row."),
            _("Deletes all rows; the space is only freed by garbage "
              "collection."));
        stats(r.back(), count(s.diskRows, _("rows")), s.diskRows, *m.deleteMs,
            _("rows/s"));
    }
    if (m.garbageCollectMs)
    {
        add(C::ServerStorage, server, _("Cleaning up deleted rows (garbage collection)"),
            formatBenchmarkMilliseconds(*m.garbageCollectMs),
            _("first full scan after the delete"),
            _("Removing the deleted rows."),
            _("The first read after the mass delete visits all deleted "
              "records. With cooperative garbage collection (Classic, "
              "SuperClassic, GCPolicy cooperative or combined) this read "
              "removes them; with GCPolicy = background a server thread "
              "does it later. In applications this cost appears at "
              "unexpected places, e.g. in a simple SELECT."));
        r.back().duration = formatBenchmarkMilliseconds(*m.garbageCollectMs);
        r.back().durationMs = *m.garbageCollectMs;
    }

    // the drive test with the minimal page cache
    const wxString cacheInfo = wxString::Format(_("page cache of %d pages"),
        m.smallCachePages.value_or(0));
    const wxString driveWhy = _("With an almost empty page cache nearly every "
        "page access becomes operating system I/O, so the result shows the "
        "storage and its drivers, controller and RAID configuration rather than "
        "RAM.");
    const double driveOps = getBenchmarkDriveOpsPerSecond(m);
    if (driveOps > 0.0)
    {
        add(C::ServerStorage, server, _("Disk test with a small cache"),
            count(driveOps, _("operations/s")),
            _("insert, read, update and delete with the minimal cache"),
            _("All operations of the drive test together."),
            driveWhy + " " + _("The rows inserted with the minimal cache are "
                "then read by primary key, updated and deleted; every part "
                "stops after its time limit. This value adds up all "
                "operations."));
    }
    if (m.smallCacheInsertMs && m.smallCacheRows)
    {
        const int rows = *m.smallCacheRows;
        const double ms = *m.smallCacheInsertMs * rows / s.diskRows;
        add(C::ServerStorage, server, _("Disk test: writing"),
            formatRate(rows, ms, _("rows/s")), cacheInfo,
            _("Inserts with a minimal page cache."),
            driveWhy + " " + _("Like the insert with the normal cache, the rows "
                "go into new pages, so both values are comparable."));
        stats(r.back(), count(rows, _("rows")), rows, ms, _("rows/s"));
    }
    if (m.smallCacheScanMs)
    {
        add(C::ServerStorage, server, _("Disk test: reading everything"),
            formatBenchmarkMilliseconds(*m.smallCacheScanMs), cacheInfo,
            _("A full table scan with the minimal page cache."),
            driveWhy + " " + _("Reads are usually still served by the "
                "operating system file cache, so this value is only slow when "
                "that cache is small as well."));
        r.back().duration = formatBenchmarkMilliseconds(*m.smallCacheScanMs);
        r.back().durationMs = *m.smallCacheScanMs;
    }
    if (m.smallCacheLookupMs && m.smallCacheLookups > 0)
    {
        add(C::ServerStorage, server, _("Disk test: reading single rows"),
            formatRate(m.smallCacheLookups, *m.smallCacheLookupMs, _("rows/s")),
            cacheInfo, _("Random primary key lookups with the minimal cache."),
            driveWhy + " " + _("The lookups jump through the table inside a "
                "stored procedure, so the network does not count."));
        stats(r.back(), count(m.smallCacheLookups, _("lookups")), m.smallCacheLookups,
            *m.smallCacheLookupMs, _("rows/s"));
    }
    if (m.smallCacheUpdateMs && m.smallCacheUpdates > 0)
    {
        add(C::ServerStorage, server, _("Disk test: changing"),
            formatRate(m.smallCacheUpdates, *m.smallCacheUpdateMs, _("rows/s")),
            cacheInfo, _("Updates of all indexed columns with the minimal cache."),
            driveWhy);
        stats(r.back(), count(m.smallCacheUpdates, _("rows")), m.smallCacheUpdates,
            *m.smallCacheUpdateMs, _("rows/s"));
    }
    if (m.smallCacheDeleteMs && m.smallCacheDeletes > 0)
    {
        add(C::ServerStorage, server, _("Disk test: deleting"),
            formatRate(m.smallCacheDeletes, *m.smallCacheDeleteMs, _("rows/s")),
            cacheInfo, _("Deletes with the minimal cache."), driveWhy);
        stats(r.back(), count(m.smallCacheDeletes, _("rows")), m.smallCacheDeletes,
            *m.smallCacheDeleteMs, _("rows/s"));
    }
    if (m.parallelDriveMs && m.parallelDriveRows > 0)
    {
        add(C::ServerStorage, server,
            wxString::Format(_("Disk test, %d connections at once"),
                s.parallelConnections),
            formatRate(double(m.parallelDriveRows), *m.parallelDriveMs, _("rows/s")),
            cacheInfo, _("Inserts with the minimal cache on several connections."),
            driveWhy + " " + _("Several connections insert at the same time: "
                "shows whether the storage serves parallel writes."));
        stats(r.back(), count(double(m.parallelDriveRows), _("rows")),
            double(m.parallelDriveRows), *m.parallelDriveMs, _("rows/s"));
    }
    if (m.tempSortMs && m.tempSortBytes > 0)
    {
        auto limit = m.serverConfig.find("TempCacheLimit");
        add(C::ServerStorage, server, _("Sorting large data (temporary files)"),
            formatMegabytesPerSecond(double(m.tempSortBytes), *m.tempSortMs),
            limit == m.serverConfig.end() ? _("sort memory of the server unknown")
                : wxString::Format(_("sort memory (TempCacheLimit) %s"),
                    formatMegabytes(wxAtof(limit->second))),
            _("Sorting more data than fits into the sort memory."),
            _("Sorts several copies of the test rows inside the server. When the "
              "data exceeds the sort memory (TempCacheLimit), Firebird writes "
              "temporary files to TempDirectories; their drive, a virus scanner "
              "watching them and the memory setting decide the speed of large "
              "ORDER BY, GROUP BY, DISTINCT and index creation."));
        stats(r.back(), count(double(m.tempSortRows), _("rows")) + ", "
            + formatMegabytes(double(m.tempSortBytes)), double(m.tempSortRows),
            *m.tempSortMs, _("rows/s"));
    }

    // server RAM / cache
    if (m.coldScanMs && m.warmScanMs)
    {
        add(C::ServerCache, server, _("Reading all rows, first time"),
            formatMegabytesPerSecond(double(m.scanBytes), *m.coldScanMs),
            wxString::Format(_("%s scanned"), formatMegabytes(double(m.scanBytes))),
            _("Reading all rows right after a new connection."),
            _("Reads all rows right after a new connection: data comes from "
              "the Firebird page cache, the operating system file cache or "
              "the disk."));
        r.back().amount = formatMegabytes(double(m.scanBytes));
        r.back().duration = formatBenchmarkMilliseconds(*m.coldScanMs);
        r.back().durationMs = *m.coldScanMs;
        r.back().rate = formatMegabytesPerSecond(double(m.scanBytes), *m.coldScanMs);
        add(C::ServerCache, server, _("Reading all rows again (from memory)"),
            formatMegabytesPerSecond(double(m.scanBytes), *m.warmScanMs),
            wxString::Format(_("%s scanned"), formatMegabytes(double(m.scanBytes))),
            _("The same scan again, now from the cache."),
            _("The same scan immediately again: shows the speed when the data "
              "is cached."));
        r.back().amount = formatMegabytes(double(m.scanBytes));
        r.back().duration = formatBenchmarkMilliseconds(*m.warmScanMs);
        r.back().durationMs = *m.warmScanMs;
        r.back().rate = formatMegabytesPerSecond(double(m.scanBytes), *m.warmScanMs);
        if (*m.warmScanMs > 0.0)
        {
            add(C::ServerCache, server, _("First time compared with again"),
                formatFactor(*m.coldScanMs / *m.warmScanMs),
                _("the operating system file cache may already hold the data"),
                _("How much faster cached data is."),
                _("A high ratio means that uncached data is expensive; a ratio "
                  "near 1 means the data was cached already or caching does "
                  "not help."));
        }
    }
    if (m.serverReadMs && m.serverSortMs)
    {
        add(C::ServerCache, server, _("Sorting in memory"),
            formatMegabytesPerSecond(double(m.scanBytes), *m.serverSortMs),
            wxString::Format(_("measured inside the server; unsorted read: %s"),
                formatBenchmarkMilliseconds(*m.serverReadMs)),
            _("ORDER BY over wide rows inside the server."),
            _("Reads the test rows inside a stored procedure once unsorted and "
              "once with ORDER BY. The difference is the sort; it runs in the "
              "sort memory as long as the data fits into TempCacheLimit."));
        r.back().amount = formatMegabytes(double(m.scanBytes));
        r.back().duration = formatBenchmarkMilliseconds(*m.serverSortMs);
        r.back().durationMs = *m.serverSortMs;
        r.back().rate = formatMegabytesPerSecond(double(m.scanBytes), *m.serverSortMs);
    }

    // network
    if (m.latencyAvgMs)
    {
        wxString details = wxString::Format(_("median %s, 95%% %s, max %s"),
            formatBenchmarkMilliseconds(m.latencyMedianMs.value_or(0.0)),
            formatBenchmarkMilliseconds(m.latencyP95Ms.value_or(0.0)),
            formatBenchmarkMilliseconds(m.latencyMaxMs.value_or(0.0)));
        if (m.latencyOutliers > 0)
        {
            details += wxString::Format(_(", %d over %.0f ms"), m.latencyOutliers,
                std::max(150.0, 10.0 * m.latencyMedianMs.value_or(0.0)));
        }
        add(C::Network, network, _("Response time of a request (round trip)"),
            formatBenchmarkMilliseconds(m.latencyMedianMs.value_or(*m.latencyAvgMs)),
            details, _("Time of a query without any server work."),
            _("A query without server work, many times: the time is the pure "
              "round trip between FlameRobin and the server. Every statement "
              "of an application pays it at least once. Single very slow "
              "round trips over 150 ms are TCP retransmissions after lost "
              "packets."));
        stats(r.back(), count(s.latencyQueries, _("queries")), s.latencyQueries,
            *m.latencyAvgMs * s.latencyQueries, _("queries/s"));
    }
    if (m.latencySparseMs)
    {
        add(C::Network, network, _("Response time after short pauses"),
            formatBenchmarkMilliseconds(*m.latencySparseMs), _("median"),
            _("The same query with pauses, like a user working."),
            _("The same query with short pauses in between, like a user "
              "working in an application. Power saving of CPUs and network "
              "adapters makes these slower than back-to-back queries."));
        r.back().amount = count(s.sparseQueries, _("queries"));
    }
    if (m.largeRoundTripMs)
    {
        add(C::Network, network, _("Response time with a larger answer (8 KB)"),
            formatBenchmarkMilliseconds(*m.largeRoundTripMs), _("median"),
            _("A query whose answer needs several network packets."),
            _("A larger answer needs several network packets; compared with "
              "the small round trip it shows bandwidth and packet problems "
              "(e.g. a too large MTU on a VPN)."));
    }
    if (m.streamMs)
    {
        add(C::Network, network, _("Transferring a large result"),
            formatMegabytesPerSecond(double(m.streamBytes), *m.streamMs),
            wxString::Format(_("%s rows, %s"), formatBenchmarkCount(double(m.streamRows)),
                formatMegabytes(double(m.streamBytes))),
            _("Transfer of many wide rows to FlameRobin."),
            _("Fetches many wide rows to FlameRobin: throughput of the whole "
              "path from server over network to client."));
        stats(r.back(), count(double(m.streamRows), _("rows")), double(m.streamRows),
            *m.streamMs, _("rows/s"));
        if (m.serverReadMs)
        {
            double transfer = *m.streamMs - *m.serverReadMs
                - m.streamClientMs.value_or(0.0);
            if (transfer < 0.0)
                transfer = 0.0;
            add(C::Network, both, _("Time split: server / transfer / FlameRobin"),
                formatBenchmarkMilliseconds(*m.serverReadMs) + " / "
                    + formatBenchmarkMilliseconds(transfer) + " / "
                    + formatBenchmarkMilliseconds(m.streamClientMs.value_or(0.0)),
                _("transfer includes the network and the client library"),
                _("Where the time of a large result set goes."),
                _("The same rows are also read inside the server; the "
                  "difference to the streaming time is spent on the network "
                  "and in the client. This separates server problems from "
                  "network and client problems."));
        }
    }
    if (m.wideInsertMs && m.wideInsertRows > 0)
    {
        add(C::Network, network, wxString::Format(_("Small writes, one request each "
            "(%d columns)"), BenchmarkSql::wideColumns),
            count(m.wideInsertRows * 1000.0 / *m.wideInsertMs, _("writes/s")),
            _("one round trip per row"),
            _("Many single rows with 100 values, each sent to the server on its own."),
            _("Inserts rows into a table with 100 columns, each with its own "
              "statement execution, so every row is one round trip with 100 "
              "parameters; it is what data entry and import applications see."));
        stats(r.back(), count(m.wideInsertRows, _("rows")), m.wideInsertRows,
            *m.wideInsertMs, _("writes/s"));
    }

    // application patterns
    if (m.rowByRowMs && m.batchMs)
    {
        add(C::Application, both, _("Rows one by one compared with in one go"),
            formatBenchmarkMilliseconds(*m.rowByRowMs) + " / "
                + formatBenchmarkMilliseconds(*m.batchMs),
            wxString::Format(_("%s vs. %s"),
                formatRate(s.rowByRowRows, *m.rowByRowMs, _("rows/s")),
                formatRate(s.rowByRowRows, *m.batchMs, _("rows/s"))),
            _("The cost of one round trip per row."),
            _("The same rows inserted with one statement execution per row "
              "from FlameRobin and with one call of a stored procedure. The "
              "difference is what round trips cost an application; batches "
              "(EXECUTE BLOCK, stored procedures, Batch API of Firebird 4+) "
              "avoid it."));
        stats(r.back(), count(s.rowByRowRows, _("rows")), s.rowByRowRows,
            *m.rowByRowMs, _("rows/s"));
    }
    if (m.prepareMs)
    {
        add(C::Application, server, _("Preparing a typical query"),
            formatBenchmarkMilliseconds(*m.prepareMs), _("per prepare"),
            _("Compiling a query on the server."),
            _("Compiles a query with a subquery, ORDER BY and ROWS again and "
              "again. Applications that prepare the same statement for every "
              "execution pay this every time; Firebird 5 caches compiled "
              "statements (MaxStatementCacheSize)."));
        stats(r.back(), count(s.prepareCount, _("prepares")), s.prepareCount,
            *m.prepareMs * s.prepareCount, _("prepares/s"));
    }
    if (m.blobWriteMs && m.blobReadMs && m.blobBytes > 0)
    {
        add(C::Application, both, _("Writing / reading large data (BLOBs)"),
            formatMegabytesPerSecond(double(m.blobBytes), *m.blobWriteMs) + " / "
                + formatMegabytesPerSecond(double(m.blobBytes), *m.blobReadMs),
            wxString::Format(_("%d BLOBs of 64 KB"), s.blobCount),
            _("Writing and reading BLOBs."),
            _("BLOBs are stored on separate pages and transferred in "
              "segments with extra round trips. Applications that load BLOBs "
              "in lists are often slow because of this."));
        r.back().amount = count(s.blobCount, _("BLOBs")) + ", "
            + formatMegabytes(double(m.blobBytes));
        r.back().duration = formatBenchmarkMilliseconds(*m.blobWriteMs) + " / "
            + formatBenchmarkMilliseconds(*m.blobReadMs);
    }
    if (m.versionReadBeforeMs && m.versionReadWithOldTxMs)
    {
        add(C::Application, server, _("Reading while an old transaction is open"),
            formatBenchmarkMilliseconds(*m.versionReadBeforeMs) + " -> "
                + formatBenchmarkMilliseconds(*m.versionReadWithOldTxMs),
            wxString::Format(_("first read after the old transaction ended: %s"),
                formatBenchmarkMilliseconds(m.versionReadAfterCommitMs.value_or(0.0))),
            _("What a long open transaction costs other readers."),
            _("Updates the same rows many times while an old snapshot "
              "transaction stays open. Firebird keeps the old record versions "
              "as long as a transaction might need them, so readers walk "
              "through them and garbage collection is blocked for everybody. "
              "When the old transaction ends, the next reader removes the "
              "versions and pays for it."));
        r.back().amount = count(s.versionRounds, _("updates"));
    }
    for (const auto& o : m.oltp)
    {
        add(C::MultiUser, both,
            wxString::Format(_("Transactions with %d users at once"), o.clients),
            count(o.seconds > 0.0 ? o.transactions / o.seconds : 0.0, _("tx/s")),
            wxString::Format(_("95%% %s, %lld conflicts"),
                formatBenchmarkMilliseconds(o.p95Ms), (long long)o.conflicts),
            _("Mixed transactions of several users at the same time."),
            _("Each transaction runs 5 primary key lookups, 1 range of 100 "
              "rows by primary key (both as procedure calls, one round trip "
              "each), 1 update and 1 insert, then commits: a typical read and "
              "write workload. Increasing the number of users shows how the "
              "throughput scales and where contention starts; 50 and 250 users "
              "show whether the server keeps up with many users."));
        BenchmarkResult& result = r.back();
        stats(result, count(double(o.transactions), _("transactions")),
            double(o.transactions), o.seconds * 1000.0, _("tx/s"));
        result.average = formatBenchmarkAverage(o.avgMs);
    }

    for (const auto& p : m.pageSizes)
    {
        add(C::ServerStorage, server,
            wxString::Format(_("Page size %d"), p.pageSize),
            wxString::Format(_("insert %s, scan %s"),
                formatBenchmarkMilliseconds(p.insertMs),
                formatBenchmarkMilliseconds(p.warmScanMs)),
            wxString::Format(_("commit %s, first scan %s, insert with "
                "minimal cache %s, page cache %d pages (%s)"),
                formatBenchmarkMilliseconds(p.commitMs),
                formatBenchmarkMilliseconds(p.coldScanMs),
                formatRate(s.diskRows, p.smallCacheInsertMs, _("rows/s")),
                p.cachePages, formatMegabytes(double(p.cachePages) * p.pageSize)),
            _("The storage tests repeated with another page size."),
            _("The storage and cache tests repeated in a separate temporary "
              "database for every page size the server supports. Larger "
              "pages make scans and index lookups cheaper but every changed "
              "page larger to write; the comparison shows which page size "
              "suits this storage."));
    }
    // the results the key values are calculated from
    const std::tuple<C, wxString, const char*> sources[] = {
        { C::ServerCpu, _("Calculation, one connection"), "cpuIterationsPerMs" },
        { C::ServerCpu, wxString::Format(_("Calculation, %d connections at once"),
            s.parallelConnections), "parallelIterationsPerMs" },
        { C::ServerStorage, _("Saving a small change (commit)"), "commitMs" },
        { C::ServerStorage, _("Writing rows (table with 5 indexes)"), "insertRowsPerSecond" },
        { C::ServerStorage, _("Disk test with a small cache"), "driveOperationsPerSecond" },
        { C::ServerStorage, wxString::Format(_("Disk test, %d connections at once"),
            s.parallelConnections), "parallelDriveRowsPerSecond" },
        { C::ServerStorage, _("Sorting large data (temporary files)"), "tempSortMBPerSecond" },
        { C::ServerCache, _("Reading all rows again (from memory)"), "scanMBPerSecond" },
        { C::ServerCache, _("Sorting in memory"), "sortMBPerSecond" },
        { C::Network, _("Response time of a request (round trip)"), "roundTripMs" },
        { C::Network, _("Transferring a large result"), "transferMBPerSecond" },
        { C::Connection, _("Connecting with login"), "connectMs" },
        { C::Network, wxString::Format(_("Small writes, one request each (%d columns)"),
            BenchmarkSql::wideColumns), "wideInsertsPerSecond" },
        { C::LocalMachine, _("Short calculation after a pause / without a pause"),
            "clientCpuBurstMs" }
    };
    for (auto& result : r)
    {
        for (const auto& source : sources)
        {
            if (result.category == std::get<0>(source) && result.test == std::get<1>(source))
                result.valueKey = std::get<2>(source);
        }
    }

    for (const auto& skipped : m.skippedTests)
    {
        add(C::Application, both, skipped.first, _("not measured"),
            skipped.second, _("This optional test could not run."),
            _("This optional test could not run with this server or client "
              "library; the other results are not affected."));
    }

    // engine statistics
    if (m.pageReads)
    {
        const wxString hint = _("Page counters of the benchmark connections.");
        const wxString why = _("Counters of the benchmark connections from "
            "MON$IO_STATS: reads/writes are physical page I/O, fetches/marks "
            "are page accesses in the cache.");
        add(C::Engine, server, _("Page reads / writes"),
            wxString::Format("%s / %s", formatBenchmarkCount(double(*m.pageReads)),
                formatBenchmarkCount(double(m.pageWrites.value_or(0)))),
            wxEmptyString, hint, why);
        add(C::Engine, server, _("Page fetches / marks"),
            wxString::Format("%s / %s",
                formatBenchmarkCount(double(m.pageFetches.value_or(0))),
                formatBenchmarkCount(double(m.pageMarks.value_or(0)))),
            wxEmptyString, hint, why);
    }
    if (m.recordPurges)
    {
        add(C::Engine, server, _("Back-outs / purges / expunges"),
            wxString::Format("%s / %s / %s",
                formatBenchmarkCount(double(m.recordBackouts.value_or(0))),
                formatBenchmarkCount(double(*m.recordPurges)),
                formatBenchmarkCount(double(m.recordExpunges.value_or(0)))),
            wxEmptyString, _("Garbage collection work of the benchmark."),
            _("Garbage collection work from MON$RECORD_STATS: back-outs undo "
              "changes, purges and expunges remove old record versions and "
              "deleted rows."));
    }
    return r;
}

wxString BenchmarkReport::toMarkdown() const
{
    // The results only, compact, so that runs can be put side by side;
    // the explanations are in the HTML report. The snapshot at the end
    // lets FlameRobin compare this report with other runs later.
    const BenchmarkMetrics& m = metrics;
    const BenchmarkSnapshot snapshot = makeBenchmarkSnapshot(*this);
    auto cell = [](const wxString& text)
    {
        return escapeMarkdownTableCell(text);
    };

    wxString md;
    md << "## " << _("Performance Benchmark & Diagnosis") << ": "
       << getBenchmarkSystemTitle(*this) << "\n\n";
    md << "| | |\n|---|---|\n";
    if (!m.localHostName.empty())
        md << "| " << _("Computer") << " | " << cell(m.localHostName) << " |\n";
    md << "| " << _("Date") << " | " << startedAt << " |\n";
    md << "| " << _("Test") << " | " << cell(getBenchmarkKindName(getBenchmarkKindId(m)))
       << ", " << intensityName << ", " << wxString::Format(_("%.0f s"), durationSeconds)
       << " |\n\n";
    if (anonymous)
    {
        md << "> " << _("Computer and host names, IP addresses, user names, paths and "
            "service names have been removed from this report.") << "\n\n";
    }
    if (cancelled)
        md << "> " << _("The run was cancelled; the results are incomplete.") << "\n\n";
    else if (!errorMessage.empty())
    {
        md << "> " << _("The run failed; the results are incomplete.") << " "
           << errorMessage << "\n\n";
    }
    if (benchmarkDatabaseCreated && !benchmarkDatabaseDropped)
    {
        md << "> " << wxString::Format(_("The temporary database %s could not be "
            "dropped."), benchmarkDatabasePath) << "\n\n";
    }

    if (earlier)
    {
        BenchmarkComparison comparison = compareBenchmarkSnapshots(snapshot, *earlier);
        comparison.newerIsCurrentRun = true;
        md << formatBenchmarkComparisonMarkdown(comparison, false) << "\n";
    }
    else
    {
        // the key values by part, with the work behind them
        md << "### " << _("Key values") << "\n\n| " << _("Part") << " | " << _("Value")
           << " | " << _("Result") << " | " << _("Work") << " |\n|---|---|---:|---|\n";
        for (const auto& part : parts)
        {
            for (const auto& v : part.values)
            {
                md << "| " << cell(part.name) << " | " << cell(v.title) << " | "
                   << cell(v.valueText) << " | "
                   << cell(formatBenchmarkWork(v.amountText, v.durationMs)) << " |\n";
            }
        }
    }

    // hardware, software and the settings that differ from the defaults
    wxString system, settings;
    for (const auto& f : getBenchmarkFacts(*this))
    {
        if (!f.key.StartsWith("conf."))
            system << "| " << cell(f.label) << " | " << cell(f.value) << " |\n";
        else if (m.serverConfigExplicit.count(f.key.Mid(5)))
            settings << "| " << cell(f.key.Mid(5)) << " | " << cell(f.value) << " |\n";
    }
    md << "\n### " << _("System") << "\n\n| | |\n|---|---|\n" << system;
    if (!settings.empty())
    {
        md << "\n### " << _("Settings in firebird.conf") << "\n\n| " << _("Setting")
           << " | " << _("Value") << " |\n|---|---|\n" << settings;
    }

    md << "\n### " << _("All measured values") << "\n\n| " << _("Part") << " | "
       << _("Test") << " | " << _("Result") << " | " << _("Amount") << " | " << _("Time")
       << " | " << _("Average") << " | " << _("Rate") << " |\n|---|---|---:|---:|---:|---:|---:|\n";
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        for (const auto& r : results)
        {
            if (getBenchmarkReportSection(r.category) != section
                || (r.amount.empty() && r.duration.empty() && r.rate.empty()
                    && section == BenchmarkCategory::Environment))
            {
                continue;
            }
            md << "| " << cell(getBenchmarkReportSectionName(section)) << " | "
               << cell(r.test) << " | " << cell(r.value) << " | " << cell(r.amount) << " | "
               << cell(r.duration) << " | " << cell(r.average) << " | " << cell(r.rate)
               << " |\n";
        }
    }
    if (!m.skippedTests.empty())
    {
        md << "\n" << _("Not measured:") << "\n\n";
        for (const auto& skipped : m.skippedTests)
            md << "- " << skipped.first << ": " << skipped.second << "\n";
    }
    md << "\n" << formatBenchmarkSnapshot(snapshot);
    return md;
}

wxString getBenchmarkIntensityName(BenchmarkIntensity intensity)
{
    switch (intensity)
    {
        case BenchmarkIntensity::Quick:
            return _("Quick");
        case BenchmarkIntensity::Deep:
            return _("Deep");
        case BenchmarkIntensity::Standard:
        default:
            return _("Standard");
    }
}

wxString getBenchmarkCategoryName(BenchmarkCategory category)
{
    switch (category)
    {
        case BenchmarkCategory::Environment:
            return _("Environment");
        case BenchmarkCategory::Connection:
            return _("Connection");
        case BenchmarkCategory::LocalMachine:
            return _("This computer");
        case BenchmarkCategory::ServerCpu:
            return _("Server processor");
        case BenchmarkCategory::ServerStorage:
            return _("Server disk");
        case BenchmarkCategory::ServerCache:
            return _("Server memory");
        case BenchmarkCategory::Network:
            return _("Network");
        case BenchmarkCategory::MultiUser:
            return _("Several users at once");
        case BenchmarkCategory::Application:
            return _("Programming patterns");
        case BenchmarkCategory::Engine:
        default:
            return _("Engine statistics");
    }
}

std::vector<BenchmarkCategory> getBenchmarkReportSections()
{
    using C = BenchmarkCategory;
    return { C::ServerCpu, C::ServerStorage, C::ServerCache, C::Network,
        C::MultiUser, C::LocalMachine, C::Application, C::Environment };
}

BenchmarkCategory getBenchmarkReportSection(BenchmarkCategory category)
{
    switch (category)
    {
        case BenchmarkCategory::Connection:
            return BenchmarkCategory::Network;
        case BenchmarkCategory::Engine:
            return BenchmarkCategory::Environment;
        default:
            return category;
    }
}

wxString getBenchmarkReportSectionName(BenchmarkCategory section)
{
    switch (section)
    {
        case BenchmarkCategory::Environment:
            return _("Settings");
        default:
            return getBenchmarkCategoryName(section);
    }
}

wxString getBenchmarkConnectionKindName(BenchmarkConnectionKind kind)
{
    switch (kind)
    {
        case BenchmarkConnectionKind::Embedded:
            return _("embedded");
        case BenchmarkConnectionKind::LocalXnet:
            return _("local XNET");
        case BenchmarkConnectionKind::TcpLoopback:
            return _("TCP loopback");
        case BenchmarkConnectionKind::TcpRemote:
            return _("TCP/IP network");
        case BenchmarkConnectionKind::Unknown:
        default:
            return _("unknown");
    }
}

wxString getBenchmarkRunLocationName(BenchmarkRunLocation location)
{
    switch (location)
    {
        case BenchmarkRunLocation::Server:
            return _("the database server");
        case BenchmarkRunLocation::Client:
            return _("a client machine");
        case BenchmarkRunLocation::Unknown:
        default:
            return _("unknown machine");
    }
}

BenchmarkConnectionKind classifyBenchmarkConnection(const wxString& protocol,
    const wxString& address)
{
    wxString p(protocol.Upper().Trim());
    if (p.empty())
        return BenchmarkConnectionKind::Embedded;
    if (p.StartsWith("XNET"))
        return BenchmarkConnectionKind::LocalXnet;
    if (!p.StartsWith("TCP"))
        return BenchmarkConnectionKind::Unknown;

    // MON$REMOTE_ADDRESS is "address/port" in Firebird 3+, plain address before
    wxString a(address.Lower().Trim());
    a = a.BeforeLast('/').empty() ? a : a.BeforeLast('/');
    if (a == "::1" || a.StartsWith("127.") || a == "::ffff:127.0.0.1")
        return BenchmarkConnectionKind::TcpLoopback;
    return BenchmarkConnectionKind::TcpRemote;
}

BenchmarkRunLocation determineBenchmarkRunLocation(BenchmarkConnectionKind kind,
    const wxString& connectionHost, const wxString& localHostName,
    bool databaseFileIsLocal)
{
    switch (kind)
    {
        case BenchmarkConnectionKind::Embedded:
        case BenchmarkConnectionKind::LocalXnet:
        case BenchmarkConnectionKind::TcpLoopback:
            return BenchmarkRunLocation::Server;
        case BenchmarkConnectionKind::TcpRemote:
        {
            // connected to the own machine through a network interface
            wxString host(connectionHost.Lower());
            wxString local(localHostName.Lower());
            if (!local.empty() && (host == local || host.StartsWith(local + ".")))
                return BenchmarkRunLocation::Server;
            if (databaseFileIsLocal)
                return BenchmarkRunLocation::Server;
            return BenchmarkRunLocation::Client;
        }
        case BenchmarkConnectionKind::Unknown:
        default:
            return BenchmarkRunLocation::Unknown;
    }
}

bool prepareBenchmarkEventTest(BenchmarkMetrics& m)
{
    // A remote client receives events through a second connection to
    // RemoteAuxPort of the server. When a firewall drops it, the client
    // waits for the TCP timeout of the operating system (up to minutes), so
    // the test only runs where it cannot hang.
    if (m.connectionKind != BenchmarkConnectionKind::TcpRemote)
        return true;
    auto aux = m.serverConfig.find("RemoteAuxPort");
    long port = 0;
    if (aux == m.serverConfig.end() || !aux->second.ToLong(&port))
    {
        m.eventResult = BenchmarkEventResult::NotTestedUnknownPort;
        // RDB$CONFIG exists from Firebird 4 on
        if (m.serverMajorVersion > 0 && m.serverMajorVersion < 4)
        {
            m.eventStatus = _("not tested: RemoteAuxPort is unknown, because "
                "Firebird 3 cannot report its configuration");
        }
        else
        {
            m.eventStatus = _("not tested: RemoteAuxPort is unknown, because "
                "only administrators can read the configuration");
        }
        return false;
    }
    if (port == 0)
    {
        m.eventResult = BenchmarkEventResult::NotTestedRandomPort;
        m.eventStatus = _("not tested: RemoteAuxPort is 0, so events use a "
            "random port that firewalls usually block");
        return false;
    }
    auto check = std::find_if(m.serverPorts.begin(), m.serverPorts.end(),
        [port](const BenchmarkPortCheck& c) { return c.port == port; });
    if (check != m.serverPorts.end()
        && check->state == BenchmarkPortCheck::State::Filtered)
    {
        m.eventResult = BenchmarkEventResult::PortFiltered;
        m.eventStatus = wxString::Format(_("not received: RemoteAuxPort %ld "
            "does not answer, a firewall drops the connection"), port);
        return false;
    }
    return true;
}

std::vector<wxString> parseBenchmarkVersionList(const std::string& infoVersion)
{
    const std::string& s = infoVersion;
    std::vector<wxString> list;
    // a counted list: number of strings, then length and text of each
    if (s.size() >= 2 && static_cast<unsigned char>(s[0]) < ' ')
    {
        const size_t count = static_cast<unsigned char>(s[0]);
        size_t pos = 1;
        while (list.size() < count && pos < s.size())
        {
            size_t length = static_cast<unsigned char>(s[pos]);
            list.push_back(wxString::FromUTF8(s.substr(pos + 1, length).c_str()).Trim());
            pos += 1 + length;
        }
        return list;
    }
    list.push_back(wxString::FromUTF8(s.c_str()).Trim());
    return list;
}

wxString getBenchmarkServerHostName(const std::vector<wxString>& versionList)
{
    // the remote layer reports the server: "...Firebird 5.0/tcp (dbserver)/P19:C"
    for (const wxString& entry : versionList)
    {
        int open = entry.Find(" (");
        int close = entry.Find(')', true);
        if (entry.Contains("/tcp") && open != wxNOT_FOUND && close > open + 2)
            return entry.Mid(open + 2, close - open - 2);
    }
    return wxEmptyString;
}

wxString getServerPlatformName(const wxString& implementation)
{
    // the version starts with a platform code, e.g. "WI-V5.0.4.1812": the
    // first letter is the operating system (W = Windows, L = Linux,
    // U = macOS, F = FreeBSD), the second one the CPU architecture
    int pos = implementation.Find("-V");
    if (pos < 2)
        return wxEmptyString;
    wxString code(implementation.Mid(pos - 2, 2).Upper());
    switch (char(code[0]))
    {
        case 'W':
            return "Windows";
        case 'L':
            return "Linux";
        case 'U':
            return "macOS";
        case 'F':
            return "FreeBSD";
        default:
            return wxEmptyString;
    }
}

wxString formatBenchmarkMilliseconds(double milliseconds)
{
    if (milliseconds >= 10000.0)
        return wxString::Format(_("%.1f s"), milliseconds / 1000.0);
    if (milliseconds >= 100.0)
        return wxString::Format(_("%.0f ms"), milliseconds);
    // embedded round trips take a few microseconds
    if (milliseconds > 0.0 && milliseconds < 0.01)
        return wxString::Format(_("%.4f ms"), milliseconds);
    return wxString::Format(_("%.2f ms"), milliseconds);
}

wxString escapeMarkdownTableCell(const wxString& text)
{
    wxString s(text);
    s.Replace("\r\n", " ");
    s.Replace("\n", " ");
    s.Replace("|", "\\|");
    return s;
}

std::vector<int> getSupportedPageSizes(int serverMajorVersion)
{
    if (serverMajorVersion > 0 && serverMajorVersion < 4)
        return { 4096, 8192, 16384 };
    return { 4096, 8192, 16384, 32768 };
}

double getBenchmarkPercentile(std::vector<double> values, double p)
{
    if (values.empty())
        return 0.0;
    // the value at the rounded position p * (n - 1) of the sorted values;
    // nth_element is enough, the values need not be sorted completely
    p = std::clamp(p, 0.0, 1.0);
    auto nth = values.begin()
        + std::ptrdiff_t(p * double(values.size() - 1) + 0.5);
    std::nth_element(values.begin(), nth, values.end());
    return *nth;
}

double getBenchmarkAverage(const std::vector<double>& values)
{
    if (values.empty())
        return 0.0;
    return std::accumulate(values.begin(), values.end(), 0.0) / double(values.size());
}

wxString getBenchmarkDirectoryFromPath(const wxString& databasePath)
{
    size_t pos = databasePath.find_last_of("/\\");
    if (pos == wxString::npos)
        return wxEmptyString;  // an alias, the directory is unknown
    return databasePath.Left(pos + 1);
}

wxString makeBenchmarkDatabasePath(const wxString& directory,
    const wxString& uniqueName)
{
    wxString dir(directory);
    dir.Trim().Trim(false);
    if (dir.empty())
        return uniqueName;
    if (!dir.EndsWith("/") && !dir.EndsWith("\\"))
    {
        // keep the separator style of the server-side path
        bool backslash = dir.Find('\\') != wxNOT_FOUND && dir.Find('/') == wxNOT_FOUND;
        dir += backslash ? "\\" : "/";
    }
    return dir + uniqueName;
}

namespace
{

wxString escapePendingField(const wxString& s)
{
    wxString r(s);
    r.Replace("%", "%25");
    r.Replace(",", "%2C");
    r.Replace("|", "%7C");
    return r;
}

wxString unescapePendingField(const wxString& s)
{
    // every '%' in an escaped field starts an escape sequence, so the
    // order of the replacements cannot create new ones
    wxString r(s);
    r.Replace("%7C", "|");
    r.Replace("%2C", ",");
    r.Replace("%25", "%");
    return r;
}

} // namespace

wxString BenchmarkPendingDatabase::toString() const
{
    return wxString(embedded ? "E" : "S") + "|" + escapePendingField(registrationId)
        + "|" + escapePendingField(clientLibrary)
        + "|" + escapePendingField(connectionString);
}

std::optional<BenchmarkPendingDatabase> BenchmarkPendingDatabase::parse(
    const wxString& entry)
{
    wxArrayString parts = wxSplit(entry, '|', '\0');
    if (parts.size() != 4 || (parts[0] != "E" && parts[0] != "S"))
        return std::nullopt;
    BenchmarkPendingDatabase result;
    result.embedded = parts[0] == "E";
    result.registrationId = unescapePendingField(parts[1]);
    result.clientLibrary = unescapePendingField(parts[2]);
    result.connectionString = unescapePendingField(parts[3]);
    if (result.connectionString.empty())
        return std::nullopt;
    return result;
}

bool isBenchmarkDatabaseName(const wxString& fileName)
{
    const wxString prefix = "FR_BENCHMARK_";
    const wxString suffix = ".FDB";
    wxString name(fileName.Upper());
    if (!name.StartsWith(prefix) || !name.EndsWith(suffix))
        return false;
    name = name.Mid(prefix.length(),
        name.length() - prefix.length() - suffix.length());

    auto isNumber = [](const wxString& s, size_t length, bool hex)
    {
        if (s.empty() || (length > 0 && s.length() != length))
            return false;
        return std::all_of(s.begin(), s.end(), [hex](wxUniChar c)
        {
            return (c >= '0' && c <= '9') || (hex && c >= 'A' && c <= 'F');
        });
    };
    // <yyyymmdd>_<hhmmss>_<6 hex digits>[_P<page size>]
    wxArrayString parts = wxSplit(name, '_', '\0');
    if (parts.size() != 3 && parts.size() != 4)
        return false;
    if (!isNumber(parts[0], 8, false) || !isNumber(parts[1], 6, false)
        || !isNumber(parts[2], 6, true))
    {
        return false;
    }
    return parts.size() == 3
        || (parts[3].StartsWith("P") && isNumber(parts[3].Mid(1), 0, false));
}

bool isBenchmarkDatabaseFile(const wxString& attachedFileName,
    const wxString& uniqueName)
{
    if (!isBenchmarkDatabaseName(uniqueName))
        return false;
    wxString file(attachedFileName.Upper());
    wxString name(uniqueName.Upper());
    if (!file.EndsWith(name))
        return false;
    // the unique name has to be the complete file name, not just its end
    if (file.length() == name.length())
        return true;
    wxChar before = file[file.length() - name.length() - 1];
    return before == '/' || before == '\\' || before == ':';
}

} // namespace fr
