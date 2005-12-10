/*
  The contents of this file are subject to the Initial Developer's Public
  License Version 1.0 (the "License"); you may not use this file except in
  compliance with the License. You may obtain a copy of the License here:
  http://www.flamerobin.org/license.html.

  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.

  The Original Code is FlameRobin (TM).

  The Initial Developer of the Original Code is Milan Babuskov.

  Portions created by the original developer
  are Copyright (C) 2005 Milan Babuskov.

  All Rights Reserved.

  $Id$

  Contributor(s):
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
    if (p >= q)
    {
        textM = wxEmptyString;
        return;
    }
    // quoted identifier -> strip and unescape double quote characters
    if (*p == '\"' && *q == '\"')
    {
        // NOTE: first parameter must point to first char, but second parameter
        //       has to point to the char *after* the last char !!!
        textM = wxString(p + 1, q);
        textM.Replace(wxT("\"\""), wxT("\""));
        return;
    }
    // strings -> strip and unescape single quote characters
    if (*p == '\'' && *q == '\'')
    {
        textM = wxString(p + 1, q - 1);
        textM.Replace(wxT("\'\'"), wxT("\'"));
        return;
    }
    textM = source.Upper();
}
//----------------------------------------------------------------------------
bool Identifier::isQuoted(const wxString &s)
{
    wxString::size_type p = s.Length();
    return (s[0] == wxChar('\"') && p > 1 && s[p-1] == wxChar('\"'));
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
