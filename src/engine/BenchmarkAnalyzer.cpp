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

// What could slow down each part of the system. A single run is not
// judged, because whether a value is fast or slow depends on the
// environment. The hints only name what FlameRobin can see in the run
// itself: the state of the machines and of Firebird (power saving, virus
// scanners, the page cache), or a value that is much slower than a related
// value of the same run (a short calculation after a pause compared with
// the same calculation back to back). When two runs are compared, the
// hints of the slower run are shown for the parts that differ clearly,
// together with the typical causes of the slower values. The advice follows
// the Firebird documentation and the comments in firebird.conf.

namespace fr
{

namespace
{

const double megabyte = 1024.0 * 1024.0;
const double gigabyte = 1024.0 * megabyte;

using C = BenchmarkCategory;

class HintFinder
{
public:
    explicit HintFinder(const BenchmarkMetrics& metrics)
        : m(metrics), s(metrics.settings)
    {
    }

    std::vector<BenchmarkHint> run()
    {
        checkPower();
        checkVirusScanners();
        checkProcessor();
        checkStorage();
        checkCache();
        checkNetwork();
        checkManyUsers();
        return hints;
    }

private:
    const BenchmarkMetrics& m;
    const BenchmarkSettings& s;
    std::vector<BenchmarkHint> hints;

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

    void add(C category, const wxString& title, const wxString& detail,
        const wxString& advice)
    {
        hints.push_back({ category, title, detail, advice });
    }

    // a hint about this machine; on the server it is the server as well
    void addLocal(const wxString& title, const wxString& detail, const wxString& advice)
    {
        add(C::LocalMachine, title, detail, advice);
        if (onServer())
            add(C::ServerCpu, title, detail, advice);
    }

    static wxString ms(double value)
    {
        return formatBenchmarkMilliseconds(value);
    }

