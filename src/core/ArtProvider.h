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

#ifndef FR_ARTPROVIDER_H
#define FR_ARTPROVIDER_H

#include <wx/artprov.h>

#define ART_FlameRobin              wxART_MAKE_ART_ID(ART_FlameRobin)
#define ART_ExecuteSqlFrame         wxART_MAKE_ART_ID(ART_ExecuteSqlFrame)

#define ART_Backup                  wxART_MAKE_ART_ID(ART_Backup)
#define ART_CharacterSet            wxART_MAKE_ART_ID(ART_CharacterSet)
#define ART_CharacterSets           wxART_MAKE_ART_ID(ART_CharacterSets)
#define ART_Column                  wxART_MAKE_ART_ID(ART_Column)
#define ART_Collation               wxART_MAKE_ART_ID(ART_Collation)
#define ART_Collations              wxART_MAKE_ART_ID(ART_Ccollations)
#define ART_Computed                wxART_MAKE_ART_ID(ART_Computed)
#define ART_DatabaseConnected       wxART_MAKE_ART_ID(ART_DatabaseConnected)
#define ART_DatabaseDisconnected    wxART_MAKE_ART_ID(ART_DatabaseDisconnected)
#define ART_DatabaseServer          wxART_MAKE_ART_ID(ART_DatabaseServer)
#define ART_DBTrigger               wxART_MAKE_ART_ID(ART_DBTrigger)
#define ART_DBTriggers              wxART_MAKE_ART_ID(ART_DBTriggers)
#define ART_DMLTrigger              wxART_MAKE_ART_ID(ART_DMLTrigger)
#define ART_DMLTriggers             wxART_MAKE_ART_ID(ART_DMLTriggers)
#define ART_DDLTrigger              wxART_MAKE_ART_ID(ART_DDLTrigger)
#define ART_DDLTriggers             wxART_MAKE_ART_ID(ART_DDLTriggers)
#define ART_Domain                  wxART_MAKE_ART_ID(ART_Domain)
#define ART_Domains                 wxART_MAKE_ART_ID(ART_Domains)
#define ART_Exception               wxART_MAKE_ART_ID(ART_Exception)
#define ART_Exceptions              wxART_MAKE_ART_ID(ART_Exceptions)
#define ART_ForeignKey              wxART_MAKE_ART_ID(ART_ForeignKey)
#define ART_Function                wxART_MAKE_ART_ID(ART_Function)
#define ART_Functions               wxART_MAKE_ART_ID(ART_Functions)
#define ART_Generator               wxART_MAKE_ART_ID(ART_Generator)
#define ART_Generators              wxART_MAKE_ART_ID(ART_Generators)
#define ART_GlobalTemporary         wxART_MAKE_ART_ID(ART_GlobalTemporary)
#define ART_GlobalTemporaries       wxART_MAKE_ART_ID(ART_GlobalTemporaries)
#define ART_Index                   wxART_MAKE_ART_ID(ART_Index)
#define ART_Indices                 wxART_MAKE_ART_ID(ART_Indices)
#define ART_Input                   wxART_MAKE_ART_ID(ART_Input)
#define ART_Object                  wxART_MAKE_ART_ID(ART_Object)
#define ART_Output                  wxART_MAKE_ART_ID(ART_Output)
#define ART_Package                 wxART_MAKE_ART_ID(ART_Package)
#define ART_Packages                wxART_MAKE_ART_ID(ART_Packages)
#define ART_ParameterInput          wxART_MAKE_ART_ID(ART_ParameterInput)
#define ART_ParameterOutput         wxART_MAKE_ART_ID(ART_ParameterOutput)
#define ART_PrimaryKey              wxART_MAKE_ART_ID(ART_PrimaryKey)
#define ART_PrimaryAndForeignKey    wxART_MAKE_ART_ID(ART_PrimaryAndForeignKey)
#define ART_Procedure               wxART_MAKE_ART_ID(ART_Procedure)
#define ART_Procedures              wxART_MAKE_ART_ID(ART_Procedures)
#define ART_Role                    wxART_MAKE_ART_ID(ART_Role)
#define ART_Roles                   wxART_MAKE_ART_ID(ART_Roles)
#define ART_Root                    wxART_MAKE_ART_ID(ART_Root)
#define ART_Server                  wxART_MAKE_ART_ID(ART_Server)
#define ART_SystemIndex             wxART_MAKE_ART_ID(ART_SystemIndex)
#define ART_SystemIndices           wxART_MAKE_ART_ID(ART_SystemIndices)
#define ART_SystemDomain            wxART_MAKE_ART_ID(ART_SystemDomain)
#define ART_SystemDomains           wxART_MAKE_ART_ID(ART_SystemDomains)
#define ART_SystemPackage           wxART_MAKE_ART_ID(ART_SystemPackage)
#define ART_SystemPackages          wxART_MAKE_ART_ID(ART_SystemPackages)
#define ART_SystemRole              wxART_MAKE_ART_ID(ART_SystemRole)
#define ART_SystemRoles             wxART_MAKE_ART_ID(ART_SystemRoles)
#define ART_SystemTable             wxART_MAKE_ART_ID(ART_SystemTable)
#define ART_SystemTables            wxART_MAKE_ART_ID(ART_SystemTables)
#define ART_Table                   wxART_MAKE_ART_ID(ART_Table)
#define ART_Tables                  wxART_MAKE_ART_ID(ART_Tables)
#define ART_Trigger                 wxART_MAKE_ART_ID(ART_Trigger)
#define ART_Triggers                wxART_MAKE_ART_ID(ART_Triggers)
#define ART_UDF                     wxART_MAKE_ART_ID(ART_UDF)
#define ART_UDFs                    wxART_MAKE_ART_ID(ART_UDFs)
#define ART_User                    wxART_MAKE_ART_ID(ART_User)
#define ART_Users                   wxART_MAKE_ART_ID(ART_Users)
#define ART_View                    wxART_MAKE_ART_ID(ART_View)
#define ART_Views                   wxART_MAKE_ART_ID(ART_Views)

#define ART_DeleteRow               wxART_MAKE_ART_ID(ART_DeleteRow)
#define ART_InsertRow               wxART_MAKE_ART_ID(ART_InsertRow)
#define ART_ToggleView              wxART_MAKE_ART_ID(ART_ToggleView)

#define ART_History                 wxART_MAKE_ART_ID(ART_History)
#define ART_ExecuteStatement        wxART_MAKE_ART_ID(ART_ExecuteStatement)
#define ART_ShowExecutionPlan       wxART_MAKE_ART_ID(ART_ShowExecutionPlan)
#define ART_CommitTransaction       wxART_MAKE_ART_ID(ART_CommitTransaction)
#define ART_RollbackTransaction     wxART_MAKE_ART_ID(ART_RollbackTransaction)

class ArtProvider : public wxArtProvider
{
private:
    wxBitmap loadBitmapFromFile(const wxArtID& id, wxSize size);
protected:
    virtual wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client,
        const wxSize& size);
};

#endif // FR_ARTPROVIDER_H
