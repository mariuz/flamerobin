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

#include <wx/arrstr.h>

#include <algorithm>
#include <cmath>

#include "engine/Benchmark.h"
#include "frversion.h"

// Comparing two runs. A single run is not judged, because whether a value
// is fast or slow depends on the environment; a comparison shows it. Every
// report contains the results of its run as a hidden comment (the
// snapshot), so a saved report can be compared with a new run, or two
// saved reports with each other, without running a test; its format is
// described in devdocs/Benchmark_report_format.md. The measurements of
// two runs are matched by their ids only, never by their titles. The
// earlier run gets 1000 points for every part and in total, the later run
// proportionally more or fewer. Where a part differs clearly, the
// comparison names what could make the slower run slower: the facts of the
// environment that differ, what was noticed on the slower run, and the
// typical causes of its slower values.

namespace fr
{

namespace
{

// format 1 (FlameRobin 26.10.0) used another marker; its parser accepts every
// format line and so would compare a report of format 2 against nothing
const char* const snapshotMarker = "FlameRobin benchmark snapshot";
const char* const oldSnapshotMarker = "FlameRobin benchmark results";
// the format of the snapshot: the major number changes when older versions
// cannot read it any more, the minor number when lines or fields are added
const int formatMajor = 2;
const int formatMinor = 0;

// a single value can neither dominate a part nor zero it
const double maximumValueRatio = 5.0;
// the typical causes of at most this many values per part
const size_t maximumAdvice = 3;

const std::pair<BenchmarkCategory, const char*> sectionIds[] = {
    { BenchmarkCategory::ServerCpu, "cpu" },
    { BenchmarkCategory::ServerStorage, "storage" },
    { BenchmarkCategory::ServerCache, "memory" },
    { BenchmarkCategory::Network, "network" },
    { BenchmarkCategory::MultiUser, "users" },
    { BenchmarkCategory::LocalMachine, "computer" },
    { BenchmarkCategory::Application, "application" }
};

const std::pair<BenchmarkIntensity, const char*> intensityIds[] = {
    { BenchmarkIntensity::Quick, "quick" },
    { BenchmarkIntensity::Standard, "standard" },
    { BenchmarkIntensity::Deep, "deep" }
};

bool parseSection(const wxString& id, BenchmarkCategory* section)
{
    if (id == "settings")
    {
        *section = BenchmarkCategory::Environment;
        return true;
    }
    for (const auto& known : sectionIds)
    {
        if (id == known.second)
        {
            *section = known.first;
            return true;
        }
    }
    return false;
}

// the fields of a line, trimmed
wxArrayString split(const wxString& value)
{
    wxArrayString parts = wxSplit(value, '|', '\0');
    for (auto& part : parts)
        part.Trim().Trim(false);
    return parts;
}

bool isDigits(const wxString& text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(),
        [](wxUniChar c) { return c >= '0' && c <= '9'; });
}

wxString intensityName(const wxString& id)
{
    for (const auto& i : intensityIds)
    {
        if (id == i.second)
            return getBenchmarkIntensityName(i.first);
    }
    return id;
}

// a value inside the comment: one line, and never the end of a comment
wxString clean(const wxString& text)
{
    wxString s = text;
    s.Replace("\r", " ");
    s.Replace("\n", " ");
    s.Replace("|", "/");
    while (s.Replace("--", "- -"))
        ;
    return s.Trim().Trim(false);
}

wxString dash()
{
    return wxString(wxUniChar(0x2013));
}

wxString arrow()
{
    return wxString(" ") + wxUniChar(0x2192) + " ";
}

wxString cell(const wxString& text)
{
    return escapeMarkdownTableCell(text);
}

wxString olderLabel()
{
    return _("Earlier run");
}

wxString newerLabel(const BenchmarkComparison& c)
{
    return c.newerIsCurrentRun ? _("This run") : _("Later run");
}

wxString runText(const BenchmarkSnapshot& s)
{
    return getBenchmarkKindName(s.kind) + ", " + intensityName(s.intensity);
}

double geometricMean(const std::vector<double>& ratios)
{
    if (ratios.empty())
        return 0.0;
    double logSum = 0.0;
    for (double r : ratios)
        logSum += std::log(r);
    return std::exp(logSum / double(ratios.size()));
}

wxString pointsText(double ratio)
{
    return ratio > 0.0 ? formatBenchmarkPoints(BenchmarkComparison::earlierPoints * ratio)
        : dash();
}

wxString join(const std::vector<wxString>& items, const wxString& with)
{
    wxString text;
    for (const wxString& item : items)
        text += (text.empty() ? wxString() : with) + item;
    return text;
}

// a change as a triangle (no emoji) and words, in bold when it is large
wxString changeMark(double ratio)
{
    const int level = getBenchmarkChangeLevel(ratio);
    wxString text = level == 0 ? wxString("=")
        : wxString(wxUniChar(ratio > 1.0 ? 0x25B2 : 0x25BC));
    text += " " + formatBenchmarkChange(ratio);
    return level == 2 ? "**" + text + "**" : text;
}

// the change of a value, or why it was not compared
wxString stateText(double ratio, BenchmarkCompareState state)
{
    switch (state)
    {
        case BenchmarkCompareState::TestChanged:
            return _("not comparable (test changed)");
        case BenchmarkCompareState::WorkChanged:
            return _("not comparable (other work)");
        case BenchmarkCompareState::OnlyNewer:
        case BenchmarkCompareState::OnlyOlder:
            return getBenchmarkCompareStateName(state);
        case BenchmarkCompareState::Compared:
        default:
            return ratio > 0.0 ? changeMark(ratio) : wxString();
    }
}

// a number of the snapshot, the same in every language: a point as the
// decimal separator, no thousands separators, no exponent, at most six
// significant digits but every digit before the point; empty for 0
wxString canonicalNumber(double value)
{
    if (!std::isfinite(value) || value == 0.0)
        return wxEmptyString;
    const int magnitude = int(std::floor(std::log10(std::abs(value))));
    wxString text = wxString::FromCDouble(value, std::clamp(5 - magnitude, 0, 12));
    if (text.Contains("."))
    {
        while (text.EndsWith("0"))
            text.RemoveLast();
        if (text.EndsWith("."))
            text.RemoveLast();
    }
    return text;
}

// a number of the snapshot; empty is 0, anything else must be a number that
// is not negative
bool parseNumber(const wxString& text, double* value)
{
    *value = 0.0;
    if (text.empty())
        return true;
    return text.ToCDouble(value) && std::isfinite(*value) && *value >= 0.0;
}

// the value of a measurement in the unit of the register, from the work
// of its result: a rate, or the time of one operation
double measuredValue(const BenchmarkMeasurementDef& def, double amount,
    const wxString& amountUnit, double ms)
{
    if (ms <= 0.0)
        return 0.0;
    const wxString unit = def.unit;
    if (unit == "ms")
        return amount > 0.0 ? ms / amount : ms;
    if (unit == "MB/s")
        return amountUnit == "bytes" ? amount / (1024.0 * 1024.0) / (ms / 1000.0) : 0.0;
    if (unit == "it/ms")
        return amount / ms;
    if (unit.EndsWith("/s"))
        return amount * 1000.0 / ms;
    return 0.0;
}

// a ratio of two saved values to five significant digits: the values have
// six, so that an exact factor (twice as fast) stays exact and two equal
// changes stay equal
double roundRatio(double ratio)
{
    if (ratio <= 0.0 || !std::isfinite(ratio))
        return 0.0;
    const double scale = std::pow(10.0, 4.0 - std::floor(std::log10(ratio)));
    return std::round(ratio * scale) / scale;
}

// the section of a measurement this version does not know, from its id
bool sectionOfId(const wxString& id, BenchmarkCategory* section)
{
    const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(id);
    if (def)
    {
        *section = def->section;
        return true;
    }
    return parseSection(id.BeforeFirst('.'), section);
}

} // namespace