    // the operating system of this machine
    wxString localOs() const
    {
        return onServer() ? m.serverPlatform : m.clientOsFamily;
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
    wxString localPowerAdvice() const
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
            return wxString::Format(_("Current setting: %s."), plan) + " " + powerAdvice(localOs());
        return unverified(powerAdvice(localOs()));
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

    static wxString join(const std::vector<wxString>& list)
    {
        wxString text;
        for (const auto& item : list)
            text += (text.empty() ? wxString() : wxString(", ")) + item;
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

    static double ratioOf(const std::optional<double>& slow,
        const std::optional<double>& fast)
    {
        if (!slow || !fast || *fast <= 0.0)
            return 0.0;
        return *slow / *fast;
    }

    void checkPower()
    {
        if (m.localOnBattery.value_or(false))
        {
            addLocal(_("This computer runs on battery"),
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

        // the same short calculation after a pause and back to back
        const double local = ratioOf(m.clientBurstIdleMs, m.clientBurstHotMs);
        if (local >= 1.4)
        {
            addLocal(onServer() ? _("Power saving slows down the server")
                                : _("Power saving slows down this computer"),
                wxString::Format(_("A short CPU burst after an idle pause takes "
                    "%.1fx as long as the same burst back to back; the CPU "
                    "clocks down when idle."), local),
                localPowerAdvice());
        }
        else if (powerAlreadyMaximum().has_value() && !*powerAlreadyMaximum())
        {
            addLocal(_("The power plan saves energy"),
                wxString::Format(_("Current setting: %s."),
                    wxString::FromUTF8(m.system.powerPlan.c_str())),
                powerAdvice(localOs()));
        }

        const double server = ratioOf(m.serverBurstIdleMs, m.serverBurstHotMs);
        if (server >= 1.4 && m.serverBurstHotMs && *m.serverBurstHotMs >= 10.0
            && !(onServer() && local >= 1.4))
        {
            add(C::ServerCpu, _("Power saving slows down the server"),
                wxString::Format(_("Server-side CPU bursts after an idle pause "
                    "take %.1fx as long as back to back."), server),
                onServer() ? localPowerAdvice() : unverified(powerAdvice(m.serverPlatform)));
        }

        if (m.latencySparseMs && m.latencyMedianMs && *m.latencyMedianMs > 0.0)
        {
            const double ratio = *m.latencySparseMs / *m.latencyMedianMs;
            // a fraction of a millisecond is noticeable only for very
            // chatty applications
            if (ratio >= 2.0 && *m.latencySparseMs - *m.latencyMedianMs >= 0.3)
            {
                add(C::Network, _("The first request after a short pause is slow"),
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

    void checkVirusScanners()
    {
        const std::vector<wxString> scanners = activeScanners();
        if (scanners.empty())
            return;
        wxString detail = wxString::Format(_("Active virus scanner: %s."), join(scanners));
        if (m.tempDirFileMs)
        {
            detail += " " + wxString::Format(_("Creating, writing and deleting a "
                "small file in the temporary directory takes %s."), ms(*m.tempDirFileMs));
        }
        add(C::LocalMachine, _("A virus scanner checks the files of this computer"),
            detail, onServer() ? antivirusAdvice(localOs())
                : _("Exclude the application directories and temporary files of "
                    "the database applications from on-access scanning on the "
                    "client."));
        if (!onServer())
            return;

        // the database directory of this server
        const std::optional<bool>& excluded = m.system.storagePathExcluded;
        if (excluded && *excluded)
            return;
        wxString databaseDetail = wxString::Format(_("Active virus scanner: %s."),
            join(scanners));
        if (m.databaseDirFileMs)
        {
            databaseDetail += " " + wxString::Format(_("Creating, writing and deleting "
                "a small file in the database directory takes %s."),
                ms(*m.databaseDirFileMs));
        }
        add(C::ServerStorage, _("A virus scanner may check the database files"),
            databaseDetail, antivirusAdvice(m.serverPlatform) + " " + (excluded
                ? _("The database directory is not excluded from Microsoft Defender "
                    "yet.")
                : _("FlameRobin can only read the exclusions with administrator "
                    "rights.")));
    }

    void checkProcessor()
    {
        if (m.cpuMs && m.parallelCpuMs && *m.parallelCpuMs > 0.0
            && s.parallelConnections > 1)
        {
            const double scaling = *m.cpuMs * s.parallelConnections / *m.parallelCpuMs;
            if (scaling < 0.5 * s.parallelConnections)
            {
                const wxString mask = configValue("CpuAffinityMask");
                const bool masked = !mask.empty() && mask != "0";
                add(C::ServerCpu, _("The server does not use all its processor cores"),
                    wxString::Format(_("%d connections working in parallel "
                        "are only %.1fx as fast as one connection."),
                        s.parallelConnections, scaling),
                    masked ? wxString::Format(_("CpuAffinityMask = %s restricts "
                        "Firebird to certain cores; set it to 0 to use all "
                        "cores."), mask)
                    : _("Give the server enough CPU cores (virtual machines "
                        "often have too few vCPUs), check the CPU load caused "
                        "by other processes and CPU limits of the "
                        "virtualization host."));
            }
        }

        const BenchmarkSystemInfo& sys = m.system;
        if (!onServer())
            return;
        if (!sys.virtualization.empty())
        {
            const wxString title = wxString::Format(_("The server is a virtual "
                "machine (%s)"), wxString::FromUTF8(sys.virtualization.c_str()));
            const wxString detail = _("Other virtual machines on the same host share "
                "CPU, storage and network; waiting for a free CPU (CPU ready) and "
                "storage limits do not show inside the machine.");
            const wxString advice = _("Reserve CPU and RAM for the database server, "
                "avoid memory overcommitment, and give its disks a low latency "
                "storage without I/O limits.");
            add(C::ServerCpu, title, detail, advice);
            add(C::ServerStorage, title, detail, advice);
        }
        const int firebirdRunning = int(std::count_if(sys.firebirdServers.begin(),
            sys.firebirdServers.end(), [](const BenchmarkServerProcess& p) { return p.running; }));
        if (firebirdRunning >= 2)
        {
            add(C::ServerCpu, wxString::Format(_("%d Firebird servers run on this "
                "computer"), firebirdRunning),
                _("They share CPU cores, RAM (every SuperServer has its own page "
                  "cache) and the drives; the results of this run depend on what "
                  "the others do."),
                _("Stop instances that are not needed, and size the page caches "
                  "of all instances together for the RAM of the machine."));
        }
        wxString others;
        for (const auto& p : sys.otherDatabaseServers)
        {
            if (p.running)
                others += (others.empty() ? wxString() : wxString(", ")) + wxString::FromUTF8(p.name.c_str());
        }
        if (!others.empty())
        {
            add(C::ServerCpu, _("Other database servers run on this computer"),
                wxString::Format(_("%s share CPU, RAM and drives with Firebird."), others),
                _("Give every server its share of the RAM (e.g. max server memory "
                  "of SQL Server) so that the operating system file cache keeps "
                  "room for Firebird."));
        }
    }

    void checkStorage()
    {
        // the raw synchronous write of the operating system in the same
        // directory: when it is much faster, the disk is not the cause
        if (m.commitMs && m.osSyncWriteMs && *m.osSyncWriteMs < *m.commitMs / 3.0)
        {
            add(C::ServerStorage, _("The disk is fast, saving a change takes longer"),
                wxString::Format(_("Committing a single-row transaction takes %s. "
                    "A plain synchronous write of the operating system in the same "
                    "directory takes only %s, so the storage itself is fast."),
                    ms(*m.commitMs), ms(*m.osSyncWriteMs)),
                antivirusAdvice(m.serverPlatform) + " " + _("Also check the server load "
                    "and whether nbackup is active."));
        }
        if (m.tempSortMs && *m.tempSortMs > 0.0 && m.serverSortMs && *m.serverSortMs > 0.0
            && m.scanBytes > 0 && m.tempSortBytes > 0)
        {
            const double big = m.tempSortBytes / megabyte / (*m.tempSortMs / 1000.0);
            const double small = m.scanBytes / megabyte / (*m.serverSortMs / 1000.0);
            if (big < 0.25 * small)
            {
                const wxString limit = configValue("TempCacheLimit");
                add(C::ServerStorage, _("Sorting large data is much slower than in memory"),
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

        const BenchmarkSystemInfo& sys = m.system;
        if (sys.storagePath.empty())
            return;
        if (sys.storageHealthWarning.value_or(false))
        {
            wxString values;
            for (const auto& v : sys.storageHealth)
            {
                if (v.warning)
                {
                    values += (values.empty() ? wxString() : wxString(", "))
                        + wxString::FromUTF8(v.name.c_str()) + ": "
                        + wxString::FromUTF8(v.value.c_str());
                }
            }
            add(C::ServerStorage, _("The disk of the database reports problems"),
                wxString::Format(_("The health values (SMART) of %s are outside "
                    "the healthy range: %s."), wxString::FromUTF8(
                    sys.storageModel.c_str()), values),
                _("Make a backup now and plan to replace the drive. Check the "
                  "full SMART data with the tools of the manufacturer."));
        }
        if (sys.storageMedia == "HDD")
        {
            add(C::ServerStorage, _("The database is on a hard disk"),
                _("Hard disks need milliseconds for every random access; "
                  "commits, index lookups and garbage collection wait for them."),
                _("Move the database to an SSD (NVMe if possible)."));
        }
        if (sys.writeCacheEnabled && !*sys.writeCacheEnabled
            && !sys.writeCachePowerProtected.value_or(false))
        {
            add(C::ServerStorage, _("The write cache of the disk is off"),
                _("Every write goes directly to the medium, which makes commits "
                  "with forced writes slow."),
                _("Turn the write cache on (device manager, policies of the "
                  "drive) if the drive or controller has power loss "
                  "protection."));
        }
        if (m.databaseDiskFreeBytes)
        {
            const double freeBytes = double(*m.databaseDiskFreeBytes);
            if (freeBytes < 2.0 * gigabyte)
            {
                add(C::ServerStorage, _("Little free disk space"),
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
        const BenchmarkDatabaseInfo& db = m.benchmarkDatabase;
        if (classicMode())
        {
            const wxString title = wxString::Format(_("Server mode %s"), serverMode());
            const wxString detail = _("Every connection has its own small page "
                "cache, so repeated queries profit less from caching.");
            const wxString advice = _("Since Firebird 3, SuperServer shares one "
                "large cache between all connections and uses all CPU cores; it is "
                "the better choice for most workloads.");
            add(C::ServerCache, title, detail, advice);
            add(C::MultiUser, title, detail, advice);
        }
        else if (db.pageSize > 0 && db.cachePages > 0
            && double(db.cachePages) * db.pageSize < double(m.scanBytes))
        {
            add(C::ServerCache, _("Firebird keeps little data in memory (small page cache)"),
                wxString::Format(_("The page cache holds %.0f MB, the "
                    "test data alone has %.0f MB. Data that does not fit "
                    "is read from the operating system cache or the disk "
                    "again."), double(db.cachePages) * db.pageSize / megabyte,
                    double(m.scanBytes) / megabyte),
                _("For SuperServer increase DefaultDbCachePages in "
                  "firebird.conf (e.g. 50000 pages) or the page buffers of "
                  "the database (gfix -buffers), keep UseFileSystemCache = "
                  "true and FileSystemCacheThreshold above the cache size."));
        }

        if (m.coldScanMs && m.warmScanMs && *m.warmScanMs > 0.0
            && *m.coldScanMs >= 5.0 * *m.warmScanMs && *m.coldScanMs >= 500.0)
        {
            add(C::ServerCache, _("Data that is not in memory is read much more slowly"),
                wxString::Format(_("The first scan took %s, the repeated "
                    "scan %s. Workloads that do not fit into the caches "
                    "will be limited by the storage."),
                    ms(*m.coldScanMs), ms(*m.warmScanMs)),
                _("Give the server enough RAM for the Firebird page cache "
                  "and the operating system file cache (as a rule of thumb "
                  "the frequently used part of the database), or use faster "
                  "storage."));
        }

        if (m.serverReadMs && m.serverSortMs && *m.serverReadMs > 0.0
            && *m.serverSortMs >= 6.0 * *m.serverReadMs && *m.serverSortMs >= 300.0)
        {
            wxString advice = _("Sorts that do not fit into memory are written "
                "to temporary files. Increase TempCacheLimit (SuperServer e.g. "
                "1 GB), place TempDirectories on fast local storage and exclude "
                "it from virus scanning.");
            const wxString limit = configValue("TempCacheLimit");
            if (!limit.empty())
                advice += " " + wxString::Format(_("Current TempCacheLimit: %s."), limit);
            add(C::ServerCache, _("Sorting takes much longer than reading"),
                wxString::Format(_("Sorting the test data takes %.1fx as long "
                    "as reading it."), *m.serverSortMs / *m.serverReadMs),
                advice);
        }

        const BenchmarkSystemInfo& sys = m.system;
        if (onServer() && sys.memoryTotalBytes > 0 && sys.memoryAvailableBytes > 0
            && double(sys.memoryAvailableBytes) < 0.1 * double(sys.memoryTotalBytes))
        {
            add(C::ServerCache, _("Little free memory (RAM)"),
                wxString::Format(_("Only %.1f of %.1f GB are free; the operating "
                    "system has little room for its file cache."),
                    sys.memoryAvailableBytes / gigabyte, sys.memoryTotalBytes / gigabyte),
                _("Check which processes use the memory, and size the page caches "
                  "so that the operating system keeps some for its file cache."));
        }
    }

    void checkNetwork()
    {
        // an old client library lacks the faster protocol of the server
        int clientMajor = 0;
        const int pos = m.clientVersion.Find("-V");
        long value = 0;
        if (pos != wxNOT_FOUND && m.clientVersion.Mid(pos + 2).BeforeFirst('.').ToLong(&value))
            clientMajor = int(value);
        if (clientMajor > 0 && clientMajor < m.serverMajorVersion
            && m.connectionKind != BenchmarkConnectionKind::Embedded)
        {
            add(C::Network, _("FlameRobin uses an old client library"),
                wxString::Format(_("The client library is version %d, the "
                    "server version %d."), clientMajor, m.serverMajorVersion),
                _("Ship the client library of the server version with the "
                  "application; newer clients support faster protocol "
                  "features of the server (e.g. wire compression, Batch "
                  "API)."));
        }

        if (m.streamMs && m.streamBytes > 0 && *m.streamMs > 0.0)
        {
            const double clientMs = m.streamClientMs.value_or(0.0);
            if (clientMs >= 0.5 * *m.streamMs && *m.streamMs >= 200.0)
            {
                add(C::LocalMachine, _("FlameRobin needs most of the time for the received data"),
                    wxString::Format(_("%s of %s are spent in FlameRobin itself."),
                        ms(clientMs), ms(*m.streamMs)),
                    _("Check the CPU load of this machine.") + " " + powerAdvice(localOs()));
            }
        }

        if (m.latencyP95Ms && m.latencyMedianMs && *m.latencyMedianMs > 0.0
            && *m.latencyP95Ms >= 3.0 * *m.latencyMedianMs
            && *m.latencyP95Ms - *m.latencyMedianMs >= 0.5)
        {
            add(C::Network, _("Response times vary a lot"),
                wxString::Format(_("5%% of the round trips take %s or more "
                    "while the median is %s."), ms(*m.latencyP95Ms), ms(*m.latencyMedianMs)),
                _("Typical for WLAN, overloaded network links or an "
                  "overloaded server. Check the network utilisation and "
                  "the CPU load of the server and the client."));
        }

        if (!remote())
            return;
        const BenchmarkNetworkPath p = m.networkPath.value_or(BenchmarkNetworkPath());
        if (p.adapterType == "Wi-Fi" || p.adapterType == "VPN")
        {
            add(C::Network, p.adapterType == "VPN" ? _("The connection runs through a VPN")
                                                   : _("The connection runs over Wi-Fi"),
                wxString::Format(_("This machine reaches the server through %s (%s). "
                    "It adds latency and fluctuations to every round trip."),
                    wxString::FromUTF8(p.adapterName.c_str()),
                    wxString::FromUTF8(p.adapterDescription.c_str())),
                _("For comparisons and for data-heavy work use a wired network "
                  "connection."));
        }
        // NAT, VPN or a proxy between client and server
        if (!m.remoteAddress.empty() && !m.system.addresses.empty())
        {
            const wxString seen = m.remoteAddress.BeforeLast('/').empty()
                ? m.remoteAddress : m.remoteAddress.BeforeLast('/');
            const bool own = std::any_of(m.system.addresses.begin(), m.system.addresses.end(),
                [&seen](const BenchmarkNetworkAddress& a)
                { return seen.Contains(wxString::FromUTF8(a.address.c_str())); });
            if (!own)
            {
                add(C::Network, _("A router or VPN sits between this computer and the server"),
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
            add(C::Network, _("Logging in takes most of the connection time"),
                wxString::Format(_("The network part of a connection takes %s, the "
                    "complete connect %s."), ms(handshake), ms(*m.connectMs)),
                _("Look at the authentication (order of AuthServer, Win_Sspi with "
                  "slow domain controllers, the SRP cost), the security database "
                  "and reverse DNS lookups of the server."));
        }
        if (m.latencyOutliers > 0)
        {
            add(C::Network, _("Network packets get lost"),
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
                wxString advice = _("The round trips of the fetches, wire encryption "
                    "or compression, network inspection by firewalls or virus "
                    "scanners and VPNs limit the transfer; a larger "
                    "TcpRemoteBufferSize helps on links with latency.");
                const wxString bufferSize = configValue("TcpRemoteBufferSize");
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
                add(C::Network, _("Large results use only part of the network speed"),
                    wxString::Format(_("The transfer reaches %.1f MB/s on a %s "
                        "link (about %.0f MB/s)."), mbPerSecond,
                        formatBenchmarkBitRate(double(p.linkSpeedBitsPerSecond)), linkMB),
                    advice);
            }
        }
    }

    void checkManyUsers()
    {
        auto tps = [](const BenchmarkOltpResult& o)
        {
            return o.seconds > 0.0 ? o.transactions / o.seconds : 0.0;
        };
        if (m.oltp.size() >= 2 && tps(m.oltp.front()) > 0.0)
        {
            const BenchmarkOltpResult& first = m.oltp.front();
            const BenchmarkOltpResult& last = m.oltp.back();
            const double scaling = tps(last) / tps(first);
            const double ideal = double(last.clients) / first.clients;
            if (scaling < 0.4 * ideal)
            {
                // every client waits only for its own round trips, so the
                // limit is almost always on the server
                add(C::MultiUser, _("More users do not get more work done"),
                    wxString::Format(_("%d clients reach %.0f transactions/s, "
                        "%d client %.0f transactions/s (%.1fx)."),
                        last.clients, tps(last), first.clients, tps(first), scaling),
                    _("Compare with the CPU scaling and commit results: "
                      "limited cores or slow commits stop the scaling. For "
                      "Classic server check LockHashSlots (e.g. 30011) and "
                      "consider SuperServer.")
                        + (remote() ? " " + _("On a client run, also check "
                            "the utilisation of the network link.") : wxString()));
            }
        }
        double peak = 0.0;
        int64_t transactions = 0, conflicts = 0;
        for (const auto& o : m.oltp)
        {
            peak = std::max(peak, tps(o));
            transactions += o.transactions;
            conflicts += o.conflicts;
        }
        const BenchmarkOltpResult* most = m.oltp.empty() ? nullptr : &m.oltp.back();
        if (most && most->clients >= 50 && peak > 0.0 && tps(*most) < 0.5 * peak)
        {
            add(C::MultiUser, _("The server slows down with many users"),
                wxString::Format(_("With %d users the server completes %.0f "
                    "transactions per second, half of its best level (%.0f)."),
                    most->clients, tps(*most), peak),
                _("Look at the number of CPU cores, the server mode (SuperServer "
                  "scales with ParallelWorkers and cores, Classic with processes), "
                  "LockHashSlots and lock conflicts; applications should use "
                  "connection pools instead of one connection per user."));
        }
        if (transactions > 0 && conflicts * 100 > transactions)
        {
            add(C::MultiUser, _("Users get in each other's way (update conflicts)"),
                wxString::Format(_("%lld of %lld transactions failed with a "
                    "conflict."), (long long)conflicts, (long long)transactions),
                _("Keep write transactions short, update rows in a fixed "
                  "order, avoid frequently updated counter rows (use "
                  "sequences instead) and handle lock conflicts in the "
                  "application."));
        }
    }
};

} // namespace

std::vector<BenchmarkHint> findBenchmarkHints(const BenchmarkMetrics& m)
{
    return HintFinder(m).run();
}

wxString getBenchmarkValueAdvice(const wxString& key)
{
    static const std::pair<const char*, const char*> advice[] = {
        { "cpuIterationsPerMs", wxTRANSLATE("A slower or busier processor: check the "
            "CPU load of the server, power saving, and on a virtual machine the CPU "
            "reservation of the host. Firebird runs one query on one core, so the "
            "speed of a single core matters most.") },
        { "parallelIterationsPerMs", wxTRANSLATE("Fewer processor cores for Firebird: "
            "a virtual machine with few vCPUs, CpuAffinityMask in firebird.conf, other "
            "programs that load the server, or CPU limits of the host.") },
        { "commitMs", wxTRANSLATE("Every saved change waits until the disk confirms "
            "it. A slower disk, a write cache that is off, a virus scanner on the "
            "database files or the storage of a virtual machine host make it "
            "slower.") },
        { "insertRowsPerSecond", wxTRANSLATE("Writing depends on the disk, the page "
            "cache and the load of the server; virus scanners on the database files "
            "slow it down as well.") },
        { "driveOperationsPerSecond", wxTRANSLATE("This value shows the disk itself: an "
            "SSD instead of a hard disk, RAID 10 instead of RAID 5 or 6, and the "
            "storage limits of a virtual machine host make the difference.") },
        { "parallelDriveRowsPerSecond", wxTRANSLATE("The disk under load from several "
            "users: the RAID level, the cache of the controller and the I/O limits "
            "of a virtual machine host make the difference.") },
        { "tempSortMBPerSecond", wxTRANSLATE("Large sorts are written to temporary "
            "files. A larger TempCacheLimit in firebird.conf keeps more of them in "
            "memory; TempDirectories should be on a fast local disk without virus "
            "scanning.") },
        { "scanMBPerSecond", wxTRANSLATE("Reading cached data depends on the speed of "
            "the processor and the memory, and on the size of the page cache "
            "(DefaultDbCachePages in firebird.conf).") },
        { "sortMBPerSecond", wxTRANSLATE("Sorting depends on the speed of the "
            "processor and on the sort memory (TempCacheLimit in firebird.conf).") },
        { "roundTripMs", wxTRANSLATE("Every request waits for the network. Wi-Fi, a "
            "VPN, more network devices on the way, power saving of the network "
            "adapters and security software that inspects the traffic make it "
            "slower.") },
        { "transferMBPerSecond", wxTRANSLATE("Large results depend on the bandwidth of "
            "the network, on wire encryption and compression, and on "
            "TcpRemoteBufferSize in firebird.conf.") },
        { "connectMs", wxTRANSLATE("Connecting includes the login: slow name "
            "resolution (DNS), the order of the authentication plugins, a slow "
            "security database, and firewalls or virus scanners that inspect new "
            "connections.") },
        { "wideInsertsPerSecond", wxTRANSLATE("Every row waits for the network, so the "
            "response time and lost packets decide this value. Applications should "
            "send rows in batches.") },
        { "oltpTransactionsPerSecond", wxTRANSLATE("Many users need processor cores and "
            "fast commits; few cores, slow commits or the Classic server mode limit "
            "it, on a client run also the network.") },
        { "clientCpuBurstMs", wxTRANSLATE("The computer running FlameRobin calculates "
            "more slowly: power saving, the battery, or other programs that load its "
            "processor.") }
    };
    for (const auto& a : advice)
    {
        if (key == a.first)
            return wxGetTranslation(a.second);
    }
    return wxEmptyString;
}

std::vector<BenchmarkSetting> getBenchmarkSettings(const BenchmarkMetrics& m)
{
    std::vector<BenchmarkSetting> list;
    auto add = [&list](const wxString& scope, const wxString& name,
        const wxString& value, const wxString& meaning)
    {
        list.push_back({ scope, name, value.empty() ? wxString("?") : value, meaning });
    };
    auto config = [&m](const wxString& name) -> wxString
    {
        auto it = m.serverConfig.find(name);
        return it == m.serverConfig.end() ? wxString() : it->second;
    };

    const wxString conf = "firebird.conf";
    add(conf, "ServerMode", config("ServerMode"),
        _("Super shares one page cache between all connections and uses all "
          "cores; Classic and SuperClassic keep a cache per connection."));
    add(conf, "DefaultDbCachePages", config("DefaultDbCachePages"),
        _("The number of database pages Firebird keeps in memory, per database "
          "(Super) or per connection (Classic)."));
    add(conf, "UseFileSystemCache", config("UseFileSystemCache"),
        _("Whether Firebird uses the file cache of the operating system as a "
          "second cache."));
    add(conf, "FileSystemCacheThreshold", config("FileSystemCacheThreshold"),
        _("Up to this page cache size Firebird uses the file cache of the "
          "operating system as well; not used when UseFileSystemCache is set."));
    add(conf, "TempCacheLimit", config("TempCacheLimit"),
        _("Memory for sorts and hash joins; larger sorts are written to "
          "temporary files."));
    add(conf, "TempDirectories", config("TempDirectories"),
        _("Where large sorts are written."));
    add(conf, "CpuAffinityMask", config("CpuAffinityMask"),
        _("The processor cores Firebird may use; 0 means all."));
    add(conf, "LockHashSlots", config("LockHashSlots"),
        _("The size of the lock table; matters for many connections in Classic "
          "mode."));
    add(conf, "TcpRemoteBufferSize", config("TcpRemoteBufferSize"),
        _("The size of the network packets for results."));
    add(conf, "WireCompression", config("WireCompression"),
        _("Compresses the network traffic; saves bandwidth and costs processor "
          "time, also on the client."));
    add(conf, "RemoteAuxPort", config("RemoteAuxPort"),
        _("The port for events (POST_EVENT); 0 means a random port."));
    if (m.serverMajorVersion >= 5)
    {
        add(conf, "MaxParallelWorkers", config("MaxParallelWorkers"),
            _("The most workers Firebird uses in parallel for sweep, backup, "
              "restore and index creation."));
        add(conf, "ParallelWorkers", config("ParallelWorkers"),
            _("The number of workers for a connection that does not ask for a "
              "number itself."));
        add(conf, "MaxStatementCacheSize", config("MaxStatementCacheSize"),
            _("The cache of compiled statements per connection."));
    }

    if (m.inspectedDatabase)
    {
        const BenchmarkDatabaseInfo& db = *m.inspectedDatabase;
        const wxString scope = _("database");
        add(scope, _("Forced writes"), db.forcedWrites ? _("on") : _("off"),
            _("Every saved change is written to the disk at once; protects the "
              "database against corruption on power loss."));
        add(scope, _("Page size"), wxString::Format("%d", db.pageSize),
            _("Larger pages give flatter indexes; it can only be changed with a "
              "backup and restore."));
        add(scope, _("Page buffers (gfix -buffers)"), wxString::Format("%d", db.pageBuffers),
            _("A value in the database header replaces DefaultDbCachePages; 0 uses "
              "the setting of the server."));
        add(scope, _("Sweep interval"), wxString::Format("%d", db.sweepInterval),
            _("After how many transactions Firebird removes old record versions "
              "automatically; 0 switches the automatic sweep off."));
        if (db.lingerSeconds)
        {
            add(scope, "LINGER", wxString::Format("%d", *db.lingerSeconds),
                _("How many seconds the database stays open after the last "
                  "connection closed."));
        }
    }
    return list;
}

} // namespace fr
