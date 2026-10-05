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

#include "engine/BenchmarkSystemInfo.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

#if defined(_WIN32)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <iphlpapi.h>
    #include <tlhelp32.h>
    #include <winioctl.h>
    #include <wbemidl.h>
    #include <oleauto.h>
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <fcntl.h>
    #include <ifaddrs.h>
    #include <netdb.h>
    #include <net/if.h>
    #include <netinet/in.h>
    #include <pwd.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
#endif
#if defined(__linux__)
    #include <filesystem>
    #include <fstream>
    #include <sys/stat.h>
    #include <sys/sysmacros.h>
    #include <sys/utsname.h>
#elif defined(__APPLE__)
    #include <filesystem>
    #include <sys/mount.h>
    #include <sys/param.h>
    #include <sys/sysctl.h>
    #include <sys/time.h>
    #include <ctime>
#endif

namespace fr
{

namespace
{

std::string trim(const std::string& s)
{
    const char* space = " \t\r\n";
    size_t first = s.find_first_not_of(space);
    if (first == std::string::npos)
        return std::string();
    size_t last = s.find_last_not_of(space);
    // firmware strings are often padded with repeated blanks
    std::string r;
    bool blank = false;
    for (size_t i = first; i <= last; ++i)
    {
        bool isBlank = s[i] == ' ' || s[i] == '\t';
        if (isBlank && blank)
            continue;
        r += isBlank ? ' ' : s[i];
        blank = isBlank;
    }
    return r;
}

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return char(std::tolower(c)); });
    return s;
}

bool contains(const std::string& s, const std::string& part)
{
    return s.find(part) != std::string::npos;
}

// firmware fills unused fields with texts like "To Be Filled By O.E.M."
std::string firmwareString(const std::string& s)
{
    std::string t = trim(s);
    std::string l = lower(t);
    const char* placeholders[] = { "to be filled", "default string",
        "system product name", "system manufacturer", "system version",
        "not specified", "not applicable", "o.e.m.", "unknown", "none",
        "undefined", "base board product name", "base board manufacturer" };
    for (const char* p : placeholders)
    {
        if (contains(l, p))
            return std::string();
    }
    return t;
}

uint16_t le16(const uint8_t* p)
{
    return uint16_t(p[0] | (p[1] << 8));
}

uint32_t le32(const uint8_t* p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16)
        | (uint32_t(p[3]) << 24);
}

// the 128 bit counters of the NVMe health log; a double is exact enough
double le128(const uint8_t* p)
{
    double v = 0.0;
    for (int i = 15; i >= 0; --i)
        v = v * 256.0 + p[i];
    return v;
}

std::string formatNumber(double v)
{
    // thousands separators make large counters readable
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f", v);
    std::string digits(buf);
    std::string r;
    int n = int(digits.size());
    for (int i = 0; i < n; ++i)
    {
        r += digits[i];
        if ((n - i - 1) % 3 == 0 && i < n - 1)
            r += ',';
    }
    return r;
}

// drive vendors use decimal units
std::string formatDecimalBytes(double bytes)
{
    const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };
    int u = 0;
    while (bytes >= 1000.0 && u < 5)
    {
        bytes /= 1000.0;
        ++u;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), u == 0 ? "%.0f %s" : "%.1f %s", bytes, units[u]);
    return buf;
}

const char* memoryTypeName(int type)
{
    switch (type)
    {
        case 0x0F: return "SDRAM";
        case 0x12: return "DDR";
        case 0x13: return "DDR2";
        case 0x14: return "DDR2 FB-DIMM";
        case 0x18: return "DDR3";
        case 0x1A: return "DDR4";
        case 0x1B: return "LPDDR";
        case 0x1C: return "LPDDR2";
        case 0x1D: return "LPDDR3";
        case 0x1E: return "LPDDR4";
        case 0x20: return "HBM";
        case 0x21: return "HBM2";
        case 0x22: return "DDR5";
        case 0x23: return "LPDDR5";
        case 0x24: return "HBM3";
        default: return "";
    }
}

const char* formFactorName(int formFactor)
{
    switch (formFactor)
    {
        case 0x09: return "DIMM";
        case 0x0B: return "soldered";
        case 0x0D: return "SODIMM";
        case 0x0F: return "FB-DIMM";
        case 0x10: return "die";
        case 0x11: return "CAMM";
        default: return "";
    }
}

} // namespace

void parseBenchmarkSmbios(const uint8_t* table, size_t size, BenchmarkSystemInfo& info)
{
    size_t pos = 0;
    while (pos + 4 <= size)
    {
        const uint8_t type = table[pos];
        const uint8_t length = table[pos + 1];
        if (length < 4 || pos + length > size)
            break;
        const uint8_t* s = table + pos;

        // the strings follow the formatted area; two zero bytes end them
        std::vector<std::string> strings;
        size_t p = pos + length;
        if (p + 1 < size && table[p] == 0 && table[p + 1] == 0)
            p += 2;
        else
        {
            while (p < size && table[p] != 0)
            {
                size_t start = p;
                while (p < size && table[p] != 0)
                    ++p;
                strings.emplace_back(reinterpret_cast<const char*>(table + start),
                    p - start);
                ++p;
            }
            ++p;
        }

        auto str = [&](size_t offset) -> std::string
        {
            if (offset >= length || s[offset] == 0 || s[offset] > strings.size())
                return std::string();
            return firmwareString(strings[s[offset] - 1]);
        };
        auto byteAt = [&](size_t offset) -> int
        {
            return offset < length ? s[offset] : -1;
        };
        auto wordAt = [&](size_t offset) -> int
        {
            return offset + 1 < length ? le16(s + offset) : -1;
        };
        auto dwordAt = [&](size_t offset) -> int64_t
        {
            return offset + 3 < length ? int64_t(le32(s + offset)) : -1;
        };

        switch (type)
        {
            case 0:     // BIOS
                info.biosVendor = str(0x04);
                info.biosVersion = str(0x05);
                info.biosDate = str(0x08);
                break;
            case 1:     // system
                info.manufacturer = str(0x04);
                info.model = str(0x05);
                break;
            case 2:     // base board
                info.boardManufacturer = str(0x04);
                info.boardProduct = str(0x05);
                break;
            case 4:     // processor; bit 6 of the status: socket populated
                if (byteAt(0x18) > 0 && (byteAt(0x18) & 0x40))
                {
                    ++info.cpuSockets;
                    int maxSpeed = wordAt(0x14);
                    if (maxSpeed > 0 && maxSpeed < 20000)
                        info.cpuMaxMHz = std::max(info.cpuMaxMHz, maxSpeed);
                    // the speed at boot, usually the base clock
                    int currentSpeed = wordAt(0x16);
                    if (currentSpeed > 0 && currentSpeed < 20000)
                        info.cpuBaseMHz = currentSpeed;
                    if (info.cpuName.empty())
                        info.cpuName = str(0x10);
                }
                break;
            case 16:    // physical memory array used as system memory
                if (byteAt(0x05) == 0x03 && wordAt(0x0D) > 0)
                    info.memorySlots += wordAt(0x0D);
                break;
            case 17:    // memory device
            {
                int sizeField = wordAt(0x0C);
                if (sizeField <= 0 || sizeField == 0xFFFF)
                    break;  // empty slot or unknown size
                BenchmarkMemoryModule module;
                if (sizeField == 0x7FFF)
                {
                    int64_t extended = dwordAt(0x1C);
                    if (extended <= 0)
                        break;
                    module.sizeBytes = (extended & 0x7FFFFFFF) * 1024 * 1024;
                }
                else if (sizeField & 0x8000)
                    module.sizeBytes = int64_t(sizeField & 0x7FFF) * 1024;
                else
                    module.sizeBytes = int64_t(sizeField) * 1024 * 1024;
                module.formFactor = formFactorName(byteAt(0x0E));
                module.locator = str(0x10);
                module.type = memoryTypeName(byteAt(0x12));
                int speed = wordAt(0x15);
                if (speed == 0xFFFF)
                    speed = int(dwordAt(0x54));
                module.speedMTs = std::max(speed, 0);
                module.manufacturer = str(0x17);
                module.partNumber = str(0x1A);
                int configured = wordAt(0x20);
                if (configured == 0xFFFF)
                    configured = int(dwordAt(0x58));
                module.configuredSpeedMTs = std::max(configured, 0);
                info.memoryModules.push_back(module);
                break;
            }
            default:
                break;
        }
        if (type == 127)    // end of table
            break;
        pos = p;
    }
}

std::vector<BenchmarkHealthValue> parseBenchmarkNvmeHealth(const uint8_t* log,
    size_t size, bool* warning)
{
    std::vector<BenchmarkHealthValue> r;
    if (size < 192)
        return r;
    bool anyWarning = false;
    auto add = [&](const std::string& name, const std::string& value, bool bad)
    {
        r.push_back({ name, value, bad });
        anyWarning = anyWarning || bad;
    };

    const int critical = log[0];
    std::string status = "OK";
    if (critical)
    {
        status.clear();
        const char* bits[] = { "spare below threshold", "temperature",
            "reliability degraded", "read-only", "volatile memory backup failed" };
        for (int i = 0; i < 5; ++i)
        {
            if (critical & (1 << i))
                status += (status.empty() ? "" : ", ") + std::string(bits[i]);
        }
    }
    add("Critical warnings", status, critical != 0);
    const int used = log[5];
    add("Wear (percentage used)", std::to_string(used) + " %", used >= 90);
    const int spare = log[3];
    const int spareThreshold = log[4];
    add("Available spare", std::to_string(spare) + " % (threshold "
        + std::to_string(spareThreshold) + " %)", spare < spareThreshold);
    const int kelvin = le16(log + 1);
    if (kelvin > 0)
    {
        int celsius = kelvin - 273;
        add("Temperature", std::to_string(celsius) + " \xC2\xB0" "C", celsius >= 70);
    }
    // data units are thousands of 512 byte blocks
    add("Data written", formatDecimalBytes(le128(log + 48) * 512000.0), false);
    add("Data read", formatDecimalBytes(le128(log + 32) * 512000.0), false);
    add("Power-on hours", formatNumber(le128(log + 128)), false);
    add("Power cycles", formatNumber(le128(log + 112)), false);
    add("Unsafe shutdowns", formatNumber(le128(log + 144)), false);
    const double mediaErrors = le128(log + 160);
    add("Media and data integrity errors", formatNumber(mediaErrors), mediaErrors > 0);
    add("Error log entries", formatNumber(le128(log + 176)), false);
    if (warning)
        *warning = anyWarning;
    return r;
}

std::vector<BenchmarkHealthValue> parseBenchmarkAtaSmart(const uint8_t* data,
    size_t size, bool* warning)
{
    std::vector<BenchmarkHealthValue> r;
    bool anyWarning = false;
    // 30 attributes of 12 bytes after the 2 byte revision: id, flags (2),
    // normalised value, worst value, raw value (6), reserved
    for (size_t offset = 2; offset + 12 <= size && offset < 2 + 30 * 12; offset += 12)
    {
        const uint8_t* a = data + offset;
        const int id = a[0];
        if (id == 0)
            continue;
        const int current = a[3];
        double raw = 0.0;
        for (int i = 10; i >= 5; --i)
            raw = raw * 256.0 + a[i];
        std::string name;
        std::string value;
        bool bad = false;
        switch (id)
        {
            case 5:
                name = "Reallocated sectors";
                value = formatNumber(raw);
                bad = raw > 0;
                break;
            case 9:
                name = "Power-on hours";
                // some drives keep minutes or other values in the upper bytes
                value = formatNumber(double(le32(a + 5)));
                break;
            case 12:
                name = "Power cycles";
                value = formatNumber(raw);
                break;
            case 177:
            case 231:
            case 233:
                name = "Remaining life";
                value = std::to_string(current) + " %";
                bad = current <= 10;
                break;
            case 194:
                name = "Temperature";
                value = std::to_string(a[5]) + " \xC2\xB0" "C";
                bad = a[5] >= 60;
                break;
            case 196:
                name = "Reallocation events";
                value = formatNumber(raw);
                bad = raw > 0;
                break;
            case 197:
                name = "Pending sectors";
                value = formatNumber(raw);
                bad = raw > 0;
                break;
            case 198:
                name = "Uncorrectable sectors";
                value = formatNumber(raw);
                bad = raw > 0;
                break;
            case 199:
                name = "Interface CRC errors (cable)";
                value = formatNumber(raw);
                bad = raw > 0;
                break;
            case 241:
                name = "Data written";
                value = formatDecimalBytes(raw * 512.0);
                break;
            default:
                continue;
        }
        if (std::any_of(r.begin(), r.end(),
            [&name](const BenchmarkHealthValue& v) { return v.name == name; }))
        {
            continue;
        }
        r.push_back({ name, value, bad });
        anyWarning = anyWarning || bad;
    }
    if (warning)
        *warning = anyWarning;
    return r;
}

