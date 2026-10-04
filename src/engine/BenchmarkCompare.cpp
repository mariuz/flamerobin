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

// Comparing two runs. Every report contains the results of its run as a
// hidden comment (the snapshot); a saved report can therefore be compared
// with a new run, or two saved reports with each other, without running a
// test. Facts of the environment (hardware, versions, settings) are
// compared as well, so that the comparison shows what was changed.

namespace fr
{

namespace
{

const char* const snapshotMarker = "FlameRobin benchmark results";

const std::pair<BenchmarkCategory, const char*> categoryIds[] = {
    { BenchmarkCategory::ServerCpu, "cpu" },
    { BenchmarkCategory::ServerStorage, "storage" },
    { BenchmarkCategory::ServerCache, "memory" },
    { BenchmarkCategory::Network, "network" },
    { BenchmarkCategory::MultiUser, "users" },
    { BenchmarkCategory::LocalMachine, "computer" }
};

// the sections of the statistics, also those without a score
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

wxString severityText(BenchmarkSeverity severity)
{
    return getBenchmarkSeverityName(severity);
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
    s.score = report.score.total;
    for (const auto& c : report.score.categories)
        s.categories[c.category] = c.points;
    s.values = getBenchmarkMeasuredValues(report.metrics);
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
    for (const auto& f : report.findings)
    {
        if ((f.severity == BenchmarkSeverity::Problem || f.severity == BenchmarkSeverity::Warning)
            && std::none_of(s.findings.begin(), s.findings.end(),
                [&f](const auto& known) { return known.title == f.title; }))
        {
            s.findings.push_back({ f.severity, getBenchmarkReportSection(f.category), f.title });
        }
    }
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
    }
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
    text += "score = " + wxString::FromCDouble(s.score, 1) + "\n";
    for (const auto& id : categoryIds)
    {
        auto it = s.categories.find(id.first);
        if (it != s.categories.end())
        {
            text += wxString("category.") + id.second + " = "
                + wxString::FromCDouble(it->second, 1) + "\n";
        }
    }
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
        for (const auto& id : sectionIds)
        {
            if (id.first == st.section)
            {
                text += wxString("stat = ") + id.second + " | " + st.key + " | "
                    + clean(st.test) + " | " + clean(st.value) + " | " + clean(st.amount)
                    + " | " + wxString::FromCDouble(st.ms, 3) + "\n";
            }
        }
    }
    for (const auto& f : s.facts)
        text += "fact." + f.first + " = " + clean(f.second) + "\n";
    for (const auto& f : s.findings)
    {
        text += wxString("finding = ") + (f.severity == BenchmarkSeverity::Problem
            ? "problem" : "warning") + " | " + sectionId(f.section) + " | "
            + clean(f.title) + "\n";
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
        double number = 0.0;
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
        else if (key == "score" && value.ToCDouble(&number))
            s.score = number;
        else if (key.StartsWith("category."))
        {
            for (const auto& id : categoryIds)
            {
                if (key.Mid(9) == id.second && value.ToCDouble(&number))
                    s.categories[id.first] = number;
            }
        }
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
        else if (key == "finding")
        {
            const wxArrayString parts = split(value);
            BenchmarkSnapshot::Finding f;
            if (parts.size() == 3 && parseSection(parts[1], &f.section))
            {
                f.severity = parts[0] == "problem" ? BenchmarkSeverity::Problem
                    : BenchmarkSeverity::Warning;
                f.title = parts[2];
                s.findings.push_back(f);
            }
        }
    }
    if (!hasFormat)
        return std::nullopt;
    return s;
}

BenchmarkComparison compareBenchmarkSnapshots(const BenchmarkSnapshot& newer,
    const BenchmarkSnapshot& older)
{
    BenchmarkComparison c;
    c.newer = newer;
    c.older = older;
    c.sameKind = newer.kind == older.kind && newer.intensity == older.intensity;
    if (newer.score > 0.0 && older.score > 0.0)
        c.scoreRatio = newer.score / older.score;

    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        auto n = newer.categories.find(section);
        auto o = older.categories.find(section);
        if (n == newer.categories.end() && o == older.categories.end())
            continue;
        BenchmarkComparedValue v;
        v.category = section;
        v.title = getBenchmarkReportSectionName(section);
        v.newerText = n != newer.categories.end() ? formatBenchmarkPoints(n->second) : dash();
        v.olderText = o != older.categories.end() ? formatBenchmarkPoints(o->second) : dash();
        if (n != newer.categories.end() && o != older.categories.end() && o->second > 0.0)
            v.ratio = n->second / o->second;
        c.categories.push_back(v);
    }
    c.values = compareBenchmarkValues(newer.values, older.values);
    auto workOf = [](const BenchmarkSnapshot& s, const wxString& key)
    {
        auto w = s.work.find(key);
        return w == s.work.end() ? wxString()
            : formatBenchmarkWork(w->second.first, w->second.second);
    };
    for (auto& v : c.values)
    {
        const wxString key = getBenchmarkValueKey(v.title);
        v.newerWork = workOf(newer, key);
        v.olderWork = workOf(older, key);
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
        // the same work (or a single operation) in less time is faster
        if (n && o && n->ms > 0.0 && o->ms > 0.0 && n->amount == o->amount)
        {
            v.ratio = o->ms / n->ms;
        }
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
        c.differences.push_back({ getBenchmarkFactLabel(key), n ? *n : dash(),
            o ? *o : dash() });
    }

    auto contains = [](const BenchmarkSnapshot& s, const wxString& title)
    {
        return std::any_of(s.findings.begin(), s.findings.end(),
            [&title](const auto& f) { return f.title == title; });
    };
    for (const auto& f : older.findings)
    {
        if (!contains(newer, f.title))
            c.solved.push_back(f);
    }
    for (const auto& f : newer.findings)
    {
        if (!contains(older, f.title))
            c.added.push_back(f);
    }
    return c;
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
    double ratio = c.scoreRatio;
    if (ratio <= 0.0)
    {
        // without a score: the mean of the values both runs have
        double logSum = 0.0;
        int count = 0;
        for (const auto& v : c.values)
        {
            if (v.ratio > 0.0)
            {
                logSum += std::log(v.ratio);
                ++count;
            }
        }
        ratio = count ? std::exp(logSum / count) : 0.0;
    }
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

