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

// Rule based interpretation of the benchmark metrics. The thresholds are
// heuristics: they were chosen well above the values measured on a current
// desktop machine with NVMe storage (Firebird 5.0, SuperServer), so that a
// warning means "clearly slower than a healthy system", not "slower than
// the fastest hardware". The configuration values follow the Firebird
// documentation and the comments in firebird.conf.

namespace fr
{

namespace
{

using Sev = BenchmarkSeverity;
using Side = BenchmarkSide;
using Who = BenchmarkAudience;
namespace Limits = BenchmarkLimits;

const double megabyte = 1024.0 * 1024.0;
const double gigabyte = 1024.0 * megabyte;

class Analyzer
{
public:
    explicit Analyzer(const BenchmarkMetrics& metrics)
        : m(metrics), s(metrics.settings)
    {
    }

    std::vector<BenchmarkFinding> run()
    {
        checkRunLocation();
        checkVersion();
        checkConnect();
        checkPowerManagement();
        checkLatency();
        checkThroughput();
        checkCpu();
        checkCommit();
        checkDiskWrites();
        checkSmallCache();
        checkLocalFiles();
        checkCache();
        checkSort();
        checkDatabaseState();
        checkClientLibrary();
        checkRoundTrips();
        checkPrepare();
        checkBlob();
        checkVersions();
        checkOltp();
        checkPageSizes();
        checkSystem();
        checkNetworkPath();
        checkPorts();
        checkEvents();
        checkTimeouts();
        checkKeepAlive();
        checkWideInserts();
        checkTempSort();
        checkManyUsers();

        if (std::none_of(findings.begin(), findings.end(),
            [](const BenchmarkFinding& f)
            {
                return f.severity == Sev::Warning || f.severity == Sev::Problem;
            }))
        {
            add(Sev::Ok, Side::Configuration, Who::Administrator,
                _("No bottleneck found"),
                _("All measured values are within the expected range."),
                wxEmptyString);
        }

        std::stable_sort(findings.begin(), findings.end(),
            [](const BenchmarkFinding& a, const BenchmarkFinding& b)
            {
                return int(a.severity) > int(b.severity);
            });
        return findings;
    }

private:
    const BenchmarkMetrics& m;
    const BenchmarkSettings& s;
    std::vector<BenchmarkFinding> findings;
    // the category of the findings added next; set by every check
    BenchmarkCategory categoryM = BenchmarkCategory::Environment;

    bool onServer() const
    {
        return m.runLocation == BenchmarkRunLocation::Server;
    }

    bool remote() const
    {
        return m.connectionKind == BenchmarkConnectionKind::TcpRemote
            && !onServer();
    }

    wxString configValue(const wxString& name) const
    {
        auto it = m.serverConfig.find(name);
        return it == m.serverConfig.end() ? wxString() : it->second;
    }

    void add(Sev severity, Side side, Who audience, const wxString& title,
        const wxString& detail, const wxString& recommendation)
    {
        findings.push_back({ severity, side, audience, title, detail,
            recommendation, categoryM });
    }

    static wxString ms(double value)
    {
        return formatBenchmarkMilliseconds(value);
    }

    // the operating system of the machine a finding is about
    wxString osOf(Side side) const
    {
        if (side == Side::Server)
            return m.serverPlatform;
        if (side == Side::Client)
            return m.clientOsFamily;
        return wxEmptyString;
    }

    wxString antivirusAdvice(const wxString& os) const
    {
        wxString common = _("Exclude the database files (*.fdb, nbackup "
            "delta files), the Firebird temporary directory and the Firebird "
            "server process from on-access virus scanning.");
        if (os == "Windows")
        {
            return common + " " + _("On Windows add the exclusions in "
                "Microsoft Defender (or the installed security product) for "
                "the folders, the file types and firebird.exe, and exclude "
                "them from \"Controlled folder access\" / ransomware "
                "protection.");
        }
        if (os == "Linux")
        {
            return common + " " + _("On Linux check on-access scanners and "
                "endpoint agents (e.g. ClamAV OnAccess, EDR agents) for the "
                "database and temporary directories.");
        }
        return common;
    }

    wxString powerAdvice(const wxString& os) const
    {
        wxString advice;
        if (os == "Windows")
        {
            advice = _("Windows: select the power plan \"High performance\" "
                "(powercfg /setactive SCHEME_MIN).");
        }
        else if (os == "Linux")
        {
            advice = _("Linux: set the CPU frequency governor to "
                "\"performance\" (cpupower frequency-set -g performance) or "
                "use a throughput oriented tuned profile.");
        }
        else if (os == "macOS")
        {
            advice = _("macOS: disable low power mode and run on mains "
                "power.");
        }
        if (!advice.empty())
            advice += " ";
        return advice + _("Set the BIOS/UEFI power profile to maximum "
            "performance, and on a virtual machine also the power policy of "
            "its host.");
    }

    // advice that FlameRobin cannot verify: it may be set already
    static wxString unverified(const wxString& advice)
    {
        return _("Tip, FlameRobin cannot check the current setting:") + " " + advice;
    }

    // the power settings of the machine running FlameRobin: empty if
    // unknown, true if they are already set to performance
    std::optional<bool> powerAlreadyMaximum() const
    {
        if (m.system.powerPlan.empty())
            return std::nullopt;
        const wxString plan = wxString::FromUTF8(m.system.powerPlan.c_str()).Lower();
        return plan.Contains("high performance") || plan.Contains("ultimate performance")
            || plan.Contains("best performance") || plan.Contains("governor performance");
    }

    // power advice for the machine running FlameRobin: nothing to change if
    // it is set already, otherwise the change and the current setting
    wxString localPowerAdvice(const wxString& os) const
    {
        const std::optional<bool> maximum = powerAlreadyMaximum();
        const wxString plan = wxString::FromUTF8(m.system.powerPlan.c_str());
        if (maximum && *maximum)
        {
            return wxString::Format(_("The power settings are already set to "
                "performance (%s), so the delay comes from elsewhere: C-states or "
                "energy settings in the BIOS/UEFI, the power policy of a "
                "virtualization host, or other programs that load the CPU."), plan);
        }
        if (maximum)
            return wxString::Format(_("Current setting: %s."), plan) + " " + powerAdvice(os);
        return unverified(powerAdvice(os));
    }

    // the virus scanners of this machine that are switched on
    std::vector<wxString> activeScanners() const
    {
        std::vector<wxString> list;
        for (const auto& a : m.system.antivirus)
        {
            wxString name = wxString::FromUTF8(a.c_str());
            if (!name.Contains("(off"))
                list.push_back(name.BeforeFirst('(').Trim());
        }
        return list;
    }

    // whether the list of virus scanners is known (Windows Security
    // Center, process list on Linux)
    bool scannersKnown() const
    {
        return !m.system.antivirus.empty()
            || m.system.antivirusNote == "no known on-access scanner running";
    }

    static wxString join(const std::vector<wxString>& list)
    {
        wxString text;
        for (const auto& item : list)
            text += (text.empty() ? "" : ", ") + item;
        return text;
    }

    wxString serverMode() const
    {
        wxString mode = configValue("ServerMode");
        if (!mode.empty())
            return mode;
        if (m.serverProcesses && *m.serverProcesses > 1)
            return "Classic";
        return wxEmptyString;
    }

    bool classicMode() const
    {
        wxString mode = serverMode();
        return mode.CmpNoCase("Classic") == 0
            || mode.CmpNoCase("SuperClassic") == 0;
    }

    void checkRunLocation()
    {
        categoryM = BenchmarkCategory::Environment;
        switch (m.runLocation)
        {
            case BenchmarkRunLocation::Server:
                if (m.connectionKind == BenchmarkConnectionKind::TcpRemote)
                {
                    add(Sev::Info, Side::Configuration, Who::Administrator,
                        _("The test ran on the server, but over a network connection"),
                        _("The connection uses the network stack although "
                          "client and server are the same machine."),
                        _("Connect local tools via localhost (or XNET on "
                          "Windows). Run the benchmark from a typical client "
                          "workstation as well to measure the network."));
                }
                else
                {
                    add(Sev::Info, Side::Configuration, Who::Administrator,
                        _("The test ran on the database server itself"),
                        wxString::Format(_("Connection: %s. The results "
                            "show the server without the network."),
                            getBenchmarkConnectionKindName(m.connectionKind)),
                        _("Run the benchmark from a typical client "
                          "workstation as well. If the client run is much "
                          "slower, the network or the client is the "
                          "bottleneck."));
                }
                break;
            case BenchmarkRunLocation::Client:
                add(Sev::Info, Side::Configuration, Who::Administrator,
                    _("The test ran on a client computer"),
                    _("The results include the network and this machine."),
                    _("Run the benchmark on the database server as well "
                      "(embedded or local connection). If the server run "
                      "is fast, the network or the client is the bottleneck."));
                break;
            case BenchmarkRunLocation::Unknown:
            default:
                break;
        }
    }

