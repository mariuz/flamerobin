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

#include "core/JsonExpressionHelper.h"
#include "sql/Identifier.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool JsonExpressionHelper::validateJson(const wxString& jsonText, wxString& errorMessage)
{
    wxString copy = jsonText;
    if (copy.Trim().IsEmpty())
    {
        errorMessage = "JSON content is empty";
        return false;
    }

    try
    {
        std::string utf8 = std::string(jsonText.ToUTF8());
        auto j = json::parse(utf8);
        errorMessage = "Valid JSON syntax";
        return true;
    }
    catch (const std::exception& e)
    {
        errorMessage = wxString::FromUTF8(e.what());
        return false;
    }
}

wxString JsonExpressionHelper::formatJson(const wxString& jsonText, int indent)
{
    try
    {
        std::string utf8 = std::string(jsonText.ToUTF8());
        auto j = json::parse(utf8);
        std::string formatted = j.dump(indent);
        return wxString::FromUTF8(formatted.c_str());
    }
    catch (...)
    {
        return jsonText;
    }
}

wxString JsonExpressionHelper::generateJsonValue(const wxString& columnName, const wxString& jsonPath, const wxString& returnType)
{
    Identifier idCol(columnName);
    wxString path = jsonPath.IsEmpty() ? "$.key" : jsonPath;
    wxString ret = returnType.IsEmpty() ? "VARCHAR(255)" : returnType;
    return "JSON_VALUE(" + idCol.getQuoted() + ", '" + path + "' RETURNING " + ret + ")";
}

wxString JsonExpressionHelper::generateJsonQuery(const wxString& columnName, const wxString& jsonPath)
{
    Identifier idCol(columnName);
    wxString path = jsonPath.IsEmpty() ? "$.key" : jsonPath;
    return "JSON_QUERY(" + idCol.getQuoted() + ", '" + path + "')";
}

wxString JsonExpressionHelper::generateJsonExists(const wxString& columnName, const wxString& jsonPath)
{
    Identifier idCol(columnName);
    wxString path = jsonPath.IsEmpty() ? "$.key" : jsonPath;
    return "JSON_EXISTS(" + idCol.getQuoted() + ", '" + path + "')";
}