wxString formatBenchmarkComparisonMarkdown(const BenchmarkComparison& c, bool standalone)
{
    wxString md;
    if (standalone)
    {
        md << "## " << _("Comparison of two benchmark runs") << "\n\n";
        md << "| | " << olderLabel() << " | " << newerLabel(c) << " |\n|---|---|---|\n";
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
           << cell(runText(c.newer)) << " |\n";
        md << "| **" << _("Score") << "** | **"
           << (c.older.score > 0.0 ? formatBenchmarkPoints(c.older.score) : dash()) << "** | **"
           << (c.newer.score > 0.0 ? formatBenchmarkPoints(c.newer.score) : dash()) << "** |\n\n";
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

    const auto largest = c.getLargestChanges(5);
    if (!largest.empty())
    {
        md << "**" << _("Largest changes") << "**\n\n";
        for (const BenchmarkComparedValue* v : largest)
        {
            const wxString change = formatBenchmarkChange(v->ratio);
            md << "- " << v->title << ": " << v->olderText << arrow() << v->newerText << ", "
               << (getBenchmarkChangeLevel(v->ratio) == 2 ? "**" + change + "**" : change) << "\n";
        }
        md << "\n";
    }
    if (!c.differences.empty())
    {
        md << "**" << _("What is different") << "**\n\n| | " << olderLabel() << " | "
           << newerLabel(c) << " |\n|---|---|---|\n";
        for (const auto& d : c.differences)
        {
            md << "| " << cell(d.label) << " | " << cell(d.olderText) << " | "
               << cell(d.newerText) << " |\n";
        }
        md << "\n";
    }
    auto findingList = [&md](const wxString& title,
        const std::vector<BenchmarkSnapshot::Finding>& list)
    {
        if (list.empty())
            return;
        md << "**" << title << "**\n\n";
        for (const auto& f : list)
        {
            md << "- " << severityText(f.severity) << " ("
               << getBenchmarkReportSectionName(f.section) << "): " << f.title << "\n";
        }
        md << "\n";
    };
    findingList(_("Problems that are gone"), c.solved);
    findingList(_("New problems"), c.added);

    // every value with the work behind it, by part
    md << "| " << _("Test") << " | " << olderLabel() << " | " << _("Work") << " | "
       << newerLabel(c) << " | " << _("Work") << " | " << _("Change")
       << " |\n|---|---:|---|---:|---|---|\n";
    auto row = [&md](const BenchmarkComparedValue& v)
    {
        const wxString change = formatBenchmarkChange(v.ratio);
        md << "| " << cell(v.title) << " | " << cell(v.olderText) << " | " << cell(v.olderWork)
           << " | " << cell(v.newerText) << " | " << cell(v.newerWork) << " | "
           << (getBenchmarkChangeLevel(v.ratio) == 2 ? "**" + change + "**" : change) << " |\n";
    };
    if (c.older.score > 0.0 || c.newer.score > 0.0)
    {
        const wxString change = formatBenchmarkChange(c.scoreRatio);
        md << "| **" << _("Score") << "** | **"
           << (c.older.score > 0.0 ? formatBenchmarkPoints(c.older.score) : dash()) << "** | | **"
           << (c.newer.score > 0.0 ? formatBenchmarkPoints(c.newer.score) : dash()) << "** | | "
           << (getBenchmarkChangeLevel(c.scoreRatio) == 2 ? "**" + change + "**" : change)
           << " |\n";
    }
    for (BenchmarkCategory section : getBenchmarkReportSections())
    {
        const BenchmarkComparedValue* points = nullptr;
        for (const auto& category : c.categories)
        {
            if (category.category == section)
                points = &category;
        }
        bool any = points != nullptr;
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const auto& v : *list)
                any = any || v.category == section;
        }
        if (!any)
            continue;
        const wxString change = points ? formatBenchmarkChange(points->ratio) : wxString();
        md << "| **" << cell(getBenchmarkReportSectionName(section)) << "** | "
           << (points ? points->olderText : wxString()) << " | | "
           << (points ? points->newerText : wxString()) << " | | "
           << (points && getBenchmarkChangeLevel(points->ratio) == 2 ? "**" + change + "**"
               : change) << " |\n";
        for (const auto* list : { &c.values, &c.stats })
        {
            for (const auto& v : *list)
            {
                if (v.category == section)
                    row(v);
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