const BenchmarkMeasurement* BenchmarkSnapshot::findMeasurement(const wxString& id) const
{
    for (const auto& m : measurements)
    {
        if (m.id == id)
            return &m;
    }
    return nullptr;
}

wxString getBenchmarkSectionId(BenchmarkCategory section)
{
    const BenchmarkCategory reportSection = getBenchmarkReportSection(section);
    for (const auto& id : sectionIds)
    {
        if (id.first == reportSection)
            return id.second;
    }
    return "settings";
}

BenchmarkSnapshot makeBenchmarkSnapshot(const BenchmarkReport& report)
{
    BenchmarkSnapshot s;
    s.title = getBenchmarkSystemTitle(report);
    s.host = report.anonymous ? wxString() : report.metrics.localHostName;
    s.date = report.startedAt;
    s.kind = getBenchmarkKindId(report.metrics);
    for (const auto& i : intensityIds)
    {
        if (report.intensity == i.first)
            s.intensity = i.second;
    }
    s.program = wxString::Format("%d.%d.%d", FR_VERSION_MAJOR, FR_VERSION_MINOR,
        FR_VERSION_RLS);

    const BenchmarkMetrics& m = report.metrics;
    const BenchmarkValues values = getBenchmarkValues(m);
    const std::map<wxString, BenchmarkWork> work = getBenchmarkWork(m);
    // a key value with the work behind it
    auto keyValue = [&values, &work](const BenchmarkMeasurementDef& def,
        BenchmarkMeasurement& x)
    {
        x.value = values.*def.value;
        x.title = wxGetTranslation(def.title);
        x.text = formatBenchmarkValue(x.value, def);
        auto w = work.find(def.id);
        if (w != work.end())
        {
            x.amount = w->second.amount;
            x.amountUnit = w->second.unit;
            x.ms = w->second.ms;
        }
    };
    for (const auto& r : report.results)
    {
        // the configuration is compared as facts
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(r.id);
        if (!def || def->section == BenchmarkCategory::Environment || s.findMeasurement(r.id))
            continue;
        const bool skipped = std::any_of(m.skippedTests.begin(), m.skippedTests.end(),
            [&r](const BenchmarkMetrics::SkippedTest& t) { return t.id == r.id; });
        BenchmarkMeasurement x;
        x.id = r.id;
        x.unit = def->unit;
        x.version = def->version;
        x.title = r.test;
        x.text = r.value;
        if (!skipped && def->kind == BenchmarkMeasurementKind::Key)
        {
            // e.g. the network of the embedded engine: no key value
            if (values.*def->value <= 0.0)
                continue;
            keyValue(*def, x);
        }
        else if (!skipped && def->kind == BenchmarkMeasurementKind::Measure)
        {
            x.value = measuredValue(*def, r.amountValue, r.amountUnit, r.durationMs);
            if (x.value > 0.0)
            {
                x.amount = r.amountValue;
                x.amountUnit = r.amountUnit;
                x.ms = r.durationMs;
            }
        }
        s.measurements.push_back(x);
    }
    // the key values without a result of their own: the best OLTP level
    for (const BenchmarkMeasurementDef& def : getBenchmarkMeasurementDefs())
    {
        if (def.kind == BenchmarkMeasurementKind::Key && def.value
            && values.*def.value > 0.0 && !s.findMeasurement(def.id))
        {
            BenchmarkMeasurement x;
            x.id = def.id;
            x.unit = def.unit;
            x.version = def.version;
            keyValue(def, x);
            s.measurements.push_back(x);
        }
    }
    for (const auto& f : getBenchmarkFacts(report))
        s.facts.push_back({ f.key, f.value });
    s.hints = report.hints;
    return s;
}

BenchmarkSnapshot anonymizeBenchmarkSnapshot(const BenchmarkSnapshot& snapshot)
{
    BenchmarkSnapshot s = snapshot;
    // a client run is named after its server, a machine without a known
    // model after itself
    if (s.kind == "client")
        s.title = _("Firebird server");
    else if (!s.host.empty() && s.title.CmpNoCase(s.host) == 0)
        s.title = _("This computer");
    s.host.clear();
    for (auto& f : s.facts)
    {
        if (f.first == "system")
            f.second = s.title;
        else if (f.first.StartsWith("conf.") && (f.second.Contains("/") || f.second.Contains("\\")))
            f.second = _("<path>");
        else if (f.first.StartsWith("conf.") && isBenchmarkAddressSetting(f.first.Mid(5), f.second))
            f.second = _("<address>");
    }
    // the details name adapters, addresses and drives
    for (auto& h : s.hints)
        h.detail.clear();
    return s;
}

