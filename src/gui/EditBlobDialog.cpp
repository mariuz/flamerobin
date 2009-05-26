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


  $Id: EditBlobDialog.cpp 1836 2009-05-10 11:22:13Z amaier $

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

#include <wx/stream.h>
#include <wx/mstream.h>
#include "core/FRError.h"
#include "core/StringUtils.h"
#include "gui/ProgressDialog.h"
#include "gui/EditBlobDialog.h"
#include "gui/StyleGuide.h"

// Static members
/* maybe later needed if plugin will be implemented
int EditBlobDialog::m_libEditBlobUseCount = 0;
wxDynamicLibrary* EditBlobDialog::m_libEditBlob = NULL;
*/

EditBlobDialog::EditBlobDialog(wxWindow* parent, wxString& blobName, IBPP::Blob blob,
    DataGridTable* dgt, unsigned row, unsigned col)
    :BaseDialog(parent, -1, wxEmptyString)
{
    m_running = false; // disable wxNotebookPageChanged-Events
    m_datagridtable = dgt;
    m_blobName = blobName;
    m_row = row;
    m_col = col;
    m_blob = blob;

    notebook      = new wxNotebook(getControlsPanel(), wxID_ANY); 
    blob_text     = new wxStyledTextCtrl(notebook, wxID_ANY);
    blob_binary   = new wxStyledTextCtrl(notebook, wxID_ANY);
    
    button_save   = new wxButton(getControlsPanel(), wxID_SAVE, wxT("Save"));
    button_cancel = new wxButton(getControlsPanel(), wxID_CANCEL, wxT("Cancel"));

    set_properties();
    do_layout();

    m_running = true;  // enable wxNotebookPageChanged-Events
}

bool EditBlobDialog::Init()
{
    // Loading BLOB into Editor
    m_blob->Open();
    frInputBlobStream inpblob(m_blob);
    bool res = LoadFromStreamAsBinary(inpblob,wxT("Loading BLOB into editor."));
    m_editormode = Binary;
    m_blob->Close();
    
    return res;
}

bool EditBlobDialog::LoadFromStreamAsText(wxInputStream& stream, const wxString& progressTitle)
{
    int toread = stream.GetSize();
    ProgressDialog pd(this,_(""));
    pd.initProgress(progressTitle, toread);
    pd.Show();
    blob_text->ClearAll();
    
    // allocate a buffer of the full size that is needed
    // for the text. So we have no troubles with splittet
    // multibyte-chars./amaier 
    //char buffer[toread+1];    
    char *buffer = (char*)malloc(toread+1);
    if (buffer == NULL)
    {        
        wxMessageBox(wxT("Not enough Memory!"),wxT("ERROR"));
        return false;
    }
    
    char* bufptr = buffer;
    int readed = 0;
    // Load text in 32k-Blocks.
    // So we can give the user the ability to cancel.
    while ((!pd.isCanceled()) && (readed < toread))
    {
        int nextread = std::min(32767,toread - readed);
        stream.Read((void*)bufptr, nextread);
        int lastread = stream.LastRead();
        if (lastread < 1)
            break;
        bufptr += lastread;
        readed += lastread;
        pd.stepProgress(lastread);
    }
    buffer[readed] = '\0';
    
    if (!pd.isCanceled())
      blob_text->SetText(std2wx(buffer));
      
    free(buffer);
    pd.Hide();
    
    return !pd.isCanceled();
}

bool EditBlobDialog::LoadFromStreamAsBinary(wxInputStream& stream, const wxString& progressTitle)
{
    int size = stream.GetSize();
    ProgressDialog pd(this,_(""));
    pd.initProgress(progressTitle, size);
    pd.Show();
    
    // set the wxStyledTextControl to ReadOnly = false to modify the text
    blob_binary->SetReadOnly(false);
    blob_binary->ClearAll();
    int col  = 0;
    int line = 0;
    wxString txtLine = wxT("");
    while (!pd.isCanceled())
    {
        char buffer[32768];
        stream.Read((void*)buffer, 32767);
        int size = stream.LastRead();
        if (size < 1)
            break;
        buffer[size] = '\0';
        
        int bufpos = 0;
        while (bufpos < size)
        {
            txtLine += wxString::Format( wxT("%02X"), (unsigned char)(buffer[bufpos]) );
            bufpos++;
            col++;
            
            if ((col % 8) == 0) 
                txtLine += wxT(" ");
            if (col >= 32)
            {
                blob_binary->AddText(txtLine);
                blob_binary->AddText(wxT("\n"));
                txtLine = wxT("");
                col = 0;
                line++;
            }
        }
        pd.stepProgress(size);
    }
    // add the last line if col > 0
    if (!pd.isCanceled())
    {
        if (col > 0)
            blob_binary->AppendText(txtLine);
    }
    blob_binary->SetReadOnly(true);
    pd.Hide();
    
    return !pd.isCanceled();
}

