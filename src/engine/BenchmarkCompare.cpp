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

// Comparing two runs. A single run is not judged, because whether a value
// is fast or slow depends on the environment; a comparison shows it. Every
// report contains the results of its run as a hidden comment (the
// snapshot), so a saved report can be compared with a new run, or two
// saved reports with each other, without running a test. The earlier run
// gets 1000 points for every part and in total, the later run
// proportionally more or fewer. Where a part differs clearly, the
// comparison names what could make the slower run slower: the facts of the
// environment that differ, what was noticed on the slower run, and the
// typical causes of its slower values.

namespace fr
{

namespace
{

const char* const snapshotMarker = "FlameRobin benchmark results";

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

wxString sectionId(BenchmarkCategory section)
{
    for (const auto& id : sectionIds)
    {
        if (id.first == section)
            return id.second;
    }
    return "settings";
}

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

} // namespace

BenchmarkSnapshot makeBenchmarkSnapshot(const BenchmarkReport& report)
{
    BenchmarkSnapshot s;
    s.title = getBenchmarkSystemTitle(report);
    s.host = report.anonymous ? wxString() : report.metrics.localHostName;
    s.date = report.startedAt;
    s.kind = getBenchmarkKindId(report.metrics);
    for (const auto& i : intensityIds)
    {
        if (report.intensityName == getBenchmarkIntensityName(i.first))
            s.intensity = i.second;
    }
    s.values = getBenchmarkValues(report.metrics);
    s.work = getBenchmarkWork(report.metrics);
    for (const auto& r : report.results)
    {
        // the configuration is compared as facts
        const BenchmarkCategory section = getBenchmarkReportSection(r.category);
        if (section != BenchmarkCategory::Environment)
            s.stats.push_back({ section, r.valueKey, r.test, r.value, r.amount, r.durationMs });
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

wxString formatBenchmarkSnapshot(const BenchmarkSnapshot& s)
{
    // an HTML comment: invisible in Markdown viewers and in the browser,
    // kept when the report is saved, copied or pasted into a ticket
    wxString text = wxString("<!-- ") + snapshotMarker + " (used by FlameRobin to "
        "compare reports)\n";
    text += "format = 1\n";
    text += "title = " + clean(s.title) + "\n";
    if (!s.host.empty())
        text += "host = " + clean(s.host) + "\n";
    text += "date = " + clean(s.date) + "\n";
    text += "kind = " + s.kind + "\n";
    text += "intensity = " + s.intensity + "\n";
    for (const auto& v : formatBenchmarkValues(s.values))
    {
        if (v.second != "0")
            text += "value." + v.first + " = " + v.second + "\n";
    }
    for (const auto& w : s.work)
    {
        text += "work." + w.first + " = " + clean(w.second.first) + " | "
            + wxString::FromCDouble(w.second.second, 3) + "\n";
    }
    for (const auto& st : s.stats)
    {
        text += "stat = " + sectionId(st.section) + " | " + st.key + " | "
            + clean(st.test) + " | " + clean(st.value) + " | " + clean(st.amount)
            + " | " + wxString::FromCDouble(st.ms, 3) + "\n";
    }
    for (const auto& f : s.facts)
        text += "fact." + f.first + " = " + clean(f.second) + "\n";
    for (const auto& h : s.hints)
    {
        text += "hint = " + sectionId(h.category) + " | " + clean(h.title) + " | "
            + clean(h.detail) + " | " + clean(h.advice) + "\n";
    }
    return text + "-->\n";
}

std::optional<BenchmarkSnapshot> parseBenchmarkSnapshot(const wxString& text)
{
    const size_t start = text.find(snapshotMarker);
    if (start == wxString::npos)
        return std::nullopt;
    size_t end = text.find("-->", start);
    if (end == wxString::npos)
        end = text.length();
    wxString rest = text.Mid(start, end - start).AfterFirst('\n');

    BenchmarkSnapshot s;
    bool hasFormat = false;
    while (!rest.empty())
    {
        wxString line = rest.BeforeFirst('\n');
        rest = rest.AfterFirst('\n');
        line.Replace("\r", "");
        if (!line.Contains("="))
            continue;
        const wxString key = line.BeforeFirst('=').Trim().Trim(false);
        const wxString value = line.AfterFirst('=').Trim().Trim(false);
        if (key == "format")
            hasFormat = true;
        else if (key == "title")
            s.title = value;
        else if (key == "host")
            s.host = value;
        else if (key == "date")
            s.date = value;
        else if (key == "kind")
            s.kind = value;
        else if (key == "intensity")
            s.intensity = value;
        else if (key.StartsWith("value."))
            parseBenchmarkValue(s.values, key.Mid(6), value);
        else if (key.StartsWith("work."))
        {
            double ms = 0.0;
            value.AfterLast('|').Trim(false).ToCDouble(&ms);
            s.work[key.Mid(5)] = { value.BeforeLast('|').Trim(), ms };
        }
        else if (key == "stat")
        {
            const wxArrayString parts = split(value);
            BenchmarkSnapshot::Stat st;
            double ms = 0.0;
            if (parts.size() == 6 && parseSection(parts[0], &st.section)
                && parts[5].ToCDouble(&ms))
            {
                st.key = parts[1];
                st.test = parts[2];
                st.value = parts[3];
                st.amount = parts[4];
                st.ms = ms;
                s.stats.push_back(st);
            }
        }
        else if (key.StartsWith("fact."))
            s.facts.push_back({ key.Mid(5), value });
        else if (key == "hint")
        {
            const wxArrayString parts = split(value);
            BenchmarkHint h;
            if (parts.size() == 4 && parseSection(parts[0], &h.category))
            {
                h.title = parts[1];
                h.detail = parts[2];
                h.advice = parts[3];
                s.hints.push_back(h);
            }
        }
    }
    if (!hasFormat)
        return std::nullopt;
    return s;
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

    c.values = compareBenchmarkValues(newer.values, older.values);
    auto workOf = [](const BenchmarkSnapshot& s, const wxString& key)
    {
        auto w = s.work.find(key);
        return w == s.work.end() ? wxString()
            : formatBenchmarkWork(w->second.first, w->second.second);
    };
    for (auto& v : c.values)
    {
        v.newerWork = workOf(newer, v.key);
        v.olderWork = workOf(older, v.key);
    }

    // the other measurements by part and test; the same work in less time
    // is faster
    auto findStat = [](const BenchmarkSnapshot& s, const BenchmarkSnapshot::Stat& like)
        -> const BenchmarkSnapshot::Stat*
    {
        for (const auto& st : s.stats)
        {
            if (st.section == like.section && st.test == like.test)
                return &st;
        }
        return nullptr;
    };
    auto addStat = [&c](const BenchmarkSnapshot::Stat& like,
        const BenchmarkSnapshot::Stat* n, const BenchmarkSnapshot::Stat* o)
    {
        BenchmarkComparedValue v;
        v.category = like.section;
        v.title = like.test;
        v.newerText = n ? n->value : dash();
        v.olderText = o ? o->value : dash();
        v.newerWork = n ? formatBenchmarkWork(n->amount, n->ms) : wxString();
        v.olderWork = o ? formatBenchmarkWork(o->amount, o->ms) : wxString();
        if (n && o && n->ms > 0.0 && o->ms > 0.0 && n->amount == o->amount)
            v.ratio = o->ms / n->ms;
        c.stats.push_back(v);
    };
    for (const auto& st : newer.stats)
    {
        if (st.key.empty())
            addStat(st, &st, findStat(older, st));
    }
    for (const auto& st : older.stats)
    {
        if (st.key.empty() && !findStat(newer, st))
            addStat(st, nullptr, &st);
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
                    && !getBenchmarkValueAdvice(v.key).empty())
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
                part.advice.push_back({ v->title, getBenchmarkValueAdvice(v->key) });
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

wxString formatBenchmarkComparisonMarkdown(const BenchmarkComparison& c, bool standalone)
{
    auto changeText = [](double ratio)
    {
        const wxString change = formatBenchmarkChange(ratio);
        return getBenchmarkChangeLevel(ratio) == 2 ? "**" + change + "**" : change;
    };
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
    md << "**" << describeBenchmarkComparison(c) << "**\n\n";
    if (!c.sameKind)
    {
        md << "> " << wxString::Format(_("The runs were made in different ways (%s and %s) "
            "and are only roughly comparable."), runText(c.older), runText(c.newer)) << "\n\n";
    }

    // the points of every part: the earlier run has 1000
    md << "| " << _("Part") << " | " << older << " | " << newer << " | " << _("Change")
       << " |\n|---|---:|---:|---|\n";
    md << "| **" << _("All parts") << "** | **" << pointsText(c.ratio > 0.0 ? 1.0 : 0.0)
       << "** | **" << pointsText(c.ratio) << "** | " << changeText(c.ratio) << " |\n";
    for (const auto& p : c.parts)
    {
        if (p.ratio <= 0.0)
            continue;
        md << "| " << cell(getBenchmarkReportSectionName(p.category)) << " | "
           << pointsText(1.0) << " | " << pointsText(p.ratio) << " | " << changeText(p.ratio)
           << " |\n";
    }
    md << "\n";

    // what could make the slower run slower, by part
    for (const auto& p : c.parts)
    {
        if (p.differences.empty() && p.hints.empty() && p.advice.empty())
            continue;
        md << "**" << getBenchmarkReportSectionName(p.category) << ": "
           << getBenchmarkReasonsTitle(c, p) << "**\n\n";
        for (const auto& d : p.differences)
            md << "- " << d.label << ": " << d.olderText << arrow() << d.newerText << "\n";
        for (const auto& h : p.hints)
            md << "- " << h.title << "\n";
        for (const auto& a : p.advice)
            md << "- " << a.first << ": " << a.second << "\n";
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

    // every value with the work behind it, by part
    md << "| " << _("Test") << " | " << older << " | " << _("Work") << " | " << newer
       << " | " << _("Work") << " | " << _("Change") << " |\n|---|---:|---|---:|---|---|\n";
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        const BenchmarkComparedPart* part = c.findPart(section);
        if (!part)
            continue;
        md << "| **" << cell(getBenchmarkReportSectionName(section)) << "** | "
           << (part->ratio > 0.0 ? pointsText(1.0) : wxString()) << " | | "
           << (part->ratio > 0.0 ? pointsText(part->ratio) : wxString()) << " | | "
           << (part->ratio > 0.0 ? changeText(part->ratio) : wxString()) << " |\n";
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const auto& v : *list)
            {
                if (v.category != section)
                    continue;
                md << "| " << cell(v.title) << " | " << cell(v.olderText) << " | "
                   << cell(v.olderWork) << " | " << cell(v.newerText) << " | "
                   << cell(v.newerWork) << " | " << changeText(v.ratio) << " |\n";
            }
        }
    }
    return md;
}

wxString BenchmarkComparison::toMarkdown() const
{
    return formatBenchmarkComparisonMarkdown(*this, true);
}

} // namespace fr