    void checkVersion()
    {
        categoryM = BenchmarkCategory::Environment;
        if (m.serverMajorVersion <= 0)
            return;
        if (m.serverMajorVersion < 4)
        {
            add(Sev::Info, Side::Configuration, Who::Administrator,
                wxString::Format(_("Firebird %s"), m.serverVersion),
                _("Firebird 3 lacks RDB$CONFIG, the Batch API, parallel "
                  "workers and the compiled statement cache."),
                _("Firebird 5 is considerably faster for multi-user "
                  "workloads (optimizer, statement cache, parallel sweep, "
                  "backup and index creation). Plan the upgrade with a "
                  "backup/restore and an application test."));
        }
        else if (m.serverMajorVersion == 4)
        {
            add(Sev::Info, Side::Configuration, Who::Administrator,
                wxString::Format(_("Firebird %s"), m.serverVersion),
                _("Firebird 4 has no parallel workers and no compiled "
                  "statement cache yet."),
                _("Firebird 5 adds parallel sweep, backup, restore and index "
                  "creation and caches compiled statements per connection."));
        }
        if (m.serverConfig.empty() && m.serverMajorVersion >= 4)
        {
            add(Sev::Info, Side::Configuration, Who::Administrator,
                _("The server settings could not be read"),
                _("RDB$CONFIG returns rows only for administrators, so the "
                  "firebird.conf values could not be checked."),
                _("Run the benchmark as SYSDBA or with the RDB$ADMIN role to "
                  "include the configuration checklist."));
        }
    }

    void checkConnect()
    {
        categoryM = BenchmarkCategory::Connection;
        if (!m.connectMs)
            return;
        double v = *m.connectMs;
        Sev severity = Limits::connectMs.rate(v);
        if (severity != Sev::Ok)
        {
            Side side = remote() ? Side::Network : Side::Server;
            add(severity, side, Who::Administrator,
                _("Connecting to the server is slow"),
                wxString::Format(_("Opening a connection takes %s on average."), ms(v)),
                _("Typical causes: slow DNS or reverse DNS lookups (test with "
                  "the IP address or a hosts entry), IPv6 tried before IPv4, "
                  "an authentication plugin that waits for a timeout (check "
                  "the order of AuthServer/AuthClient, e.g. Win_Sspi before "
                  "Srp), the SRP hash cost, a security database with forced "
                  "writes on slow storage, firewalls or virus scanners "
                  "inspecting the connection."));
        }
        if (v >= 50.0)
        {
            add(Sev::Info, Side::Client, Who::Developer,
                _("Every new connection takes noticeable time"),
                wxString::Format(_("Every new connection costs %s."), ms(v)),
                _("Keep connections open or use a connection pool instead of "
                  "connecting per request. With SuperServer (Firebird 3+) the "
                  "database setting LINGER (ALTER DATABASE SET LINGER TO 60) "
                  "keeps the page cache when the last connection closes."));
        }
    }

    void checkPowerManagement()
    {
        categoryM = BenchmarkCategory::LocalMachine;
        if (m.localOnBattery.value_or(false))
        {
            add(Sev::Warning, onServer() ? Side::Server : Side::Client,
                Who::Administrator, _("This computer runs on battery"),
                onServer() ? _("Notebooks lower the CPU clock and often the "
                                 "storage speed on battery. All values of this "
                                 "run are lower than on mains power, those of "
                                 "the database server included, because it runs "
                                 "on this machine.")
                           : _("Notebooks lower the CPU clock on battery. The "
                                 "values of this machine are lower than on "
                                 "mains power."),
                _("Connect the power supply and repeat the run. For comparisons "
                  "always measure on mains power with the power mode set to "
                  "best performance."));
        }
        auto burstRatio = [](const std::optional<double>& idle,
            const std::optional<double>& hot) -> double
        {
            if (!idle || !hot || *hot <= 0.0)
                return 0.0;
            return *idle / *hot;
        };

        double local = burstRatio(m.clientBurstIdleMs, m.clientBurstHotMs);
        if (local >= 1.4)
        {
            Side side = onServer() ? Side::Server : Side::Client;
            add(local >= 2.0 ? Sev::Problem : Sev::Warning, side,
                Who::Administrator,
                onServer() ? _("Power saving slows down the server")
                           : _("Power saving slows down this computer"),
                wxString::Format(_("A short CPU burst after an idle pause takes "
                    "%.1fx as long as the same burst back to back; the CPU "
                    "clocks down when idle."), local),
                localPowerAdvice(osOf(side)));
        }

        categoryM = BenchmarkCategory::ServerCpu;
        double server = burstRatio(m.serverBurstIdleMs, m.serverBurstHotMs);
        if (server >= 1.4 && m.serverBurstHotMs && *m.serverBurstHotMs >= 10.0)
        {
            add(server >= 2.0 ? Sev::Problem : Sev::Warning, Side::Server,
                Who::Administrator,
                _("Power saving slows down the server"),
                wxString::Format(_("Server-side CPU bursts after an idle pause "
                    "take %.1fx as long as back to back."), server),
                onServer() ? localPowerAdvice(m.serverPlatform)
                           : unverified(powerAdvice(m.serverPlatform)));
        }

        categoryM = BenchmarkCategory::Network;
        if (m.latencySparseMs && m.latencyMedianMs && *m.latencyMedianMs > 0.0)
        {
            double ratio = *m.latencySparseMs / *m.latencyMedianMs;
            double extra = *m.latencySparseMs - *m.latencyMedianMs;
            // a fraction of a millisecond is noticeable only for very
            // chatty applications; a problem needs a real delay
            if (ratio >= 2.0 && extra >= 0.3)
            {
                Side side = remote() ? Side::Network : Side::Server;
                add(ratio >= 4.0 && extra >= 1.0 ? Sev::Problem : Sev::Warning, side,
                    Who::Administrator,
                    _("The first request after a short pause is slow"),
                    wxString::Format(_("A round trip after a short pause takes "
                        "%s instead of %s. Interactive applications send "
                        "exactly this kind of sparse queries."),
                        ms(*m.latencySparseMs), ms(*m.latencyMedianMs)),
                    unverified(powerAdvice(m.serverPlatform) + " " + _("Also disable "
                        "\"Energy Efficient Ethernet\" and power saving of the "
                        "network adapters on client and server, and check "
                        "interrupt moderation.")));
            }
        }
    }

    void checkLatency()
    {
        categoryM = BenchmarkCategory::Network;
        if (!m.latencyAvgMs)
            return;
        double median = m.latencyMedianMs.value_or(*m.latencyAvgMs);

        if (remote())
        {
            Sev severity = Limits::remoteRoundTripMs.rate(median);
            if (severity != Sev::Ok)
            {
                add(severity, Side::Network,
                    Who::Administrator,
                    _("Requests over the network take long"),
                    wxString::Format(_("Every round trip takes %s (median). An "
                        "application that runs 1000 small queries waits %s "
                        "for the network alone; a healthy wired LAN needs "
                        "less than 0.5 ms."), ms(median), ms(median * 1000.0)),
                    _("Use a wired connection instead of WLAN, avoid VPN "
                      "tunnels for database traffic, check switches, cabling "
                      "and network drivers. For WAN access run the "
                      "application on a terminal server next to the "
                      "database."));
            }
            if (median >= 0.5)
            {
                add(Sev::Info, Side::Network, Who::Developer,
                    _("Small queries mostly wait for the network"),
                    wxString::Format(_("Each statement costs at least %s on "
                        "this network."), ms(median)),
                    _("Reduce round trips: fewer and larger queries, joins "
                      "instead of query loops, stored procedures or EXECUTE "
                      "BLOCK for multi-step work, prepared statements, and "
                      "the Batch API (Firebird 4+) for many inserts."));
            }
        }
        else if (m.connectionKind == BenchmarkConnectionKind::TcpLoopback
            || m.connectionKind == BenchmarkConnectionKind::TcpRemote)
        {
            Sev severity = Limits::localRoundTripMs.rate(median);
            if (severity != Sev::Ok)
            {
                add(severity, Side::Server,
                    Who::Administrator,
                    _("Even the local connection is slow"),
                    wxString::Format(_("A local round trip takes %s; "
                        "normally it needs well below 0.1 ms."), ms(median)),
                    _("Security software often filters local TCP traffic. "
                      "Exclude the Firebird process and port from network "
                      "inspection of the virus scanner or firewall.") + " "
                        + (m.serverPlatform == "Windows"
                            ? _("Local tools on Windows can use XNET instead of TCP.")
                            : wxString()));
            }
        }

        if (m.latencyP95Ms && median > 0.0 && *m.latencyP95Ms >= 3.0 * median
            && *m.latencyP95Ms - median >= 0.5)
        {
            add(Sev::Warning, remote() ? Side::Network : Side::Server,
                Who::Administrator,
                _("Response times vary a lot"),
                wxString::Format(_("5%% of the round trips take %s or more "
                    "while the median is %s."), ms(*m.latencyP95Ms), ms(median)),
                _("Typical for WLAN, overloaded network links or an "
                  "overloaded server. Check the network utilisation and "
                  "the CPU load of the server and the client."));
        }
    }