BenchmarkComparison anonymizeBenchmarkComparison(const BenchmarkComparison& comparison)
{
    BenchmarkComparison c = compareBenchmarkSnapshots(
        anonymizeBenchmarkSnapshot(comparison.newer),
        anonymizeBenchmarkSnapshot(comparison.older));
    c.newerIsCurrentRun = comparison.newerIsCurrentRun;
    for (const BenchmarkSnapshot* s : { &comparison.newer, &comparison.older })
    {
        wxString host = s->host;
        host.Trim().Trim(false);
        if (host.length() >= 3)
            c.hiddenTexts.push_back({ host, _("<hidden>") });
    }
    std::stable_sort(c.hiddenTexts.begin(), c.hiddenTexts.end(),
        [](const auto& a, const auto& b) { return a.first.length() > b.first.length(); });
    return c;
}

wxString formatBenchmarkSnapshot(const BenchmarkSnapshot& s)
{
    // an HTML comment: invisible in Markdown viewers and in the browser,
    // kept when the report is saved, copied or pasted into a ticket
    wxString text = wxString("<!-- ") + snapshotMarker + " (used by FlameRobin to "
        "compare reports)\n";
    text += wxString::Format("format = %d.%d\n", formatMajor, formatMinor);
    text += "program = " + clean(s.program) + "\n";
    text += "run.title = " + clean(s.title) + "\n";
    if (!s.host.empty())
        text += "run.host = " + clean(s.host) + "\n";
    text += "run.date = " + clean(s.date) + "\n";
    text += "run.kind = " + clean(s.kind) + "\n";
    text += "run.intensity = " + clean(s.intensity) + "\n";
    // value | unit | version | amount | unit of the amount | ms | title |
    // text: the first six fields are read by their position, later minor
    // formats may add fields at the end
    for (const auto& m : s.measurements)
    {
        text += "m." + m.id + " = " + canonicalNumber(m.value) + " | " + clean(m.unit)
            + wxString::Format(" | v%d | ", m.version) + canonicalNumber(m.amount) + " | "
            + clean(m.amountUnit) + " | " + canonicalNumber(m.ms) + " | " + clean(m.title)
            + " | " + clean(m.text) + "\n";
    }
    for (const auto& f : s.facts)
        text += "fact." + f.first + " = " + clean(f.second) + "\n";
    for (const auto& h : s.hints)
    {
        text += "note." + h.id + " = " + getBenchmarkSectionId(h.category) + " | "
            + clean(h.title) + " | " + clean(h.detail) + " | " + clean(h.advice) + "\n";
    }
    return text + "-->\n";
}

wxString getBenchmarkComparisonMarker()
{
    return "<!-- FlameRobin benchmark comparison (a saved comparison of two runs; it has "
        "no results of its own and cannot be compared again) -->\n";
}

bool isBenchmarkComparisonPage(const wxString& text)
{
    return text.find("<!-- FlameRobin benchmark comparison (") != wxString::npos;
}

size_t findBenchmarkSnapshot(const wxString& text)
{
    const size_t start = text.find(wxString("<!-- ") + snapshotMarker);
    if (start != wxString::npos)
        return start;
    return text.find(wxString("<!-- ") + oldSnapshotMarker);
}

BenchmarkSnapshotResult parseBenchmarkSnapshot(const wxString& text)
{
    BenchmarkSnapshotResult result;
    size_t start = text.find(snapshotMarker);
    if (start == wxString::npos)
        start = text.find(oldSnapshotMarker);
    if (start == wxString::npos)
        return result;
    // a report that was cut off, e.g. when it was copied
    const size_t end = text.find("-->", start);
    result.status = BenchmarkSnapshotStatus::Damaged;
    if (end == wxString::npos)
        return result;

    // key = value; Windows line ends as well
    std::vector<std::pair<wxString, wxString>> lines;
    for (wxString line : wxSplit(text.Mid(start, end - start).AfterFirst('\n'), '\n', '\0'))
    {
        line.Replace("\r", "");
        if (line.Contains("="))
        {
            lines.push_back({ line.BeforeFirst('=').Trim().Trim(false),
                line.AfterFirst('=').Trim().Trim(false) });
        }
    }

    // the format decides how the rest is read: a newer minor format only
    // adds lines and fields, another major format is not compatible
    auto format = std::find_if(lines.begin(), lines.end(),
        [](const auto& line) { return line.first == "format"; });
    if (format == lines.end())
        return result;
    result.format = format->second;
    long major = 0;
    const wxString majorText = result.format.BeforeFirst('.');
    if (!isDigits(majorText) || !majorText.ToLong(&major)
        || (result.format.Contains(".") && !isDigits(result.format.AfterFirst('.'))))
    {
        return result;
    }
    if (major < formatMajor)
    {
        result.status = BenchmarkSnapshotStatus::TooOld;
        return result;
    }
    if (major > formatMajor)
    {
        result.status = BenchmarkSnapshotStatus::TooNew;
        return result;
    }

    BenchmarkSnapshot& s = result.snapshot;
    for (const auto& line : lines)
    {
        const wxString& key = line.first;
        const wxString& value = line.second;
        if (key == "program")
            s.program = value;
        else if (key == "run.title")
            s.title = value;
        else if (key == "run.host")
            s.host = value;
        else if (key == "run.date")
            s.date = value;
        else if (key == "run.kind")
            s.kind = value;
        else if (key == "run.intensity")
            s.intensity = value;
        else if (key.StartsWith("m."))
        {
            // a line that cannot be read is skipped like an unknown one
            const wxString id = key.Mid(2);
            wxArrayString fields = split(value);
            while (fields.size() < 8)
                fields.push_back(wxEmptyString);
            BenchmarkMeasurement x;
            x.id = id;
            x.unit = fields[1];
            x.amountUnit = fields[4];
            x.title = fields[6];
            x.text = fields[7];
            long version = 0;
            if (isBenchmarkMeasurementId(id) && !s.findMeasurement(id)
                && parseNumber(fields[0], &x.value) && fields[2].StartsWith("v")
                && isDigits(fields[2].Mid(1)) && fields[2].Mid(1).ToLong(&version)
                && version >= 1 && parseNumber(fields[3], &x.amount)
                && parseNumber(fields[5], &x.ms))
            {
                x.version = int(version);
                s.measurements.push_back(x);
            }
        }
        else if (key.StartsWith("fact."))
            s.facts.push_back({ key.Mid(5), value });
        else if (key.StartsWith("note."))
        {
            const wxArrayString fields = split(value);
            BenchmarkHint h;
            h.id = key.Mid(5);
            if (fields.size() >= 4 && isBenchmarkMeasurementId(h.id)
                && parseSection(fields[0], &h.category))
            {
                h.title = fields[1];
                h.detail = fields[2];
                h.advice = fields[3];
                s.hints.push_back(h);
            }
        }
    }
    result.status = BenchmarkSnapshotStatus::Ok;
    return result;
}

