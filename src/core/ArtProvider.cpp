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

#include "core/EmbeddedSVGs.h"

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

    // Map ART IDs to embedded SVGs
    auto fromSVG = [](const char* svg_data) {
        return wxBitmapBundle::FromSVG(svg_data, wxSize(16, 16));
    };

    // Special case for FlameRobin icon
    if (id == ART_FlameRobin)
        return fromSVG(svg_flamerobin);

    if (id == ART_ExecuteSqlFrame)
        return fromSVG(svg_sqlicon);

    if (id == ART_Backup) return fromSVG(svg_backup);
    if (id == ART_CharacterSet) return fromSVG(svg_characterset);
    if (id == ART_CharacterSets) return fromSVG(svg_characterset);
    if (id == ART_Column) return fromSVG(svg_column);
    if (id == ART_Collation) return fromSVG(svg_collation);
    if (id == ART_Collations) return fromSVG(svg_collation);
    if (id == ART_CommitTransaction) return fromSVG(svg_committransaction);
    if (id == ART_Computed) return fromSVG(svg_function);
    if (id == ART_DatabaseConnected || id == ART_DatabaseDisconnected)
        return fromSVG(svg_database);
    if (id == ART_DatabaseServer || id == ART_Server)
        return fromSVG(svg_database);
    if (id == ART_DBTrigger) return fromSVG(svg_trigger);
    if (id == ART_DBTriggers) return fromSVG(svg_trigger);
    if (id == ART_DMLTrigger) return fromSVG(svg_trigger);
    if (id == ART_DMLTriggers) return fromSVG(svg_trigger);
    if (id == ART_DDLTrigger) return fromSVG(svg_trigger);
    if (id == ART_DDLTriggers) return fromSVG(svg_trigger);
    if (id == ART_Domain) return fromSVG(svg_domain);
    if (id == ART_Domains) return fromSVG(svg_domain);
    if (id == ART_Exception || id == ART_Exceptions)
        return fromSVG(svg_exception);
    if (id == ART_ExecuteStatement) return fromSVG(svg_executestatement);
    if (id == ART_ForeignKey) return fromSVG(svg_fk);
    if (id == ART_Function) return fromSVG(svg_function);
    if (id == ART_Functions) return fromSVG(svg_functions);
    if (id == ART_Generator) return fromSVG(svg_generator);
    if (id == ART_Generators) return fromSVG(svg_generators);
    if (id == ART_GlobalTemporary || id == ART_GlobalTemporaries)
        return fromSVG(svg_table);
    if (id == ART_History || id == ART_ShowProfiler) return fromSVG(svg_history);
    if (id == ART_Object) return fromSVG(svg_package);
    if (id == ART_Output || id == ART_ParameterOutput) return fromSVG(svg_go_back);
    if (id == ART_ParameterInput || id == ART_Input) return fromSVG(svg_go_forward);
    if (id == ART_PrimaryAndForeignKey) return fromSVG(svg_pkfk);
    if (id == ART_PrimaryKey) return fromSVG(svg_pk);
    if (id == ART_Package) return fromSVG(svg_package);
    if (id == ART_Packages) return fromSVG(svg_package);
    if (id == ART_Procedure) return fromSVG(svg_procedure);
    if (id == ART_Procedures) return fromSVG(svg_procedures);
    if (id == ART_Publication) return fromSVG(svg_table);
    if (id == ART_Publications) return fromSVG(svg_tables);
    if (id == ART_Replication) return fromSVG(svg_database);
    if (id == ART_Role || id == ART_Roles || id == ART_SystemRole || id == ART_SystemRoles)
        return fromSVG(svg_role);
    if (id == ART_RollbackTransaction) return fromSVG(svg_rollbacktransaction);
    if (id == ART_Root) return fromSVG(svg_database);
    if (id == ART_ShowExecutionPlan || id == ART_Explain) return fromSVG(svg_plan);
    if (id == ART_SystemIndex) return fromSVG(svg_index);
    if (id == ART_SystemIndices) return fromSVG(svg_indices);
    if (id == ART_SystemDomain) return fromSVG(svg_domain);
    if (id == ART_SystemDomains) return fromSVG(svg_domains);
    if (id == ART_SystemPackage) return fromSVG(svg_package);
    if (id == ART_SystemPackages) return fromSVG(svg_package);
    if (id == ART_SystemTable) return fromSVG(svg_table);
    if (id == ART_SystemTables) return fromSVG(svg_tables);
    if (id == ART_Table) return fromSVG(svg_table);
    if (id == ART_Tables) return fromSVG(svg_tables);
    if (id == ART_Trigger) return fromSVG(svg_trigger);
    if (id == ART_Triggers) return fromSVG(svg_triggers);
    if (id == ART_UDF) return fromSVG(svg_function);
    if (id == ART_UDFs) return fromSVG(svg_functions);
    if (id == ART_User || id == ART_Users) return fromSVG(svg_user);
    if (id == ART_View) return fromSVG(svg_view);
    if (id == ART_Views) return fromSVG(svg_views);
    if (id == ART_Index) return fromSVG(svg_index);
    if (id == ART_Indices) return fromSVG(svg_indices);
    if (id == ART_DeleteRow) return fromSVG(svg_redx);
    if (id == ART_InsertRow) return fromSVG(svg_new);
    if (id == ART_ToggleView) return fromSVG(svg_view);

    return wxBitmapBundle();
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

