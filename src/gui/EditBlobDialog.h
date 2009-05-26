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


  $Id: EditBlobDialog.h 1836 2009-05-10 11:22:13Z amaier $

*/

#ifndef FR_EDITBLOBDIALOG_H
#define FR_EDITBLOBDIALOG_H

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/stc/stc.h>
#include <wx/image.h>
#include <wx/stream.h>

#include <ibpp.h>

#include "gui/BaseDialog.h"
#include "controls/DataGridTable.h"

// Helper-Class to solve wxNotebookEvent problem
// with PAGE_CHAGING-EVENT under windows
/*class wxFRNotebook : public wxNotebook
{
    public:
        int m_newSelection;
        //virtual int SetSelection(size_t page);
        virtual int ChangeSelection(size_t page);
        
        wxFRNotebook(wxWindow *parent,
                   wxWindowID id,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = 0,
                   const wxString& name = wxNotebookNameStr)
                   : wxNotebook(parent,id,pos,size,style,name) {};
};*/

class EditBlobDialog : public BaseDialog {
public:
    EditBlobDialog(wxWindow* parent, wxString& blobName, IBPP::Blob blob,
        DataGridTable* dgt, unsigned row, unsigned col);
    virtual ~EditBlobDialog();
    bool Init();
private:
    typedef enum EditorMode { Binary = 1, Text = 2 };

    DataGridTable *m_datagridtable;
    wxString m_blobName;
    unsigned m_row;
    unsigned m_col;
    IBPP::Blob m_blob;
    EditorMode m_editormode;
    bool m_running;
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
    
    void set_properties();
    void do_layout();
    // Events
    void OnClose(wxCloseEvent& event);
    void OnSaveButtonClick(wxCommandEvent& event);
    void OnCancelButtonClick(wxCommandEvent& event);
    void OnNotebookPageChanged(wxNotebookEvent& event);
protected:
    wxNotebook* notebook;
    wxStyledTextCtrl* blob_text;
    wxStyledTextCtrl* blob_binary;
    wxButton* button_save;
    wxButton* button_cancel;
    DECLARE_EVENT_TABLE()
};

// Helper-Class for streaming into blob / buffer
class frInputBlobStream : public wxInputStream
{
    public:
        frInputBlobStream(IBPP::Blob blob);
        virtual ~frInputBlobStream();
        //virtual wxInputStream& Read(void *buffer, size_t size);
        virtual size_t GetSize() const;
    protected:
        virtual size_t OnSysRead(void *buffer, size_t size);
           
    //x wxStreamError GetLastError() const { return m_lasterror; }
    //x virtual bool IsOk() const { return GetLastError() == wxSTREAM_NO_ERROR; }
    //x bool operator!() const { return !IsOk(); }

    // reset the stream state
    //void Reset() { m_lasterror = wxSTREAM_NO_ERROR; }

    // this doesn't make sense for all streams, always test its return value
    //virtual wxFileOffset GetLength() const { return wxInvalidOffset; }

    // returns true if the streams supports seeking to arbitrary offsets
    //virtual bool IsSeekable() const { return false; }
    //protected:
    //virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    //virtual wxFileOffset OnSysTell() const;

    //size_t m_lastcount;
    //wxStreamError m_lasterror;

    private:
        IBPP::Blob m_blob;
        int m_size;
    /*class WXDLLIMPEXP_BASE wxStreamBase
{
public:
// - = überschrieben   x = nicht überschrieben

    - wxStreamBase();
    - virtual ~wxStreamBase();

    // error testing
    x wxStreamError GetLastError() const { return m_lasterror; }
    x virtual bool IsOk() const { return GetLastError() == wxSTREAM_NO_ERROR; }
    x bool operator!() const { return !IsOk(); }

    // reset the stream state
    void Reset() { m_lasterror = wxSTREAM_NO_ERROR; }

    // this doesn't make sense for all streams, always test its return value
    virtual size_t GetSize() const;
    virtual wxFileOffset GetLength() const { return wxInvalidOffset; }

    // returns true if the streams supports seeking to arbitrary offsets
    virtual bool IsSeekable() const { return false; }

protected:
    virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;

    size_t m_lastcount;
    wxStreamError m_lasterror;

    friend class wxStreamBuffer;

    DECLARE_NO_COPY_CLASS(wxStreamBase)
};
*/
};

class frOutputBlobStream : public wxOutputStream
{
    public:
        frOutputBlobStream(IBPP::Blob blob);
        virtual ~frOutputBlobStream();
        
    //x wxStreamError GetLastError() const { return m_lasterror; }
    //x virtual bool IsOk() const { return GetLastError() == wxSTREAM_NO_ERROR; }
    //x bool operator!() const { return !IsOk(); }

    // reset the stream state
    //void Reset() { m_lasterror = wxSTREAM_NO_ERROR; }

    // this doesn't make sense for all streams, always test its return value
    //virtual size_t GetSize() const;
    //virtual wxFileOffset GetLength() const { return wxInvalidOffset; }

    // returns true if the streams supports seeking to arbitrary offsets
    //virtual bool IsSeekable() const { return false; }
    //protected:
    //virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    //virtual wxFileOffset OnSysTell() const;

    //size_t m_lastcount;
    //wxStreamError m_lasterror;
    
    protected:
        virtual size_t OnSysWrite(const void *buffer, size_t bufsize);
    private:
        IBPP::Blob m_blob;
    /*class WXDLLIMPEXP_BASE wxStreamBase
{
public:
// - = überschrieben   x = nicht überschrieben

    - wxStreamBase();
    - virtual ~wxStreamBase();

    // error testing
    x wxStreamError GetLastError() const { return m_lasterror; }
    x virtual bool IsOk() const { return GetLastError() == wxSTREAM_NO_ERROR; }
    x bool operator!() const { return !IsOk(); }

    // reset the stream state
    void Reset() { m_lasterror = wxSTREAM_NO_ERROR; }

    // this doesn't make sense for all streams, always test its return value
    virtual size_t GetSize() const;
    virtual wxFileOffset GetLength() const { return wxInvalidOffset; }

    // returns true if the streams supports seeking to arbitrary offsets
    virtual bool IsSeekable() const { return false; }

protected:
    virtual wxFileOffset OnSysSeek(wxFileOffset seek, wxSeekMode mode);
    virtual wxFileOffset OnSysTell() const;

    size_t m_lastcount;
    wxStreamError m_lasterror;

    friend class wxStreamBuffer;

    DECLARE_NO_COPY_CLASS(wxStreamBase)
};
*/
};

#endif // FR_EDITBLOBDIALOG_H