    void checkThroughput()
    {
        categoryM = BenchmarkCategory::Network;
        if (!m.streamMs || !m.serverReadMs || m.streamBytes <= 0)
            return;
        double clientMs = m.streamClientMs.value_or(0.0);
        double transferMs = std::max(0.0, *m.streamMs - *m.serverReadMs - clientMs);
        double mbPerSecond = double(m.streamBytes) / megabyte
            / (*m.streamMs / 1000.0);
        double transferShare = transferMs / *m.streamMs;

        Sev transferSeverity = Limits::transferMBPerSecond.rate(mbPerSecond);
        if (transferShare >= 0.7 && transferSeverity != Sev::Ok)
        {
            wxString advice = remote()
                ? _("Check the bandwidth of the network path (1 Gbit/s "
                    "delivers about 100 MB/s). On slow or WAN links enable "
                    "WireCompression = true in firebird.conf on client and "
                    "server.")
                : _("The local transfer is slow; check virus scanners or "
                    "firewalls that inspect local traffic.");
            wxString bufferSize = configValue("TcpRemoteBufferSize");
            if (!bufferSize.empty() && bufferSize != "32767")
            {
                advice += " " + wxString::Format(_("Increase "
                    "TcpRemoteBufferSize (currently %s) to 32767 for faster "
                    "transfer of large result sets."), bufferSize);
            }
            if (m.wireEncrypted.value_or(false))
            {
                advice += " " + _("Wire encryption is active; it costs CPU "
                    "time on both sides (keep it enabled on untrusted "
                    "networks).");
            }
            add(transferSeverity,
                remote() ? Side::Network : Side::Server, Who::Administrator,
                _("Large results arrive slowly"),
                wxString::Format(_("Streaming the test data reaches %.1f MB/s; "
                    "%.0f%% of the time is spent on the way between server "
                    "and FlameRobin, the server itself needs %s."),
                    mbPerSecond, transferShare * 100.0, ms(*m.serverReadMs)),
                advice);
            add(Sev::Info, Side::Network, Who::Developer,
                _("Applications should fetch less data"),
                wxString::Format(_("Large result sets are limited to %.1f MB/s "
                    "on this path."), mbPerSecond),
                _("Select only the needed columns, page with ROWS/FIRST, do "
                  "not load whole tables into grids, and aggregate on the "
                  "server."));
        }

        categoryM = BenchmarkCategory::LocalMachine;
        if (clientMs > 0.0 && clientMs >= 0.5 * *m.streamMs && *m.streamMs >= 200.0)
        {
            Side side = onServer() ? Side::Server : Side::Client;
            add(Sev::Warning, side, Who::Administrator,
                _("This computer is slow at processing received data"),
                wxString::Format(_("%s of %s are spent in FlameRobin itself."),
                    ms(clientMs), ms(*m.streamMs)),
                _("Check the CPU load of this machine.") + " "
                    + powerAdvice(osOf(side)));
        }
    }

    void checkCpu()
    {
        categoryM = BenchmarkCategory::ServerCpu;
        if (m.cpuMs)
        {
            double serverMs = m.cpuServerMs.value_or(*m.cpuMs);
            double perMs = serverMs > 0.0 ? s.cpuIterations / serverMs : 0.0;
            Sev severity = Limits::cpuIterationsPerMs.rate(perMs);
            if (perMs > 0.0 && severity != Sev::Ok)
            {
                add(severity, Side::Server,
                    Who::Hardware,
                    _("The server calculates slowly"),
                    wxString::Format(_("The compute test runs %.0f "
                        "iterations/ms; a current server CPU reaches more "
                        "than 1000."), perMs),
                    _("Check the CPU load of the server and CPU "
                      "overcommitment of the virtualization host (CPU ready "
                      "time). A CPU with a high single core clock helps "
                      "Firebird most, because one statement runs on one "
                      "core.") + " "
                        + powerAdvice(m.serverPlatform));
            }
        }

        if (m.cpuMs && m.parallelCpuMs && *m.parallelCpuMs > 0.0
            && s.parallelConnections > 1)
        {
            double scaling = *m.cpuMs * s.parallelConnections / *m.parallelCpuMs;
            if (scaling < 0.5 * s.parallelConnections)
            {
                wxString mask = configValue("CpuAffinityMask");
                bool masked = !mask.empty() && mask != "0";
                wxString advice = masked
                    ? wxString::Format(_("CpuAffinityMask = %s restricts "
                        "Firebird to certain cores; set it to 0 to use all "
                        "cores."), mask)
                    : _("Give the server enough CPU cores (virtual machines "
                        "often have too few vCPUs), check the CPU load caused "
                        "by other processes and CPU limits of the "
                        "virtualization host.");
                add(scaling < 1.5 ? Sev::Problem : Sev::Warning, Side::Server,
                    masked ? Who::Administrator : Who::Hardware,
                    _("The server does not use all its processor cores"),
                    wxString::Format(_("%d connections working in parallel "
                        "are only %.1fx as fast as one connection."),
                        s.parallelConnections, scaling),
                    advice);
            }
        }
    }

    void checkCommit()
    {
        categoryM = BenchmarkCategory::ServerStorage;
        if (!m.commitMs)
            return;
        double v = *m.commitMs;
        if (!m.benchmarkDatabase.forcedWrites && m.benchmarkDatabase.pageSize > 0)
        {
            add(Sev::Info, Side::Configuration, Who::Administrator,
                _("Changes were not written safely during the test (forced writes off)"),
                _("The commit and write results are faster than with the "
                  "safe default setting."),
                wxEmptyString);
        }
        const Sev severity = Limits::commitMs.rate(v);
        if (severity == Sev::Ok)
            return;

        // compare with the raw operating system sync write when available
        wxString detail = wxString::Format(_("Committing a single-row "
            "transaction takes %s."), ms(v));
        if (m.osSyncWriteMs && *m.osSyncWriteMs < v / 3.0)
        {
            detail += " " + wxString::Format(_("A plain synchronous write "
                "of the operating system in the same directory takes only "
                "%s, so the storage itself is fast."), ms(*m.osSyncWriteMs));
            add(severity, Side::Server,
                Who::Administrator, _("Saving changes is slow"), detail,
                antivirusAdvice(m.serverPlatform) + " " + _("Also check the "
                    "server load and whether nbackup is active."));
        }
        else
        {
            wxString advice = _("Every commit waits for the storage (forced "
                "writes). Use SSD/NVMe storage with power-loss protection or "
                "a RAID controller with battery backed write-back cache; on "
                "virtual machines check the storage latency of the host.");
            if (m.serverPlatform == "Windows")
            {
                advice += " " + _("On a domain controller Windows disables "
                    "the write cache of the disk holding the Active Directory "
                    "database; do not put databases on that disk.");
            }
            add(severity, Side::Server,
                Who::Hardware, _("Saving changes is slow"), detail,
                advice + " " + antivirusAdvice(m.serverPlatform));
        }
        add(Sev::Info, Side::Server, Who::Developer,
            _("Each saved change takes noticeable time"),
            wxString::Format(_("Every commit costs %s on this server."), ms(v)),
            _("Group related changes into one transaction instead of "
              "committing every row; avoid auto-commit for bulk work."));
    }

    void checkDiskWrites()
    {
        categoryM = BenchmarkCategory::ServerStorage;
        if (!m.insertMs || *m.insertMs <= 0.0)
            return;
        double rowsPerSecond = s.diskRows * 1000.0 / *m.insertMs;
        Sev severity = Limits::insertRowsPerSecond.rate(rowsPerSecond);
        if (severity == Sev::Ok)
            return;
        add(severity, Side::Server,
            Who::Hardware,
            _("Writing many rows is slow"),
            wxString::Format(_("Inserting into a table with 5 indexes "
                "reaches %.0f rows/s; a healthy server reaches more than "
                "15000 rows/s."), rowsPerSecond),
            storageAdvice() + " " + _("Also check the page cache size and "
                "the server load.") + " " + antivirusAdvice(m.serverPlatform));
    }

    wxString storageAdvice() const
    {
        return _("Check the storage under load: SSD/NVMe instead of HDD, "
            "RAID 10 instead of RAID 5/6 for write workloads, a RAID "
            "controller with battery or flash backed write-back cache, current "
            "storage and controller drivers, and on virtual machines the "
            "latency and I/O limits of the host storage.");
    }

    void checkSmallCache()
    {
        categoryM = BenchmarkCategory::ServerStorage;
        if (!m.smallCacheInsertMs || *m.smallCacheInsertMs <= 0.0)
            return;
        if (m.smallCachePages && *m.smallCachePages > 1000)
        {
            add(Sev::Info, Side::Server, Who::Administrator,
                _("The disk test could not use a small cache"),
                wxString::Format(_("The server used %d cache pages instead "
                    "of a minimal cache, so the drive test measured the "
                    "cache, not the storage."), *m.smallCachePages),
                _("Another connection probably kept the temporary "
                  "database open, so the new page buffers in its header "
                  "were not read."));
            return;
        }
        double rowsPerSecond = s.diskRows * 1000.0 / *m.smallCacheInsertMs;
        Sev severity = Limits::storageRowsPerSecond.rate(rowsPerSecond);
        if (severity == Sev::Ok)
            return;
        add(severity, Side::Server,
            Who::Hardware,
            _("The disk is slow"),
            wxString::Format(_("With a minimal page cache, inserting into a "
                "table with 5 indexes reaches only %.0f rows/s. Every query "
                "that does not find its pages in a cache waits for this "
                "storage."), rowsPerSecond),
            storageAdvice());
    }

