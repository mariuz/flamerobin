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

// The description of the tested system for the report: a title, the
// overview cards, the complete details, the glossary and the explanations
// of the units. Shared by the HTML and the Markdown report.

namespace fr
{

namespace
{

wxString utf8(const std::string& s)
{
    return wxString::FromUTF8(s.c_str());
}

wxString separator()
{
    return wxString(" ") + wxUniChar(0x00B7) + " ";
}

wxString gigabytes(int64_t bytes)
{
    const double gb = bytes / (1024.0 * 1024.0 * 1024.0);
    return wxString::Format(gb >= 10.0 || gb == std::floor(gb) ? _("%.0f GB") : _("%.1f GB"), gb);
}

wxString yesNo(bool value)
{
    return value ? _("yes") : _("no");
}

wxString megahertz(int mhz)
{
    return wxString::Format(_("%.1f GHz"), mhz / 1000.0);
}

wxString duration(int64_t seconds)
{
    if (seconds >= 86400)
        return wxString::Format(_("%lld days, %lld hours"), (long long)(seconds / 86400),
            (long long)(seconds % 86400 / 3600));
    if (seconds >= 3600)
        return wxString::Format(_("%lld hours, %lld min"), (long long)(seconds / 3600),
            (long long)(seconds % 3600 / 60));
    return wxString::Format(_("%lld min"), (long long)(seconds / 60));
}

bool isServerRun(const BenchmarkMetrics& m)
{
    return m.runLocation == BenchmarkRunLocation::Server
        || m.connectionKind == BenchmarkConnectionKind::Embedded;
}

// IPv4 addresses first, IPv6 only when there is no IPv4 address; the short
// list leaves out virtual switches (Hyper-V, WSL, Docker) when there are
// other adapters
wxString addressList(const BenchmarkSystemInfo& s, bool all)
{
    const bool physical = std::any_of(s.addresses.begin(), s.addresses.end(),
        [](const BenchmarkNetworkAddress& a) { return a.type != "virtual"; });
    wxString v4, v6;
    for (const auto& a : s.addresses)
    {
        if (!all && physical && a.type == "virtual")
            continue;
        wxString& list = a.address.find(':') == std::string::npos ? v4 : v6;
        wxString entry = utf8(a.address);
        if (all)
            entry += " (" + utf8(a.adapter)
                + (a.type.empty() ? wxString() : ", " + utf8(a.type)) + ")";
        list += (list.empty() ? "" : ", ") + entry;
    }
    if (all)
        return v4 + (!v4.empty() && !v6.empty() ? ", " : "") + v6;
    return v4.empty() ? v6 : v4;
}

wxString cpuText(const BenchmarkSystemInfo& s)
{
    // "13th Gen Intel(R) Core(TM) i7-13800H" -> "13th Gen Intel Core i7-13800H"
    wxString name = utf8(s.cpuName);
    for (const char* mark : { "(R)", "(r)", "(TM)", "(tm)" })
        name.Replace(mark, "");
    name.Replace(" CPU @", " @");
    while (name.Replace("  ", " "))
        ;
    return name.Trim().Trim(false);
}

wxString coresText(const BenchmarkSystemInfo& s, int fallbackThreads)
{
    if (s.cpuCores <= 0)
        return fallbackThreads > 0 ? wxString::Format(_("%d logical CPUs"), fallbackThreads)
                                   : wxString();
    wxString text = wxString::Format(_("%d cores"), s.cpuCores);
    if (s.cpuPerformanceCores > 0 && s.cpuEfficiencyCores > 0)
    {
        text += wxString::Format(_(" (%d performance + %d efficiency)"),
            s.cpuPerformanceCores, s.cpuEfficiencyCores);
    }
    if (s.cpuThreads > 0)
        text += wxString::Format(_(", %d threads"), s.cpuThreads);
    if (s.cpuSockets > 1)
        text += wxString::Format(_(", %d sockets"), s.cpuSockets);
    return text;
}

wxString memoryText(const BenchmarkSystemInfo& s)
{
    if (s.memoryTotalBytes <= 0)
        return wxString();
    wxString text = gigabytes(s.memoryTotalBytes);
    if (!s.memoryModules.empty())
    {
        const BenchmarkMemoryModule& first = s.memoryModules.front();
        text += " " + utf8(first.type);
        if (first.configuredSpeedMTs > 0 || first.speedMTs > 0)
        {
            text += wxString::Format(_("-%d"), first.configuredSpeedMTs > 0
                ? first.configuredSpeedMTs : first.speedMTs);
        }
    }
    if (s.memoryAvailableBytes > 0)
        text += wxString::Format(_(", %s free"), gigabytes(s.memoryAvailableBytes));
    return text;
}

wxString storageText(const BenchmarkSystemInfo& s, const BenchmarkMetrics& m)
{
    wxString text = utf8(s.storageModel);
    wxString kind = utf8(s.storageBus);
    if (!s.storageMedia.empty())
        kind += (kind.empty() ? "" : " ") + utf8(s.storageMedia);
    if (!kind.empty())
        text += (text.empty() ? wxString() : separator()) + kind;
    if (m.databaseDiskFreeBytes)
        text += (text.empty() ? wxString() : separator())
            + wxString::Format(_("%s free"), gigabytes(*m.databaseDiskFreeBytes));
    return text;
}

wxString osText(const BenchmarkSystemInfo& s, const wxString& fallback)
{
    if (s.osName.empty())
        return fallback;
    wxString text = utf8(s.osName);
    if (!s.osVersion.empty())
        text += " " + utf8(s.osVersion);
    if (!s.architecture.empty())
        text += ", " + utf8(s.architecture);
    return text;
}

wxString userText(const BenchmarkSystemInfo& s)
{
    if (s.osUser.empty())
        return wxString();
    wxString text = utf8(s.osUser);
    if (s.elevated)
        text += *s.elevated ? _(", administrator rights") : _(", no administrator rights");
    return text;
}

wxString powerText(const BenchmarkMetrics& m)
{
    wxString text;
    if (m.localOnBattery)
        text = *m.localOnBattery ? _("battery") : _("mains");
    if (!m.system.powerPlan.empty())
        text += (text.empty() ? wxString() : separator()) + utf8(m.system.powerPlan);
    return text;
}

wxString firebirdUserText(const BenchmarkMetrics& m)
{
    if (m.firebirdUser.empty())
        return wxString();
    wxString text = m.firebirdUser;
    if (!m.firebirdRole.empty() && m.firebirdRole != "NONE")
        text += wxString::Format(_(", role %s"), m.firebirdRole);
    if (!m.serverConfig.empty())
        text += _(", administrator");
    return text;
}

wxString firebirdText(const BenchmarkMetrics& m)
{
    wxString text = m.serverVersion.empty() ? wxString() : "Firebird " + m.serverVersion;
    auto mode = m.serverConfig.find("ServerMode");
    if (mode != m.serverConfig.end())
        text += " " + (mode->second.CmpNoCase("Super") == 0 ? wxString("SuperServer")
            : mode->second);
    else if (m.serverProcesses)
        text += *m.serverProcesses > 1 ? _(" Classic") : _(" SuperServer or SuperClassic");
    if (!m.serverPlatform.empty())
        text += wxString::Format(_(" on %s"), m.serverPlatform);
    return text;
}

wxString accessText(const BenchmarkMetrics& m)
{
    switch (m.connectionKind)
    {
        case BenchmarkConnectionKind::Embedded:
            return _("embedded engine in FlameRobin");
        case BenchmarkConnectionKind::LocalXnet:
            return _("local XNET connection");
        case BenchmarkConnectionKind::TcpLoopback:
            return wxString::Format(_("local TCP connection, port %d"), m.serverPort);
        case BenchmarkConnectionKind::TcpRemote:
            return wxString::Format(_("TCP/IP, port %d"), m.serverPort);
        default:
            return getBenchmarkConnectionKindName(m.connectionKind);
    }
}

wxString configText(const BenchmarkMetrics& m)
{
    if (m.serverConfig.empty())
    {
        return m.serverMajorVersion >= 4 ? _("not readable (needs administrator rights)")
                                         : _("not readable before Firebird 4");
    }
    return m.serverConfigExplicit.empty() ? _("all values at their default")
        : wxString::Format(_("%d values set in firebird.conf"),
            int(m.serverConfigExplicit.size()));
}

wxString runningServers(const std::vector<BenchmarkServerProcess>& list)
{
    int running = int(std::count_if(list.begin(), list.end(),
        [](const BenchmarkServerProcess& p) { return p.running; }));
    if (list.empty())
        return wxString();
    return wxString::Format(_("%d installed, %d running"), int(list.size()), running);
}

wxString networkAdapterText(const BenchmarkNetworkPath& p)
{
    wxString text = utf8(p.adapterType);
    if (!p.adapterName.empty())
        text += (text.empty() ? "" : " ") + wxString("(") + utf8(p.adapterName) + ")";
    if (p.linkSpeedBitsPerSecond > 0)
        text += ", " + formatBenchmarkBitRate(double(p.linkSpeedBitsPerSecond));
    return text;
}

wxString portsText(const BenchmarkMetrics& m)
{
    wxString open, closed, filtered;
    for (const auto& c : m.serverPorts)
    {
        wxString& list = c.state == BenchmarkPortCheck::State::Open ? open
            : c.state == BenchmarkPortCheck::State::Closed ? closed : filtered;
        list += (list.empty() ? "" : ", ") + wxString::Format("%d", c.port);
    }
    wxString text = open.empty() ? wxString() : wxString::Format(_("open: %s"), open);
    if (!filtered.empty())
        text += (text.empty() ? "" : "; ") + wxString::Format(_("blocked: %s"), filtered);
    return text;
}

// server paths may come from another operating system: both separators
wxString fileNameOf(const wxString& path)
{
    size_t slash = path.find_last_of("/\\");
    return slash == wxString::npos ? path : path.Mid(slash + 1);
}

wxString directoryOf(const wxString& path)
{
    size_t slash = path.find_last_of("/\\");
    return slash == wxString::npos ? wxString() : path.Left(slash);
}

void addRow(BenchmarkInfoGroup& group, const wxString& name, const wxString& value)
{
    if (!value.empty())
        group.rows.push_back({ name, value });
}

} // namespace

wxString getBenchmarkSystemTitle(const BenchmarkReport& report)
{
    const BenchmarkMetrics& m = report.metrics;
    if (isServerRun(m))
    {
        if (!m.system.model.empty())
            return utf8(m.system.model);
        return m.localHostName.empty() ? _("This machine") : m.localHostName;
    }
    wxString server = m.serverHostName.empty() ? m.connectionHost : m.serverHostName;
    return server.empty() ? _("Firebird server") : wxString::Format(_("Server %s"), server);
}

wxString getBenchmarkSystemSpecs(const BenchmarkReport& report)
{
    const BenchmarkMetrics& m = report.metrics;
    const BenchmarkSystemInfo& s = m.system;
    std::vector<wxString> parts;
    auto add = [&parts](const wxString& text)
    {
        if (!text.empty())
            parts.push_back(text);
    };
    if (isServerRun(m))
    {
        add(cpuText(s));
        if (s.memoryTotalBytes > 0)
            add(wxString::Format(_("%s RAM"), gigabytes(s.memoryTotalBytes)));
        if (!s.storageModel.empty())
        {
            add(utf8(s.storageModel) + (s.storageBus.empty() ? wxString()
                : " (" + utf8(s.storageBus) + ")"));
        }
        add(utf8(s.osName));
        add(firebirdText(m) + (m.connectionKind == BenchmarkConnectionKind::Embedded
            ? _(", embedded") : wxString()));
    }
    else
    {
        add(firebirdText(m));
        wxString client = wxString::Format(_("measured from %s"), m.localHostName);
        if (m.networkPath && !m.networkPath->adapterType.empty())
            client += wxString::Format(_(" over %s"), utf8(m.networkPath->adapterType));
        add(client);
    }
    wxString text;
    for (const auto& p : parts)
        text += (text.empty() ? wxString() : separator()) + p;
    return text;
}

std::vector<BenchmarkInfoGroup> getBenchmarkSystemOverview(const BenchmarkReport& report)
{
    const BenchmarkMetrics& m = report.metrics;
    const BenchmarkSystemInfo& s = m.system;
    std::vector<BenchmarkInfoGroup> groups;
    const wxString computer = s.fullHostName.empty() ? m.localHostName : utf8(s.fullHostName);

    // the machine running FlameRobin: in a server run the server itself
    BenchmarkInfoGroup machine;
    machine.title = isServerRun(m) ? _("This machine (database server)")
                                   : _("This machine (client)");
    addRow(machine, _("Computer"), computer);
    addRow(machine, _("IP address"), addressList(s, false));
    addRow(machine, _("Model"), utf8(s.model));
    addRow(machine, _("CPU"), cpuText(s));
    addRow(machine, _("Cores"), coresText(s, m.localCpuCount));
    addRow(machine, _("RAM"), memoryText(s));
    if (isServerRun(m))
        addRow(machine, _("Storage"), storageText(s, m));
    addRow(machine, _("Power"), powerText(m));
    addRow(machine, _("OS"), osText(s, m.clientPlatform));
    addRow(machine, _("User"), userText(s));
    if (!s.virtualization.empty())
        addRow(machine, _("Virtualization"), utf8(s.virtualization)
            + (s.container.empty() ? wxString() : ", " + utf8(s.container)));

    BenchmarkInfoGroup firebird;
    firebird.title = isServerRun(m) ? _("Firebird") : _("Database server");
    if (!isServerRun(m))
    {
        addRow(firebird, _("Host"), m.connectionHost);
        if (!m.serverHostName.empty() && m.serverHostName.CmpNoCase(m.connectionHost) != 0)
            addRow(firebird, _("Server name"), m.serverHostName);
        if (m.networkPath)
        {
            wxString addresses;
            for (const auto& a : m.networkPath->serverAddresses)
                addresses += (addresses.empty() ? "" : ", ") + utf8(a);
            addRow(firebird, _("IP address"), addresses);
        }
    }
    addRow(firebird, _("Firebird"), firebirdText(m));
    addRow(firebird, _("Access"), accessText(m));
    addRow(firebird, _("Firebird user"), firebirdUserText(m));
    addRow(firebird, _("Configuration"), configText(m));
    if (isServerRun(m))
    {
        addRow(firebird, _("Firebird servers"), runningServers(s.firebirdServers));
        addRow(firebird, _("Virus scanner"), [&s]()
        {
            wxString text;
            for (const auto& a : s.antivirus)
                text += (text.empty() ? "" : ", ") + utf8(a);
            return text;
        }());
    }

    groups.push_back(machine);
    groups.push_back(firebird);

    if (!isServerRun(m))
    {
        BenchmarkInfoGroup network;
        network.title = _("Network path");
        if (m.networkPath)
        {
            const BenchmarkNetworkPath& p = *m.networkPath;
            addRow(network, _("Adapter"), networkAdapterText(p));
            addRow(network, _("Local address"), utf8(p.localAddress));
            if (p.sameSubnet)
                addRow(network, _("Route"), *p.sameSubnet ? _("same subnet")
                    : _("through a router"));
        }
        addRow(network, _("Seen by the server as"), m.remoteAddress);
        if (m.latencyMedianMs)
            addRow(network, _("Round trip"), formatBenchmarkMilliseconds(*m.latencyMedianMs));
        addRow(network, _("Server ports"), portsText(m));
        addRow(network, _("Events"), m.eventMs ? _("received") : m.eventStatus);
        groups.push_back(network);
    }
    else
    {
        BenchmarkInfoGroup database;
        database.title = _("Test database");
        const BenchmarkDatabaseInfo& db = m.benchmarkDatabase;
        if (db.pageSize > 0)
        {
            addRow(database, _("Page size"), wxString::Format(_("%d KB, ODS %d.%d"),
                db.pageSize / 1024, db.odsMajor, db.odsMinor));
            addRow(database, _("Page cache"), wxString::Format(_("%d pages = %.0f MB"),
                db.cachePages, db.cachePages * double(db.pageSize) / 1048576.0));
            addRow(database, _("Forced writes"), db.forcedWrites ? _("on") : _("off"));
        }
        addRow(database, _("Directory"), m.benchmarkDirectory);
        if (!s.fileSystem.empty())
            addRow(database, _("File system"), utf8(s.fileSystem));
        groups.push_back(database);
    }
    return groups;
}

std::vector<BenchmarkInfoGroup> getBenchmarkSystemDetails(const BenchmarkReport& report)
{
    const BenchmarkMetrics& m = report.metrics;
    const BenchmarkSystemInfo& s = m.system;
    std::vector<BenchmarkInfoGroup> groups;
    const wxString where = isServerRun(m) ? _("this machine, the database server")
                                          : _("this machine, the client");

    BenchmarkInfoGroup cpu;
    cpu.title = wxString::Format(_("CPU of %s"), where);
    addRow(cpu, _("Name"), cpuText(s));
    addRow(cpu, _("Cores"), coresText(s, m.localCpuCount));
    if (s.cpuBaseMHz > 0)
        addRow(cpu, _("Base clock"), megahertz(s.cpuBaseMHz));
    if (s.cpuMaxMHz > 0)
        addRow(cpu, _("Maximum clock"), megahertz(s.cpuMaxMHz));
    auto cache = [](int64_t bytes)
    {
        return bytes >= 1048576 ? wxString::Format(_("%.1f MB"), bytes / 1048576.0)
                                : wxString::Format(_("%lld KB"), (long long)(bytes / 1024));
    };
    if (s.cacheL1Bytes > 0)
        addRow(cpu, _("L1 cache"), cache(s.cacheL1Bytes));
    if (s.cacheL2Bytes > 0)
        addRow(cpu, _("L2 cache"), cache(s.cacheL2Bytes));
    if (s.cacheL3Bytes > 0)
        addRow(cpu, _("L3 cache"), cache(s.cacheL3Bytes));
    groups.push_back(cpu);

    BenchmarkInfoGroup memory;
    memory.title = _("Memory");
    if (s.memoryTotalBytes > 0)
        addRow(memory, _("Installed"), gigabytes(s.memoryTotalBytes));
    if (s.memoryAvailableBytes > 0)
        addRow(memory, _("Available"), gigabytes(s.memoryAvailableBytes));
    if (s.swapTotalBytes > 0)
        addRow(memory, _("Page file / swap"), gigabytes(s.swapTotalBytes));
    if (s.memorySlots > 0)
    {
        addRow(memory, _("Slots"), wxString::Format(_("%d of %d used"),
            int(s.memoryModules.size()), s.memorySlots));
    }
    for (const auto& module : s.memoryModules)
    {
        wxString text = gigabytes(module.sizeBytes) + " " + utf8(module.type);
        if (!module.formFactor.empty())
            text += " " + utf8(module.formFactor);
        if (module.speedMTs > 0)
            text += wxString::Format(_(", %d MT/s"), module.speedMTs);
        if (module.configuredSpeedMTs > 0 && module.configuredSpeedMTs != module.speedMTs)
            text += wxString::Format(_(" (runs at %d MT/s)"), module.configuredSpeedMTs);
        if (!module.manufacturer.empty() || !module.partNumber.empty())
            text += ", " + utf8(module.manufacturer) + " " + utf8(module.partNumber);
        addRow(memory, module.locator.empty() ? _("Module") : utf8(module.locator), text.Trim());
    }
    addRow(memory, _("Modules"), utf8(s.memoryModulesNote));
    groups.push_back(memory);

    if (!s.storagePath.empty())
    {
        BenchmarkInfoGroup storage;
        storage.title = _("Drive of the database directory");
        addRow(storage, _("Directory"), utf8(s.storagePath));
        addRow(storage, _("Model"), utf8(s.storageModel));
        addRow(storage, _("Firmware"), utf8(s.storageFirmware));
        addRow(storage, _("Connection"), utf8(s.storageBus));
        addRow(storage, _("Type"), utf8(s.storageMedia));
        if (s.storageSizeBytes > 0)
            addRow(storage, _("Size"), gigabytes(s.storageSizeBytes));
        if (m.databaseDiskFreeBytes)
            addRow(storage, _("Free"), gigabytes(*m.databaseDiskFreeBytes));
        if (!s.fileSystem.empty())
        {
            addRow(storage, _("File system"), utf8(s.fileSystem)
                + (s.fileSystemClusterBytes > 0 ? wxString::Format(_(", %lld KB clusters"),
                    (long long)(s.fileSystemClusterBytes / 1024)) : wxString()));
        }
        addRow(storage, _("Mount options"), utf8(s.mountOptions));
        addRow(storage, _("I/O scheduler"), utf8(s.ioScheduler));
        if (s.writeCacheEnabled)
            addRow(storage, _("Write cache"), *s.writeCacheEnabled ? _("on") : _("off"));
        if (s.writeCachePowerProtected)
        {
            addRow(storage, _("Write cache buffer flushing"), *s.writeCachePowerProtected
                ? _("off (flushes are ignored)") : _("on"));
        }
        if (s.trimEnabled)
            addRow(storage, _("TRIM"), *s.trimEnabled ? _("on") : _("off"));
        if (s.storageTemperatureC)
            addRow(storage, _("Temperature"),
                wxString::Format(wxString::FromUTF8("%d \xC2\xB0" "C"),
                *s.storageTemperatureC));
        groups.push_back(storage);

        BenchmarkInfoGroup health;
        health.title = _("Drive health (SMART)");
        for (const auto& v : s.storageHealth)
            addRow(health, utf8(v.name), utf8(v.value) + (v.warning ? _(" (check)") : wxString()));
        if (s.storageHealth.empty())
            addRow(health, _("Values"), s.storageHealthNote.empty() ? _("not reported")
                : utf8(s.storageHealthNote));
        groups.push_back(health);
    }

    BenchmarkInfoGroup machine;
    machine.title = _("Machine");
    addRow(machine, _("Computer"), s.fullHostName.empty() ? m.localHostName : utf8(s.fullHostName));
    addRow(machine, _("Manufacturer"), utf8(s.manufacturer));
    addRow(machine, _("Model"), utf8(s.model));
    if (!s.boardProduct.empty())
        addRow(machine, _("Mainboard"), utf8(s.boardManufacturer) + " " + utf8(s.boardProduct));
    if (!s.biosVersion.empty())
    {
        addRow(machine, _("BIOS"), (utf8(s.biosVendor) + " " + utf8(s.biosVersion)
            + (s.biosDate.empty() ? wxString() : ", " + utf8(s.biosDate))).Trim(false));
    }
    addRow(machine, _("Virtualization"), s.virtualization.empty() ? _("none found")
        : utf8(s.virtualization));
    addRow(machine, _("Container"), utf8(s.container));
    groups.push_back(machine);

    BenchmarkInfoGroup os;
    os.title = _("Operating system");
    addRow(os, _("Name"), osText(s, m.clientPlatform));
    addRow(os, _("Kernel"), utf8(s.kernel));
    if (s.uptimeSeconds > 0)
        addRow(os, _("Running since"), duration(s.uptimeSeconds));
    addRow(os, _("User"), userText(s));
    addRow(os, _("Power"), powerText(m));
    if (s.tcpKeepAliveSeconds)
        addRow(os, _("TCP keepalive"), wxString::Format(_("after %d min idle"),
            *s.tcpKeepAliveSeconds / 60));
    for (const auto& setting : s.osSettings)
        addRow(os, utf8(setting.first), utf8(setting.second));
    groups.push_back(os);

    BenchmarkInfoGroup security;
    security.title = _("Virus scanner and firewall");
    wxString scanners;
    for (const auto& a : s.antivirus)
        scanners += (scanners.empty() ? "" : ", ") + utf8(a);
    addRow(security, _("Virus scanner"), scanners.empty() ? utf8(s.antivirusNote) : scanners);
    if (s.defenderRealtime)
        addRow(security, _("Defender real-time protection"),
            *s.defenderRealtime ? _("on") : _("off"));
    if (s.controlledFolderAccess)
        addRow(security, _("Controlled folder access"),
            *s.controlledFolderAccess ? _("on") : _("off"));
    if (!s.storagePath.empty())
    {
        addRow(security, _("Database directory excluded"), s.storagePathExcluded
            ? yesNo(*s.storagePathExcluded) : _("unknown (needs administrator rights)"));
    }
    addRow(security, _("Firewall"), s.firewallProfiles.empty() ? utf8(s.firewallNote)
        : (s.firewallProfiles == "off" ? _("off") : wxString::Format(_("on for %s"),
            utf8(s.firewallProfiles))));
    groups.push_back(security);

    BenchmarkInfoGroup network;
    network.title = _("Network of this machine");
    addRow(network, _("Addresses"), addressList(s, true));
    for (const auto& p : s.listeningPorts)
    {
        wxString text = utf8(p.address);
        if (!p.process.empty())
            text += ", " + utf8(p.process);
        if (!p.firewall.empty())
            text += ", " + utf8(p.firewall);
        addRow(network, wxString::Format(_("Port %d listens"), p.port), text);
    }
    if (m.networkPath)
    {
        const BenchmarkNetworkPath& p = *m.networkPath;
        addRow(network, _("Adapter to the server"), networkAdapterText(p));
        addRow(network, _("Adapter description"), utf8(p.adapterDescription));
        if (p.mtu > 0)
            addRow(network, _("MTU"), wxString::Format("%d", p.mtu));
        addRow(network, _("Wi-Fi signal"), utf8(p.wifiSignal));
        if (p.resolveMs >= 0.0)
            addRow(network, _("Name resolution"), formatBenchmarkMilliseconds(p.resolveMs));
        addRow(network, _("Note"), utf8(p.note));
    }
    for (const auto& c : m.serverPorts)
    {
        addRow(network, wxString::Format(_("Server port %d"), c.port),
            c.state == BenchmarkPortCheck::State::Open
                ? wxString::Format(_("open (%s)"), formatBenchmarkMilliseconds(c.ms))
                : c.state == BenchmarkPortCheck::State::Closed ? _("closed, nothing listens")
                : _("no answer, blocked by a firewall"));
    }
    groups.push_back(network);

    BenchmarkInfoGroup servers;
    servers.title = _("Database servers on this machine");
    for (const auto& p : s.firebirdServers)
    {
        addRow(servers, utf8(p.name), utf8(p.detail) + (p.detail.empty() ? "" : ", ")
            + (p.running ? _("running") : _("stopped")));
    }
    for (const auto& p : s.otherDatabaseServers)
    {
        addRow(servers, utf8(p.name), utf8(p.detail) + (p.detail.empty() ? "" : ", ")
            + (p.running ? _("running") : _("stopped")));
    }
    if (servers.rows.empty())
        addRow(servers, _("Servers"), _("none found"));
    groups.push_back(servers);

    BenchmarkInfoGroup connection;
    connection.title = _("Firebird and the connection");
    addRow(connection, _("Server"), firebirdText(m));
    addRow(connection, _("Implementation"), m.serverImplementation);
    for (size_t i = 1; i < m.versionList.size(); ++i)
        addRow(connection, _("Remote layer"), m.versionList[i]);
    addRow(connection, _("Server host name"), m.serverHostName);
    addRow(connection, _("Access"), accessText(m));
    addRow(connection, _("Protocol"), m.remoteProtocol + (m.protocolVersion.empty()
        ? wxString() : " " + m.protocolVersion));
    if (m.wireCompressed)
        addRow(connection, _("Wire compression"), *m.wireCompressed ? _("on") : _("off"));
    if (m.wireEncrypted)
    {
        addRow(connection, _("Wire encryption"), *m.wireEncrypted
            ? (m.wireCryptPlugin.empty() ? _("on") : m.wireCryptPlugin) : _("off"));
    }
    addRow(connection, _("Login"), m.authMethod);
    addRow(connection, _("Security database"), m.securityDatabase);
    addRow(connection, _("Firebird user"), firebirdUserText(m));
    addRow(connection, _("Client address seen by the server"), m.remoteAddress);
    addRow(connection, _("Client host seen by the server"), m.clientHostSeenByServer);
    addRow(connection, _("Client library"), m.clientVersion);
    if (m.idleTimeoutSeconds)
        addRow(connection, _("Idle timeout"), *m.idleTimeoutSeconds > 0
            ? wxString::Format(_("%d min"), *m.idleTimeoutSeconds / 60) : _("none"));
    if (m.statementTimeoutMs)
        addRow(connection, _("Statement timeout"), *m.statementTimeoutMs > 0
            ? formatBenchmarkMilliseconds(*m.statementTimeoutMs) : _("none"));
    groups.push_back(connection);

    BenchmarkInfoGroup config;
    config.title = _("firebird.conf: values that are set");
    for (const auto& name : m.serverConfigExplicit)
    {
        auto value = m.serverConfig.find(name);
        addRow(config, name, value == m.serverConfig.end() ? wxString() : value->second);
    }
    if (config.rows.empty())
        addRow(config, _("Values"), configText(m));
    groups.push_back(config);

    // groups without content are left out
    groups.erase(std::remove_if(groups.begin(), groups.end(),
        [](const BenchmarkInfoGroup& g) { return g.rows.empty(); }), groups.end());
    return groups;
}

BenchmarkReport BenchmarkReport::anonymized() const
{
    BenchmarkReport r = *this;
    r.anonymous = true;
    BenchmarkMetrics& m = r.metrics;
    BenchmarkSystemInfo& s = m.system;

    // every private text and what replaces it; longer texts are replaced
    // first, so that a host name inside a full name is handled correctly
    std::vector<std::pair<wxString, wxString>> privateTexts;
    auto hide = [&privateTexts](const wxString& text, const wxString& replacement)
    {
        wxString t = text;
        t.Trim().Trim(false);
        // very short texts would replace parts of ordinary words
        if (t.length() >= 3 && t != replacement)
            privateTexts.push_back({ t, replacement });
    };
    const wxString thisMachine = m.runLocation == BenchmarkRunLocation::Client
        ? _("client") : _("server");
    const wxString path = _("<path>");
    const wxString address = _("<address>");
    const wxString user = _("<user>");
    const wxString hidden = _("<hidden>");

    hide(utf8(s.fullHostName), thisMachine);
    hide(m.localHostName, thisMachine);
    hide(m.connectionHost, _("server"));
    hide(m.serverHostName, _("server"));
    hide(m.clientHostSeenByServer, _("client"));
    // the domain of the computer name
    if (s.fullHostName.find('.') != std::string::npos)
        hide(utf8(s.fullHostName.substr(s.fullHostName.find('.') + 1)), hidden);
    hide(utf8(s.osUser), user);
    if (s.osUser.find('\\') != std::string::npos)
    {
        hide(utf8(s.osUser.substr(s.osUser.find('\\') + 1)), user);
        hide(utf8(s.osUser.substr(0, s.osUser.find('\\'))), hidden);
    }
    if (m.firebirdUser.CmpNoCase("SYSDBA") != 0)
        hide(m.firebirdUser, user);
    if (m.firebirdRole.CmpNoCase("NONE") != 0 && m.firebirdRole.CmpNoCase("RDB$ADMIN") != 0)
        hide(m.firebirdRole, hidden);
    for (const auto& a : s.addresses)
        hide(utf8(a.address), address);
    hide(m.remoteAddress.BeforeLast('/').empty() ? m.remoteAddress
        : m.remoteAddress.BeforeLast('/'), address);
    if (m.networkPath)
    {
        hide(utf8(m.networkPath->serverHost), _("server"));
        hide(utf8(m.networkPath->localAddress), address);
        for (const auto& a : m.networkPath->serverAddresses)
            hide(utf8(a), address);
    }
    hide(directoryOf(m.benchmarkDatabase.fileName), path);
    hide(m.benchmarkDirectory, path);
    hide(m.tempDirectory, path);
    hide(utf8(s.storagePath), path);
    if (m.inspectedDatabase)
        hide(m.inspectedDatabase->fileName, path);
    hide(benchmarkDatabasePath, path);
    for (const auto& p : s.firebirdServers)
    {
        hide(utf8(p.name), _("Firebird service"));
        hide(utf8(p.detail), path);
    }
    for (auto& p : s.otherDatabaseServers)
    {
        hide(utf8(p.name), _("Other database server"));
        hide(utf8(p.detail), path);
        p.name = std::string(_("Other database server").utf8_str());
    }
    for (const auto& kv : m.serverConfig)
    {
        // directories and file names in firebird.conf
        if (kv.second.Contains("/") || kv.second.Contains("\\"))
            hide(kv.second, path);
    }
    std::stable_sort(privateTexts.begin(), privateTexts.end(),
        [](const auto& a, const auto& b) { return a.first.length() > b.first.length(); });
    auto scrub = [&privateTexts](wxString& text)
    {
        for (const auto& p : privateTexts)
            text.Replace(p.first, p.second);
    };
    auto scrubUtf8 = [&scrub](std::string& text)
    {
        wxString t = utf8(text);
        scrub(t);
        text = std::string(t.utf8_str());
    };

    // the raw values, so that the system description is clean
    m.localHostName = thisMachine;
    if (!m.connectionHost.empty())
        m.connectionHost = _("server");
    m.serverHostName.clear();
    m.clientHostSeenByServer.clear();
    m.remoteAddress = m.remoteAddress.empty() ? wxString() : address;
    m.benchmarkDirectory = m.benchmarkDirectory.empty() ? wxString() : path;
    m.tempDirectory = path;
    m.benchmarkDatabase.fileName.clear();
    if (m.inspectedDatabase)
        m.inspectedDatabase->fileName.clear();
    for (auto& v : m.versionList)
        scrub(v);
    for (auto& kv : m.serverConfig)
        scrub(kv.second);
    for (auto& skipped : m.skippedTests)
        scrub(skipped.second);
    scrub(m.firebirdUser);
    scrub(m.firebirdRole);
    s.fullHostName.clear();
    s.osUser = s.osUser.empty() ? std::string() : std::string(user.utf8_str());
    for (auto& a : s.addresses)
        a.address = std::string(address.utf8_str());
    scrubUtf8(s.storagePath);
    for (auto& p : s.firebirdServers)
    {
        scrubUtf8(p.name);
        scrubUtf8(p.detail);
    }
    for (auto& p : s.otherDatabaseServers)
        scrubUtf8(p.detail);
    for (auto& l : s.listeningPorts)
    {
        // firewall rules have names of companies and products
        if (l.firewall.rfind("allowed by", 0) == 0)
            l.firewall = "allowed by a rule";
        else if (l.firewall.rfind("blocked by", 0) == 0)
            l.firewall = "blocked by a rule";
    }
    if (m.networkPath)
    {
        m.networkPath->serverHost = std::string(_("server").utf8_str());
        m.networkPath->localAddress = std::string(address.utf8_str());
        for (auto& a : m.networkPath->serverAddresses)
            a = std::string(address.utf8_str());
        scrubUtf8(m.networkPath->note);
    }

    // the earlier run of a comparison may come from another machine: its
    // computer name goes, paths in its settings as well
    if (r.earlier)
    {
        r.earlier->host.clear();
        scrub(r.earlier->title);
        for (auto& f : r.earlier->facts)
        {
            scrub(f.second);
            if (f.first.StartsWith("conf.") && (f.second.Contains("/") || f.second.Contains("\\")))
                f.second = path;
        }
        for (auto& h : r.earlier->hints)
        {
            scrub(h.title);
            scrub(h.detail);
            scrub(h.advice);
        }
    }
    r.target = hidden;
    r.benchmarkDatabasePath = fileNameOf(benchmarkDatabasePath);
    for (auto& p : r.undroppedDatabases)
        p = fileNameOf(p);
    scrub(r.errorMessage);
    for (auto& result : r.results)
    {
        scrub(result.machine);
        scrub(result.test);
        scrub(result.value);
        scrub(result.details);
    }
    for (auto& h : r.hints)
    {
        scrub(h.title);
        scrub(h.detail);
        scrub(h.advice);
    }
    for (auto& s : r.settings)
    {
        scrub(s.value);
        if (s.value.Contains("/") || s.value.Contains("\\"))
            s.value = path;
    }
    for (auto& p : r.parts)
        scrub(p.machine);
    return r;
}

wxString getBenchmarkUnitHint(const wxString& value)
{
    // longer units first, so that "it/ms" is not taken for "ms"
    static const std::pair<const char*, const char*> units[] = {
        { "operations/s", wxTRANSLATE("Operations per second (insert, read, update and "
            "delete together); higher is better.") },
        { "connections/s", wxTRANSLATE("Connections per second; higher is better.") },
        { "prepares/s", wxTRANSLATE("Statement preparations per second; higher is better.") },
        { "queries/s", wxTRANSLATE("Queries per second; higher is better.") },
        { "files/s", wxTRANSLATE("Files per second; higher is better.") },
        { "rows/s", wxTRANSLATE("Rows per second; higher is better.") },
        { "transactions/s", wxTRANSLATE("Transactions per second: small units of work "
            "(read, change, save) that were completed; higher is better.") },
        { "tx/s", wxTRANSLATE("Transactions per second: small units of work (read, change, "
            "save) that were completed; higher is better.") },
        { "writes/s", wxTRANSLATE("Rows written per second, each sent to the server on its "
            "own; higher is better.") },
        { "it/ms", wxTRANSLATE("Iterations per millisecond: how many rounds of a fixed "
            "calculation the server manages in a thousandth of a second; higher is better.") },
        { "it/s", wxTRANSLATE("Iterations per second: rounds of a fixed calculation; higher "
            "is better.") },
        { "Gbit/s", wxTRANSLATE("Link speed of the network adapter in billions of bits per "
            "second; 1 Gbit/s carries about 110 MB/s.") },
        { "Mbit/s", wxTRANSLATE("Link speed of the network adapter in millions of bits per "
            "second.") },
        { "MB/s", wxTRANSLATE("Megabytes per second (1 MB = 1,048,576 bytes); higher is "
            "better.") },
        { "\xC2\xB5s", wxTRANSLATE("Microseconds: thousandths of a millisecond; lower is "
            "better.") },
        { " ms", wxTRANSLATE("Milliseconds: thousandths of a second; lower is better.") },
        { "points", wxTRANSLATE("Points compare two runs: the earlier run has 1000, the "
            "later one more when it is faster and fewer when it is slower; twice as fast "
            "gives 2000 points, half as fast 500.") }
    };
    for (const auto& u : units)
    {
        if (value.Contains(wxString::FromUTF8(u.first)))
            return wxGetTranslation(u.second);
    }
    return wxEmptyString;
}

wxString getBenchmarkFactLabel(const wxString& key)
{
    static const std::pair<const char*, const char*> labels[] = {
        { "system", wxTRANSLATE("System") },
        { "firebird", wxTRANSLATE("Firebird") },
        { "access", wxTRANSLATE("Connection") },
        { "os", wxTRANSLATE("Operating system") },
        { "cpu", wxTRANSLATE("Processor") },
        { "cores", wxTRANSLATE("Cores") },
        { "ram", wxTRANSLATE("Memory (RAM)") },
        { "storage", wxTRANSLATE("Disk") },
        { "power", wxTRANSLATE("Power plan") },
        { "scanner", wxTRANSLATE("Virus scanner") },
        { "virtual", wxTRANSLATE("Virtual machine") },
        { "network", wxTRANSLATE("Network adapter") },
        { "pagesize", wxTRANSLATE("Page size") },
        { "pagecache", wxTRANSLATE("Page cache") },
        { "forcedwrites", wxTRANSLATE("Forced writes") },
        { "wire", wxTRANSLATE("Network compression and encryption") }
    };
    for (const auto& l : labels)
    {
        if (key == l.first)
            return wxGetTranslation(l.second);
    }
    if (key.StartsWith("conf."))
        return "firebird.conf: " + key.Mid(5);
    return key;
}

std::vector<BenchmarkFact> getBenchmarkFacts(const BenchmarkReport& report)
{
    // only what stays the same between two runs on an unchanged system:
    // no free memory or disk space, no addresses
    const BenchmarkMetrics& m = report.metrics;
    const BenchmarkSystemInfo& s = m.system;
    std::vector<BenchmarkFact> list;
    auto add = [&list](const wxString& key, const wxString& value)
    {
        wxString text = value;
        if (!text.Trim().Trim(false).empty())
            list.push_back({ key, getBenchmarkFactLabel(key), text });
    };
    add("system", getBenchmarkSystemTitle(report));
    add("firebird", firebirdText(m));
    add("access", accessText(m));
    add("os", osText(s, m.clientPlatform));
    add("cpu", cpuText(s));
    add("cores", coresText(s, m.localCpuCount));
    if (s.memoryTotalBytes > 0)
    {
        wxString ram = gigabytes(s.memoryTotalBytes);
        if (!s.memoryModules.empty())
        {
            const BenchmarkMemoryModule& first = s.memoryModules.front();
            ram += " " + utf8(first.type);
            const int speed = first.configuredSpeedMTs > 0 ? first.configuredSpeedMTs
                : first.speedMTs;
            if (speed > 0)
                ram += wxString::Format("-%d", speed);
        }
        add("ram", ram);
    }
    if (isServerRun(m))
    {
        wxString storage = utf8(s.storageModel);
        wxString kind = utf8(s.storageBus);
        if (!s.storageMedia.empty())
            kind += (kind.empty() ? "" : " ") + utf8(s.storageMedia);
        if (!kind.empty())
            storage += (storage.empty() ? wxString() : separator()) + kind;
        add("storage", storage);
    }
    add("power", powerText(m));
    wxString scanners;
    for (const auto& a : s.antivirus)
        scanners += (scanners.empty() ? "" : ", ") + utf8(a);
    add("scanner", scanners);
    add("virtual", utf8(s.virtualization) + (s.container.empty() ? wxString()
        : ", " + utf8(s.container)));
    if (m.networkPath && !isServerRun(m))
    {
        wxString adapter = utf8(m.networkPath->adapterType);
        if (m.networkPath->linkSpeedBitsPerSecond > 0)
        {
            adapter += (adapter.empty() ? "" : ", ")
                + formatBenchmarkBitRate(double(m.networkPath->linkSpeedBitsPerSecond));
        }
        add("network", adapter);
    }
    const BenchmarkDatabaseInfo& db = m.benchmarkDatabase;
    if (db.pageSize > 0)
        add("pagesize", wxString::Format(_("%d KB"), db.pageSize / 1024));
    if (db.cachePages > 0 && db.pageSize > 0)
    {
        add("pagecache", wxString::Format(_("%d pages = %s MB"), db.cachePages,
            wxString::FromCDouble(db.cachePages * double(db.pageSize) / 1048576.0, 0)));
    }
    if (db.pageSize > 0)
        add("forcedwrites", db.forcedWrites ? _("on") : _("off"));
    if (m.wireCompressed || m.wireEncrypted)
    {
        add("wire", wxString::Format(_("compression %s, encryption %s"),
            m.wireCompressed.value_or(false) ? _("on") : _("off"),
            m.wireEncrypted.value_or(false) ? _("on") : _("off")));
    }
    for (const auto& kv : m.serverConfig)
    {
        // FlameRobin sets the providers of the engine inside it itself
        if (kv.first == "Providers" && m.connectionKind == BenchmarkConnectionKind::Embedded)
            continue;
        add("conf." + kv.first, kv.second == "<null>" ? wxString() : kv.second);
    }
    return list;
}

std::vector<BenchmarkGlossaryEntry> getBenchmarkGlossary()
{
    return {
        { _("Key value"), _("The most telling value of a test, as work per second or as "
            "the time of one operation, with the work behind it. A single run shows the "
            "values without judging them: whether a value is fast or slow depends on the "
            "environment.") },
        { _("Points"), _("When two runs are compared, the earlier run gets 1000 points for "
            "every part and in total, the later run proportionally more or fewer: twice as "
            "fast gives 2000 points. A part is the geometric mean of its key values and the "
            "total the geometric mean of the parts, so that no single value dominates.") },
        { _("Possible reason"), _("Where a part of the system differs clearly between two "
            "runs, the comparison names what could make the slower run slower: the "
            "hardware, versions and settings that differ, what FlameRobin noticed on the "
            "slower run, and the typical causes of its slower values.") },
        { _("Round trip"), _("One request from FlameRobin to the server and the answer "
            "back. Every statement needs at least one; on a network it is the most "
            "important value for interactive applications.") },
        { _("Latency"), _("The waiting time of a single operation, e.g. a round trip or a "
            "commit.") },
        { _("Median"), _("The middle value: half of the measurements are faster, half "
            "slower. Not distorted by single outliers like the average.") },
        { _("95th percentile (95 %)"), _("95 % of the measurements are at least this "
            "fast; shows how slow the slow cases are.") },
        { _("OLTP"), _("Online Transaction Processing: many short transactions of many "
            "users, like order entry or booking systems.") },
        { _("Transaction (tx)"), _("A unit of work that is committed (made permanent) or "
            "rolled back as a whole.") },
        { _("Commit"), _("Makes the changes of a transaction permanent. With forced writes "
            "Firebird waits until the storage confirms the write.") },
        { _("Forced writes"), _("Firebird writes changed pages synchronously to the drive "
            "at every commit. Safe against power failures, but every commit waits for the "
            "storage.") },
        { _("Page / page size"), _("Firebird stores everything in pages of 4 to 32 KB. "
            "Larger pages make scans cheaper and every changed page larger to write.") },
        { _("Page cache (page buffers)"), _("The pages Firebird keeps in RAM "
            "(DefaultDbCachePages, gfix -buffers). SuperServer shares one cache among all "
            "connections, Classic has one per connection.") },
        { _("Operating system file cache"), _("RAM the operating system uses to cache file "
            "contents; Firebird uses it in addition to its page cache "
            "(UseFileSystemCache).") },
        { _("Sort memory (TempCacheLimit)"), _("RAM for sorts (ORDER BY, GROUP BY, "
            "DISTINCT, index creation). Larger sorts go to temporary files in "
            "TempDirectories.") },
        { _("Disk test with a small cache"), _("Database work with a page cache of only 50 "
            "pages: nearly every page access goes to the disk, so the result shows the disk "
            "instead of the memory.") },
        { _("Garbage collection"), _("Removing old record versions and deleted rows. "
            "Firebird keeps old versions as long as a transaction might need them (MVCC).") },
        { _("Record version (MVCC)"), _("Firebird keeps the old version of a changed row "
            "for transactions that started before the change (multi-version concurrency "
            "control); readers never wait for writers.") },
        { _("SuperServer / Classic / SuperClassic"), _("Server architectures of Firebird: "
            "one process with a shared page cache, one process per connection, or one "
            "process with one cache per connection.") },
        { _("Embedded"), _("The Firebird engine runs inside the application, without a "
            "server process and without a network.") },
        { _("XNET"), _("Local connection between processes on Windows through shared "
            "memory, without TCP/IP.") },
        { _("RemoteServicePort"), _("The TCP port of the Firebird server, 3050 by default.") },
        { _("RemoteAuxPort"), _("The port for database events. With 0 Firebird uses a "
            "random port, which firewalls usually block.") },
        { _("Events (POST_EVENT)"), _("Notifications from triggers or procedures to the "
            "connected applications.") },
        { _("Wire compression / encryption"), _("Firebird can compress and encrypt the "
            "network traffic (WireCompression, WireCrypt); both cost CPU.") },
        { _("SRP / Win_Sspi"), _("Login methods of Firebird: password based Secure Remote "
            "Password, or Windows authentication.") },
        { _("TCP keepalive"), _("Small packets on an idle connection that keep firewalls "
            "and NAT routers from dropping it.") },
        { _("NAT"), _("Network address translation: a router replaces the address of the "
            "client, so the server sees another address.") },
        { _("VPN"), _("Virtual private network: an encrypted tunnel that adds latency.") },
        { _("MTU"), _("Maximum transmission unit: the largest network packet. A wrong MTU "
            "on VPNs splits or drops large packets.") },
        { _("Retransmission"), _("A lost network packet is only sent again after at "
            "least 200 ms; single very slow round trips show lost packets.") },
        { _("IOPS"), _("Input/output operations per second, a common measure of the "
            "speed of disks.") },
        { _("NVMe / SSD / HDD"), _("Kinds of drives: NVMe SSDs are connected directly to "
            "the CPU and the fastest; SATA SSDs are slower; hard disks (HDD) have moving "
            "heads and are much slower for random access.") },
        { _("SMART"), _("Self-monitoring of drives: wear, temperature, errors and "
            "operating hours.") },
        { _("TRIM"), _("Tells an SSD which blocks are free, which keeps it fast.") },
        { _("Write cache"), _("Memory in the drive that collects writes. Without power "
            "protection, flushes make sure the data really is on the drive.") },
        { _("Power plan / governor"), _("Energy settings of the operating system; power "
            "saving lowers the CPU clock between requests.") },
        { _("On-access virus scanner"), _("Checks every file access; slows down database "
            "files and temporary files unless they are excluded.") },
        { _("PSQL"), _("The procedural language of Firebird for stored procedures, "
            "triggers and EXECUTE BLOCK.") },
        { _("GTT"), _("Global temporary table: rows only live for a transaction or "
            "connection, without database I/O.") },
        { _("BLOB"), _("Binary large object: large texts or binary data, stored on pages "
            "of their own.") },
        { _("ODS"), _("On-disk structure: the version of the database file format.") },
        { _("MON$ / RDB$CONFIG"), _("System tables with monitoring information and the "
            "effective server configuration (Firebird 4+).") },
        { _("Geometric mean"), _("The n-th root of the product of n values; a value twice "
            "as good compensates a value half as good.") }
    };
}

} // namespace fr
