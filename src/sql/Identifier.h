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
#ifndef FR_IDENTIFIER_H
#define FR_IDENTIFIER_H

#include <set>
//----------------------------------------------------------------------------
//! The purpose of this class is to abstract all the work with identifiers
//! so that we don't have to struggle with quoted identifiers all over the
//! place. If also makes matching easier (upper/lower case problems)
class Identifier
{
public:
    typedef std::set<wxString> keywordContainer;
    Identifier();
    Identifier(const wxString& source);
    void set(const wxString& source);
    void setDirect(const wxString& source);

    static const keywordContainer& getKeywordSet();
    static wxString getKeywords(bool lowerCase = false);

    bool equals(const Identifier& other) const;
    wxString get() const;
    wxString getQuoted() const;

private:
    wxString textM;
    bool needsQuoting() const;
    bool isReserved() const;
};
//----------------------------------------------------------------------------
#endif
