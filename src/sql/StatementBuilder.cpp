/*
  Copyright (c) 2004-2022 The FlameRobin Development Team

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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/textbuf.h>

#include "config/Config.h"
#include "sql/StatementBuilder.h"

StatementBuilder::StatementBuilder()
    : indentCharsM(0), indentLevelM(0), lineWrappingM(false), maxLineLengthM(0)
{
    keywordsUpperCaseM = config().get("SQLKeywordsUpperCase", true);
    // take settings for line wrapping from vertical editor line settings:
    // enable wrapping only if the marker is shown, and if so wrap to the
    // same column, indent by editor tab size
    
    if (config().get("sqlEditorShowEdge", false))
    {
        lineWrappingM = true;
        maxLineLengthM = config().get("sqlEditorEdgeColumn", 80);
        indentCharsM = config().get("sqlEditorTabSize", 4);
    }
}

StatementBuilder& StatementBuilder::operator<< (const ControlToken ct)
{
    switch (ct)
    {
        case NewLine:
            addNewLine();
            break;
        case IncIndent:
            ++indentLevelM;
            break;
        case DecIndent:
            if (indentLevelM)
                --indentLevelM;
            break;
        case DisableLineWrapping:
            lineWrappingM = false;
            break;
        case EnableLineWrapping:
            if (maxLineLengthM)
                lineWrappingM = true;
            break;
        default:
            wxASSERT(false);
            break;
    }
    return (*this);
}

StatementBuilder& StatementBuilder::operator<< (const char c)
{
    if (lineWrappingM && currentLineM.Length() + 1 > maxLineLengthM)
    {
        if (c != ' ')
            addNewLine();
    }
    currentLineM += c;
    return (*this);
}

StatementBuilder& StatementBuilder::operator<< (const wxString& s)
{
    if (lineWrappingM && currentLineM.Length() + s.Length() > maxLineLengthM)
        addNewLine();
    currentLineM += s;
    return (*this);
}

StatementBuilder& StatementBuilder::operator<< (const SqlTokenType stt)
{
    wxString kw(SqlTokenizer::getKeyword(stt, keywordsUpperCaseM));
    if (lineWrappingM && currentLineM.Length() + kw.Length() > maxLineLengthM)
        addNewLine();
    currentLineM += kw;
    return (*this);
}

StatementBuilder::operator wxString() const
{
    return completedLinesM + currentLineM;
}

void StatementBuilder::addNewLine()
{
    completedLinesM += currentLineM;
    completedLinesM += wxTextBuffer::GetEOL();
    currentLineM = wxString(wxChar(' '), static_cast<unsigned long>(indentLevelM) * static_cast<unsigned long>(indentCharsM));
}

void StatementBuilder::reset()
{
    completedLinesM.clear();
    currentLineM.clear();
    indentLevelM = 0;
}

