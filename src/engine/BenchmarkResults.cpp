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

#include <wx/filename.h>

#include <algorithm>

#include "engine/BenchmarkResults.h"

// The results of the benchmark dialog: every test of a session, the saved
// reports that were opened and the comparisons made of them. They stay
// until they are closed or FlameRobin ends; nothing is written to disk,
// because the reports contain the names of computers and servers.

namespace fr
{

namespace
{

// "hh:mm" of "yyyy-mm-dd hh:mm:ss"; empty without a time
wxString getTime(const wxString& date)
{
    return date.length() >= 16 ? date.Mid(11, 5) : wxString();
}

wxString shorten(const wxString& text, size_t length)
{
    return text.length() > length ? text.Left(length - 3) + "..." : text;
}

wxString getFileName(const wxString& path)
{
    const wxString name = wxFileName(path).GetFullName();
    return name.empty() ? wxString(_("file")) : name;
}

wxString getIntensityName(const wxString& intensityId)
{
    if (intensityId == "quick")
        return getBenchmarkIntensityName(BenchmarkIntensity::Quick);
    if (intensityId == "standard")
        return getBenchmarkIntensityName(BenchmarkIntensity::Standard);
    if (intensityId == "deep")
        return getBenchmarkIntensityName(BenchmarkIntensity::Deep);
    return intensityId;
}

// how a run was made, in a few words
wxString getKindWord(const wxString& kindId)
{
    if (kindId == "embedded")
        return _("this computer");
    if (kindId == "local")
        return _("on the server");
    if (kindId == "client")
        return _("client");
    return kindId;
}

// the same in a sentence: "Result 4 was made over the network (client)"
wxString getKindPhrase(const wxString& kindId)
{
    if (kindId == "embedded")
        return _("with the embedded engine");
    if (kindId == "local")
        return _("on the server");
    if (kindId == "client")
        return _("over the network (client)");
    return kindId;
}

// date, intensity, how and what was tested, as far as known
wxString describeRun(const BenchmarkSnapshot& s)
{
    wxString text;
    for (const wxString& part : { s.date.Left(16), getIntensityName(s.intensity),
        getKindWord(s.kind), s.title })
    {
        if (part.empty())
            continue;
        if (!text.empty())
            text += ", ";
        text += part;
    }
    return text;
}

// the short name in the label of a comparison: the number of a test, the
// file name of a saved report
wxString getShortName(const BenchmarkResultEntry& entry)
{
    if (entry.kind == BenchmarkResultKind::Run)
        return wxString::Format("%d", entry.id);
    return getFileName(entry.path);
}

// the short name of the label in a sentence: "result 3" or the file name
wxString getNameInSentence(const wxString& shortName)
{
    long number = 0;
    if (shortName.ToLong(&number))
        return wxString::Format(_("result %ld"), number);
    return shortName;
}

// at the start of a sentence: "Result 3"
wxString getSentenceStart(const BenchmarkResultEntry& entry)
{
    if (entry.kind == BenchmarkResultKind::Run)
        return wxString::Format(_("Result %d"), entry.id);
    return getBenchmarkResultName(entry);
}

// a part of a file name: letters, digits, '-' and '_'
wxString getFileNamePart(const wxString& name)
{
    wxString part = name;
    for (const char* extension : { ".html", ".htm", ".md" })
    {
        if (part.Lower().EndsWith(extension))
            part = part.Left(part.length() - wxStrlen(extension));
    }
    for (size_t i = 0; i < part.length(); ++i)
    {
        const wxUniChar c = part[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
            || c == '-' || c == '_'))
        {
            part[i] = '-';
        }
    }
    return part;
}

} // namespace

bool hasBenchmarkData(const BenchmarkReport& report)
{
    // a test that failed while it created its database has its settings,
    // but no value
    const BenchmarkSnapshot snapshot = makeBenchmarkSnapshot(report);
    return std::any_of(snapshot.measurements.begin(), snapshot.measurements.end(),
        [](const BenchmarkMeasurement& m) { return m.value > 0.0; });
}

int BenchmarkResultStore::add(BenchmarkResultEntry entry)
{
    if (isFull())
        return 0;
    entry.id = ++lastIdM;
    entriesM.push_back(std::move(entry));
    return lastIdM;
}

