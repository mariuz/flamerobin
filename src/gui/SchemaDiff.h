/*
  Copyright (c) 2004-2026 The FlameRobin Development Team

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

#ifndef FR_SCHEMADIFF_H
#define FR_SCHEMADIFF_H

#include <wx/wx.h>
#include <vector>
#include "metadata/database.h"

struct SchemaDiffOptions
{
    bool compareTables = true;
    bool compareColumns = true;
    bool compareIndices = true;
    bool compareViews = true;
    bool compareProcedures = true;
    bool compareTriggers = true;
    bool compareDomains = true;
    bool compareGenerators = true;
    bool compareExceptions = true;
    bool generateDropStatements = false;
};

struct SchemaDiffItem
{
    enum DifferenceType {
        diffMissingInTarget,
        diffExtraInTarget,
        diffModified
    };

    wxString objectType;
    wxString objectName;
    DifferenceType diffType;
    wxString description;
    wxString migrationSql;
};

class SchemaDiff
{
public:
    static std::vector<SchemaDiffItem> compareDatabases(
        Database* sourceDb, Database* targetDb, const SchemaDiffOptions& options);

    static wxString generateMigrationScript(
        const std::vector<SchemaDiffItem>& diffItems, Database* sourceDb, Database* targetDb);
};

#endif