wxString getBenchmarkSnapshotProblem(const BenchmarkSnapshotResult& result)
{
    switch (result.status)
    {
        case BenchmarkSnapshotStatus::Ok:
            return wxEmptyString;
        case BenchmarkSnapshotStatus::TooOld:
            return wxString::Format(_("The report was made with an older version of "
                "FlameRobin (report format %s) and cannot be compared. Run the test "
                "again with this version."), result.format);
        case BenchmarkSnapshotStatus::TooNew:
            return wxString::Format(_("The report was made with a newer version of "
                "FlameRobin (report format %s). Please update FlameRobin to compare "
                "it."), result.format);
        case BenchmarkSnapshotStatus::Damaged:
            return _("The results in the report are incomplete or damaged and cannot "
                "be read.");
        case BenchmarkSnapshotStatus::NotAReport:
        default:
            return _("The file contains no benchmark results. Please select a report "
                "that was saved or copied with FlameRobin (HTML or Markdown).");
    }
}

wxString hideBenchmarkSnapshotTexts(const wxString& comment,
    const std::function<wxString(const wxString&)>& hide)
{
    // only the texts: the key of every line, and the value, unit, version
    // and work of a measurement and the section of a note stay
    wxString result;
    const wxArrayString lines = wxSplit(comment, '\n', '\0');
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const wxString& line = lines[i];
        const int equals = line.Find(" = ");
        if (equals == wxNOT_FOUND)
            result += line;
        else
        {
            const wxString key = line.Left(equals);
            wxArrayString fields = wxSplit(line.Mid(equals + 3), '|', '\0');
            const size_t kept = key.StartsWith("m.") ? 6 : key.StartsWith("note.") ? 1 : 0;
            for (size_t f = kept; f < fields.size(); ++f)
                fields[f] = hide(fields[f]);
            result += key + " = " + wxJoin(fields, '|', '\0');
        }
        if (i + 1 < lines.size())
            result += "\n";
    }
    return result;
}

bool isBenchmarkFactOfPart(const wxString& key, BenchmarkCategory part, bool serverRun)
{
    using C = BenchmarkCategory;
    auto in = [&key](std::initializer_list<const char*> keys)
    {
        return std::any_of(keys.begin(), keys.end(),
            [&key](const char* k) { return key == k; });
    };
    // the machine running FlameRobin; on a run on the server that is the
    // server as well
    const bool machine = in({ "os", "cpu", "cores", "ram", "power", "scanner", "virtual" });
    if (part == C::LocalMachine)
        return machine && key != "ram";
    if (machine && !serverRun)
        return false;
    switch (part)
    {
        case C::ServerCpu:
            return in({ "firebird", "os", "cpu", "cores", "power", "virtual",
                "conf.ServerMode", "conf.CpuAffinityMask" });
        case C::ServerStorage:
            return in({ "firebird", "os", "storage", "scanner", "virtual", "pagesize",
                "forcedwrites", "conf.TempDirectories", "conf.TempCacheLimit" });
        case C::ServerCache:
            return in({ "firebird", "cpu", "ram", "pagesize", "pagecache",
                "conf.ServerMode", "conf.DefaultDbCachePages", "conf.UseFileSystemCache",
                "conf.FileSystemCacheThreshold", "conf.TempCacheLimit" });
        case C::Network:
            return in({ "firebird", "access", "network", "wire", "conf.WireCompression",
                "conf.WireCrypt", "conf.TcpRemoteBufferSize", "conf.AuthServer",
                "conf.AuthClient", "conf.RemoteServicePort", "conf.RemoteAuxPort" });
        case C::MultiUser:
            return in({ "firebird", "cores", "forcedwrites", "conf.ServerMode",
                "conf.CpuAffinityMask", "conf.LockHashSlots", "conf.DefaultDbCachePages",
                "conf.ParallelWorkers", "conf.MaxParallelWorkers" });
        default:
            return false;
    }
}