    void checkLocalFiles()
    {
        categoryM = BenchmarkCategory::LocalMachine;
        auto check = [this](const std::optional<double>& value,
            const wxString& where, bool databaseDirectory)
        {
            if (!value)
                return;
            Sev severity = Limits::fileOperationMs.rate(*value);
            if (severity == Sev::Ok)
                return;
            Side side = onServer() ? Side::Server : Side::Client;
            wxString detail = wxString::Format(_("Creating, writing and deleting a "
                "small file in the %s takes %s; without on-access scanning it "
                "takes well below 0.5 ms."), where, ms(*value));
            const std::vector<wxString> scanners = activeScanners();
            if (scannersKnown() && scanners.empty())
            {
                add(severity, side, Who::Administrator, _("Creating and deleting files is slow"),
                    detail + " " + _("No active virus scanner was found."),
                    _("Other causes: a slow or nearly full drive, drive encryption, "
                      "backup or synchronisation agents (OneDrive, Dropbox), "
                      "indexing services, or an endpoint protection that does not "
                      "register as a virus scanner."));
                return;
            }
            if (!scanners.empty())
                detail += " " + wxString::Format(_("Active virus scanner: %s."), join(scanners));
            wxString advice = onServer() ? antivirusAdvice(osOf(side))
                : _("Exclude the application directories and temporary files of "
                    "the database applications from on-access scanning on the "
                    "client.");
            const std::optional<bool>& excluded = m.system.storagePathExcluded;
            if (databaseDirectory && excluded && *excluded)
            {
                advice = _("The database directory is already excluded from "
                    "Microsoft Defender. If another product scans it, add the "
                    "exclusion there; otherwise look for other causes such as "
                    "drive encryption or backup agents.");
            }
            else if (databaseDirectory && excluded)
            {
                advice += " " + _("The database directory is not excluded from "
                    "Microsoft Defender yet.");
            }
            else
            {
                advice += " " + _("If the exclusions exist already, the cause is "
                    "elsewhere; FlameRobin can only read them with administrator "
                    "rights.");
            }
            add(severity, side, Who::Administrator,
                _("A virus scanner probably slows down file access"), detail, advice);
        };
        check(m.tempDirFileMs, _("temporary directory"), false);
        categoryM = BenchmarkCategory::ServerStorage;
        check(m.databaseDirFileMs, _("database directory"), true);

        if (m.databaseDiskFreeBytes)
        {
            double freeBytes = double(*m.databaseDiskFreeBytes);
            double dbSize = m.inspectedDatabase
                ? double(m.inspectedDatabase->pages) * m.inspectedDatabase->pageSize
                : 0.0;
            if (freeBytes < 2.0 * gigabyte || freeBytes < dbSize)
            {
                add(Sev::Warning, Side::Server, Who::Hardware,
                    _("Little free disk space"),
                    wxString::Format(_("Only %.1f GB are free on the drive "
                        "of the database."), freeBytes / gigabyte),
                    _("Firebird needs free space for database growth, "
                      "temporary sort files and backups. Fragmented, nearly "
                      "full file systems are slow."));
            }
        }
    }

    void checkCache()
    {
        categoryM = BenchmarkCategory::ServerCache;
        // the page cache of the selected database when it was inspected,
        // otherwise the server default used by the benchmark database
        const BenchmarkDatabaseInfo& db = m.inspectedDatabase
            ? *m.inspectedDatabase : m.benchmarkDatabase;
        if (db.pageSize <= 0 || db.cachePages <= 0)
            return;
        double cacheBytes = double(db.cachePages) * db.pageSize;
        double dbBytes = m.inspectedDatabase ? double(db.pages) * db.pageSize : 0.0;

        if (m.inspectedDatabase && db.pageSize < 8192)
        {
            categoryM = BenchmarkCategory::Environment;
            add(Sev::Warning, Side::Configuration, Who::Administrator,
                _("The page size of the database is small"),
                wxString::Format(_("The selected database uses %d byte "
                    "pages."), db.pageSize),
                _("Use 8192 or 16384 byte pages (32768 for very large "
                  "databases on Firebird 4+) for better index depth and I/O "
                  "efficiency; change it with gbak backup and restore "
                  "(-page_size)."));
        }

        categoryM = BenchmarkCategory::ServerCache;
        double workingSet = std::max(double(m.scanBytes), dbBytes * 0.1);
        if (!classicMode() && cacheBytes < workingSet && cacheBytes < gigabyte)
        {
            add(Sev::Warning, Side::Configuration, Who::Administrator,
                _("Firebird keeps little data in memory (small page cache)"),
                wxString::Format(_("The page cache holds %.0f MB, the "
                    "test data alone has %.0f MB%s. Data that does not fit "
                    "is read from the operating system cache or the disk "
                    "again."),
                    cacheBytes / megabyte, double(m.scanBytes) / megabyte,
                    dbBytes > 0.0
                        ? wxString::Format(_(", the selected database %.0f MB"),
                            dbBytes / megabyte)
                        : wxString()),
                _("For SuperServer increase DefaultDbCachePages in "
                  "firebird.conf (e.g. 50000 pages) or the page buffers of "
                  "the database (gfix -buffers), keep UseFileSystemCache = "
                  "true and FileSystemCacheThreshold above the cache size. "
                  "See the configuration checklist."));
        }
        else if (classicMode())
        {
            add(Sev::Info, Side::Configuration, Who::Administrator,
                wxString::Format(_("Server mode %s"), serverMode()),
                _("Every connection has its own small page cache, so "
                  "repeated queries profit less from caching."),
                _("Since Firebird 3, SuperServer shares one large cache "
                  "between all connections and uses all CPU cores; it is the "
                  "better choice for most workloads."));
        }

        if (m.coldScanMs && m.warmScanMs && *m.warmScanMs > 0.0
            && *m.coldScanMs >= 5.0 * *m.warmScanMs && *m.coldScanMs >= 500.0)
        {
            add(Sev::Info, Side::Server, Who::Hardware,
                _("Data that is not in memory is read much more slowly"),
                wxString::Format(_("The first scan took %s, the repeated "
                    "scan %s. Workloads that do not fit into the caches "
                    "will be limited by the storage."),
                    ms(*m.coldScanMs), ms(*m.warmScanMs)),
                _("Give the server enough RAM for the Firebird page cache "
                  "and the operating system file cache (as a rule of thumb "
                  "the frequently used part of the database), or use faster "
                  "storage."));
        }
    }

    void checkSort()
    {
        categoryM = BenchmarkCategory::ServerCache;
        if (!m.serverReadMs || !m.serverSortMs || *m.serverReadMs <= 0.0)
            return;
        double ratio = *m.serverSortMs / *m.serverReadMs;
        if (ratio < 6.0 || *m.serverSortMs < 300.0)
            return;
        wxString advice = _("Sorts that do not fit into memory are written "
            "to temporary files. Increase TempCacheLimit (SuperServer e.g. "
            "1 GB), place TempDirectories on fast local storage and exclude "
            "it from virus scanning.");
        wxString limit = configValue("TempCacheLimit");
        if (!limit.empty())
            advice += " " + wxString::Format(_("Current TempCacheLimit: %s."), limit);
        add(Sev::Warning, Side::Server, Who::Administrator,
            _("Sorting is slow"),
            wxString::Format(_("Sorting the test data takes %.1fx as long "
                "as reading it."), ratio),
            advice);
        add(Sev::Info, Side::Server, Who::Developer,
            _("Sort only the columns you need"),
            _("Sorting wide rows is expensive."),
            _("Sort only the needed columns, avoid ORDER BY/DISTINCT/UNION "
              "where not needed, and use indexes for ORDER BY with ROWS."));
    }

    void checkDatabaseState()
    {
        categoryM = BenchmarkCategory::Environment;
        // only the selected database can have a history worth checking
        if (!m.inspectedDatabase)
            return;
        const BenchmarkDatabaseInfo& db = *m.inspectedDatabase;
        if (!db.forcedWrites)
        {
            add(Sev::Warning, Side::Configuration, Who::Administrator,
                _("The selected database does not write changes safely (forced writes off)"),
                _("Writes are fast, but a power loss or operating system "
                  "crash can corrupt the database."),
                _("Enable forced writes (gfix -write sync) unless the "
                  "storage has a battery or flash backed write cache and "
                  "the risk is accepted consciously."));
        }
        int64_t gap = db.nextTransaction - db.oldestActiveTransaction;
        if (db.nextTransaction > 0 && gap > 100000)
        {
            add(gap > 1000000 ? Sev::Problem : Sev::Warning,
                Side::Configuration, Who::Developer,
                _("A transaction has been open for a long time"),
                wxString::Format(_("The oldest active transaction is %lld "
                    "transactions behind. Garbage collection cannot remove "
                    "old record versions, so the database gets slower over "
                    "time."), (long long)gap),
                _("Find the transaction in MON$TRANSACTIONS and fix the "
                  "application so that it commits regularly; use read-only "
                  "read committed transactions for long running reads."));
        }
        if (db.backupState && *db.backupState != 0)
        {
            add(Sev::Problem, Side::Configuration, Who::Administrator,
                _("The database is locked for a backup (nbackup)"),
                _("The database is locked by nbackup; all changes go to the "
                  "delta file, which costs performance and disk space."),
                _("Finish the backup (nbackup -N or ALTER DATABASE END "
                  "BACKUP) if no backup is running."));
        }
        if (db.cryptState != 0)
        {
            add(Sev::Info, Side::Server, Who::Administrator,
                _("The database is encrypted"),
                _("Encryption costs CPU time for every page read and write."),
                wxEmptyString);
        }
    }

