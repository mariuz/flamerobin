/*
Copyright (c) 2004, 2005, 2006 The FlameRobin Development Team

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


  $Id$

*/
// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "config/Config.h"
#include "sql/Identifier.h"
//----------------------------------------------------------------------------
Identifier::Identifier(const wxString& source)
{
    setText(source);
}
//----------------------------------------------------------------------------
Identifier::Identifier()
{
}
//----------------------------------------------------------------------------
void Identifier::setText(const wxString& source)
{
    // although it may not be completely correct we right-trim everything we
    // get. This means that users can't use quoted identifiers which end with
    // a space - but who does that anyway
    textM = source.Strip();
}
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
bool Identifier::isQuoted(const wxString &s)
{
    wxString::size_type p = s.Length();
    return (s[0] == wxChar('\"') && p > 1 && s[p - 1] == wxChar('\"'));
}
//----------------------------------------------------------------------------
wxString& Identifier::escape(wxString& s)
{
    s.Replace(wxT("\""), wxT("\"\""));
    return s;
}
//----------------------------------------------------------------------------
wxString& Identifier::strip(wxString& s)
{
    if (isQuoted(s))
        s = s.SubString(1, s.Length()-2);
    return s;
}
//----------------------------------------------------------------------------
wxString& Identifier::quote(wxString &s)
{
    s = wxT("\"") + s + wxT("\"");
    return s;
}
//----------------------------------------------------------------------------
wxString Identifier::userString(const wxString& s)
{
    if (s.IsEmpty())
        return wxEmptyString;
    wxString ret(s);
    bool alwaysQuote = !config().get(wxT("quoteOnlyWhenNeeded"), true);
    bool quoteCharsAreRegular = config().get(wxT("quoteCharsAreRegular"), false);
    if (alwaysQuote)
    {
        if (quoteCharsAreRegular)
            return quote(escape(ret));
        else
            return quote(escape(strip(ret)));
    }
    else
    {
        if (isQuoted(ret))   // pass the quoted text as-it-is
            return ret;
        bool quoteMixedCase = config().get(wxT("quoteMixedCase"), false);
        if (quoteMixedCase && ret.Upper() != ret && ret.Lower() != ret)
            return quote(escape(ret));
        if (Identifier::needsQuoting(ret.Upper()))    // special chars
            return quote(escape(ret));
        return ret;
    }
}
//----------------------------------------------------------------------------
const Identifier::keywordContainer& Identifier::getKeywordSet()
{
    // placed here, so others can't access it until it is initialized
    static keywordContainer keywords;
    if (keywords.empty())   // load
    {
        #include "keywords.txt"
    }
    return keywords;
}
//----------------------------------------------------------------------------
wxString Identifier::getKeywords(bool lowerCase)
{
    static wxString resultLower;
    static wxString resultUpper;
    wxString& s = (lowerCase ? resultLower : resultUpper);
    if (s.IsEmpty())
    {
        for (keywordContainer::const_iterator it = getKeywordSet().begin();
            it != getKeywordSet().end(); ++it)
        {
            s += (lowerCase ? (*it) : (*it).Upper()) + wxT(" ");
        }
    }
    return s;
}
//----------------------------------------------------------------------------
bool Identifier::isReserved(const wxString& s)
{
    // needed to be like this, since RogueWave std library does not implement
    // operator == for const_iterator vs iterator, and find() returns a
    // non-const iterator.
    const Identifier::keywordContainer& k = Identifier::getKeywordSet();
    Identifier::keywordContainer::const_iterator ci = k.find(s.Lower());
    return (ci != k.end());
}
//----------------------------------------------------------------------------
bool Identifier::needsQuoting(const wxString& s)
{
    if (s.IsEmpty())
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
    return isReserved(s);
}
//----------------------------------------------------------------------------
bool Identifier::equals(const Identifier& rhs) const
{
    return textM == rhs.textM;
}
//----------------------------------------------------------------------------
bool Identifier::equals(const wxString& rhs) const
{
    if (needsQuoting(textM))
        return (0 == rhs.Cmp(textM));
    else
        return (0 == rhs.CmpNoCase(textM));
}
//----------------------------------------------------------------------------
wxString Identifier::get() const
{
    return textM;
}
//----------------------------------------------------------------------------
wxString Identifier::getQuoted() const
{
    // retrieved only once, needs restart to change (but it is much efficient)
    bool alwaysQuote = !config().get(wxT("quoteOnlyWhenNeeded"), true);
    if (alwaysQuote || needsQuoting(textM))
    {
        wxString retval(textM);
        return quote(escape(retval));
    }
    else
        return textM;
}
//----------------------------------------------------------------------------
