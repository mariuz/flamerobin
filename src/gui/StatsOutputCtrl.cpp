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

#include "config/Config.h"
#include "gui/FRStyle.h"
#include "gui/FRStyleManager.h"
#include "gui/StatsOutputCtrl.h"

namespace
{

class WritableStcScope
{
private:
    wxStyledTextCtrl* ctrlM;
    bool wasReadOnlyM;

public:
    explicit WritableStcScope(wxStyledTextCtrl* ctrl)
        : ctrlM(ctrl), wasReadOnlyM(ctrl ? ctrl->GetReadOnly() : false)
    {
        if (ctrlM && wasReadOnlyM)
            ctrlM->SetReadOnly(false);
    }

    ~WritableStcScope()
    {
        if (ctrlM && wasReadOnlyM)
            ctrlM->SetReadOnly(true);
    }
};

enum
{
    ID_CTX_EXPAND_ALL = wxID_HIGHEST + 420,
    ID_CTX_COLLAPSE_ALL,
    ID_CTX_CLEAR_HISTORY
};

} // namespace

StatsExpandPrefs loadStatsExpandPrefs(Config& cfg)
{
    StatsExpandPrefs prefs;
    prefs.clearStatsOnExecute = cfg.get("SQLEditorClearStatsOnExecute", true);
    prefs.showStatsHistory = cfg.get("SQLEditorShowStatsHistory", true);
    prefs.expandPrepare = cfg.get("statsExpandPrepare", false);
    prefs.expandFields = cfg.get("statsExpandFields", false);
    prefs.expandPlan = cfg.get("statsExpandPlan", true);
    prefs.expandParameters = cfg.get("statsExpandParameters", false);
    prefs.expandExecution = cfg.get("statsExpandExecution", false);
    prefs.expandBufferMetrics = cfg.get("statsExpandBufferMetrics", true);
    prefs.expandRecordMetrics = cfg.get("statsExpandRecordMetrics", true);
    prefs.expandDmlMetrics = cfg.get("statsExpandDmlMetrics", true);
    prefs.expandMemoryMetrics = cfg.get("statsExpandMemoryMetrics", true);
    prefs.expandTableReads = cfg.get("statsExpandTableReads", true);
    return prefs;
}

StatsOutputCtrl::StatsOutputCtrl(
    wxWindow* parent, wxWindowID id, bool isHistoryPane)
    : wxStyledTextCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME),
      isHistoryPaneM(isHistoryPane)
{
    SetLexer(wxSTC_LEX_NULL);
    SetWrapMode(wxSTC_WRAP_WORD);
    SetReadOnly(true);

    SetMarginWidth(0, 0);
    SetMarginWidth(1, 0);
    SetMarginType(kFoldMarginIndex, wxSTC_MARGIN_SYMBOL);
    SetMarginMask(kFoldMarginIndex, wxSTC_MASK_FOLDERS);
    SetMarginWidth(kFoldMarginIndex, 16);
    SetMarginSensitive(kFoldMarginIndex, true);

    MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_ARROWDOWN);
    MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_ARROW);
    MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY);
    MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY);
    MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_ARROW);
    MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_ARROWDOWN);
    MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY);

    SetAutomaticFold(wxSTC_AUTOMATICFOLD_SHOW | wxSTC_AUTOMATICFOLD_CLICK
        | wxSTC_AUTOMATICFOLD_CHANGE);
    SetFoldFlags(0);

    Bind(wxEVT_CONTEXT_MENU, &StatsOutputCtrl::OnContextMenu, this);
    setupStyles();
}