    void checkClientLibrary()
    {
        categoryM = BenchmarkCategory::Environment;
        if (m.clientVersion.empty() || m.serverMajorVersion <= 0)
            return;
        // e.g. "WI-V3.0.10.33601 Firebird 3.0"
        int clientMajor = 0;
        int pos = m.clientVersion.Find("-V");
        if (pos != wxNOT_FOUND)
        {
            long value = 0;
            if (m.clientVersion.Mid(pos + 2).BeforeFirst('.').ToLong(&value))
                clientMajor = int(value);
        }
        if (clientMajor > 0 && clientMajor < m.serverMajorVersion)
        {
            add(Sev::Warning, onServer() ? Side::Server : Side::Client,
                Who::Developer,
                _("FlameRobin uses an old client library"),
                wxString::Format(_("The client library is version %d, the "
                    "server version %d."), clientMajor, m.serverMajorVersion),
                _("Ship the client library of the server version with the "
                  "application; newer clients support faster protocol "
                  "features of the server (e.g. wire compression, Batch "
                  "API)."));
        }
    }

    void checkRoundTrips()
    {
        categoryM = BenchmarkCategory::Application;
        if (!m.rowByRowMs || !m.batchMs || *m.batchMs <= 0.0)
            return;
        double ratio = *m.rowByRowMs / *m.batchMs;
        if (ratio < 3.0)
            return;
        add(ratio >= 10.0 ? Sev::Warning : Sev::Info, Side::Client,
            Who::Developer,
            _("Sending rows one by one is expensive"),
            wxString::Format(_("Inserting %d rows one statement at a time "
                "takes %s, the same work in one server call %s (%.0fx)."),
                s.rowByRowRows, ms(*m.rowByRowMs), ms(*m.batchMs), ratio),
            _("Move loops to the server (stored procedures, EXECUTE BLOCK, "
              "INSERT ... SELECT, MERGE) or use the Batch API of Firebird 4+ "
              "for bulk inserts; keep statements prepared."));
    }

    void checkPrepare()
    {
        categoryM = BenchmarkCategory::Application;
        if (!m.prepareMs || *m.prepareMs < 3.0)
            return;
        wxString advice = _("Prepare statements once and execute them many "
            "times with parameters; avoid building SQL with literal values.");
        if (m.serverMajorVersion >= 5)
        {
            advice += " " + _("Firebird 5 caches compiled statements per "
                "connection; MaxStatementCacheSize (default 2 MB) can be "
                "increased for applications with many different statements.");
        }
        add(*m.prepareMs >= 10.0 ? Sev::Warning : Sev::Info, Side::Server,
            Who::Developer,
            _("Preparing queries takes long"),
            wxString::Format(_("Preparing a typical query takes %s."),
                ms(*m.prepareMs)),
            advice);
    }

    void checkBlob()
    {
        categoryM = BenchmarkCategory::Application;
        if (!m.blobReadMs || *m.blobReadMs <= 0.0 || m.blobBytes <= 0)
            return;
        double readMb = double(m.blobBytes) / megabyte / (*m.blobReadMs / 1000.0);
        if (readMb >= 20.0)
            return;
        add(readMb < 5.0 ? Sev::Warning : Sev::Info,
            remote() ? Side::Network : Side::Server, Who::Developer,
            _("Reading and writing large data (BLOBs) is slow"),
            wxString::Format(_("Reading BLOBs reaches %.1f MB/s."), readMb),
            _("Load BLOBs only when they are shown (not in lists or grids), "
              "keep large documents outside of frequently read tables, and "
              "read them with large segments. For text BLOBs on slow links "
              "WireCompression helps."));
    }

    void checkVersions()
    {
        categoryM = BenchmarkCategory::Application;
        if (!m.versionReadBeforeMs || *m.versionReadBeforeMs <= 0.0)
            return;
        const double before = *m.versionReadBeforeMs;
        const double withOld = m.versionReadWithOldTxMs.value_or(0.0);
        const double afterEnd = m.versionReadAfterCommitMs.value_or(0.0);
        const bool slowWithOld = withOld >= 2.0 * before;
        // the reader that removes the old versions after the long
        // transaction ended
        const bool slowCleanup = afterEnd >= 10.0 * before && afterEnd >= 5.0;
        if (!slowWithOld && !slowCleanup)
            return;

        wxString detail;
        if (slowWithOld)
        {
            detail = wxString::Format(_("After %d updates of the same rows, "
                "reading them in a transaction that was started before takes "
                "%.1fx as long (%s instead of %s)."), s.versionRounds,
                withOld / before, ms(withOld), ms(before));
        }
        if (slowCleanup)
        {
            detail += (detail.empty() ? "" : " ") + wxString::Format(_("When "
                "the old transaction ended, the next read took %s instead of "
                "%s, because it removed the old record versions (garbage "
                "collection)."), ms(afterEnd), ms(before));
        }
        add(withOld >= 5.0 * before || afterEnd >= 100.0 ? Sev::Warning : Sev::Info,
            Side::Server, Who::Developer,
            _("Long open transactions slow down reading"),
            detail,
            _("Keep transactions short; use read-only READ COMMITTED "
              "transactions for reports and lookups; do not keep datasets "
              "open in SNAPSHOT transactions; commit (not COMMIT RETAINING "
              "only) regularly so that garbage collection can work."));
    }

    // hardware and software of the machine running FlameRobin; with a run
    // on the server that is the database server
    void checkSystem()
    {
        const BenchmarkSystemInfo& sys = m.system;
        const Side side = onServer() ? Side::Server : Side::Client;
        if (!sys.storagePath.empty())
        {
            categoryM = BenchmarkCategory::ServerStorage;
            if (sys.storageHealthWarning.value_or(false))
            {
                wxString values;
                for (const auto& v : sys.storageHealth)
                {
                    if (v.warning)
                    {
                        values += (values.empty() ? "" : ", ")
                            + wxString::FromUTF8(v.name.c_str()) + ": "
                            + wxString::FromUTF8(v.value.c_str());
                    }
                }
                add(Sev::Problem, Side::Server, Who::Hardware,
                    _("The disk of the database reports problems"),
                    wxString::Format(_("The health values (SMART) of %s are outside "
                        "the healthy range: %s."), wxString::FromUTF8(
                        sys.storageModel.c_str()), values),
                    _("Make a backup now and plan to replace the drive. Check the "
                      "full SMART data with the tools of the manufacturer."));
            }
            if (sys.storageMedia == "HDD")
            {
                add(m.commitMs && *m.commitMs > 3.0 ? Sev::Warning : Sev::Info,
                    Side::Server, Who::Hardware, _("The database is on a hard disk"),
                    _("Hard disks need milliseconds for every random access; "
                      "commits, index lookups and garbage collection wait for them."),
                    _("Move the database to an SSD (NVMe if possible)."));
            }
            if (sys.writeCachePowerProtected.value_or(false))
            {
                add(Sev::Warning, Side::Server, Who::Hardware,
                    _("The disk confirms writes before they are safe"),
                    _("\"Turn off Windows write-cache buffer flushing\" is set for "
                      "this drive: commits look fast, but committed data can get "
                      "lost on a power failure unless the drive or its controller "
                      "protects its cache (capacitors, battery)."),
                    _("Keep the option only for drives or controllers with power "
                      "loss protection; a UPS alone does not protect against "
                      "crashes of the drive firmware."));
            }
            else if (sys.writeCacheEnabled && !*sys.writeCacheEnabled)
            {
                add(m.commitMs && *m.commitMs > 2.0 ? Sev::Warning : Sev::Info,
                    Side::Server, Who::Hardware, _("The write cache of the disk is off"),
                    _("Every write goes directly to the medium, which makes commits "
                      "with forced writes slow."),
                    _("Turn the write cache on (device manager, policies of the "
                      "drive) if the drive or controller has power loss "
                      "protection."));
            }
        }
        categoryM = BenchmarkCategory::Environment;
        int firebirdRunning = int(std::count_if(sys.firebirdServers.begin(),
            sys.firebirdServers.end(), [](const BenchmarkServerProcess& p) { return p.running; }));
        if (onServer() && firebirdRunning >= 2)
        {
            add(Sev::Info, Side::Server, Who::Administrator,
                wxString::Format(_("%d Firebird servers run on this computer"),
                    firebirdRunning),
                _("They share CPU cores, RAM (every SuperServer has its own page "
                  "cache) and the drives; the results of this run depend on what "
                  "the others do."),
                _("Stop instances that are not needed, and size the page caches "
                  "of all instances together for the RAM of the machine."));
        }
        int othersRunning = int(std::count_if(sys.otherDatabaseServers.begin(),
            sys.otherDatabaseServers.end(),
            [](const BenchmarkServerProcess& p) { return p.running; }));
        if (onServer() && othersRunning > 0)
        {
            wxString names;
            for (const auto& p : sys.otherDatabaseServers)
            {
                if (p.running)
                    names += (names.empty() ? "" : ", ") + wxString::FromUTF8(p.name.c_str());
            }
            add(Sev::Info, Side::Server, Who::Administrator,
                _("Other database servers run on this computer"),
                wxString::Format(_("%s share CPU, RAM and drives with Firebird."), names),
                _("Give every server its share of the RAM (e.g. max server memory "
                  "of SQL Server) so that the operating system file cache keeps "
                  "room for Firebird."));
        }
        if (onServer() && !sys.virtualization.empty())
        {
            add(Sev::Info, Side::Server, Who::Hardware,
                wxString::Format(_("The server is a virtual machine (%s)"),
                    wxString::FromUTF8(sys.virtualization.c_str())),
                _("Other virtual machines on the same host share CPU, storage and "
                  "network; waiting for a free CPU (CPU ready) and storage limits "
                  "do not show inside the machine."),
                _("Reserve CPU and RAM for the database server, avoid memory "
                  "overcommitment, and give its disks a low latency storage "
                  "without I/O limits."));
        }
        if (onServer() && sys.memoryTotalBytes > 0 && sys.memoryAvailableBytes > 0
            && double(sys.memoryAvailableBytes) < 0.1 * double(sys.memoryTotalBytes))
        {
            add(Sev::Warning, side, Who::Administrator, _("Little free memory (RAM)"),
                wxString::Format(_("Only %.1f of %.1f GB are free; the operating "
                    "system has little room for its file cache."),
                    sys.memoryAvailableBytes / gigabyte, sys.memoryTotalBytes / gigabyte),
                _("Check which processes use the memory, and size the page caches "
                  "so that the operating system keeps some for its file cache."));
        }
    }

