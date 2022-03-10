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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/filename.h>
#include <wx/image.h>
#include <wx/mstream.h>

#include "config/Config.h"
#include "core/ArtProvider.h"

// these have size 32x32
#include "flamerobin.xpm"
#include "sqlicon.xpm"

#include "backup32.xpm"
#include "column32.xpm"
#include "database32.xpm"
#include "domain32.xpm"
#include "function32.xpm"
#include "generator32.xpm"
#include "index32.xpm"
#include "package32.xpm"
#include "procedure32.xpm"
#include "server32.xpm"
#include "systemdomain32.xpm"
#include "systempackage32.xpm"
#include "systemtable32.xpm"
#include "table32.xpm"
#include "toggle32.xpm"
#include "trigger32.xpm"
#include "view32.xpm"

// these have size 24x24
#include "delete24.xpm"
#include "insert24.xpm"
#include "history24.xpm"
#include "execute24.xpm"
#include "ok24.xpm"
#include "plan24.xpm"
#include "redx24.xpm"
#include "toggle24.xpm"

// these have size 16x16
#include "column.xpm"
#include "database.xpm"
#include "databaseserver.xpm"
#include "dbtrigger.xpm"
#include "dbtriggers.xpm"
#include "ddltrigger.xpm"
#include "ddltriggers.xpm"
#include "delete16.xpm"
#include "dmltrigger.xpm"
#include "dmltriggers.xpm"
#include "domain.xpm"
#include "exception16_png.cpp"
#include "execute16.xpm"
#include "fk16_png.cpp"
#include "function.xpm"
#include "functions.xpm"
#include "generator.xpm"
#include "generators.xpm"
#include "globaltemporary.xpm"
#include "globaltemporaries.xpm"
#include "history.xpm"
#include "index.xpm"
#include "indices.xpm"
#include "insert16.xpm"
#include "input.xpm"
#include "pk16_png.cpp"
#include "pkfk16_png.cpp"
#include "object.xpm"
#include "ok.xpm"
#include "output.xpm"
#include "plan16.xpm"
#include "package.xpm"
#include "packages.xpm"
#include "procedure.xpm"
#include "procedures.xpm"
#include "redx.xpm"
#include "role16_png.cpp"
#include "root.xpm"
#include "server.xpm"
#include "systemindex.xpm"
#include "systemindices.xpm"
#include "systemdomain.xpm"
#include "systemdomains.xpm"
#include "systempackage.xpm"
#include "systempackages.xpm"
#include "systemtable.xpm"
#include "systemtables.xpm"
#include "table.xpm"
#include "tables.xpm"
#include "toggle16.xpm"
#include "trigger.xpm"
#include "UDF.xpm"
#include "UDFs.xpm"
#include "user.xpm"
#include "users.xpm"
#include "view.xpm"

wxBitmap bitmapFromEmbeddedPNG(const unsigned char* data, size_t len)
{
    wxMemoryInputStream is(data, len);
    wxImage img(is);
    return wxBitmap(img);
}

