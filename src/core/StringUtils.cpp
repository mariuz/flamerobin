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

#include <fstream>
#include <sstream>

#include "core/FRError.h"
#include "core/StringUtils.h"

std::string wx2std(const wxString& input, wxMBConv* conv)
{
    if (input.empty())
        return "";
    if (!conv)
        conv = wxConvCurrent;
    const wxWX2MBbuf buf(input.mb_str(*conv));
    // conversion may fail and return 0, which isn't a safe value to pass
    // to std:string constructor
    if (!buf)
        return "";
    return std::string(buf);
}

wxString std2wxIdentifier(const std::string& input, wxMBConv* conv)
{
    if (input.empty())
        return wxEmptyString;
    if (!conv)
        conv = wxConvCurrent;
    // trim trailing whitespace
    size_t last = input.find_last_not_of(" ");
    return wxString(input.c_str(), *conv,
        (last == std::string::npos) ? std::string::npos : last + 1);
}

wxString getHtmlCharset()
{
    return "UTF-8";
}

wxString escapeHtmlChars(const wxString& input, bool processNewlines)
{
    if (input.empty())
        return input;
    wxString result;
    wxString::const_iterator start = input.begin();
    while (start != input.end())
    {
        wxString::const_iterator stop = start;
        while (stop != input.end())
        {
            const wxChar c = *stop;
            if (c == '&' || c == '<' || c == '>' || c == '"'
                || (processNewlines && (c == '\r' || c == '\n')))
            {
                if (stop > start)
                    result += wxString(start, stop);
                if (c == '&')
                    result += "&amp;";
                else if (c == '<')
                    result += "&lt;";
                else if (c == '>')
                    result += "&gt;";
                else if (c == '"')
                    result += "&quot;";
                else if (c == '\n')
                    result += "<BR>";
                else if (c == '\r')
                    /* swallow silently */;
                else
                    wxASSERT_MSG(false, "escape not handled");
                // start processing *after* the replaced character
                ++stop;
                start = stop;
                break;
            }
            ++stop;
        }
        if (stop > start)
            result += wxString(start, stop);
        start = stop;
    }
    return result;
}

wxString escapeXmlChars(const wxString& input)
{
    if (input.empty())
        return input;
    wxString result;
    wxString::const_iterator start = input.begin();
    while (start != input.end())
    {
        wxString::const_iterator stop = start;
        while (stop != input.end())
        {
            const wxChar c = *stop;
            if (c == '&' || c == '<' || c == '>' || c == '"')
            {
                if (stop > start)
                    result += wxString(start, stop);
                if (c == '&')
                    result += "&amp;";
                else if (c == '<')
                    result += "&lt;";
                else if (c == '>')
                    result += "&gt;";
                else if (c == '"')
                    result += "&quot;";
                else
                    wxASSERT_MSG(false, "escape not handled");
                // start processing *after* the replaced character
                ++stop;
                start = stop;
                break;
            }
            ++stop;
        }
        if (stop > start)
            result += wxString(start, stop);
        start = stop;
    }
    return result;
}

wxString wxArrayToString(const wxArrayString& arrayStr, const wxString& delimiter)
{
    wxString result;
    for (wxArrayString::const_iterator it = arrayStr.begin();
        it != arrayStr.end(); ++it)
    {
        if (result.IsEmpty())
            result << *(it);
        else
            result << delimiter << *(it);
    }
    return result;
}

wxString loadEntireFile(const wxFileName& filename)
{
    if (!filename.FileExists())
    {
        wxString msg;
        msg.Printf(_("The file \"%s\" does not exist."),
            filename.GetFullPath().c_str());
        throw FRError(msg);
    }

    // read entire file into wxString buffer
    std::ifstream filex(wx2std(filename.GetFullPath()).c_str());
    if (!filex)
    {
        wxString msg;
        msg.Printf(_("The file \"%s\" cannot be opened."),
            filename.GetFullPath().c_str());
        throw FRError(msg);
    }

    std::stringstream ss;
    ss << filex.rdbuf();
    wxString s(ss.str());
    filex.close();
    return s;
}

wxString wrapText(const wxString& text, size_t maxWidth, size_t indent)
{
    if (text.Length() <= maxWidth)
        return text;

    enum { none, insideSingle, insideDouble, } wrapState = none;

    wxString indentStr;
    bool eol(false);
    wxString wrappedText;
    wxString line;

    wxString::const_iterator lastSpace = text.end();
    wxString::const_iterator lineStart = text.begin();
    for (wxString::const_iterator it = lineStart; ; ++it)
    {
        if (it == text.end())
        {
            wrappedText << indentStr << line;
            break;
        }

        if (eol)
        {
            wrappedText += '\n';
            if (indentStr.IsEmpty())
                indentStr.Pad(indent);

            lastSpace = text.end();
            line.clear();
            lineStart = it;
            eol = false;
        }

        if (*it == '\n')
        {
            wrappedText << indentStr << line;
            eol = true;
        }
        else
        {
            if (wrapState == insideSingle)
            {
                if (*it == '\'')
                    wrapState = none;
            }
            else if (wrapState == insideDouble)
            {
                if (*it == '"')
                    wrapState = none;
            }
            else
            {
                if (*it == ' ')
                    lastSpace = it;
                else if (*it == '\'')
                    wrapState = insideSingle;
                else if (*it == '"')
                    wrapState = insideDouble;
            }

            line += *it;

            if (lastSpace != text.end())
            {
                size_t width = line.Length();
                if ((width > maxWidth - indentStr.Length()) && (wrapState == none))
                {
                    // remove the last word from this line
                    line.erase(lastSpace - lineStart, it + 1 - lineStart);
                    wrappedText << indentStr << line;
                    eol = true;

                    // go back to the last word of this line which we didn't
                    // output yet
                    it = lastSpace;
                }
            }
            // else: no wrapping at all or impossible to wrap
        }
    }
    return wrappedText;
}

