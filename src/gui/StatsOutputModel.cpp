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

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/tokenzr.h>

#include "gui/StatsOutputModel.h"

namespace
{

size_t utf8Len(const wxString& s)
{
    return s.ToUTF8().length();
}

class LineBuilder
{
private:
    StatsFormattedLine lineM;
    size_t bytePosM = 0;

public:
    LineBuilder(int foldDepth, bool isFoldHeader = false,
        bool defaultExpanded = true)
    {
        lineM.foldDepth = foldDepth;
        lineM.isFoldHeader = isFoldHeader;
        lineM.defaultExpanded = defaultExpanded;
    }

    LineBuilder& append(const wxString& part, StatsStyle style)
    {
        if (part.empty())
            return *this;
        size_t len = utf8Len(part);
        if (len > 0)
        {
            if (!lineM.spans.empty() && lineM.spans.back().style == style
                && lineM.spans.back().byteOffset + lineM.spans.back().byteLength == bytePosM)
            {
                lineM.spans.back().byteLength += len;
            }
            else
            {
                lineM.spans.push_back({bytePosM, len, style});
            }
            bytePosM += len;
        }
        lineM.text += part;
        return *this;
    }

    StatsFormattedLine build() const
    {
        return lineM;
    }
};

void appendPlanLineWithHighlights(
    const wxString& rawLine,
    std::vector<StatsFormattedLine>& outLines)
{
    LineBuilder lb(2, false, true);
    lb.append("      ", StatsStyle::Default);

    wxString upper = rawLine.Upper();
    size_t pos = 0;
    const size_t n = rawLine.length();
    const wxString markers[] = {"NATURAL", "FULL SCAN", "TABLE SCAN"};

    while (pos < n)
    {
        size_t bestIdx = wxString::npos;
        size_t bestLen = 0;
        for (const wxString& m : markers)
        {
            size_t found = upper.find(m, pos);
            if (found != wxString::npos && (bestIdx == wxString::npos || found < bestIdx))
            {
                bestIdx = found;
                bestLen = m.length();
            }
        }
        if (bestIdx == wxString::npos)
        {
            lb.append(rawLine.Mid(pos), StatsStyle::Plan);
            break;
        }
        if (bestIdx > pos)
            lb.append(rawLine.Mid(pos, bestIdx - pos), StatsStyle::Plan);
        lb.append(rawLine.Mid(bestIdx, bestLen), StatsStyle::MetricWarning);
        pos = bestIdx + bestLen;
    }

    outLines.push_back(lb.build());
}

} // namespace

wxString SessionStatsHistoryIndex::normalizeSql(const wxString& sql)
{
    wxString s = sql;
    s.Replace("\r\n", "\n");
    s.Replace("\r", "\n");
    s.Trim(true);
    s.Trim(false);
    return s;
}

void SessionStatsHistoryIndex::clear()
{
    recordsM.clear();
}

void SessionStatsHistoryIndex::startRun(
    int runId, const wxString& timeStr, int startLine)
{
    for (auto& rec : recordsM)
    {
        if (rec.runId == runId)
        {
            if (startLine > rec.endLine)
                rec.endLine = startLine;
            return;
        }
    }
    SessionStatsRunRecord rec;
    rec.runId = runId;
    rec.timeStr = timeStr;
    rec.startLine = startLine;
    rec.endLine = startLine;
    recordsM.push_back(rec);
}

void SessionStatsHistoryIndex::updateRunEndLine(int runId, int endLine)
{
    for (auto it = recordsM.rbegin(); it != recordsM.rend(); ++it)
    {
        if (it->runId == runId)
        {
            if (endLine >= it->startLine)
                it->endLine = endLine;
            return;
        }
    }
}

void SessionStatsHistoryIndex::bindRunToHistory(
    int runId, size_t historyPos, const wxString& sql)
{
    for (auto it = recordsM.rbegin(); it != recordsM.rend(); ++it)
    {
        if (it->runId == runId)
        {
            it->hasHistoryPosition = true;
            it->historyPosition = historyPos;
            it->normalizedSql = normalizeSql(sql);
            return;
        }
    }
}