void StatsOutputCtrl::setupStyles()
{
    SetLexer(wxSTC_LEX_NULL);
    stylerManager().assignGlobal(this);
    SetWrapMode(wxSTC_WRAP_WORD);

    SetMarginWidth(0, 0);
    SetMarginWidth(1, 0);
    SetMarginType(kFoldMarginIndex, wxSTC_MARGIN_SYMBOL);
    SetMarginMask(kFoldMarginIndex, wxSTC_MASK_FOLDERS);
    SetMarginWidth(kFoldMarginIndex, 16);
    SetMarginSensitive(kFoldMarginIndex, true);

    bool isDark = false;
    wxColour baseBg = StyleGetBackground(wxSTC_STYLE_DEFAULT);
    FRStyle* globalStyle = stylerManager().getGlobalStyle();
    if (globalStyle)
    {
        baseBg = globalStyle->getbgColor();
        double luminance = baseBg.Red() * 0.299
            + baseBg.Green() * 0.587 + baseBg.Blue() * 0.114;
        isDark = (luminance < 128.0);
    }
    else
    {
        isDark = FRStyleManager::isEffectivelyDark();
    }

    wxColour marginBg = isDark ? wxColour(32, 36, 42) : wxColour(242, 244, 248);
    wxColour markerFg = isDark ? wxColour(185, 195, 210) : wxColour(75, 85, 100);
    SetFoldMarginColour(true, marginBg);
    SetFoldMarginHiColour(true, marginBg);
    const int folderMarkers[] = {
        wxSTC_MARKNUM_FOLDEROPEN,
        wxSTC_MARKNUM_FOLDER,
        wxSTC_MARKNUM_FOLDERSUB,
        wxSTC_MARKNUM_FOLDERTAIL,
        wxSTC_MARKNUM_FOLDEREND,
        wxSTC_MARKNUM_FOLDEROPENMID,
        wxSTC_MARKNUM_FOLDERMIDTAIL
    };
    for (int m : folderMarkers)
    {
        MarkerSetForeground(m, markerFg);
        MarkerSetBackground(m, marginBg);
    }

    const int sErr = static_cast<int>(StatsStyle::Error);
    const int sSql = static_cast<int>(StatsStyle::Sql);
    const int sRun = static_cast<int>(StatsStyle::RunHeader);
    const int sGrp = static_cast<int>(StatsStyle::GroupHeader);
    const int sPln = static_cast<int>(StatsStyle::Plan);
    const int sLbl = static_cast<int>(StatsStyle::MetricLabel);
    const int sVal = static_cast<int>(StatsStyle::MetricValue);
    const int sWrn = static_cast<int>(StatsStyle::MetricWarning);

    if (isDark)
    {
        wxColour runBg(42, 54, 72);
        StyleSetForeground(sErr, wxColour(255, 110, 110));
        StyleSetBold(sErr, true);

        StyleSetForeground(sSql, wxColour(135, 206, 250));

        StyleSetForeground(sRun, wxColour(235, 242, 252));
        StyleSetBackground(sRun, runBg);
        StyleSetBold(sRun, true);
        StyleSetEOLFilled(sRun, true);

        StyleSetForeground(sGrp, wxColour(205, 218, 238));
        StyleSetBold(sGrp, true);

        StyleSetForeground(sPln, wxColour(152, 222, 162));
        StyleSetForeground(sLbl, wxColour(168, 178, 192));

        StyleSetForeground(sVal, wxColour(232, 238, 246));
        StyleSetBold(sVal, true);

        StyleSetForeground(sWrn, wxColour(255, 190, 75));
        StyleSetBold(sWrn, true);

        IndicatorSetForeground(kIndicatorHistoryMatch, wxColour(95, 165, 255));
    }
    else
    {
        wxColour runBg(226, 236, 250);
        StyleSetForeground(sErr, wxColour(195, 25, 25));
        StyleSetBold(sErr, true);

        StyleSetForeground(sSql, wxColour(0, 72, 165));

        StyleSetForeground(sRun, wxColour(20, 38, 68));
        StyleSetBackground(sRun, runBg);
        StyleSetBold(sRun, true);
        StyleSetEOLFilled(sRun, true);

        StyleSetForeground(sGrp, wxColour(35, 52, 80));
        StyleSetBold(sGrp, true);

        StyleSetForeground(sPln, wxColour(20, 115, 50));
        StyleSetForeground(sLbl, wxColour(90, 100, 115));

        StyleSetForeground(sVal, wxColour(20, 26, 36));
        StyleSetBold(sVal, true);

        StyleSetForeground(sWrn, wxColour(195, 92, 0));
        StyleSetBold(sWrn, true);

        IndicatorSetForeground(kIndicatorHistoryMatch, wxColour(40, 115, 220));
    }

    IndicatorSetStyle(kIndicatorHistoryMatch, wxSTC_INDIC_ROUNDBOX);
    IndicatorSetAlpha(kIndicatorHistoryMatch, 40);
    IndicatorSetOutlineAlpha(kIndicatorHistoryMatch, 140);
    IndicatorSetUnder(kIndicatorHistoryMatch, true);
}

void StatsOutputCtrl::setClearHistoryCallback(std::function<void()> cb)
{
    clearHistoryCallbackM = std::move(cb);
}

void StatsOutputCtrl::clearAllContent()
{
    WritableStcScope ws(this);
    ClearAll();
    EmptyUndoBuffer();
    activeRunHeaderLineM = -1;
    activePrepareStartLineM = -1;
    activePrepareLineCountM = 0;
    activeExecStartLineM = -1;
    activeExecLineCountM = 0;
}

