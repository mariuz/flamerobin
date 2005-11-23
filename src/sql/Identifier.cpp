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
#include "Identifier.h"
//----------------------------------------------------------------------------
Identifier::Identifier(const wxString& source)
{
    set(source);
}
//----------------------------------------------------------------------------
Identifier::Identifier()
{
}
//----------------------------------------------------------------------------
void Identifier::setDirect(const wxString& source)
{
    textM = source;
}
//----------------------------------------------------------------------------
void Identifier::set(const wxString& source)
{
    if (source.IsEmpty())
        return;
    wxString temp(source);          // remove the constness
    temp.Trim(true);   // maybe these could be removed as parser is not going
    temp.Trim(false);  // to send whitespace anyway
    if (temp[0] == wxChar('\"'))
    {
        wxString::size_type p = temp.Length();
        if (temp[p-1] == wxChar('\"'))
            textM = temp.SubString(1, p-2);
        else                    // a really strange occurence of identifier
            textM = temp;     // starting with quote and not ending with it
    }
    else
        textM = temp.Upper();
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
bool Identifier::isReserved() const
{
    // needed to be like this, since RogueWave std library does not implement
    // operator == for const_iterator vs iterator, and find() returns a
    // non-const iterator.
    const Identifier::keywordContainer& k = Identifier::getKeywordSet();
    Identifier::keywordContainer::const_iterator ci = k.find(textM.Lower());
    return (ci == k.end());
}
//----------------------------------------------------------------------------
bool Identifier::needsQuoting() const
{
    if (isReserved() || !textM.IsAscii() || textM != textM.Upper())
        return true;

    // isalnum can return true for letters in character set from
    // locale. That's why we need isAscii check above
    for (wxString::size_type i = 0; i < textM.Length(); i++)
    {
        wxChar c = textM[i];
        if (!wxIsalnum(c) || wxIsspace(c))
            return true;
    }
    return false;
}
//----------------------------------------------------------------------------
bool Identifier::equals(const Identifier& other) const
{
    return textM == other.textM;
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
    static bool alwaysQuoteM = config().get(wxT("alwaysQuoteIdentifiers"), false);
    if (alwaysQuoteM || needsQuoting())
        return wxT("\"") + textM + wxT("\"");
    else
        return textM;
}
//----------------------------------------------------------------------------
