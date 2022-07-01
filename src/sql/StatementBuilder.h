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

#ifndef FR_STATEMENTBUILDER_H
#define FR_STATEMENTBUILDER_H

#include <wx/string.h>
#include "sql/SqlTokenizer.h"

class StatementBuilder
{
private:
    wxString completedLinesM;
    wxString currentLineM;
    bool keywordsUpperCaseM;
    unsigned indentCharsM;
    unsigned indentLevelM;
    bool lineWrappingM;
    unsigned maxLineLengthM;
    void addNewLine();
public:
    StatementBuilder();

    enum ControlToken { NewLine, IncIndent, DecIndent,
        DisableLineWrapping, EnableLineWrapping };
    StatementBuilder& operator<< (const ControlToken ct);

    StatementBuilder& operator<< (const char c);
    StatementBuilder& operator<< (const wxString& s);
    StatementBuilder& operator<< (const SqlTokenType stt);

    operator wxString() const;

    void reset();
};

#endif // FR_STATEMENT_BUILDER_H
