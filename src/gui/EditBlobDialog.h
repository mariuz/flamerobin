/*
  Copyright (c) 2004-2009 The FlameRobin Development Team

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

#ifndef FR_EDITBLOBDIALOG_H
#define FR_EDITBLOBDIALOG_H

#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/notebook.h>
#include <wx/stc/stc.h>
#include <wx/stream.h>
#include <wx/wx.h>

#include <set>

#include <ibpp.h>

#include "controls/DataGridTable.h"
#include "controls/DataGrid.h"
#include "gui/BaseDialog.h"
//-----------------------------------------------------------------------------
// Helper Class of wxStyledTextCtrl to handle NULL values 
class EditBlobDialogSTC : public wxStyledTextCtrl 
{
public:
    EditBlobDialogSTC(wxWindow *parent, wxWindowID id=wxID_ANY);
    
    void setIsNull(bool isNull);
    bool getIsNull() { return isNullM; }

    void ClearAll();
    void SetText(const wxString& text);
protected:
    void OnChar(wxKeyEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
private:
    bool isNullM;
};
//-----------------------------------------------------------------------------
// Main Class: EditBlobDialog
class EditBlobDialog : public BaseDialog 
{
public:
    EditBlobDialog(wxWindow* parent);
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
    
    void set_properties();
    void do_layout();
    
    // misc
    void blob_textSetReadonly(bool readonly);
    
    // Events
    void OnDataModified(wxStyledTextEvent& WXUNUSED(event));
    void OnNotebookPageChanged(wxNotebookEvent& WXUNUSED(event));
    void OnClose(wxCloseEvent& event);
    void OnResetButtonClick(wxCommandEvent& WXUNUSED(event));
    void OnSaveButtonClick(wxCommandEvent& WXUNUSED(event));
protected:
    wxNotebook* notebook;
    EditBlobDialogSTC* blob_text;
    EditBlobDialogSTC* blob_binary;
    wxStaticText* blob_noDataText;
    wxPanel* blob_noData;
    wxButton* button_reset;
    wxButton* button_save;
    DECLARE_EVENT_TABLE()
};
//-----------------------------------------------------------------------------
// Helper-Class for streaming into blob / buffer
class FRInputBlobStream : public wxInputStream
{
    public:
        FRInputBlobStream(IBPP::Blob blob);
        virtual ~FRInputBlobStream();
        virtual size_t GetSize() const;
    protected:
        virtual size_t OnSysRead(void *buffer, size_t size);          
    private:
        IBPP::Blob blobM;
        int sizeM;
};
//-----------------------------------------------------------------------------
class FROutputBlobStream : public wxOutputStream
{
    public:
        FROutputBlobStream(IBPP::Blob blob);
        virtual ~FROutputBlobStream();
        
        virtual bool Close();
    protected:
        virtual size_t OnSysWrite(const void *buffer, size_t bufsize);
    private:
        IBPP::Blob blobM;
};
//-----------------------------------------------------------------------------
#endif // FR_EDITBLOBDIALOG_H