std::string getBenchmarkPowerPlanName(const std::string& guid)
{
    static const std::map<std::string, std::string> names = {
        // power plans
        { "381b4222-f694-41f0-9685-ff5bb260df2e", "Balanced" },
        { "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c", "High performance" },
        { "a1841308-3541-4fab-bc81-f71556f20b4a", "Power saver" },
        { "e9a42b02-d5df-448d-aa00-03f14749eb61", "Ultimate performance" },
        // power modes of Windows 10 and 11
        { "961cc777-2547-4f9d-8174-7d86181b8a7a", "Best power efficiency" },
        { "00000000-0000-0000-0000-000000000000", "Balanced" },
        { "3af9b8d9-7c97-431d-ad78-34a8bfea439f", "Better performance" },
        { "ded574b5-45a0-4f42-8737-46345c09c238", "Best performance" }
    };
    std::string key = lower(trim(guid));
    if (!key.empty() && key.front() == '{')
        key = key.substr(1, key.size() - 2);
    auto it = names.find(key);
    return it == names.end() ? std::string() : it->second;
}

std::string getBenchmarkHypervisor(const std::string& manufacturer,
    const std::string& model)
{
    const std::string s = lower(manufacturer + " " + model);
    const std::pair<const char*, const char*> known[] = {
        { "vmware", "VMware" }, { "virtualbox", "VirtualBox" },
        { "innotek", "VirtualBox" }, { "qemu", "KVM / QEMU" }, { "kvm", "KVM / QEMU" },
        { "xen", "Xen" }, { "parallels", "Parallels" }, { "amazon ec2", "Amazon EC2" },
        { "google compute engine", "Google Compute Engine" },
        { "openstack", "OpenStack" }, { "bhyve", "bhyve" }, { "nutanix", "Nutanix AHV" },
        { "bochs", "Bochs" }
    };
    for (const auto& k : known)
    {
        if (contains(s, k.first))
            return k.second;
    }
    // Hyper-V and Azure virtual machines
    if (contains(s, "microsoft corporation") && contains(s, "virtual"))
        return "Hyper-V";
    return std::string();
}

int64_t getBenchmarkMeminfoBytes(const std::string& meminfo, const std::string& key)
{
    std::istringstream in(meminfo);
    std::string line;
    while (std::getline(in, line))
    {
        if (line.compare(0, key.size() + 1, key + ":") != 0)
            continue;
        long long kb = 0;
        if (std::sscanf(line.c_str() + key.size() + 1, "%lld", &kb) == 1)
            return int64_t(kb) * 1024;
    }
    return -1;
}

std::string getBenchmarkOsReleaseValue(const std::string& osRelease,
    const std::string& key)
{
    std::istringstream in(osRelease);
    std::string line;
    while (std::getline(in, line))
    {
        if (line.compare(0, key.size() + 1, key + "=") != 0)
            continue;
        std::string v = trim(line.substr(key.size() + 1));
        if (v.size() >= 2 && (v.front() == '"' || v.front() == '\'') && v.back() == v.front())
            v = v.substr(1, v.size() - 2);
        return v;
    }
    return std::string();
}

bool findBenchmarkMount(const std::string& mountinfo, const std::string& path,
    std::string* fileSystem, std::string* options)
{
    // "36 35 98:0 /root /mnt rw,noatime master:1 - ext4 /dev/sda1 rw,errors=..."
    std::istringstream in(mountinfo);
    std::string line;
    size_t best = 0;
    bool found = false;
    while (std::getline(in, line))
    {
        std::istringstream fields(line);
        std::vector<std::string> f;
        std::string field;
        while (fields >> field)
            f.push_back(field);
        auto dash = std::find(f.begin(), f.end(), "-");
        if (f.size() < 6 || dash == f.end() || dash + 3 > f.end())
            continue;
        std::string mountPoint = f[4];
        // spaces are escaped as \040
        for (size_t i; (i = mountPoint.find("\\040")) != std::string::npos; )
            mountPoint.replace(i, 4, " ");
        bool matches = mountPoint == "/" || path == mountPoint
            || (path.compare(0, mountPoint.size(), mountPoint) == 0
                && path.size() > mountPoint.size() && path[mountPoint.size()] == '/');
        if (!matches || (found && mountPoint.size() < best))
            continue;
        best = mountPoint.size();
        found = true;
        if (fileSystem)
            *fileSystem = *(dash + 1);
        if (options)
        {
            // mount and file system options, each once; the layer
            // directories of overlay file systems (containers) are noise
            options->clear();
            std::set<std::string> seen;
            std::istringstream all(f[5] + "," + *(dash + 3));
            std::string option;
            while (std::getline(all, option, ','))
            {
                if (option.empty() || contains(option, "dir=") || option.size() > 40
                    || !seen.insert(option).second)
                {
                    continue;
                }
                *options += (options->empty() ? "" : ",") + option;
            }
        }
    }
    return found;
}

std::string getBenchmarkSelectedChoice(const std::string& choices)
{
    size_t open = choices.find('[');
    size_t close = choices.find(']', open == std::string::npos ? 0 : open);
    if (open == std::string::npos || close == std::string::npos)
        return trim(choices);
    return choices.substr(open + 1, close - open - 1);
}

int matchBenchmarkFirewallRule(const std::string& rule, int port,
    const std::string& program, std::string* name)
{
    // "v2.31|Action=Allow|Active=TRUE|Dir=In|Protocol=6|LPort=3050|App=...|Name=...|"
    std::map<std::string, std::vector<std::string>> fields;
    std::istringstream in(rule);
    std::string part;
    while (std::getline(in, part, '|'))
    {
        size_t equals = part.find('=');
        if (equals != std::string::npos)
            fields[lower(part.substr(0, equals))].push_back(part.substr(equals + 1));
    }
    auto first = [&fields](const char* key) -> std::string
    {
        auto it = fields.find(key);
        return it == fields.end() || it->second.empty() ? std::string() : it->second.front();
    };
    if (lower(first("active")) != "true" || lower(first("dir")) != "in")
        return 0;
    // rules for an app package or a service do not apply to the server
    if (!first("pfn").empty() || !first("apppkgid").empty() || !first("svc").empty())
        return 0;
    const std::string protocol = first("protocol");
    if (!protocol.empty() && protocol != "6")   // 6: TCP
        return 0;
    auto ports = fields.find("lport");
    if (ports != fields.end())
    {
        bool matches = false;
        for (const std::string& list : ports->second)
        {
            std::istringstream items(list);
            std::string item;
            while (std::getline(items, item, ','))
            {
                int low = 0, high = 0;
                int n = std::sscanf(item.c_str(), "%d-%d", &low, &high);
                if ((n == 1 && low == port) || (n == 2 && port >= low && port <= high))
                    matches = true;
            }
        }
        if (!matches)
            return 0;
    }
    const std::string app = lower(first("app"));
    if (!app.empty())
    {
        // paths with environment variables are compared by file name
        auto fileName = [](const std::string& path)
        {
            size_t slash = path.find_last_of("\\/");
            return slash == std::string::npos ? path : path.substr(slash + 1);
        };
        const std::string p = lower(program);
        if (p.empty() || (contains(app, "%") ? fileName(app) != fileName(p) : app != p))
            return 0;
    }
    if (name)
    {
        // built-in rules have resource names such as "@FirewallAPI.dll,-28502"
        *name = first("name");
        if (!name->empty() && name->front() == '@')
            *name = "a built-in Windows rule";
    }
    const std::string action = lower(first("action"));
    return action == "allow" ? 1 : action == "block" ? -1 : 0;
}

namespace
{

#if defined(_WIN32) || defined(__linux__)

#if defined(_WIN32)
// the executable name of a service command line
std::string executableName(const std::string& commandLine)
{
    std::string s = trim(commandLine);
    if (!s.empty() && s.front() == '"')
        s = s.substr(1, s.find('"', 1) == std::string::npos ? std::string::npos
            : s.find('"', 1) - 1);
    else
    {
        size_t exe = lower(s).find(".exe");
        if (exe != std::string::npos)
            s = s.substr(0, exe + 4);
    }
    size_t slash = s.find_last_of("\\/");
    return lower(slash == std::string::npos ? s : s.substr(slash + 1));
}
#endif

std::string directoryOf(const std::string& commandLine)
{
    std::string s = trim(commandLine);
    if (!s.empty() && s.front() == '"')
        s = s.substr(1, s.find('"', 1) == std::string::npos ? std::string::npos
            : s.find('"', 1) - 1);
    size_t slash = s.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : s.substr(0, slash);
}

// Firebird server executables; the guardian only watches the server
bool isFirebirdServer(const std::string& exe)
{
    return exe == "firebird.exe" || exe == "fbserver.exe"
        || exe == "fb_inet_server.exe" || exe == "fb_smp_server.exe"
        || exe == "firebird" || exe == "fbserver" || exe == "fb_inet_server"
        || exe == "fb_smp_server";
}

// other database servers that share CPU, RAM and drives with Firebird
std::string otherDatabaseServer(const std::string& exe)
{
    const std::pair<const char*, const char*> known[] = {
        { "sqlservr", "Microsoft SQL Server" }, { "postgres", "PostgreSQL" },
        { "mysqld", "MySQL" }, { "mariadbd", "MariaDB" }, { "oracle", "Oracle" },
        { "mongod", "MongoDB" }, { "ibserver", "InterBase" }
    };
    std::string name = exe;
    if (name.size() > 4 && name.compare(name.size() - 4, 4, ".exe") == 0)
        name.resize(name.size() - 4);
    for (const auto& k : known)
    {
        if (name == k.first)
            return k.second;
    }
    return std::string();
}

void addServer(std::vector<BenchmarkServerProcess>& list, const std::string& name,
    const std::string& detail, bool running)
{
    for (auto& s : list)
    {
        if (s.name == name && s.detail == detail)
        {
            s.running = s.running || running;
            return;
        }
    }
    list.push_back({ name, detail, running });
}

// ports of database servers and the usual range of Firebird instances
bool isInterestingPort(int port, const std::string& process)
{
    return (port >= 3050 && port <= 3060) || isFirebirdServer(process)
        || !otherDatabaseServer(process).empty();
}

#endif

#if defined(_WIN32)

std::string utf8(const std::wstring& w)
{
    if (w.empty())
        return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), nullptr, 0,
        nullptr, nullptr);
    std::string s(size_t(std::max(n, 0)), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}

std::wstring wide(const std::string& s)
{
    if (s.empty())
        return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), nullptr, 0);
    std::wstring w(size_t(std::max(n, 0)), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), int(s.size()), &w[0], n);
    return w;
}

// registry access in the 64 bit view, also from a 32 bit FlameRobin
class RegistryKey
{
public:
    RegistryKey(const wchar_t* path)
    {
        statusM = RegOpenKeyExW(HKEY_LOCAL_MACHINE, path, 0,
            KEY_READ | KEY_WOW64_64KEY, &keyM);
    }
    ~RegistryKey()
    {
        if (statusM == ERROR_SUCCESS)
            RegCloseKey(keyM);
    }
    RegistryKey(const RegistryKey&) = delete;
    RegistryKey& operator=(const RegistryKey&) = delete;

    bool isOpen() const { return statusM == ERROR_SUCCESS; }
    bool isAccessDenied() const { return statusM == ERROR_ACCESS_DENIED; }

    std::optional<std::string> getString(const wchar_t* name) const
    {
        if (!isOpen())
            return std::nullopt;
        DWORD type = 0, size = 0;
        if (RegQueryValueExW(keyM, name, nullptr, &type, nullptr, &size) != ERROR_SUCCESS
            || (type != REG_SZ && type != REG_EXPAND_SZ))
        {
            return std::nullopt;
        }
        std::wstring value(size / sizeof(wchar_t) + 1, L'\0');
        if (RegQueryValueExW(keyM, name, nullptr, nullptr,
            reinterpret_cast<BYTE*>(&value[0]), &size) != ERROR_SUCCESS)
        {
            return std::nullopt;
        }
        value.resize(wcsnlen(value.c_str(), value.size()));
        return utf8(value);
    }

    std::optional<DWORD> getDword(const wchar_t* name) const
    {
        if (!isOpen())
            return std::nullopt;
        DWORD type = 0, value = 0, size = sizeof(value);
        if (RegQueryValueExW(keyM, name, nullptr, &type,
            reinterpret_cast<BYTE*>(&value), &size) != ERROR_SUCCESS || type != REG_DWORD)
        {
            return std::nullopt;
        }
        return value;
    }