BenchmarkComparison compareBenchmarkSnapshots(const BenchmarkSnapshot& newer,
    const BenchmarkSnapshot& older)
{
    BenchmarkComparison c;
    c.newer = newer;
    c.older = older;
    c.sameKind = newer.kind == older.kind && newer.intensity == older.intensity;

    // a measurement of both runs, matched by its id; it is compared only if
    // both runs have it from the same version of the test and, where every
    // run of an intensity does the same work, with the same work. The key
    // values of runs of different intensities are compared as well, roughly
    // (see sameKind).
    const bool sameIntensity = newer.intensity == older.intensity;
    auto compare = [sameIntensity](const wxString& id, BenchmarkCategory section,
        const BenchmarkMeasurement* n, const BenchmarkMeasurement* o)
    {
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(id);
        auto text = [def](const BenchmarkMeasurement* x)
        {
            if (!x)
                return dash();
            if (def && def->kind == BenchmarkMeasurementKind::Key && x->value > 0.0)
                return formatBenchmarkValue(x->value, *def);
            return x->text.empty() ? dash() : x->text;
        };
        auto work = [](const BenchmarkMeasurement* x)
        {
            return x ? formatBenchmarkWork(formatBenchmarkAmount(x->amount, x->amountUnit),
                x->ms) : wxString();
        };
        BenchmarkComparedValue v;
        v.category = section;
        v.id = id;
        v.title = n ? n->title : o->title;
        if (def && def->kind == BenchmarkMeasurementKind::Key)
            v.title = wxGetTranslation(def->title);
        else if (v.title.empty())
            v.title = id;
        v.newerText = text(n);
        v.olderText = text(o);
        v.newerWork = work(n);
        v.olderWork = work(o);
        v.newerVersion = n ? n->version : 0;
        v.olderVersion = o ? o->version : 0;
        if (!o)
            v.state = BenchmarkCompareState::OnlyNewer;
        else if (!n)
            v.state = BenchmarkCompareState::OnlyOlder;
        else if (n->version != o->version)
            v.state = BenchmarkCompareState::TestChanged;
        // information, an id this version does not know, or a test that did
        // not run in one of the runs: shown, not compared
        else if (def && def->kind != BenchmarkMeasurementKind::Info && n->value > 0.0
            && o->value > 0.0)
        {
            // a value without its work (e.g. a commit faster than the round
            // trips it was corrected by) cannot differ in its work
            const bool sameWork = n->amountUnit.empty() || o->amountUnit.empty()
                || (n->amount == o->amount && n->amountUnit == o->amountUnit);
            if (!sameWork && def->amountFixed
                && (def->kind == BenchmarkMeasurementKind::Measure || sameIntensity))
            {
                v.state = BenchmarkCompareState::WorkChanged;
            }
            else
            {
                v.ratio = roundRatio(def->higherIsBetter ? n->value / o->value
                    : o->value / n->value);
            }
        }
        return v;
    };

    // the key values in the order of the register
    for (const BenchmarkMeasurementDef& def : getBenchmarkMeasurementDefs())
    {
        const BenchmarkMeasurement* n = newer.findMeasurement(def.id);
        const BenchmarkMeasurement* o = older.findMeasurement(def.id);
        if (def.kind == BenchmarkMeasurementKind::Key && (n || o))
            c.values.push_back(compare(def.id, def.section, n, o));
    }
    // the other measurements in the order of the newer run, then those that
    // only the older one has
    auto isKey = [](const wxString& id)
    {
        const BenchmarkMeasurementDef* def = findBenchmarkMeasurementDef(id);
        return def && def->kind == BenchmarkMeasurementKind::Key;
    };
    for (const auto& n : newer.measurements)
    {
        BenchmarkCategory section;
        if (!isKey(n.id) && sectionOfId(n.id, &section))
            c.stats.push_back(compare(n.id, section, &n, older.findMeasurement(n.id)));
    }
    for (const auto& o : older.measurements)
    {
        BenchmarkCategory section;
        if (!isKey(o.id) && !newer.findMeasurement(o.id) && sectionOfId(o.id, &section))
            c.stats.push_back(compare(o.id, section, nullptr, &o));
    }

    // the facts of both runs in the order of the newer one
    auto find = [](const BenchmarkSnapshot& s, const wxString& key) -> const wxString*
    {
        for (const auto& f : s.facts)
        {
            if (f.first == key)
                return &f.second;
        }
        return nullptr;
    };
    std::vector<wxString> keys;
    for (const auto* s : { &newer, &older })
    {
        for (const auto& f : s->facts)
        {
            if (std::find(keys.begin(), keys.end(), f.first) == keys.end())
                keys.push_back(f.first);
        }
    }
    for (const wxString& key : keys)
    {
        const wxString* n = find(newer, key);
        const wxString* o = find(older, key);
        if (n && o && *n == *o)
            continue;
        c.differences.push_back({ key, getBenchmarkFactLabel(key), n ? *n : dash(),
            o ? *o : dash() });
    }

    // every part: the geometric mean of its key values, and where it
    // differs clearly, what could make the slower run slower
    std::vector<double> partRatios;
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        if (section == BenchmarkCategory::Environment)
            continue;
        BenchmarkComparedPart part;
        part.category = section;
        std::vector<double> ratios;
        bool measured = false;
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const auto& v : *list)
                measured = measured || v.category == section;
        }
        for (const auto& v : c.values)
        {
            if (v.category == section && v.ratio > 0.0)
            {
                ratios.push_back(std::clamp(v.ratio, 1.0 / maximumValueRatio,
                    maximumValueRatio));
            }
        }
        if (!measured)
            continue;
        part.ratio = geometricMean(ratios);
        if (part.ratio > 0.0)
            partRatios.push_back(part.ratio);
        if (getBenchmarkChangeLevel(part.ratio) > 0)
        {
            const bool newerSlower = part.ratio < 1.0;
            const BenchmarkSnapshot& slower = newerSlower ? newer : older;
            const bool serverRun = slower.kind != "client";
            for (const auto& d : c.differences)
            {
                if (isBenchmarkFactOfPart(d.key, section, serverRun))
                    part.differences.push_back(d);
            }
            for (const auto& h : slower.hints)
            {
                if (h.category == section)
                    part.hints.push_back(h);
            }
            // the typical causes of the values that changed most, so that
            // the tips stay short
            std::vector<const BenchmarkComparedValue*> slowerValues;
            for (const auto& v : c.values)
            {
                if (v.category == section && getBenchmarkChangeLevel(v.ratio) > 0
                    && (v.ratio < 1.0) == newerSlower
                    && !getBenchmarkValueAdvice(v.id).empty())
                {
                    slowerValues.push_back(&v);
                }
            }
            std::stable_sort(slowerValues.begin(), slowerValues.end(),
                [](const BenchmarkComparedValue* a, const BenchmarkComparedValue* b)
                {
                    return std::abs(std::log(a->ratio)) > std::abs(std::log(b->ratio));
                });
            if (slowerValues.size() > maximumAdvice)
                slowerValues.resize(maximumAdvice);
            for (const BenchmarkComparedValue* v : slowerValues)
                part.advice.push_back({ v->title, getBenchmarkValueAdvice(v->id) });
        }
        c.parts.push_back(part);
    }
    c.ratio = geometricMean(partRatios);
    return c;
}

const BenchmarkComparedPart* BenchmarkComparison::findPart(BenchmarkCategory category) const
{
    for (const auto& p : parts)
    {
        if (p.category == category)
            return &p;
    }
    return nullptr;
}

