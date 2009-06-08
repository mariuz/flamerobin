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
#include "gui/BaseDialog.h"

class EditBlobDialog : public BaseDialog {
public:
    EditBlobDialog(wxWindow* parent, wxString& blobName, IBPP::Blob blob,
        DataGridTable* dgt, unsigned row, unsigned col);
    virtual ~EditBlobDialog();
    bool Init();
private:
    typedef enum EditorMode { Binary = wxID_HIGHEST+1, Text = wxID_HIGHEST+2 };

    DataGridTable* dataGridTableM;
    wxString blobNameM;
    unsigned rowM;
    unsigned colM;
    IBPP::Blob blobM;
    EditorMode editorModeM;
    bool runningM;
    wxString dialogCaptionM;
    
    std::set<EditorMode> dataValidM;
    wxMemoryOutputStream* cacheM;
    bool dataModifiedM;
    bool loadingM;
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
    
    // Loading (Blob/Stream)
    bool LoadFromStreamAsBinary(wxInputStream& stream, const wxString& progressTitle);
    bool LoadFromStreamAsText(wxInputStream& stream, const wxString& progressTitle);
    // Saving (Blob/Stream)
    bool SaveToStream(wxOutputStream& stream, const wxString& progressTitle);
    
    void SetDataModified(bool value);
    void set_properties();
    void do_layout();
    // Events
    void OnSaveButtonClick(wxCommandEvent& event);
    void OnNotebookPageChanged(wxNotebookEvent& event);
    void OnDataModified(wxStyledTextEvent& event);
protected:
    wxNotebook* notebook;
    wxStyledTextCtrl* blob_text;
    wxStyledTextCtrl* blob_binary;
    wxButton* button_save;
    wxButton* button_cancel;
    DECLARE_EVENT_TABLE()
};

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
class FROutputBlobStream : public wxOutputStream
{
    public:
        FROutputBlobStream(IBPP::Blob blob);
        virtual ~FROutputBlobStream();
    protected:
        virtual size_t OnSysWrite(const void *buffer, size_t bufsize);
    private:
        IBPP::Blob blobM;
};

#endif // FR_EDITBLOBDIALOG_H