int StatsOutputCtrl::getLastContentLine() const
{
    int lines = GetLineCount();
    if (lines <= 1 && GetLength() == 0)
        return 0;
    // Because every appended line ends with '\n', GetLineCount() - 1 is the
    // trailing empty sentinel line; the last content line is GetLineCount() - 2.
    return (lines >= 2) ? (lines - 2) : 0;
}

int StatsOutputCtrl::appendFormattedLineInternal(const StatsFormattedLine& line)
{
    WritableStcScope ws(this);
    int startPos = GetLength();
    int lineIdx = LineFromPosition(startPos);
    AppendText(line.text + "\n");
    styleAndFoldLine(lineIdx, line);

    int trailingLine = GetLineCount() - 1;
    if (trailingLine > lineIdx)
        SetFoldLevel(trailingLine, wxSTC_FOLDLEVELBASE);

    return lineIdx;
}

void StatsOutputCtrl::styleAndFoldLine(
    int lineIdx, const StatsFormattedLine& line)
{
    int lineStartPos = PositionFromLine(lineIdx);
    int lineEndPos = GetLineEndPosition(lineIdx);
    int byteLen = lineEndPos - lineStartPos;

    if (byteLen > 0)
    {
        StartStyling(lineStartPos);
        SetStyling(byteLen, static_cast<int>(StatsStyle::Default));
        for (const auto& span : line.spans)
        {
            if (span.byteOffset >= static_cast<size_t>(byteLen))
                continue;
            size_t maxLen = static_cast<size_t>(byteLen) - span.byteOffset;
            size_t applyLen = span.byteLength < maxLen ? span.byteLength : maxLen;
            if (applyLen > 0)
            {
                StartStyling(lineStartPos + static_cast<int>(span.byteOffset));
                SetStyling(static_cast<int>(applyLen), static_cast<int>(span.style));
            }
        }
    }

    int foldLevel = wxSTC_FOLDLEVELBASE + line.foldDepth;
    if (line.isFoldHeader)
        foldLevel |= wxSTC_FOLDLEVELHEADERFLAG;
    SetFoldLevel(lineIdx, foldLevel);
}

void StatsOutputCtrl::applyGroupFoldState(int headerLine, bool expanded)
{
    if (headerLine < 0 || headerLine >= GetLineCount())
        return;
    bool isExpanded = GetFoldExpanded(headerLine);
    if (isExpanded != expanded)
    {
        FoldLine(headerLine,
            expanded ? wxSTC_FOLDACTION_EXPAND : wxSTC_FOLDACTION_CONTRACT);
    }
}

void StatsOutputCtrl::replaceLineBlock(
    int startLine,
    int oldLineCount,
    const std::vector<StatsFormattedLine>& newLines)
{
    if (startLine < 0 || oldLineCount <= 0 || startLine >= GetLineCount())
    {
        appendGroupLines(newLines);
        return;
    }

    WritableStcScope ws(this);
    int blockStartPos = PositionFromLine(startLine);
    int afterOldLine = startLine + oldLineCount;
    int blockEndPos = (afterOldLine < GetLineCount())
        ? PositionFromLine(afterOldLine)
        : GetLength();

    wxString replacement;
    for (const auto& nl : newLines)
        replacement += nl.text + "\n";

    SetTargetStart(blockStartPos);
    SetTargetEnd(blockEndPos);
    ReplaceTarget(replacement);

    int delta = static_cast<int>(newLines.size()) - oldLineCount;
    if (activeExecStartLineM > startLine)
        activeExecStartLineM += delta;

    for (size_t i = 0; i < newLines.size(); ++i)
        styleAndFoldLine(startLine + static_cast<int>(i), newLines[i]);

    int trailingLine = GetLineCount() - 1;
    if (trailingLine >= startLine + static_cast<int>(newLines.size()))
        SetFoldLevel(trailingLine, wxSTC_FOLDLEVELBASE);

    if (!newLines.empty() && newLines[0].isFoldHeader)
        applyGroupFoldState(startLine, newLines[0].defaultExpanded);
}

