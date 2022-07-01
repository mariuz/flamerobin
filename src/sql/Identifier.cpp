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

#include "config/Config.h"
#include "core/Observer.h"
#include "core/Subject.h"
#include "sql/Identifier.h"
#include "sql/SqlTokenizer.h"

// IdentifierQuotes: class to cache config data for identifier quoting
class IdentifierQuotes: public ConfigCache
{
private:
    bool quoteAlwaysM;
    bool quoteCharsAreRegularM;
    bool quoteMixedCaseM;
protected:
    virtual void loadFromConfig();
public:
    IdentifierQuotes();

    static IdentifierQuotes& get();

    bool getQuoteAlways();
    bool getQuoteCharsAreRegular();
    bool getQuoteMixedCase();
};

IdentifierQuotes::IdentifierQuotes()
    : ConfigCache(config()), quoteAlwaysM(false),
        quoteCharsAreRegularM(false), quoteMixedCaseM(true)
{
}

IdentifierQuotes& IdentifierQuotes::get()
{
    static IdentifierQuotes iq;
    return iq;
}

void IdentifierQuotes::loadFromConfig()
{
    quoteAlwaysM = !config().get("quoteOnlyWhenNeeded", true);
    quoteCharsAreRegularM = config().get("quoteCharsAreRegular", false);
    quoteMixedCaseM = config().get("quoteMixedCase", false);
}

bool IdentifierQuotes::getQuoteAlways()
{
    ensureCacheValid();
    return quoteAlwaysM;
}

bool IdentifierQuotes::getQuoteCharsAreRegular()
{
    ensureCacheValid();
    return quoteCharsAreRegularM;
}

bool IdentifierQuotes::getQuoteMixedCase()
{
    ensureCacheValid();
    return quoteMixedCaseM;
}

// Identifier class
Identifier::Identifier(const wxString& source, int sqldialect)
   : dialectM(sqldialect)
{
    setText(source);
}

Identifier::Identifier(int sqldialect)
   : dialectM(sqldialect)
{
}

void Identifier::setText(const wxString& source)
{
    // although it may not be completely correct we right-trim everything we
    // get. This means that users can't use quoted identifiers which end with
    // a space - but who does that anyway
    textM = source.Strip();
}

void Identifier::setFromSql(const wxString& source)
{
    // const wxChar pointers to first and last characters
    const wxChar* p = source.c_str();
    const wxChar* q = p + source.Length() - 1;
    // skip leading and trailing whitespace
    while (q > p && wxIsspace(*p))
        p++;
    while (q > p && wxIsspace(*q))
        q--;
    if (p > q) // p is first, q is last character, so they may be equal...
    {
        textM = wxEmptyString;
        return;
    }
    // strings/quoted identifier -> strip and unescape single/double quotes
    if (*q == *p && (*p == '\"' || *p == '\''))
    {
        // NOTE: first parameter must point to first char, but second parameter
        //       has to point to the char *after* the last char !!!
        textM = wxString(p + 1, q);
        wxString escapedChar(p, 1);
        textM.Replace(escapedChar + escapedChar, escapedChar);
        return;
    }
    // set to uppercased input parameter, no leading and trailing whitespace
    textM = wxString(p, q + 1).Upper();
}

bool Identifier::isQuoted(const wxString &s)
{
    wxString::size_type p = s.Length();
    return (s[0] == wxChar('\"') && p > 1 && s[p - 1] == wxChar('\"'));
}

wxString& Identifier::escape(wxString& s)
{
    s.Replace("\"", "\"\"");
    return s;
}

wxString& Identifier::strip(wxString& s)
{
    if (isQuoted(s))
        s = s.SubString(1, s.Length()-2);
    return s;
}

wxString& Identifier::quote(wxString &s, int sqldialect)
{
   if (sqldialect == 1)
      return s;

    s = "\"" + s + "\"";
    return s;
}

bool hasBothCases(const wxString& value)
{
    if (value.empty())
        return false;

    bool hasLower = false;
    bool hasUpper = false;
    const wxChar* p = value.c_str();
    while (*p != 0)
    {
        if (*p >= 'A' && *p <= 'Z')
            hasUpper = true;
        if (*p >= 'a' && *p <= 'z')
            hasLower = true;
        if (hasUpper && hasLower)
            return true;
        p++;
    }
    return false;
}

wxString Identifier::userString(const wxString& s, int sqldialect)
{
    if (s.IsEmpty())
        return wxEmptyString;
    wxString ret(s);
    if (IdentifierQuotes::get().getQuoteAlways())
    {
        if (IdentifierQuotes::get().getQuoteCharsAreRegular())
            return quote(escape(ret), sqldialect);
        else
            return quote(escape(strip(ret)), sqldialect);
    }
    else
    {
        if (isQuoted(ret))   // pass the quoted text as-it-is
            return ret;
        if (IdentifierQuotes::get().getQuoteMixedCase() && hasBothCases(ret))
            return quote(escape(ret), sqldialect);
        if (Identifier::needsQuoting(ret.Upper(), sqldialect))    // special chars
            return quote(escape(ret), sqldialect);
        return ret;
    }
}

bool Identifier::needsQuoting(const wxString& s, int sqldialect)
{
    if (s.IsEmpty())
        return false;

   if (sqldialect == 1)
       return false;

    const wxChar* p = s.c_str();
    // first character: only 'A'..'Z' allowed, else quotes needed
    if (*p < 'A' || *p > 'Z')
        return true;
    p++;
    // after first character: 'A'..'Z', '0'..'9', '_', '$' allowed
    while (*p != 0)
    {
        bool validChar = (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')
            || *p == '_' || *p == '$';
        if (!validChar)
            return true;
        p++;
    }

    // may still need quotes if reserved word
    return SqlTokenizer::isReservedWord(s);
}

bool Identifier::equals(const Identifier& rhs) const
{
    return textM == rhs.textM;
}

bool Identifier::equals(const wxString& rhs) const
{
    if (needsQuoting(textM))
        return (0 == rhs.Cmp(textM));
    else
        return (0 == rhs.CmpNoCase(textM));
}

wxString Identifier::get() const
{
    return textM;
}

wxString Identifier::getQuoted() const
{
    if (IdentifierQuotes::get().getQuoteAlways() || needsQuoting(textM, dialectM))
    {
        wxString retval(textM);
        return quote(escape(retval), dialectM);
    }
    else
        return textM;
}

