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
#include "characterset.xpm"
#include "charactersets.xpm"
#include "column.xpm"
#include "collation.xpm"
#include "collations.xpm"
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
    wxBitmapBundle bundle = CreateBitmapBundle(id, client, size);
    if (bundle.IsOk())
        return bundle.GetBitmap(size);
    return wxNullBitmap;
}

wxBitmapBundle ArtProvider::loadBitmapBundleFromFile(const wxArtID& id)
{
    wxString name(id.Lower());
    if (name.substr(0, 4) == "art_")
        name.erase(0, 4);
    else if (name.substr(0, 6) == "wxart_")
        name.erase(0, 6);

    // Try SVG first
    wxFileName svgName(config().getImagesPath() + "svg/" + name + ".svg");
    if (svgName.FileExists())
    {
        // For SVGs, we don't need to specify a size here, wxBitmapBundle 
        // will use the SVG to generate any size requested later.
        // We use a base size of 16x16 as a hint.
        return wxBitmapBundle::FromSVGFile(svgName.GetFullPath(), wxSize(16, 16));
    }

    // Try traditional multi-size PNGs if SVG is not found
    // (naming convention: iconname_16x16.png, iconname_32x32.png, etc.)
    // For now, let's just return a bundle if we find at least one.
    // wxBitmapBundle will automatically pick the best match if we provide multiple.
    
    // Fallback to existing loadBitmapFromFile for individual files
    // This is less efficient than a real bundle but maintains compatibility.
    wxBitmap bmp16 = loadBitmapFromFile(id, wxSize(16, 16));
    wxBitmap bmp24 = loadBitmapFromFile(id, wxSize(24, 24));
    wxBitmap bmp32 = loadBitmapFromFile(id, wxSize(32, 32));

    wxVector<wxBitmap> bitmaps;
    if (bmp16.IsOk()) bitmaps.push_back(bmp16);
    if (bmp24.IsOk()) bitmaps.push_back(bmp24);
    if (bmp32.IsOk()) bitmaps.push_back(bmp32);

    if (!bitmaps.empty())
        return wxBitmapBundle::FromBitmaps(bitmaps);

    return wxBitmapBundle();
}