const SessionStatsRunRecord* SessionStatsHistoryIndex::findMatchingRun(
    size_t historyPos, const wxString& sql) const
{
    const wxString norm = normalizeSql(sql);
    if (norm.empty())
        return nullptr;

    // Primary match: latest session run bound to this exact StatementHistory
    // position whose SQL still matches (guards against index shifts after deleteItems).
    for (auto it = recordsM.rbegin(); it != recordsM.rend(); ++it)
    {
        if (it->hasHistoryPosition && it->historyPosition == historyPos
            && it->normalizedSql == norm)
        {
            return &(*it);
        }
    }

    // Fallback: latest session run with identical normalized SQL text.
    for (auto it = recordsM.rbegin(); it != recordsM.rend(); ++it)
    {
        if (!it->normalizedSql.empty() && it->normalizedSql == norm)
            return &(*it);
    }

    return nullptr;
}

namespace StatsFormatter
{

wxString extractSqlPreview(const wxString& sql, size_t maxChars)
{
    wxStringTokenizer tkz(sql, "\r\n");
    wxString firstLine;
    while (tkz.HasMoreTokens())
    {
        wxString candidate = tkz.GetNextToken();
        candidate.Trim(true);
        candidate.Trim(false);
        if (!candidate.empty())
        {
            firstLine = candidate;
            break;
        }
    }
    firstLine.Replace("\t", " ");
    if (firstLine.length() > maxChars && maxChars > 3)
        firstLine = firstLine.Left(maxChars - 3) + "...";
    return firstLine;
}

wxString formatRunLabel(int runId, int statementIdx, int totalStatementsInRun)
{
    if (totalStatementsInRun > 1)
        return wxString::Format("#%d.%d", runId, statementIdx);
    return wxString::Format("#%d", runId);
}

StatsFormattedLine formatRunHeaderLine(
    const wxString& runLabel,
    const wxString& timeStr,
    const wxString& sqlPreview,
    const wxString& statusText,
    bool isError)
{
    LineBuilder lb(0, true, true);
    lb.append(runLabel + "  " + timeStr + "  ", StatsStyle::RunHeader);
    lb.append(sqlPreview, StatsStyle::RunHeader);
    if (!statusText.empty())
    {
        lb.append("  [" + statusText + "]",
            isError ? StatsStyle::Error : StatsStyle::RunHeader);
    }
    return lb.build();
}

StatsFormattedLine formatStandaloneLine(
    const wxString& text, StatsStyle style, int foldDepth)
{
    LineBuilder lb(foldDepth, false, true);
    wxString clean = text;
    clean.Replace("\r", "");
    clean.Replace("\n", " ");
    lb.append(clean, style);
    return lb.build();
}

std::vector<StatsFormattedLine> formatPrepareGroup(
    const wxString& sql,
    bool startedTransaction,
    const wxString& elapsedStr,
    bool inProgress,
    bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;

    LineBuilder hdr(1, true, inProgress ? true : defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("Prepare"), StatsStyle::GroupHeader);
    if (inProgress)
        hdr.append("  (" + wxString(_("preparing...")) + ")", StatsStyle::MetricLabel);
    else if (!elapsedStr.empty())
        hdr.append("  (" + elapsedStr + ")", StatsStyle::MetricValue);
    lines.push_back(hdr.build());

    if (startedTransaction)
    {
        LineBuilder tx(2);
        tx.append("      " + wxString(_("Starting transaction...")), StatsStyle::MetricLabel);
        lines.push_back(tx.build());
    }

    wxString normalized = sql;
    normalized.Replace("\r\n", "\n");
    normalized.Replace("\r", "\n");
    normalized.Trim(true);
    wxArrayString sqlLines = wxSplit(normalized, '\n', '\0');
    for (const wxString& sl : sqlLines)
    {
        LineBuilder sqlLb(2);
        sqlLb.append("      ", StatsStyle::Default);
        sqlLb.append(sl, StatsStyle::Sql);
        lines.push_back(sqlLb.build());
    }

    if (!inProgress && !elapsedStr.empty())
    {
        LineBuilder doneLb(2);
        doneLb.append("      " + wxString::Format(_("Statement prepared (elapsed time: %s)."),
            elapsedStr.c_str()), StatsStyle::MetricLabel);
        lines.push_back(doneLb.build());
    }

    return lines;
}

std::vector<StatsFormattedLine> formatFieldsGroup(
    const std::vector<wxString>& fieldLines,
    bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    if (fieldLines.empty())
        return lines;

    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(wxString::Format(_("Fields (%zu)"), fieldLines.size()),
        StatsStyle::GroupHeader);
    lines.push_back(hdr.build());

    for (const wxString& f : fieldLines)
    {
        LineBuilder fl(2);
        fl.append("      ", StatsStyle::Default);
        int colonPos = f.Find(':');
        if (colonPos != wxNOT_FOUND)
        {
            fl.append(f.Left(colonPos + 1), StatsStyle::MetricLabel);
            wxString rest = f.Mid(colonPos + 1);
            int typePos = rest.Find(" Type:");
            if (typePos != wxNOT_FOUND)
            {
                fl.append(rest.Left(typePos), StatsStyle::Sql);
                fl.append(rest.Mid(typePos), StatsStyle::MetricValue);
            }
            else
            {
                fl.append(rest, StatsStyle::Sql);
            }
        }
        else
        {
            fl.append(f, StatsStyle::Sql);
        }
        lines.push_back(fl.build());
    }

    return lines;
}

std::vector<StatsFormattedLine> formatPlanGroup(
    const wxString& planText,
    bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    wxString normalized = planText;
    normalized.Replace("\r\n", "\n");
    normalized.Replace("\r", "\n");

    wxArrayString rawLines = wxSplit(normalized, '\n', '\0');
    std::vector<wxString> nonEmpty;
    for (const wxString& rl : rawLines)
    {
        wxString t = rl;
        t.Trim(true);
        if (!t.empty())
            nonEmpty.push_back(t);
    }
    if (nonEmpty.empty())
        nonEmpty.push_back(_("Plan not available."));

    bool hasNaturalScan = false;
    for (const wxString& rl : nonEmpty)
    {
        wxString u = rl.Upper();
        if (u.Contains("NATURAL") || u.Contains("FULL SCAN") || u.Contains("TABLE SCAN"))
        {
            hasNaturalScan = true;
            break;
        }
    }

    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("Query Plan"), StatsStyle::GroupHeader);
    if (hasNaturalScan)
        hdr.append("  [Full/Natural Scan]", StatsStyle::MetricWarning);
    lines.push_back(hdr.build());