int StatsOutputCtrl::beginStatementRun(
    const wxString& runLabel,
    const wxString& timeStr,
    const wxString& sqlPreview)
{
    WritableStcScope ws(this);
    if (GetLength() > 0)
    {
        StatsFormattedLine sep = StatsFormatter::formatStandaloneLine(
            wxEmptyString, StatsStyle::Default, 0);
        appendFormattedLineInternal(sep);
    }

    activeRunLabelM = runLabel;
    activeRunTimeStrM = timeStr;
    activeRunSqlPreviewM = sqlPreview;
    activePrepareStartLineM = -1;
    activePrepareLineCountM = 0;
    activeExecStartLineM = -1;
    activeExecLineCountM = 0;

    StatsFormattedLine hdr = StatsFormatter::formatRunHeaderLine(
        runLabel, timeStr, sqlPreview, _("running..."), false);
    activeRunHeaderLineM = appendFormattedLineInternal(hdr);
    return activeRunHeaderLineM;
}

void StatsOutputCtrl::finishStatementRun(
    const wxString& statusText, bool isError)
{
    if (activeRunHeaderLineM < 0 || activeRunHeaderLineM >= GetLineCount())
        return;

    bool wasExpanded = GetFoldExpanded(activeRunHeaderLineM);
    StatsFormattedLine updated = StatsFormatter::formatRunHeaderLine(
        activeRunLabelM, activeRunTimeStrM, activeRunSqlPreviewM,
        statusText, isError);

    WritableStcScope ws(this);
    int lineStart = PositionFromLine(activeRunHeaderLineM);
    int lineEnd = GetLineEndPosition(activeRunHeaderLineM);
    SetTargetStart(lineStart);
    SetTargetEnd(lineEnd);
    ReplaceTarget(updated.text);
    styleAndFoldLine(activeRunHeaderLineM, updated);
    if (!wasExpanded)
        applyGroupFoldState(activeRunHeaderLineM, false);

    activeRunHeaderLineM = -1;
    activePrepareStartLineM = -1;
    activePrepareLineCountM = 0;
    activeExecStartLineM = -1;
    activeExecLineCountM = 0;
}

void StatsOutputCtrl::beginPrepareGroup(
    const wxString& sql, bool startedTransaction)
{
    auto lines = StatsFormatter::formatPrepareGroup(
        sql, startedTransaction, wxEmptyString, true, true);
    if (lines.empty())
        return;
    activePrepareStartLineM = appendFormattedLineInternal(lines[0]);
    for (size_t i = 1; i < lines.size(); ++i)
        appendFormattedLineInternal(lines[i]);
    activePrepareLineCountM = static_cast<int>(lines.size());
}

void StatsOutputCtrl::completePrepareGroup(
    const wxString& sql,
    bool startedTransaction,
    const wxString& elapsedStr,
    bool defaultExpanded)
{
    auto lines = StatsFormatter::formatPrepareGroup(
        sql, startedTransaction, elapsedStr, false, defaultExpanded);
    if (activePrepareStartLineM >= 0 && activePrepareLineCountM > 0)
    {
        replaceLineBlock(activePrepareStartLineM, activePrepareLineCountM, lines);
        activePrepareStartLineM = -1;
        activePrepareLineCountM = 0;
    }
    else
    {
        appendGroupLines(lines);
    }
}

void StatsOutputCtrl::beginExecutionGroup()
{
    auto lines = StatsFormatter::formatExecutionGroup(
        wxEmptyString, wxEmptyString, true, true);
    if (lines.empty())
        return;
    activeExecStartLineM = appendFormattedLineInternal(lines[0]);
    for (size_t i = 1; i < lines.size(); ++i)
        appendFormattedLineInternal(lines[i]);
    activeExecLineCountM = static_cast<int>(lines.size());
}

void StatsOutputCtrl::completeExecutionGroup(
    const wxString& elapsedStr,
    const wxString& affectedRowsMsg,
    bool defaultExpanded)
{
    auto lines = StatsFormatter::formatExecutionGroup(
        elapsedStr, affectedRowsMsg, false, defaultExpanded);
    if (activeExecStartLineM >= 0 && activeExecLineCountM > 0)
    {
        replaceLineBlock(activeExecStartLineM, activeExecLineCountM, lines);
        activeExecStartLineM = -1;
        activeExecLineCountM = 0;
    }
    else
    {
        appendGroupLines(lines);
    }
}

void StatsOutputCtrl::appendGroupLines(
    const std::vector<StatsFormattedLine>& lines)
{
    if (lines.empty())
        return;

    int firstLine = appendFormattedLineInternal(lines[0]);
    for (size_t i = 1; i < lines.size(); ++i)
        appendFormattedLineInternal(lines[i]);

    if (lines[0].isFoldHeader && !lines[0].defaultExpanded)
        applyGroupFoldState(firstLine, false);
}

