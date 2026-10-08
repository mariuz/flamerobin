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

#ifndef FR_BENCHMARKSYSTEMINFO_H
#define FR_BENCHMARKSYSTEMINFO_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// Hardware and software of the machine FlameRobin runs on, for the
// benchmark report. The module has no wxWidgets dependency; all strings are
// UTF-8. Values the operating system does not report, or only to
// administrators, stay empty. Serial numbers and MAC addresses are never
// read.

namespace fr
{

/// An installed memory module (SMBIOS type 17)
struct BenchmarkMemoryModule
{
    std::string locator;            // e.g. "DIMM 1" or "Controller0-ChannelA"
    std::string type;               // e.g. "DDR5"
    std::string formFactor;         // e.g. "SODIMM"
    std::string manufacturer;
    std::string partNumber;
    int64_t sizeBytes = 0;
    int speedMTs = 0;               // rated speed
    int configuredSpeedMTs = 0;     // speed in use
};

/// A value of the drive health (NVMe health log or SMART attribute)
struct BenchmarkHealthValue
{
    std::string name;
    std::string value;
    bool warning = false;           // outside the healthy range
};

/// A database server installed or running on this machine
struct BenchmarkServerProcess
{
    std::string name;               // service or process name
    std::string detail;             // e.g. the program directory
    bool running = false;
};

/// An IP address of this machine and the adapter it belongs to
struct BenchmarkNetworkAddress
{
    std::string adapter;
    std::string type;               // "Ethernet", "Wi-Fi", "VPN", ...
    std::string address;
};

/// A TCP port this machine listens on (database servers and 3050 - 3060)
struct BenchmarkListeningPort
{
    int port = 0;
    std::string address;            // "all addresses", "127.0.0.1", ...
    std::string process;            // empty if not known
    // Windows firewall: the inbound rule that allows the port, "blocked by
    // ..." or "no inbound rule"; empty if the firewall is off or unknown
    std::string firewall;
};

/// Result of a TCP connection attempt from this machine to a server port
struct BenchmarkPortCheck
{
    enum class State
    {
        Open,       // connection accepted
        Closed,     // the server answered, but nothing listens
        Filtered    // no answer: a firewall drops the packets
    };

    int port = 0;
    State state = State::Filtered;
    double ms = 0.0;
};

struct BenchmarkSystemInfo
{
    // computer name with domain, and the addresses of the active adapters
    // (without loopback)
    std::string fullHostName;
    std::vector<BenchmarkNetworkAddress> addresses;
    // the user FlameRobin runs as; elevated: administrator rights (root)
    std::string osUser;
    std::optional<bool> elevated;

    // machine (SMBIOS / DMI)
    std::string manufacturer;
    std::string model;
    std::string boardManufacturer;
    std::string boardProduct;
    std::string biosVendor;
    std::string biosVersion;
    std::string biosDate;
    std::string virtualization;     // hypervisor of a virtual machine
    std::string container;          // e.g. "Docker", "WSL 2"

    // CPU
    std::string cpuName;
    int cpuSockets = 0;
    int cpuCores = 0;
    int cpuThreads = 0;
    int cpuPerformanceCores = 0;    // hybrid CPUs only
    int cpuEfficiencyCores = 0;
    int cpuBaseMHz = 0;
    int cpuMaxMHz = 0;
    // all cache instances added up, as the Windows Task Manager shows them
    int64_t cacheL1Bytes = 0;
    int64_t cacheL2Bytes = 0;
    int64_t cacheL3Bytes = 0;

    // memory
    int64_t memoryTotalBytes = 0;
    int64_t memoryAvailableBytes = 0;
    int64_t swapTotalBytes = 0;     // page file or swap space
    int memorySlots = 0;
    std::vector<BenchmarkMemoryModule> memoryModules;
    std::string memoryModulesNote;  // why the modules are not known

    // drive of a directory on this machine (the benchmark directory)
    std::string storagePath;
    std::string storageModel;
    std::string storageFirmware;
    std::string storageBus;         // "NVMe", "SATA", "SAS", "RAID", "USB", "virtual", ...
    std::string storageMedia;       // "SSD" or "HDD"
    int64_t storageSizeBytes = 0;
    std::string fileSystem;
    int64_t fileSystemClusterBytes = 0;
    std::string mountOptions;
    std::string ioScheduler;
    std::optional<bool> writeCacheEnabled;
    std::optional<bool> writeCachePowerProtected;
    std::optional<bool> trimEnabled;
    std::optional<int> storageTemperatureC;
    // NVMe health log or SMART; storageHealthNote says why it is missing
    std::vector<BenchmarkHealthValue> storageHealth;
    std::optional<bool> storageHealthWarning;
    std::string storageHealthNote;