int BenchmarkResultStore::addRun(const BenchmarkReport& report,
    const std::optional<BenchmarkSnapshot>& baseline)
{
    if (!hasBenchmarkData(report) || isFull())
        return 0;
    BenchmarkResultEntry entry;
    entry.kind = BenchmarkResultKind::Run;
    // the earlier report is the baseline of the entry, set where it is shown
    BenchmarkReport measured = report;
    measured.earlier.reset();
    entry.snapshot = makeBenchmarkSnapshot(measured);
    entry.report = std::make_shared<const BenchmarkReport>(std::move(measured));
    entry.baseline = baseline;
    entry.incomplete = report.cancelled || !report.errorMessage.empty();
    return add(std::move(entry));
}

int BenchmarkResultStore::addSavedReport(const BenchmarkSnapshot& snapshot,
    const wxString& html, const wxString& path)
{
    BenchmarkResultEntry entry;
    entry.kind = BenchmarkResultKind::SavedReport;
    entry.snapshot = snapshot;
    entry.html = html;
    entry.path = path;
    entry.saved = true;
    return add(std::move(entry));
}

int BenchmarkResultStore::addComparison(const BenchmarkResultEntry& older,
    const BenchmarkResultEntry& newer)
{
    // the same comparison of two tabs is selected, not made again; the
    // reversed order is another comparison (the reference changes)
    if (older.id != 0 && newer.id != 0)
    {
        for (const BenchmarkResultEntry& e : entriesM)
        {
            if (e.kind == BenchmarkResultKind::Comparison && e.olderId == older.id
                && e.newerId == newer.id)
            {
                return e.id;
            }
        }
    }
    if (isFull())
        return 0;
    BenchmarkResultEntry entry;
    entry.kind = BenchmarkResultKind::Comparison;
    entry.comparison = std::make_shared<const BenchmarkComparison>(
        compareBenchmarkSnapshots(newer.snapshot, older.snapshot));
    entry.olderId = older.id;
    entry.newerId = newer.id;
    entry.olderName = getShortName(older);
    entry.newerName = getShortName(newer);
    return add(std::move(entry));
}

int BenchmarkResultStore::close(int id)
{
    auto it = std::find_if(entriesM.begin(), entriesM.end(),
        [id](const BenchmarkResultEntry& e) { return e.id == id; });
    if (it == entriesM.end())
        return selectedM;
    const size_t index = size_t(it - entriesM.begin());
    entriesM.erase(it);
    if (selectedM == id)
    {
        // the right neighbour, else the left one
        if (index < entriesM.size())
            selectedM = entriesM[index].id;
        else
            selectedM = index > 0 ? entriesM[index - 1].id : 0;
    }
    return selectedM;
}

int BenchmarkResultStore::closeOthers(int id)
{
    entriesM.erase(std::remove_if(entriesM.begin(), entriesM.end(),
        [id](const BenchmarkResultEntry& e) { return e.id != id; }), entriesM.end());
    selectedM = entriesM.empty() ? 0 : id;
    return selectedM;
}

int BenchmarkResultStore::closeAll()
{
    entriesM.clear();
    selectedM = 0;
    return 0;
}

int BenchmarkResultStore::getSelected() const
{
    return selectedM;
}

bool BenchmarkResultStore::select(int id)
{
    if (id != 0 && !find(id))
        return false;
    selectedM = id;
    return true;
}

void BenchmarkResultStore::markSaved(int id, const wxString& path)
{
    for (BenchmarkResultEntry& e : entriesM)
    {
        if (e.id == id)
        {
            e.saved = true;
            e.path = path;
        }
    }
}

const std::vector<BenchmarkResultEntry>& BenchmarkResultStore::getEntries() const
{
    return entriesM;
}

const BenchmarkResultEntry* BenchmarkResultStore::find(int id) const
{
    for (const BenchmarkResultEntry& e : entriesM)
    {
        if (e.id == id)
            return &e;
    }
    return nullptr;
}

