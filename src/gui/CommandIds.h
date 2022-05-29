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
#ifndef FR_COMMANDIDS_H
#define FR_COMMANDIDS_H

namespace Cmds
{
    enum {
        // SQL View:
        View_Editor = 401,
        View_Statistics,
        View_Data,
        View_SplitView,
        View_Wrap_long_lines,
        View_Set_editor_font,
        Find_Selected_Object,

        // SQL History
        History_Search,
        History_EnableLogging,

        // SQL Query
        Query_Execute,
        Query_Show_plan,
        Query_Execute_selection,
        Query_Execute_from_cursor,
        Query_Commit,
        Query_Rollback,
        // next 4: order is important, because EVT_MENU_RANGE is used
        Query_TransactionConcurrency,
        Query_TransactionReadDirty,
        Query_TransactionReadCommitted,
        Query_TransactionConsistency,
        Query_TransactionLockResolution,
        Query_TransactionReadOnly,

        // SQL Data grid
        DataGrid_Insert_row,
        DataGrid_Delete_row,
        DataGrid_SetFieldToNULL,
        DataGrid_FetchAll,
        DataGrid_CancelFetchAll,

        DataGrid_EditBlob,
        DataGrid_ExportBlob,
        DataGrid_ImportBlob,
        DataGrid_Copy_with_header,
        DataGrid_Copy_as_insert,
        DataGrid_Copy_as_inList,
        DataGrid_Copy_as_update,
        DataGrid_Save_as_html,
        DataGrid_Save_as_csv,
        DataGrid_Set_header_font,
        DataGrid_Set_cell_font,
        DataGrid_Log_changes,

        Menu_RegisterServer = 600, 
        Menu_Manual, 
        Menu_RelNotes, 
        Menu_License,
        Menu_URLHomePage, 
        Menu_URLProjectPage, 
        Menu_URLFeatureRequest,
        Menu_URLBugReport,
        Menu_NewObject, 
        Menu_DatabaseRegistrationInfo, 
        Menu_RegisterDatabase,
        Menu_CreateDatabase, 
        Menu_ManageUsers, 
        Menu_UnRegisterServer,
        Menu_ServerProperties, 
        Menu_Reconnect, 
        Menu_ConnectAs,
        Menu_ExecuteProcedure, 
        Menu_UnRegisterDatabase, 
        Menu_Backup,
        Menu_Restore, 
        Menu_Connect, 
        Menu_Disconnect, 
        Menu_ExecuteStatements,
        Menu_CreateObject, 
        Menu_DatabasePreferences,
        Menu_ShowAllGeneratorValues, 
        Menu_BrowseData,
        Menu_ObjectProperties, 
        Menu_ObjectRefresh, 
        Menu_DropObject,
        Menu_ShowGeneratorValue, 
        Menu_SetGeneratorValue,
        Menu_AddColumn, 
        Menu_RestoreIntoNew,
        Menu_MonitorEvents, 
        Menu_GetServerVersion, 
        Menu_AlterObject,
        Menu_DropDatabase, 
        Menu_RecreateDatabase, 
        Menu_DatabaseProperties,
        Menu_GenerateData, 
        Menu_CloneDatabase,
        Menu_ExecuteFunction,
        Menu_ShowStatisticsValue,
        Menu_SetStatisticsValue,
        Menu_ShowAllStatisticsValue,
        Menu_RebuildObject,
        Menu_ActiveObject,
        Menu_InactiveObject,

        // view menu
        Menu_ToggleStatusBar, 
        Menu_ToggleSearchBar, 
        Menu_ToggleDisconnected,

        // create new ... (stuff)
        Menu_CreateDBTrigger,
        Menu_CreateDDLTrigger,
        Menu_CreateDMLTrigger,
        Menu_CreateDomain,
        Menu_CreateException, 
        Menu_CreateFunction,
        Menu_CreateGTTTable,
        Menu_CreateGenerator, 
        Menu_CreateIndex,
        Menu_CreatePackage,
        Menu_CreateProcedure,
        Menu_CreateRole,
        Menu_CreateTable,         
        Menu_CreateUDF,
        Menu_CreateUser,
        Menu_CreateView,

        // blob editor
        BlobEditor_ChangeLineBreak,
        BlobEditor_Menu_BLOB,
        BlobEditor_Menu_BLOBSaveToFile,
        BlobEditor_Menu_BLOBLoadFromFile,
        BlobEditor_ProgressCancel,

        // 100 templates
        Menu_TemplateFirst = 700, 
        Menu_TemplateLast = 799,
        Menu_TemplateMenu,

        // for easier copy/paste of above items (no need to mess with comma)
        Last_menu
    };
};

#endif