    // name and text of every string value
    std::vector<std::pair<std::string, std::string>> getStringValues() const
    {
        std::vector<std::pair<std::string, std::string>> values;
        for (DWORD i = 0; isOpen(); ++i)
        {
            wchar_t name[256];
            DWORD nameLength = DWORD(std::size(name));
            DWORD type = 0, size = 0;
            LONG rc = RegEnumValueW(keyM, i, name, &nameLength, nullptr, &type,
                nullptr, &size);
            if (rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
                break;
            if (type != REG_SZ)
                continue;
            std::wstring data(size / sizeof(wchar_t) + 1, L'\0');
            nameLength = DWORD(std::size(name));
            if (RegEnumValueW(keyM, i, name, &nameLength, nullptr, nullptr,
                reinterpret_cast<BYTE*>(&data[0]), &size) != ERROR_SUCCESS)
            {
                continue;
            }
            data.resize(wcsnlen(data.c_str(), data.size()));
            values.push_back({ utf8(std::wstring(name, nameLength)), utf8(data) });
        }
        return values;
    }

    std::vector<std::string> getValueNames() const
    {
        std::vector<std::string> names;
        for (DWORD i = 0; isOpen(); ++i)
        {
            wchar_t name[1024];
            DWORD length = DWORD(std::size(name));
            if (RegEnumValueW(keyM, i, name, &length, nullptr, nullptr, nullptr,
                nullptr) != ERROR_SUCCESS)
            {
                break;
            }
            names.push_back(utf8(std::wstring(name, length)));
        }
        return names;
    }

private:
    HKEY keyM = nullptr;
    LONG statusM = ERROR_FILE_NOT_FOUND;
};

class Handle
{
public:
    explicit Handle(HANDLE h) : hM(h) {}
    ~Handle()
    {
        if (isValid())
            CloseHandle(hM);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    bool isValid() const { return hM != INVALID_HANDLE_VALUE && hM != nullptr; }
    HANDLE get() const { return hM; }
private:
    HANDLE hM;
};

void readWindowsSmbios(BenchmarkSystemInfo& info)
{
    const DWORD signature = 0x52534D42;     // 'RSMB'
    UINT size = GetSystemFirmwareTable(signature, 0, nullptr, 0);
    if (size <= 8)
        return;
    std::vector<uint8_t> data(size);
    if (GetSystemFirmwareTable(signature, 0, data.data(), size) != size)
        return;
    // RawSMBIOSData: four version bytes and the table length before the table
    const uint32_t length = le32(data.data() + 4);
    parseBenchmarkSmbios(data.data() + 8, std::min<size_t>(length, size - 8), info);
}

void readWindowsCpu(BenchmarkSystemInfo& info)
{
    RegistryKey cpu(L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0");
    if (auto name = cpu.getString(L"ProcessorNameString"))
        info.cpuName = trim(*name);
    // a measured value; the firmware knows the base clock better
    if (auto mhz = cpu.getDword(L"~MHz"); mhz && info.cpuBaseMHz == 0)
        info.cpuBaseMHz = int(*mhz);

    DWORD length = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &length);
    if (length == 0)
        return;
    std::vector<uint8_t> buffer(length);
    auto* first = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());
    if (!GetLogicalProcessorInformationEx(RelationAll, first, &length))
        return;
    int sockets = 0;
    std::vector<int> efficiencyClasses;
    for (DWORD offset = 0; offset < length; )
    {
        auto* entry = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(
            buffer.data() + offset);
        if (entry->Size == 0)
            break;
        switch (entry->Relationship)
        {
            case RelationProcessorCore:
                ++info.cpuCores;
                efficiencyClasses.push_back(entry->Processor.EfficiencyClass);
                for (WORD g = 0; g < entry->Processor.GroupCount; ++g)
                {
                    KAFFINITY mask = entry->Processor.GroupMask[g].Mask;
                    for (; mask; mask &= mask - 1)
                        ++info.cpuThreads;
                }
                break;
            case RelationProcessorPackage:
                ++sockets;
                break;
            case RelationCache:
                if (entry->Cache.Level == 1)
                    info.cacheL1Bytes += entry->Cache.CacheSize;
                else if (entry->Cache.Level == 2)
                    info.cacheL2Bytes += entry->Cache.CacheSize;
                else if (entry->Cache.Level == 3)
                    info.cacheL3Bytes += entry->Cache.CacheSize;
                break;
            default:
                break;
        }
        offset += entry->Size;
    }
    if (sockets > 0)
        info.cpuSockets = sockets;
    // hybrid CPUs: the performance cores have the highest efficiency class
    if (!efficiencyClasses.empty())
    {
        int highest = *std::max_element(efficiencyClasses.begin(), efficiencyClasses.end());
        if (highest > 0)
        {
            info.cpuPerformanceCores = int(std::count(efficiencyClasses.begin(),
                efficiencyClasses.end(), highest));
            info.cpuEfficiencyCores = info.cpuCores - info.cpuPerformanceCores;
        }
    }
}

void readWindowsMemory(BenchmarkSystemInfo& info)
{
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx(&status))
        return;
    info.memoryTotalBytes = int64_t(status.ullTotalPhys);
    info.memoryAvailableBytes = int64_t(status.ullAvailPhys);
    // the commit limit is the physical memory plus the page files
    if (status.ullTotalPageFile > status.ullTotalPhys)
        info.swapTotalBytes = int64_t(status.ullTotalPageFile - status.ullTotalPhys);
}

bool queryStorageProperty(HANDLE device, STORAGE_PROPERTY_ID id, void* out, DWORD size,
    DWORD* error = nullptr)
{
    STORAGE_PROPERTY_QUERY query{};
    query.PropertyId = id;
    query.QueryType = PropertyStandardQuery;
    DWORD returned = 0;
    if (!DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query),
        out, size, &returned, nullptr) || returned == 0)
    {
        if (error)
            *error = GetLastError();
        return false;
    }
    return true;
}

const char* busTypeName(int busType)
{
    switch (busType)
    {
        case 1: return "SCSI";
        case 2: return "ATAPI";
        case 3: return "ATA";
        case 6: return "Fibre Channel";
        case 7: return "USB";
        case 8: return "RAID";
        case 9: return "iSCSI";
        case 10: return "SAS";
        case 11: return "SATA";
        case 12: return "SD card";
        case 13: return "MMC";
        case 14: return "virtual";
        case 15: return "virtual disk file";
        case 16: return "Storage Spaces";
        case 17: return "NVMe";
        case 18: return "storage class memory";
        case 19: return "UFS";
        default: return "";
    }
}

// The NVMe health log through the storage driver (Windows 10 and later);
// the structures are declared here, older SDKs do not have them.
struct NvmeProtocolData
{
    DWORD protocolType;
    DWORD dataType;
    DWORD requestValue;
    DWORD requestSubValue;
    DWORD dataOffset;
    DWORD dataLength;
    DWORD fixedReturnStatus;
    DWORD requestSubValue2;
    DWORD requestSubValue3;
    DWORD requestSubValue4;
};

bool readNvmeHealth(HANDLE device, BenchmarkSystemInfo& info, DWORD* error)
{
    const DWORD logSize = 512;
    const DWORD propertyIdProtocolSpecific = 50;    // StorageDeviceProtocolSpecificProperty
    const DWORD headerSize = 2 * sizeof(DWORD);     // PropertyId, QueryType
    std::vector<uint8_t> buffer(headerSize + sizeof(NvmeProtocolData) + logSize);
    DWORD* query = reinterpret_cast<DWORD*>(buffer.data());
    query[0] = propertyIdProtocolSpecific;
    query[1] = 0;   // PropertyStandardQuery
    auto* request = reinterpret_cast<NvmeProtocolData*>(buffer.data() + headerSize);
    request->protocolType = 3;      // ProtocolTypeNvme
    request->dataType = 2;          // NVMeDataTypeLogPage
    request->requestValue = 2;      // SMART / health information
    request->requestSubValue = 0;
    request->dataOffset = sizeof(NvmeProtocolData);
    request->dataLength = logSize;
    DWORD returned = 0;
    if (!DeviceIoControl(device, IOCTL_STORAGE_QUERY_PROPERTY, buffer.data(),
        DWORD(buffer.size()), buffer.data(), DWORD(buffer.size()), &returned, nullptr))
    {
        *error = GetLastError();
        return false;
    }
    // the answer starts with Version and Size, followed by the request
    // with the offset of the data relative to the request
    auto* answer = reinterpret_cast<NvmeProtocolData*>(buffer.data() + headerSize);
    size_t dataStart = headerSize + answer->dataOffset;
    if (answer->dataLength < 192 || dataStart + answer->dataLength > buffer.size())
    {
        *error = ERROR_INVALID_DATA;
        return false;
    }
    bool warning = false;
    info.storageHealth = parseBenchmarkNvmeHealth(buffer.data() + dataStart,
        answer->dataLength, &warning);
    info.storageHealthWarning = warning;
    return true;
}

bool readAtaSmart(HANDLE device, BenchmarkSystemInfo& info, DWORD* error)
{
    STORAGE_PREDICT_FAILURE failure{};
    DWORD returned = 0;
    if (!DeviceIoControl(device, IOCTL_STORAGE_PREDICT_FAILURE, nullptr, 0, &failure,
        sizeof(failure), &returned, nullptr))
    {
        *error = GetLastError();
        return false;
    }
    bool warning = false;
    info.storageHealth = parseBenchmarkAtaSmart(failure.VendorSpecific,
        sizeof(failure.VendorSpecific), &warning);
    if (failure.PredictFailure)
    {
        info.storageHealth.insert(info.storageHealth.begin(),
            { "Failure predicted", "yes", true });
        warning = true;
    }
    info.storageHealthWarning = warning;
    return !info.storageHealth.empty();
}