    // the way from this client to the server
    void checkNetworkPath()
    {
        categoryM = BenchmarkCategory::Network;
        if (!remote())
            return;
        const BenchmarkNetworkPath p = m.networkPath.value_or(BenchmarkNetworkPath());
        if (p.adapterType == "Wi-Fi" || p.adapterType == "VPN")
        {
            add(Sev::Info, Side::Network, Who::Administrator,
                p.adapterType == "VPN" ? _("The connection runs through a VPN")
                                       : _("The connection runs over Wi-Fi"),
                wxString::Format(_("This machine reaches the server through %s (%s). "
                    "It adds latency and fluctuations to every round trip."),
                    wxString::FromUTF8(p.adapterName.c_str()),
                    wxString::FromUTF8(p.adapterDescription.c_str())),
                _("For comparisons and for data-heavy work use a wired network "
                  "connection."));
        }
        if (p.resolveMs >= 100.0)
        {
            add(Sev::Warning, Side::Network, Who::Administrator,
                _("Finding the server by its name is slow"),
                wxString::Format(_("Finding the address of %s takes %s."),
                    wxString::FromUTF8(p.serverHost.c_str()), ms(p.resolveMs)),
                _("Check the DNS servers of this machine, or connect with the IP "
                  "address or a hosts entry."));
        }
        // NAT, VPN or a proxy between client and server
        if (!m.remoteAddress.empty() && !m.system.addresses.empty())
        {
            const wxString seen = m.remoteAddress.BeforeLast('/').empty()
                ? m.remoteAddress : m.remoteAddress.BeforeLast('/');
            bool own = std::any_of(m.system.addresses.begin(), m.system.addresses.end(),
                [&seen](const BenchmarkNetworkAddress& a)
                { return seen.Contains(wxString::FromUTF8(a.address.c_str())); });
            if (!own)
            {
                add(Sev::Info, Side::Network, Who::Administrator,
                    _("A router or VPN sits between this computer and the server"),
                    wxString::Format(_("The server sees this machine as %s, which is "
                        "none of its own addresses: network address translation "
                        "(NAT), a VPN or a proxy is between them."), seen),
                    _("Devices between client and server add latency and often "
                      "drop idle connections; see TCP keepalive."));
            }
        }
        // the share of the login in the connect time
        double handshake = -1.0;
        for (const auto& c : m.serverPorts)
        {
            if (c.port == m.serverPort && c.state == BenchmarkPortCheck::State::Open)
                handshake = c.ms;
        }
        if (handshake > 0.0 && m.connectMs && *m.connectMs > 50.0
            && *m.connectMs > 10.0 * handshake)
        {
            add(Sev::Info, Side::Server, Who::Administrator,
                _("Logging in takes most of the connection time"),
                wxString::Format(_("The network part of a connection takes %s, the "
                    "complete connect %s."), ms(handshake), ms(*m.connectMs)),
                _("Look at the authentication (order of AuthServer, Win_Sspi with "
                  "slow domain controllers, the SRP cost), the security database "
                  "and reverse DNS lookups of the server."));
        }
        if (m.latencyOutliers > 0)
        {
            const double share = 100.0 * m.latencyOutliers / std::max(1, s.latencyQueries);
            add(share >= 0.2 ? Sev::Warning : Sev::Info, Side::Network,
                Who::Administrator, _("Network packets get lost"),
                wxString::Format(_("%d of %d round trips took over 150 ms. TCP sends "
                    "a lost packet again only after such a timeout."),
                    m.latencyOutliers, s.latencyQueries),
                _("Look for Wi-Fi interference, faulty cables or ports, duplex "
                  "mismatches, overloaded switches, firewalls and VPNs."));
        }
        // the transfer compared with the link speed
        if (p.linkSpeedBitsPerSecond >= 100000000 && m.streamMs && *m.streamMs > 0.0
            && m.streamBytes > 0)
        {
            const double mbPerSecond = m.streamBytes / megabyte / (*m.streamMs / 1000.0);
            const double linkMB = p.linkSpeedBitsPerSecond / 8.0 / megabyte;
            if (mbPerSecond < 0.2 * linkMB)
            {
                add(Sev::Info, Side::Network, Who::Administrator,
                    _("Large results use only part of the network speed"),
                    wxString::Format(_("The transfer reaches %.1f MB/s on a %s "
                        "link (about %.0f MB/s)."), mbPerSecond,
                        formatBenchmarkBitRate(double(p.linkSpeedBitsPerSecond)), linkMB),
                    _("The round trips of the fetches, wire encryption or "
                      "compression, network inspection by firewalls or virus "
                      "scanners and VPNs limit the transfer; a larger "
                      "TcpRemoteBufferSize helps on links with latency."));
            }
        }
    }

    // ports of the server from a client; listening ports of this machine
    void checkPorts()
    {
        categoryM = BenchmarkCategory::Network;
        for (const auto& l : m.system.listeningPorts)
        {
            if (!onServer() || l.firewall != "no inbound rule"
                || (!isFirebirdProcess(l.process) && l.port != m.serverPort))
            {
                continue;
            }
            add(Sev::Info, Side::Server, Who::Administrator,
                wxString::Format(_("The firewall may block port %d for other computers"), l.port),
                wxString::Format(_("%s listens on port %d, but the Windows firewall "
                    "has no rule that lets other machines connect."),
                    wxString::FromUTF8(l.process.c_str()), l.port),
                _("If clients on other machines use this server, allow inbound TCP "
                  "connections to this port (and a fixed RemoteAuxPort for "
                  "events) for firebird.exe."));
        }
    }

    static bool isFirebirdProcess(const std::string& process)
    {
        return process.find("firebird") != std::string::npos
            || process.find("fbserver") != std::string::npos
            || process.find("fb_inet_server") != std::string::npos
            || process.find("fb_smp_server") != std::string::npos;
    }

    void checkEvents()
    {
        categoryM = BenchmarkCategory::Network;
        if (m.eventMs || m.eventStatus.empty())
            return;
        if (m.eventStatus.StartsWith("not tested: RemoteAuxPort is 0"))
        {
            add(Sev::Warning, Side::Server, Who::Administrator,
                _("Events use a random port that firewalls block"),
                _("RemoteAuxPort is 0, so the server opens a random port for every "
                  "event connection. Firewalls between client and server usually "
                  "block it, and applications then never receive their events."),
                _("Set RemoteAuxPort in firebird.conf to a fixed port (e.g. 3051), "
                  "restart the server and allow the port in the firewalls."));
        }
        else if (m.eventStatus.StartsWith("not tested"))
        {
            add(Sev::Info, Side::Server, Who::Administrator,
                _("Event delivery was not tested"), m.eventStatus,
                unverified(_("set RemoteAuxPort to a fixed port and allow it in the "
                    "firewalls, if applications use events (POST_EVENT).")));
        }
        else
        {
            add(Sev::Problem, Side::Network, Who::Administrator,
                _("Events do not reach this computer"),
                wxString::Format(_("Event delivery: %s. Applications that wait for "
                    "events (POST_EVENT) do not react."), m.eventStatus),
                _("Allow the RemoteAuxPort of the server in all firewalls between "
                  "client and server; with network address translation the server "
                  "needs a fixed RemoteAuxPort that is forwarded as well."));
        }
    }

    void checkTimeouts()
    {
        categoryM = BenchmarkCategory::Environment;
        long configured = 0;
        configValue("ConnectionIdleTimeout").ToLong(&configured);
        const int idleSeconds = m.idleTimeoutSeconds.value_or(int(configured) * 60);
        if (idleSeconds > 0)
        {
            add(Sev::Warning, Side::Server, Who::Administrator,
                _("Firebird closes unused connections after a while"),
                wxString::Format(_("Connections that are idle for %d minutes are "
                    "closed by the server (ConnectionIdleTimeout). Applications then "
                    "report lost connections, like with a firewall that drops idle "
                    "connections."), idleSeconds / 60),
                _("Set ConnectionIdleTimeout to 0 in firebird.conf, or make sure the "
                  "applications reconnect, or set a longer timeout per connection."));
        }
    }

