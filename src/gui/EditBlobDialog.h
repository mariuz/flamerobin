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

#ifndef FR_EDITBLOBDIALOG_H
#define FR_EDITBLOBDIALOG_H

#include <wx/mstream.h>
#include <wx/notebook.h>
#include <wx/stc/stc.h>
#include <wx/wx.h>

#include <set>

#include "controls/DataGrid.h"
#include "gui/BaseDialog.h"
#include "gui/CommandManager.h"


class EditBlobDialogProgressSizer; // declared in cpp
class EditBlobDialogSTCText; // declared in cpp
class EditBlobDialogSTC;     // declared in cpp

// Main Class: EditBlobDialog
class EditBlobDialog : public BaseDialog
{
public:
    EditBlobDialog(wxWindow* parent, wxMBConv* converterM);
    virtual ~EditBlobDialog();
    // close without saving (for rollback-transaction)
    void closeDontSave(); 
    // DataGrid is needed to update a blob-cell due to blob is saved
    bool setBlob(DataGrid* dg, DataGridTable* dgt, 
        IBPP::Statement* st, unsigned row, unsigned col, bool saveOldValue = true);
private:
    enum EditorMode { noData = wxID_HIGHEST+1, 
                      binary = wxID_HIGHEST+2, 
                      text = wxID_HIGHEST+3 };

    IBPP::Blob blobM;
    DataGridTable* dataGridTableM;
    DataGrid* dataGridM;
    wxString dialogCaptionM;
    EditorMode editorModeM;
    wxString fieldNameM;
    unsigned colM;
    unsigned rowM;
    bool runningM;
    IBPP::Statement* statementM; 
    wxMBConv* converterM;  //TODO: if possible, use directly from the database object

    std::set<EditorMode> dataValidM;
    wxMemoryOutputStream* cacheM;
    bool cacheIsNullM;
    bool dataModifiedM;
    bool loadingM;
    bool readonlyM;
    /*
    // activate later if plugin will be implemented
    // Dialog-Plugin-lib
    static int m_libEditBlobUseCount;
    static wxDynamicLibrary* m_libEditBlob;
    */
    
    // Switching Editor-Mode 
    // B = Binary
    // T = Text
    // H = HTML
    // I = Image (bmp/jpg/...)
    // R = RTF (Win only)
    // 
    // o = allowed
    // x = not allowed (e.g. makes no sence)
    //
    // X B T H I R
    // B - o o o o
    // T o - o x o
    // H o o - x x
    // I o x x - x
    // R o o x x -

    // cache/data-functions
    void cacheDelete();
    void dataSetModified(bool value, EditorMode editorMode);
    void dataUpdateGUI();
    // notebook-functions
    void notebookAddPageById(int pageId);
    int notebookGetPageIndexById(int pageId);
    void notebookRemovePageById(int pageId);
    void notebookSelectPageById(int pageId);
    // Loading - (calls LoadFromStreamAsXXXX)
    bool loadBlob();
    // Loading (Blob/Stream)
    bool loadFromStreamAsBinary(wxInputStream& stream, bool isNull, const wxString& progressTitle);
    bool loadFromStreamAsText(wxInputStream& stream, bool isNull, const wxString& progressTitle);
    // Saving - (calls SaveToStream)
    void saveBlob();
    // Saving (Blob/Stream)
    bool saveToStream(wxOutputStream& stream, bool* isNull, const wxString& progressTitle);

    // initialization
    void buildMenus(CommandManager& cm);
    void do_layout();
    void set_properties();
    
    // progress
    void progressBegin(const wxString& progressTitle, int maxPosition, bool canCancel);
    void progressCancel();
    void progressEnd();
    
    // misc
    void blob_textSetReadonly(bool readonly);
    
    // Events
    void OnClose(wxCloseEvent& event);
    void OnDataModified(wxStyledTextEvent& WXUNUSED(event));
    void OnMenuBLOBButtonClick(wxCommandEvent& WXUNUSED(event));
    void OnMenuBLOBLoadFromFile(wxCommandEvent& WXUNUSED(event));
    void OnMenuBLOBSaveToFile(wxCommandEvent& WXUNUSED(event));
    void OnNotebookPageChanged(wxNotebookEvent& WXUNUSED(event));
    void OnProgressCancel(wxCommandEvent& WXUNUSED(event));
    void OnResetButtonClick(wxCommandEvent& WXUNUSED(event));
    void OnSaveButtonClick(wxCommandEvent& WXUNUSED(event));
protected:
    wxNotebook* notebook;
    EditBlobDialogSTCText* blob_text;
    EditBlobDialogSTC* blob_binary;
    wxPanel* blob_noData;
    wxStaticText* blob_noDataText;
    EditBlobDialogProgressSizer* progress;
    wxButton* button_reset;
    wxButton* button_save;
    wxButton* button_menu_blob;
    wxMenu* menu_blob;
    DECLARE_EVENT_TABLE()
};

#endif // FR_EDITBLOBDIALOG_H