std::vector<int> BenchmarkResultStore::getUnsavedIds() const
{
    std::vector<int> ids;
    for (const BenchmarkResultEntry& e : entriesM)
    {
        if (e.kind == BenchmarkResultKind::Run && !e.saved)
            ids.push_back(e.id);
    }
    return ids;
}

std::vector<int> BenchmarkResultStore::getComparableIds() const
{
    std::vector<int> ids;
    for (const BenchmarkResultEntry& e : entriesM)
    {
        if (e.kind != BenchmarkResultKind::Comparison)
            ids.push_back(e.id);
    }
    return ids;
}

bool BenchmarkResultStore::isFull() const
{
    return entriesM.size() >= maximumEntries;
}

std::vector<int> getBenchmarkCloseQuestion(const BenchmarkResultStore& store,
    BenchmarkCloseScope scope, int id)
{
    std::vector<int> ids;
    for (int unsaved : store.getUnsavedIds())
    {
        if (scope == BenchmarkCloseScope::All
            || (scope == BenchmarkCloseScope::One) == (unsaved == id))
        {
            ids.push_back(unsaved);
        }
    }
    return ids;
}

wxString formatBenchmarkResultIds(const std::vector<int>& ids)
{
    wxString text;
    for (int id : ids)
        text += (text.empty() ? "" : ", ") + wxString::Format("%d", id);
    return text;
}

wxString getBenchmarkResultLabel(const BenchmarkResultEntry& entry)
{
    switch (entry.kind)
    {
        case BenchmarkResultKind::Run:
        {
            const wxString time = getTime(entry.snapshot.date);
            const wxString label = time.empty() ? wxString::Format("%d", entry.id)
                : wxString::Format("%d  %s", entry.id, time);
            return entry.saved ? label : "* " + label;
        }
        case BenchmarkResultKind::SavedReport:
            return shorten(getFileName(entry.path), 22);
        case BenchmarkResultKind::Comparison:
        default:
            return wxString::Format(_("%s vs %s"), shorten(entry.olderName, 16),
                shorten(entry.newerName, 16));
    }
}

wxString describeBenchmarkResult(const BenchmarkResultEntry& entry)
{
    switch (entry.kind)
    {
        case BenchmarkResultKind::Run:
        {
            wxString state = entry.saved ? _("saved") : _("not saved");
            if (entry.incomplete)
                state = _("incomplete") + ", " + state;
            return wxString::Format(_("%d  Run, %s (%s)"), entry.id,
                describeRun(entry.snapshot), state);
        }
        case BenchmarkResultKind::SavedReport:
            return wxString::Format(_("File %s, %s (opened)"), getFileName(entry.path),
                describeRun(entry.snapshot));
        case BenchmarkResultKind::Comparison:
        default:
            return wxString::Format(_("Comparison of %s (earlier) and %s (later)"),
                getNameInSentence(entry.olderName), getNameInSentence(entry.newerName));
    }
}

wxString getBenchmarkResultHeader(const BenchmarkResultEntry& entry)
{
    switch (entry.kind)
    {
        case BenchmarkResultKind::Run:
            if (entry.incomplete)
            {
                return wxString::Format(entry.saved ? _("Result %d - incomplete, saved")
                    : _("Result %d - incomplete, not saved"), entry.id);
            }
            return wxString::Format(entry.saved ? _("Result %d - saved")
                : _("Result %d - not saved"), entry.id);
        case BenchmarkResultKind::SavedReport:
            return wxString::Format(_("%s - shown as saved"), getFileName(entry.path));
        case BenchmarkResultKind::Comparison:
        default:
            return wxString::Format(_("Comparison %s - %s is the reference"),
                getBenchmarkResultLabel(entry), getNameInSentence(entry.olderName));
    }
}

wxString getBenchmarkResultName(const BenchmarkResultEntry& entry)
{
    switch (entry.kind)
    {
        case BenchmarkResultKind::Run:
            return wxString::Format(_("result %d"), entry.id);
        case BenchmarkResultKind::SavedReport:
            return getFileName(entry.path);
        case BenchmarkResultKind::Comparison:
        default:
            return wxString::Format(_("comparison %s"), getBenchmarkResultLabel(entry));
    }
}