    void checkKeepAlive()
    {
        categoryM = BenchmarkCategory::Network;
        if (!m.system.tcpKeepAliveSeconds || m.connectionKind == BenchmarkConnectionKind::Embedded)
            return;
        const int minutes = *m.system.tcpKeepAliveSeconds / 60;
        if (minutes < 60)
            return;
        wxString advice = m.clientOsFamily == "Windows"
            ? _("On Windows set KeepAliveTime (DWORD, milliseconds) under "
                "HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters, e.g. "
                "300000 for 5 minutes, and restart.")
            : m.clientOsFamily == "macOS"
            ? _("On macOS set net.inet.tcp.keepidle (milliseconds), e.g. to "
                "300000 for 5 minutes (sysctl).")
            : _("On Linux set net.ipv4.tcp_keepalive_time, e.g. to 300 seconds "
                "(sysctl, /etc/sysctl.d).");
        add(Sev::Info, onServer() ? Side::Server : Side::Client, Who::Administrator,
            _("Unused connections may be cut off by firewalls"),
            wxString::Format(_("This machine sends the first keepalive packet after "
                "%d minutes of idle time. Firewalls, NAT routers and load balancers "
                "often drop idle connections after 5 to 60 minutes; applications "
                "then find their connection gone."), minutes),
            advice + " " + (onServer() ? _("Do the same on the clients.")
                : unverified(_("set a short keepalive time on the server as "
                    "well."))));
    }

    void checkWideInserts()
    {
        categoryM = BenchmarkCategory::Network;
        if (!remote() || !m.wideInsertMs || *m.wideInsertMs <= 0.0)
            return;
        const double iops = m.wideInsertRows * 1000.0 / *m.wideInsertMs;
        if (iops < 400.0)
        {
            add(Sev::Warning, Side::Network, Who::Administrator,
                _("Few small writes per second over the network"),
                wxString::Format(_("Only %.0f rows per second can be written when "
                    "every row is sent on its own; a fast local network manages 400 "
                    "and more."), iops),
                _("Check latency and packet loss of the network; applications "
                  "should send rows in batches instead of one by one."));
        }
    }

    void checkTempSort()
    {
        categoryM = BenchmarkCategory::ServerStorage;
        if (!m.tempSortMs || *m.tempSortMs <= 0.0 || !m.serverSortMs
            || *m.serverSortMs <= 0.0 || m.scanBytes <= 0 || m.tempSortBytes <= 0)
        {
            return;
        }
        const double big = m.tempSortBytes / megabyte / (*m.tempSortMs / 1000.0);
        const double small = m.scanBytes / megabyte / (*m.serverSortMs / 1000.0);
        if (big < 0.25 * small)
        {
            const wxString limit = configValue("TempCacheLimit");
            add(Sev::Warning, Side::Server, Who::Administrator,
                _("Sorting large data is slow"),
                wxString::Format(_("Sorting more data than fits into the sort memory "
                    "reaches %.0f MB/s, a sort in memory %.0f MB/s."), big, small),
                (limit.empty() ? unverified(_("raise TempCacheLimit in "
                    "firebird.conf (e.g. 1 GB with enough RAM)."))
                    : wxString::Format(_("Raise TempCacheLimit (now %s bytes) in "
                        "firebird.conf, e.g. to 1 GB with enough RAM."), limit))
                + " " + _("Put TempDirectories on a fast local drive and exclude it "
                    "from virus scanning."));
        }
    }

    void checkManyUsers()
    {
        categoryM = BenchmarkCategory::MultiUser;
        double peak = 0.0;
        for (const auto& o : m.oltp)
        {
            if (o.seconds > 0.0)
                peak = std::max(peak, o.transactions / o.seconds);
        }
        const BenchmarkOltpResult* most = m.oltp.empty() ? nullptr : &m.oltp.back();
        if (!most || most->clients < 50 || most->seconds <= 0.0 || peak <= 0.0)
            return;
        const double tps = most->transactions / most->seconds;
        if (tps < 0.5 * peak)
        {
            add(Sev::Warning, Side::Server, Who::Administrator,
                _("The server slows down with many users"),
                wxString::Format(_("With %d users the server completes %.0f "
                    "transactions per second, half of its best level (%.0f)."),
                    most->clients, tps, peak),
                _("Look at the number of CPU cores, the server mode (SuperServer "
                  "scales with ParallelWorkers and cores, Classic with processes), "
                  "LockHashSlots and lock conflicts; applications should use "
                  "connection pools instead of one connection per user."));
        }
    }

    void checkPageSizes()
    {
        categoryM = BenchmarkCategory::ServerStorage;
        if (m.requestedPageSize > 0 && m.benchmarkDatabase.pageSize > 0
            && m.benchmarkDatabase.pageSize != m.requestedPageSize)
        {
            add(Sev::Info, Side::Configuration, Who::Administrator,
                _("Page size adjusted"),
                wxString::Format(_("Firebird %s does not support a page size "
                    "of %d; the benchmark used %d."), m.serverVersion,
                    m.requestedPageSize, m.benchmarkDatabase.pageSize),
                _("Page sizes of 32768 bytes need Firebird 4 or later."));
        }
        if (m.pageSizes.size() < 2)
            return;
        // relative cost of every page size compared with the best one in
        // writes, reads and storage I/O; lower is better
        auto best = [this](auto value)
        {
            double b = 0.0;
            for (const auto& p : m.pageSizes)
            {
                double v = value(p);
                if (v > 0.0 && (b == 0.0 || v < b))
                    b = v;
            }
            return b;
        };
        double bestInsert = best([](const BenchmarkPageSizeResult& p) { return p.insertMs; });
        double bestScan = best([](const BenchmarkPageSizeResult& p) { return p.warmScanMs; });
        double bestDrive = best([](const BenchmarkPageSizeResult& p)
            { return p.smallCacheInsertMs; });
        const BenchmarkPageSizeResult* winner = nullptr;
        double winnerScore = 0.0;
        wxString table;
        for (const auto& p : m.pageSizes)
        {
            double score = 0.0;
            if (bestInsert > 0.0)
                score += p.insertMs / bestInsert;
            if (bestScan > 0.0)
                score += p.warmScanMs / bestScan;
            if (bestDrive > 0.0)
                score += p.smallCacheInsertMs / bestDrive;
            if (!winner || score < winnerScore)
            {
                winner = &p;
                winnerScore = score;
            }
            // the time limited drive test as a rate, not extrapolated time
            table += wxString::Format(_("%d: insert %s, scan %s, minimal "
                "cache insert %.0f rows/s; "), p.pageSize, ms(p.insertMs),
                ms(p.warmScanMs), p.smallCacheInsertMs > 0.0
                    ? s.diskRows * 1000.0 / p.smallCacheInsertMs : 0.0);
        }
        wxString current = m.inspectedDatabase
            ? wxString::Format(_(" The selected database uses %d."),
                m.inspectedDatabase->pageSize)
            : wxString();
        add(Sev::Info, Side::Configuration, Who::Administrator,
            wxString::Format(_("Page size %d performs best on this server"),
                winner->pageSize),
            table + current,
            _("The comparison weighs writes, cached reads and storage I/O "
              "equally. Workloads dominated by reports and large scans "
              "profit from larger pages, workloads with many small updates "
              "from smaller ones. The page size of an existing database can "
              "only be changed with a gbak backup and restore "
              "(-page_size)."));
    }