std::vector<const BenchmarkComparedValue*> BenchmarkComparison::getLargestChanges(
    size_t maximum) const
{
    std::vector<const BenchmarkComparedValue*> list;
    for (const auto& v : values)
    {
        if (getBenchmarkChangeLevel(v.ratio) > 0)
            list.push_back(&v);
    }
    std::stable_sort(list.begin(), list.end(),
        [](const BenchmarkComparedValue* a, const BenchmarkComparedValue* b)
        {
            return std::abs(std::log(a->ratio)) > std::abs(std::log(b->ratio));
        });
    if (list.size() > maximum)
        list.resize(maximum);
    return list;
}

int getBenchmarkChangeLevel(double ratio)
{
    if (ratio <= 0.0)
        return 0;
    // repeated runs on the same machine differ by up to about 10 %
    const double factor = std::max(ratio, 1.0 / ratio);
    if (factor < 1.15)
        return 0;
    return factor < 1.4 ? 1 : 2;
}

wxString formatBenchmarkChange(double ratio)
{
    if (ratio <= 0.0)
        return dash();
    if (getBenchmarkChangeLevel(ratio) == 0)
        return _("about the same");
    if (ratio >= 2.0)
        return wxString::Format(_("%.1f times as fast"), ratio);
    if (ratio > 1.0)
        return wxString::Format(_("%.0f %% faster"), (ratio - 1.0) * 100.0);
    if (ratio <= 0.5)
        return wxString::Format(_("%.1f times slower"), 1.0 / ratio);
    return wxString::Format(_("%.0f %% slower"), (1.0 - ratio) * 100.0);
}

wxString describeBenchmarkComparison(const BenchmarkComparison& c)
{
    const double ratio = c.ratio;
    if (ratio <= 0.0)
        return _("The two runs have no values in common.");
    const wxString subject = c.newerIsCurrentRun ? _("This run") : _("The later run");
    if (getBenchmarkChangeLevel(ratio) == 0)
    {
        return wxString::Format(_("%s is about as fast overall as the earlier run."),
            subject);
    }
    return wxString::Format(_("%s is %s overall than the earlier run."), subject,
        ratio > 1.0 ? (ratio >= 2.0 ? wxString::Format(_("%.1f times faster"), ratio)
            : wxString::Format(_("%.0f %% faster"), (ratio - 1.0) * 100.0))
        : (ratio <= 0.5 ? wxString::Format(_("%.1f times slower"), 1.0 / ratio)
            : wxString::Format(_("%.0f %% slower"), (1.0 - ratio) * 100.0)));
}

wxString getBenchmarkReasonsTitle(const BenchmarkComparison& c,
    const BenchmarkComparedPart& part)
{
    if (part.ratio >= 1.0)
        return _("What could make the earlier run slower");
    return c.newerIsCurrentRun ? _("What could make this run slower")
        : _("What could make the later run slower");
}

BenchmarkVerdict getBenchmarkVerdict(const BenchmarkComparison& comparison)
{
    BenchmarkVerdict verdict;
    for (const BenchmarkHeadlineChange& change : getBenchmarkHeadlineChanges(comparison))
    {
        if (change.state != BenchmarkCompareState::Compared || change.ratio <= 0.0)
            ++verdict.notComparable;
        else if (getBenchmarkChangeLevel(change.ratio) == 0)
            ++verdict.same;
        else if (change.ratio > 1.0)
            ++verdict.faster;
        else
            ++verdict.slower;
    }
    return verdict;
}

std::vector<std::pair<int, wxString>> describeBenchmarkVerdict(const BenchmarkVerdict& verdict)
{
    // the first count names what is counted
    std::vector<std::pair<int, wxString>> list;
    auto add = [&list](int direction, int count, const wxString& first, const wxString& next)
    {
        if (count > 0)
            list.push_back({ direction, list.empty() ? first : next });
    };
    add(1, verdict.faster, wxString::Format(wxPLURAL("%d key figure faster",
        "%d key figures faster", verdict.faster), verdict.faster),
        wxString::Format(_("%d faster"), verdict.faster));
    add(-1, verdict.slower, wxString::Format(wxPLURAL("%d key figure slower",
        "%d key figures slower", verdict.slower), verdict.slower),
        wxString::Format(_("%d slower"), verdict.slower));
    add(0, verdict.same, wxString::Format(wxPLURAL("%d key figure about the same",
        "%d key figures about the same", verdict.same), verdict.same),
        wxString::Format(_("%d about the same"), verdict.same));
    return list;
}

std::vector<wxString> getBenchmarkVerdictNotes(const BenchmarkComparison& comparison)
{
    std::vector<wxString> notes;
    for (const BenchmarkComparedPart& p : comparison.parts)
    {
        const wxString word = getBenchmarkReportSectionWord(p.category);
        switch (getBenchmarkPartState(comparison, p.category))
        {
            case BenchmarkCompareState::OnlyNewer:
                notes.push_back(wxString::Format(comparison.newerIsCurrentRun
                    ? _("%s measured in this run only")
                    : _("%s measured in the later run only"), word));
                break;
            case BenchmarkCompareState::OnlyOlder:
                notes.push_back(wxString::Format(_("%s measured in the earlier run only"),
                    word));
                break;
            case BenchmarkCompareState::Compared:
                break;
            default:
                notes.push_back(wxString::Format(_("%s not comparable"), word));
                break;
        }
    }
    const int notCompared = getBenchmarkVerdict(comparison).notComparable;
    if (notCompared > 0)
    {
        notes.push_back(wxString::Format(wxPLURAL("%d key figure not compared",
            "%d key figures not compared", notCompared), notCompared));
    }
    return notes;
}

BenchmarkCompareState getBenchmarkPartState(const BenchmarkComparison& comparison,
    BenchmarkCategory section)
{
    using S = BenchmarkCompareState;
    const BenchmarkComparedPart* part = comparison.findPart(section);
    if (part && part->ratio > 0.0)
        return S::Compared;
    int newer = 0, older = 0, compared = 0, other = 0;
    for (const auto* list : { &comparison.values, &comparison.stats })
    {
        for (const BenchmarkComparedValue& v : *list)
        {
            if (v.category != section)
                continue;
            if (v.state == S::OnlyNewer)
                ++newer;
            else if (v.state == S::OnlyOlder)
                ++older;
            else if (v.state == S::Compared)
                ++compared;
            else
                ++other;
        }
    }
    if (compared > 0 || newer + older + other == 0)
        return S::Compared;
    if (older + other == 0)
        return S::OnlyNewer;
    if (newer + other == 0)
        return S::OnlyOlder;
    return S::TestChanged;
}