void readWindowsDrive(const std::string& path, BenchmarkSystemInfo& info)
{
    std::wstring wpath = wide(path);
    wchar_t volume[MAX_PATH];
    if (!GetVolumePathNameW(wpath.c_str(), volume, MAX_PATH))
        return;
    info.storagePath = path;
    wchar_t fileSystem[64] = L"";
    if (GetVolumeInformationW(volume, nullptr, 0, nullptr, nullptr, nullptr,
        fileSystem, DWORD(std::size(fileSystem))))
    {
        info.fileSystem = utf8(fileSystem);
    }
    DWORD sectorsPerCluster = 0, bytesPerSector = 0, freeClusters = 0, clusters = 0;
    if (GetDiskFreeSpaceW(volume, &sectorsPerCluster, &bytesPerSector, &freeClusters,
        &clusters))
    {
        info.fileSystemClusterBytes = int64_t(sectorsPerCluster) * bytesPerSector;
    }
    if (GetDriveTypeW(volume) == DRIVE_REMOTE)
    {
        info.storageBus = "network share";
        return;
    }

    wchar_t volumeName[MAX_PATH];
    if (!GetVolumeNameForVolumeMountPointW(volume, volumeName, MAX_PATH))
        return;
    std::wstring volumeDevice(volumeName);
    if (!volumeDevice.empty() && volumeDevice.back() == L'\\')
        volumeDevice.pop_back();
    // no access rights are needed for the queries below
    Handle volumeHandle(CreateFileW(volumeDevice.c_str(), 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
    if (!volumeHandle.isValid())
        return;
    uint8_t extentBuffer[sizeof(VOLUME_DISK_EXTENTS) + 8 * sizeof(DISK_EXTENT)];
    DWORD returned = 0;
    if (!DeviceIoControl(volumeHandle.get(), IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        nullptr, 0, extentBuffer, sizeof(extentBuffer), &returned, nullptr))
    {
        return;
    }
    auto* extents = reinterpret_cast<VOLUME_DISK_EXTENTS*>(extentBuffer);
    if (extents->NumberOfDiskExtents == 0)
        return;
    std::wstring drive = L"\\\\.\\PhysicalDrive"
        + std::to_wstring(extents->Extents[0].DiskNumber);
    Handle device(CreateFileW(drive.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr, OPEN_EXISTING, 0, nullptr));
    if (!device.isValid())
        return;

    uint8_t descriptorBuffer[1024] = {};
    if (queryStorageProperty(device.get(), StorageDeviceProperty, descriptorBuffer,
        sizeof(descriptorBuffer)))
    {
        auto* d = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(descriptorBuffer);
        auto text = [&](DWORD offset) -> std::string
        {
            if (offset == 0 || offset >= sizeof(descriptorBuffer))
                return std::string();
            return trim(reinterpret_cast<const char*>(descriptorBuffer + offset));
        };
        std::string vendor = text(d->VendorIdOffset);
        std::string product = text(d->ProductIdOffset);
        // NVMe drives often have no vendor, or the product contains it
        if (vendor.empty() || lower(vendor) == "nvme"
            || lower(product).compare(0, vendor.size(), lower(vendor)) == 0)
        {
            info.storageModel = product;
        }
        else
            info.storageModel = vendor + " " + product;
        info.storageFirmware = text(d->ProductRevisionOffset);
        info.storageBus = busTypeName(d->BusType);
    }
    if (extents->NumberOfDiskExtents > 1)
        info.storageBus += info.storageBus.empty() ? "spanned volume" : ", spanned volume";

    DEVICE_SEEK_PENALTY_DESCRIPTOR seek{};
    if (queryStorageProperty(device.get(), StorageDeviceSeekPenaltyProperty, &seek,
        sizeof(seek)))
    {
        info.storageMedia = seek.IncursSeekPenalty ? "HDD" : "SSD";
    }
    DEVICE_TRIM_DESCRIPTOR trimDescriptor{};
    if (queryStorageProperty(device.get(), StorageDeviceTrimProperty, &trimDescriptor,
        sizeof(trimDescriptor)))
    {
        info.trimEnabled = trimDescriptor.TrimEnabled != 0;
    }
    STORAGE_WRITE_CACHE_PROPERTY cache{};
    if (queryStorageProperty(device.get(), StorageDeviceWriteCacheProperty, &cache,
        sizeof(cache)))
    {
        if (cache.WriteCacheEnabled == WriteCacheEnabled)
            info.writeCacheEnabled = true;
        else if (cache.WriteCacheEnabled == WriteCacheDisabled)
            info.writeCacheEnabled = false;
        // "Turn off Windows write-cache buffer flushing" in the device
        // manager: flush requests (forced writes) are ignored
        info.writeCachePowerProtected = cache.UserDefinedPowerProtection != 0;
    }
    DISK_GEOMETRY_EX geometry{};
    if (DeviceIoControl(device.get(), IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, nullptr, 0,
        &geometry, sizeof(geometry), &returned, nullptr))
    {
        info.storageSizeBytes = int64_t(geometry.DiskSize.QuadPart);
    }

    DWORD error = 0;
    bool health = info.storageBus == "NVMe" ? readNvmeHealth(device.get(), info, &error)
        : readAtaSmart(device.get(), info, &error);
    if (!health)
    {
        info.storageHealthNote = error == ERROR_ACCESS_DENIED
            ? "needs administrator rights"
            : "not reported by the drive or its driver";
    }
    for (const auto& v : info.storageHealth)
    {
        int celsius = 0;
        if (v.name == "Temperature" && std::sscanf(v.value.c_str(), "%d", &celsius) == 1)
            info.storageTemperatureC = celsius;
    }
}

void readWindowsVersion(BenchmarkSystemInfo& info)
{
    RegistryKey version(L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
    std::string product = version.getString(L"ProductName").value_or("Windows");
    std::string build = version.getString(L"CurrentBuildNumber").value_or("");
    // Windows 11 still reports "Windows 10" as product name
    if (std::atoi(build.c_str()) >= 22000 && product.compare(0, 10, "Windows 10") == 0)
        product.replace(0, 10, "Windows 11");
    info.osName = product;
    std::string display = version.getString(L"DisplayVersion").value_or("");
    std::string text = display;
    if (!build.empty())
    {
        text += (text.empty() ? "" : ", ") + std::string("build ") + build;
        if (auto ubr = version.getDword(L"UBR"))
            text += "." + std::to_string(*ubr);
    }
    info.osVersion = text;

    SYSTEM_INFO system{};
    GetNativeSystemInfo(&system);
    switch (system.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64: info.architecture = "x64"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: info.architecture = "arm64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: info.architecture = "x86"; break;
        default: break;
    }
    info.uptimeSeconds = int64_t(GetTickCount64() / 1000);
}

void readWindowsPowerPlan(BenchmarkSystemInfo& info)
{
    RegistryKey schemes(L"SYSTEM\\CurrentControlSet\\Control\\Power\\User\\PowerSchemes");
    std::string plan;
    if (auto guid = schemes.getString(L"ActivePowerScheme"))
    {
        plan = getBenchmarkPowerPlanName(*guid);
        if (plan.empty())
            plan = "custom power plan";
    }
    // the power mode of Windows 10 and 11, for mains and battery power
    SYSTEM_POWER_STATUS status{};
    bool battery = GetSystemPowerStatus(&status) && status.ACLineStatus == 0;
    if (auto overlay = schemes.getString(battery ? L"ActiveOverlayDcPowerScheme"
        : L"ActiveOverlayAcPowerScheme"))
    {
        std::string mode = getBenchmarkPowerPlanName(*overlay);
        if (!mode.empty())
            plan += (plan.empty() ? "" : ", ") + std::string("mode ") + mode;
    }
    info.powerPlan = plan;
}

std::string normalisedWindowsPath(std::string path)
{
    std::replace(path.begin(), path.end(), '/', '\\');
    path = lower(path);
    if (!path.empty() && path.back() != '\\')
        path += '\\';
    return path;
}

void readWindowsDefender(BenchmarkSystemInfo& info)
{
    // the policy wins over the local setting
    const wchar_t* realtimeKeys[] = {
        L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection",
        L"SOFTWARE\\Microsoft\\Windows Defender\\Real-Time Protection" };
    for (const wchar_t* path : realtimeKeys)
    {
        RegistryKey key(path);
        if (auto disabled = key.getDword(L"DisableRealtimeMonitoring"))
        {
            info.defenderRealtime = *disabled == 0;
            break;
        }
        if (key.isOpen() && !info.defenderRealtime && path == realtimeKeys[1])
            info.defenderRealtime = true;
    }
    RegistryKey folders(L"SOFTWARE\\Microsoft\\Windows Defender\\Windows Defender "
        L"Exploit Guard\\Controlled Folder Access");
    if (auto enabled = folders.getDword(L"EnableControlledFolderAccess"))
        info.controlledFolderAccess = *enabled == 1;

    if (info.storagePath.empty())
        return;
    // the exclusion lists can only be read by administrators
    const wchar_t* exclusionKeys[] = {
        L"SOFTWARE\\Microsoft\\Windows Defender\\Exclusions\\Paths",
        L"SOFTWARE\\Policies\\Microsoft\\Windows Defender\\Exclusions\\Paths" };
    const std::string target = normalisedWindowsPath(info.storagePath);
    bool readable = false;
    for (const wchar_t* path : exclusionKeys)
    {
        RegistryKey key(path);
        if (!key.isOpen())
            continue;
        readable = true;
        for (const std::string& excluded : key.getValueNames())
        {
            std::string e = normalisedWindowsPath(excluded);
            if (target.compare(0, e.size(), e) == 0)
                info.storagePathExcluded = true;
        }
    }
    if (readable && !info.storagePathExcluded)
        info.storagePathExcluded = false;
}

// the antivirus products registered in the Windows Security Center; Windows
// Server has no Security Center
void readWindowsAntivirus(BenchmarkSystemInfo& info)
{
    static const GUID clsidWbemLocator = { 0x4590f811, 0x1d3a, 0x11d0,
        { 0x89, 0x1f, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24 } };
    static const GUID iidWbemLocator = { 0xdc12a687, 0x737f, 0x11cf,
        { 0x88, 0x4d, 0x00, 0xaa, 0x00, 0x4b, 0x2e, 0x24 } };

    HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    // RPC_E_CHANGED_MODE: COM is already initialised on this thread, which
    // works as well but must not be undone here
    const bool uninitialize = SUCCEEDED(init);
    IWbemLocator* locator = nullptr;
    IWbemServices* services = nullptr;
    IEnumWbemClassObject* rows = nullptr;
    BSTR space = SysAllocString(L"ROOT\\SecurityCenter2");
    BSTR language = SysAllocString(L"WQL");
    BSTR query = SysAllocString(L"SELECT displayName, productState FROM AntiVirusProduct");
    bool queried = false;
    if (SUCCEEDED(CoCreateInstance(clsidWbemLocator, nullptr, CLSCTX_INPROC_SERVER,
            iidWbemLocator, reinterpret_cast<void**>(&locator)))
        && SUCCEEDED(locator->ConnectServer(space, nullptr, nullptr, nullptr, 0,
            nullptr, nullptr, &services))
        && SUCCEEDED(CoSetProxyBlanket(services, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE,
            nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
            EOAC_NONE))
        && SUCCEEDED(services->ExecQuery(language, query,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &rows)))
    {
        queried = true;
        for (;;)
        {
            IWbemClassObject* row = nullptr;
            ULONG count = 0;
            if (FAILED(rows->Next(3000, 1, &row, &count)) || count == 0)
                break;
            VARIANT name, state;
            VariantInit(&name);
            VariantInit(&state);
            if (SUCCEEDED(row->Get(L"displayName", 0, &name, nullptr, nullptr))
                && name.vt == VT_BSTR)
            {
                std::string product = utf8(name.bstrVal);
                if (SUCCEEDED(row->Get(L"productState", 0, &state, nullptr, nullptr))
                    && (state.vt == VT_I4 || state.vt == VT_UI4))
                {
                    // 0x1000: scanning enabled, 0x10: signatures out of date
                    const unsigned value = unsigned(state.lVal);
                    const bool on = (value & 0x1000) != 0;
                    product += on ? " (on" : " (off";
                    product += on && (value & 0x10) ? ", out of date)" : ")";
                }
                info.antivirus.push_back(product);
            }
            VariantClear(&name);
            VariantClear(&state);
            row->Release();
        }
    }
    if (rows)
        rows->Release();
    if (services)
        services->Release();
    if (locator)
        locator->Release();
    SysFreeString(space);
    SysFreeString(language);
    SysFreeString(query);
    if (uninitialize)
        CoUninitialize();
    if (!queried)
        info.antivirusNote = "not reported (no Windows Security Center, e.g. on Windows Server)";
}

void readWindowsServices(BenchmarkSystemInfo& info)
{
    SC_HANDLE manager = OpenSCManagerW(nullptr, nullptr,
        SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
    if (!manager)
        return;
    DWORD needed = 0, count = 0, resume = 0;
    EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_STATE_ALL, nullptr, 0, &needed, &count, &resume, nullptr);
    std::vector<uint8_t> buffer(needed + 4096);
    resume = 0;
    if (EnumServicesStatusExW(manager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32,
        SERVICE_STATE_ALL, buffer.data(), DWORD(buffer.size()), &needed, &count,
        &resume, nullptr))
    {
        auto* list = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
        for (DWORD i = 0; i < count; ++i)
        {
            SC_HANDLE service = OpenServiceW(manager, list[i].lpServiceName,
                SERVICE_QUERY_CONFIG);
            if (!service)
                continue;
            DWORD size = 0;
            QueryServiceConfigW(service, nullptr, 0, &size);
            std::vector<uint8_t> config(size + 16);
            auto* c = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(config.data());
            if (size > 0 && QueryServiceConfigW(service, c, DWORD(config.size()), &size)
                && c->lpBinaryPathName)
            {
                const std::string command = utf8(c->lpBinaryPathName);
                const std::string exe = executableName(command);
                const bool running = list[i].ServiceStatusProcess.dwCurrentState
                    == SERVICE_RUNNING;
                const std::string name = utf8(list[i].lpServiceName);
                if (isFirebirdServer(exe))
                    addServer(info.firebirdServers, name, directoryOf(command), running);
                else
                {
                    std::string other = otherDatabaseServer(exe);
                    if (!other.empty())
                        addServer(info.otherDatabaseServers, other + " (" + name + ")",
                            directoryOf(command), running);
                }
            }
            CloseServiceHandle(service);
        }
    }
    CloseServiceHandle(manager);
}

// listening TCP ports of database servers and in the Firebird range, with
// the Windows firewall rules that apply to them
void readWindowsPorts(BenchmarkSystemInfo& info)
{
    // process names and paths; the snapshot needs no rights
    std::map<DWORD, std::string> names;
    Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (snapshot.isValid() && Process32FirstW(snapshot.get(), &entry))
    {
        do
            names[entry.th32ProcessID] = lower(utf8(entry.szExeFile));
        while (Process32NextW(snapshot.get(), &entry));
    }
    auto pathOf = [](DWORD pid) -> std::string
    {
        Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        wchar_t path[MAX_PATH];
        DWORD size = DWORD(std::size(path));
        if (process.isValid() && QueryFullProcessImageNameW(process.get(), 0, path, &size))
            return utf8(std::wstring(path, size));
        return std::string();
    };

    using GetExtendedTcpTableFunction = DWORD (WINAPI*)(PVOID, PDWORD, BOOL, ULONG,
        TCP_TABLE_CLASS, ULONG);
    HMODULE library = LoadLibraryW(L"iphlpapi.dll");
    auto getTable = library ? reinterpret_cast<GetExtendedTcpTableFunction>(
        reinterpret_cast<void*>(GetProcAddress(library, "GetExtendedTcpTable"))) : nullptr;
    struct Listener
    {
        int port;
        std::string address;
        DWORD pid;
    };
    std::vector<Listener> listeners;
    for (ULONG family : { ULONG(AF_INET), ULONG(AF_INET6) })
    {
        DWORD size = 0;
        if (!getTable || getTable(nullptr, &size, FALSE, family,
            TCP_TABLE_OWNER_PID_LISTENER, 0) != ERROR_INSUFFICIENT_BUFFER)
        {
            continue;
        }
        std::vector<uint8_t> buffer(size + 1024);
        size = DWORD(buffer.size());
        if (getTable(buffer.data(), &size, FALSE, family, TCP_TABLE_OWNER_PID_LISTENER,
            0) != NO_ERROR)
        {
            continue;
        }
        if (family == AF_INET)
        {
            auto* table = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(buffer.data());
            for (DWORD i = 0; i < table->dwNumEntries; ++i)
            {
                const auto& row = table->table[i];
                const uint8_t* a = reinterpret_cast<const uint8_t*>(&row.dwLocalAddr);
                std::string address = row.dwLocalAddr == 0 ? "all addresses"
                    : std::to_string(a[0]) + "." + std::to_string(a[1]) + "."
                        + std::to_string(a[2]) + "." + std::to_string(a[3]);
                listeners.push_back({ ntohs(u_short(row.dwLocalPort)), address,
                    row.dwOwningPid });
            }
        }
        else
        {
            auto* table = reinterpret_cast<MIB_TCP6TABLE_OWNER_PID*>(buffer.data());
            for (DWORD i = 0; i < table->dwNumEntries; ++i)
            {
                const auto& row = table->table[i];
                bool any = std::all_of(std::begin(row.ucLocalAddr),
                    std::end(row.ucLocalAddr), [](UCHAR c) { return c == 0; });
                listeners.push_back({ ntohs(u_short(row.dwLocalPort)),
                    any ? "all addresses (IPv6)" : "IPv6 address", row.dwOwningPid });
            }
        }
    }
    if (library)
        FreeLibrary(library);

    // the firewall: profiles that are switched on, and the inbound rules;
    // group policies override the local settings
    const std::pair<const wchar_t*, const char*> profiles[] = {
        { L"DomainProfile", "Domain" }, { L"StandardProfile", "Private" },
        { L"PublicProfile", "Public" } };
    const wchar_t* policyNames[] = { L"DomainProfile", L"PrivateProfile", L"PublicProfile" };
    bool anyProfile = false;
    for (size_t i = 0; i < std::size(profiles); ++i)
    {
        RegistryKey local((std::wstring(L"SYSTEM\\CurrentControlSet\\Services\\"
            L"SharedAccess\\Parameters\\FirewallPolicy\\") + profiles[i].first).c_str());
        RegistryKey policy((std::wstring(L"SOFTWARE\\Policies\\Microsoft\\"
            L"WindowsFirewall\\") + policyNames[i]).c_str());
        std::optional<DWORD> enabled = policy.getDword(L"EnableFirewall");
        if (!enabled)
            enabled = local.getDword(L"EnableFirewall");
        if (enabled && *enabled)
        {
            info.firewallProfiles += (anyProfile ? ", " : "") + std::string(profiles[i].second);
            anyProfile = true;
        }
    }
    std::vector<std::string> rules;
    bool rulesReadable = false;
    for (const wchar_t* path : { L"SYSTEM\\CurrentControlSet\\Services\\SharedAccess\\"
        L"Parameters\\FirewallPolicy\\FirewallRules",
        L"SOFTWARE\\Policies\\Microsoft\\WindowsFirewall\\FirewallRules" })
    {
        RegistryKey key(path);
        rulesReadable = rulesReadable || key.isOpen();
        for (const auto& value : key.getStringValues())
            rules.push_back(value.second);
    }
    if (!anyProfile)
        info.firewallProfiles = "off";
    if (!rulesReadable)
        info.firewallNote = "the firewall rules are not readable";

    for (const Listener& l : listeners)
    {
        std::string process = names.count(l.pid) ? names[l.pid] : std::string();
        if (!isInterestingPort(l.port, process))
            continue;
        auto same = std::find_if(info.listeningPorts.begin(), info.listeningPorts.end(),
            [&](const BenchmarkListeningPort& p)
            { return p.port == l.port && p.process == process; });
        if (same != info.listeningPorts.end())
        {
            // IPv4 and IPv6 listeners of the same server
            if (l.address.compare(0, 3, "all") == 0)
                same->address = "all addresses";
            continue;
        }
        BenchmarkListeningPort port;
        port.port = l.port;
        port.address = l.address;
        port.process = process;
        if (anyProfile && rulesReadable)
        {
            const std::string program = pathOf(l.pid);
            std::string allowedBy, blockedBy;
            for (const std::string& rule : rules)
            {
                std::string name;
                int match = matchBenchmarkFirewallRule(rule, l.port, program, &name);
                if (match < 0 && blockedBy.empty())
                    blockedBy = name.empty() ? "a rule" : name;
                else if (match > 0 && allowedBy.empty())
                    allowedBy = name.empty() ? "a rule" : name;
            }
            if (!blockedBy.empty())
                port.firewall = "blocked by \"" + blockedBy + "\"";
            else if (!allowedBy.empty())
                port.firewall = "allowed by \"" + allowedBy + "\"";
            else if (l.address.compare(0, 9, "127.0.0.1") != 0)
                port.firewall = "no inbound rule";
        }
        info.listeningPorts.push_back(port);
    }
    std::sort(info.listeningPorts.begin(), info.listeningPorts.end(),
        [](const BenchmarkListeningPort& a, const BenchmarkListeningPort& b)
        { return a.port < b.port; });

    RegistryKey tcp(L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters");
    info.tcpKeepAliveSeconds = int(tcp.getDword(L"KeepAliveTime").value_or(7200000) / 1000);
}

void collectPlatform(const std::string& storagePath, BenchmarkSystemInfo& info)
{
    readWindowsSmbios(info);
    readWindowsCpu(info);
    readWindowsMemory(info);
    readWindowsVersion(info);
    readWindowsPowerPlan(info);
    if (!storagePath.empty())
        readWindowsDrive(storagePath, info);
    readWindowsDefender(info);
    readWindowsAntivirus(info);
    readWindowsServices(info);
    readWindowsPorts(info);
    if (info.memoryModules.empty())
        info.memoryModulesNote = "not reported by the firmware";
}

#elif defined(__linux__)

namespace fs = std::filesystem;

std::string readFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return std::string();
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

std::string readLine(const fs::path& path)
{
    return trim(readFile(path));
}

int64_t readNumber(const fs::path& path)
{
    std::string s = readLine(path);
    return s.empty() ? -1 : std::atoll(s.c_str());
}

// "0-3,8,10-11" -> { 0, 1, 2, 3, 8, 10, 11 }
std::set<int> parseCpuList(const std::string& list)
{
    std::set<int> cpus;
    std::istringstream in(list);
    std::string part;
    while (std::getline(in, part, ','))
    {
        int first = 0, last = 0;
        int n = std::sscanf(part.c_str(), "%d-%d", &first, &last);
        if (n == 2)
        {
            for (int c = first; c <= last; ++c)
                cpus.insert(c);
        }
        else if (n == 1)
            cpus.insert(first);
    }
    return cpus;
}

int64_t parseCacheSize(const std::string& s)
{
    // "48K", "2048K", "36M"
    long long value = 0;
    char unit = 0;
    if (std::sscanf(s.c_str(), "%lld%c", &value, &unit) < 1)
        return 0;
    if (unit == 'K')
        return value * 1024;
    if (unit == 'M')
        return value * 1024 * 1024;
    return value;
}

void readLinuxMachine(BenchmarkSystemInfo& info)
{
    // the raw table is readable by root only
    std::string table = readFile("/sys/firmware/dmi/tables/DMI");
    if (!table.empty())
    {
        parseBenchmarkSmbios(reinterpret_cast<const uint8_t*>(table.data()),
            table.size(), info);
    }
    else
        info.memoryModulesNote = "needs root rights (dmidecode)";
    const fs::path dmi("/sys/class/dmi/id");
    auto fill = [&dmi](std::string& target, const char* file)
    {
        if (target.empty())
            target = firmwareString(readFile(dmi / file));
    };
    fill(info.manufacturer, "sys_vendor");
    fill(info.model, "product_name");
    fill(info.boardManufacturer, "board_vendor");
    fill(info.boardProduct, "board_name");
    fill(info.biosVendor, "bios_vendor");
    fill(info.biosVersion, "bios_version");
    fill(info.biosDate, "bios_date");
}

void readLinuxCpu(BenchmarkSystemInfo& info)
{
    std::istringstream cpuinfo(readFile("/proc/cpuinfo"));
    std::string line;
    bool hypervisor = false;
    while (std::getline(cpuinfo, line))
    {
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        if (info.cpuName.empty() && (key == "model name" || key == "Model"))
            info.cpuName = value;
        if (key == "flags" && contains(" " + value + " ", " hypervisor "))
            hypervisor = true;
    }
    if (hypervisor && info.virtualization.empty())
        info.virtualization = "virtual machine";

    // topology: threads, cores (package, core id) and packages
    const fs::path cpus("/sys/devices/system/cpu");
    std::set<std::pair<int64_t, int64_t>> cores;
    std::set<int64_t> packages;
    std::map<int, std::pair<int64_t, int64_t>> coreOfCpu;
    std::set<std::tuple<int64_t, std::string, std::string>> caches;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(cpus, ec))
    {
        std::string name = entry.path().filename().string();
        if (name.size() < 4 || name.compare(0, 3, "cpu") != 0
            || !std::isdigit(static_cast<unsigned char>(name[3])))
        {
            continue;
        }
        int64_t package = readNumber(entry.path() / "topology" / "physical_package_id");
        int64_t core = readNumber(entry.path() / "topology" / "core_id");
        if (core < 0)
            continue;
        ++info.cpuThreads;
        cores.insert({ package, core });
        packages.insert(package);
        coreOfCpu[std::atoi(name.c_str() + 3)] = { package, core };
        for (const auto& index : fs::directory_iterator(entry.path() / "cache", ec))
        {
            int64_t level = readNumber(index.path() / "level");
            std::string type = readLine(index.path() / "type");
            std::string shared = readLine(index.path() / "shared_cpu_list");
            std::string size = readLine(index.path() / "size");
            // every cache instance once, identified by the CPUs sharing it
            if (level > 0 && caches.insert({ level, type, shared }).second)
            {
                int64_t bytes = parseCacheSize(size);
                if (level == 1)
                    info.cacheL1Bytes += bytes;
                else if (level == 2)
                    info.cacheL2Bytes += bytes;
                else if (level == 3)
                    info.cacheL3Bytes += bytes;
            }
        }
    }
    info.cpuCores = int(cores.size());
    info.cpuSockets = std::max(info.cpuSockets, int(packages.size()));
    // Intel hybrid CPUs list their performance and efficiency CPUs
    auto countCores = [&coreOfCpu](const std::string& list)
    {
        std::set<std::pair<int64_t, int64_t>> unique;
        for (int cpu : parseCpuList(list))
        {
            auto it = coreOfCpu.find(cpu);
            if (it != coreOfCpu.end())
                unique.insert(it->second);
        }
        return int(unique.size());
    };
    std::string performance = readLine("/sys/devices/cpu_core/cpus");
    std::string efficiency = readLine("/sys/devices/cpu_atom/cpus");
    if (!performance.empty() && !efficiency.empty())
    {
        info.cpuPerformanceCores = countCores(performance);
        info.cpuEfficiencyCores = countCores(efficiency);
    }
    int64_t base = readNumber(cpus / "cpu0" / "cpufreq" / "base_frequency");
    int64_t max = readNumber(cpus / "cpu0" / "cpufreq" / "cpuinfo_max_freq");
    if (base > 0)
        info.cpuBaseMHz = int(base / 1000);
    if (max > 0)
        info.cpuMaxMHz = int(max / 1000);
    std::string governor = readLine(cpus / "cpu0" / "cpufreq" / "scaling_governor");
    if (!governor.empty())
    {
        info.powerPlan = "governor " + governor;
        std::string epp = readLine(cpus / "cpu0" / "cpufreq"
            / "energy_performance_preference");
        if (!epp.empty())
            info.powerPlan += ", energy preference " + epp;
    }
}

void readLinuxMemory(BenchmarkSystemInfo& info)
{
    std::string meminfo = readFile("/proc/meminfo");
    info.memoryTotalBytes = std::max<int64_t>(0, getBenchmarkMeminfoBytes(meminfo, "MemTotal"));
    info.memoryAvailableBytes = std::max<int64_t>(0,
        getBenchmarkMeminfoBytes(meminfo, "MemAvailable"));
    info.swapTotalBytes = std::max<int64_t>(0, getBenchmarkMeminfoBytes(meminfo, "SwapTotal"));
}

void readLinuxDrive(const std::string& path, BenchmarkSystemInfo& info)
{
    std::error_code ec;
    fs::path real = fs::canonical(path, ec);
    if (ec)
        return;
    info.storagePath = path;
    findBenchmarkMount(readFile("/proc/self/mountinfo"), real.string(),
        &info.fileSystem, &info.mountOptions);
    struct stat st{};
    if (stat(real.c_str(), &st) != 0)
        return;
    info.fileSystemClusterBytes = int64_t(st.st_blksize);

    // the block device; a partition's parent is the disk, device mapper
    // devices (LVM, encryption) lead to the disk through their slaves
    fs::path device = fs::canonical(fs::path("/sys/dev/block")
        / (std::to_string(major(st.st_dev)) + ":" + std::to_string(minor(st.st_dev))), ec);
    if (ec)
        return;
    for (int depth = 0; depth < 4; ++depth)
    {
        if (fs::exists(device / "partition"))
            device = device.parent_path();
        fs::path slaves = device / "slaves";
        if (!fs::exists(slaves) || fs::is_empty(slaves, ec))
            break;
        device = fs::canonical(fs::directory_iterator(slaves, ec)->path(), ec);
        if (ec)
            return;
    }
    const std::string name = device.filename().string();
    info.storageModel = trim(readFile(device / "device" / "model"));
    info.storageFirmware = trim(readFile(device / "device" / "firmware_rev"));
    if (info.storageFirmware.empty())
        info.storageFirmware = trim(readFile(device / "device" / "rev"));
    int64_t rotational = readNumber(device / "queue" / "rotational");
    if (rotational >= 0)
        info.storageMedia = rotational ? "HDD" : "SSD";
    int64_t sectors = readNumber(device / "size");
    if (sectors > 0)
        info.storageSizeBytes = sectors * 512;
    info.ioScheduler = getBenchmarkSelectedChoice(readLine(device / "queue" / "scheduler"));
    const std::string link = fs::read_symlink(device / "device", ec).string()
        + fs::canonical(device, ec).string();
    if (name.compare(0, 4, "nvme") == 0)
        info.storageBus = "NVMe";
    else if (name.compare(0, 2, "vd") == 0)
        info.storageBus = "virtio";
    else if (name.compare(0, 3, "xvd") == 0)
        info.storageBus = "Xen";
    else if (name.compare(0, 6, "mmcblk") == 0)
        info.storageBus = "eMMC / SD card";
    else if (contains(link, "/usb"))
        info.storageBus = "USB";
    else if (contains(link, "/ata"))
        info.storageBus = "SATA";
    else if (name.compare(0, 2, "sd") == 0)
        info.storageBus = "SCSI / SAS";
    // the temperature through hwmon (nvme driver, drivetemp module)
    for (const fs::path& base : { device / "device", device / "device" / "device" })
    {
        for (const auto& entry : fs::directory_iterator(base, ec))
        {
            std::string entryName = entry.path().filename().string();
            fs::path hwmon = entry.path();
            if (entryName == "hwmon")
            {
                auto first = fs::directory_iterator(hwmon, ec);
                if (ec || first == fs::directory_iterator())
                    continue;
                hwmon = first->path();
            }
            else if (entryName.compare(0, 5, "hwmon") != 0)
                continue;
            int64_t milli = readNumber(hwmon / "temp1_input");
            if (milli > 0 && !info.storageTemperatureC)
                info.storageTemperatureC = int(milli / 1000);
        }
    }
    info.storageHealthNote = "needs root rights";
}

void readLinuxSoftware(BenchmarkSystemInfo& info)
{
    std::string release = readFile("/etc/os-release");
    if (release.empty())
        release = readFile("/usr/lib/os-release");
    info.osName = getBenchmarkOsReleaseValue(release, "PRETTY_NAME");
    if (info.osName.empty())
        info.osName = "Linux";
    struct utsname names{};
    if (uname(&names) == 0)
    {
        info.kernel = std::string(names.sysname) + " " + names.release;
        info.architecture = names.machine;
    }
    double uptime = 0.0;
    if (std::sscanf(readFile("/proc/uptime").c_str(), "%lf", &uptime) == 1)
        info.uptimeSeconds = int64_t(uptime);

    std::string hypervisor = getBenchmarkHypervisor(info.manufacturer, info.model);
    if (!hypervisor.empty())
        info.virtualization = hypervisor;
    if (contains(lower(readFile("/proc/version")), "microsoft"))
    {
        info.container = "WSL 2";
        info.virtualization = "Hyper-V";
    }
    else if (fs::exists("/.dockerenv"))
        info.container = "Docker";
    else if (fs::exists("/run/.containerenv"))
        info.container = "Podman";
    else
    {
        std::string cgroup = readFile("/proc/1/cgroup");
        if (contains(cgroup, "kubepods"))
            info.container = "Kubernetes";
        else if (contains(cgroup, "docker"))
            info.container = "Docker";
        else if (contains(cgroup, "lxc"))
            info.container = "LXC";
    }

    auto setting = [&info](const char* name, const std::string& value)
    {
        if (!value.empty())
            info.osSettings.push_back({ name, value });
    };
    setting("vm.swappiness", readLine("/proc/sys/vm/swappiness"));
    setting("vm.dirty_ratio", readLine("/proc/sys/vm/dirty_ratio"));
    setting("vm.dirty_background_ratio", readLine("/proc/sys/vm/dirty_background_ratio"));
    setting("transparent huge pages", getBenchmarkSelectedChoice(
        readLine("/sys/kernel/mm/transparent_hugepage/enabled")));
}

// running database servers and on-access scanners, by process name
void readLinuxProcesses(BenchmarkSystemInfo& info)
{
    const std::pair<const char*, const char*> scanners[] = {
        { "clamd", "ClamAV" }, { "wdavdaemon", "Microsoft Defender for Endpoint" },
        { "falcon-sensor", "CrowdStrike Falcon" }, { "sentinelone", "SentinelOne" },
        { "s1-agent", "SentinelOne" }, { "savd", "Sophos" }, { "sophos", "Sophos" },
        { "esets", "ESET" }, { "ds_agent", "Trend Micro Deep Security" },
        { "kesl", "Kaspersky" }, { "mfetpd", "Trellix / McAfee" },
        { "cbagentd", "VMware Carbon Black" }, { "bdsecd", "Bitdefender" } };
    std::map<std::string, int> firebird;
    std::set<std::string> others;
    std::set<std::string> found;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec))
    {
        std::string pid = entry.path().filename().string();
        if (pid.empty() || !std::isdigit(static_cast<unsigned char>(pid[0])))
            continue;
        std::string comm = readLine(entry.path() / "comm");
        if (comm.empty())
            continue;
        if (isFirebirdServer(comm))
        {
            std::error_code linkError;
            std::string exe = fs::read_symlink(entry.path() / "exe", linkError).string();
            ++firebird[linkError ? std::string() : directoryOf(exe)];
            continue;
        }
        std::string other = otherDatabaseServer(comm);
        if (!other.empty())
            others.insert(other);
        for (const auto& s : scanners)
        {
            if (comm.compare(0, std::strlen(s.first), s.first) == 0)
                found.insert(s.second);
        }
    }
    for (const auto& f : firebird)
    {
        addServer(info.firebirdServers, f.second > 1
            ? "Firebird (" + std::to_string(f.second) + " processes)" : "Firebird",
            f.first, true);
    }
    for (const auto& o : others)
        addServer(info.otherDatabaseServers, o, std::string(), true);
    info.antivirus.assign(found.begin(), found.end());
    if (found.empty())
        info.antivirusNote = "no known on-access scanner running";
}

// listening TCP ports from /proc/net/tcp; the owning process is only known
// for processes of the same user (or as root)
void readLinuxPorts(BenchmarkSystemInfo& info)
{
    std::map<std::string, std::string> processOfInode;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator("/proc", ec))
    {
        std::string pid = entry.path().filename().string();
        if (pid.empty() || !std::isdigit(static_cast<unsigned char>(pid[0])))
            continue;
        std::error_code fdError;
        for (const auto& fd : fs::directory_iterator(entry.path() / "fd", fdError))
        {
            std::error_code linkError;
            std::string target = fs::read_symlink(fd.path(), linkError).string();
            if (!linkError && target.compare(0, 8, "socket:[") == 0)
                processOfInode[target.substr(8, target.size() - 9)] =
                    readLine(entry.path() / "comm");
        }
    }
    for (const char* file : { "/proc/net/tcp", "/proc/net/tcp6" })
    {
        std::istringstream table(readFile(file));
        std::string line;
        std::getline(table, line);  // header
        while (std::getline(table, line))
        {
            std::istringstream fields(line);
            std::string slot, local, remote, state, queues, timer, retransmits, uid,
                timeout, inode;
            if (!(fields >> slot >> local >> remote >> state >> queues >> timer
                >> retransmits >> uid >> timeout >> inode) || state != "0A")
            {
                continue;   // 0A: LISTEN
            }
            size_t colon = local.find(':');
            if (colon == std::string::npos)
                continue;
            int port = int(std::strtol(local.c_str() + colon + 1, nullptr, 16));
            std::string address = local.substr(0, colon);
            std::string process = processOfInode.count(inode) ? processOfInode[inode]
                : std::string();
            if (!isInterestingPort(port, process))
                continue;
            bool any = address.find_first_not_of('0') == std::string::npos;
            std::string text = any ? "all addresses"
                : address == "0100007F" ? "127.0.0.1" : "one address";
            auto same = std::find_if(info.listeningPorts.begin(),
                info.listeningPorts.end(), [&](const BenchmarkListeningPort& p)
                { return p.port == port; });
            if (same != info.listeningPorts.end())
            {
                if (any)
                    same->address = text;
                if (same->process.empty())
                    same->process = process;
                continue;
            }
            info.listeningPorts.push_back({ port, text, process, std::string() });
        }
    }
    std::sort(info.listeningPorts.begin(), info.listeningPorts.end(),
        [](const BenchmarkListeningPort& a, const BenchmarkListeningPort& b)
        { return a.port < b.port; });
    info.firewallNote = "needs root rights (nft list ruleset)";
    int64_t keepAlive = readNumber("/proc/sys/net/ipv4/tcp_keepalive_time");
    if (keepAlive > 0)
        info.tcpKeepAliveSeconds = int(keepAlive);
}