wxString getBenchmarkResultFileName(const BenchmarkResultEntry& entry)
{
    switch (entry.kind)
    {
        case BenchmarkResultKind::Run:
        {
            // the digits of "yyyy-mm-dd hh:mm"
            wxString stamp;
            for (const wxUniChar c : entry.snapshot.date.Left(16))
            {
                if (c >= '0' && c <= '9')
                    stamp += c;
                else if (c == ' ')
                    stamp += '-';
            }
            if (stamp.length() != 13)
                return wxString::Format("flamerobin-benchmark-%d.html", entry.id);
            return wxString::Format("flamerobin-benchmark-%d-%s.html", entry.id, stamp);
        }
        case BenchmarkResultKind::SavedReport:
            return getFileName(entry.path);
        case BenchmarkResultKind::Comparison:
        default:
            return "flamerobin-benchmark-comparison-" + getFileNamePart(entry.olderName)
                + "-vs-" + getFileNamePart(entry.newerName) + ".html";
    }
}

BenchmarkPair orderBenchmarkPair(const BenchmarkResultEntry& a, const BenchmarkResultEntry& b)
{
    // "yyyy-mm-dd hh:mm:ss" sorts as text
    const wxString& first = a.snapshot.date;
    const wxString& second = b.snapshot.date;
    if (!first.empty() && !second.empty() && second < first)
        return { &b, &a };
    return { &a, &b };
}

std::vector<BenchmarkPairCheck> checkBenchmarkPair(const BenchmarkResultEntry& older,
    const BenchmarkResultEntry& newer)
{
    bool same = &older == &newer;
    if (older.id != 0 && newer.id != 0)
        same = same || older.id == newer.id;
    else if (older.kind == BenchmarkResultKind::SavedReport
        && newer.kind == BenchmarkResultKind::SavedReport)
    {
        // a file that has a tab and is chosen once more
        same = same || (!older.path.empty() && older.path == newer.path);
    }
    if (same)
        return { BenchmarkPairCheck::SameResult };
    std::vector<BenchmarkPairCheck> checks;
    if (older.snapshot.kind != newer.snapshot.kind)
        checks.push_back(BenchmarkPairCheck::DifferentKind);
    if (older.snapshot.intensity != newer.snapshot.intensity)
        checks.push_back(BenchmarkPairCheck::DifferentIntensity);
    if (older.incomplete || newer.incomplete)
        checks.push_back(BenchmarkPairCheck::Incomplete);
    return checks;
}

wxString describeBenchmarkPairCheck(BenchmarkPairCheck check,
    const BenchmarkResultEntry& older, const BenchmarkResultEntry& newer)
{
    switch (check)
    {
        case BenchmarkPairCheck::SameResult:
            return _("Please choose two different results.");
        case BenchmarkPairCheck::DifferentKind:
            return wxString::Format(_("%s was made %s and %s %s: they measure different "
                "things and are only roughly comparable. The comparison says so as well."),
                getSentenceStart(older), getKindPhrase(older.snapshot.kind),
                getBenchmarkResultName(newer), getKindPhrase(newer.snapshot.kind));
        case BenchmarkPairCheck::DifferentIntensity:
            return wxString::Format(_("%s is a %s run and %s a %s run: the comparison is "
                "only roughly valid."), getSentenceStart(older),
                getIntensityName(older.snapshot.intensity), getBenchmarkResultName(newer),
                getIntensityName(newer.snapshot.intensity));
        case BenchmarkPairCheck::Incomplete:
        default:
            return _("A cancelled or failed test is incomplete: only the values both "
                "results have are compared.");
    }
}

wxString describeBenchmarkPairOrder(const BenchmarkResultEntry& older,
    const BenchmarkResultEntry& newer)
{
    // the time of a test of this session, the date of a file
    auto describe = [](const BenchmarkResultEntry& e)
    {
        const wxString when = e.kind == BenchmarkResultKind::Run
            ? getTime(e.snapshot.date) : e.snapshot.date.Left(16);
        return when.empty() ? getBenchmarkResultName(e)
            : getBenchmarkResultName(e) + " (" + when + ")";
    };
    return wxString::Format(_("Earlier: %s   Later: %s"), describe(older), describe(newer));
}

} // namespace fr
