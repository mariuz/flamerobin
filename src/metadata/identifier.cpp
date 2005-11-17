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
#include "identifier.h"
//----------------------------------------------------------------------------
Identifier::Identifier(const wxString& source)
{
    textM.Trim(true);   // maybe these could be removed as parser is not going
    textM.Trim(false);  // to send whitespace anyway

    if (textM[0] == wxChar('\"'))
    {
        wxString::size_t p = textM.Length();
        if (textM[p-1] == wxChar('\"'))
            textM = source.SubString(1, p-2);
        else                    // a really strange occurence of identifier
            textM = source;     // starting with quote and not ending with it
    }
    else
        textM = source.Upper();
}
//----------------------------------------------------------------------------
static const keywordContainer& getKeywordSet()
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
static wxString getKeywords(bool lowerCase)
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
    return (getKeywordSet().find(textM.Lower()) == getKeywordSet().end());
}
//----------------------------------------------------------------------------
bool Identifier::needsQuoting() const
{
    if (isReserved() || !wxIsascii(textM) || textM != textM.Upper())
        return true;

    // isalnum can return true for letters in character set from
    // locale. That's why we need isAscii check above
    for (wxString::size_t i = 0; i < textM.Length(); i++)
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
    static bool alwaysQuoteM = config().get("alwaysQuoteIdentifiers", false);
    if (alwaysQuoteM || needsQuoting())
        return wxT("\"") + textM + wxT("\"");
    else
        return textM;
}
//----------------------------------------------------------------------------