void collectPlatform(const std::string& storagePath, BenchmarkSystemInfo& info)
{
    readLinuxMachine(info);
    readLinuxCpu(info);
    readLinuxMemory(info);
    if (!storagePath.empty())
        readLinuxDrive(storagePath, info);
    readLinuxSoftware(info);
    readLinuxProcesses(info);
    readLinuxPorts(info);
}

#elif defined(__APPLE__)

std::string sysctlString(const char* name)
{
    size_t size = 0;
    if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0)
        return std::string();
    std::string value(size, '\0');
    if (sysctlbyname(name, &value[0], &size, nullptr, 0) != 0)
        return std::string();
    return trim(std::string(value.c_str()));
}

int64_t sysctlNumber(const char* name)
{
    int64_t value = 0;
    size_t size = sizeof(value);
    if (sysctlbyname(name, &value, &size, nullptr, 0) != 0)
        return -1;
    if (size == sizeof(int32_t))
        return int64_t(*reinterpret_cast<int32_t*>(&value));
    return value;
}

void collectPlatform(const std::string& storagePath, BenchmarkSystemInfo& info)
{
    info.manufacturer = "Apple";
    info.model = sysctlString("hw.model");
    info.cpuName = sysctlString("machdep.cpu.brand_string");
    info.cpuCores = int(std::max<int64_t>(0, sysctlNumber("hw.physicalcpu")));
    info.cpuThreads = int(std::max<int64_t>(0, sysctlNumber("hw.logicalcpu")));
    info.cpuSockets = int(std::max<int64_t>(1, sysctlNumber("hw.packages")));
    // Apple silicon: performance level 0 and efficiency level 1
    int64_t performance = sysctlNumber("hw.perflevel0.physicalcpu");
    int64_t efficiency = sysctlNumber("hw.perflevel1.physicalcpu");
    if (performance > 0 && efficiency > 0)
    {
        info.cpuPerformanceCores = int(performance);
        info.cpuEfficiencyCores = int(efficiency);
    }
    int64_t maxHz = sysctlNumber("hw.cpufrequency_max");
    if (maxHz > 0)
        info.cpuMaxMHz = int(maxHz / 1000000);
    // per core values; the totals depend on the core type
    int64_t l1 = sysctlNumber("hw.l1dcachesize") + sysctlNumber("hw.l1icachesize");
    int64_t l2 = sysctlNumber("hw.l2cachesize");
    int64_t l3 = sysctlNumber("hw.l3cachesize");
    if (l1 > 0)
        info.cacheL1Bytes = l1 * std::max(1, info.cpuCores);
    if (l2 > 0)
        info.cacheL2Bytes = l2;
    if (l3 > 0)
        info.cacheL3Bytes = l3;
    info.memoryTotalBytes = std::max<int64_t>(0, sysctlNumber("hw.memsize"));
    xsw_usage swap{};
    size_t size = sizeof(swap);
    if (sysctlbyname("vm.swapusage", &swap, &size, nullptr, 0) == 0)
        info.swapTotalBytes = int64_t(swap.xsu_total);
    info.memoryModulesNote = "not read on macOS";

    info.osName = "macOS " + sysctlString("kern.osproductversion");
    info.kernel = "Darwin " + sysctlString("kern.osrelease");
    info.architecture = sysctlString("hw.machine");
    timeval boot{};
    size = sizeof(boot);
    if (sysctlbyname("kern.boottime", &boot, &size, nullptr, 0) == 0 && boot.tv_sec > 0)
        info.uptimeSeconds = int64_t(std::time(nullptr) - boot.tv_sec);
    if (sysctlNumber("kern.hv_vmm_present") == 1)
        info.virtualization = "virtual machine";
    int64_t keepIdle = sysctlNumber("net.inet.tcp.keepidle");   // milliseconds
    if (keepIdle > 0)
        info.tcpKeepAliveSeconds = int(keepIdle / 1000);

    if (!storagePath.empty())
    {
        struct statfs fsInfo{};
        if (statfs(storagePath.c_str(), &fsInfo) == 0)
        {
            info.storagePath = storagePath;
            info.fileSystem = fsInfo.f_fstypename;
            info.fileSystemClusterBytes = int64_t(fsInfo.f_bsize);
        }
        info.storageHealthNote = "not read on macOS";
    }
}