    for (const wxString& rl : nonEmpty)
        appendPlanLineWithHighlights(rl, lines);

    return lines;
}

std::vector<StatsFormattedLine> formatParametersGroup(
    int paramCount,
    bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(wxString::Format(_("Parameters (%d)"), paramCount),
        StatsStyle::GroupHeader);
    lines.push_back(hdr.build());

    LineBuilder detail(2);
    detail.append("      ", StatsStyle::Default);
    detail.append(_("Parameters: "), StatsStyle::MetricLabel);
    detail.append(wxString::Format("%d", paramCount), StatsStyle::MetricValue);
    lines.push_back(detail.build());

    return lines;
}

std::vector<StatsFormattedLine> formatExecutionGroup(
    const wxString& elapsedStr,
    const wxString& affectedRowsMsg,
    bool inProgress,
    bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    LineBuilder hdr(1, true, inProgress ? true : defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("Execution"), StatsStyle::GroupHeader);
    if (inProgress)
    {
        hdr.append("  (" + wxString(_("executing...")) + ")", StatsStyle::MetricLabel);
    }
    else
    {
        if (!elapsedStr.empty())
            hdr.append("  (" + elapsedStr + ")", StatsStyle::MetricValue);
        if (!affectedRowsMsg.empty())
            hdr.append("  " + affectedRowsMsg, StatsStyle::MetricLabel);
    }
    lines.push_back(hdr.build());

    if (inProgress)
    {
        LineBuilder det(2);
        det.append("      " + wxString(_("Executing statement...")), StatsStyle::MetricLabel);
        lines.push_back(det.build());
    }
    else
    {
        if (!elapsedStr.empty())
        {
            LineBuilder det(2);
            det.append("      " + wxString::Format(_("Statement executed (elapsed time: %s)."),
                elapsedStr.c_str()), StatsStyle::MetricLabel);
            lines.push_back(det.build());
        }
        if (!affectedRowsMsg.empty())
        {
            LineBuilder aff(2);
            aff.append("      " + affectedRowsMsg, StatsStyle::MetricValue);
            lines.push_back(aff.build());
        }
    }

    return lines;
}