bool EditBlobDialog::SaveToStream(wxOutputStream& stream, const wxString& progressTitle)
{
    ProgressDialog pd(this, progressTitle); //_("Saving Editor-Data"));
    pd.Show();    
    switch (m_editormode)
    {
        case Binary :
            {
                const int maxbufsize = 32768;
                char buffer[maxbufsize]; 
                int bufsize = 0;
                wxString txt = blob_binary->GetText();
                int txtpos = 0;
                while ((txtpos < txt.Length()) && (!pd.isCanceled()))
                {
                    char ch1 = txt.GetChar(txtpos);
                    // skip spaces and cr
                    if ((ch1 == ' ') || (ch1 == 0x0A) || (ch1 == 0x0D))
                    {
                        txtpos++;
                        continue;
                    }
                    char ch2 = txt.GetChar(txtpos+1);
                    
                    int dig1 = 0;
                    int dig2 = 0;
                    
                    if (isdigit(ch1)) dig1 = ch1 - '0';
                    else if ((ch1 >= 'A') && (ch1 <= 'F')) dig1 = ch1 - 'A' + 10;
                    else if ((ch1 >= 'a') && (ch1 <= 'f')) dig1 = ch1 - 'a' + 10;
                    else FRError(wxT("Wrong HEX-value: "+ch1));
                    
                    if (isdigit(ch2)) dig2 = ch2 - '0';
                    else if ((ch2 >= 'A') && (ch2 <= 'F')) dig2 = ch2 - 'A' + 10;
                    else if ((ch2 >= 'a') && (ch2 <= 'f')) dig2 = ch2 - 'a' + 10;
                    else FRError(wxT("Wrong HEX-value: "+ch2));

                    buffer[bufsize] = dig1*16 + dig2;
                    
                    if (bufsize >= maxbufsize-1)
                    {
                        stream.Write(buffer,bufsize);
                        pd.stepProgress(bufsize);
                        bufsize = 0;
                    } 
                    else bufsize++;
                    txtpos += 2;
                };
                if (bufsize > 0) 
                {
                    stream.Write(buffer,bufsize);
                    pd.stepProgress(bufsize);
                }
            }
            break;
        case Text : 
            {
                std::string txt = wx2std(blob_text->GetText());
                stream.Write(txt.c_str(),txt.length());
            }
            break;
        default :
            FRError(wxT("Unknown editormode!"));
    }
    pd.Hide();     

    return !pd.isCanceled();
}

EditBlobDialog::~EditBlobDialog()
{
    /*
    // activate later if plugin will be implemented
    m_libEditBlobUseCount--;
    if (m_libEditBlobUseCount = 0)
    {
        // TODO unload lib
      if (m_libEditBlob)
        delete m_libEditBlob;
    }
    */
}

void EditBlobDialog::set_properties()
{
    SetTitle(wxT("Edit BLOB: "+m_blobName));
    blob_text->SetId(Text);
    blob_binary->SetId(Binary);
}

void EditBlobDialog::do_layout()
{
    wxBoxSizer* sizerControls = new wxBoxSizer(wxVERTICAL);
    // *** TEXT-EDIT-LAYOUT ***
    blob_text->SetSizeHints(200, 100);
    blob_text->SetSize(200, 100);
    // fixed-width font (i think it is better for showing)
    wxFont ftxt(styleguide().getEditorFontSize(),
        wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
        false);
    blob_text->StyleSetFont(0,ftxt);   
    // numbering on text should be usefull
    blob_text->SetMarginType(0,wxSTC_MARGIN_NUMBER);
    blob_text->SetMarginWidth(0,30);
    blob_text->SetMarginWidth(1,0);
    blob_text->SetMarginWidth(2,0);

    // *** BINARY-SHOW-LAYOUT ***
    blob_binary->SetSizeHints(200, 100);
    blob_binary->SetSize(200, 100);
    blob_binary->SetReadOnly(true);
    // fixed-width font
    wxFont fbin(styleguide().getEditorFontSize(),
        wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
        false);
    blob_binary->StyleSetFont(0,fbin);   
    // set with of the viewport to avoid "empty" space after each line
    blob_binary->SetScrollWidth(600);
    // set up a left-margin for numbering lines
    // TODO - calc with of dialog to fit excact a binary line without scrolling"
    blob_binary->SetMarginType(0,wxSTC_MARGIN_NUMBER);
    blob_binary->SetMarginWidth(0,30);
    blob_binary->SetMarginWidth(1,0);
    blob_binary->SetMarginWidth(2,0);
    
    notebook->AddPage(blob_binary, wxT("Binary"), false);
    notebook->AddPage(blob_text,   wxT("Text"),   false);
    
    sizerControls->Add(notebook, 1, wxEXPAND);

    wxSizer* sizerButtons = styleguide().createButtonSizer(button_save, button_cancel);

    layoutSizers(sizerControls, sizerButtons, true);

    SetSize(620, 400);
    Centre();
}