#else

void collectPlatform(const std::string&, BenchmarkSystemInfo&)
{
}

#endif

// true if both addresses (same family) share the first prefixBits bits
bool sameNetwork(const sockaddr* a, const sockaddr* b, int prefixBits)
{
    const uint8_t* x = nullptr;
    const uint8_t* y = nullptr;
    int bytes = 0;
    if (a->sa_family != b->sa_family || prefixBits <= 0)
        return false;
    if (a->sa_family == AF_INET)
    {
        x = reinterpret_cast<const uint8_t*>(
            &reinterpret_cast<const sockaddr_in*>(a)->sin_addr);
        y = reinterpret_cast<const uint8_t*>(
            &reinterpret_cast<const sockaddr_in*>(b)->sin_addr);
        bytes = 4;
    }
    else if (a->sa_family == AF_INET6)
    {
        x = reinterpret_cast<const uint8_t*>(
            &reinterpret_cast<const sockaddr_in6*>(a)->sin6_addr);
        y = reinterpret_cast<const uint8_t*>(
            &reinterpret_cast<const sockaddr_in6*>(b)->sin6_addr);
        bytes = 16;
    }
    else
        return false;
    for (int bit = 0; bit < std::min(prefixBits, bytes * 8); ++bit)
    {
        int mask = 0x80 >> (bit % 8);
        if ((x[bit / 8] & mask) != (y[bit / 8] & mask))
            return false;
    }
    return true;
}