std::vector<const BenchmarkComparedPart*> getBenchmarkStandOut(
    const BenchmarkComparison& comparison, size_t maximum)
{
    std::vector<const BenchmarkComparedPart*> list;
    for (const BenchmarkComparedPart& p : comparison.parts)
    {
        if (getBenchmarkChangeLevel(p.ratio) > 0)
            list.push_back(&p);
    }
    std::stable_sort(list.begin(), list.end(),
        [](const BenchmarkComparedPart* a, const BenchmarkComparedPart* b)
        {
            return std::abs(std::log(a->ratio)) > std::abs(std::log(b->ratio));
        });
    if (list.size() > maximum)
        list.resize(maximum);
    return list;
}

wxString getBenchmarkShortAdvice(const wxString& advice)
{
    // an advice can start with the current setting, which is no advice
    const wxString current = wxString(_("Current setting: %s.")).BeforeFirst('%');
    wxString text = advice;
    if (!current.empty() && text.StartsWith(current) && text.find(". ") != wxString::npos)
        text = text.Mid(text.find(". ") + 2);
    const size_t end = text.find(". ");
    return end == wxString::npos ? text : text.Left(end + 1);
}

wxString getBenchmarkLikelyReason(const BenchmarkComparedPart& part)
{
    // a fact that differs explains more than a note of one run; the
    // typical causes are the list of reasons itself, not one of them
    if (!part.differences.empty())
    {
        const BenchmarkComparedFact& d = part.differences.front();
        return d.label + ": " + d.olderText + arrow() + d.newerText;
    }
    if (!part.hints.empty())
        return part.hints.front().title;
    return wxEmptyString;
}

wxString getBenchmarkTryAdvice(const BenchmarkComparedPart& part)
{
    if (!part.differences.empty())
    {
        const wxString advice = getBenchmarkFactAdvice(part.differences.front().key);
        if (!advice.empty())
            return advice;
    }
    for (const BenchmarkHint& h : part.hints)
    {
        if (!h.advice.empty())
            return getBenchmarkShortAdvice(h.advice);
    }
    if (!part.differences.empty() || !part.advice.empty())
    {
        return _("Repeat the slower run under the same conditions; if the difference "
            "stays, check the possible reasons one by one.");
    }
    return wxEmptyString;
}

wxString getBenchmarkCompareStateName(BenchmarkCompareState state)
{
    switch (state)
    {
        case BenchmarkCompareState::OnlyNewer:
            return _("new");
        case BenchmarkCompareState::OnlyOlder:
            return _("no longer measured");
        case BenchmarkCompareState::TestChanged:
        case BenchmarkCompareState::WorkChanged:
            return _("not comparable");
        case BenchmarkCompareState::Compared:
        default:
            return wxEmptyString;
    }
}

wxString describeBenchmarkCompareState(const BenchmarkComparison& comparison,
    const BenchmarkComparedValue& value)
{
    if (value.state == BenchmarkCompareState::TestChanged)
    {
        return wxString::Format(_("The test changed (version %d, earlier version %d)."),
            value.newerVersion, value.olderVersion);
    }
    if (value.state != BenchmarkCompareState::WorkChanged)
        return wxEmptyString;
    auto amount = [&value](const BenchmarkSnapshot& run)
    {
        const BenchmarkMeasurement* x = run.findMeasurement(value.id);
        return x ? formatBenchmarkAmount(x->amount, x->amountUnit) : wxString();
    };
    return wxString::Format(_("Other work: %s, earlier %s."), amount(comparison.newer),
        amount(comparison.older));
}

wxString formatBenchmarkComparedWork(const BenchmarkComparison& comparison,
    const BenchmarkComparedValue& value)
{
    const BenchmarkMeasurement* n = comparison.newer.findMeasurement(value.id);
    const BenchmarkMeasurement* o = comparison.older.findMeasurement(value.id);
    // the same work: its amount once and both times
    if (n && o && n->amount > 0.0 && n->amount == o->amount
        && n->amountUnit == o->amountUnit && n->ms > 0.0 && o->ms > 0.0)
    {
        return formatBenchmarkAmount(n->amount, n->amountUnit) + ": "
            + formatBenchmarkMilliseconds(o->ms) + arrow() + formatBenchmarkMilliseconds(n->ms);
    }
    if (value.olderWork.empty() || value.olderWork == value.newerWork)
        return value.newerWork;
    if (value.newerWork.empty())
        return value.olderWork;
    return value.olderWork + arrow() + value.newerWork;
}