    void checkOltp()
    {
        categoryM = BenchmarkCategory::MultiUser;
        if (m.oltp.size() < 2)
            return;
        auto tps = [](const BenchmarkOltpResult& o)
        {
            return o.seconds > 0.0 ? o.transactions / o.seconds : 0.0;
        };
        const BenchmarkOltpResult& first = m.oltp.front();
        const BenchmarkOltpResult& last = m.oltp.back();
        double base = tps(first);
        if (base > 0.0)
        {
            double scaling = tps(last) / base;
            double ideal = double(last.clients) / first.clients;
            if (scaling < 0.4 * ideal)
            {
                // every client waits only for its own round trips, so the
                // limit is almost always on the server
                add(scaling < 1.5 ? Sev::Warning : Sev::Info, Side::Server,
                    Who::Administrator,
                    _("More users do not get more work done"),
                    wxString::Format(_("%d clients reach %.0f transactions/s, "
                        "%d client %.0f transactions/s (%.1fx)."),
                        last.clients, tps(last), first.clients, base, scaling),
                    _("Compare with the CPU scaling and commit results: "
                      "limited cores or slow commits stop the scaling. For "
                      "Classic server check LockHashSlots (e.g. 30011) and "
                      "consider SuperServer.")
                        + (remote() ? " " + _("On a client run, also check "
                            "the utilisation of the network link.")
                                    : wxString()));
            }
        }
        int64_t transactions = 0;
        int64_t conflicts = 0;
        for (const auto& o : m.oltp)
        {
            transactions += o.transactions;
            conflicts += o.conflicts;
        }
        if (transactions > 0 && conflicts * 100 > transactions)
        {
            add(Sev::Info, Side::Server, Who::Developer,
                _("Users get in each other's way (update conflicts)"),
                wxString::Format(_("%lld of %lld transactions failed with a "
                    "conflict."), (long long)conflicts, (long long)transactions),
                _("Keep write transactions short, update rows in a fixed "
                  "order, avoid frequently updated counter rows (use "
                  "sequences instead) and handle lock conflicts in the "
                  "application."));
        }
    }
};

wxString yesNo(bool value)
{
    return value ? _("yes") : _("no");
}

} // namespace

std::vector<BenchmarkFinding> analyzeBenchmark(const BenchmarkMetrics& m)
{
    return Analyzer(m).run();
}

std::vector<BenchmarkConfigCheck> buildBenchmarkChecklist(const BenchmarkMetrics& m)
{
    std::vector<BenchmarkConfigCheck> c;
    auto config = [&m](const wxString& name) -> wxString
    {
        auto it = m.serverConfig.find(name);
        return it == m.serverConfig.end() ? wxString() : it->second;
    };
    auto add = [&c](const wxString& scope, const wxString& setting,
        const wxString& current, const wxString& recommended,
        const wxString& reason, bool needsChange)
    {
        c.push_back({ scope, setting, current.empty() ? wxString("?") : current,
            recommended, reason, needsChange });
    };
    auto toLong = [](const wxString& value) -> long
    {
        long v = 0;
        return value.ToLong(&v) ? v : -1;
    };

    wxString mode = config("ServerMode");
    bool classic = mode.CmpNoCase("Classic") == 0
        || mode.CmpNoCase("SuperClassic") == 0;
    if (mode.empty() && m.serverProcesses)
    {
        classic = *m.serverProcesses > 1;
        mode = classic ? wxString("Classic") : wxString("Super / SuperClassic");
    }
    const int pageSize = m.inspectedDatabase && m.inspectedDatabase->pageSize > 0
        ? m.inspectedDatabase->pageSize
        : (m.benchmarkDatabase.pageSize > 0 ? m.benchmarkDatabase.pageSize : 8192);
    const wxString conf = "firebird.conf";

    add(conf, "ServerMode", mode, "Super",
        _("SuperServer shares the page cache and uses all cores (Firebird 3+)."),
        classic);

    wxString cache = config("DefaultDbCachePages");
    long cacheValue = toLong(cache);
    wxString cacheRecommended = classic ? wxString("256 - 2048")
        : wxString::Format(_("50000 or more (%.0f MB with %d byte pages) "
            "if the server RAM allows it"), 50000.0 * pageSize / megabyte,
            pageSize);
    add(conf, "DefaultDbCachePages", cache, cacheRecommended,
        classic ? _("Classic keeps one cache per connection.")
                : _("The shared cache of SuperServer should hold the "
                    "frequently used part of the databases; leave enough RAM "
                    "for the operating system file cache."),
        cacheValue >= 0 && (classic ? cacheValue > 4096 : cacheValue < 50000));

    // Firebird 4+ ignores FileSystemCacheThreshold when UseFileSystemCache
    // is set explicitly
    wxString fsCache = config("UseFileSystemCache");
    bool fsCacheExplicit = m.serverConfigExplicit.count("UseFileSystemCache") > 0;
    wxString threshold = config("FileSystemCacheThreshold");
    long thresholdValue = toLong(threshold);
    add(conf, "FileSystemCacheThreshold", threshold,
        fsCacheExplicit ? _("not used (UseFileSystemCache is set)")
                        : _("greater than DefaultDbCachePages"),
        _("Otherwise Firebird bypasses the operating system file cache."),
        !fsCacheExplicit && thresholdValue >= 0 && cacheValue >= 0
            && thresholdValue <= cacheValue);

    add(conf, "UseFileSystemCache", fsCache, "true",
        _("The operating system file cache is a second level cache."),
        !fsCache.empty() && fsCache.CmpNoCase("true") != 0);

    wxString temp = config("TempCacheLimit");
    long tempValue = toLong(temp);
    long tempRecommended = classic ? 67108864 : 1073741824;
    add(conf, "TempCacheLimit", temp,
        classic ? wxString("67108864 (64 MB)") : wxString("1073741824 (1 GB)"),
        _("Sorts and hash joins that fit into this memory do not use "
          "temporary files."),
        tempValue >= 0 && tempValue < tempRecommended);

    add(conf, "TempDirectories", config("TempDirectories"),
        _("fast local SSD, excluded from virus scanning"),
        _("Large sorts are written there."), false);

    wxString mask = config("CpuAffinityMask");
    add(conf, "CpuAffinityMask", mask, "0",
        _("0 lets SuperServer use all CPU cores."),
        !mask.empty() && mask != "0");

    if (classic)
    {
        wxString slots = config("LockHashSlots");
        add(conf, "LockHashSlots", slots, "30011",
            _("A larger prime number shortens the lock manager chains for "
              "many connections."), toLong(slots) >= 0 && toLong(slots) < 30011);
    }

    wxString buffer = config("TcpRemoteBufferSize");
    add(conf, "TcpRemoteBufferSize", buffer, "32767",
        _("Larger network packets for large result sets."),
        !buffer.empty() && buffer != "32767");

    add(conf, "WireCompression", config("WireCompression"),
        _("true for slow or WAN connections, otherwise false"),
        _("Compression saves bandwidth and costs CPU; it must also be "
          "enabled on the client."), false);

    if (m.serverMajorVersion >= 5)
    {
        wxString maxWorkers = config("MaxParallelWorkers");
        add(conf, "MaxParallelWorkers", maxWorkers,
            _("number of CPU cores of the server"),
            _("Firebird 5 uses parallel workers for sweep, gbak backup and "
              "restore and index creation; the default 1 uses one core."),
            toLong(maxWorkers) == 1);
        add(conf, "ParallelWorkers", config("ParallelWorkers"),
            _("1 to half of MaxParallelWorkers"),
            _("Default number of workers for a connection that does not "
              "request a number itself."), false);
        add(conf, "MaxStatementCacheSize", config("MaxStatementCacheSize"),
            _("2 MB (default); more for applications with many statements"),
            _("Cache of compiled statements per connection (Firebird 5)."),
            false);
    }

    if (m.inspectedDatabase)
    {
        const BenchmarkDatabaseInfo& db = *m.inspectedDatabase;
        const wxString scope = _("database");
        add(scope, _("Forced writes"), yesNo(db.forcedWrites), _("yes"),
            _("Protects the database against corruption on power loss."),
            !db.forcedWrites);
        add(scope, _("Page size"), wxString::Format("%d", db.pageSize),
            "8192 - 16384",
            _("Larger pages give flatter indexes; change with gbak restore."),
            db.pageSize < 8192);
        add(scope, _("Page buffers (gfix -buffers)"),
            wxString::Format("%d", db.pageBuffers),
            _("0 = use DefaultDbCachePages, or a larger explicit value"),
            _("A value in the database header overrides the server default."),
            false);
        add(scope, _("Sweep interval"), wxString::Format("%d", db.sweepInterval),
            _("20000 (default) or 0 with a scheduled sweep at night"),
            _("Automatic sweeps can start during working hours."), false);
    }
    if (m.serverMajorVersion >= 3 && !classic)
    {
        wxString linger;
        if (m.inspectedDatabase && m.inspectedDatabase->lingerSeconds)
            linger = wxString::Format("%d", *m.inspectedDatabase->lingerSeconds);
        add(_("database"), "LINGER", linger,
            _("e.g. 60 seconds (ALTER DATABASE SET LINGER TO 60)"),
            _("Keeps the database and its cache open for a while after the "
              "last connection closed; helps applications that connect "
              "often."), false);
    }

    // operating system of the server
    const wxString serverOs = m.serverPlatform.empty()
        ? wxString(_("server OS")) : m.serverPlatform + _(" (server)");
    if (m.serverPlatform == "Windows")
    {
        add(serverOs, _("Power plan"), wxEmptyString, _("High performance"),
            _("Balanced power plans lower the CPU clock between requests."),
            false);
        add(serverOs, _("Virus scanner exclusions"), wxEmptyString,
            _("database and temp directories, *.fdb, firebird.exe"),
            _("On-access scanning slows down every page write."), false);
        add(serverOs, _("Server roles"), wxEmptyString,
            _("databases not on the Active Directory disk"),
            _("On a domain controller Windows disables the write cache of "
              "the disk holding the Active Directory database."), false);
    }
    else if (m.serverPlatform == "Linux")
    {
        add(serverOs, _("CPU frequency governor"), wxEmptyString, "performance",
            _("Power saving governors lower the CPU clock between requests."),
            false);
        add(serverOs, _("Open files limit of the firebird user"), wxEmptyString,
            "65535",
            _("Firebird needs several file handles per connection."), false);
        add(serverOs, _("Mount options of the database file system"),
            wxEmptyString, "noatime",
            _("Avoids a metadata write on every read."), false);
        add(serverOs, "vm.swappiness", wxEmptyString, "10",
            _("Keeps the database cache in RAM."), false);
    }
    add(serverOs, _("Virtual machine"), wxEmptyString,
        _("reserved RAM and CPU, no memory overcommit, no I/O limits"),
        _("Overcommitted hosts cause unpredictable pauses."), false);

    // operating system of the client
    if (m.runLocation == BenchmarkRunLocation::Client)
    {
        const wxString clientOs = m.clientOsFamily.empty()
            ? wxString(_("client OS")) : m.clientOsFamily + _(" (client)");
        add(clientOs, _("Power plan / governor"), wxEmptyString,
            _("High performance"),
            _("Interactive applications suffer most from power saving."),
            false);
        add(clientOs, _("Network adapter"), wxEmptyString,
            _("wired, Energy Efficient Ethernet off"),
            _("WLAN and power saving of the adapter add latency."), false);
        add(clientOs, _("Firebird client library"), m.clientVersion,
            m.serverMajorVersion > 0
                ? wxString::Format(_("version %d"), m.serverMajorVersion)
                : _("version of the server"),
            _("The client library should match the server version."),
            false);
    }
    return c;
}

} // namespace fr
