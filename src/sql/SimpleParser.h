#ifndef FR_PARSER_H
#define FR_PARSER_H

#include <vector>
//-----------------------------------------------------------------------------
// collection of functions to parse SQL scripts
class SimpleParser
{
public:
    static bool stripSql(wxString &sql);
    static wxString::size_type nextToken(wxString& in, wxString& out);
    static wxString::size_type getTableNames(std::vector<wxString>& list, wxString sql);
    static void removeComments(wxString& sql, const wxString startComment, const wxString endComment);
};
//-----------------------------------------------------------------------------
#endif
