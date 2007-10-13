/*
  Copyright (c) 2004-2007 The FlameRobin Development Team

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

#include "core/ArtProvider.h"
//-----------------------------------------------------------------------------
// these have size 32x32
#include "flamerobin.xpm"

#include "backup.xpm"
#include "column32.xpm"
#include "database32.xpm"
#include "domain32.xpm"
#include "function32.xpm"
#include "generator32.xpm"
#include "procedure32.xpm"
#include "server32.xpm"
#include "systemtable32.xpm"
#include "table32.xpm"
#include "trigger32.xpm"
#include "view32.xpm"

// these have size 24x24
#include "delete24.xpm"
#include "insert24.xpm"

// these have size 16x16
#include "column.xpm"
#include "database.xpm"
#include "delete16.xpm"
#include "domain.xpm"
#include "function.xpm"
#include "generator.xpm"
#include "generators.xpm"
#include "insert16.xpm"
#include "key.xpm"
#include "object.xpm"
#include "procedure.xpm"
#include "procedures.xpm"
#include "root.xpm"
#include "server.xpm"
#include "systemtable.xpm"
#include "systemtables.xpm"
#include "table.xpm"
#include "tables.xpm"
#include "trigger.xpm"
#include "view.xpm"
//-----------------------------------------------------------------------------
wxBitmap ArtProvider::CreateBitmap(const wxArtID& id,
    const wxArtClient& client, const wxSize& size)
{
    if (id == ART_FlameRobin)
        return wxBitmap(flamerobin_xpm);

    if (client == wxART_FRAME_ICON || size == wxSize(32, 32))
    {
        if (id == ART_Backup)
            return wxBitmap(backup32_xpm);
        if (id == ART_Column)
            return wxBitmap(column32_xpm);
        if (id == ART_Database)
            return wxBitmap(database32_xpm);
        if (id == ART_Domain)
            return wxBitmap(domain32_xpm);
        if (id == ART_Function)
            return wxBitmap(function32_xpm);
        if (id == ART_Generator)
            return wxBitmap(generator32_xpm);
        if (id == ART_Procedure)
            return wxBitmap(procedure32_xpm);
        if (id == ART_Server)
            return wxBitmap(server32_xpm);
        if (id == ART_SystemTable)
            return wxBitmap(systemtable32_xpm);
        if (id == ART_Table)
            return wxBitmap(table32_xpm);
        if (id == ART_Trigger)
            return wxBitmap(trigger32_xpm);
        if (id == ART_View)
            return wxBitmap(view32_xpm);
    }

    if (size == wxSize(24, 24))
    {
        if (id == ART_DeleteRow)
            return wxBitmap(delete24_xpm);
        if (id == ART_InsertRow)
            return wxBitmap(insert24_xpm);
    }

    if (size == wxSize(16, 16))
    {
        if (id == ART_Column)
            return wxBitmap(column_xpm);
        if (id == ART_Database)
            return wxBitmap(database_xpm);
        if (id == ART_Domain)
            return wxBitmap(domain_xpm);
        if (id == ART_Function)
            return wxBitmap(function_xpm);
        if (id == ART_Generator)
            return wxBitmap(generator_xpm);
        if (id == ART_Generators)
            return wxBitmap(generators_xpm);
        if (id == ART_Object)
            return wxBitmap(object_xpm);
        if (id == ART_PrimaryKey)
            return wxBitmap(key_xpm);
        if (id == ART_Procedure)
            return wxBitmap(procedure_xpm);
        if (id == ART_Procedures)
            return wxBitmap(procedures_xpm);
        if (id == ART_Root)
            return wxBitmap(root_xpm);
        if (id == ART_Server)
            return wxBitmap(server_xpm);
        if (id == ART_SystemTable)
            return wxBitmap(systemtable_xpm);
        if (id == ART_SystemTables)
            return wxBitmap(systemtables_xpm);
        if (id == ART_Table)
            return wxBitmap(table_xpm);
        if (id == ART_Tables)
            return wxBitmap(tables_xpm);
        if (id == ART_Trigger)
            return wxBitmap(trigger_xpm);
        if (id == ART_View)
            return wxBitmap(view_xpm);

        if (id == ART_DeleteRow)
            return wxBitmap(delete16_xpm);
        if (id == ART_InsertRow)
            return wxBitmap(insert16_xpm);
    }
    return wxNullBitmap;
}
//-----------------------------------------------------------------------------