wxString normalizeNumericInput(const wxString& source, bool isInteger)
{
    wxString s(source);
    s.Trim(true).Trim(false);
    if (s.empty())
        return s;

    // Check special values
    wxString lower = s.Lower();
    if (lower == "nan" || lower == "infinity" || lower == "+infinity" || lower == "-infinity")
        return s;

    // Check for sign prefix
    wxString sign;
    if (s[0] == '+' || s[0] == '-')
    {
        sign = s[0];
        s.erase(0, 1);
        s.Trim(false);
    }

    if (s.empty())
        return sign;

    // Check for exponent (e.g. 1.23e-4 or 1,23E+10)
    wxString mantissa = s;
    wxString exponent;
    if (!isInteger)
    {
        int expIdx = -1;
        for (int i = (int)s.Length() - 1; i >= 0; --i)
        {
            if (s[i] == 'e' || s[i] == 'E')
            {
                expIdx = i;
                break;
            }
        }
        if (expIdx != -1)
        {
            mantissa = s.substr(0, expIdx);
            exponent = s.substr(expIdx);
            exponent.Replace(" ", "");
        }
    }

    // Remove all whitespace in mantissa (including standard spaces, tabs, NBSP, etc.)
    wxString cleanMantissa;
    for (size_t i = 0; i < mantissa.Length(); ++i)
    {
        wxChar ch = mantissa[i];
        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n'
            && ch != (wxChar)0x00A0 && ch != (wxChar)0x202F)
        {
            cleanMantissa += ch;
        }
    }

    if (cleanMantissa.empty())
        return sign + exponent;

    int dotCount = 0;
    int commaCount = 0;
    int lastDot = -1;
    int lastComma = -1;

    for (size_t i = 0; i < cleanMantissa.Length(); ++i)
    {
        if (cleanMantissa[i] == '.')
        {
            dotCount++;
            lastDot = (int)i;
        }
        else if (cleanMantissa[i] == ',')
        {
            commaCount++;
            lastComma = (int)i;
        }
    }

    wxString resultMantissa;

    if (dotCount > 0 && commaCount > 0)
    {
        // Both dot and comma present
        if (lastComma > lastDot)
        {
            // European format: "1.234.567,89" -> dots are thousands, comma is decimal
            for (size_t i = 0; i < cleanMantissa.Length(); ++i)
            {
                if (cleanMantissa[i] == '.')
                    continue; // drop thousand dot
                else if (cleanMantissa[i] == ',')
                {
                    if ((int)i == lastComma)
                        resultMantissa += (isInteger ? "" : ".");
                }
                else
                    resultMantissa += cleanMantissa[i];
            }
        }
        else
        {
            // US format: "1,234,567.89" -> commas are thousands, dot is decimal
            for (size_t i = 0; i < cleanMantissa.Length(); ++i)
            {
                if (cleanMantissa[i] == ',')
                    continue; // drop thousand comma
                else if (cleanMantissa[i] == '.')
                {
                    if ((int)i == lastDot)
                        resultMantissa += (isInteger ? "" : ".");
                }
                else
                    resultMantissa += cleanMantissa[i];
            }
        }
    }
    else if (commaCount > 0)
    {
        // Only commas present (no dots)
        if (isInteger)
        {
            // All commas are thousand separators
            for (size_t i = 0; i < cleanMantissa.Length(); ++i)
            {
                if (cleanMantissa[i] != ',')
                    resultMantissa += cleanMantissa[i];
            }
        }
        else
        {
            if (commaCount == 1)
            {
                // Single comma is decimal separator: "3,14" -> "3.14"
                for (size_t i = 0; i < cleanMantissa.Length(); ++i)
                {
                    if (cleanMantissa[i] == ',')
                        resultMantissa += '.';
                    else
                        resultMantissa += cleanMantissa[i];
                }
            }
            else
            {
                // Multiple commas: treat as thousand separators
                for (size_t i = 0; i < cleanMantissa.Length(); ++i)
                {
                    if (cleanMantissa[i] != ',')
                        resultMantissa += cleanMantissa[i];
                }
            }
        }
    }
    else if (dotCount > 0)
    {
        // Only dots present (no commas)
        if (isInteger)
        {
            // All dots are thousand separators
            for (size_t i = 0; i < cleanMantissa.Length(); ++i)
            {
                if (cleanMantissa[i] != '.')
                    resultMantissa += cleanMantissa[i];
            }
        }
        else
        {
            if (dotCount == 1)
            {
                // Single dot is decimal separator: "3.14"
                resultMantissa = cleanMantissa;
            }
            else
            {
                // Multiple dots: e.g. "1.234.567" -> treat as thousand separators
                for (size_t i = 0; i < cleanMantissa.Length(); ++i)
                {
                    if (cleanMantissa[i] != '.')
                        resultMantissa += cleanMantissa[i];
                }
            }
        }
    }
    else
    {
        // No dots or commas
        resultMantissa = cleanMantissa;
    }

    return sign + resultMantissa + exponent;
}
