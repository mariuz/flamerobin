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

#ifndef EXECUTESQL_H
#define EXECUTESQL_H

// These functions are used to show (and execute) sql statements
// they are separated here since:
// a) it is used in many places and improves compilation time a lot
// b) it allows us to change the way it is done easily
#include "metadata/MetadataClasses.h"

class ExecuteSqlFrame;

ExecuteSqlFrame* showSql(wxWindow* parent, const wxString& title,
    DatabasePtr database, const wxString &sql);
void execSql(wxWindow* parent, const wxString& title, DatabasePtr database,
    const wxString &sql, bool closeWindow);

#endif // EXECUTESQL_H
