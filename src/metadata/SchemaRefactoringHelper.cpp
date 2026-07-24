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

#include "metadata/SchemaRefactoringHelper.h"
#include "sql/Identifier.h"
#include "sql/SqlTokenizer.h"
#include <wx/regex.h>

wxString SchemaRefactoringHelper::qualifyObjectReferences(const wxString& sql, const wxString& schemaName, const std::vector<wxString>& objectNames)
{
    if (sql.IsEmpty() || schemaName.IsEmpty() || objectNames.empty())
        return sql;

    wxString result = sql;
    Identifier idSchema(schemaName);
    wxString prefix = idSchema.getQuoted() + ".";

    for (const auto& obj : objectNames)
    {
        if (obj.IsEmpty())
            continue;

        Identifier idObj(obj);
        wxString targetName = idObj.get();

        // Pattern matching unqualified object identifier occurrences
        wxString pattern = "(\\b)" + wxRegEx::QuoteMeta(targetName) + "(\\b)";
        wxRegEx regex(pattern, wxRE_ICASE);

        if (regex.IsValid())
        {
            wxString qualifiedReplacement = prefix + idObj.getQuoted();
            // Perform substitution if not already schema-qualified
            wxRegEx checkAlreadyQualified("\\b" + wxRegEx::QuoteMeta(idSchema.get()) + "\\s*\\.\\s*" + wxRegEx::QuoteMeta(targetName) + "\\b", wxRE_ICASE);
            if (!checkAlreadyQualified.IsValid() || !checkAlreadyQualified.Matches(result))
            {
                regex.Replace(&result, qualifiedReplacement);
            }
        }
    }

    return result;
}

wxString SchemaRefactoringHelper::getDropSchemaStatement(const wxString& schemaName, bool cascade)
{
    Identifier idSchema(schemaName);
    return "DROP SCHEMA " + idSchema.getQuoted() + (cascade ? " CASCADE;\n" : " RESTRICT;\n");
}