wxString formatBenchmarkComparisonMarkdown(const BenchmarkComparison& c, bool standalone)
{
    const wxString older = olderLabel();
    const wxString newer = newerLabel(c);

    wxString md;
    if (standalone)
    {
        md << "## " << _("Comparison of two benchmark runs") << "\n\n";
        md << "| | " << older << " | " << newer << " |\n|---|---|---|\n";
        md << "| " << _("System") << " | " << cell(c.older.title) << " | "
           << cell(c.newer.title) << " |\n";
        if (!c.older.host.empty() || !c.newer.host.empty())
        {
            md << "| " << _("Computer") << " | " << cell(c.older.host) << " | "
               << cell(c.newer.host) << " |\n";
        }
        md << "| " << _("Date") << " | " << cell(c.older.date) << " | " << cell(c.newer.date)
           << " |\n";
        md << "| " << _("Test") << " | " << cell(runText(c.older)) << " | "
           << cell(runText(c.newer)) << " |\n\n";
    }
    else
    {
        md << "### " << wxString::Format(_("Compared with %s (%s)"), c.older.title,
            c.older.date) << "\n\n";
    }
    // the main statement: the key figures by their change, the points, and
    // what one run did not measure
    const wxString dot = wxString(" ") + wxUniChar(0x00B7) + " ";
    std::vector<wxString> counts;
    for (const auto& count : describeBenchmarkVerdict(getBenchmarkVerdict(c)))
        counts.push_back(count.second);
    if (!counts.empty())
        md << "**" << join(counts, dot) << "**\n\n";
    std::vector<wxString> notes;
    if (c.ratio > 0.0)
    {
        notes.push_back(wxString::Format(_("Overall %s points; the earlier run has %s"),
            pointsText(c.ratio), pointsText(1.0)));
    }
    else
        notes.push_back(describeBenchmarkComparison(c));
    for (const wxString& note : getBenchmarkVerdictNotes(c))
        notes.push_back(note);
    md << join(notes, dot) << "\n\n";
    if (c.ratio > 0.0)
        md << _("Differences of less than 15 % are normal between two runs.") << "\n\n";
    if (!c.sameKind)
    {
        md << "> " << wxString::Format(_("The runs were made in different ways (%s and %s) "
            "and are only roughly comparable."), runText(c.older), runText(c.newer)) << "\n\n";
    }

    // one line per key figure
    const std::vector<BenchmarkHeadlineChange> figures = getBenchmarkHeadlineChanges(c);
    if (!figures.empty())
    {
        md << "| " << _("Part") << " | " << _("Key figure") << " | " << older << " | " << newer
           << " | " << _("Change") << " | " << _("Points") << " |\n|---|---|---:|---:|---|---:|\n";
        for (const BenchmarkHeadlineChange& h : figures)
        {
            auto number = [&h](const wxString& text)
            {
                return text.empty() ? dash() : text + " " + h.unit;
            };
            wxString points = dash();
            if (h.state == BenchmarkCompareState::Compared && h.ratio > 0.0)
            {
                const long delta = std::lround(h.ratio * BenchmarkComparison::earlierPoints)
                    - long(BenchmarkComparison::earlierPoints);
                points = pointsText(h.ratio) + " (" + (delta == 0
                    ? wxString(wxUniChar(0x00B1)) + "0" : wxString::Format("%+ld", delta)) + ")";
            }
            md << "| " << cell(getBenchmarkReportSectionWord(h.section)) << " | "
               << cell(h.name) << " | " << cell(number(h.olderNumber)) << " | "
               << cell(number(h.newerNumber)) << " | " << stateText(h.ratio, h.state)
               << " | " << points << " |\n";
        }
        md << "\n";
    }

    // the parts that differ clearly, with every possible reason
    const std::vector<const BenchmarkComparedPart*> standOut = getBenchmarkStandOut(c, 5);
    if (!standOut.empty())
    {
        md << "**" << _("What stands out") << "**\n\n";
        for (const BenchmarkComparedPart* p : standOut)
        {
            md << "- **" << getBenchmarkReportSectionWord(p->category) << "** "
               << changeMark(p->ratio) << dot << getBenchmarkReportSectionName(p->category)
               << "\n";
            const wxString likely = getBenchmarkLikelyReason(*p);
            const wxString advice = getBenchmarkTryAdvice(*p);
            if (!likely.empty())
                md << "  - " << _("Likely") << ": " << likely << "\n";
            if (!advice.empty())
                md << "  - " << _("What to try") << ": " << advice << "\n";
            if (p->differences.empty() && p->hints.empty() && p->advice.empty())
            {
                md << "  - " << _("No difference was found in the facts and notes of the two "
                    "runs.") << "\n";
                continue;
            }
            md << "  - " << getBenchmarkReasonsTitle(c, *p) << ":\n";
            for (const auto& d : p->differences)
                md << "    - " << d.label << ": " << d.olderText << arrow() << d.newerText << "\n";
            // the same advice of two hints once
            std::vector<wxString> shown;
            for (const auto& h : p->hints)
            {
                const bool repeated = std::find(shown.begin(), shown.end(), h.advice)
                    != shown.end();
                shown.push_back(h.advice);
                md << "    - " << h.title << (h.detail.empty() ? wxString() : ". " + h.detail)
                   << (h.advice.empty() || repeated ? wxString() : " " + h.advice) << "\n";
            }
            for (const auto& a : p->advice)
                md << "    - " << a.first << ": " << a.second << "\n";
        }
        md << "\n";
    }

    if (!c.differences.empty())
    {
        md << "**" << _("What is different") << "**\n\n| | " << older << " | " << newer
           << " |\n|---|---|---|\n";
        for (const auto& d : c.differences)
        {
            md << "| " << cell(d.label) << " | " << cell(d.olderText) << " | "
               << cell(d.newerText) << " |\n";
        }
        md << "\n";
    }

    // every value of both runs, one table per part
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        wxString rows;
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const auto& v : *list)
            {
                if (v.category != section)
                    continue;
                const wxString note = describeBenchmarkCompareState(c, v);
                rows << "| " << cell(v.title + (note.empty() ? wxString() : " (" + note + ")"))
                     << " | " << cell(v.olderText) << " | " << cell(v.newerText) << " | "
                     << stateText(v.ratio, v.state) << " | "
                     << cell(formatBenchmarkComparedWork(c, v)) << " |\n";
            }
        }
        if (rows.empty())
            continue;
        const BenchmarkComparedPart* part = c.findPart(section);
        wxString head = getBenchmarkReportSectionWord(section) + dot
            + getBenchmarkReportSectionName(section);
        if (part && part->ratio > 0.0)
        {
            head += ": " + wxString::Format(_("%s points"), pointsText(part->ratio)) + " ("
                + formatBenchmarkChange(part->ratio) + ")";
        }
        else if (getBenchmarkPartState(c, section) == BenchmarkCompareState::OnlyNewer)
        {
            head += " (" + (c.newerIsCurrentRun ? _("only in this run")
                : _("only in the later run")) + ")";
        }
        else if (getBenchmarkPartState(c, section) == BenchmarkCompareState::OnlyOlder)
            head += " (" + _("only in the earlier run") + ")";
        md << "#### " << head << "\n\n| " << _("Test") << " | " << older << " | " << newer
           << " | " << _("Change") << " | " << _("Work") << " |\n|---|---:|---:|---|---|\n"
           << rows << "\n";
    }
    return md;
}

wxString BenchmarkComparison::toMarkdown() const
{
    return hideBenchmarkTexts(formatBenchmarkComparisonMarkdown(*this, true)
        + "\n" + getBenchmarkComparisonMarker(), hiddenTexts, false);
}

} // namespace fr