wxBitmapBundle ArtProvider::CreateBitmapBundle(const wxArtID& id,
    const wxArtClient& client, const wxSize& WXUNUSED(size))
{
    wxBitmapBundle loadedBundle = loadBitmapBundleFromFile(id);
    if (loadedBundle.IsOk())
        return loadedBundle;

    // Special case for FlameRobin icon
    if (id == ART_FlameRobin)
        return wxBitmapBundle::FromBitmap(wxBitmap(flamerobin32_xpm));

    if (id == ART_ExecuteSqlFrame)
        return wxBitmapBundle::FromBitmap(wxBitmap(sqlicon32_xpm));

    // Map ART IDs to embedded XPMs (wrapping them in bundles)
    // We can provide both 16x16 and 32x32 versions if available
    auto fromXPM = [](const char* const* xpm16, const char* const* xpm32 = nullptr) {
        if (xpm32)
        {
            wxVector<wxBitmap> v;
            v.push_back(wxBitmap(xpm16));
            v.push_back(wxBitmap(xpm32));
            return wxBitmapBundle::FromBitmaps(v);
        }
        return wxBitmapBundle::FromBitmap(wxBitmap(xpm16));
    };

    if (id == ART_Backup) return fromXPM(database_xpm, backup32_xpm);
    if (id == ART_CharacterSet) return fromXPM(characterset_xpm);
    if (id == ART_CharacterSets) return fromXPM(charactersets_xpm);
    if (id == ART_Column) return fromXPM(column_xpm, column32_xpm);
    if (id == ART_Collation) return fromXPM(collation_xpm);
    if (id == ART_Collations) return fromXPM(collations_xpm);
    if (id == ART_CommitTransaction)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(ok_xpm));
        v.push_back(wxBitmap(ok24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_Computed) return fromXPM(function_xpm);
    if (id == ART_DatabaseConnected || id == ART_DatabaseDisconnected)
        return fromXPM(database_xpm, database32_xpm);
    if (id == ART_DatabaseServer || id == ART_Server)
        return fromXPM(databaseserver_xpm, server32_xpm);
    if (id == ART_DBTrigger) return fromXPM(DBTrigger_xpm, trigger32_xpm);
    if (id == ART_DBTriggers) return fromXPM(DBTriggers_xpm, trigger32_xpm);
    if (id == ART_DMLTrigger) return fromXPM(DMLTrigger_xpm, trigger32_xpm);
    if (id == ART_DMLTriggers) return fromXPM(DMLTriggers_xpm, trigger32_xpm);
    if (id == ART_DDLTrigger) return fromXPM(DDLTrigger_xpm, trigger32_xpm);
    if (id == ART_DDLTriggers) return fromXPM(DDLTriggers_xpm, trigger32_xpm);
    if (id == ART_Domain) return fromXPM(domain_xpm, domain32_xpm);
    if (id == ART_Domains) return fromXPM(domain_xpm, domain32_xpm);
    if (id == ART_Exception || id == ART_Exceptions)
        return wxBitmapBundle::FromBitmap(bitmapFromEmbeddedPNG(exception16_png, sizeof(exception16_png)));
    if (id == ART_ExecuteStatement)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(execute16_xpm));
        v.push_back(wxBitmap(execute24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_ForeignKey)
        return wxBitmapBundle::FromBitmap(bitmapFromEmbeddedPNG(fk16_png, sizeof(fk16_png)));
    if (id == ART_Function) return fromXPM(function_xpm, function32_xpm);
    if (id == ART_Functions) return fromXPM(functions_xpm, function32_xpm);
    if (id == ART_Generator) return fromXPM(generator_xpm, generator32_xpm);
    if (id == ART_Generators) return fromXPM(generators_xpm, generator32_xpm);
    if (id == ART_GlobalTemporary || id == ART_GlobalTemporaries)
        return fromXPM(globaltemporary_xpm);
    if (id == ART_History || id == ART_ShowProfiler)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(history_xpm));
        v.push_back(wxBitmap(history24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_Object) return fromXPM(object_xpm);
    if (id == ART_Output) return fromXPM(output_xpm);
    if (id == ART_ParameterInput || id == ART_Input) return fromXPM(input_xpm);
    if (id == ART_ParameterOutput) return fromXPM(output_xpm);
    if (id == ART_PrimaryAndForeignKey)
        return wxBitmapBundle::FromBitmap(bitmapFromEmbeddedPNG(pkfk16_png, sizeof(pkfk16_png)));
    if (id == ART_PrimaryKey)
        return wxBitmapBundle::FromBitmap(bitmapFromEmbeddedPNG(pk16_png, sizeof(pk16_png)));
    if (id == ART_Package) return fromXPM(package_xpm, package32_xpm);
    if (id == ART_Packages) return fromXPM(packages_xpm, package32_xpm);
    if (id == ART_Procedure) return fromXPM(procedure_xpm, procedure32_xpm);
    if (id == ART_Procedures) return fromXPM(procedures_xpm, procedure32_xpm);
    if (id == ART_Publication) return fromXPM(table_xpm, table32_xpm);
    if (id == ART_Publications) return fromXPM(tables_xpm, table32_xpm);
    if (id == ART_Replication) return fromXPM(databaseserver_xpm, server32_xpm);
    if (id == ART_Role || id == ART_Roles || id == ART_SystemRole || id == ART_SystemRoles)
        return wxBitmapBundle::FromBitmap(bitmapFromEmbeddedPNG(role16_png, sizeof(role16_png)));
    if (id == ART_RollbackTransaction)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(redx_xpm));
        v.push_back(wxBitmap(redx24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_Root) return fromXPM(root_xpm);
    if (id == ART_ShowExecutionPlan || id == ART_Explain)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(plan16_xpm));
        v.push_back(wxBitmap(plan24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_SystemIndex) return fromXPM(systemindex_xpm, systemindex32_xpm);
    if (id == ART_SystemIndices) return fromXPM(systemindices_xpm, systemindex32_xpm);
    if (id == ART_SystemDomain) return fromXPM(systemdomain_xpm, systemdomain32_xpm);
    if (id == ART_SystemDomains) return fromXPM(systemdomains_xpm, systemdomain32_xpm);
    if (id == ART_SystemPackage) return fromXPM(systempackage_xpm, systempackage32_xpm);
    if (id == ART_SystemPackages) return fromXPM(systempackages_xpm, systempackage32_xpm);
    if (id == ART_SystemTable) return fromXPM(systemtable_xpm, systemtable32_xpm);
    if (id == ART_SystemTables) return fromXPM(systemtables_xpm, systemtable32_xpm);
    if (id == ART_Table) return fromXPM(table_xpm, table32_xpm);
    if (id == ART_Tables) return fromXPM(tables_xpm, table32_xpm);
    if (id == ART_Trigger) return fromXPM(trigger_xpm, trigger32_xpm);
    if (id == ART_Triggers) return fromXPM(trigger_xpm, trigger32_xpm);
    if (id == ART_UDF) return fromXPM(UDF_xpm);
    if (id == ART_UDFs) return fromXPM(UDFs_xpm);
    if (id == ART_User || id == ART_Users) return fromXPM(user_xpm);
    if (id == ART_View) return fromXPM(view_xpm, view32_xpm);
    if (id == ART_Views) return fromXPM(view_xpm, view32_xpm);
    if (id == ART_Index) return fromXPM(index_xpm, index32_xpm);
    if (id == ART_Indices) return fromXPM(indices_xpm, index32_xpm);
    if (id == ART_DeleteRow)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(delete16_xpm));
        v.push_back(wxBitmap(delete24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_InsertRow)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(insert16_xpm));
        v.push_back(wxBitmap(insert24_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }
    if (id == ART_ToggleView)
    {
        wxVector<wxBitmap> v;
        v.push_back(wxBitmap(toggle16_xpm));
        v.push_back(wxBitmap(toggle24_xpm));
        v.push_back(wxBitmap(toggle32_xpm));
        return wxBitmapBundle::FromBitmaps(v);
    }

    return wxBitmapBundle();
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

