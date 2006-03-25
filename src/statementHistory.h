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

#ifndef FR_HISTORY_H
#define FR_HISTORY_H

#include <wx/wx.h>
#include <vector>
//-----------------------------------------------------------------------------
class Database;
//-----------------------------------------------------------------------------
class StatementHistory
{
public:
    typedef size_t Position;

private:
    StatementHistory(const wxString& storageName);
    wxString getFilename(Position item);
    wxString storageNameM;
    Position sizeM;

public:
    // copy ctor needed for std:: containers
    StatementHistory(const StatementHistory& source);

    //! reads granularity from config() and gives pointer to appropriate history object
    static StatementHistory& get(Database *db);

    wxString get(Position position);
    void add(const wxString&);
    void deleteItems(const std::vector<Position>& items);
    Position size();
};
//-----------------------------------------------------------------------------
#endif
