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
#include <wx/wxcrt.h>

#include <algorithm>
#include <iterator>

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

int BenchmarkResultStore::addSavedComparison(const wxString& html, const wxString& path)
{
    BenchmarkResultEntry entry;
    entry.kind = BenchmarkResultKind::SavedReport;
    entry.comparisonPage = true;
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

void BenchmarkResultStore::discardUnsaved()
{
    for (int id : getUnsavedIds())
        close(id);
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
        if (e.kind != BenchmarkResultKind::Comparison && !e.comparisonPage)
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
            if (entry.comparisonPage)
                return wxString::Format(_("File %s, comparison (opened)"), getFileName(entry.path));
            return wxString::Format(_("File %s, %s (opened)"), getFileName(entry.path),
                describeRun(entry.snapshot));
        case BenchmarkResultKind::Comparison:
        default:
            return wxString::Format(_("Comparison of %s (earlier) and %s (later)"),
                getNameInSentence(entry.olderName), getNameInSentence(entry.newerName));
    }
}

wxString getBenchmarkResultSummary(const BenchmarkResultEntry& entry)
{
    wxString text;
    for (const wxString& part : { getTime(entry.snapshot.date),
        getIntensityName(entry.snapshot.intensity), getKindWord(entry.snapshot.kind) })
    {
        if (!part.empty())
            text += (text.empty() ? "" : ", ") + part;
    }
    return text;
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

namespace
{

// the elements of a FlameRobin report; anything else (a script, a frame,
// an object, a form, a link to a style sheet, an image) is not shown
const char* const plainElements[] = { "html", "head", "meta", "title", "style", "body",
    "main", "header", "footer", "nav", "section", "article", "aside", "div", "span", "p",
    "b", "i", "u", "s", "small", "strong", "em", "code", "pre", "kbd", "mark", "abbr",
    "sub", "sup", "br", "hr", "wbr", "h1", "h2", "h3", "h4", "h5", "h6", "ul", "ol",
    "li", "dl", "dt", "dd", "table", "caption", "colgroup", "col", "thead", "tbody",
    "tfoot", "tr", "th", "td", "details", "summary", "label", "input", "figure",
    "figcaption", "svg", "g", "defs", "symbol", "use", "path", "circle", "ellipse",
    "rect", "line", "polyline", "polygon", "text", "tspan", "desc", "lineargradient",
    "radialgradient", "stop", "clippath" };

// attributes that load or run something; href only as a link within the page
const char* const loadingAttributes[] = { "src", "srcset", "action", "formaction",
    "background", "poster", "data", "codebase", "dynsrc", "lowsrc", "longdesc",
    "manifest", "ping", "cite", "usemap", "profile", "archive", "classid" };

bool isNameChar(wxUniChar c)
{
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == ':'
        || c == '_';
}

// a style sheet or a style attribute without anything that is loaded or
// run: no address, no expression or behavior of Internet Explorer, no
// import; escapes could hide a word, the reports have none
bool isPlainCss(const wxString& lowerCss)
{
    wxString css = lowerCss;
    // a comment could split a word
    for (size_t start = css.find("/*"); start != wxString::npos; start = css.find("/*"))
    {
        const size_t end = css.find("*/", start + 2);
        if (end == wxString::npos)
            return false;
        css.erase(start, end + 2 - start);
    }
    if (css.find('\\') != wxString::npos)
        return false;
    for (const char* word : { "url(", "expression", "behavior", "-moz-binding",
        "@import", "javascript:", "vbscript:" })
    {
        if (css.Contains(word))
            return false;
    }
    return true;
}

} // namespace

bool isBenchmarkReportHtmlPlain(const wxString& html)
{
    const wxString text = html.Lower();
    const size_t length = text.length();
    size_t pos = 0;
    // within <svg> <title> and <style> hold markup, not text
    int svgDepth = 0;
    while ((pos = text.find('<', pos)) != wxString::npos)
    {
        if (text.compare(pos, 4, "<!--") == 0)
        {
            const size_t end = text.find("-->", pos + 4);
            if (end == wxString::npos)
                return false;
            // the conditional comments of Internet Explorer contain markup
            // that it shows and runs
            const wxString comment = text.substr(pos + 4, end - pos - 4);
            if (comment.StartsWith("[if") || comment.Contains("<![endif]"))
                return false;
            // a browser also ends a comment with "<!-->", "<!--->" or "--!>"
            if (comment.StartsWith(">") || comment.StartsWith("->")
                || comment.Contains("--!>"))
            {
                return false;
            }
            pos = end + 3;
            continue;
        }
        if (text.compare(pos, 9, "<!doctype") == 0)
        {
            pos = text.find('>', pos);
            if (pos == wxString::npos)
                return false;
            continue;
        }
        // CDATA, processing instructions and the like
        if (text.compare(pos, 2, "<!") == 0 || text.compare(pos, 2, "<?") == 0)
            return false;
        size_t i = pos + 1;
        const bool closing = i < length && text[i] == '/';
        if (closing)
            ++i;
        const size_t nameStart = i;
        while (i < length && isNameChar(text[i]))
            ++i;
        const wxString name = text.substr(nameStart, i - nameStart);
        if (name.empty())
        {
            // a '<' in the text
            ++pos;
            continue;
        }
        if (std::none_of(std::begin(plainElements), std::end(plainElements),
            [&name](const char* e) { return name == e; }))
        {
            return false;
        }
        // the attributes up to the end of the tag
        bool httpEquiv = false;
        bool compatible = false;
        bool selfClosing = false;
        wxString inputType;
        std::vector<wxString> attributes;
        while (true)
        {
            selfClosing = false;
            while (i < length && (wxIsspace(text[i]) || text[i] == '/'))
                selfClosing = text[i++] == '/';
            if (i >= length)
                return false;
            if (text[i] == '>')
                break;
            selfClosing = false;
            const size_t attributeStart = i;
            while (i < length && !wxIsspace(text[i]) && text[i] != '=' && text[i] != '>'
                && text[i] != '/')
            {
                ++i;
            }
            const wxString attribute = text.substr(attributeStart, i - attributeStart);
            // a browser takes the first of two, the checks below the last
            if (std::find(attributes.begin(), attributes.end(), attribute)
                != attributes.end())
            {
                return false;
            }
            attributes.push_back(attribute);
            while (i < length && wxIsspace(text[i]))
                ++i;
            wxString value;
            if (i < length && text[i] == '=')
            {
                ++i;
                while (i < length && wxIsspace(text[i]))
                    ++i;
                if (i < length && (text[i] == '"' || text[i] == '\''))
                {
                    const wxUniChar quote = text[i];
                    const size_t end = text.find(quote, i + 1);
                    if (end == wxString::npos)
                        return false;
                    value = text.substr(i + 1, end - i - 1);
                    i = end + 1;
                }
                else
                {
                    const size_t valueStart = i;
                    while (i < length && !wxIsspace(text[i]) && text[i] != '>')
                        ++i;
                    value = text.substr(valueStart, i - valueStart);
                }
            }
            if (attribute.StartsWith("on") || std::any_of(std::begin(loadingAttributes),
                std::end(loadingAttributes), [&attribute](const char* a)
                    { return attribute == a; }))
            {
                return false;
            }
            if ((attribute == "href" || attribute == "xlink:href") && !value.StartsWith("#"))
                return false;
            // a browser decodes character references in a value, which
            // could hide a word
            if (attribute == "style" && (value.Contains("&") || !isPlainCss(value)))
                return false;
            if (attribute == "http-equiv")
            {
                httpEquiv = true;
                compatible = value == "x-ua-compatible";
            }
            if (attribute == "type")
                inputType = value;
        }
        // only the mode of Internet Explorer, never a refresh to elsewhere
        if (name == "meta" && httpEquiv && !compatible)
            return false;
        // the radio buttons and check boxes of the views, no image button
        if (name == "input" && inputType != "radio" && inputType != "checkbox")
            return false;
        pos = i + 1;
        if (name == "svg")
        {
            if (closing)
                svgDepth = std::max(svgDepth - 1, 0);
            else if (!selfClosing)
                ++svgDepth;
        }
        if (!closing && (name == "style" || name == "title"))
        {
            // the reports have none within <svg>, where they hold markup
            if (svgDepth > 0)
                return false;
            const size_t end = text.find("</" + name, pos);
            if (end == wxString::npos)
                return false;
            if (name == "style" && !isPlainCss(text.substr(pos, end - pos)))
                return false;
            pos = end;
        }
    }
    return true;
}

wxString applyBenchmarkReportTheme(const wxString& html, const wxString& theme)
{
    const wxString lower = html.Lower();
    const size_t start = lower.find("<html");
    if (start == wxString::npos)
        return html;
    const size_t end = lower.find('>', start);
    if (end == wxString::npos)
        return html;
    // the tag without its themes
    wxString tag = html.substr(start + 5, end - start - 5);
    for (size_t at = tag.Lower().find("data-theme"); at != wxString::npos;
        at = tag.Lower().find("data-theme"))
    {
        size_t stop = at + 10;
        while (stop < tag.length() && wxIsspace(tag[stop]))
            ++stop;
        if (stop < tag.length() && tag[stop] == '=')
        {
            ++stop;
            while (stop < tag.length() && wxIsspace(tag[stop]))
                ++stop;
            if (stop < tag.length() && (tag[stop] == '"' || tag[stop] == '\''))
            {
                const size_t close = tag.find(wxUniChar(tag[stop]), stop + 1);
                stop = close == wxString::npos ? tag.length() : close + 1;
            }
            else
            {
                while (stop < tag.length() && !wxIsspace(tag[stop]))
                    ++stop;
            }
        }
        // with the space in front of it
        size_t from = at;
        while (from > 0 && wxIsspace(tag[from - 1]))
            --from;
        tag.erase(from, stop - from);
    }
    if (theme == "dark" || theme == "light")
        tag = " data-theme=\"" + theme + "\"" + tag;
    return html.substr(0, start + 5) + tag + html.substr(end);
}

wxString addBenchmarkReportPolicy(const wxString& html)
{
    // 'unsafe-eval' only for the scripts of wxWebView::RunScript(); the
    // page itself cannot start a script without inline or loaded ones
    const wxString policy = "<meta http-equiv=\"Content-Security-Policy\" content=\""
        "default-src 'none'; script-src 'unsafe-eval'; style-src 'unsafe-inline'; "
        "img-src data:; font-src data:; base-uri 'none'; form-action 'none'\">";
    // before the doctype the page would be shown in quirks mode
    const wxString lower = html.Lower();
    size_t start = 0;
    while (start < lower.length() && wxIsspace(lower[start]))
        ++start;
    if (lower.compare(start, 9, "<!doctype") == 0)
    {
        const size_t end = lower.find('>', start);
        if (end != wxString::npos)
            return html.substr(0, end + 1) + policy + html.substr(end + 1);
    }
    return policy + html;
}

} // namespace fr
