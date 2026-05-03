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

#include <wx/wx.h>
#include <iostream>
#include "metadata/publication.h"

// Simple test framework mock for FlameRobin
bool check(bool condition, const wxString& message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message.mb_str() << std::endl;
        return false;
    }
    return true;
}

int main()
{
    // Initialize wx
    wxInitializer initializer;

    bool ok = true;

    // We can't easily create a full Database object without a backend,
    // so we test the logic that doesn't strictly depend on a live connection.
    
    // Create a publication with no database (it will have null database pointer)
    // and manually set its properties to verify they are stored and retrieved correctly.
    
    // Note: Publication constructor needs DatabasePtr. We'll pass an empty one.
    DatabasePtr db;
    Publication pub(db, "MY_PUB");
    
    ok = check(pub.getName_() == "MY_PUB", "Publication name") && ok;
    ok = check(pub.getType() == ntPublication, "Publication type") && ok;
    ok = check(pub.getTypeName() == "PUBLICATION", "Publication type name") && ok;
    
    pub.setAllTables(true);
    ok = check(pub.getAllTables() == true, "All tables property") && ok;
    
    wxArrayString tables;
    tables.Add("TABLE1");
    tables.Add("TABLE2");
    pub.setTables(tables);
    pub.setAllTables(false);
    
    ok = check(pub.getAllTables() == false, "All tables property false") && ok;
    wxArrayString retrievedTables = pub.getTables();
    ok = check(retrievedTables.GetCount() == 2, "Tables count") && ok;
    ok = check(retrievedTables[0] == "TABLE1", "Table 1 name") && ok;
    ok = check(retrievedTables[1] == "TABLE2", "Table 2 name") && ok;

    if (ok)
    {
        std::cout << "All Publication tests PASSED" << std::endl;
        return 0;
    }
    else
    {
        return 1;
    }
}