    // software
    std::string osName;             // e.g. "Windows 11 Pro"
    std::string osVersion;          // e.g. "25H2, build 26200.6584"
    std::string kernel;             // Linux and macOS
    std::string architecture;       // e.g. "x64", "arm64"
    int64_t uptimeSeconds = 0;
    std::string powerPlan;          // Windows power plan and mode, Linux governor
    std::vector<std::string> antivirus;     // products and their state
    std::string antivirusNote;
    // Microsoft Defender: real-time protection, controlled folder access,
    // and whether the benchmark directory is excluded from scanning
    std::optional<bool> defenderRealtime;
    std::optional<bool> controlledFolderAccess;
    std::optional<bool> storagePathExcluded;
    std::vector<BenchmarkServerProcess> firebirdServers;
    std::vector<BenchmarkServerProcess> otherDatabaseServers;
    std::vector<BenchmarkListeningPort> listeningPorts;
    // Windows firewall on for the profiles "Domain", "Private", "Public"
    std::string firewallProfiles;
    std::string firewallNote;
    // idle time after which TCP sends keepalive probes on idle connections;
    // firewalls and NAT routers often drop idle connections much earlier
    std::optional<int> tcpKeepAliveSeconds;
    // operating system settings that matter for databases (name, value)
    std::vector<std::pair<std::string, std::string>> osSettings;
};

///
/// The way from this machine to a Firebird server over TCP/IP: name
/// resolution and the network adapter that carries the connection.
///
struct BenchmarkNetworkPath
{
    std::string serverHost;
    std::string serverAddress;      // resolved IP address used for the route
    std::vector<std::string> serverAddresses;   // all resolved addresses
    double resolveMs = -1.0;        // name resolution, -1 if not measured
    std::string localAddress;
    std::string adapterName;
    std::string adapterDescription; // e.g. the network card
    std::string adapterType;        // "Ethernet", "Wi-Fi", "VPN", "loopback", ...
    int64_t linkSpeedBitsPerSecond = 0;
    int mtu = 0;
    std::optional<bool> sameSubnet; // false: routed through a gateway
    std::string wifiSignal;         // Linux: signal level of the WLAN
    std::string note;               // why the adapter is unknown
};

/// A Firebird installation on this machine. Its client library can run the
/// engine inside FlameRobin (embedded), which needs no server connection
/// and no login.
struct BenchmarkFirebirdInstallation
{
    std::string directory;
    std::string clientLibrary;      // empty: the default client library
    std::string version;            // e.g. "5.0.4.1812"; empty if unknown
    std::string service;            // service name; empty if none
    bool running = false;           // the service of the installation runs
};

/// the installations whose engine plugin is present, running services first
std::vector<BenchmarkFirebirdInstallation> findBenchmarkFirebirdInstallations();

/// reads hardware and software of this machine; storagePath is a directory
/// on this machine whose drive is described (empty: none). Takes up to a
/// second (Windows Security Center).
BenchmarkSystemInfo collectBenchmarkSystemInfo(const std::string& storagePath);
/// resolves the server name and finds the adapter used to reach it; sends
/// no packets
BenchmarkNetworkPath probeBenchmarkNetworkPath(const std::string& host);
/// "Ethernet", "Wi-Fi", "VPN", ... from the adapter name and description
/// (VPN clients often appear as Ethernet adapters)
std::string classifyBenchmarkAdapter(const std::string& type,
    const std::string& name, const std::string& description);
/// tries TCP connections to the ports of a server, all at the same time,
/// and closes them at once; takes at most timeoutMs
std::vector<BenchmarkPortCheck> checkBenchmarkPorts(const std::string& host,
    const std::vector<int>& ports, int timeoutMs);
/// a Windows firewall rule ("v2.31|Action=Allow|Dir=In|LPort=3050|...")
/// that applies to inbound TCP connections to port (and program, if the
/// rule names one): 1 allows, -1 blocks, 0 does not apply
int matchBenchmarkFirewallRule(const std::string& rule, int port,
    const std::string& program, std::string* name);

// The parts that do not depend on the operating system, for the unit tests.

/// a raw SMBIOS structure table (without the entry point): machine, board,
/// BIOS, CPU sockets and speed, memory slots and modules
void parseBenchmarkSmbios(const uint8_t* table, size_t size, BenchmarkSystemInfo& info);
/// the 512 byte NVMe SMART / health information log page (log identifier 2)
std::vector<BenchmarkHealthValue> parseBenchmarkNvmeHealth(const uint8_t* log,
    size_t size, bool* warning);
/// the 512 byte ATA SMART data (attribute table at offset 2)
std::vector<BenchmarkHealthValue> parseBenchmarkAtaSmart(const uint8_t* data,
    size_t size, bool* warning);
/// name of a Windows power plan or power mode GUID; empty if unknown
std::string getBenchmarkPowerPlanName(const std::string& guid);
/// hypervisor from the machine manufacturer and model; empty for hardware
std::string getBenchmarkHypervisor(const std::string& manufacturer,
    const std::string& model);
/// value of a key in /proc/meminfo in bytes; -1 if missing
int64_t getBenchmarkMeminfoBytes(const std::string& meminfo, const std::string& key);
/// a value of /etc/os-release, without quotes
std::string getBenchmarkOsReleaseValue(const std::string& osRelease,
    const std::string& key);
/// the entry of /proc/self/mountinfo with the longest mount point that
/// contains path: file system type and mount options
bool findBenchmarkMount(const std::string& mountinfo, const std::string& path,
    std::string* fileSystem, std::string* options);
/// the selected entry of a sysfs choice list such as "mq-deadline [none]"
std::string getBenchmarkSelectedChoice(const std::string& choices);

} // namespace fr

#endif // FR_BENCHMARKSYSTEMINFO_H