void EditBlobDialog::OnClose(wxCloseEvent& event)
{
    // make sure parent window is properly activated again
    wxWindow* p = GetParent();
    if (p)
    {
        p->Enable();
        Hide();
        p->Raise();
    }
    Destroy();
}

void EditBlobDialog::OnSaveButtonClick(wxCommandEvent& event)
{
    // Save Editor-Data into BLOB
    DataGridRowsBlob b = m_datagridtable->setBlobPrepare(m_row,m_col);
    b.blob->Create();
    frOutputBlobStream bs(b.blob);
    SaveToStream(bs,wxT("Saving Data to BLOB"));
    b.blob->Close();
    
    m_datagridtable->setBlob(b);
    
    m_running = false;
    
    // Close window
    Close();
}

void EditBlobDialog::OnCancelButtonClick(wxCommandEvent& event)
{
    Close();
}

void EditBlobDialog::OnNotebookPageChanged(wxNotebookEvent& event)
{
    if (!m_running) 
        return;        
    int page = event.GetSelection();
    int oldpage = event.GetOldSelection();
    if ((page < 0) || (oldpage < 0))
        return;
        
       
    int pageId = notebook->GetPage(page)->GetId();
    
    wxMemoryOutputStream outBuf(NULL,0);
    if (!SaveToStream(outBuf,wxT("Switching editor-mode (Saving)")))
    {
        wxMessageBox(wxT("A error occured while switching editor-mode. (Saving)"));
        //event.Veto();
        notebook->ChangeSelection(oldpage);
        return;
    }

    bool loadOk = false;
    wxString loadTitle = wxT("Switching editor-mode (Loading)");
    wxMemoryInputStream inBuf(outBuf);
    switch (pageId)
    {
        case Binary :
            loadOk = LoadFromStreamAsBinary(inBuf,loadTitle);
            break;
        case Text : 
            loadOk = LoadFromStreamAsText(inBuf,loadTitle);
            break;
    }
    if (!loadOk)
    {
        wxMessageBox(wxT("A error occured while switching editor-mode. (Loading)"));
        notebook->ChangeSelection(oldpage);
        //event.Veto();
        return;
    }
    m_editormode = (EditorMode)pageId;
}

//! event handling
BEGIN_EVENT_TABLE(EditBlobDialog, BaseDialog)
    //EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, EditBlobDialog::OnNotebookPageChanged)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, EditBlobDialog::OnNotebookPageChanged)
    EVT_BUTTON(wxID_SAVE,   EditBlobDialog::OnSaveButtonClick)
    EVT_BUTTON(wxID_CANCEL, EditBlobDialog::OnCancelButtonClick)
    EVT_CLOSE(EditBlobDialog::OnClose)
END_EVENT_TABLE()


// Helper-Class for streaming into blob / buffer
// frInputBlobStream
frInputBlobStream::frInputBlobStream(IBPP::Blob blob)
    :wxInputStream()
{
    m_blob = blob;
    blob->Info(&m_size, 0, 0);
}

frInputBlobStream::~frInputBlobStream()
{
}

size_t frInputBlobStream::OnSysRead(void *buffer, size_t size)
{
    return m_blob->Read(buffer,size);
}

size_t frInputBlobStream::GetSize() const
{
    return m_size;
}

// Helper-Class for streaming into blob / buffer
// frOutputBlobStream
frOutputBlobStream::frOutputBlobStream(IBPP::Blob blob)
    :wxOutputStream()
{
    m_blob = blob;
}

frOutputBlobStream::~frOutputBlobStream()
{
}

size_t frOutputBlobStream::OnSysWrite(const void *buffer, size_t bufsize)
{
    if (bufsize == 0)
        return 0;
        
    m_blob->Write(buffer,bufsize);
    return bufsize;
}

// Helper-Class 
/*int wxFRNotebook::SetSelection(size_t page)
{
    m_newSelection = page;
    wxString x = wxString::Format("%i",page);
    wxMessageBox(x);
    wxNotebook::SetSelection(page);
}*/

/*int wxFRNotebook::ChangeSelection(size_t page)
{
    m_newSelection = page;
    wxString x = wxString::Format("%i",page);
    wxMessageBox(wxT("ChangeSelection"));
    wxNotebook::ChangeSelection(page);
}*/