void StatsOutputCtrl::appendStandaloneLine(
    const wxString& text, StatsStyle style, int foldDepth)
{
    StatsFormattedLine line = StatsFormatter::formatStandaloneLine(
        text, style, foldDepth);
    appendFormattedLineInternal(line);
}

void StatsOutputCtrl::expandAll()
{
    int count = GetLineCount();
    for (int line = 0; line < count; ++line)
    {
        int lvl = GetFoldLevel(line);
        if ((lvl & wxSTC_FOLDLEVELHEADERFLAG) && !GetFoldExpanded(line))
            FoldLine(line, wxSTC_FOLDACTION_EXPAND);
    }
}

void StatsOutputCtrl::collapseAll(bool collapseTopLevelRuns)
{
    int count = GetLineCount();
    // Collapse inner groups first so re-expanding a top-level run shows tidy sections
    for (int line = 0; line < count; ++line)
    {
        int lvl = GetFoldLevel(line);
        int depth = (lvl & wxSTC_FOLDLEVELNUMBERMASK) - wxSTC_FOLDLEVELBASE;
        if ((lvl & wxSTC_FOLDLEVELHEADERFLAG) && depth >= 1 && GetFoldExpanded(line))
            FoldLine(line, wxSTC_FOLDACTION_CONTRACT);
    }

    if (collapseTopLevelRuns)
    {
        for (int line = 0; line < count; ++line)
        {
            int lvl = GetFoldLevel(line);
            int depth = (lvl & wxSTC_FOLDLEVELNUMBERMASK) - wxSTC_FOLDLEVELBASE;
            if ((lvl & wxSTC_FOLDLEVELHEADERFLAG) && depth == 0 && GetFoldExpanded(line))
                FoldLine(line, wxSTC_FOLDACTION_CONTRACT);
        }
    }
}

void StatsOutputCtrl::highlightLineRange(int startLine, int endLine)
{
    clearHistoryHighlight();
    int totalLines = GetLineCount();
    if (startLine < 0 || startLine >= totalLines)
        return;
    if (endLine < startLine)
        endLine = startLine;
    if (endLine >= totalLines)
        endLine = totalLines - 1;

    EnsureVisibleEnforcePolicy(startLine);

    int startPos = PositionFromLine(startLine);
    int endPos = GetLineEndPosition(endLine);
    if (endPos > startPos)
    {
        WritableStcScope ws(this);
        SetIndicatorCurrent(kIndicatorHistoryMatch);
        IndicatorFillRange(startPos, endPos - startPos);
    }

    ScrollToLine(startLine);
}

void StatsOutputCtrl::clearHistoryHighlight()
{
    int len = GetLength();
    if (len > 0)
    {
        WritableStcScope ws(this);
        SetIndicatorCurrent(kIndicatorHistoryMatch);
        IndicatorClearRange(0, len);
    }
}

void StatsOutputCtrl::OnContextMenu(wxContextMenuEvent& WXUNUSED(event))
{
    wxMenu menu;
    menu.Append(wxID_COPY, _("&Copy"));
    menu.Enable(wxID_COPY, GetSelectionStart() != GetSelectionEnd());
    menu.Append(wxID_SELECTALL, _("Select &All"));
    menu.Enable(wxID_SELECTALL, GetLength() > 0);
    menu.AppendSeparator();
    menu.Append(ID_CTX_EXPAND_ALL, _("&Expand All"));
    menu.Enable(ID_CTX_EXPAND_ALL, GetLength() > 0);
    menu.Append(ID_CTX_COLLAPSE_ALL, _("C&ollapse All"));
    menu.Enable(ID_CTX_COLLAPSE_ALL, GetLength() > 0);
    if (isHistoryPaneM && clearHistoryCallbackM)
    {
        menu.AppendSeparator();
        menu.Append(ID_CTX_CLEAR_HISTORY, _("Clear &History"));
        menu.Enable(ID_CTX_CLEAR_HISTORY, GetLength() > 0);
    }

    int sel = GetPopupMenuSelectionFromUser(menu);
    if (sel == wxID_COPY)
        Copy();
    else if (sel == wxID_SELECTALL)
        SelectAll();
    else if (sel == ID_CTX_EXPAND_ALL)
        expandAll();
    else if (sel == ID_CTX_COLLAPSE_ALL)
        collapseAll(isHistoryPaneM);
    else if (sel == ID_CTX_CLEAR_HISTORY && clearHistoryCallbackM)
        clearHistoryCallbackM();
}