bool sameAddress(const sockaddr* a, const sockaddr* b)
{
    return sameNetwork(a, b, a->sa_family == AF_INET ? 32 : 128);
}

#ifndef _WIN32
// Windows reports the prefix length of an address directly
int prefixLength(const sockaddr* netmask)
{
    if (!netmask)
        return 0;
    const uint8_t* m = nullptr;
    int bytes = 0;
    if (netmask->sa_family == AF_INET)
    {
        m = reinterpret_cast<const uint8_t*>(
            &reinterpret_cast<const sockaddr_in*>(netmask)->sin_addr);
        bytes = 4;
    }
    else if (netmask->sa_family == AF_INET6)
    {
        m = reinterpret_cast<const uint8_t*>(
            &reinterpret_cast<const sockaddr_in6*>(netmask)->sin6_addr);
        bytes = 16;
    }
    int bits = 0;
    for (int i = 0; i < bytes; ++i)
    {
        for (int b = 7; b >= 0 && (m[i] & (1 << b)); --b)
            ++bits;
    }
    return bits;
}
#endif

std::string addressText(const sockaddr* address, size_t length)
{
    char text[NI_MAXHOST] = "";
    if (getnameinfo(address, socklen_t(length), text, sizeof(text), nullptr, 0,
        NI_NUMERICHOST) != 0)
    {
        return std::string();
    }
    return text;
}

// Resolves host and finds the local address the system would use for it:
// connect() on a UDP socket chooses the route but sends nothing.
bool resolveAndRoute(BenchmarkNetworkPath& path, sockaddr_storage* server,
    sockaddr_storage* local)
{
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result = nullptr;
    auto start = std::chrono::steady_clock::now();
    int rc = getaddrinfo(path.serverHost.c_str(), "3050", &hints, &result);
    path.resolveMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    if (rc != 0 || !result)
    {
        path.note = "the server name could not be resolved";
        return false;
    }
    std::memcpy(server, result->ai_addr, std::min(sizeof(*server),
        size_t(result->ai_addrlen)));
    path.serverAddress = addressText(result->ai_addr, result->ai_addrlen);
    for (addrinfo* r = result; r; r = r->ai_next)
    {
        std::string text = addressText(r->ai_addr, r->ai_addrlen);
        if (!text.empty() && std::find(path.serverAddresses.begin(),
            path.serverAddresses.end(), text) == path.serverAddresses.end())
        {
            path.serverAddresses.push_back(text);
        }
    }
    bool routed = false;
#if defined(_WIN32)
    SOCKET s = socket(result->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (s != INVALID_SOCKET)
#else
    int s = socket(result->ai_family, SOCK_DGRAM, IPPROTO_UDP);
    if (s >= 0)
#endif
    {
        socklen_t length = sizeof(*local);
        if (connect(s, result->ai_addr, socklen_t(result->ai_addrlen)) == 0
            && getsockname(s, reinterpret_cast<sockaddr*>(local), &length) == 0)
        {
            path.localAddress = addressText(reinterpret_cast<sockaddr*>(local), length);
            routed = true;
        }
#if defined(_WIN32)
        closesocket(s);
#else
        close(s);
#endif
    }
    freeaddrinfo(result);
    if (!routed)
        path.note = "no route to the server";
    return routed;
}

// A network adapter of this machine with its addresses
struct Adapter
{
    struct Address
    {
        sockaddr_storage address;
        int prefixLength;
        std::string text;
    };

    std::string name;
    std::string description;
    std::string type;
    int64_t linkSpeed = 0;      // bit/s
    int mtu = 0;
    bool up = false;
    bool loopback = false;
    std::string wifiSignal;
    std::vector<Address> addresses;
};

std::vector<Adapter> readAdapters()
{
    std::vector<Adapter> adapters;
#if defined(_WIN32)
    // iphlpapi is loaded at run time, so that FlameRobin does not need
    // another import library
    using GetAdaptersAddressesFunction = ULONG (WINAPI*)(ULONG, ULONG, PVOID,
        PIP_ADAPTER_ADDRESSES, PULONG);
    HMODULE library = LoadLibraryW(L"iphlpapi.dll");
    auto getAdapters = library ? reinterpret_cast<GetAdaptersAddressesFunction>(
        reinterpret_cast<void*>(GetProcAddress(library, "GetAdaptersAddresses")))
        : nullptr;
    std::vector<uint8_t> buffer(32 * 1024);
    ULONG size = ULONG(buffer.size());
    ULONG rc = ERROR_NOT_SUPPORTED;
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST
        | GAA_FLAG_SKIP_DNS_SERVER;
    for (int attempt = 0; getAdapters && attempt < 3; ++attempt)
    {
        rc = getAdapters(AF_UNSPEC, flags, nullptr,
            reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()), &size);
        if (rc != ERROR_BUFFER_OVERFLOW)
            break;
        buffer.resize(size);
    }
    for (auto* a = rc == NO_ERROR
        ? reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data()) : nullptr; a; a = a->Next)
    {
        Adapter adapter;
        adapter.name = utf8(a->FriendlyName ? a->FriendlyName : L"");
        adapter.description = utf8(a->Description ? a->Description : L"");
        switch (a->IfType)
        {
            case IF_TYPE_ETHERNET_CSMACD: adapter.type = "Ethernet"; break;
            case IF_TYPE_IEEE80211: adapter.type = "Wi-Fi"; break;
            case IF_TYPE_SOFTWARE_LOOPBACK: adapter.type = "loopback"; break;
            case IF_TYPE_PPP: adapter.type = "VPN"; break;
            case IF_TYPE_TUNNEL: adapter.type = "tunnel"; break;
            case IF_TYPE_PROP_VIRTUAL: adapter.type = "virtual"; break;
            case 243: case 244: adapter.type = "mobile broadband"; break;
            default: break;
        }
        adapter.type = classifyBenchmarkAdapter(adapter.type, adapter.name,
            adapter.description);
        if (a->TransmitLinkSpeed != ~0ULL)
            adapter.linkSpeed = int64_t(a->TransmitLinkSpeed);
        adapter.mtu = int(a->Mtu);
        adapter.up = a->OperStatus == IfOperStatusUp;
        adapter.loopback = a->IfType == IF_TYPE_SOFTWARE_LOOPBACK;
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next)
        {
            Adapter::Address address{};
            std::memcpy(&address.address, u->Address.lpSockaddr,
                std::min(sizeof(address.address), size_t(u->Address.iSockaddrLength)));
            address.prefixLength = u->OnLinkPrefixLength;
            address.text = addressText(u->Address.lpSockaddr, u->Address.iSockaddrLength);
            adapter.addresses.push_back(address);
        }
        adapters.push_back(adapter);
    }
    if (library)
        FreeLibrary(library);
#else
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0)
        return adapters;
    for (ifaddrs* i = interfaces; i; i = i->ifa_next)
    {
        if (!i->ifa_addr
            || (i->ifa_addr->sa_family != AF_INET && i->ifa_addr->sa_family != AF_INET6))
        {
            continue;
        }
        auto adapter = std::find_if(adapters.begin(), adapters.end(),
            [i](const Adapter& a) { return a.name == i->ifa_name; });
        if (adapter == adapters.end())
        {
            adapters.push_back(Adapter());
            adapter = adapters.end() - 1;
            adapter->name = i->ifa_name;
            adapter->up = (i->ifa_flags & IFF_UP) && (i->ifa_flags & IFF_RUNNING);
            adapter->loopback = (i->ifa_flags & IFF_LOOPBACK) != 0;
        }
        Adapter::Address address{};
        const size_t length = i->ifa_addr->sa_family == AF_INET ? sizeof(sockaddr_in)
            : sizeof(sockaddr_in6);
        std::memcpy(&address.address, i->ifa_addr, length);
        address.prefixLength = prefixLength(i->ifa_netmask);
        address.text = addressText(i->ifa_addr, length);
        adapter->addresses.push_back(address);
    }
    freeifaddrs(interfaces);
    for (Adapter& adapter : adapters)
    {
        std::string type = adapter.loopback ? "loopback" : "";
#if defined(__linux__)
        namespace fs = std::filesystem;
        const fs::path net = fs::path("/sys/class/net") / adapter.name;
        std::error_code ec;
        int64_t arp = readNumber(net / "type");
        if (arp == 772)
            type = "loopback";
        else if (fs::exists(net / "wireless") || fs::exists(net / "phy80211"))
            type = "Wi-Fi";
        else if (!fs::exists(net / "device"))
            type = "virtual";
        else if (arp == 1)
            type = "Ethernet";
        int64_t speed = readNumber(net / "speed");     // Mbit/s, -1 for Wi-Fi
        if (speed > 0)
            adapter.linkSpeed = speed * 1000000;
        int64_t mtu = readNumber(net / "mtu");
        if (mtu > 0)
            adapter.mtu = int(mtu);
        adapter.description = fs::read_symlink(net / "device" / "driver", ec)
            .filename().string();
        // "wlan0: 0000   54.  -56.  -256 ..." link quality and signal level
        std::istringstream wireless(readFile("/proc/net/wireless"));
        std::string line;
        while (std::getline(wireless, line))
        {
            std::istringstream fields(line);
            std::string name, status;
            double quality = 0.0, level = 0.0;
            if (fields >> name >> status >> quality >> level && name == adapter.name + ":")
                adapter.wifiSignal = std::to_string(int(level)) + " dBm";
        }
#endif
        adapter.type = classifyBenchmarkAdapter(type, adapter.name, adapter.description);
    }
#endif
    return adapters;
}

// the user FlameRobin runs as, and whether it has administrator rights
void readUser(BenchmarkSystemInfo& info)
{
#if defined(_WIN32)
    wchar_t name[256];
    DWORD length = DWORD(std::size(name));
    if (GetUserNameW(name, &length))
    {
        info.osUser = utf8(name);
        wchar_t domain[256];
        DWORD domainLength = GetEnvironmentVariableW(L"USERDOMAIN", domain,
            DWORD(std::size(domain)));
        if (domainLength > 0 && domainLength < std::size(domain))
            info.osUser = utf8(domain) + "\\" + info.osUser;
    }
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        TOKEN_ELEVATION elevation{};
        DWORD size = 0;
        if (GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
            info.elevated = elevation.TokenIsElevated != 0;
        CloseHandle(token);
    }
    wchar_t full[256];
    DWORD fullLength = DWORD(std::size(full));
    if (GetComputerNameExW(ComputerNameDnsFullyQualified, full, &fullLength))
        info.fullHostName = utf8(full);
#else
    if (passwd* user = getpwuid(geteuid()))
        info.osUser = user->pw_name;
    info.elevated = geteuid() == 0;
    char host[256] = "";
    if (gethostname(host, sizeof(host) - 1) == 0)
    {
        info.fullHostName = host;
        // the domain, if the resolver knows the canonical name
        addrinfo hints{};
        hints.ai_flags = AI_CANONNAME;
        addrinfo* result = nullptr;
        if (std::strchr(host, '.') == nullptr
            && getaddrinfo(host, nullptr, &hints, &result) == 0 && result)
        {
            if (result->ai_canonname && std::strchr(result->ai_canonname, '.'))
                info.fullHostName = result->ai_canonname;
            freeaddrinfo(result);
        }
    }
#endif
}

} // namespace

std::string classifyBenchmarkAdapter(const std::string& type, const std::string& name,
    const std::string& description)
{
    const std::string n = lower(name);
    const std::string d = lower(description);
    const char* vpnNames[] = { "tun", "tap", "wg", "ppp", "ipsec", "utun", "tailscale",
        "zt", "nordlynx" };
    for (const char* p : vpnNames)
    {
        if (n.compare(0, std::strlen(p), p) == 0)
            return "VPN";
    }
    const char* vpnDescriptions[] = { "vpn", "wireguard", "tap-windows", "openvpn",
        "anyconnect", "fortinet", "globalprotect", "pangp", "zscaler", "juniper",
        "pulse secure", "sonicwall", "check point", "tailscale", "zerotier", "nordlynx",
        "wintun" };
    for (const char* p : vpnDescriptions)
    {
        if (contains(d, p) || contains(n, p))
            return "VPN";
    }
    // virtual switches of Hyper-V, WSL, VMware and VirtualBox
    const char* virtualDescriptions[] = { "hyper-v virtual", "vethernet",
        "vmware virtual", "virtualbox", "host-only", "docker", "veth" };
    for (const char* p : virtualDescriptions)
    {
        if (contains(d, p) || n.compare(0, std::strlen(p), p) == 0)
            return "virtual";
    }
    return type;
}