std::vector<StatsFormattedLine> formatBufferMetricsGroup(
    int fetches, int reads, int writes, int marks, bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("Page Buffer Metrics"), StatsStyle::GroupHeader);
    if (reads > 0 || writes > 0)
    {
        hdr.append(wxString::Format("  (Reads: %d, Writes: %d)", reads, writes),
            StatsStyle::MetricWarning);
    }
    lines.push_back(hdr.build());

    LineBuilder det(2);
    det.append("      ", StatsStyle::Default);
    det.append(_("Fetches: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", fetches), StatsStyle::MetricValue);
    det.append(" | ", StatsStyle::MetricLabel);
    det.append(_("Reads: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", reads),
        reads > 0 ? StatsStyle::MetricWarning : StatsStyle::MetricValue);
    det.append(" | ", StatsStyle::MetricLabel);
    det.append(_("Writes: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", writes),
        writes > 0 ? StatsStyle::MetricWarning : StatsStyle::MetricValue);
    det.append(" | ", StatsStyle::MetricLabel);
    det.append(_("Marks: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", marks), StatsStyle::MetricValue);
    lines.push_back(det.build());

    return lines;
}

std::vector<StatsFormattedLine> formatRecordMetricsGroup(
    int indexedReads, int sequentialScans, bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("Record I/O Metrics"), StatsStyle::GroupHeader);
    if (sequentialScans > 0)
    {
        hdr.append(wxString::Format("  (Sequential Scans: %d)", sequentialScans),
            StatsStyle::MetricWarning);
    }
    lines.push_back(hdr.build());

    LineBuilder det(2);
    det.append("      ", StatsStyle::Default);
    det.append(_("Indexed Reads (ridx): "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", indexedReads), StatsStyle::MetricValue);
    det.append(" | ", StatsStyle::MetricLabel);
    det.append(_("Sequential Scans (rseq): "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", sequentialScans),
        sequentialScans > 0 ? StatsStyle::MetricWarning : StatsStyle::MetricValue);
    lines.push_back(det.build());

    return lines;
}

std::vector<StatsFormattedLine> formatDmlMetricsGroup(
    int inserts, int updates, int deletes, bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("DML Modifications"), StatsStyle::GroupHeader);
    lines.push_back(hdr.build());

    LineBuilder det(2);
    det.append("      ", StatsStyle::Default);
    det.append(_("Inserts: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", inserts), StatsStyle::MetricValue);
    det.append(" | ", StatsStyle::MetricLabel);
    det.append(_("Updates: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", updates), StatsStyle::MetricValue);
    det.append(" | ", StatsStyle::MetricLabel);
    det.append(_("Deletes: "), StatsStyle::MetricLabel);
    det.append(wxString::Format("%d", deletes), StatsStyle::MetricValue);
    lines.push_back(det.build());

    return lines;
}

std::vector<StatsFormattedLine> formatMemoryMetricsGroup(
    int deltaBytes, bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(_("Memory Consumption"), StatsStyle::GroupHeader);
    lines.push_back(hdr.build());

    LineBuilder det(2);
    det.append("      ", StatsStyle::Default);
    det.append(_("Delta Memory: "), StatsStyle::MetricLabel);
    det.append(wxString::Format(_("%d bytes"), deltaBytes), StatsStyle::MetricValue);
    lines.push_back(det.build());

    return lines;
}

std::vector<StatsFormattedLine> formatTableActivityGroup(
    const std::vector<StatsTableActivityItem>& items, bool defaultExpanded)
{
    std::vector<StatsFormattedLine> lines;
    if (items.empty())
        return lines;

    LineBuilder hdr(1, true, defaultExpanded);
    hdr.append("  ", StatsStyle::Default);
    hdr.append(wxString::Format(_("Per-Table Activity (%zu)"), items.size()),
        StatsStyle::GroupHeader);
    lines.push_back(hdr.build());

    for (const auto& item : items)
    {
        LineBuilder det(2);
        det.append("      ", StatsStyle::Default);
        det.append(item.relationName + ": ", StatsStyle::Sql);
        if (item.inserts > 0)
        {
            det.append(wxString::Format(_("%d inserts. "), item.inserts),
                StatsStyle::MetricValue);
        }
        if (item.updates > 0)
        {
            det.append(wxString::Format(_("%d updates. "), item.updates),
                StatsStyle::MetricValue);
        }
        if (item.deletes > 0)
        {
            det.append(wxString::Format(_("%d deletes. "), item.deletes),
                StatsStyle::MetricValue);
        }
        if (item.indexedReads > 0)
        {
            det.append(wxString::Format(_("%d reads index. "), item.indexedReads),
                StatsStyle::MetricValue);
        }
        if (item.sequentialReads > 0)
        {
            det.append(wxString::Format(_("%d reads sequence. "), item.sequentialReads),
                StatsStyle::MetricWarning);
        }
        lines.push_back(det.build());
    }

    return lines;
}

} // namespace StatsFormatter
