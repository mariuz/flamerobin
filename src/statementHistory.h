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

#ifndef FR_HISTORY_H
#define FR_HISTORY_H

#include <wx/wx.h>
#include <vector>

class Database;

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
    wxDateTime getDateTime(Position position);
    void add(const wxString&);
    void deleteItems(const std::vector<Position>& items);
    Position size();
};

#endif