BenchmarkNetworkPath probeBenchmarkNetworkPath(const std::string& host)
{
    BenchmarkNetworkPath path;
    path.serverHost = host;
    sockaddr_storage server{};
    sockaddr_storage local{};
#if defined(_WIN32)
    WSADATA wsa;
    const bool started = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#endif
    if (resolveAndRoute(path, &server, &local))
    {
        for (const Adapter& adapter : readAdapters())
        {
            for (const Adapter::Address& a : adapter.addresses)
            {
                if (!sameAddress(reinterpret_cast<const sockaddr*>(&a.address),
                    reinterpret_cast<const sockaddr*>(&local)))
                {
                    continue;
                }
                path.adapterName = adapter.name;
                path.adapterDescription = adapter.description;
                path.adapterType = adapter.type;
                path.linkSpeedBitsPerSecond = adapter.linkSpeed;
                path.mtu = adapter.mtu;
                path.wifiSignal = adapter.wifiSignal;
                path.sameSubnet = sameNetwork(reinterpret_cast<const sockaddr*>(&a.address),
                    reinterpret_cast<const sockaddr*>(&server), a.prefixLength);
            }
        }
        if (path.adapterName.empty())
            path.note = "the network adapter is not known";
    }
#if defined(_WIN32)
    if (started)
        WSACleanup();
#endif
    return path;
}

std::vector<BenchmarkPortCheck> checkBenchmarkPorts(const std::string& host,
    const std::vector<int>& ports, int timeoutMs)
{
    using State = BenchmarkPortCheck::State;
    std::vector<BenchmarkPortCheck> results;
#if defined(_WIN32)
    WSADATA wsa;
    const bool started = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    using Socket = SOCKET;
    const Socket invalid = INVALID_SOCKET;
    const int refusedError = WSAECONNREFUSED;
#else
    using Socket = int;
    const Socket invalid = -1;
    const int refusedError = ECONNREFUSED;
#endif
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* address = nullptr;
    if (getaddrinfo(host.c_str(), nullptr, &hints, &address) == 0 && address)
    {
        struct Attempt
        {
            BenchmarkPortCheck check;
            Socket socket;
            bool done;
        };
        std::vector<Attempt> attempts;
        const auto start = std::chrono::steady_clock::now();
        auto elapsed = [&start]()
        {
            return std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - start).count();
        };
        // all connections are started at once, without waiting
        for (int port : ports)
        {
            Attempt a{ { port, State::Filtered, 0.0 },
                socket(address->ai_family, SOCK_STREAM, IPPROTO_TCP), false };
            if (a.socket == invalid)
                continue;
#if defined(_WIN32)
            u_long nonBlocking = 1;
            ioctlsocket(a.socket, FIONBIO, &nonBlocking);
#else
            fcntl(a.socket, F_SETFL, fcntl(a.socket, F_GETFL, 0) | O_NONBLOCK);
#endif
            sockaddr_storage target{};
            std::memcpy(&target, address->ai_addr, std::min(sizeof(target),
                size_t(address->ai_addrlen)));
            if (target.ss_family == AF_INET)
                reinterpret_cast<sockaddr_in*>(&target)->sin_port = htons(uint16_t(port));
            else
                reinterpret_cast<sockaddr_in6*>(&target)->sin6_port = htons(uint16_t(port));
            if (connect(a.socket, reinterpret_cast<sockaddr*>(&target),
                socklen_t(address->ai_addrlen)) == 0)
            {
                a.check.state = State::Open;
                a.done = true;
            }
            else
            {
#if defined(_WIN32)
                const int error = WSAGetLastError();
                const bool pending = error == WSAEWOULDBLOCK;
#else
                const int error = errno;
                const bool pending = error == EINPROGRESS;
#endif
                if (!pending)
                {
                    a.check.state = error == refusedError ? State::Closed : State::Filtered;
                    a.done = true;
                }
            }
            a.check.ms = elapsed();
            attempts.push_back(a);
        }
        // a refused connection fails quickly (Windows tries it again for
        // about a second); a firewall that drops packets gives no answer
        for (;;)
        {
            fd_set writable, failed;
            FD_ZERO(&writable);
            FD_ZERO(&failed);
            Socket highest = 0;
            int pending = 0;
            for (const Attempt& a : attempts)
            {
                if (a.done)
                    continue;
                FD_SET(a.socket, &writable);
                FD_SET(a.socket, &failed);
                highest = std::max(highest, a.socket);
                ++pending;
            }
            const int remaining = timeoutMs - int(elapsed());
            if (pending == 0 || remaining <= 0)
                break;
            timeval wait{ remaining / 1000, (remaining % 1000) * 1000 };
            if (select(int(highest + 1), nullptr, &writable, &failed, &wait) <= 0)
                break;
            for (Attempt& a : attempts)
            {
                if (a.done || (!FD_ISSET(a.socket, &writable) && !FD_ISSET(a.socket, &failed)))
                    continue;
                int error = 0;
                socklen_t length = sizeof(error);
                getsockopt(a.socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error),
                    &length);
                a.check.state = error == 0 ? State::Open
                    : error == refusedError ? State::Closed : State::Filtered;
                a.check.ms = elapsed();
                a.done = true;
            }
        }
        for (Attempt& a : attempts)
        {
            if (!a.done)
                a.check.ms = timeoutMs;
#if defined(_WIN32)
            shutdown(a.socket, SD_BOTH);
            closesocket(a.socket);
#else
            shutdown(a.socket, SHUT_RDWR);
            close(a.socket);
#endif
            results.push_back(a.check);
        }
        freeaddrinfo(address);
    }
#if defined(_WIN32)
    if (started)
        WSACleanup();
#endif
    return results;
}

BenchmarkSystemInfo collectBenchmarkSystemInfo(const std::string& storagePath)
{
    BenchmarkSystemInfo info;
    collectPlatform(storagePath, info);
    readUser(info);
#if defined(_WIN32)
    WSADATA wsa;
    const bool started = WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#endif
    for (const Adapter& adapter : readAdapters())
    {
        if (!adapter.up || adapter.loopback)
            continue;
        for (const Adapter::Address& a : adapter.addresses)
        {
            // link-local IPv6 addresses exist on every adapter
            if (a.text.empty() || lower(a.text).compare(0, 5, "fe80:") == 0)
                continue;
            info.addresses.push_back({ adapter.name, adapter.type, a.text });
        }
    }
#if defined(_WIN32)
    if (started)
        WSACleanup();
#endif
    if (info.virtualization.empty())
        info.virtualization = getBenchmarkHypervisor(info.manufacturer, info.model);
    return info;
}

std::vector<BenchmarkFirebirdInstallation> findBenchmarkFirebirdInstallations()
{
    std::vector<BenchmarkFirebirdInstallation> list;
#if defined(_WIN32)
    auto exists = [](const std::string& path)
    {
        return GetFileAttributesW(wide(path).c_str()) != INVALID_FILE_ATTRIBUTES;
    };
    auto add = [&list, &exists](std::string directory, const std::string& service,
        bool running)
    {
        while (!directory.empty() && (directory.back() == '\\' || directory.back() == '/'))
            directory.pop_back();
        const std::string client = directory + "\\fbclient.dll";
        // Firebird 3 has engine12, 4 and 5 engine13, 6 engine14
        if (directory.empty() || !exists(client)
            || (!exists(directory + "\\plugins\\engine12.dll")
                && !exists(directory + "\\plugins\\engine13.dll")
                && !exists(directory + "\\plugins\\engine14.dll")))
        {
            return;
        }
        for (auto& known : list)
        {
            if (lower(known.directory) == lower(directory))
            {
                if (known.service.empty() || (running && !known.running))
                {
                    known.service = service;
                    known.running = running;
                }
                return;
            }
        }
        BenchmarkFirebirdInstallation installation;
        installation.directory = directory;
        installation.clientLibrary = client;
        installation.service = service;
        installation.running = running;
        DWORD handle = 0;
        const std::wstring path = wide(client);
        DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
        std::vector<uint8_t> data(size);
        VS_FIXEDFILEINFO* version = nullptr;
        UINT length = 0;
        if (size > 0 && GetFileVersionInfoW(path.c_str(), 0, size, data.data())
            && VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&version), &length)
            && version && length >= sizeof(VS_FIXEDFILEINFO))
        {
            installation.version = std::to_string(HIWORD(version->dwFileVersionMS)) + "."
                + std::to_string(LOWORD(version->dwFileVersionMS)) + "."
                + std::to_string(HIWORD(version->dwFileVersionLS)) + "."
                + std::to_string(LOWORD(version->dwFileVersionLS));
        }
        list.push_back(installation);
    };

    // the services, then the instances the installer registered, then the
    // default folders
    BenchmarkSystemInfo services;
    readWindowsServices(services);
    for (const auto& s : services.firebirdServers)
        add(s.detail, s.name, s.running);
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Firebird Project\\Firebird Server\\"
        L"Instances", 0, KEY_READ | KEY_WOW64_64KEY, &key) == ERROR_SUCCESS)
    {
        for (DWORD i = 0;; ++i)
        {
            wchar_t name[256];
            DWORD nameLength = 256;
            wchar_t value[1024];
            DWORD valueBytes = sizeof(value) - sizeof(wchar_t);
            DWORD type = 0;
            if (RegEnumValueW(key, i, name, &nameLength, nullptr, &type,
                reinterpret_cast<BYTE*>(value), &valueBytes) != ERROR_SUCCESS)
            {
                break;
            }
            if (type == REG_SZ || type == REG_EXPAND_SZ)
            {
                value[valueBytes / sizeof(wchar_t)] = L'\0';
                add(utf8(std::wstring(value)), std::string(), false);
            }
        }
        RegCloseKey(key);
    }
    wchar_t programFiles[MAX_PATH];
    if (GetEnvironmentVariableW(L"ProgramFiles", programFiles, MAX_PATH) > 0)
    {
        WIN32_FIND_DATAW entry;
        const std::string root = utf8(std::wstring(programFiles)) + "\\Firebird";
        HANDLE find = FindFirstFileW(wide(root + "\\*").c_str(), &entry);
        if (find != INVALID_HANDLE_VALUE)
        {
            do
            {
                if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    && entry.cFileName[0] != L'.')
                {
                    add(root + "\\" + utf8(std::wstring(entry.cFileName)), std::string(),
                        false);
                }
            }
            while (FindNextFileW(find, &entry));
            FindClose(find);
        }
    }
#elif defined(__linux__) || defined(__APPLE__)
    namespace fs = std::filesystem;
    std::error_code error;
    auto hasEngine = [&error](const std::string& plugins)
    {
        for (const auto& entry : fs::directory_iterator(plugins, error))
        {
            if (entry.path().filename().string().rfind("libEngine", 0) == 0)
                return true;
        }
        return false;
    };
    // an empty client library: the default one, which finds the engine
    // through its own configuration (packages of the distributions)
    auto add = [&](const std::string& directory, const std::string& client,
        const std::string& plugins)
    {
        if ((!client.empty() && !fs::exists(client, error)) || !hasEngine(plugins))
            return;
        if (std::any_of(list.begin(), list.end(),
            [&directory](const auto& known) { return known.directory == directory; }))
        {
            return;
        }
        BenchmarkFirebirdInstallation installation;
        installation.directory = directory;
        installation.clientLibrary = client;
        // the real file name carries the version: libfbclient.so.5.0.4
        const std::string real = client.empty() ? std::string()
            : fs::canonical(client, error).filename().string();
        const size_t so = real.find(".so.");
        installation.version = so != std::string::npos ? real.substr(so + 4)
            : fs::path(directory).filename().string();
        list.push_back(installation);
    };
    if (const char* root = getenv("FIREBIRD"))
        add(root, std::string(root) + "/lib/libfbclient.so", std::string(root) + "/plugins");
#if defined(__APPLE__)
    add("/Library/Frameworks/Firebird.framework",
        "/Library/Frameworks/Firebird.framework/Libraries/libfbclient.dylib",
        "/Library/Frameworks/Firebird.framework/Resources/plugins");
#else
    add("/opt/firebird", "/opt/firebird/lib/libfbclient.so", "/opt/firebird/plugins");
    for (const char* base : { "/usr/lib/x86_64-linux-gnu/firebird",
        "/usr/lib/aarch64-linux-gnu/firebird", "/usr/lib64/firebird", "/usr/lib/firebird" })
    {
        for (const auto& version : fs::directory_iterator(base, error))
        {
            add(version.path().string(), std::string(),
                version.path().string() + "/plugins");
        }
    }
#endif
#endif
    // the running server first, then the newest version
    std::stable_sort(list.begin(), list.end(),
        [](const BenchmarkFirebirdInstallation& a, const BenchmarkFirebirdInstallation& b)
        {
            if (a.running != b.running)
                return a.running;
            return a.version > b.version;
        });
    return list;
}

} // namespace fr