wxBitmap ArtProvider::CreateBitmap(const wxArtID& id,
    const wxArtClient& client, const wxSize& size)
{
    wxBitmap loadedBmp(loadBitmapFromFile(id, size));
    if (loadedBmp.IsOk())
        return loadedBmp;

    if (id == ART_FlameRobin)
        return wxBitmap(flamerobin32_xpm);

    if (id == ART_ExecuteSqlFrame)
        return wxBitmap(sqlicon32_xpm);

    if (client == wxART_FRAME_ICON || size == wxSize(32, 32))
    {
        if (id == ART_Backup)
            return wxBitmap(backup32_xpm);
        if (id == ART_Column)
            return wxBitmap(column32_xpm);
        if (id == ART_DatabaseConnected)
            return wxBitmap(database32_xpm);
        if (id == ART_DatabaseDisconnected)
            return wxBitmap(database32_xpm);
        if (id == ART_Domain)
            return wxBitmap(domain32_xpm);
        if (id == ART_Function)
            return wxBitmap(function32_xpm);
        if (id == ART_Generator)
            return wxBitmap(generator32_xpm);
        if (id == ART_Index)
            return wxBitmap(index32_xpm);
        if (id == ART_Package)
            return wxBitmap(package32_xpm);
        if (id == ART_Procedure)
            return wxBitmap(procedure32_xpm);
        if (id == ART_Server)
            return wxBitmap(server32_xpm);
        if (id == ART_SystemDomain)
            return wxBitmap(systemdomain32_xpm);
        if (id == ART_SystemIndex)
            return wxBitmap(systemindex32_xpm);
        if (id == ART_SystemPackage)
            return wxBitmap(systempackage32_xpm);
        if (id == ART_SystemTable)
            return wxBitmap(systemtable32_xpm);
        if (id == ART_Table)
            return wxBitmap(table32_xpm);
        if (id == ART_Trigger)
            return wxBitmap(trigger32_xpm);
        if (id == ART_ToggleView)
            return wxBitmap(toggle32_xpm);
        if (id == ART_View)
            return wxBitmap(view32_xpm);
    }

    if (size == wxSize(24, 24))
    {
        if (id == ART_CommitTransaction)
            return wxBitmap(ok24_xpm);
        if (id == ART_DeleteRow)
            return wxBitmap(delete24_xpm);
        if (id == ART_ExecuteStatement)
            return wxBitmap(execute24_xpm);
        if (id == ART_History)
            return wxBitmap(history24_xpm);
        if (id == ART_InsertRow)
            return wxBitmap(insert24_xpm);
        if (id == ART_RollbackTransaction)
            return wxBitmap(redx24_xpm);
        if (id == ART_ShowExecutionPlan)
            return wxBitmap(plan24_xpm);
        if (id == ART_ToggleView)
            return wxBitmap(toggle24_xpm);
    }

    if (size == wxSize(16, 16))
    {
        if (id == ART_Column)
            return wxBitmap(column_xpm);
        if (id == ART_CommitTransaction)
            return wxBitmap(ok_xpm);
        if (id == ART_Computed)
            return wxBitmap(function_xpm);
        if (id == ART_DatabaseConnected)
            return wxBitmap(database_xpm);
        if (id == ART_DatabaseDisconnected)
            return wxBitmap(database_xpm);
        if (id == ART_DBTrigger)
            return wxBitmap(DBTrigger_xpm);
        if (id == ART_DBTriggers)
            return wxBitmap(DBTriggers_xpm);
        if (id == ART_DMLTrigger)
            return wxBitmap(DMLTrigger_xpm);
        if (id == ART_DMLTriggers)
            return wxBitmap(DMLTriggers_xpm);
        if (id == ART_DDLTrigger)
            return wxBitmap(DDLTrigger_xpm);
        if (id == ART_DDLTriggers)
            return wxBitmap(DDLTriggers_xpm);
        if (id == ART_Domain)
            return wxBitmap(domain_xpm);
        if (id == ART_Domains)
            return wxBitmap(domain_xpm);
        if (id == ART_Exception)
            return bitmapFromEmbeddedPNG(exception16_png, sizeof(exception16_png));
        if (id == ART_Exceptions)
            return bitmapFromEmbeddedPNG(exception16_png, sizeof(exception16_png));
        if (id == ART_ExecuteStatement)
            return wxBitmap(execute16_xpm);
        if (id == ART_ForeignKey)
            return bitmapFromEmbeddedPNG(fk16_png, sizeof(fk16_png));
        if (id == ART_Function)
            return wxBitmap(function_xpm);
        if (id == ART_Functions)
            return wxBitmap(functions_xpm);
        if (id == ART_Generator)
            return wxBitmap(generator_xpm);
        if (id == ART_Generators)
            return wxBitmap(generators_xpm);
        if (id == ART_GlobalTemporary)
            return wxBitmap(globaltemporary_xpm);
        if (id == ART_GlobalTemporaries)
            return wxBitmap(globaltemporaries_xpm);
        if (id == ART_History)
            return wxBitmap(history_xpm);
        if (id == ART_Object)
            return wxBitmap(object_xpm);
        if (id == ART_Output)
            return wxBitmap(output_xpm);
        if (id == ART_ParameterInput)
            return wxBitmap(input_xpm);
        if (id == ART_ParameterOutput)
            return wxBitmap(output_xpm);
        if (id == ART_PrimaryAndForeignKey)
            return bitmapFromEmbeddedPNG(pkfk16_png, sizeof(pkfk16_png));
        if (id == ART_PrimaryKey)
            return bitmapFromEmbeddedPNG(pk16_png, sizeof(pk16_png));
        if (id == ART_Package)
            return wxBitmap(package_xpm);
        if (id == ART_Packages)
            return wxBitmap(packages_xpm);
        if (id == ART_Procedure)
            return wxBitmap(procedure_xpm);
        if (id == ART_Procedures)
            return wxBitmap(procedures_xpm);
        if (id == ART_Role)
            return bitmapFromEmbeddedPNG(role16_png, sizeof(role16_png));
        if (id == ART_Roles)
            return bitmapFromEmbeddedPNG(role16_png, sizeof(role16_png));
        if (id == ART_RollbackTransaction)
            return wxBitmap(redx_xpm);
        if (id == ART_Root)
            return wxBitmap(root_xpm);
        if (id == ART_Server)
            return wxBitmap(databaseserver_xpm);
        if (id == ART_ShowExecutionPlan)
            return wxBitmap(plan16_xpm);
        if (id == ART_SystemIndex)
            return wxBitmap(systemindex32_xpm);
        if (id == ART_SystemIndices)
            return wxBitmap(systemindices_xpm);
        if (id == ART_SystemDomain)
            return wxBitmap(systemdomain_xpm);
        if (id == ART_SystemPackages)
            return wxBitmap(systempackages_xpm);
        if (id == ART_SystemPackage)
            return wxBitmap(systempackage_xpm);
        if (id == ART_SystemDomains)
            return wxBitmap(systemdomains_xpm);
        if (id == ART_SystemRole)
            return bitmapFromEmbeddedPNG(role16_png, sizeof(role16_png));
        if (id == ART_SystemRoles)
            return bitmapFromEmbeddedPNG(role16_png, sizeof(role16_png));
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
        if (id == ART_Triggers)
            return wxBitmap(trigger_xpm);
        if (id == ART_UDF)
            return wxBitmap(UDF_xpm);
        if (id == ART_UDFs)
            return wxBitmap(UDFs_xpm);
        if (id == ART_User)
            return wxBitmap(user_xpm);
        if (id == ART_Users)
            return wxBitmap(users_xpm);
        if (id == ART_View)
            return wxBitmap(view_xpm);
        if (id == ART_Views)
            return wxBitmap(view_xpm);
        if (id == ART_Index)
            return wxBitmap(index_xpm);
        if (id == ART_Input)
            return wxBitmap(input_xpm);
        if (id == ART_Indices)
            return wxBitmap(indices_xpm);
        if (id == ART_DeleteRow)
            return wxBitmap(delete16_xpm);
        if (id == ART_InsertRow)
            return wxBitmap(insert16_xpm);
        if (id == ART_ToggleView)
            return wxBitmap(toggle16_xpm);
    }
//    return wxBitmap(toggle16_xpm);
    return wxNullBitmap;
}

wxBitmap ArtProvider::loadBitmapFromFile(const wxArtID& id, wxSize size)
{
    wxString name(id.Lower());
    if (name.substr(0, 4) == "art_")
        name.erase(0, 4);
    if (size == wxDefaultSize)
        size = wxSize(32, 32);
    wxFileName fname(config().getImagesPath() + name
        + wxString::Format("_%dx%d", size.GetWidth(), size.GetHeight()));

    wxArrayString imgExts;
    imgExts.Add("png");
    imgExts.Add("xpm");
    imgExts.Add("bmp");

    for (size_t i = 0; i < imgExts.GetCount(); ++i)
    {
        fname.SetExt(imgExts[i]);
        wxLogDebug("Trying to load image file \"%s\"",
            fname.GetFullPath().c_str());
        if (fname.FileExists())
        {
            wxImage img(fname.GetFullPath());
            if (img.IsOk() && wxSize(img.GetWidth(), img.GetHeight()) == size)
                return wxBitmap(img);
        }
    }

    return wxNullBitmap;
}

